#ifndef LS_TOTO_FORMAT_H
#define LS_TOTO_FORMAT_H

#include <stddef.h>
#include <time.h>

#include "ls_toto_cli.h"
#include "ls_toto_fs.h"

/* Upper bound on columns considered by the multi-column layout. */
#define LS_TOTO_MAX_COLS 1024

/* Seconds in half a Gregorian year: the "recent file" cutoff for -l. */
#define LS_TOTO_SIX_MONTHS 15778476

/* Write the 10-char mode string ("drwxr-xr-x") plus NUL into out. */
void ls_toto_format_mode(enum ls_toto_ftype type, unsigned int perm,
                         char out[11]);

/* -h size: powers of 1024 rounded up; plain bytes below 1024; one decimal
 * below 10 ("1.5K"), none from 10 up ("10K"). */
void ls_toto_format_size_human(long long size, char *out, size_t outsz);

/* "%b %e %H:%M" when t is within the past six months of now (and not in the
 * future), else "%b %e  %Y". */
void ls_toto_format_time(time_t t, time_t now, char *out, size_t outsz);

/* -F indicator for e, or '\0' for none. '@' is omitted in long format. */
char ls_toto_classify_suffix(const struct ls_toto_entry *e, int long_format);

/* Long listing lines (no "total" line). */
void ls_toto_print_long(const struct ls_toto_entry *items, size_t count,
                        const struct ls_toto_opts *opts, time_t now);

/* Multi-column, filled down then across, fitting in `width` characters. */
void ls_toto_print_columns(const struct ls_toto_entry *items, size_t count,
                           const struct ls_toto_opts *opts, size_t width);

/* One name per line. */
void ls_toto_print_one_per_line(const struct ls_toto_entry *items,
                                size_t count,
                                const struct ls_toto_opts *opts);

#endif
