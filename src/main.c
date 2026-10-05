/*
 * main.c — entry point for ls-toto.
 *
 * Chain of thought:
 *   - Responsibility: parse arguments, dispatch --help / --version and bad
 *     option diagnostics, then hand the operands to ls_toto_run().
 *   - Kept thin on purpose: no filesystem, sorting, or formatting logic.
 *   - Syscalls: none directly. Heap: none.
 *   - Standard: C11.
 */
#include "ls_toto.h"
#include "ls_toto_cli.h"
#include "ls_toto_emit.h"
#include "ls_toto_list.h"

int main(int argc, char **argv)
{
    struct ls_toto_opts opts;
    int noperands;

    switch (ls_toto_parse_args(argc, argv, &opts, &noperands)) {
    case LS_TOTO_PARSE_HELP:
        print_help();
        return LS_TOTO_EXIT_OK;
    case LS_TOTO_PARSE_VERSION:
        print_version();
        return LS_TOTO_EXIT_OK;
    case LS_TOTO_PARSE_BAD_SHORT:
        ls_toto_emit_invalid_option(opts.bad_char);
        return LS_TOTO_EXIT_SERIOUS;
    case LS_TOTO_PARSE_BAD_LONG:
        ls_toto_emit_unrecognized_option(opts.bad_arg);
        return LS_TOTO_EXIT_SERIOUS;
    case LS_TOTO_PARSE_OK:
    default:
        break;
    }

    return ls_toto_run(&opts, noperands, argv + 1);
}
