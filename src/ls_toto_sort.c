/*
 * ls_toto_sort.c — ordering of listing entries.
 *
 * Chain of thought:
 *   - Responsibility: sort struct ls_toto_entry arrays by name, mtime, or
 *     size, optionally reversed.
 *   - C11 qsort() has no context argument (qsort_r is not portable), so each
 *     key/direction pair gets its own comparator instead of global state.
 *   - Name order is strcmp (C locale only; no setlocale / strcoll).
 *   - Time and size sorts break ties by name, as GNU ls does; -r reverses
 *     the whole comparison, tie-breaks included.
 *   - Syscalls: none (pure, linked into tests). Heap: none.
 *   - Standard: C11.
 */
#include <stdlib.h>
#include <string.h>

#include "ls_toto_sort.h"

static int cmp_name(const void *a, const void *b)
{
    const struct ls_toto_entry *x = a;
    const struct ls_toto_entry *y = b;
    return strcmp(x->name, y->name);
}

static int cmp_time(const void *a, const void *b)
{
    const struct ls_toto_entry *x = a;
    const struct ls_toto_entry *y = b;

    if (x->mtime != y->mtime) {
        return x->mtime > y->mtime ? -1 : 1;
    }
    return strcmp(x->name, y->name);
}

static int cmp_size(const void *a, const void *b)
{
    const struct ls_toto_entry *x = a;
    const struct ls_toto_entry *y = b;

    if (x->size != y->size) {
        return x->size > y->size ? -1 : 1;
    }
    return strcmp(x->name, y->name);
}

static int cmp_name_rev(const void *a, const void *b)
{
    return cmp_name(b, a);
}

static int cmp_time_rev(const void *a, const void *b)
{
    return cmp_time(b, a);
}

static int cmp_size_rev(const void *a, const void *b)
{
    return cmp_size(b, a);
}

void ls_toto_sort_entries(struct ls_toto_entry *items, size_t count,
                          enum ls_toto_sort_key key, int reverse)
{
    int (*cmp)(const void *, const void *);

    if (count < 2) {
        return;
    }
    switch (key) {
    case LS_TOTO_SORT_TIME:
        cmp = reverse ? cmp_time_rev : cmp_time;
        break;
    case LS_TOTO_SORT_SIZE:
        cmp = reverse ? cmp_size_rev : cmp_size;
        break;
    case LS_TOTO_SORT_NAME:
    default:
        cmp = reverse ? cmp_name_rev : cmp_name;
        break;
    }
    qsort(items, count, sizeof items[0], cmp);
}
