#ifndef LS_TOTO_EMIT_H
#define LS_TOTO_EMIT_H

/* All functions write a single diagnostic to stderr. Those that report a
 * system error read errno on entry; callers must not clobber it first. */

/* "ls-toto: <context>: <strerror(errno)>" */
void ls_toto_emit_error(const char *context);

/* "ls-toto: cannot access '<path>': <strerror(errno)>" */
void ls_toto_emit_cannot_access(const char *path);

/* "ls-toto: cannot open directory '<path>': <strerror(errno)>" */
void ls_toto_emit_cannot_open_dir(const char *path);

/* "ls-toto: invalid option -- '<c>'" + hint line */
void ls_toto_emit_invalid_option(char c);

/* "ls-toto: unrecognized option '<arg>'" + hint line */
void ls_toto_emit_unrecognized_option(const char *arg);

#endif
