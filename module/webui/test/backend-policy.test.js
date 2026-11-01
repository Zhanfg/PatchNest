import { describe, expect, it } from 'vitest';
import { FRONTIER_PROTOCOL, parseFrontierReport, evaluateBackendPolicy } from '../backend-policy.js';

function sample(overrides = {}) {
    const state = {
        patchnest_probe: FRONTIER_PROTOCOL,
        kernel_release: '6.6.147-android15-9-gki', kmi: 'android15-6.6',
        kmi_signal: 'release_tag_match', susfs_lkm_build_target: 'listed',
        susfs_control: 'not_observed',
        susfs_path: 'not_observed', susfs_kstat: 'not_observed',
        susfs_open_redirect: 'not_observed', susfs_hide_mounts: 'not_observed',
        susfs_hide_modules: 'not_observed', susfs_map: 'not_observed',
        susfs_opath: 'not_observed', btf_vmlinux: 'not_observed',
        selinux_enforcing: 'unknown', susfs_lkm: 'not_observed',
        kernelpatch_lkm: 'not_observed', module_loading_policy: 'not_disabled',
        coexistence: 'unverified', syscall_45_ownership: 'unverified',
        kpm_abi: 'unverified', module_loading: 'never',
        conclusion: 'manual_review_required',
        ...overrides,
    };
    return Object.entries(state).map(([key, value]) => key + '=' + value).join('\n') + '\n';
}
describe('PatchNest backend policy v1', () => {
    it('keeps all mutation denied even with GKI release tag', () => {
        const plan = evaluateBackendPolicy(parseFrontierReport(sample()));
        expect(plan.kmiConfidence).toBe('tag_only');
        expect(plan.susfsSource).toBe('not_observed');
        expect(plan.decisions.install_susfs_lkm).toMatch(/^DENY_/);
        expect(plan.decisions.register_vfs_hook).toMatch(/^DENY_/);
        expect(plan.decisions.register_syscall_hook).toMatch(/^DENY_/);
        expect(plan.decisions.kpm_control).toMatch(/^DENY_/);
        expect(plan.mode).toBe('read_only');
        expect(plan.resources.every(r => r.lease === 'unverified')).toBe(true);
    });
    it('recognizes a present SUSFS provider without claiming a known owner', () => {
        const report = parseFrontierReport(sample({
            susfs_path: 'observed', susfs_control: 'observed',
            susfs_lkm: 'observed', susfs_open_redirect: 'observed',
        }));
        const plan = evaluateBackendPolicy(report);
        expect(plan.susfsInterfaces).toEqual(['susfs_path', 'susfs_open_redirect']);
        expect(plan.susfsSource).toBe('lkm_observed');
        expect(plan.decisions.install_susfs_lkm).toBe('DENY_DUPLICATE_OR_UNKNOWN_PROVIDER');
        expect(plan.decisions.write_susfs_rules).toMatch(/^DENY_/);
    });
    it('handles visible controls but an unobserved/hidden provider', () => {
        const plan = evaluateBackendPolicy(parseFrontierReport(sample({
            susfs_kstat: 'observed', susfs_control: 'observed',
        })));
        expect(plan.susfsSource).toBe('control_interface_observed_provider_unknown');
        expect(plan.decisions.install_susfs_lkm).toMatch(/^DENY_/);
    });
    it('recognizes dual LKM observation without granting dual-hook ownership', () => {
        const plan = evaluateBackendPolicy(parseFrontierReport(sample({
            susfs_lkm: 'observed', kernelpatch_lkm: 'observed',
            coexistence: 'multiple_lkms_observed_unverified',
        })));
        expect(plan.kernelpatchLkm).toBe('observed');
        expect(plan.decisions.install_kernelpatch_lkm).toMatch(/^DENY_/);
        expect(plan.resources[1].owner).toBe('unverified');
    });
    it('rejects malformed, ambiguous, and downgraded observations', () => {
        const valid = sample();
        expect(() => parseFrontierReport(valid + 'kmi=android15-6.6\n')).toThrow();
        expect(() => parseFrontierReport(valid.replace(FRONTIER_PROTOCOL, 'KPM_FRONTIER_V1'))).toThrow();
        expect(() => parseFrontierReport(valid.replace('kpm_abi=unverified', 'kpm_abi=verified'))).toThrow();
        expect(() => parseFrontierReport(valid.replace('syscall_45_ownership=unverified', 'syscall_45_ownership=none'))).toThrow();
        expect(() => parseFrontierReport(valid.replace('susfs_path=not_observed', 'susfs_path=observed'))).toThrow();
        expect(() => parseFrontierReport(valid.replace('kmi=android15-6.6', 'kmi=android14-6.1'))).toThrow();
        expect(() => parseFrontierReport(valid.replace('kmi_signal=release_tag_match', 'kmi_signal=unresolved'))).toThrow();
        expect(() => parseFrontierReport(valid.replace('kernel_release=6.6.147-android15-9-gki', 'kernel_release=6.6.147-custom'))).toThrow();
        expect(() => parseFrontierReport(valid + 'unexpected=foo\n')).toThrow();
        expect(() => parseFrontierReport(valid + '\0')).toThrow();
        expect(() => parseFrontierReport(valid.repeat(200))).toThrow();
    });
});
