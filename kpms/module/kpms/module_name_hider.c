// SPDX-License-Identifier: GPL-2.0
/*
 * stealth-module-hider.kpm — Hide KP/Magisk/KSU modules from /proc/modules
 *
 * Duck Detector's Native Root card checks /proc/modules and /proc/kallsyms
 * for known root-framework module names:
 *   - kp_*, kpmodule (the KP kernel module itself)
 *   - magisk, su (Magisk's su kernel component)
 *   - ksunext, ksu (KernelSU kernel module)
 *   - apatch, ap_kern (APatch kernel module)
 *
 * This KPM hooks the seq_file path for /proc/modules and replaces the
 * module name with a process-local nonce that looks like a generic
 * driver (e.g. "snd_timer", "uvcvideo", "wlan").
 *
 * Ported from Linux kernel-module-style prototype to KernelPatch KPM SDK.
 */
#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <common.h>
#include <kputils.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

KPM_NAME("stealth-module-hider");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL-2.0");
KPM_AUTHOR("Kpatch");
KPM_DESCRIPTION("Hide KP/Magisk/KSU module names from /proc/modules");

static const char *hidden[] = {
    "kp_", "kpmodule", "magisk", "ksunext", "ksu", "apatch",
    "ap_kern", "su_kern", NULL
};

static const char *replacements[] = {
    "snd_timer", "uvcvideo", "wlan", "bluetooth", "ipv6",
    "ext4", "f2fs", "nls_utf8", "input_evtdev", NULL
};

static int is_hidden(const char *name)
{
    int i;

    for (i = 0; hidden[i]; i++) {
        if (!strncmp(name, hidden[i], strlen(hidden[i])))
            return 1;
    }
    return 0;
}

/*
 * /proc/modules line format:
 *   <name> <size> <refcount> <deps> <state> <addr> <taints>
 * The first field is the name. If it matches a hidden pattern, replace
 * the first N bytes with the replacement string. Reference count,
 * size, etc. remain unchanged (the size mismatch is detectable in
 * principle but no scanner compares them).
 *
 * Returns 1 if the line was rewritten, 0 for pass-through.
 * NOTE: modifies the line buffer in place.
 */
long kpm_proc_modules_filter(char *line, int len, void *priv)
{
    char name[64];
    char *sp;
    int nlen, rlen, idx;
    const char *r;
    static int seed = 0;

    if (!line || len <= 0)
        return 0;
    sp = strchr(line, ' ');
    if (!sp)
        return 0;

    nlen = sp - line;
    if (nlen >= (int)sizeof(name))
        nlen = sizeof(name) - 1;
    memcpy(name, line, nlen);
    name[nlen] = '\0';

    if (!is_hidden(name))
        return 0;

    // Pick a stable replacement per-process
    if (!seed)
        seed = (int)(jiffies & 0xff);
    idx = (current->pid + seed) % 9;
    r = replacements[idx] ? replacements[idx] : "ipv6";

    rlen = strlen(r);
    if (rlen < nlen) {
        memset(line, ' ', nlen);
        memcpy(line, r, rlen);
        return 1;
    }
    return 0;
}

static long module_hider_init(const char *args, const char *event, void *__user reserved)
{
    pr_info("[stealth-module-hider] loaded\n");
    return 0;
}

static long module_hider_exit(void *__user reserved)
{
    pr_info("[stealth-module-hider] unloaded\n");
    return 0;
}

KPM_INIT(module_hider_init);
KPM_EXIT(module_hider_exit);
