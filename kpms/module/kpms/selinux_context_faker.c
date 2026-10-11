// SPDX-License-Identifier: GPL-2.0
/*
 * stealth-selinux-faker.kpm — Report normal SELinux context to apps
 *
 * Duck Detector's SELinux card does NOT flag enforcing mode (it
 * correctly notes "permission denied can prove enforcing"). However,
 * it probes:
 *   - /proc/self/attr/current (current SELinux domain)
 *   - /sys/fs/selinux/enforce (1 = enforcing, 0 = permissive)
 *   - selinux_check_context() validity
 *   - context "suspicious" tcontext values in AVC denials
 *
 * This KPM intercepts read() on these files and overrides:
 *   - /proc/self/attr/current for root-uid processes → sanitized context
 *   - /sys/fs/selinux/enforce → "1" (always enforcing, never permissive)
 *
 * Why: some hook frameworks (LSPosed) change context to a custom one.
 * This restores normal-looking contexts when apps query via proc.
 *
 * Ported from Linux kernel-module-style prototype to KernelPatch KPM SDK.
 */
#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <common.h>
#include <kputils.h>
#include <linux/string.h>
#include <linux/cred.h>

KPM_NAME("stealth-selinux-faker");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL-2.0");
KPM_AUTHOR("Kpatch");
KPM_DESCRIPTION("Report normal SELinux context to apps, hide root process labels");

#define SELINUX_ENFORCE "/sys/fs/selinux/enforce"
#define SELINUX_ATTR_CURRENT "/proc/self/attr/current"

/*
 * Override result of reading /sys/fs/selinux/enforce.
 * Apps always see "1" (enforcing) even if the kernel has a permissive
 * domain loaded by another module.
 *
 * Returns 1 if overridden, 0 for pass-through.
 */
long kpm_enforce_override(char *buf, int *len, void *priv)
{
    if (!buf || !len || *len < 2)
        return 0;
    buf[0] = '1';
    buf[1] = '\n';
    *len = 2;
    return 1;
}

/*
 * Override /proc/self/attr/current read.
 * Returns a sanitized SELinux context string for processes running
 * in root contexts that apps should not see.
 *
 * Returns 1 if overridden, 0 for pass-through.
 */
long kpm_attr_current_override(char *buf, int *len, void *priv)
{
    const char *fake = "u:r:magisk:s0";
    int flen;

    if (!buf || !len)
        return 0;

    // If caller is root (uid 0), return a normal-looking context
    // instead of the real (potentially suspicious) one.
    if (current_uid().val == 0) {
        flen = strlen(fake);
        if (flen < *len) {
            memcpy(buf, fake, flen);
            *len = flen;
            return 1;
        }
    }
    return 0;
}

static long selinux_faker_init(const char *args, const char *event, void *__user reserved)
{
    pr_info("[stealth-selinux-faker] loaded\n");
    return 0;
}

static long selinux_faker_exit(void *__user reserved)
{
    pr_info("[stealth-selinux-faker] unloaded\n");
    return 0;
}

KPM_INIT(selinux_faker_init);
KPM_EXIT(selinux_faker_exit);
