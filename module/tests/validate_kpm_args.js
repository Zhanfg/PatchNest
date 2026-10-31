#!/usr/bin/env node
/* Regression: exercise the exact legacy load lines under POSIX sh.
 * Keep the shell arguments opaque; no option separator and no word-split.
 */
const fs = require('node:fs');
const path = require('node:path');
const cp = require('node:child_process');
const assert = require('node:assert/strict');

const root = path.resolve(__dirname, '..');
const service = fs.readFileSync(path.join(root, 'module/service.sh'), 'utf8');
const installer = fs.readFileSync(path.join(root, 'module/install_kpm.sh'), 'utf8');
const serviceLoad = service.match(/(kpatch kpm load "\$kpm" "\$args")/);
const installLoad = installer.match(/(kpatch kpm load "\$KPM_DIR\/\$\{MOD_ID\}\.kpm" "\$MOD_ARGS")/);
assert(serviceLoad, 'boot service must forward exactly one raw KPM args string');
assert(installLoad, 'installer must forward exactly one raw KPM args string');
assert(!service.includes('args=[$args]'), 'raw arguments must not leak into boot logs');

function invoke(command, env) {
  const script = 'kpatch() { printf "%s\\n" "$#"; printf "<%s>\\n" "$@"; }\\n' + command;
  const res = cp.spawnSync('/bin/sh', ['-c', script], { encoding: 'utf8', env: { ...process.env, ...env } });
  assert.equal(res.status, 0, res.stderr);
  return res.stdout.trim().split('\\n');
}
for (const raw of ['-a two words "quoted" \'single\'', '', 'αβγ snow 雪']) {
  const expected = ['4', '<kpm>', '<load>', '</tmp/demo.kpm>', '<' + raw + '>'];
  assert.deepEqual(invoke(serviceLoad[1], {kpm: '/tmp/demo.kpm', args: raw}), expected);
  assert.deepEqual(invoke(installLoad[1], {KPM_DIR:'/tmp', MOD_ID:'demo', MOD_ARGS: raw}), expected);
}
console.log('Legacy KPM parameter forwarding: PASS');
