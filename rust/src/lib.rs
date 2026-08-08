#![forbid(unsafe_code)]

pub mod probe;

/// Stable process exit categories shared with the hardened C CLI.
#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CliExit {
    Ok = 0,
    Usage = 2,
    Permission = 3,
    Unsupported = 4,
    NotFound = 5,
    Io = 6,
    Kernel = 8,
}

impl CliExit {
    pub const fn from_errno(errno: i32) -> Self {
        match errno {
            1 | 13 => Self::Permission,   // EPERM | EACCES
            38 | 95 => Self::Unsupported, // ENOSYS | EOPNOTSUPP
            2 => Self::NotFound,          // ENOENT
            5 | 12 | 14 => Self::Io,      // EIO | ENOMEM | EFAULT
            _ => Self::Kernel,
        }
    }
}

/// Explicit ABI families. They must never be mixed implicitly inside one
/// binary/package. The syscall backend will select exactly one profile before
/// issuing a state-changing call.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AbiProfile {
    /// Zhanfg/KernelPatch-Public 0.13.x family.
    Public1158,
    /// KernelSU-Next/KPatch-Next family used by the current C PatchNest CLI.
    Next2026,
}

impl AbiProfile {
    pub const fn token(self) -> u16 {
        match self {
            Self::Public1158 => 0x1158,
            Self::Next2026 => 0x2026,
        }
    }

    pub const fn hello_magic(self) -> u32 {
        match self {
            Self::Public1158 => 0x1158_1158,
            Self::Next2026 => 0x2026_2026,
        }
    }

