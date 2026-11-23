#!/bin/sh
# Generate fresh Ed25519 keys and exercise the public verification API.
set -e

TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
VERIFY_SCRIPT="$TEST_DIR/../module/kpm_verify.sh"
TEST_TMP=$(mktemp -d "${TMPDIR:-/tmp}/kpm_verify_test.XXXXXX")
trap 'rm -rf -- "$TEST_TMP"' EXIT
trap 'exit 1' HUP INT TERM
mkdir "$TEST_TMP/verify-tmp" "$TEST_TMP/no-openssl" "$TEST_TMP/failing-openssl"
TMPDIR="$TEST_TMP/verify-tmp"
export TMPDIR
LOG=
PNDIR=

openssl genpkey -algorithm ed25519 -out "$TEST_TMP/private.pem" >/dev/null 2>&1
openssl genpkey -algorithm ed25519 -out "$TEST_TMP/other-private.pem" >/dev/null 2>&1
openssl pkey -in "$TEST_TMP/private.pem" -pubout -outform DER -out "$TEST_TMP/public.der" 2>/dev/null
openssl pkey -in "$TEST_TMP/other-private.pem" -pubout -outform DER -out "$TEST_TMP/other-public.der" 2>/dev/null
PUBLIC_HEX=$(tail -c 32 "$TEST_TMP/public.der" | od -An -v -tx1 | tr -d ' \n')
OTHER_PUBLIC_HEX=$(tail -c 32 "$TEST_TMP/other-public.der" | od -An -v -tx1 | tr -d ' \n')
printf 'module\000bytes\200\377\n' > "$TEST_TMP/module"
openssl pkeyutl -sign -inkey "$TEST_TMP/private.pem" -rawin \
    -in "$TEST_TMP/module" -out "$TEST_TMP/signature.bin"
SIGNATURE_HEX=$(od -An -v -tx1 "$TEST_TMP/signature.bin" | tr -d ' \n')
printf '%s\n' "$SIGNATURE_HEX" > "$TEST_TMP/signature"
case "$SIGNATURE_HEX" in
    0*) BAD_SIGNATURE_HEX="1${SIGNATURE_HEX#?}" ;;
    *) BAD_SIGNATURE_HEX="0${SIGNATURE_HEX#?}" ;;
esac
printf '%s\n' "$BAD_SIGNATURE_HEX" > "$TEST_TMP/bad-signature"
printf 'module\000bytes\200\376\n' > "$TEST_TMP/tampered-module"

# Preserve the verifier's normal external dependencies while hiding openssl.
for tool in awk tr wc rm mktemp basename date; do
    ln -s "$(command -v "$tool")" "$TEST_TMP/no-openssl/$tool"
done
cat > "$TEST_TMP/failing-openssl/openssl" <<'MOCK'
#!/bin/sh
printf '%s\n' "$1" > "$OPENSSL_MARKER"
exit 42
MOCK
chmod +x "$TEST_TMP/failing-openssl/openssl"
OPENSSL_MARKER="$TEST_TMP/openssl-invoked"
export OPENSSL_MARKER

. "$VERIFY_SCRIPT"
KPM_SIGN_PUBKEY_HEX=$PUBLIC_HEX
trap > "$TEST_TMP/traps-before"

expect_status() {
    expected=$1
    name=$2
    shift 2
    if verify_kpm_sig "$@"; then
        actual=0
    else
        actual=$?
    fi
    if [ "$actual" -ne "$expected" ]; then
        printf 'FAIL %s (expected %s, got %s)\n' "$name" "$expected" "$actual" >&2
        exit 1
    fi
    printf 'PASS %s\n' "$name"
}

expect_status 0 'valid signature' "$TEST_TMP/module" "$TEST_TMP/signature"
expect_status 1 'tampered signature' "$TEST_TMP/module" "$TEST_TMP/bad-signature"
expect_status 1 'tampered content' "$TEST_TMP/tampered-module" "$TEST_TMP/signature"
KPM_SIGN_PUBKEY_HEX=$OTHER_PUBLIC_HEX
expect_status 1 'wrong public key' "$TEST_TMP/module" "$TEST_TMP/signature"
KPM_SIGN_PUBKEY_HEX=$PUBLIC_HEX
expect_status 1 'missing signature' "$TEST_TMP/module" "$TEST_TMP/missing-signature"
KPM_SIGN_PUBKEY_HEX="z${PUBLIC_HEX#?}"
expect_status 1 'invalid public key' "$TEST_TMP/module" "$TEST_TMP/signature"
KPM_SIGN_PUBKEY_HEX=$PUBLIC_HEX

(
    PATH="$TEST_TMP/no-openssl"
    if command -v openssl >/dev/null 2>&1; then
        printf 'FAIL openssl unavailable setup\n' >&2
        exit 1
    fi
    expect_status 1 'openssl unavailable' "$TEST_TMP/module" "$TEST_TMP/signature"
)
PATH="$TEST_TMP/failing-openssl:$PATH" expect_status 1 'pkeyutl execution failure' \
    "$TEST_TMP/module" "$TEST_TMP/signature"
if ! grep -qx pkeyutl "$OPENSSL_MARKER"; then
    printf 'FAIL pkeyutl execution failure setup\n' >&2
    exit 1
fi

trap > "$TEST_TMP/traps-after"
if ! cmp -s "$TEST_TMP/traps-before" "$TEST_TMP/traps-after"; then
    printf 'FAIL caller traps preserved\n' >&2
    exit 1
fi
printf 'PASS caller traps preserved\n'
for path in "$TMPDIR"/* "$TMPDIR"/.[!.]* "$TMPDIR"/..?*; do
    if [ -e "$path" ] || [ -L "$path" ]; then
        printf 'FAIL temporary files cleaned up\n' >&2
        exit 1
    fi
done
printf 'PASS temporary files cleaned up\n'
