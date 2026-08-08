/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#ifndef _KPU_SUPERCALL_H_
#define _KPU_SUPERCALL_H_

#include <unistd.h>
#include <sys/syscall.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "uapi/scdefs.h"
#include "../version"

/*
 * PatchNest ships two deliberately separate userspace build profiles.
 *
 * NEXT2026 is the upstream-derived default. PUBLIC1158 is a compatibility
 * build for Zhanfg/KernelPatch-Public 0.13.x. We do not auto-probe and then
 * issue mutations because several command IDs have incompatible meanings
 * across the two families (notably 0x1100/0x1101).
 */
#ifdef PATCHNEST_ABI_PUBLIC1158
#define PATCHNEST_ABI_NAME "public1158"
#define PATCHNEST_SC_TOKEN 0x1158u
#define PATCHNEST_HELLO_MAGIC 0x11581158u
#define PATCHNEST_HELLO_ECHO "hello1158"
#define PATCHNEST_REHOOK_SUPPORTED 0
#define PATCHNEST_DEFAULT_KEY_FILE "/data/adb/patchnest/superkey"
#else
#define PATCHNEST_ABI_NAME "next2026"
#define PATCHNEST_SC_TOKEN 0x2026u
#define PATCHNEST_HELLO_MAGIC 0x20262026u
#define PATCHNEST_HELLO_ECHO "hello2026"
#define PATCHNEST_REHOOK_SUPPORTED 1
#endif

static inline const char *sc_abi_name(void)
{
    return PATCHNEST_ABI_NAME;
}

static inline uint32_t sc_expected_hello_magic(void)
{
    return PATCHNEST_HELLO_MAGIC;
}

static inline const char *sc_expected_hello_echo(void)
{
    return PATCHNEST_HELLO_ECHO;
}

static inline bool sc_rehook_supported(void)
{
    return PATCHNEST_REHOOK_SUPPORTED != 0;
}

#ifdef PATCHNEST_ABI_PUBLIC1158
static inline const char *sc_public_key(void)
{
    const char *env_key = getenv("PATCHNEST_SUPERKEY");
    if (env_key) {
        size_t len = strnlen(env_key, SUPERCALL_KEY_MAX_LEN);
        if (len > 0 && len < SUPERCALL_KEY_MAX_LEN)
            return env_key;
        return NULL;
    }

    static char file_key[SUPERCALL_KEY_MAX_LEN];
    FILE *fp = fopen(PATCHNEST_DEFAULT_KEY_FILE, "r");
    if (!fp)
        return NULL;

    if (!fgets(file_key, sizeof(file_key), fp)) {
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    file_key[strcspn(file_key, "\r\n")] = '\0';
    size_t len = strnlen(file_key, sizeof(file_key));
    if (len == 0 || len >= sizeof(file_key)) {
        memset(file_key, 0, sizeof(file_key));
        return NULL;
    }
    return file_key;
}
#else
static inline const char *sc_public_key(void)
{
    return NULL;
}
#endif

static inline const char *sc_key(void)
{
#ifdef PATCHNEST_ABI_PUBLIC1158
    return sc_public_key();
#else
    return NULL;
#endif
}

static inline long sc_normalize_syscall_result(long rc)
{
    /*
     * Linux kernel syscall handlers return -errno, but libc syscall(2)
     * wrappers (including Android/Bionic) expose that as -1 and store the
     * original error in errno. Convert it back immediately so higher layers
     * can use a stable, libc-independent negative errno contract.
     */
    if (rc != -1)
        return rc;

    int error = errno;
    return error > 0 ? -(long)error : -EIO;
}

static inline long ver_and_cmd(long cmd)
{
    uint32_t version_code = (MAJOR << 16) + (MINOR << 8) + PATCH;
    return ((long)version_code << 32) | ((long)PATCHNEST_SC_TOKEN << 16) | (cmd & 0xFFFF);
}

static inline long compact_cmd(long cmd)
{
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key)
        return ver_and_cmd(cmd);
#endif
    long ver = sc_normalize_syscall_result(
        syscall(__NR_supercall, key, ver_and_cmd(SUPERCALL_KERNELPATCH_VER)));
    if (ver >= 0xa05) return ver_and_cmd(cmd);
    return cmd;
}

static inline long sc_hello(void)
{
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key)
        return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_HELLO)));
}

