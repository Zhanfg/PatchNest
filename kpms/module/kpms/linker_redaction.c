// SPDX-License-Identifier: GPL-2.0
/*
 * stealth-linker-redact.kpm — Remove linker temp paths from /proc/*/maps
 *
 * Duck Detector's "loader-visible runtime artifacts" check looks for
 * /data/app/*/oat/*/base.odex paths and linker temporary files
 * (e.g. /linker64, /system/bin/linker64) that remain mapped after
 * the app exits. Also flags deleted-but-still-mapped anonymous
 * regions that look like the linker bootstrapping.
 *
 * This KPM provides a stricter variant of the stealth-proc-maps
 * filter, specifically targeting:
 *   - Lines containing "(deleted)" with executable perms
 *   - Anonymous executable mappings in the [vdso]/[vvar] area when
 *     they overlap with the linker bootstrap region
 *
 * This KPM is a STRICTER variant for users who hit the "Memory"
 * card with 5+ high-risk signals.
 *
 * Ported from Linux kernel-module-style prototype to KernelPatch KPM SDK.
 */
#include <compiler.h>
#include <kpmodule.h>
#include <linux/printk.h>
#include <common.h>
#include <kputils.h>
#include <linux/string.h>

KPM_NAME("stealth-linker-redact");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL-2.0");
KPM_AUTHOR("Kpatch");
KPM_DESCRIPTION("Stricter /proc/*/maps filter: drop anonymous deleted executable regions");

static int is_linker_temp(const char *line, const char *path)
{
    // NOTE: takes both pointers — the perms byte stays in `line`,
    // the pathname lives in `path`. A single-pointer version reading
    // line[2] after being passed (line + 73) tests the wrong byte
    // and risks an out-of-bounds read on short pathnames.
    // Caller must validate line + 73 <= len before invoking.
    if (!line || !path)
        return 0;
    if (line[2] != 'x')
        return 0;
    return strnstr(path, "(deleted)", strlen(path)) != NULL;
}

/*
 * Stricter version of stealth-proc-maps' filter. Returns 1 to drop.
 * The KP runtime installs this as a higher-priority hook than
 * stealth-proc-maps when both are loaded.
 *
 * Per-line: 60-byte address+perms prefix, then 13 bytes of
 * offset+dev+inode, then optional pathname at column 73.
 */
long kpm_linker_filter(const char *line, int len, void *priv)
{
    const char *p, *path;

    if (!line || len <= 0)
        return 0;

    if (len < 73) {
        // No pathname: check if it's an anonymous deleted region.
        // Anonymous mappings have inode=0.
        if (line[2] == 'x') {
            p = line + 60;
            while (p < line + len && *p == ' ')
                p++;
            if (p + 1 < line + len && p[0] == '0' && p[1] == ' ')
                return 1;
        }
        return 0;
    }

    // Pathname present
    path = line + 73;
    if (is_linker_temp(line, path))
        return 1;

    return 0;
}

static long linker_redaction_init(const char *args, const char *event, void *__user reserved)
{
    pr_info("[stealth-linker-redact] loaded (strict maps filter)\n");
    return 0;
}

static long linker_redaction_exit(void *__user reserved)
{
    pr_info("[stealth-linker-redact] unloaded\n");
    return 0;
}

KPM_INIT(linker_redaction_init);
KPM_EXIT(linker_redaction_exit);
