/*
 * test_cli_parse.c — ls_toto_parse_args() and scan_meta_flags() behaviour.
 *
 * Chain of thought:
 *   - The parser is pure, so it is exercised directly with synthetic argv
 *     arrays. argv strings live in local char arrays because the parser
 *     compacts operands in place.
 *   - Covers: combined short flags, -h meaning human-readable, help winning
 *     over version, short meta flags, GNU permutation, "--", bad short and
 *     long options, and last-wins conflicts.
 *   - Heap: none.
 *   - Standard: C11 + <assert.h>.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ls_toto_cli.h"
#include "test_cli_parse.h"

#define ARGC(a) ((int)(sizeof (a) / sizeof (a)[0]) - 1)

static void combined_flags(void)
{
    char a0[] = "ls-toto", a1[] = "-la";
    char *argv[] = { a0, a1, NULL };
    struct ls_toto_opts o;
    int n;

    assert(ls_toto_parse_args(ARGC(argv), argv, &o, &n) == LS_TOTO_PARSE_OK);
    assert(o.layout == LS_TOTO_LAYOUT_LONG);
    assert(o.hidden == LS_TOTO_SHOW_ALL);
    assert(n == 0);
    printf("PASS: cli_parse combined -la\n");
}

static void h_is_human(void)
{
    char a0[] = "ls-toto", a1[] = "-h";
    char *argv[] = { a0, a1, NULL };
    struct ls_toto_opts o;
    int n;

    assert(ls_toto_parse_args(ARGC(argv), argv, &o, &n) == LS_TOTO_PARSE_OK);
    assert(o.human == 1);
    printf("PASS: cli_parse -h is human-readable\n");
}

static void help_wins(void)
{
    char a0[] = "ls-toto", a1[] = "--version", a2[] = "-z", a3[] = "--help";
    char *argv[] = { a0, a1, a2, a3, NULL };

    assert(scan_meta_flags(ARGC(argv), argv) == LS_TOTO_PARSE_HELP);
    printf("PASS: cli_parse --help wins over --version\n");
}

static void short_meta(void)
{
    char a0[] = "ls-toto", a1[] = "--v";
    char *argv[] = { a0, a1, NULL };
    char b1[] = "--h";
    char *argv2[] = { a0, b1, NULL };

    assert(scan_meta_flags(ARGC(argv), argv) == LS_TOTO_PARSE_VERSION);
    assert(scan_meta_flags(ARGC(argv2), argv2) == LS_TOTO_PARSE_HELP);
    printf("PASS: cli_parse --v / --h\n");
}

static void permutation(void)
{
    char a0[] = "ls-toto", a1[] = "a", a2[] = "-l", a3[] = "b";
    char *argv[] = { a0, a1, a2, a3, NULL };
    struct ls_toto_opts o;
    int n;

    assert(ls_toto_parse_args(ARGC(argv), argv, &o, &n) == LS_TOTO_PARSE_OK);
    assert(n == 2);
    assert(strcmp(argv[1], "a") == 0);
    assert(strcmp(argv[2], "b") == 0);
    assert(o.layout == LS_TOTO_LAYOUT_LONG);
    printf("PASS: cli_parse permutation a -l b\n");
}

static void end_of_options(void)
{
    char a0[] = "ls-toto", a1[] = "--", a2[] = "-l", a3[] = "--help";
    char *argv[] = { a0, a1, a2, a3, NULL };
    struct ls_toto_opts o;
    int n;

    assert(scan_meta_flags(ARGC(argv), argv) == LS_TOTO_PARSE_NONE);
    assert(ls_toto_parse_args(ARGC(argv), argv, &o, &n) == LS_TOTO_PARSE_OK);
    assert(n == 2);
    assert(strcmp(argv[1], "-l") == 0);
    assert(strcmp(argv[2], "--help") == 0);
    assert(o.layout == LS_TOTO_LAYOUT_DEFAULT);
    printf("PASS: cli_parse -- ends options\n");
}

static void dash_alone_is_operand(void)
{
    char a0[] = "ls-toto", a1[] = "-";
    char *argv[] = { a0, a1, NULL };
    struct ls_toto_opts o;
    int n;

    assert(ls_toto_parse_args(ARGC(argv), argv, &o, &n) == LS_TOTO_PARSE_OK);
    assert(n == 1);
    assert(strcmp(argv[1], "-") == 0);
    printf("PASS: cli_parse - is an operand\n");
}

static void bad_short(void)
{
    char a0[] = "ls-toto", a1[] = "-lz";
    char *argv[] = { a0, a1, NULL };
    struct ls_toto_opts o;
    int n;

    assert(ls_toto_parse_args(ARGC(argv), argv, &o, &n)
           == LS_TOTO_PARSE_BAD_SHORT);
    assert(o.bad_char == 'z');
    printf("PASS: cli_parse -z is invalid\n");
}

static void bad_long(void)
{
    char a0[] = "ls-toto", a1[] = "--foo";
    char *argv[] = { a0, a1, NULL };
    struct ls_toto_opts o;
    int n;

    assert(ls_toto_parse_args(ARGC(argv), argv, &o, &n)
           == LS_TOTO_PARSE_BAD_LONG);
    assert(strcmp(o.bad_arg, "--foo") == 0);
    printf("PASS: cli_parse --foo is unrecognized\n");
}

static void long_forms(void)
{
    char a0[] = "ls-toto", a1[] = "--almost-all", a2[] = "--reverse",
         a3[] = "--recursive", a4[] = "--human-readable",
         a5[] = "--directory", a6[] = "--classify";
    char *argv[] = { a0, a1, a2, a3, a4, a5, a6, NULL };
    struct ls_toto_opts o;
    int n;

    assert(ls_toto_parse_args(ARGC(argv), argv, &o, &n) == LS_TOTO_PARSE_OK);
    assert(o.hidden == LS_TOTO_SHOW_ALMOST);
    assert(o.reverse && o.recursive && o.human && o.directory && o.classify);
    assert(n == 0);
    printf("PASS: cli_parse long option forms\n");
}

static void last_wins(void)
{
    char a0[] = "ls-toto", a1[] = "-l1", a2[] = "-tS", a3[] = "-aA";
    char *argv[] = { a0, a1, a2, a3, NULL };
    char b1[] = "-1l", b2[] = "-St", b3[] = "-Aa";
    char *argv2[] = { a0, b1, b2, b3, NULL };
    struct ls_toto_opts o;
    int n;

    assert(ls_toto_parse_args(ARGC(argv), argv, &o, &n) == LS_TOTO_PARSE_OK);
    assert(o.layout == LS_TOTO_LAYOUT_ONE);
    assert(o.sort == LS_TOTO_SORT_SIZE);
    assert(o.hidden == LS_TOTO_SHOW_ALMOST);

    assert(ls_toto_parse_args(ARGC(argv2), argv2, &o, &n)
           == LS_TOTO_PARSE_OK);
    assert(o.layout == LS_TOTO_LAYOUT_LONG);
    assert(o.sort == LS_TOTO_SORT_TIME);
    assert(o.hidden == LS_TOTO_SHOW_ALL);
    printf("PASS: cli_parse last of -l/-1, -t/-S, -a/-A wins\n");
}

void test_cli_parse_run(void)
{
    combined_flags();
    h_is_human();
    help_wins();
    short_meta();
    permutation();
    end_of_options();
    dash_alone_is_operand();
    bad_short();
    bad_long();
    long_forms();
    last_wins();
}
