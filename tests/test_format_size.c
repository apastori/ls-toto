/*
 * test_format_size.c — ls_toto_format_size_human() rounding.
 *
 * Chain of thought:
 *   - -h rounds up to the next tenth below 10 and to the next unit from 10
 *     up, using powers of 1024; sizes below 1024 print as plain bytes.
 *   - Boundaries around 1 KiB, 10 KiB, and 1.5 MiB pin down both regimes.
 *   - Heap: none.
 *   - Standard: C11 + <assert.h>.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ls_toto_format.h"
#include "test_format_size.h"

static void check(long long size, const char *want)
{
    char out[16];

    ls_toto_format_size_human(size, out, sizeof out);
    assert(strcmp(out, want) == 0);
}

void test_format_size_run(void)
{
    check(0, "0");
    check(1023, "1023");
    printf("PASS: format_size plain bytes below 1024\n");

    check(1024, "1.0K");
    check(1025, "1.1K");
    check(1536, "1.5K");
    printf("PASS: format_size one decimal below 10\n");

    check(10240, "10K");
    check(10241, "11K");
    check(1572864, "1.5M");
    printf("PASS: format_size whole units from 10 up\n");
}
