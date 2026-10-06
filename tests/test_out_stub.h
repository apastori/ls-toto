#ifndef LS_TOTO_TEST_OUT_STUB_H
#define LS_TOTO_TEST_OUT_STUB_H

/* Bytes written through ls_toto_out_*() since the last reset, NUL-terminated. */
const char *test_out_stub_buffer(void);

/* Discard all captured bytes. */
void test_out_stub_reset(void);

#endif
