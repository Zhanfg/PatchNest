# PatchNest CLI

PatchNest CLI is the standalone Android `kpatch` user-space client consumed by [`PatchNest-Module`](https://github.com/Zhanfg/PatchNest-Module).

## Source provenance

The current source snapshot is imported from:

- Upstream repository: `KernelSU-Next/KPatch-Next`
- Upstream commit: `0fe6d142266b80e5aa445a7ea1534f88a8f33a35`
- Upstream directory: `user/`
- Version: `0.13.5-2`

`SOURCE_IMPORT_MANIFEST.json` is the machine-readable source record. CI clones that exact upstream commit and compares every imported `.c` and `.h` file before building.

The standalone repository also carries the two build inputs generated or referenced by the upstream full-tree build:

- `banner`
- `src/uapi/scdefs.h`

## Build

Requirements:

- CMake
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

CI verifies that the result is an ARM64 Android ELF and compares it byte-for-byte with the existing `0.13.5-2` Release asset.

## Repository responsibility

This repository contains only the user-space CLI. It does not contain:

- KernelPatch core or `kpimg`
- `kptools`
- PatchNest module installer or WebUI
- KPM catalog sources

Those components remain in their dedicated repositories.

## License

GPL-2.0. See [`LICENSE`](LICENSE) and the retained upstream copyright notices.
