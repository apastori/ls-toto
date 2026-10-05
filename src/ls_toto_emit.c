/*
 * ls_toto_emit.c — stderr diagnostics.
 *
 * Chain of thought:
 *   - Responsibility: every message ls-toto prints to stderr, in GNU ls
 *     wording with the "ls-toto: " prefix.
 *   - errno is captured on entry, before any stdio call can change it.
 *   - Callers flush pending stdout (ls_toto_out_flush) first when ordering
 *     matters; this module does not, so a write failure inside the flush can
 *     still report itself without recursing.
 *   - Syscalls: none directly; stderr is unbuffered stdio.
 *   - Heap: none.
 *   - Standard: C11.
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "ls_toto_emit.h"

#define LS_TOTO_HINT "Try 'ls-toto --help' for more information.\n"

void ls_toto_emit_error(const char *context)
{
    int err = errno;
    fprintf(stderr, "ls-toto: %s: %s\n", context, strerror(err));
}

void ls_toto_emit_cannot_access(const char *path)
{
    int err = errno;
    fprintf(stderr, "ls-toto: cannot access '%s': %s\n", path,
            strerror(err));
}

void ls_toto_emit_cannot_open_dir(const char *path)
{
    int err = errno;
    fprintf(stderr, "ls-toto: cannot open directory '%s': %s\n", path,
            strerror(err));
}

void ls_toto_emit_invalid_option(char c)
{
    fprintf(stderr, "ls-toto: invalid option -- '%c'\n" LS_TOTO_HINT, c);
}

void ls_toto_emit_unrecognized_option(const char *arg)
{
    fprintf(stderr, "ls-toto: unrecognized option '%s'\n" LS_TOTO_HINT, arg);
}
