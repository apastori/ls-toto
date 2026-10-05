/*
 * ls_toto_out.c — buffered raw stdout writer.
 *
 * Chain of thought:
 *   - Responsibility: collect listing bytes in a static 8 KiB buffer and
 *     emit them with write(2) — the project never uses stdio for listings.
 *   - Flush when the next chunk would overflow the buffer; chunks larger
 *     than the buffer bypass it. ls_toto_run() flushes once before exit and
 *     before every diagnostic so stdout/stderr ordering matches GNU ls.
 *   - try_write_all() retries on EINTR and short writes; any other failure
 *     is fatal (exit 2), as a partial listing cannot be recovered.
 *   - Syscalls: write. Heap: none.
 *   - Standard: C11 + POSIX write() (MinGW provides it via <unistd.h>).
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ls_toto.h"
#include "ls_toto_emit.h"
#include "ls_toto_out.h"

static char   out_buf[LS_TOTO_OUT_BUFSZ];
static size_t out_len;

/*
 * Pre:  buf != NULL, len > 0.
 * Post: all len bytes written, or the process exited.
 * EINTR: retry. Short write: advance and continue.
 * Other errors: "ls-toto: write: <reason>", exit(LS_TOTO_EXIT_SERIOUS).
 */
static void try_write_all(int fd, const char *buf, size_t len)
{
    while (len > 0) {
        ssize_t n = write(fd, buf, len);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            ls_toto_emit_error("write");
            exit(LS_TOTO_EXIT_SERIOUS);
        }
        if (n == 0) {
            errno = EIO;
            ls_toto_emit_error("write");
            exit(LS_TOTO_EXIT_SERIOUS);
        }
        buf += n;
        len -= (size_t)n;
    }
}

void ls_toto_out_flush(void)
{
    if (out_len > 0) {
        size_t len = out_len;
        out_len = 0;
        try_write_all(STDOUT_FILENO, out_buf, len);
    }
}

void ls_toto_out_bytes(const char *p, size_t n)
{
    if (n > sizeof out_buf - out_len) {
        ls_toto_out_flush();
        if (n >= sizeof out_buf) {
            try_write_all(STDOUT_FILENO, p, n);
            return;
        }
    }
    memcpy(out_buf + out_len, p, n);
    out_len += n;
}

void ls_toto_out_str(const char *s)
{
    ls_toto_out_bytes(s, strlen(s));
}

void ls_toto_out_char(char c)
{
    if (out_len == sizeof out_buf) {
        ls_toto_out_flush();
    }
    out_buf[out_len++] = c;
}
