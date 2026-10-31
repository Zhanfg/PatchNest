#!/bin/bash
# PatchNest reproducible local build entry point.
set -euo pipefail

if [[ "${1:-}" == "clean" ]]; then
    rm -rf out module/bin module/webroot
    exit 0
fi

mkdir -p out module/bin module/webroot

# Build WebUI.
cd webui
pnpm build || { pnpm install --frozen-lockfile && pnpm build; }
cd ..

# Read versions and digests from version.properties using literal keys.
get_ver() {
    local key="$1"
    [ -f version.properties ] || return 1
    grep -F "${key}=" version.properties \
        | head -n 1 \
        | cut -d= -f2- \
        | xargs \
        | sed 's/^"//;s/"$//'
}

asset_digest_key() {
    local asset_name="$1"
    local tag="$2"
    case "$asset_name" in
        Magisk-*.apk) printf 'magisk_apk_%s\n' "$tag" ;;
        *) printf '%s_%s\n' "${asset_name//[-.]/_}" "$tag" ;;
    esac
}

download_assets() {
    local repo="$1"
    local tag="$2"
    local outdir="$3"
    shift 3
    local patterns=("$@")

    local url="https://api.github.com/repos/$repo/releases"
    if [[ "$tag" == "latest" ]]; then
        url="$url/latest"
    else
        url="$url/tags/$tag"
    fi

    local release_json
    release_json=$(curl -fsSL "$url")

    for pattern in "${patterns[@]}"; do
        local regex="${pattern//\*/.*}"
        local asset_data
        asset_data=$(printf '%s' "$release_json" | jq -r ".assets[] | select(.name | test(\"$regex\")) | .name + \"\t\" + .browser_download_url" | head -n 1)
        if [[ -z "$asset_data" ]]; then
            echo "ERROR: could not find asset matching '$pattern' in $repo release $tag" >&2
            exit 1
        fi

        local asset_name download_url
        asset_name=$(printf '%s' "$asset_data" | cut -f1)
        download_url=$(printf '%s' "$asset_data" | cut -f2)
        echo "Downloading $asset_name from $download_url"
        curl -fsSL "$download_url" -o "$outdir/$asset_name"
        if [[ ! -s "$outdir/$asset_name" ]]; then
            echo "ERROR: downloaded asset is empty: $asset_name" >&2
            exit 1
        fi

        local key expected
        key=$(asset_digest_key "$asset_name" "$tag")
        expected=$(get_ver "$key" || true)
        if [[ -z "$expected" ]]; then
            echo "ERROR: no trusted SHA256 pinned for $key" >&2
            exit 1
        fi
        echo "$expected  $outdir/$asset_name" | sha256sum -c - \
            || { echo "ERROR: SHA256 mismatch for $asset_name" >&2; exit 1; }
    done
}

VERSION_KERNELPATCH=$(get_ver "kernelpatch")
VERSION_KERNELPATCH="${VERSION_KERNELPATCH:-latest}"
VERSION_PATCHNEST=$(get_ver "patchnest")
VERSION_PATCHNEST="${VERSION_PATCHNEST:-latest}"
VERSION_MAGISKBOOT=$(get_ver "magiskboot")
VERSION_MAGISKBOOT="${VERSION_MAGISKBOOT:-latest}"

# Fetch KernelPatch binaries from the public core repository.
if [[ ! -f "module/bin/kpimg" || ! -f "module/bin/kptools" ]]; then
    download_assets "Zhanfg/KernelPatch-Public" "$VERSION_KERNELPATCH" "module/bin" "kpimg-linux" "kptools-android"
    mv module/bin/kpimg-linux module/bin/kpimg
    mv module/bin/kptools-android module/bin/kptools
fi

# Build the KPM manager CLI from this exact source revision.
# A pinned 0.13.5-2 binary predates loadv/ctlv, so bundling that binary
# alongside the new WebUI would produce a broken feature despite green UI tests.
# Do not silently ship a stale binary when NDK is unavailable.
if [[ -z "${ANDROID_NDK_HOME:-}" ]]; then
    echo "ERROR: ANDROID_NDK_HOME is required to build the matching PatchNest CLI" >&2
    exit 1
fi
TOOLCHAIN="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake"
if [[ ! -f "$TOOLCHAIN" ]]; then
    echo "ERROR: Android NDK CMake toolchain not found: $TOOLCHAIN" >&2
    exit 1
fi
cmake -S ../cli -B ../cli/build/module-arm64 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DCMAKE_BUILD_TYPE=Release \
    -DANDROID_PLATFORM=android-33 \
    -DANDROID_ABI=arm64-v8a
cmake --build ../cli/build/module-arm64 --target kpatch --parallel
install -m 0755 ../cli/build/module-arm64/kpatch module/bin/kpatch
if command -v strings >/dev/null 2>&1; then
    strings module/bin/kpatch | grep -Fx loadv >/dev/null || {
        echo "ERROR: newly built CLI lacks loadv command" >&2; exit 1;
    }
    strings module/bin/kpatch | grep -Fx ctlv >/dev/null || {
        echo "ERROR: newly built CLI lacks ctlv command" >&2; exit 1;
    }
fi
echo "Bundled kpatch CLI from current source revision $(git rev-parse --short HEAD)"

# Fetch and extract magiskboot from the pinned official Magisk APK.
if [[ ! -f "module/bin/magiskboot" ]]; then
    download_assets "topjohnwu/Magisk" "$VERSION_MAGISKBOOT" "module/bin" "Magisk*.apk"

    APK=$(printf '%s\n' module/bin/Magisk*.apk 2>/dev/null | head -n 1)
    if [[ ! -f "$APK" ]]; then
        echo "ERROR: no Magisk APK downloaded" >&2
        exit 1
    fi
    if ! unzip -p "$APK" 'lib/arm64-v8a/libmagiskboot.so' > "module/bin/magiskboot" 2>/dev/null; then
        echo "ERROR: lib/arm64-v8a/libmagiskboot.so not found inside $APK" >&2
        exit 1
    fi
    if [[ ! -s "module/bin/magiskboot" ]]; then
        echo "ERROR: extracted magiskboot is empty" >&2
        exit 1
    fi
    rm "$APK"
fi

# Build kp-safemode when an Android NDK is available locally.
if [[ ! -f "module/bin/kp-safemode" && -n "${ANDROID_NDK_HOME:-}" ]]; then
    echo "Building kp-safemode with NDK clang..."
    NDK_CLANG="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android24-clang"
    if [[ ! -x "$NDK_CLANG" ]]; then
        NDK_CLANG="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/clang"
    fi
    if [[ -x "$NDK_CLANG" ]]; then
        "$NDK_CLANG" -static -O2 -Wall -Wextra -o module/bin/kp-safemode module/tools/kp-safemode.c
        chmod +x module/bin/kp-safemode
        echo "✓ kp-safemode built"
    else
        echo "⚠ NDK clang not found at $NDK_CLANG; skipping kp-safemode"
    fi
fi

# KPM sources and catalog artifacts live in the canonical monorepo at kpms/.
# PatchNest Module consumes the generated catalog at runtime and does not
# bundle catalog KPM binaries into the module archive.

commit_number=$(git rev-list --count HEAD)
commit_hash=$(git rev-parse --short HEAD)

cd module
zip -r "../out/PatchNest-${commit_number}-${commit_hash}.zip" .
cd ..
