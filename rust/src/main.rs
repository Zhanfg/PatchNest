use std::env;
use std::process::ExitCode;

use patchnest_cli::{parse_command, CliExit, Command};

fn exit(code: CliExit) -> ExitCode {
    ExitCode::from(code as u8)
}

fn main() -> ExitCode {
    let args: Vec<String> = env::args().skip(1).collect();
    let command = match parse_command(&args) {
        Ok(command) => command,
        Err(error) => {
            eprintln!("{}", error.0);
            return exit(CliExit::Usage);
        }
    };

    match command {
        Command::Help => {
            println!("PatchNest Rust refactor foundation");
            println!("This branch implements parsing and ABI contracts only; kernel supercalls remain disabled.");
            exit(CliExit::Ok)
        }
        Command::Version => {
            println!("rust-foundation-0.1.0");
            exit(CliExit::Ok)
        }
        _ => {
            eprintln!(
                "Rust supercall backend is intentionally disabled in the foundation phase; use the reviewed C CLI for production."
            );
            exit(CliExit::Unsupported)
        }
    }
}
