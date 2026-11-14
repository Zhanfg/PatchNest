# PatchNest Rust refactor foundation

This directory is an isolated refactor track. It does **not** replace the reviewed C release path yet.

## Phase 0 scope

Implemented now:

- stable CLI exit-code contract matching the hardened C CLI;
- strict command/UID parsing;
- compatibility parsing for both `kpm load PATH ARGS` and the historical module call shape `kpm load PATH -- ARGS`;
- explicit ABI profiles instead of one implicit supercall family:
  - `Public1158` for `Zhanfg/KernelPatch-Public`;
  - `Next2026` for `KernelSU-Next/KPatch-Next`;
- unit tests proving the two ABI families have different handshake magic/tokens and must not be silently mixed.

Not implemented yet:

- raw Android supercall backend;
- superkey acquisition/authentication;
- automatic ABI probing;
- state-changing kernel operations;
- replacement of `kpatch-android` in PatchNest-Module.

The `patchnest-rs` executable therefore returns the stable `unsupported` status for kernel-touching commands. This is deliberate: the Rust branch must never be mistaken for a flash-ready replacement before integration/device validation exists.

## Refactor rule

The migration sequence is:

1. reproduce the C parser/exit contract in safe Rust;
2. introduce an ABI profile layer;
3. add read-only handshake/version probes for one profile at a time;
4. add fixture tests against captured syscall behavior;
5. add Android arm64 builds and compare behavior with the C CLI;
6. add state-changing KPM/exclude/rehook operations only after read-only parity passes;
7. switch PatchNest-Module to Rust only after package-level and physical-device rollback tests pass.

No C production path is removed during these phases.
