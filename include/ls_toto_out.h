#ifndef LS_TOTO_OUT_H
#define LS_TOTO_OUT_H

#include <stddef.h>

#define LS_TOTO_OUT_BUFSZ 8192

/* Buffered stdout writer. Bytes are flushed with raw write(2) when the
 * buffer would overflow and on ls_toto_out_flush(). A write failure prints
 * "ls-toto: write: <reason>" and exits with LS_TOTO_EXIT_SERIOUS. */
void ls_toto_out_bytes(const char *p, size_t n);
void ls_toto_out_str(const char *s);
void ls_toto_out_char(char c);
void ls_toto_out_flush(void);

#endif
