/*
 * ls_toto_list.c — ls_toto_run(): operand ordering, headers, total line,
 * and -R recursion.
 *
 * Chain of thought:
 *   - Responsibility: orchestrate a whole invocation. Stat every operand,
 *     list non-directories first as one group, then each directory (sorted
 *     with the same key), recursing depth-first in sorted order with -R.
 *   - Headers ("path:") appear when there was more than one operand, any
 *     file operand was printed, or -R is set; sections are separated by one
 *     blank line, matching GNU ls.
 *   - Operands use stat() (follow symlinks) unless -l or -d; directory
 *     members always use lstat(), so -R never follows symlinks and never
 *     descends into "." or "..".
 *   - Exit status: inaccessible operand or unopenable operand directory is
 *     serious (2); an unopenable subdirectory during -R is minor (1).
 *   - Pending stdout is flushed before each diagnostic so interleaving with
 *     stderr matches GNU ls on a terminal.
 *   - Recursion frees each directory's entry array before descending; only
 *     the child path strings stay alive.
 *   - Syscalls: isatty (layout choice); filesystem access via ls_toto_fs.
 *   - Heap: operand entry arrays, per-directory entry arrays, child paths.
 *   - Standard: C11 + POSIX isatty()/getenv().
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ls_toto.h"
#include "ls_toto_emit.h"
#include "ls_toto_format.h"
#include "ls_toto_fs.h"
#include "ls_toto_list.h"
#include "ls_toto_out.h"
#include "ls_toto_sort.h"

#define LS_TOTO_DEFAULT_WIDTH 80

enum ls_toto_view { VIEW_LONG, VIEW_ONE, VIEW_COLUMNS };

struct run_ctx {
    const struct ls_toto_opts *opts;
    enum ls_toto_view          view;
    size_t                     width;
    time_t                     now;
    int                        print_headers;
    int                        first_header;
    int                        status;
};

static void worsen(struct run_ctx *ctx, int status)
{
    if (status > ctx->status) {
        ctx->status = status;
    }
}

/* Terminal width from COLUMNS (positive integer), else 80. */
static size_t terminal_width(void)
{
    const char *s = getenv("COLUMNS");
    char *end;
    long v;

    if (s == NULL || *s == '\0') {
        return LS_TOTO_DEFAULT_WIDTH;
    }
    errno = 0;
    v = strtol(s, &end, 10);
    if (errno != 0 || *end != '\0' || v <= 0) {
        return LS_TOTO_DEFAULT_WIDTH;
    }
    return (size_t)v;
}

static void print_entries(const struct run_ctx *ctx,
                          const struct ls_toto_entry *items, size_t count)
{
    switch (ctx->view) {
    case VIEW_LONG:
        ls_toto_print_long(items, count, ctx->opts, ctx->now);
        break;
    case VIEW_COLUMNS:
        ls_toto_print_columns(items, count, ctx->opts, ctx->width);
        break;
    case VIEW_ONE:
    default:
        ls_toto_print_one_per_line(items, count, ctx->opts);
        break;
    }
}

/* "total N": allocated space in 1 KiB units (rounded up), or -h form. */
static void print_total(const struct run_ctx *ctx,
                        const struct ls_toto_entry *items, size_t count)
{
    long long blocks = 0;
    char buf[32];

    for (size_t i = 0; i < count; i++) {
        blocks += items[i].blocks;
    }
    if (ctx->opts->human) {
        ls_toto_format_size_human(blocks * 512, buf, sizeof buf);
    } else {
        snprintf(buf, sizeof buf, "%lld", (blocks + 1) / 2);
    }
    ls_toto_out_str("total ");
    ls_toto_out_str(buf);
    ls_toto_out_char('\n');
}

