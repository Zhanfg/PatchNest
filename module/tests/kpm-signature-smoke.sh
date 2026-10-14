#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Public Ed25519 fixture from PatchNest verifier; never contains a private key.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
. "$ROOT/module/module/kpm_verify.sh"
TESTDIR=$(mktemp -d)
trap 'rm -rf "$TESTDIR"' EXIT

PUB="a6cee3371d164daf9ad2ed38ecaf1d492e7867fc6df31f810e69eaa0dd45259b"
SIG="886199b494a8dcb9ddec3a48385f4ea7e3cbefcc90198c6c807fd2434125b20e32f1b8cdd817e782f7fcf80860c4a32f7c49006089a4efefb4b734bbcb30f703"

test "$KPM_SIGN_PUBKEY_HEX" = "$PUB"
printf 'probe' > "$TESTDIR/sample.kpm"
printf '%s\n' "$SIG" > "$TESTDIR/sample.kpm.sig"
verify_kpm_sig "$TESTDIR/sample.kpm" "$TESTDIR/sample.kpm.sig"

printf 'probe-modified' > "$TESTDIR/sample.kpm"
if verify_kpm_sig "$TESTDIR/sample.kpm" "$TESTDIR/sample.kpm.sig"; then
    echo "ERROR: tampered module accepted" >&2
    exit 1
fi

printf 'probe' > "$TESTDIR/sample.kpm"
printf '%s\n' "${SIG%?}0" > "$TESTDIR/sample.kpm.sig"
if verify_kpm_sig "$TESTDIR/sample.kpm" "$TESTDIR/sample.kpm.sig"; then
    echo "ERROR: tampered signature accepted" >&2
    exit 1
fi

printf 'broken\n' > "$TESTDIR/sample.kpm.sig"
if verify_kpm_sig "$TESTDIR/sample.kpm" "$TESTDIR/sample.kpm.sig"; then
    echo "ERROR: malformed signature accepted" >&2
    exit 1
fi

if verify_kpm_sig "$TESTDIR/missing.kpm" "$TESTDIR/sample.kpm.sig"; then
    echo "ERROR: missing module accepted" >&2
    exit 1
fi

echo "PASS: valid/tampered/invalid/missing Ed25519 checks"
