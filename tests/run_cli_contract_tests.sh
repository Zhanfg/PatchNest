#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
out="${TMPDIR:-/tmp}/patchnest-cli-contract-test"

${CC:-cc} \
  -std=c11 -D_GNU_SOURCE \
  -Wall -Wextra -Werror \
  -I"$repo_root/src" \
  "$repo_root/tests/cli_contract_test.c" \
  "$repo_root/src/kpatch.c" \
  "$repo_root/src/kpm.c" \
  "$repo_root/src/kpextension.c" \
  -Wl,--wrap=syscall \
  -o "$out"

"$out"
rm -f "$out"
