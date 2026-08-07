#!/usr/bin/env node
'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { execFileSync } = require('node:child_process');

const repoRoot = path.resolve(__dirname, '..');
const validator = ['scripts/validate.js'];

function copyRepo() {
  const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'patchnest-kpm-policy-'));
  fs.cpSync(repoRoot, temp, {
    recursive: true,
    filter: (source) => path.basename(source) !== '.git' && !source.includes(`${path.sep}build-reports${path.sep}`),
  });
  return temp;
}

function run(root) {
  return execFileSync(process.execPath, validator, {
    cwd: root,
    encoding: 'utf8',
    stdio: ['ignore', 'pipe', 'pipe'],
  });
}

function expectRejected(name, mutate, expectedText) {
  const root = copyRepo();
  try {
    mutate(root);
    let rejected = false;
    try {
      run(root);
    } catch (error) {
      rejected = true;
      const output = `${error.stdout || ''}\n${error.stderr || ''}`;
      if (expectedText) assert.match(output, expectedText, `${name}: wrong rejection reason`);
    }
    assert.equal(rejected, true, `${name}: bypass fixture unexpectedly passed`);
    process.stdout.write(`PASS reject: ${name}\n`);
  } finally {
    fs.rmSync(root, { recursive: true, force: true });
  }
}

const baseline = copyRepo();
try {
  run(baseline);
  process.stdout.write('PASS baseline build graph\n');
} finally {
  fs.rmSync(baseline, { recursive: true, force: true });
}

expectRejected(
  'undeclared second source object',
  (root) => {
    const dir = path.join(root, 'modules/diagnostic-hello');
    fs.writeFileSync(path.join(dir, 'extra.c'), 'int hidden_extra(void) { return 0; }\n');
    const makefile = path.join(dir, 'Makefile');
    fs.writeFileSync(
      makefile,
      fs.readFileSync(makefile, 'utf8').replace(
        'SOURCES := diagnostic_hello.c',
        'SOURCES := diagnostic_hello.c extra.c',
      ),
    );
  },
  /undeclared checked-in candidate file|SOURCES differs/,
);

expectRejected(
  'undeclared local header',
  (root) => {
    const dir = path.join(root, 'modules/diagnostic-hello');
    fs.writeFileSync(path.join(dir, 'extra.h'), '#define EXTRA 1\n');
    const source = path.join(dir, 'diagnostic_hello.c');
    fs.writeFileSync(source, `#include "extra.h"\n${fs.readFileSync(source, 'utf8')}`);
  },
  /undeclared checked-in candidate file|header is not in build\.localHeaders/,
);

expectRejected(
  'linker script removed from final link flags',
  (root) => {
    const makefile = path.join(root, 'modules/diagnostic-hello/Makefile');
    fs.writeFileSync(
      makefile,
      fs.readFileSync(makefile, 'utf8').replace(' -Wl,-T,$(LINKER_SCRIPT)', ''),
    );
  },
  /LDFLAGS missing -Wl,-T,\$\(LINKER_SCRIPT\)/,
);

expectRejected(
  'compile stage receives linker-only flag',
  (root) => {
    const makefile = path.join(root, 'modules/diagnostic-hello/Makefile');
    fs.writeFileSync(
      makefile,
      fs.readFileSync(makefile, 'utf8').replace(
        '$(CC) $(CFLAGS) $(DEPFLAGS)',
        '$(CC) -Tdiagnostic_hello.lds $(CFLAGS) $(DEPFLAGS)',
      ),
    );
  },
  /compile-only recipe contains linker-script flag/,
);

expectRejected(
  'network-capable build recipe',
  (root) => {
    const makefile = path.join(root, 'modules/diagnostic-hello/Makefile');
    fs.appendFileSync(makefile, '\nnetwork-test:\n\tcurl https://example.invalid/object.o -o object.o\n');
  },
  /network-capable build recipe is forbidden/,
);

expectRejected(
  'prebuilt object checked into candidate directory',
  (root) => {
    fs.writeFileSync(path.join(root, 'modules/diagnostic-hello/extra.o'), 'not-an-object');
  },
  /undeclared checked-in candidate file|prebuilt object is forbidden/,
);

process.stdout.write('build policy bypass tests: PASS\n');
