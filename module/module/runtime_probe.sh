#!/system/bin/sh
# PatchNest dual-backend capability probe. Read-only; no flashing/loading.
# This script accepts no parameters (safe for Android shell-only launchers).
MODDIR=${0%/*}
PATH="$MODDIR/bin:/system/bin:/system/xbin:$PATH"
export PATH

if [ -e /dev/patchnest ]; then
    if [ ! -x "$MODDIR/bin/patchnest-status" ]; then
        printf 'backend=native-unverified\ncapabilities=unknown\nready=0\n'
        exit 4
    fi
    # Trust the native kernel-provided ABI rather than the root manager.
    status=$("$MODDIR/bin/patchnest-status" 2>/dev/null)
    rc=$?
    if [ "$rc" -ne 0 ]; then
        printf 'backend=native-incompatible\ncapabilities=unknown\nready=0\n'
        exit 4
    fi
    printf '%s\n' "$status"
    case "$status" in
        *"safety=1"*) printf 'ready=1\n' ;;
        *) printf 'ready=0\n' ;;
    esac
    exit 0
fi

# Existing patched kernels have an older supercall ABI; a HELLO proves
# only that legacy handshake, NOT that the new unified ABI supports KPM.
if [ -x "$MODDIR/bin/kpatch" ] &&
    "$MODDIR/bin/kpatch" hello >/dev/null 2>&1; then
    printf 'backend=patched-legacy\nabi=legacy\ncapabilities=unknown\nready=0\n'
    exit 0
fi

printf 'backend=none\ncapabilities=0x0\nready=0\n'
exit 4
