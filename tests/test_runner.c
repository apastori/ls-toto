/*
 * test_runner.c — unit-test entry point (build/tests/test_core).
 *
 * Chain of thought:
 *   - Runs every suite in a fixed order: cli_parse, sort_order, format_mode,
 *     format_size, fs_readdir.
 *   - Suites use assert(); a failure aborts and make test exits non-zero.
 *   - Heap: none.
 *   - Standard: C11.
 */
#include <stdio.h>

#include "test_cli_parse.h"
#include "test_format_mode.h"
#include "test_format_size.h"
#include "test_fs_readdir.h"
#include "test_sort_order.h"

int main(void)
{
    test_cli_parse_run();
    test_sort_order_run();
    test_format_mode_run();
    test_format_size_run();
    test_fs_readdir_run();
    printf("All tests passed.\n");
    return 0;
}
