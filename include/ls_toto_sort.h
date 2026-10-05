#ifndef LS_TOTO_SORT_H
#define LS_TOTO_SORT_H

#include <stddef.h>

#include "ls_toto_cli.h"
#include "ls_toto_fs.h"

/*
 * Sort items in place.
 *
 * LS_TOTO_SORT_NAME: strcmp order (C locale).
 * LS_TOTO_SORT_TIME: newer mtime first; ties by name.
 * LS_TOTO_SORT_SIZE: larger size first; ties by name.
 * reverse != 0 reverses the final order.
 */
void ls_toto_sort_entries(struct ls_toto_entry *items, size_t count,
                          enum ls_toto_sort_key key, int reverse);

#endif
