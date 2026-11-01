// SPDX-License-Identifier: GPL-2.0-or-later
// Strict, read-only PatchNest multi-backend policy evaluator v1.
// This is NOT a kernel hook lease, root authorization, or a loader.
export const FRONTIER_PROTOCOL = 'KPM_FRONTIER_V2';
export const BROKER_PROTOCOL = 'PATCHNEST_BACKEND_POLICY_V1';

const keys = new Set([
    'patchnest_probe', 'kernel_release', 'kmi', 'kmi_signal',
    'susfs_lkm_build_target', 'susfs_control', 'susfs_path', 'susfs_kstat',
    'susfs_open_redirect', 'susfs_hide_mounts', 'susfs_hide_modules',
    'susfs_map', 'susfs_opath', 'btf_vmlinux', 'selinux_enforcing',
    'susfs_lkm', 'kernelpatch_lkm', 'module_loading_policy',
    'coexistence', 'syscall_45_ownership', 'kpm_abi',
    'module_loading', 'conclusion',
]);
const observations = [
    'susfs_control', 'susfs_path', 'susfs_kstat',
    'susfs_open_redirect', 'susfs_hide_mounts', 'susfs_hide_modules',
    'susfs_map', 'susfs_opath', 'btf_vmlinux',
    'susfs_lkm', 'kernelpatch_lkm',
];
const allowedKmi = new Set([
    'unresolved', 'android12-5.10', 'android13-5.10', 'android13-5.15',
    'android14-5.15', 'android14-6.1', 'android15-6.6',
    'android16-6.12', 'android17-6.18',
]);

// Independently cross-check the shell's candidate KMI against the kernel
// release text. A parsed field is not a trusted ABI attestation.
function expectedTaggedKmi(release) {
    const match = /^(5\.10|5\.15|6\.1|6\.6|6\.12|6\.18)\.[0-9]+-android(12|13|14|15|16|17)-[A-Za-z0-9._+:-]+$/.exec(release);
    if (!match) return 'unresolved';
    const candidate = 'android' + match[2] + '-' + match[1];
    return allowedKmi.has(candidate) ? candidate : 'unresolved';
}

function oneOf(value, choices, key) {
    if (!choices.includes(value)) throw new Error('Invalid field: ' + key);
}
export function parseFrontierReport(raw) {
    if (typeof raw !== 'string' || raw.length > 8192 || raw.includes('\0')) {
        throw new Error('Invalid probe input');
    }
    const data = Object.create(null);
    const lines = raw.replace(/\r\n/g, '\n').split('\n');
    if (lines.length > 40) throw new Error('Probe output too long');
    for (const line of lines) {
        if (!line) continue;
        const match = /^([a-z0-9_]+)=([A-Za-z0-9_.:+-]{1,160})$/.exec(line);
        if (!match || !keys.has(match[1]) || Object.hasOwn(data, match[1])) {
            throw new Error('Malformed or duplicate probe field');
        }
        data[match[1]] = match[2];
    }
    for (const key of keys) {
        if (!Object.hasOwn(data, key)) throw new Error('Missing field: ' + key);
    }
    oneOf(data.patchnest_probe, [FRONTIER_PROTOCOL], 'patchnest_probe');
    oneOf(data.kmi_signal, ['release_tag_match', 'unresolved'], 'kmi_signal');
    if (!allowedKmi.has(data.kmi)) throw new Error('Invalid KMI');
    if (data.kmi !== expectedTaggedKmi(data.kernel_release)) {
        throw new Error('KMI does not match observed kernel release');
    }
    if ((data.kmi === 'unresolved') !== (data.kmi_signal === 'unresolved')) {
        throw new Error('Inconsistent KMI evidence');
    }
    oneOf(data.susfs_lkm_build_target, ['listed', 'unverified'], 'susfs_lkm_build_target');
    if ((data.kmi_signal === 'unresolved') !==
        (data.susfs_lkm_build_target === 'unverified')) {
        throw new Error('Inconsistent build target evidence');
    }
    for (const key of observations) {
        oneOf(data[key], ['observed', 'not_observed'], key);
    }
    const controlExpected = data.susfs_path === 'observed' ||
        data.susfs_kstat === 'observed' ? 'observed' : 'not_observed';
    if (data.susfs_control !== controlExpected) {
        throw new Error('Inconsistent SUSFS control observation');
    }
    oneOf(data.selinux_enforcing, ['0', '1', 'unknown'], 'selinux_enforcing');
    oneOf(data.module_loading_policy, ['unknown', 'not_disabled', 'disabled'], 'module_loading_policy');
    oneOf(data.coexistence, ['unverified', 'multiple_lkms_observed_unverified'], 'coexistence');
    const multi = data.susfs_lkm === 'observed' && data.kernelpatch_lkm === 'observed';
    if ((data.coexistence === 'multiple_lkms_observed_unverified') !== multi) {
        throw new Error('Inconsistent coexistence evidence');
    }
    for (const field of ['syscall_45_ownership', 'kpm_abi']) {
        oneOf(data[field], ['unverified'], field);
    }
    oneOf(data.module_loading, ['never'], 'module_loading');
    oneOf(data.conclusion, ['manual_review_required'], 'conclusion');
    return Object.freeze({ ...data });
}

