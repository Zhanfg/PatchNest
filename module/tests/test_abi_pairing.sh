#!/usr/bin/env bash
set -euo pipefail

module_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
checker="$module_dir/check-abi-pairing.sh"
fixture_dir=$(mktemp -d)
trap 'rm -rf "$fixture_dir"' EXIT
kpimg="$fixture_dir/kpimg"
kpatch="$fixture_dir/kpatch"

assert_failure() {
    local output
    if output=$(bash "$checker" "$kpimg" "$kpatch" 2>&1); then
        echo "Expected ABI check to fail" >&2
        exit 1
    fi
    for expected in "$@"; do
        if [[ "$output" != *"$expected"* ]]; then
            printf 'Expected error to contain %s, got:\n%s\n' "$expected" "$output" >&2
            exit 1
        fi
    done
}

printf '\001[+] KP I hello1158\n\000' > "$kpimg"
printf '\002hello1158\000' > "$kpatch"
bash "$checker" "$kpimg" "$kpatch"

printf '\002hello2026\000' > "$kpatch"
assert_failure hello1158 hello2026

printf '\002hello1158suffix\000' > "$kpatch"
assert_failure kpatch

printf '\001no-marker\000' > "$kpimg"
printf '\002hello1158\000' > "$kpatch"
assert_failure kpimg

printf '\001hello1158\000hello2026\000' > "$kpimg"
assert_failure hello1158 hello2026

printf '\001hello9876\000hello9876\000' > "$kpimg"
printf '\002hello9876\000hello9876\000' > "$kpatch"
bash "$checker" "$kpimg" "$kpatch"

printf '%s\n' 'ABI pairing tests passed'
