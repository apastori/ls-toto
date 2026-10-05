#ifndef LS_TOTO_LIST_H
#define LS_TOTO_LIST_H

#include "ls_toto_cli.h"

/*
 * List the given operands ("." when noperands == 0) according to *opts.
 *
 * Pre:  opts != NULL; operands[0 .. noperands-1] are valid C strings.
 * Post: listing written to stdout and flushed; diagnostics on stderr.
 * Returns the worst enum ls_toto_exit value encountered.
 * Exits with LS_TOTO_EXIT_SERIOUS on write or allocation failure.
 */
int ls_toto_run(const struct ls_toto_opts *opts, int noperands,
                char **operands);

#endif