export function evaluateBackendPolicy(data) {
    // This policy never grants a kernel mutation. Lack of visible evidence
    // cannot prove that an owner or hidden module is absent.
    const interfaceKeys = [
        'susfs_path', 'susfs_kstat', 'susfs_open_redirect',
        'susfs_hide_mounts', 'susfs_hide_modules', 'susfs_map', 'susfs_opath',
    ];
    const seen = interfaceKeys.filter(k => data[k] === 'observed');
    const susfsLkm = data.susfs_lkm === 'observed';
    const kpLkm = data.kernelpatch_lkm === 'observed';
    const anySusfs = susfsLkm || seen.length > 0;
    const susfsSource = susfsLkm
        ? 'lkm_observed'
        : (seen.length ? 'control_interface_observed_provider_unknown' : 'not_observed');
    const resources = Object.freeze([
        { name: 'root_core', owner: 'external_or_unverified', lease: 'unverified' },
        { name: 'syscall_dispatch', owner: 'unverified', lease: 'unverified' },
        { name: 'vfs_lsm', owner: anySusfs ? 'susfs_evidence_observed' : 'unverified', lease: 'unverified' },
        { name: 'kpm_callback', owner: 'unverified', lease: 'unverified' },
    ]);
    const decisions = Object.freeze({
        inspect: 'READ_ONLY',
        existing_susfs_control: seen.length ? 'OBSERVED_NOT_AUTHORIZED' : 'NOT_OBSERVED',
        install_susfs_lkm: anySusfs ? 'DENY_DUPLICATE_OR_UNKNOWN_PROVIDER' : 'DENY_UNVERIFIED_KMI_AND_OWNER',
        install_kernelpatch_lkm: kpLkm ? 'DENY_DUPLICATE_OR_UNKNOWN_PROVIDER' : 'DENY_UNVERIFIED_SYSCALL_OWNER',
        register_vfs_hook: 'DENY_NO_KERNEL_OWNERSHIP_LEASE',
        register_syscall_hook: 'DENY_NO_KERNEL_OWNERSHIP_LEASE',
        kpm_control: 'DENY_UNVERIFIED_KPM_ABI',
        write_susfs_rules: 'DENY_UNVERIFIED_SUSFS_CONTROL_ABI',
    });
    return Object.freeze({
        protocol: BROKER_PROTOCOL,
        mode: 'read_only',
        verdict: 'manual_review_required',
        kernel: data.kernel_release,
        kmi: data.kmi,
        kmiConfidence: data.kmi_signal === 'release_tag_match' ? 'tag_only' : 'unresolved',
        susfsSource,
        susfsInterfaces: Object.freeze(seen),
        kernelpatchLkm: kpLkm ? 'observed' : 'not_observed',
        btf: data.btf_vmlinux,
        selinux: data.selinux_enforcing,
        resources,
        decisions,
    });
}
