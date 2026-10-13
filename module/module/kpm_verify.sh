#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
#
# PatchNest KPM Ed25519 verifier.
# Source this file in service.sh and call:
#   verify_kpm_sig /path/module.kpm /path/module.kpm.sig
# Return 0 only for a valid detached Ed25519 signature.
#
# The signature file contains one 128-character hexadecimal line.
# The public key below is a *public* release verification key; do not
# put private signing keys in the module or repository.

KPM_SIGN_PUBKEY_HEX="a6cee3371d164daf9ad2ed38ecaf1d492e7867fc6df31f810e69eaa0dd45259b"
KPM_ED25519_SPKI_PREFIX="302a300506032b6570032100"

kpm_verify__log() {
    if [ -n "${LOG:-}" ]; then
        printf '[%s] kpm_verify: %s\n' "$(date)" "$1" >> "$LOG" 2>/dev/null || :
    fi
}

# Portable shell decoder: use octal escapes. Non-POSIX \\x escapes break
# on dash and some Android shells, producing a text file rather than bytes.
kpm_verify__hex_to_bin() {
    _hex=$1
    _out=$2
    [ -n "$_hex" ] || return 1
    [ $((${#_hex} % 2)) -eq 0 ] || return 1
    case "$_hex" in *[!0-9a-fA-F]*) return 1 ;; esac
    _esc=""
    while [ -n "$_hex" ]; do
        _tail=${_hex#??}
        _pair=${_hex%"$_tail"}
        _hex=$_tail
        _oct=$(printf '%03o' "0x$_pair") || return 1
        _esc="${_esc}\\0${_oct}"
    done
    printf '%b' "$_esc" > "$_out" || return 1
    [ "$(wc -c < "$_out" | tr -d ' ')" -eq $((${#_esc} / 5)) ] || return 1
    return 0
}

# Each verification occurs in a subshell so its umask/trap cannot affect
# service.sh or any other caller that sources this library.
verify_kpm_sig() (
    umask 077
    _kpm=$1
    _sig=$2

    if [ ! -f "$_kpm" ] || [ ! -f "$_sig" ]; then
        kpm_verify__log "module or signature missing"
        exit 1
    fi
    if ! command -v openssl >/dev/null 2>&1; then
        kpm_verify__log "openssl not available; fail closed"
        exit 1
    fi

    _sig_hex=$(awk 'NF { print; exit }' "$_sig" 2>/dev/null | tr -d ' \t\r\n')
    [ "${#_sig_hex}" -eq 128 ] || exit 1
    case "$_sig_hex" in *[!0-9a-fA-F]*) exit 1 ;; esac

    _dir=$(mktemp -d "${TMPDIR:-/data/local/tmp}/patchnest-verify.XXXXXXXX" 2>/dev/null) ||
        _dir=$(mktemp -d "/tmp/patchnest-verify.XXXXXXXX" 2>/dev/null) || exit 1
    [ -d "$_dir" ] && [ ! -L "$_dir" ] || exit 1
    trap 'rm -rf "$_dir"' EXIT
    trap 'exit 1' HUP INT TERM

    if ! kpm_verify__hex_to_bin "$KPM_ED25519_SPKI_PREFIX$KPM_SIGN_PUBKEY_HEX" "$_dir/pub.der"; then
        kpm_verify__log "invalid public verification key"
        exit 1
    fi
    if ! kpm_verify__hex_to_bin "$_sig_hex" "$_dir/sig.bin"; then
        exit 1
    fi

    # Ed25519 uses the message bytes directly. 'openssl dgst -ed25519'
    # is incorrect; pkeyutl -rawin supports Ed25519 on OpenSSL 3.x.
    if openssl pkeyutl -verify -rawin -pubin -keyform DER \
        -inkey "$_dir/pub.der" -in "$_kpm" \
        -sigfile "$_dir/sig.bin" >/dev/null 2>&1; then
        kpm_verify__log "signature verified: $(basename "$_kpm")"
        exit 0
    fi

    kpm_verify__log "signature invalid or unsupported: $(basename "$_kpm")"
    exit 1
)
