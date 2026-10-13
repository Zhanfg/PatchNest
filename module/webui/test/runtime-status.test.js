import { describe, it, expect, vi } from 'vitest';

vi.mock('kernelsu-alt', () => ({ exec: vi.fn() }));
import { parseRuntimeProbe } from '../runtime-status.js';

describe('PatchNest backend capability negotiation', () => {
    it('never calls native status-only adapter KPM-capable', () => {
        const state = parseRuntimeProbe(
            'abi=1.0\nbackend=native\ncapabilities=0x0\nsafety=0\nloaded=0\n');
        expect(state.backend).toBe('native');
        expect(state.ready).toBe(false);
        expect(state.canLoadKpm).toBe(false);
    });

    it('needs both verified ABI and live capability before allowing loads', () => {
        const state = parseRuntimeProbe(
            'abi=1.0\nbackend=native\ncapabilities=0x3\nsafety=1\nloaded=1\n');
        expect(state.canLoadKpm).toBe(true);
    });

    it('does not upgrade legacy patched backend by forged capabilities', () => {
        const state = parseRuntimeProbe(
            'backend=patched-legacy\nabi=1.0\ncapabilities=0xffffffff\nsafety=1\n');
        expect(state.ready).toBe(false);
        expect(state.capabilities).toBe(0);
        expect(state.canLoadKpm).toBe(false);
    });

    it('rejects malformed and unknown backends', () => {
        const state = parseRuntimeProbe('backend=unknown\ncapabilities=garbage');
        expect(state.backend).toBe('none');
        expect(state.canLoadKpm).toBe(false);
    });
});
