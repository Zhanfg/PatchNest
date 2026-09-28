#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
obj="${TMPDIR:-/tmp}/patchnest-main-argv0.o"
out="${TMPDIR:-/tmp}/patchnest-argv0-test"

${CC:-cc} \
  -std=c11 -D_GNU_SOURCE \
  -Wall -Wextra -Werror \
  -I"$repo_root/src" \
  -Dmain=patchnest_cli_main \
  -c "$repo_root/src/main.c" \
  -o "$obj"

${CC:-cc} \
  -std=c11 -D_GNU_SOURCE \
  -Wall -Wextra -Werror \
  -I"$repo_root/src" \
  "$repo_root/tests/argv0_test.c" \
  "$obj" \
  -o "$out"

"$out"
rm -f "$obj" "$out"