    pub const fn hello_echo(self) -> &'static str {
        match self {
            Self::Public1158 => "hello1158",
            Self::Next2026 => "hello2026",
        }
    }

    pub const fn requires_key(self) -> bool {
        matches!(self, Self::Public1158)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum KpmCommand {
    Load { path: String, args: Option<String> },
    Control { name: String, args: String },
    Unload { name: String },
    Num,
    List,
    Info { name: String },
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Command {
    Help,
    Version,
    Hello,
    KpVersion,
    KernelVersion,
    Kpm(KpmCommand),
    ExcludeSet { uid: u32, excluded: bool },
    ExcludeGet { uid: u32 },
    Rehook { enabled: bool },
    RehookStatus,
    Bootlog,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ParseError(pub &'static str);

fn parse_uid(text: &str) -> Result<u32, ParseError> {
    if text.is_empty() || !text.bytes().all(|b| b.is_ascii_digit()) {
        return Err(ParseError("UID must contain decimal digits only"));
    }
    if text != "0" && text.starts_with('0') {
        return Err(ParseError(
            "UID zero and leading-zero aliases are not canonical",
        ));
    }
    text.parse::<u32>()
        .map_err(|_| ParseError("UID is outside the supported range"))
}

fn parse_bool01(text: &str) -> Result<bool, ParseError> {
    match text {
        "0" => Ok(false),
        "1" => Ok(true),
        _ => Err(ParseError("value must be exactly 0 or 1")),
    }
}

fn parse_kpm(args: &[String]) -> Result<Command, ParseError> {
    let Some(sub) = args.first().map(String::as_str) else {
        return Err(ParseError("missing kpm subcommand"));
    };

    let command = match sub {
        "load" => {
            let path = args.get(1).ok_or(ParseError("missing KPM path"))?.clone();
            let tail = &args[2..];
            let kpm_args = match tail {
                [] => None,
                [single] => Some(single.clone()),
                [marker, single] if marker == "--" => Some(single.clone()),
                _ => return Err(ParseError("kpm load accepts PATH [ARGS] or PATH -- ARGS")),
            };
            KpmCommand::Load {
                path,
                args: kpm_args,
            }
        }
        "ctl0" => {
            if args.len() != 3 {
                return Err(ParseError("kpm ctl0 requires NAME and ARGS"));
            }
            KpmCommand::Control {
                name: args[1].clone(),
                args: args[2].clone(),
            }
        }
        "unload" => {
            if args.len() != 2 {
                return Err(ParseError("kpm unload requires NAME"));
            }
            KpmCommand::Unload {
                name: args[1].clone(),
            }
        }
        "num" if args.len() == 1 => KpmCommand::Num,
        "list" if args.len() == 1 => KpmCommand::List,
        "info" if args.len() == 2 => KpmCommand::Info {
            name: args[1].clone(),
        },
        _ => return Err(ParseError("invalid kpm command or argument count")),
    };

    Ok(Command::Kpm(command))
}

pub fn parse_command(args: &[String]) -> Result<Command, ParseError> {
    let Some(command) = args.first().map(String::as_str) else {
        return Err(ParseError("missing command"));
    };

    match command {
        "-h" | "--help" | "help" if args.len() == 1 => Ok(Command::Help),
        "-v" | "--version" if args.len() == 1 => Ok(Command::Version),
        "hello" if args.len() == 1 => Ok(Command::Hello),
        "kpver" if args.len() == 1 => Ok(Command::KpVersion),
        "kver" if args.len() == 1 => Ok(Command::KernelVersion),
        "bootlog" if args.len() == 1 => Ok(Command::Bootlog),
        "rehook_status" if args.len() == 1 => Ok(Command::RehookStatus),
        "rehook" if args.len() == 2 => match args[1].as_str() {
            "enable" => Ok(Command::Rehook { enabled: true }),
            "disable" => Ok(Command::Rehook { enabled: false }),
            _ => Err(ParseError("rehook accepts enable or disable")),
        },
        "exclude_get" if args.len() == 2 => Ok(Command::ExcludeGet {
            uid: parse_uid(&args[1])?,
        }),
        "exclude_set" if args.len() == 3 => Ok(Command::ExcludeSet {
            uid: parse_uid(&args[1])?,
            excluded: parse_bool01(&args[2])?,
        }),
        "kpm" => parse_kpm(&args[1..]),
        _ => Err(ParseError("invalid command or argument count")),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn av(values: &[&str]) -> Vec<String> {
        values.iter().map(|value| (*value).to_owned()).collect()
    }

    #[test]
    fn abi_profiles_are_not_interchangeable() {
        assert_eq!(AbiProfile::Public1158.token(), 0x1158);
        assert_eq!(AbiProfile::Next2026.token(), 0x2026);
        assert_ne!(
            AbiProfile::Public1158.hello_magic(),
            AbiProfile::Next2026.hello_magic()
        );
        assert!(AbiProfile::Public1158.requires_key());
        assert!(!AbiProfile::Next2026.requires_key());
    }

    #[test]
    fn stable_exit_mapping_matches_c_contract() {
        assert_eq!(CliExit::from_errno(1), CliExit::Permission);
        assert_eq!(CliExit::from_errno(13), CliExit::Permission);
        assert_eq!(CliExit::from_errno(38), CliExit::Unsupported);
        assert_eq!(CliExit::from_errno(95), CliExit::Unsupported);
        assert_eq!(CliExit::from_errno(2), CliExit::NotFound);
        assert_eq!(CliExit::from_errno(14), CliExit::Io);
        assert_eq!(CliExit::from_errno(22), CliExit::Kernel);
    }

    #[test]
    fn uid_zero_must_be_canonical() {
        assert!(parse_command(&av(&["exclude_get", "0"])).is_ok());
        assert!(parse_command(&av(&["exclude_get", "00"])).is_err());
        assert!(parse_command(&av(&["exclude_get", "-1"])).is_err());
        assert!(parse_command(&av(&["exclude_get", "123junk"])).is_err());
    }

    #[test]
    fn kpm_load_supports_current_and_legacy_module_call_shapes() {
        assert_eq!(
            parse_command(&av(&["kpm", "load", "/x/a.kpm", "mode=1"])).unwrap(),
            Command::Kpm(KpmCommand::Load {
                path: "/x/a.kpm".into(),
                args: Some("mode=1".into()),
            })
        );
        assert_eq!(
            parse_command(&av(&["kpm", "load", "/x/a.kpm", "--", "mode=1"])).unwrap(),
            Command::Kpm(KpmCommand::Load {
                path: "/x/a.kpm".into(),
                args: Some("mode=1".into()),
            })
        );
    }

    #[test]
    fn extra_kpm_arguments_fail_closed() {
        assert!(parse_command(&av(&["kpm", "load", "/x/a.kpm", "a", "b"])).is_err());
        assert!(parse_command(&av(&["kpm", "num", "unexpected"])).is_err());
    }
}
