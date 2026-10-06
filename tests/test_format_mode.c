/*
 * test_format_mode.c — ls_toto_format_mode() 10-character mode strings.
 *
 * Chain of thought:
 *   - The formatter takes a platform-neutral type plus permission bits, so
 *     these cases run on every platform, Win32 builds included.
 *   - Covers plain permissions, symlinks, setuid with and without the
 *     execute bit, and the sticky bit on a directory.
 *   - Heap: none.
 *   - Standard: C11 + <assert.h>.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ls_toto_format.h"
#include "test_format_mode.h"

static void check(enum ls_toto_ftype type, unsigned int perm,
                  const char *want)
{
    char out[11];

    ls_toto_format_mode(type, perm, out);
    assert(strcmp(out, want) == 0);
}

void test_format_mode_run(void)
{
    check(LS_TOTO_FT_DIR, 0755, "drwxr-xr-x");
    check(LS_TOTO_FT_REG, 0644, "-rw-r--r--");
    check(LS_TOTO_FT_LNK, 0777, "lrwxrwxrwx");
    printf("PASS: format_mode basic permissions\n");

    check(LS_TOTO_FT_REG, 04755, "-rwsr-xr-x");
    check(LS_TOTO_FT_REG, 04644, "-rwSr--r--");
    check(LS_TOTO_FT_DIR, 01777, "drwxrwxrwt");
    printf("PASS: format_mode setuid / sticky bits\n");
}
