# PatchNest CLI

PatchNest CLI is the standalone Android `kpatch` user-space client consumed by [`PatchNest-Module`](https://github.com/Zhanfg/PatchNest-Module).

## Source provenance

The current source snapshot is imported from:

- Upstream repository: `KernelSU-Next/KPatch-Next`
- Upstream commit: `0fe6d142266b80e5aa445a7ea1534f88a8f33a35`
- Upstream directory: `user/`
- Version: `0.13.5-2`

`SOURCE_IMPORT_MANIFEST.json` is the machine-readable source record. CI checks the local and pinned upstream `.c`/`.h` file lists in both directions, compares every imported file, and verifies the generated or referenced build inputs:

- `banner`
- `src/uapi/scdefs.h`
- `version`
- `revision`
- `LICENSE`

## Legacy release history

The historical tag `0.13.5-2` points to commit:

```text
d8152d4afdafa7f4f58e25223d2ca8e1e9b131cd
```

That commit predates the source restoration and contains only the initial repository shell. The `kpatch-android` asset attached to the release is retained as an immutable legacy artifact; the tag must not be presented as a source-bearing release.

The complete CLI source was restored later in commit:

```text
c04610d663b3257216fc91221754494e4193aa1f
```

Despite the historical tag defect, CI reproduces the existing release asset byte-for-byte from the pinned upstream snapshot. Its SHA-256 is:

```text
d6a654816f11c8d297ca59aaace9c61537238f26191738c14610d2dcf39bf3b0
```

Future releases must use a new tag that points to a commit containing the complete corresponding source. The legacy tag and asset are never overwritten.

## Build

Requirements:

- CMake 3.10 or later
- Ninja
- Android NDK r26b

```bash
cmake -S . -B build/android \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DANDROID_PLATFORM=android-33 \
  -DANDROID_ABI=arm64-v8a

cmake --build build/android --target kpatch --parallel
cp build/android/kpatch out/kpatch-android
```

CI performs the following checks:

1. the legacy tag still points to its recorded commit;
2. the source snapshot matches the pinned upstream commit in both directions;
3. strict format, declaration, pointer-type, and return-type warning gates pass;
4. two independent build directories produce byte-identical binaries;
5. the output is an Android ARM64 PIE using `/system/bin/linker64`;
6. the output matches the immutable `0.13.5-2` release asset byte-for-byte;
7. a machine-readable `build-provenance.json` is uploaded with the verification artifact.

## Repository responsibility

This repository contains only the user-space CLI. It does not contain:

- KernelPatch core or `kpimg`
- `kptools`
- PatchNest module installer or WebUI
- KPM catalog sources

Those components remain in their dedicated repositories.

## License

GPL-2.0. See [`LICENSE`](LICENSE) and the retained upstream copyright notices.
