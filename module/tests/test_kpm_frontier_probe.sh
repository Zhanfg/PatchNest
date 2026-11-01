#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu
TOP=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PROBE="$TOP/module/tools/kpm_frontier_probe.sh"
DIR=$(mktemp -d)
trap 'rm -rf "$DIR"' EXIT HUP INT TERM
mkdir -p "$DIR/proc/sys/kernel" "$DIR/sys/module"
assert_line() {
    printf '%s\n' "$output" | grep -Fqx "$1" || {
        printf '%s\n' "Expected: $1" "Actual output:" "$output" >&2
        exit 1
    }
}
run_probe() {
    output=$(PN_FRONTIER_FS_ROOT="$DIR" sh "$PROBE")
}

printf '%s\n' '6.6.147-android15-9-gki' > "$DIR/proc/sys/kernel/osrelease"
printf '%s\n' 0 > "$DIR/proc/sys/kernel/modules_disabled"
run_probe
assert_line 'patchnest_probe=KPM_FRONTIER_V2'
assert_line 'kmi=android15-6.6'
assert_line 'kmi_signal=release_tag_match'
assert_line 'susfs_lkm_build_target=listed'
assert_line 'susfs_control=not_observed'
assert_line 'susfs_path=not_observed'
assert_line 'susfs_hide_mounts=not_observed'
assert_line 'btf_vmlinux=not_observed'
assert_line 'selinux_enforcing=unknown'
assert_line 'susfs_lkm=not_observed'
assert_line 'kernelpatch_lkm=not_observed'
assert_line 'syscall_45_ownership=unverified'
assert_line 'module_loading=never'

mkdir -p "$DIR/sys/module/susfs_guard_lkm" "$DIR/sys/module/kernelpatch" "$DIR/sys/kernel/btf" "$DIR/sys/fs/selinux"
: > "$DIR/sys/kernel/btf/vmlinux"
printf '%s\n' 1 > "$DIR/sys/fs/selinux/enforce"
: > "$DIR/proc/susfs_hide_mounts"
: > "$DIR/proc/susfs_path"
printf '%s\n' 1 > "$DIR/proc/sys/kernel/modules_disabled"
run_probe
assert_line 'susfs_control=observed'
assert_line 'susfs_hide_mounts=observed'
assert_line 'btf_vmlinux=observed'
assert_line 'selinux_enforcing=1'
assert_line 'susfs_lkm=observed'
assert_line 'kernelpatch_lkm=observed'
assert_line 'coexistence=multiple_lkms_observed_unverified'
assert_line 'module_loading_policy=disabled'
assert_line 'conclusion=manual_review_required'

rm -rf "$DIR/sys/module/susfs_guard_lkm" "$DIR/sys/module/kernelpatch" "$DIR/proc/susfs_path"
printf '%s\n' 'susfs_guard_lkm 100 0 - Live 0x00000000' > "$DIR/proc/modules"
run_probe
assert_line 'susfs_lkm=observed'
assert_line 'susfs_control=not_observed'

printf '%s\n' '6.6.147-android14-9-gki' > "$DIR/proc/sys/kernel/osrelease"
run_probe
assert_line 'kmi=unresolved'
assert_line 'susfs_lkm_build_target=unverified'

printf '%s\n' '6.6.147-vendor-custom' > "$DIR/proc/sys/kernel/osrelease"
run_probe
assert_line 'kmi=unresolved'

printf '%s\n' '5.15.149-android13-8-gki' > "$DIR/proc/sys/kernel/osrelease"
run_probe
assert_line 'kmi=android13-5.15'

printf '%s\n' '5.15.149-android14-8-gki' > "$DIR/proc/sys/kernel/osrelease"
run_probe
assert_line 'kmi=android14-5.15'

printf '%s\n' '6.18.12-android17-2-gki' > "$DIR/proc/sys/kernel/osrelease"
run_probe
assert_line 'kmi=android17-6.18'

printf '%s\n' 'unknown' > "$DIR/proc/sys/kernel/osrelease"
run_probe
assert_line 'kmi=unresolved'
printf '%s\n' 'KPM frontier probe: PASS (read-only, no module loading)'
