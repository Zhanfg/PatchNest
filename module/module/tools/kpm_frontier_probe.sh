#!/system/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# PatchNest capability probe v1: zero command-line arguments, read-only.
# A positive observation is evidence; no observation never proves absence.
# This script must not load/unload a module, write to /proc, patch boot, or hook.
set -u

# The override exists solely to run fixtures in CI; devices normally use '/'.
ROOT=${PN_FRONTIER_FS_ROOT:-/}
case "$ROOT" in
    /*) ;;
    *) printf '%s\n' 'error=invalid_probe_root'; exit 2 ;;
esac

fs_path() {
    if [ "$ROOT" = "/" ]; then
        printf '%s' "$1"
    else
        printf '%s%s' "${ROOT%/}" "$1"
    fi
}

# Keep the printed kernel release free of arbitrary control characters.
release_file=$(fs_path /proc/sys/kernel/osrelease)
if [ -r "$release_file" ]; then
    release=$(sed -n '1p' "$release_file" 2>/dev/null | LC_ALL=C tr -cd 'A-Za-z0-9._+:-')
else
    release=''
fi
[ -n "$release" ] || release=unknown

# Strict KMI: never choose by closest kernel series or Android OS version.
# The names below describe upstream GKI/KMI build targets, not proof of ABI.
kmi=unresolved
case "$release" in
    5.10.*-android12-*) kmi=android12-5.10 ;;
    5.10.*-android13-*) kmi=android13-5.10 ;;
    5.15.*-android13-*) kmi=android13-5.15 ;;
    5.15.*-android14-*) kmi=android14-5.15 ;;
    6.1.*-android14-*)  kmi=android14-6.1 ;;
    6.6.*-android15-*)  kmi=android15-6.6 ;;
    6.12.*-android16-*) kmi=android16-6.12 ;;
    6.18.*-android17-*) kmi=android17-6.18 ;;
esac
if [ "$kmi" = unresolved ]; then
    kmi_signal=unresolved
    susfs_lkm_build_target=unverified
else
    kmi_signal=release_tag_match
    susfs_lkm_build_target=listed
fi

module_seen() {
    name=$1
    [ -d "$(fs_path "/sys/module/$name")" ] && return 0
    modules=$(fs_path /proc/modules)
    if [ -r "$modules" ]; then
        grep -q "^$name " "$modules" 2>/dev/null && return 0
    fi
    return 1
}

susfs_lkm=not_observed
kernelpatch_lkm=not_observed
if module_seen susfs_guard_lkm; then susfs_lkm=observed; fi
if module_seen kernelpatch; then kernelpatch_lkm=observed; fi

susfs_control=not_observed
if [ -e "$(fs_path /proc/susfs_path)" ] ||
   [ -e "$(fs_path /proc/susfs_kstat)" ]; then
    susfs_control=observed
fi

disabled_file=$(fs_path /proc/sys/kernel/modules_disabled)
module_loading_policy=unknown
if [ -r "$disabled_file" ]; then
    case "$(sed -n '1p' "$disabled_file" 2>/dev/null)" in
        0) module_loading_policy=not_disabled ;;
        1) module_loading_policy=disabled ;;
    esac
fi

coexistence=unverified
if [ "$susfs_lkm" = observed ] && [ "$kernelpatch_lkm" = observed ]; then
    coexistence=multiple_lkms_observed_unverified
fi

# Observable procfs interfaces; presence is not a permission or ABI handshake.
# Never write any control node or dump path/rule contents.
susfs_node_state() {
    if [ -e "$(fs_path "/proc/$1")" ]; then
        printf '%s' observed
    else
        printf '%s' not_observed
    fi
}
susfs_path=$(susfs_node_state susfs_path)
susfs_kstat=$(susfs_node_state susfs_kstat)
susfs_open_redirect=$(susfs_node_state susfs_open_redirect)
susfs_hide_mounts=$(susfs_node_state susfs_hide_mounts)
susfs_hide_modules=$(susfs_node_state susfs_hide_modules)
susfs_map=$(susfs_node_state susfs_map)
susfs_opath=$(susfs_node_state susfs_opath)

btf_vmlinux=not_observed
if [ -r "$(fs_path /sys/kernel/btf/vmlinux)" ]; then
    btf_vmlinux=observed
fi
selinux_enforcing=unknown
if [ -r "$(fs_path /sys/fs/selinux/enforce)" ]; then
    case "$(sed -n '1p' "$(fs_path /sys/fs/selinux/enforce)" 2>/dev/null)" in
        0) selinux_enforcing=0 ;;
        1) selinux_enforcing=1 ;;
    esac
fi

printf '%s\n' \
    "patchnest_probe=KPM_FRONTIER_V2" \
    "kernel_release=$release" \
    "kmi=$kmi" \
    "kmi_signal=$kmi_signal" \
    "susfs_lkm_build_target=$susfs_lkm_build_target" \
    "susfs_control=$susfs_control" \
    "susfs_path=$susfs_path" \
    "susfs_kstat=$susfs_kstat" \
    "susfs_open_redirect=$susfs_open_redirect" \
    "susfs_hide_mounts=$susfs_hide_mounts" \
    "susfs_hide_modules=$susfs_hide_modules" \
    "susfs_map=$susfs_map" \
    "susfs_opath=$susfs_opath" \
    "btf_vmlinux=$btf_vmlinux" \
    "selinux_enforcing=$selinux_enforcing" \
    "susfs_lkm=$susfs_lkm" \
    "kernelpatch_lkm=$kernelpatch_lkm" \
    "module_loading_policy=$module_loading_policy" \
    "coexistence=$coexistence" \
    "syscall_45_ownership=unverified" \
    "kpm_abi=unverified" \
    "module_loading=never" \
    "conclusion=manual_review_required"
