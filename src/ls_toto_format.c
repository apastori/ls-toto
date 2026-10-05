/*
 * ls_toto_format.c — mode strings, sizes, times, -F suffixes, and the three
 * output layouts (long, columns, one per line).
 *
 * Chain of thought:
 *   - Responsibility: turn platform-neutral struct ls_toto_entry arrays into
 *     text. All bytes go through ls_toto_out_*(), so this module performs no
 *     I/O itself and links into tests against a stub writer.
 *   - Human-readable sizes use integer arithmetic (powers of 1024, rounded
 *     up like GNU's ceiling mode) to avoid floating-point rounding surprises.
 *   - Columns follow GNU's algorithm: try the largest column count first;
 *     each column is as wide as its longest name plus a 2-space gutter (none
 *     after the last column); the layout fits if the total is strictly less
 *     than the width. Column widths live in a fixed stack array so the module
 *     never allocates.
 *   - Name widths are byte lengths (C locale; no multibyte width handling).
 *   - Syscalls: none. localtime()/strftime() for -l timestamps.
 *   - Heap: none.
 *   - Standard: C11.
 */
#include <stdio.h>
#include <string.h>

#include "ls_toto_format.h"
#include "ls_toto_out.h"

void ls_toto_format_mode(enum ls_toto_ftype type, unsigned int perm,
                         char out[11])
{
    static const char type_chars[] = {
        [LS_TOTO_FT_REG]  = '-',
        [LS_TOTO_FT_DIR]  = 'd',
        [LS_TOTO_FT_LNK]  = 'l',
        [LS_TOTO_FT_FIFO] = 'p',
        [LS_TOTO_FT_SOCK] = 's',
        [LS_TOTO_FT_CHR]  = 'c',
        [LS_TOTO_FT_BLK]  = 'b',
    };

    out[0] = type_chars[type];
    out[1] = (perm & 0400) ? 'r' : '-';
    out[2] = (perm & 0200) ? 'w' : '-';
    if (perm & 04000) {
        out[3] = (perm & 0100) ? 's' : 'S';
    } else {
        out[3] = (perm & 0100) ? 'x' : '-';
    }
    out[4] = (perm & 0040) ? 'r' : '-';
    out[5] = (perm & 0020) ? 'w' : '-';
    if (perm & 02000) {
        out[6] = (perm & 0010) ? 's' : 'S';
    } else {
        out[6] = (perm & 0010) ? 'x' : '-';
    }
    out[7] = (perm & 0004) ? 'r' : '-';
    out[8] = (perm & 0002) ? 'w' : '-';
    if (perm & 01000) {
        out[9] = (perm & 0001) ? 't' : 'T';
    } else {
        out[9] = (perm & 0001) ? 'x' : '-';
    }
    out[10] = '\0';
}

/*
 * Pre:  out != NULL, outsz >= 6 (longest output is "1023K" + NUL).
 * Post: out holds the human-readable form of size (negative sizes print 0).
 */
void ls_toto_format_size_human(long long size, char *out, size_t outsz)
{
    static const char units[] = "KMGTPE";
    unsigned long long s;
    unsigned long long d = 1024;
    unsigned long long q;
    unsigned long long r;
    size_t e = 0;

    if (size < 1024) {
        snprintf(out, outsz, "%lld", size < 0 ? 0LL : size);
        return;
    }

    s = (unsigned long long)size;
    while (s / d >= 1024 && e + 1 < sizeof units - 1) {
        d *= 1024;
        e++;
    }
    q = s / d;
    r = s % d;

    if (q < 10) {
        /* tenths, rounded up; r * 10 + d - 1 cannot overflow for d <= 2^60 */
        unsigned long long t = q * 10 + (r * 10 + d - 1) / d;
        if (t < 100) {
            snprintf(out, outsz, "%llu.%llu%c", t / 10, t % 10, units[e]);
        } else {
            snprintf(out, outsz, "10%c", units[e]);
        }
        return;
    }

    q += (r != 0);
    if (q >= 1024 && e + 1 < sizeof units - 1) {
        snprintf(out, outsz, "1.0%c", units[e + 1]);
    } else {
        snprintf(out, outsz, "%llu%c", q, units[e]);
    }
}

void ls_toto_format_time(time_t t, time_t now, char *out, size_t outsz)
{
    struct tm *tm = localtime(&t);
    int recent = t > now - LS_TOTO_SIX_MONTHS && t <= now;

    if (tm == NULL
        || strftime(out, outsz, recent ? "%b %e %H:%M" : "%b %e  %Y",
                    tm) == 0) {
        snprintf(out, outsz, "%lld", (long long)t);
    }
}

char ls_toto_classify_suffix(const struct ls_toto_entry *e, int long_format)
{
    switch (e->type) {
    case LS_TOTO_FT_DIR:  return '/';
    case LS_TOTO_FT_LNK:  return long_format ? '\0' : '@';
    case LS_TOTO_FT_FIFO: return '|';
    case LS_TOTO_FT_SOCK: return '=';
    case LS_TOTO_FT_REG:  return (e->perm & 0111) ? '*' : '\0';
    default:              return '\0';
    }
}

/* Display width of a name including its -F suffix. */
static size_t name_width(const struct ls_toto_entry *e,
                         const struct ls_toto_opts *opts, int long_format)
{
    size_t w = strlen(e->name);
    if (opts->classify && ls_toto_classify_suffix(e, long_format) != '\0') {
        w++;
    }
    return w;
}

