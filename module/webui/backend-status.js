// SPDX-License-Identifier: GPL-2.0-or-later
// The KPM page only displays evidence; it must never call a loader here.
import { exec } from 'kernelsu-alt';
import { modDir, escapeShell } from './constants.js';
import { parseFrontierReport, evaluateBackendPolicy } from './backend-policy.js';

let attached = false;
let busy = false;
function element(id) { return document.getElementById(id); }

function useChinese() {
    return (typeof navigator !== 'undefined' &&
        (navigator.language || '').toLowerCase().startsWith('zh'));
}

export async function refreshBackendDiagnosis() {
    const holder = element('backend-diagnostics-summary');
    if (!holder || busy) return;
    busy = true;
    const zh = useChinese();
    holder.textContent = zh ? '正在读取内核能力…' : 'Reading kernel capabilities…';
    try {
        const command = 'sh ' + escapeShell(modDir + '/tools/kpm_frontier_probe.sh');
        const result = await exec(command, { env: { PATH: '/system/bin' } });
        if (!result || result.errno !== 0) throw new Error('probe_unavailable');
        const state = evaluateBackendPolicy(parseFrontierReport(result.stdout));
        const provider = state.susfsSource === 'lkm_observed'
            ? (zh ? '已观察到 SUSFS-LKM' : 'SUSFS-LKM observed')
            : (state.susfsSource === 'control_interface_observed_provider_unknown'
                ? (zh ? '存在 SUSFS 接口，来源未验证' : 'SUSFS interface observed; source unverified')
                : (zh ? '未观察到 SUSFS；不代表不存在' : 'SUSFS not observed; absence unproven'));
        const parts = [
            provider,
            (zh ? '候选 KMI：' : 'KMI candidate: ') + state.kmi,
            (zh ? '控制节点：' : 'Control nodes: ') + state.susfsInterfaces.length,
            (zh ? 'Hook 所有权：未验证' : 'Hook ownership: unverified'),
            (zh ? '仅只读诊断，禁止自动加载' : 'Read-only only; loading prohibited'),
        ];
        holder.textContent = parts.join(' · ');
        holder.dataset.probeState = 'review_required';
    } catch (_error) {
        // Never interpret a failed probe as an absent or safe-to-install backend.
        holder.textContent = zh
            ? '无法验证后端能力。状态未知；禁止自动加载。'
            : 'Backend status unverified. Automatic loading prohibited.';
        holder.dataset.probeState = 'unavailable';
    } finally {
        busy = false;
    }
}

export function initBackendDiagnosis() {
    if (attached) return;
    attached = true;
    const refresh = element('backend-diagnostics-refresh');
    if (refresh) refresh.addEventListener('click', () => { void refreshBackendDiagnosis(); });
    void refreshBackendDiagnosis();
}