static inline bool sc_ready(void)
{
    return sc_hello() == (long)sc_expected_hello_magic();
}

static inline long sc_klog(const char *msg)
{
    if (!msg || strlen(msg) <= 0) return -EINVAL;
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_KLOG), msg));
}

static inline uint32_t sc_kp_ver(void)
{
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return 0;
#endif
    long ret = sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_KERNELPATCH_VER)));
    return ret < 0 ? 0u : (uint32_t)ret;
}

static inline uint32_t sc_k_ver(void)
{
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return 0;
#endif
    long ret = sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_KERNEL_VER)));
    return ret < 0 ? 0u : (uint32_t)ret;
}

static inline long sc_kpm_load(const char *path, const char *args, void *reserved)
{
    if (!path || strlen(path) <= 0) return -EINVAL;
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_KPM_LOAD), path, args, reserved));
}

static inline long sc_kpm_control(const char *name, const char *ctl_args, char *out_msg, long outlen)
{
    if (!name || strlen(name) <= 0) return -EINVAL;
    if (!ctl_args || strlen(ctl_args) <= 0) return -EINVAL;
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_KPM_CONTROL), name, ctl_args, out_msg, outlen));
}

static inline long sc_kpm_unload(const char *name, void *reserved)
{
    if (!name || strlen(name) <= 0) return -EINVAL;
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_KPM_UNLOAD), name, reserved));
}

static inline long sc_kpm_nums(void)
{
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_KPM_NUMS)));
}

static inline long sc_kpm_list(char *names_buf, int buf_len)
{
    if (!names_buf || buf_len <= 0) return -EINVAL;
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_KPM_LIST), names_buf, buf_len));
}

static inline long sc_kpm_info(const char *name, char *buf, int buf_len)
{
    if (!buf || buf_len <= 0) return -EINVAL;
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_KPM_INFO), name, buf, buf_len));
}

static inline long sc_bootlog(void)
{
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_BOOTLOG)));
}

static inline long sc_panic(void)
{
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, compact_cmd(SUPERCALL_PANIC)));
}

static inline long sc_kstorage_read(int gid, long did, void *out_data, int offset, int dlen)
{
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, ver_and_cmd(SUPERCALL_KSTORAGE_READ), gid, did,
                out_data, (((long)offset << 32) | dlen)));
}

static inline long sc_kstorage_write(int gid, long did, void *data, int offset, int dlen)
{
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, ver_and_cmd(SUPERCALL_KSTORAGE_WRITE), gid, did,
                data, (((long)offset << 32) | dlen)));
}

static inline long sc_kstorage_remove(int gid, long did)
{
    const char *key = sc_key();
#ifdef PATCHNEST_ABI_PUBLIC1158
    if (!key) return -EACCES;
#endif
    return sc_normalize_syscall_result(
        syscall(__NR_supercall, key, ver_and_cmd(SUPERCALL_KSTORAGE_REMOVE), gid, did));
}

static inline long sc_set_ap_mod_exclude(uid_t uid, int exclude)
{
    if(exclude) {
        return sc_kstorage_write(KSTORAGE_EXCLUDE_LIST_GROUP, uid, &exclude, 0, sizeof(exclude));
    } else {
        return sc_kstorage_remove(KSTORAGE_EXCLUDE_LIST_GROUP, uid);
    }
}

static inline long sc_get_ap_mod_exclude(uid_t uid)
{
    int exclude = 0;
    long rc = sc_kstorage_read(KSTORAGE_EXCLUDE_LIST_GROUP, uid, &exclude, 0, sizeof(exclude));
    if (rc < 0) return rc;
    return exclude ? 1 : 0;
}

static inline int sc_rehook_syscall(int enable)
{
    if (!sc_rehook_supported())
        return -EOPNOTSUPP;
    return (int)sc_normalize_syscall_result(
        syscall(__NR_supercall, sc_key(), ver_and_cmd(SUPERCALL_REHOOK_SYSCALL), (long)enable));
}

static inline int sc_rehook_status(void)
{
    if (!sc_rehook_supported())
        return -EOPNOTSUPP;
    return (int)sc_normalize_syscall_result(
        syscall(__NR_supercall, sc_key(), ver_and_cmd(SUPERCALL_REHOOK_STATUS)));
}

#endif