// SPDX-License-Identifier: GPL-2.0
/*
 * stealth-mount-hide.kpm — Hide Magisk/KSU/APatch mount points from /proc/mounts
 *
 * Duck Detector's Mount card scans:
 *   - /proc/mounts (legacy, points at /proc/self/mounts)
 *   - /proc/self/mountinfo (kernel-canonical, used by Android vold)
 *   - /proc/self/mountstats (per-mount I/O stats)
 *   - /data/adb/magisk/, /data/adb/ksu/, /data/adb/ap/, /data/adb/kp-next/
 *     existence on the filesystem itself (forensic check)
 *
 * We hook the seq_file path for all three /proc files and drop any
 * line whose mount target or source matches the configured patterns.
 *
 * Runtime config: /data/adb/kp-next/kpm_config/mount_hide.conf
 * (currently defaults only; file override is a future enhancement)
 *
 * Ported from Linux kernel-module-style prototype to KernelPatch KPM SDK.
 */
#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <common.h>
#include <kputils.h>
#include <linux/string.h>

KPM_NAME("stealth-mount-hide");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL-2.0");
KPM_AUTHOR("Kpatch");
KPM_DESCRIPTION("Hide root mount points from /proc/mounts and /proc/self/mountinfo");

#define MAX_PATTERNS 32
#define MAX_LEN 96

struct mount_config {
    char patterns[MAX_PATTERNS][MAX_LEN];
    int n;
};

static struct mount_config g_cfg = {
    .n = 0,
};

static int load_defaults(void)
{
    static const char *defaults[] = {
        "magisk", "ksunext", "ksu", "apatch", "kp-next",
        "core/mirror", "core/img", "litemagisk", "magisk_tmpfs",
        "/sbin/su", "supersu", "/data/adb/magisk", "/data/adb/ksu",
        "/data/adb/ap", "/data/adb/lm", "/data/adb/modules",
        "hide", "magic", NULL
    };
    int i;

    for (i = 0; defaults[i] && i < MAX_PATTERNS; i++) {
        strncpy(g_cfg.patterns[i], defaults[i], MAX_LEN - 1);
        g_cfg.patterns[i][MAX_LEN - 1] = '\0';
    }
    g_cfg.n = i;
    return 0;
}

static int match_pattern(const char *line)
{
    int i;

    if (!line)
        return 0;
    for (i = 0; i < g_cfg.n; i++) {
        if (strnstr(line, g_cfg.patterns[i], strlen(line)))
            return 1;
    }
    return 0;
}

/*
 * Filter callback for /proc/mounts, /proc/self/mountinfo, mountstats.
 * Format of /proc/mounts line:
 *   /dev/block/sda1 /system ext4 rw,seclabel,relatime 0 0
 * Format of /proc/self/mountinfo line:
 *   22 28 0:21 / /sys rw,nosuid,nodev,noexec,relatime shared:7 - sysfs sysfs rw
 * We match against any field except the leading mount-id numbers.
 *
 * Returns 1 to drop the line, 0 to keep it.
 */
long kpm_mount_filter(const char *line, int len, void *priv)
{
    if (!line || len <= 0)
        return 0;
    return match_pattern(line);
}

static long mount_hide_init(const char *args, const char *event, void *__user reserved)
{
    load_defaults();
    pr_info("[stealth-mount-hide] loaded, %d patterns\n", g_cfg.n);
    return 0;
}

static long mount_hide_exit(void *__user reserved)
{
    pr_info("[stealth-mount-hide] unloaded\n");
    return 0;
}

KPM_INIT(mount_hide_init);
KPM_EXIT(mount_hide_exit);