static int is_dot_or_dotdot(const char *name)
{
    return name[0] == '.'
           && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

/*
 * List one directory (and its subdirectories with -R).
 *
 * Pre:  path names a directory (as given or joined from its parent).
 * Post: header, optional total line, and entries written to the buffer;
 *       on open failure a diagnostic is emitted and ctx->status worsened
 *       (serious for operands, minor for recursion).
 */
static void list_dir(struct run_ctx *ctx, const char *path, int is_operand)
{
    struct ls_toto_entry *items;
    size_t count;
    char **subdirs = NULL;
    size_t nsub = 0;

    if (ls_toto_fs_read_dir(path, ctx->opts->hidden, ctx->view == VIEW_LONG,
                            &items, &count) != 0) {
        int err = errno;
        ls_toto_out_flush();
        errno = err;
        ls_toto_emit_cannot_open_dir(path);
        worsen(ctx, is_operand ? LS_TOTO_EXIT_SERIOUS : LS_TOTO_EXIT_MINOR);
        return;
    }

    if (ctx->print_headers) {
        if (!ctx->first_header) {
            ls_toto_out_char('\n');
        }
        ctx->first_header = 0;
        ls_toto_out_str(path);
        ls_toto_out_str(":\n");
    }

    ls_toto_sort_entries(items, count, ctx->opts->sort, ctx->opts->reverse);
    if (ctx->view == VIEW_LONG) {
        print_total(ctx, items, count);
    }
    print_entries(ctx, items, count);

    if (ctx->opts->recursive && count > 0) {
        subdirs = malloc(count * sizeof *subdirs);
        if (subdirs == NULL) {
            ls_toto_out_flush();
            errno = ENOMEM;
            ls_toto_emit_error("malloc");
            exit(LS_TOTO_EXIT_SERIOUS);
        }
        for (size_t i = 0; i < count; i++) {
            if (items[i].type == LS_TOTO_FT_DIR
                && !is_dot_or_dotdot(items[i].name)) {
                subdirs[nsub++] = ls_toto_path_join(path, items[i].name);
            }
        }
    }
    ls_toto_entries_free(items, count);

    for (size_t i = 0; i < nsub; i++) {
        list_dir(ctx, subdirs[i], 0);
        free(subdirs[i]);
    }
    free(subdirs);
}

int ls_toto_run(const struct ls_toto_opts *opts, int noperands,
                char **operands)
{
    static char dot[] = ".";
    char *default_operands[] = { dot };
    struct run_ctx ctx;
    struct ls_toto_entry *files = NULL;
    struct ls_toto_entry *dirs = NULL;
    size_t nfiles = 0, cap_files = 0;
    size_t ndirs = 0, cap_dirs = 0;
    int nargs = noperands > 0 ? noperands : 1;
    char **args = noperands > 0 ? operands : default_operands;
    int is_tty = isatty(STDOUT_FILENO);
    int follow;

    ctx.opts = opts;
    if (opts->layout == LS_TOTO_LAYOUT_LONG) {
        ctx.view = VIEW_LONG;
    } else if (opts->layout == LS_TOTO_LAYOUT_ONE || !is_tty) {
        ctx.view = VIEW_ONE;
    } else {
        ctx.view = VIEW_COLUMNS;
    }
    ctx.width = ctx.view == VIEW_COLUMNS ? terminal_width()
                                         : LS_TOTO_DEFAULT_WIDTH;
    ctx.now = time(NULL);
    ctx.first_header = 1;
    ctx.status = LS_TOTO_EXIT_OK;

    follow = !(ctx.view == VIEW_LONG || opts->directory);
    for (int i = 0; i < nargs; i++) {
        struct ls_toto_entry e;

        if (ls_toto_fs_stat(args[i], follow, ctx.view == VIEW_LONG,
                            &e) != 0) {
            int err = errno;
            ls_toto_out_flush();
            errno = err;
            ls_toto_emit_cannot_access(args[i]);
            worsen(&ctx, LS_TOTO_EXIT_SERIOUS);
            continue;
        }
        if (!opts->directory && e.type == LS_TOTO_FT_DIR) {
            ls_toto_entries_push(&dirs, &ndirs, &cap_dirs, &e);
        } else {
            ls_toto_entries_push(&files, &nfiles, &cap_files, &e);
        }
    }

    ls_toto_sort_entries(files, nfiles, opts->sort, opts->reverse);
    ls_toto_sort_entries(dirs, ndirs, opts->sort, opts->reverse);

    ctx.print_headers = noperands > 1 || nfiles > 0 || opts->recursive;

    if (nfiles > 0) {
        print_entries(&ctx, files, nfiles);
        if (ndirs > 0) {
            ls_toto_out_char('\n');
        }
    }
    ls_toto_entries_free(files, nfiles);

    for (size_t i = 0; i < ndirs; i++) {
        list_dir(&ctx, dirs[i].name, 1);
    }
    ls_toto_entries_free(dirs, ndirs);

    ls_toto_out_flush();
    return ctx.status;
}
