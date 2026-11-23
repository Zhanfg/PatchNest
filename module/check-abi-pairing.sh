#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C

hello_marker() {
    local binary=$1 markers
    # kpimg embeds the marker in a printk message. kpatch embeds the echo itself.
    markers=$(strings -a "$binary" | awk -F '[^[:alnum:]_]+' '{
        for (i = 1; i <= NF; i++)
            if ($i ~ /^hello[0-9]+$/) print $i
    }' | sort -u) || return 1

    if [[ -z "$markers" || "$markers" == *$'\n'* ]]; then
        printf 'ERROR: expected one distinct ABI hello marker in %s, found: %s\n' \
            "$binary" "${markers:-none}" >&2
        return 1
    fi
    printf '%s\n' "$markers"
}

core_hello=$(hello_marker "$1")
cli_hello=$(hello_marker "$2")
if [[ "$core_hello" != "$cli_hello" ]]; then
    printf 'ERROR: ABI mismatch: %s contains %s, %s contains %s\n' \
        "$1" "$core_hello" "$2" "$cli_hello" >&2
    exit 1
fi
printf 'Verified ABI pairing: %s and %s contain %s\n' "$1" "$2" "$core_hello"
