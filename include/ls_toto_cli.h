#ifndef LS_TOTO_CLI_H
#define LS_TOTO_CLI_H

#define LS_TOTO_ARG_HELP           "--help"
#define LS_TOTO_ARG_HELP_SHORT     "--h"
#define LS_TOTO_ARG_VERSION        "--version"
#define LS_TOTO_ARG_VERSION_SHORT  "--v"

#define LS_TOTO_ARG_ALL            "--all"
#define LS_TOTO_ARG_ALMOST_ALL     "--almost-all"
#define LS_TOTO_ARG_REVERSE        "--reverse"
#define LS_TOTO_ARG_RECURSIVE      "--recursive"
#define LS_TOTO_ARG_HUMAN          "--human-readable"
#define LS_TOTO_ARG_DIRECTORY      "--directory"
#define LS_TOTO_ARG_CLASSIFY       "--classify"

#define LS_TOTO_ARG_END_OPTS       "--"

enum ls_toto_layout {
    LS_TOTO_LAYOUT_DEFAULT,     /* columns on a tty, one per line otherwise */
    LS_TOTO_LAYOUT_LONG,        /* -l */
    LS_TOTO_LAYOUT_ONE          /* -1 */
};

enum ls_toto_sort_key {
    LS_TOTO_SORT_NAME,
    LS_TOTO_SORT_TIME,          /* -t */
    LS_TOTO_SORT_SIZE           /* -S */
};

enum ls_toto_hidden {
    LS_TOTO_HIDE_DOT,           /* default: skip names starting with '.' */
    LS_TOTO_SHOW_ALL,           /* -a */
    LS_TOTO_SHOW_ALMOST         /* -A: all except "." and ".." */
};

struct ls_toto_opts {
    enum ls_toto_layout   layout;
    enum ls_toto_sort_key sort;
    enum ls_toto_hidden   hidden;
    int                   reverse;
    int                   recursive;
    int                   human;
    int                   directory;
    int                   classify;
    char                  bad_char;     /* set on LS_TOTO_PARSE_BAD_SHORT */
    const char           *bad_arg;      /* set on LS_TOTO_PARSE_BAD_LONG */
};

enum ls_toto_parse_status {
    LS_TOTO_PARSE_OK,
    LS_TOTO_PARSE_HELP,
    LS_TOTO_PARSE_VERSION,
    LS_TOTO_PARSE_BAD_SHORT,
    LS_TOTO_PARSE_BAD_LONG
};

/*
 * Parse argv into *opts.
 *
 * Pre:  argc >= 1, argv[0..argc-1] valid, opts != NULL, noperands != NULL.
 * Post: on LS_TOTO_PARSE_OK, operands are compacted in their original order
 *       into argv[1 .. *noperands] (GNU permutation: options may follow
 *       operands; everything after "--" is an operand).
 *       On BAD_SHORT / BAD_LONG, opts->bad_char / opts->bad_arg identify
 *       the offending option. Help wins over version; both are detected
 *       anywhere before "--".
 * Pure: performs no I/O.
 */
enum ls_toto_parse_status ls_toto_parse_args(int argc, char **argv,
                                             struct ls_toto_opts *opts,
                                             int *noperands);

void print_help(void);
void print_version(void);

#endif
