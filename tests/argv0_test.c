/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int patchnest_cli_main(int argc, char **argv);

uint32_t version(void) { return 0; }
void hello(void) {}
void kpv(void) {}
void kv(void) {}
void bootlog(void) {}
void panic(void) {}
int kpm_main(int argc, char **argv) { (void)argc; (void)argv; return 0; }
int kpexclude_set_main(int argc, char **argv) { (void)argc; (void)argv; return 0; }
int kpexclude_get_main(int argc, char **argv) { (void)argc; (void)argv; return 0; }
int kprehook_main(int argc, char **argv) { (void)argc; (void)argv; return 0; }
int kprehook_status_main(int argc, char **argv) { (void)argc; (void)argv; return 0; }

static void run_case(size_t len)
{
    char *name = malloc(len + 1);
    assert(name != NULL);
    memset(name, 'p', len);
    name[len] = '\0';

    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        FILE *sink = freopen("/dev/null", "w", stdout);
        (void)sink;
        char *argv[] = { name, "--help", NULL };
        int rc = patchnest_cli_main(2, argv);
        _exit(rc);
    }

    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
    free(name);
}

int main(void)
{
    run_case(1024);
    run_case(65536);
    puts("argv[0] boundary tests: PASS");
    return 0;
}
