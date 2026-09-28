#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
work="${TMPDIR:-/tmp}/patchnest-cli-contract-$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT HUP INT TERM

${CC:-cc} \
  -std=c11 -D_GNU_SOURCE \
  -Wall -Wextra -Werror \
  -I"$repo_root/src" \
  "$repo_root/tests/cli_contract_test.c" \
  "$repo_root/src/kpatch.c" \
  "$repo_root/src/kpm.c" \
  "$repo_root/src/kpextension.c" \
  -Wl,--wrap=syscall \
  -o "$work/cli-contract"

"$work/cli-contract"

${CC:-cc} \
  -std=c11 -D_GNU_SOURCE \
  -DPATCHNEST_ABI_PUBLIC1158=1 \
  -DSUPERCALL_KEY_MAX_LEN=0x40 \
  -Wall -Wextra -Werror \
  -I"$repo_root/src" \
  "$repo_root/tests/public1158_profile_test.c" \
  "$repo_root/src/kpatch.c" \
  -Wl,--wrap=syscall \
  -o "$work/public1158-profile"

"$work/public1158-profile"
