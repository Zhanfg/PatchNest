#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cc=${CC:-cc}
work="${TMPDIR:-/tmp}/patchnest-sanitizers-$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT HUP INT TERM

san_flags='-fsanitize=address,undefined -fno-omit-frame-pointer -g'
common_flags='-std=c11 -D_GNU_SOURCE -Wall -Wextra -Werror'

# Exercise the syscall/CLI contract under ASan + UBSan, including hello ABI
# mismatch/error handling used by PatchNest-Module as its readiness gate.
# shellcheck disable=SC2086
$cc $common_flags $san_flags \
  -I"$repo_root/src" \
  "$repo_root/tests/cli_contract_test.c" \
  "$repo_root/src/kpatch.c" \
  "$repo_root/src/kpm.c" \
  "$repo_root/src/kpextension.c" \
  -Wl,--wrap=syscall \
  -o "$work/cli-contract"

ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  "$work/cli-contract"

# Compile the real main.c and exercise long argv[0] values under the same sanitizers.
# shellcheck disable=SC2086
$cc $common_flags $san_flags \
  -I"$repo_root/src" \
  -Dmain=patchnest_cli_main \
  -c "$repo_root/src/main.c" \
  -o "$work/main.o"

# shellcheck disable=SC2086
$cc $common_flags $san_flags \
  -I"$repo_root/src" \
  "$repo_root/tests/argv0_test.c" \
  "$work/main.o" \
  -o "$work/argv0"

ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  "$work/argv0"

printf '%s\n' 'ASan/UBSan CLI tests: PASS'
