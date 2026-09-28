# Upstream baseline

PatchNest CLI source is imported from:

- Repository: `KernelSU-Next/KPatch-Next`
- Commit: `0fe6d142266b80e5aa445a7ea1534f88a8f33a35`
- Directory: `user/`
- Upstream version: `0.13.5-2`

Only the standalone user-space CLI sources are imported. Kernel and patching-tool sources remain in their dedicated repositories.

## Historical release boundary

The local tag `0.13.5-2` points to the initial repository commit:

```text
d8152d4afdafa7f4f58e25223d2ca8e1e9b131cd
```

That tagged commit does not contain the imported CLI source. It is retained only as an immutable record of the already-published release. The current source restoration begins at:

```text
c04610d663b3257216fc91221754494e4193aa1f
```

CI proves that the pinned upstream snapshot reproduces the historical `kpatch-android` asset byte-for-byte, but this does not retroactively make the legacy tag source-bearing.

## Synchronisation rule

An upstream update must be made on a dedicated branch and must update all of the following together:

1. `SOURCE_IMPORT_MANIFEST.json`;
2. imported `src/*.c` and `src/*.h` files;
3. `src/uapi/scdefs.h` and other recorded build inputs when changed;
4. the declared version and revision files;
5. the compatibility and release notes;
6. the expected artifact digest only after a new, source-bearing tag is created.

Automatic merging from upstream is not allowed. Each update requires source review, deterministic-build verification, and PatchNest-Module compatibility testing.
