# PatchNest Diagnostic Hello

This KPM is a non-invasive build and lifecycle diagnostic. It exists to prove that the repository can compile a KPM from reviewed source against a pinned KernelPatch SDK and produce auditable provenance.

## Behavior

- logs a message when loaded;
- returns the constant string `patchnest-diagnostic-ok` through control channel 0;
- logs a message when unloaded.

It does **not** hook kernel functions, alter SELinux or mount state, hide processes/modules/files, spoof boot state, or install persistence.

## Build status

The artifact is build-only and is not listed in `kpm_repo.json`. CI uploads it as a temporary workflow artifact together with ELF inspection output and `build-provenance.json`.

Moving it into the installable catalog requires physical validation of:

1. load;
2. control channel response;
3. unload;
4. repeated load/unload;
5. reboot behavior;
6. failure recovery.

## Provenance

The module interface is derived from the GPL-2.0-or-later `demo-hello` example in `KernelSU-Next/KPatch-Next`, pinned to commit `0fe6d142266b80e5aa445a7ea1534f88a8f33a35`. The implementation narrows the example to a constant bounded response and checks user-copy failures.
