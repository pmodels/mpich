/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mpl.h"

#include "utest.h"


void test_nullstr(void)
{
    char *orig;
    char *str;
    char *next;

    str = NULL;
    next = MPL_strsep(&str, "|");
    UT_TEST_CHECK(next == NULL);
    UT_TEST_CHECK(str == NULL);
}

void test_empty_str(void)
{
    char *orig;
    char *str;
    char *next;

    orig = MPL_strdup("");
    str = orig;
    next = MPL_strsep(&str, "|");
    UT_TEST_CHECK(str == NULL);
    UT_TEST_CHECK(next == orig);
    MPL_free(orig);
}

void test_str(void)
{
    char *orig;
    char *str;
    char *next;

    orig = MPL_strdup("a|b|c");
    str = orig;
    next = MPL_strsep(&str, "|");
    UT_TEST_CHECK(next == orig);
    UT_TEST_CHECK(0 == strcmp(next, "a"));
    next = MPL_strsep(&str, "|");
    UT_TEST_CHECK(0 == strcmp(next, "b"));
    next = MPL_strsep(&str, "|");
    UT_TEST_CHECK(0 == strcmp(next, "c"));
    next = MPL_strsep(&str, "|");
    UT_TEST_CHECK(next == NULL);
    UT_TEST_CHECK(str == NULL);
    MPL_free(orig);
}

void test_mix_separator(void)
{
    char *orig;
    char *str;
    char *next;

    orig = MPL_strdup("a|b:c");
    str = orig;
    next = MPL_strsep(&str, ":|");
    UT_TEST_CHECK(next == orig);
    UT_TEST_CHECK(0 == strcmp(next, "a"));
    next = MPL_strsep(&str, ":|");
    UT_TEST_CHECK(0 == strcmp(next, "b"));
    next = MPL_strsep(&str, ":|");
    UT_TEST_CHECK(0 == strcmp(next, "c"));
    next = MPL_strsep(&str, ":|");
    UT_TEST_CHECK(next == NULL);
    UT_TEST_CHECK(str == NULL);
    MPL_free(orig);
}

void test_mix_combined_sep(void)
{
    char *orig;
    char *str;
    char *next;

    orig = MPL_strdup("a|:b:c");
    str = orig;
    next = MPL_strsep(&str, ":|");
    UT_TEST_CHECK(next == orig);
    UT_TEST_CHECK(0 == strcmp(next, "a"));
    next = MPL_strsep(&str, ":|");
    UT_TEST_CHECK(0 == strcmp(next, ""));
    next = MPL_strsep(&str, ":|");
    UT_TEST_CHECK(0 == strcmp(next, "b"));
    next = MPL_strsep(&str, ":|");
    UT_TEST_CHECK(0 == strcmp(next, "c"));
    next = MPL_strsep(&str, ":|");
    UT_TEST_CHECK(next == NULL);
    UT_TEST_CHECK(str == NULL);
    MPL_free(orig);
}

UT_TEST_SET = {
    { "null", test_nullstr },
    { "empty string", test_empty_str },
    { "a|b|c", test_str },
    { "a|b:c", test_mix_separator },
    { "a|:b:c", test_mix_combined_sep },
    { NULL, NULL }
};
