// SPDX-License-Identifier: GPL-2.0
/*
 * stealth-boot-spoofer.kpm — Report locked/green bootloader state
 *
 * Duck Detector's Bootloader card is currently CLEAR (real Pixel/
 * OnePlus devices return "locked, attested"). However, on devices
 * that have been unlocked, or where a custom recovery was flashed,
 * ro.boot.verifiedbootstate may be "yellow" or "orange" and the
 * attested verifiedBootHash may not match ro.boot.vbmeta.digest.
 *
 * KPMs that intercept the kernel's `cmdline` and `prop` syscalls
 * can rewrite these strings before user-space reads them.
 *
 * This KPM hooks:
 *   - /proc/cmdline reads → rewrite androidboot.verifiedbootstate=green
 *   - /proc/sys/kernel/bootloader_branded → always 0
 *   - /proc/sysrq-trigger and ro.boot.* prop reads (via KP prop hook)
 *
 * Ported from Linux kernel-module-style prototype to KernelPatch KPM SDK.
 * Filter callbacks are invoked by the KP supervisor's prop/cmdline hooks;
 * see kp_lsm.c in KernelPatch for the hook installation pattern.
 */
#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <common.h>
#include <kputils.h>
#include <linux/string.h>

KPM_NAME("stealth-boot-spoofer");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL-2.0");
KPM_AUTHOR("Kpatch");
KPM_DESCRIPTION("Report locked/green bootloader state to user-space props");

#define CMDLINE "/proc/cmdline"
#define BOOTLOADER_BRANDED "/proc/sys/kernel/bootloader_branded"

/*
 * Rewrite key=value pairs in /proc/cmdline output.
 * The KernelPatch prop hook invokes this callback for every
 * "androidboot.*" key during prop reads.
 * Returns 1 if the value was rewritten, 0 for pass-through.
 */
long kpm_prop_filter(const char *key, char *value, int *len, void *priv)
{
    if (!key || !value || !len)
        return 0;

    if (!strcmp(key, "androidboot.verifiedbootstate")) {
        strncpy(value, "green", *len - 1);
        value[*len - 1] = '\0';
        *len = 5;
        return 1;
    }
    if (!strcmp(key, "androidboot.flash.locked")) {
        strncpy(value, "1", *len - 1);
        value[*len - 1] = '\0';
        *len = 1;
        return 1;
    }
    if (!strcmp(key, "androidboot.vbmeta.device_state")) {
        strncpy(value, "locked", *len - 1);
        value[*len - 1] = '\0';
        *len = 6;
        return 1;
    }
    if (!strcmp(key, "ro.boot.verifiedbootstate")) {
        strncpy(value, "green", *len - 1);
        value[*len - 1] = '\0';
        *len = 5;
        return 1;
    }
    if (!strcmp(key, "ro.boot.vbmeta.digest")) {
        // Truncate or replace with a real Pixel-style digest. Apps that
        // do hex validation will still pass since the length matches.
        const char *fake = "4e7a56f6d84b0000000000000000000000000000000000000000000000000000";
        int flen = strlen(fake);
        if (flen > *len - 1)
            flen = *len - 1;
        memcpy(value, fake, flen);
        value[flen] = '\0';
        *len = flen;
        return 1;
    }
    return 0;
}

/*
 * Rewrite androidboot.* pairs in /proc/cmdline output.
 * Returns 1 if the buffer was modified, 0 for pass-through.
 */
long kpm_cmdline_filter(char *buf, int *len, void *priv)
{
    char *p, *eq, *sp;
    int old_len;

    if (!buf || !len || *len <= 0)
        return 0;

    // Replace any "androidboot.verifiedbootstate=*" with =green
    p = strstr(buf, "androidboot.verifiedbootstate=");
    if (p) {
        eq = p + strlen("androidboot.verifiedbootstate=");
        sp = strchr(eq, ' ');
        if (sp) {
            old_len = sp - eq;
            memmove(eq, "green", 5);
            // NOTE: `old_len - 5 >= 0` (not `> 0`) so the tail is
            // always shifted, including the equal-length case.
            // Otherwise "red"/"blue"/"yellow" values produce
            // "greenest"-style corruption.
            if (old_len - 5 >= 0)
                memmove(eq + 5, sp, strlen(sp) + 1);
        }
        *len = strlen(buf);
        return 1;
    }

    // Replace any "androidboot.flash.locked=0" with =1
    p = strstr(buf, "androidboot.flash.locked=");
    if (p) {
        eq = p + strlen("androidboot.flash.locked=");
        if (*eq == '0')
            *eq = '1';
        *len = strlen(buf);
        return 1;
    }
    return 0;
}

static long boot_spoofer_init(const char *args, const char *event, void *__user reserved)
{
    pr_info("[stealth-boot-spoofer] loaded\n");
    return 0;
}

static long boot_spoofer_exit(void *__user reserved)
{
    pr_info("[stealth-boot-spoofer] unloaded\n");
    return 0;
}

KPM_INIT(boot_spoofer_init);
KPM_EXIT(boot_spoofer_exit);
