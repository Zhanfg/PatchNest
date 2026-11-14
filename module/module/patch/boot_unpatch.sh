#!/system/bin/sh
#######################################################################################
# APatch Boot Image Unpatcher
# Imported from https://github.com/bmax121/APatch/blob/main/app/src/main/assets/boot_unpatch.sh
#######################################################################################

MODPATH=${0%/*}
ARCH=$(getprop ro.product.cpu.abi)
PNDIR="/data/adb/patchnest"
BACKUP_DIR="$PNDIR/backup"
AUTORECOVERY_MARKER="$PNDIR/autorecovery_active"

# Load utility functions
. "$MODPATH/util_functions.sh"

BOOTIMAGE=$1

# Fail closed on unknown backup provenance or an A/B slot mismatch.
# PN_FLASH_RECOVERY=1 explicitly authorizes recovery from a verified backup.
# Default mode only exports a freshly generated unpatched candidate.
. "$MODPATH/backup_guard.sh" || exit 1

auto_unpatch() {
    [ -n "$BOOTIMAGE" ] && [ -e "$BOOTIMAGE" ] || {
        >&2 echo "! recovery target missing"
        return 1
    }
    if [ ! -b "$BOOTIMAGE" ] && [ ! -c "$BOOTIMAGE" ]; then
        >&2 echo "! recovery flash requires a real block device"
        return 1
    fi

    local target_slot chosen tmpdir target_resolved suffix byname
    target_slot=unknown
    target_resolved=$(readlink -f "$BOOTIMAGE" 2>/dev/null) || return 1
    for suffix in _a _b; do
        byname="/dev/block/by-name/boot$suffix"
        if [ -e "$byname" ] &&
           [ "$(readlink -f "$byname" 2>/dev/null)" = "$target_resolved" ]; then
            target_slot=$suffix
        fi
    done
    case "$BOOTIMAGE" in
        */boot_a) target_slot=_a ;;
        */boot_b) target_slot=_b ;;
    esac
    case "$target_slot" in
        _a|_b) ;;
        *) >&2 echo "! cannot establish target A/B slot"; return 1 ;;
    esac

    chosen=$(pn_select_verified_backup "$BACKUP_DIR" "$BOOTIMAGE" "$target_slot") || {
        >&2 echo "! no verified matching target/slot backup; refusing recovery flash"
        return 1
    }

    # Hash and partition identity are verified by backup_guard.sh.
    # Also inspect the image format immediately before flashing.
    tmpdir=$(mktemp -d) || return 1
    if ! (cd "$tmpdir" && magiskboot unpack "$chosen" >/dev/null 2>&1); then
        rm -rf "$tmpdir"
        >&2 echo "! backup cannot be unpacked; refusing flash"
        return 1
    fi
    rm -rf "$tmpdir"

    echo "- Recovery target: $BOOTIMAGE / $target_slot"
    echo "- Verified source: $chosen"
    if ! flash_image "$chosen" "$BOOTIMAGE"; then
        >&2 echo "! recovery flashing failed"
        return 1
    fi
    echo "0" > "$PNDIR/boot_count" 2>/dev/null || true
    echo "- Verified restore completed"
    return 0
}

if [ "${PN_FLASH_RECOVERY:-0}" = "1" ]; then
    auto_unpatch
    exit $?
fi

# Do not ever reuse an unrelated old output in this working directory.
if [ -e "new-boot.img" ]; then
    >&2 echo "! stale new-boot.img present; use an empty patching workspace"
    exit 1
fi

[ -e "$BOOTIMAGE" ] || { echo "- $BOOTIMAGE does not exist!"; exit 1; }

echo "- Target image: $BOOTIMAGE"

  # Check for dependencies
command -v magiskboot >/dev/null 2>&1 || { echo "- Command magiskboot not found!"; exit 1; }
command -v kptools >/dev/null 2>&1 || { echo "- Command kptools not found!"; exit 1; }

if [ ! -f kernel ]; then
echo "- Unpacking boot image"
magiskboot unpack "$BOOTIMAGE" >/dev/null 2>&1
if [ $? -ne 0 ]; then
    >&2 echo "! Unpack error: $?"
    exit 1
  fi
fi

if [ -n "$(kptools -i kernel -l 2>/dev/null | grep patched=true)" ]; then
	echo "- kernel has been patched "
  mv kernel kernel.ori || exit 1
  echo "- Unpatching kernel"
  if ! kptools -u --image kernel.ori --out kernel; then
    >&2 echo "! Unpatch error"
    exit 1
  fi
  echo "- Repacking boot image"
  if ! magiskboot repack "$BOOTIMAGE" >/dev/null 2>&1; then
    >&2 echo "! Repack error"
    exit 1
  fi

else
  echo "- no need unpatch"
  exit 0
fi

# Generated candidate is always offline-only by default. Direct recovery
# uses PN_FLASH_RECOVERY=1 and *only* the verified original backup.
if [ ! -s "new-boot.img" ]; then
    >&2 echo "! unpack/repack produced no output"
    exit 1
fi
VERIFY_UNPATCH_DIR=$(mktemp -d) || exit 1
OUTPUT_ABSPATH=$(pwd)/new-boot.img
if ! (cd "$VERIFY_UNPATCH_DIR" && magiskboot unpack "$OUTPUT_ABSPATH" >/dev/null 2>&1); then
    rm -rf "$VERIFY_UNPATCH_DIR"
    >&2 echo "! unpatched output is structurally invalid"
    exit 1
fi
rm -rf "$VERIFY_UNPATCH_DIR"
save_image_to_storage "new-boot.img"
echo "- Unpatched image exported; current device partition was NOT written"
exit 0
