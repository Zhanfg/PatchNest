use crate::AbiProfile;

/// Capability names are explicit because command numbers are not globally
/// interchangeable between KernelPatch families.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Capability {
    Kpm,
    Exclude,
    Rehook,
    Event,
}

impl AbiProfile {
    pub const fn supports(self, capability: Capability) -> bool {
        match (self, capability) {
            (Self::Public1158, Capability::Kpm | Capability::Exclude | Capability::Event) => true,
            (Self::Public1158, Capability::Rehook) => false,
            (Self::Next2026, Capability::Kpm | Capability::Exclude | Capability::Rehook) => true,
            (Self::Next2026, Capability::Event) => false,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ProbeFailure {
    /// The candidate ABI rejected an unauthenticated read-only hello.
    Permission,
    Unsupported,
    Io,
    Other(i32),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ProbeError {
    /// Public1158 may require a device superkey before even its hello reaches
    /// the handler. The caller can retry after loading the protected key.
    KeyRequired,
    /// A successful syscall returned a hello value belonging to no reviewed
    /// PatchNest ABI profile.
    UnknownMagic(u32),
    /// Neither reviewed profile produced a trustworthy read-only result.
    Unresolved(ProbeFailure),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ProbeResult {
    pub profile: AbiProfile,
    pub authenticated: bool,
}

/// Narrow backend boundary for the future Android syscall adapter. The probe
/// operation must be read-only. No state-changing method belongs to this trait.
pub trait ReadOnlyProbeBackend {
    fn hello(
        &self,
        profile: AbiProfile,
        key: Option<&str>,
    ) -> Result<u32, ProbeFailure>;
}

fn classify_magic(magic: u32, authenticated: bool) -> Result<ProbeResult, ProbeError> {
    if magic == AbiProfile::Next2026.hello_magic() {
        return Ok(ProbeResult {
            profile: AbiProfile::Next2026,
            authenticated,
        });
    }
    if magic == AbiProfile::Public1158.hello_magic() {
        return Ok(ProbeResult {
            profile: AbiProfile::Public1158,
            authenticated,
        });
    }
    Err(ProbeError::UnknownMagic(magic))
}

/// Probe without performing any mutation.
///
/// Ordering is intentional:
/// 1. try the keyless Next2026 hello;
/// 2. if permission is denied, require a protected key before attempting the
///    Public1158 profile;
/// 3. never fall through from an unknown successful magic to another profile.
///
/// This prevents a future backend from "trying commands until something
/// works", which would be unsafe because 0x1100/0x1101 have conflicting
/// meanings across the two families.
pub fn probe_profile<B: ReadOnlyProbeBackend>(
    backend: &B,
    public_key: Option<&str>,
) -> Result<ProbeResult, ProbeError> {
    match backend.hello(AbiProfile::Next2026, None) {
        Ok(magic) => return classify_magic(magic, false),
        Err(ProbeFailure::Permission) => {}
        Err(other) => return Err(ProbeError::Unresolved(other)),
    }

    let key = public_key.filter(|key| !key.is_empty()).ok_or(ProbeError::KeyRequired)?;
    match backend.hello(AbiProfile::Public1158, Some(key)) {
        Ok(magic) => classify_magic(magic, true),
        Err(other) => Err(ProbeError::Unresolved(other)),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::cell::RefCell;

    struct ScriptedBackend {
        calls: RefCell<Vec<(AbiProfile, Option<String>)>>,
        next: Result<u32, ProbeFailure>,
        public: Result<u32, ProbeFailure>,
    }

    impl ReadOnlyProbeBackend for ScriptedBackend {
        fn hello(
            &self,
            profile: AbiProfile,
            key: Option<&str>,
        ) -> Result<u32, ProbeFailure> {
            self.calls
                .borrow_mut()
                .push((profile, key.map(ToOwned::to_owned)));
            match profile {
                AbiProfile::Next2026 => self.next,
                AbiProfile::Public1158 => self.public,
            }
        }
    }

    #[test]
    fn next_profile_stops_after_first_read_only_hello() {
        let backend = ScriptedBackend {
            calls: RefCell::new(Vec::new()),
            next: Ok(0x2026_2026),
            public: Ok(0x1158_1158),
        };
        let result = probe_profile(&backend, Some("unused")).unwrap();
        assert_eq!(result.profile, AbiProfile::Next2026);
        assert!(!result.authenticated);
        assert_eq!(backend.calls.borrow().len(), 1);
    }

    #[test]
    fn public_profile_requires_key_after_permission_denial() {
        let backend = ScriptedBackend {
            calls: RefCell::new(Vec::new()),
            next: Err(ProbeFailure::Permission),
            public: Ok(0x1158_1158),
        };
        assert_eq!(probe_profile(&backend, None), Err(ProbeError::KeyRequired));
        assert_eq!(backend.calls.borrow().len(), 1);

        let result = probe_profile(&backend, Some("device-key")).unwrap();
        assert_eq!(result.profile, AbiProfile::Public1158);
        assert!(result.authenticated);
        let calls = backend.calls.borrow();
        assert_eq!(calls[calls.len() - 1].1.as_deref(), Some("device-key"));
    }

    #[test]
    fn unknown_success_never_falls_through() {
        let backend = ScriptedBackend {
            calls: RefCell::new(Vec::new()),
            next: Ok(0xdead_beef),
            public: Ok(0x1158_1158),
        };
        assert_eq!(
            probe_profile(&backend, Some("device-key")),
            Err(ProbeError::UnknownMagic(0xdead_beef))
        );
        assert_eq!(backend.calls.borrow().len(), 1);
    }

    #[test]
    fn public_rehook_is_structurally_forbidden() {
        assert!(AbiProfile::Public1158.supports(Capability::Kpm));
        assert!(AbiProfile::Public1158.supports(Capability::Exclude));
        assert!(AbiProfile::Public1158.supports(Capability::Event));
        assert!(!AbiProfile::Public1158.supports(Capability::Rehook));
        assert!(AbiProfile::Next2026.supports(Capability::Rehook));
        assert!(!AbiProfile::Next2026.supports(Capability::Event));
    }
}
