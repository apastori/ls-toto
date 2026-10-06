/*
 * test_fs_readdir.c — POSIX opendir()/readdir() sanity.
 *
 * Chain of thought:
 *   - ls_toto_fs.c is not linked into the tests, so this checks the POSIX
 *     primitives it relies on directly: every directory lists "." and "..",
 *     and opening a missing path fails with ENOENT.
 *   - Win32 filesystem builds have no <dirent.h> guarantee; the case prints
 *     SKIP there.
 *   - Heap: none.
 *   - Standard: C11 + POSIX.1-2008 (<dirent.h>).
 */
#include <stdio.h>

#include "test_fs_readdir.h"

#ifndef LS_TOTO_WIN32_FS

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

void test_fs_readdir_run(void)
{
    DIR *d = opendir(".");
    struct dirent *de;
    int saw_dot = 0;
    int saw_dotdot = 0;

    assert(d != NULL);
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0) {
            saw_dot = 1;
        } else if (strcmp(de->d_name, "..") == 0) {
            saw_dotdot = 1;
        }
    }
    closedir(d);
    assert(saw_dot && saw_dotdot);
    printf("PASS: fs_readdir lists . and ..\n");

    errno = 0;
    d = opendir("does-not-exist");
    assert(d == NULL);
    assert(errno == ENOENT);
    printf("PASS: fs_readdir missing directory -> ENOENT\n");
}

#else

void test_fs_readdir_run(void)
{
    printf("SKIP: fs_readdir\n");
}

#endif