static void out_name(const struct ls_toto_entry *e,
                     const struct ls_toto_opts *opts, int long_format)
{
    ls_toto_out_str(e->name);
    if (opts->classify) {
        char c = ls_toto_classify_suffix(e, long_format);
        if (c != '\0') {
            ls_toto_out_char(c);
        }
    }
}

static void out_spaces(size_t n)
{
    while (n-- > 0) {
        ls_toto_out_char(' ');
    }
}

static void out_right(const char *s, size_t width)
{
    size_t len = strlen(s);
    if (len < width) {
        out_spaces(width - len);
    }
    ls_toto_out_str(s);
}

static void out_left(const char *s, size_t width)
{
    size_t len = strlen(s);
    ls_toto_out_str(s);
    if (len < width) {
        out_spaces(width - len);
    }
}

static void format_size(const struct ls_toto_entry *e,
                        const struct ls_toto_opts *opts,
                        char *buf, size_t bufsz)
{
    if (opts->human) {
        ls_toto_format_size_human(e->size, buf, bufsz);
    } else {
        snprintf(buf, bufsz, "%lld", e->size);
    }
}

void ls_toto_print_long(const struct ls_toto_entry *items, size_t count,
                        const struct ls_toto_opts *opts, time_t now)
{
    size_t w_nlink = 0;
    size_t w_owner = 0;
    size_t w_group = 0;
    size_t w_size = 0;
    char buf[64];

    for (size_t i = 0; i < count; i++) {
        size_t len;

        snprintf(buf, sizeof buf, "%lu", items[i].nlink);
        len = strlen(buf);
        if (len > w_nlink) w_nlink = len;

        len = strlen(items[i].owner);
        if (len > w_owner) w_owner = len;

        len = strlen(items[i].group);
        if (len > w_group) w_group = len;

        format_size(&items[i], opts, buf, sizeof buf);
        len = strlen(buf);
        if (len > w_size) w_size = len;
    }

    for (size_t i = 0; i < count; i++) {
        const struct ls_toto_entry *e = &items[i];
        char mode[11];

        ls_toto_format_mode(e->type, e->perm, mode);
        ls_toto_out_str(mode);
        ls_toto_out_char(' ');

        snprintf(buf, sizeof buf, "%lu", e->nlink);
        out_right(buf, w_nlink);
        ls_toto_out_char(' ');

        out_left(e->owner, w_owner);
        ls_toto_out_char(' ');
        out_left(e->group, w_group);
        ls_toto_out_char(' ');

        format_size(e, opts, buf, sizeof buf);
        out_right(buf, w_size);
        ls_toto_out_char(' ');

        ls_toto_format_time(e->mtime, now, buf, sizeof buf);
        ls_toto_out_str(buf);
        ls_toto_out_char(' ');

        out_name(e, opts, 1);
        if (e->type == LS_TOTO_FT_LNK && e->link_target != NULL) {
            ls_toto_out_str(" -> ");
            ls_toto_out_str(e->link_target);
        }
        ls_toto_out_char('\n');
    }
}

/*
 * Fill colw[0 .. cols-1] for a down-then-across layout with `cols` columns
 * and return the total line length.
 */
static size_t layout_columns(const struct ls_toto_entry *items, size_t count,
                             const struct ls_toto_opts *opts, size_t cols,
                             size_t *colw)
{
    size_t rows = (count + cols - 1) / cols;
    size_t total = 0;

    memset(colw, 0, cols * sizeof colw[0]);
    for (size_t i = 0; i < count; i++) {
        size_t c = i / rows;
        size_t w = name_width(&items[i], opts, 0) + (c == cols - 1 ? 0 : 2);
        if (w > colw[c]) {
            colw[c] = w;
        }
    }
    for (size_t c = 0; c < cols; c++) {
        total += colw[c];
    }
    return total;
}

void ls_toto_print_columns(const struct ls_toto_entry *items, size_t count,
                           const struct ls_toto_opts *opts, size_t width)
{
    size_t colw[LS_TOTO_MAX_COLS];
    size_t max_cols;
    size_t cols = 1;
    size_t rows;

    if (count == 0) {
        return;
    }

    max_cols = width / 3;
    if (max_cols == 0) max_cols = 1;
    if (max_cols > count) max_cols = count;
    if (max_cols > LS_TOTO_MAX_COLS) max_cols = LS_TOTO_MAX_COLS;

    for (size_t c = max_cols; c > 1; c--) {
        if (layout_columns(items, count, opts, c, colw) < width) {
            cols = c;
            break;
        }
    }
    if (cols == 1) {
        layout_columns(items, count, opts, 1, colw);
    }

    rows = (count + cols - 1) / cols;
    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < cols; c++) {
            size_t idx = c * rows + r;
            if (idx >= count) {
                break;
            }
            out_name(&items[idx], opts, 0);
            if (idx + rows < count) {
                out_spaces(colw[c] - name_width(&items[idx], opts, 0));
            }
        }
        ls_toto_out_char('\n');
    }
}

void ls_toto_print_one_per_line(const struct ls_toto_entry *items,
                                size_t count,
                                const struct ls_toto_opts *opts)
{
    for (size_t i = 0; i < count; i++) {
        out_name(&items[i], opts, 0);
        ls_toto_out_char('\n');
    }
}
