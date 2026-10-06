/*
 * test_out_stub.c — in-memory replacement for src/ls_toto_out.c.
 *
 * Chain of thought:
 *   - ls_toto_format.c writes through ls_toto_out_*(); the unit tests link
 *     the formatter without the real writer (which performs write(2)), so
 *     these definitions collect bytes into a static buffer instead.
 *   - Bytes beyond the buffer are dropped; tests never need more.
 *   - test_out_stub_buffer() exposes the captured bytes and
 *     test_out_stub_reset() discards them; ls_toto_out_flush() keeps them,
 *     as a real flush would not lose output.
 *   - Syscalls: none. Heap: none.
 *   - Standard: C11.
 */
#include <string.h>

#include "ls_toto_out.h"
#include "test_out_stub.h"

static char   stub_buf[LS_TOTO_OUT_BUFSZ];
static size_t stub_len;

void ls_toto_out_bytes(const char *p, size_t n)
{
    size_t room = sizeof stub_buf - 1 - stub_len;
    if (n > room) {
        n = room;
    }
    memcpy(stub_buf + stub_len, p, n);
    stub_len += n;
    stub_buf[stub_len] = '\0';
}

void ls_toto_out_str(const char *s)
{
    ls_toto_out_bytes(s, strlen(s));
}

void ls_toto_out_char(char c)
{
    ls_toto_out_bytes(&c, 1);
}

void ls_toto_out_flush(void)
{
}

const char *test_out_stub_buffer(void)
{
    return stub_buf;
}

void test_out_stub_reset(void)
{
    stub_len = 0;
    stub_buf[0] = '\0';
}
