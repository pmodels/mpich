/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* CFLAGS: -I include */
/* SOURCE: src/str/mpl_str.c */

#include "mpl_str.h"

/* TEST: MPL_strsep - NULL */
int main(void)
{
    char *str;
    char *next;

    str = NULL;
    next = MPL_strsep(&str, "|");
    assert(next == NULL);
    assert(str == NULL);
    return 0;
}

/* TEST: MPL_strsep - empty */
int main(void)
{
    char *orig;
    char *str;
    char *next;

    orig = strdup("");
    str = orig;
    next = MPL_strsep(&str, "|");
    assert(str == NULL);
    assert(next == orig);
    free(orig);

    return 0;
}

/* TEST: MPL_strsep - a|b|c */
int main(void)
{
    char *orig;
    char *str;
    char *next;

    orig = strdup("a|b|c");
    str = orig;
    next = MPL_strsep(&str, "|");
    assert(next == orig);
    assert(0 == strcmp(next, "a"));
    next = MPL_strsep(&str, "|");
    assert(0 == strcmp(next, "b"));
    next = MPL_strsep(&str, "|");
    assert(0 == strcmp(next, "c"));
    next = MPL_strsep(&str, "|");
    assert(next == NULL);
    assert(str == NULL);
    free(orig);

    return 0;
}

/* TEST: MPL_strsep - a|b:c */
int main(void)
{
    char *orig;
    char *str;
    char *next;

    orig = strdup("a|b:c");
    str = orig;
    next = MPL_strsep(&str, ":|");
    assert(next == orig);
    assert(0 == strcmp(next, "a"));
    next = MPL_strsep(&str, ":|");
    assert(0 == strcmp(next, "b"));
    next = MPL_strsep(&str, ":|");
    assert(0 == strcmp(next, "c"));
    next = MPL_strsep(&str, ":|");
    assert(next == NULL);
    assert(str == NULL);
    free(orig);

    return 0;
}

/* TEST: MPL_strsep - a|:b:c */
int main(void)
{
    char *orig;
    char *str;
    char *next;

    orig = strdup("a|:b:c");
    str = orig;
    next = MPL_strsep(&str, ":|");
    assert(next == orig);
    assert(0 == strcmp(next, "a"));
    next = MPL_strsep(&str, ":|");
    assert(0 == strcmp(next, ""));
    next = MPL_strsep(&str, ":|");
    assert(0 == strcmp(next, "b"));
    next = MPL_strsep(&str, ":|");
    assert(0 == strcmp(next, "c"));
    next = MPL_strsep(&str, ":|");
    assert(next == NULL);
    assert(str == NULL);
    free(orig);

    return 0;
}
