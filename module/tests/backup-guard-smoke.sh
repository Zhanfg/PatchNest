#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host unit tests only: no block-device access and no flash operations.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
. "$ROOT/module/module/patch/backup_guard.sh"
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT HUP INT TERM
mkdir "$T/backups"
printf 'mock target boot A' > "$T/device-boot-a.img"
target=$(readlink -f "$T/device-boot-a.img")
backup="$T/backups/boot_backup_20261014_000100_101.img"
printf 'known boot snapshot' > "$backup"
digest=$(sha256sum "$backup" | awk '{print $1}')
cat > "${backup%.img}.json" <<EOF
{
  "target_path": "$target",
  "target_slot": "_a",
  "backup_sha256": "$digest",
  "backup_verified": true
}
EOF
pn_verify_backup "$backup" "$target" "_a"
! pn_verify_backup "$backup" "$target" "_b"
! pn_verify_backup "$backup" "$T/wrong-partition" "_a"
[ "$(pn_select_verified_backup "$T/backups" "$target" "_a")" = "$backup" ]

# Newer, corrupt, or unproven backups must not win.
later="$T/backups/boot_backup_20261014_000200_202.img"
cp "$backup" "$later"
cat > "${later%.img}.json" <<EOF
{
  "target_path": "$target",
  "target_slot": "_a",
  "backup_sha256": "$digest",
  "backup_verified": false
}
EOF
[ "$(pn_select_verified_backup "$T/backups" "$target" "_a")" = "$backup" ]
sed -i 's/"backup_verified": true/"backup_verified": false/' "${backup%.img}.json"
! pn_select_verified_backup "$T/backups" "$target" "_a" >/dev/null

sed -i 's/"backup_verified": false/"backup_verified": true/' "${backup%.img}.json"
printf 'modified\n' >> "$backup"
! pn_verify_backup "$backup" "$target" "_a"

# No matching metadata, symlinked image, or unknown slot accepted.
rm -f "$backup"
ln -s "$later" "$backup"
! pn_verify_backup "$backup" "$target" "_a"
! pn_verify_backup "$later" "$target" "unknown"
echo "PASS: verified recovery guard rejects wrong slot, target, digest, and backup state"
