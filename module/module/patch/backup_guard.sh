#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# PatchNest verified-restore guard. SOURCE this file from boot_unpatch.sh.
# Never silently choose the newest backup without checking partition
# identity and SHA-256. Old backups without provenance are manual-only.

pn_manifest_field() {
    _pn_file=$1
    _pn_key=$2
    # Only schema-owned single-line string fields (not arbitrary JSON).
    case "$_pn_key" in target_path|target_slot|backup_sha256) ;;
        *) return 1 ;;
    esac
    _pn_value=$(sed -n "s/^[[:space:]]*\"$_pn_key\"[[:space:]]*:[[:space:]]*\"\([^\"]*\)\"[[:space:]]*,\{0,1\}[[:space:]]*$/\1/p" "$_pn_file" 2>/dev/null)
    [ -n "$_pn_value" ] || return 1
    printf '%s\n' "$_pn_value"
}

pn_verify_backup() {
    _pn_image=$1
    _pn_target=$2
    _pn_slot=$3
    _pn_meta="${_pn_image%.img}.json"
    [ -f "$_pn_image" ] && [ ! -L "$_pn_image" ] || return 1
    [ -f "$_pn_meta" ] && [ ! -L "$_pn_meta" ] || return 1
    grep -Eq '^[[:space:]]*"backup_verified"[[:space:]]*:[[:space:]]*true[[:space:]]*,?[[:space:]]*$' "$_pn_meta" || return 1
    _pn_stored_target=$(pn_manifest_field "$_pn_meta" target_path) || return 1
    _pn_stored_slot=$(pn_manifest_field "$_pn_meta" target_slot) || return 1
    _pn_expected=$(pn_manifest_field "$_pn_meta" backup_sha256) || return 1
    _pn_actual_target=$(readlink -f "$_pn_target" 2>/dev/null) || return 1
    [ "$_pn_stored_target" = "$_pn_actual_target" ] || return 1

    # A/B restore must have a known, matching slot; unknown is unsafe.
    case "$_pn_slot" in
        _a|_b) [ "$_pn_stored_slot" = "$_pn_slot" ] || return 1 ;;
        *) return 1 ;;
    esac
    case "$_pn_expected" in
        *[!0-9a-f]*|"") return 1 ;;
    esac
    [ "${#_pn_expected}" -eq 64 ] || return 1
    command -v sha256sum >/dev/null 2>&1 || return 1
    _pn_actual=$(sha256sum "$_pn_image" 2>/dev/null | awk '{print $1}')
    [ "$_pn_actual" = "$_pn_expected" ] || return 1
    return 0
}

pn_select_verified_backup() {
    _pn_dir=$1
    _pn_target=$2
    _pn_slot=$3
    _pn_chosen=""
    # Glob order uses the timestamp suffix, not unverified mtime.
    for _pn_candidate in "$_pn_dir"/boot_backup_*.img; do
        [ -f "$_pn_candidate" ] || continue
        if pn_verify_backup "$_pn_candidate" "$_pn_target" "$_pn_slot"; then
            _pn_chosen=$_pn_candidate
        fi
    done
    [ -n "$_pn_chosen" ] || return 1
    printf '%s\n' "$_pn_chosen"
}
