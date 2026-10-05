/*
 * ls_toto_cli.c — command-line parsing and help/version text.
 *
 * Chain of thought:
 *   - Responsibility: turn argv into struct ls_toto_opts plus an ordered
 *     operand list, and print --help / --version text.
 *   - Meta flags (--help/--h, --version/--v) are found by scanning argv up to
 *     "--" with argv_has_exact(), so help wins over version and over any bad
 *     option, matching GNU behaviour.
 *   - Parsing is hand-rolled (getopt_long is a GNU extension) and permutes
 *     like GNU: options may follow operands; "--" ends option parsing.
 *   - Conflicting options are last-wins: -l/-1, -t/-S, -a/-A.
 *   - Syscalls: none in the parser (pure, linked into tests). Help/version
 *     use stdio, which the rules allow for that text only.
 *   - Heap: none. Operands are compacted in place inside argv.
 *   - Standard: C11.
 */
#include <stdio.h>
#include <string.h>

#include "ls_toto.h"
#include "ls_toto_cli.h"

/* Index of the first exact match of flag in argv[1 .. end-1], or 0. */
static int argv_has_exact(int end, char **argv, const char *flag)
{
    for (int i = 1; i < end; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return i;
        }
    }
    return 0;
}

/* Position of the first "--" in argv, or argc when absent. */
static int end_of_options(int argc, char **argv)
{
    int pos = argv_has_exact(argc, argv, LS_TOTO_ARG_END_OPTS);
    return pos != 0 ? pos : argc;
}

static enum ls_toto_parse_status scan_meta_flags(int argc, char **argv)
{
    int end = end_of_options(argc, argv);

    if (argv_has_exact(end, argv, LS_TOTO_ARG_HELP)
        || argv_has_exact(end, argv, LS_TOTO_ARG_HELP_SHORT)) {
        return LS_TOTO_PARSE_HELP;
    }
    if (argv_has_exact(end, argv, LS_TOTO_ARG_VERSION)
        || argv_has_exact(end, argv, LS_TOTO_ARG_VERSION_SHORT)) {
        return LS_TOTO_PARSE_VERSION;
    }
    return LS_TOTO_PARSE_OK;
}

/* Apply one short option letter. Returns 0 if c is not a known option. */
static int apply_short(struct ls_toto_opts *opts, char c)
{
    switch (c) {
        case 'a': opts->hidden = LS_TOTO_SHOW_ALL; break;
        case 'A': opts->hidden = LS_TOTO_SHOW_ALMOST; break;
        case 'l': opts->layout = LS_TOTO_LAYOUT_LONG; break;
        case '1': opts->layout = LS_TOTO_LAYOUT_ONE; break;
        case 'r': opts->reverse = 1; break;
        case 'R': opts->recursive = 1; break;
        case 't': opts->sort = LS_TOTO_SORT_TIME; break;
        case 'S': opts->sort = LS_TOTO_SORT_SIZE; break;
        case 'h': opts->human = 1; break;
        case 'd': opts->directory = 1; break;
        case 'F': opts->classify = 1; break;
        default: return 0;
    }
    return 1;
}

/* Apply one "--name" option. Returns 0 if arg is not a known long option. */
static int apply_long(struct ls_toto_opts *opts, const char *arg)
{
    static const struct {
        const char *name;
        char        letter;
    } table[] = {
        { LS_TOTO_ARG_ALL,        'a' },
        { LS_TOTO_ARG_ALMOST_ALL, 'A' },
        { LS_TOTO_ARG_REVERSE,    'r' },
        { LS_TOTO_ARG_RECURSIVE,  'R' },
        { LS_TOTO_ARG_HUMAN,      'h' },
        { LS_TOTO_ARG_DIRECTORY,  'd' },
        { LS_TOTO_ARG_CLASSIFY,   'F' },
    };

    for (size_t i = 0; i < sizeof table / sizeof table[0]; i++) {
        if (strcmp(arg, table[i].name) == 0) {
            return apply_short(opts, table[i].letter);
        }
    }
    return 0;
}

enum ls_toto_parse_status ls_toto_parse_args(int argc, char **argv,
                                             struct ls_toto_opts *opts,
                                             int *noperands)
{
    static const struct ls_toto_opts defaults = {
        LS_TOTO_LAYOUT_DEFAULT, LS_TOTO_SORT_NAME, LS_TOTO_HIDE_DOT,
        0, 0, 0, 0, 0, '\0', NULL
    };
    enum ls_toto_parse_status meta;
    int only_operands = 0;
    int n = 0;

    *opts = defaults;
    *noperands = 0;

    meta = scan_meta_flags(argc, argv);
    if (meta != LS_TOTO_PARSE_OK) {
        return meta;
    }

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        if (only_operands || arg[0] != '-' || arg[1] == '\0') {
            argv[1 + n++] = arg;
        } else if (strcmp(arg, LS_TOTO_ARG_END_OPTS) == 0) {
            only_operands = 1;
        } else if (arg[1] == '-') {
            if (!apply_long(opts, arg)) {
                opts->bad_arg = arg;
                return LS_TOTO_PARSE_BAD_LONG;
            }
        } else {
            for (const char *p = arg + 1; *p != '\0'; p++) {
                if (!apply_short(opts, *p)) {
                    opts->bad_char = *p;
                    return LS_TOTO_PARSE_BAD_SHORT;
                }
            }
        }
    }

    *noperands = n;
    return LS_TOTO_PARSE_OK;
}

void print_help(void)
{
    fputs("Usage: ls-toto [OPTION]... [FILE]...\n"
          "List information about the FILEs (the current directory by "
          "default).\n"
          "Sort entries alphabetically unless -t or -S is given.\n"
          "\n"
          "  -a, --all             do not ignore entries starting with .\n"
          "  -A, --almost-all      do not list implied . and ..\n"
          "  -d, --directory       list directories themselves, not their "
          "contents\n"
          "  -F, --classify        append indicator (one of */=@|) to "
          "entries\n"
          "  -h, --human-readable  with -l, print sizes like 1K 234M 2G "
          "etc.\n"
          "  -l                    use a long listing format\n"
          "  -r, --reverse         reverse order while sorting\n"
          "  -R, --recursive       list subdirectories recursively\n"
          "  -S                    sort by file size, largest first\n"
          "  -t                    sort by modification time, newest "
          "first\n"
          "  -1                    list one file per line\n"
          "      --help, --h       display this help and exit\n"
          "      --version, --v    output version information and exit\n"
          "\n"
          "Exit status:\n"
          " 0  if OK,\n"
          " 1  if minor problems (e.g., cannot access subdirectory),\n"
          " 2  if serious trouble (e.g., cannot access command-line "
          "argument).\n",
          stdout);
}

void print_version(void)
{
    printf("ls-toto %s\n", LS_TOTO_VERSION_STRING);
}
