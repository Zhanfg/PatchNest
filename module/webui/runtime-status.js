// PatchNest runtime backend detection. Read-only and fail-closed.
//
// The legacy "kpatch hello" handshake does NOT imply that the new
// PatchNest ABI or its KPM safety capabilities are available.
import { exec } from 'kernelsu-alt';
import { modDir, escapeShell } from './constants.js';

const KNOWN_BACKENDS = new Set([
    'native', 'native-unverified', 'native-incompatible',
    'patched-legacy', 'none',
]);
const CAP_KPM_LOAD = 1 << 1;

export function parseRuntimeProbe(output) {
    const raw = {};
    for (const line of String(output || '').split(/\r?\n/)) {
        const pos = line.indexOf('=');
        if (pos <= 0) continue;
        const key = line.slice(0, pos).trim();
        if (!/^(abi|backend|capabilities|safety|loaded|ready)$/.test(key)) continue;
        raw[key] = line.slice(pos + 1).trim();
    }
    const backend = KNOWN_BACKENDS.has(raw.backend) ? raw.backend : 'none';
    const caps = /^0x[0-9a-f]+$/i.test(raw.capabilities || '')
        ? Number.parseInt(raw.capabilities, 16) : 0;
    const abiVerified = backend === 'native' && raw.abi === '1.0';
    const ready = abiVerified && raw.safety === '1';
    return {
        backend,
        abi: abiVerified ? raw.abi : null,
        capabilities: abiVerified ? caps : 0,
        ready,
        canLoadKpm: ready && (caps & CAP_KPM_LOAD) !== 0,
    };
}

export async function queryRuntimeStatus() {
    try {
        const result = await exec(
            `sh ${escapeShell(modDir + '/runtime_probe.sh')}`,
            { env: { PATH: `${modDir}/bin:/system/bin` } },
        );
        // For unsupported/native-incompatible backends the script returns 4
        // but still prints a useful backend value. Never infer capabilities.
        return parseRuntimeProbe(result?.stdout);
    } catch (_) {
        return parseRuntimeProbe('backend=none');
    }
}
