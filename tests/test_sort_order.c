/*
 * test_sort_order.c — ls_toto_sort_entries() ordering.
 *
 * Chain of thought:
 *   - Entries are built on the stack with string-literal names; the sorter
 *     only reads name, mtime, and size.
 *   - Covers strcmp name order (uppercase before lowercase in the C locale),
 *     -t and -S with name tie-breaks, and -r for each key.
 *   - Heap: none.
 *   - Standard: C11 + <assert.h>.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ls_toto_sort.h"
#include "test_sort_order.h"

static void set(struct ls_toto_entry *e, char *name, time_t mtime,
                long long size)
{
    memset(e, 0, sizeof *e);
    e->name = name;
    e->mtime = mtime;
    e->size = size;
}

static void expect(const struct ls_toto_entry *e, const char *a,
                   const char *b, const char *c)
{
    assert(strcmp(e[0].name, a) == 0);
    assert(strcmp(e[1].name, b) == 0);
    assert(strcmp(e[2].name, c) == 0);
}

static void by_name(void)
{
    struct ls_toto_entry e[3];
    char b[] = "b", a[] = "a", C[] = "C";

    set(&e[0], b, 0, 0);
    set(&e[1], a, 0, 0);
    set(&e[2], C, 0, 0);
    ls_toto_sort_entries(e, 3, LS_TOTO_SORT_NAME, 0);
    expect(e, "C", "a", "b");
    ls_toto_sort_entries(e, 3, LS_TOTO_SORT_NAME, 1);
    expect(e, "b", "a", "C");
    printf("PASS: sort_order name and reverse\n");
}

static void by_time(void)
{
    struct ls_toto_entry e[3];
    char old[] = "old", y[] = "y", x[] = "x";

    set(&e[0], old, 100, 0);
    set(&e[1], y, 200, 0);
    set(&e[2], x, 200, 0);
    ls_toto_sort_entries(e, 3, LS_TOTO_SORT_TIME, 0);
    expect(e, "x", "y", "old");
    ls_toto_sort_entries(e, 3, LS_TOTO_SORT_TIME, 1);
    expect(e, "old", "y", "x");
    printf("PASS: sort_order mtime (newest first, name tie-break) and "
           "reverse\n");
}

static void by_size(void)
{
    struct ls_toto_entry e[3];
    char small[] = "small", q[] = "q", p[] = "p";

    set(&e[0], small, 0, 10);
    set(&e[1], q, 0, 500);
    set(&e[2], p, 0, 500);
    ls_toto_sort_entries(e, 3, LS_TOTO_SORT_SIZE, 0);
    expect(e, "p", "q", "small");
    ls_toto_sort_entries(e, 3, LS_TOTO_SORT_SIZE, 1);
    expect(e, "small", "q", "p");
    printf("PASS: sort_order size (largest first, name tie-break) and "
           "reverse\n");
}

void test_sort_order_run(void)
{
    by_name();
    by_time();
    by_size();
}
