/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#define ARGS_MAX 1024
int wrank, wsize;

#ifdef MULTI_TESTS
extern struct mpitest alltests[];
int num_tests;

FILE *test_in;
FILE *test_out;

int cmp_testname(const void *a, const void *b)
{
    return strcmp(((struct mpitest *) a)->name, ((struct mpitest *) b)->name);
}

static void init_multi_tests(void)
{
    int n = 0;
    while (alltests[n].name) {
        n++;
    }

    num_tests = n;

    qsort(alltests, num_tests, sizeof(struct mpitest), cmp_testname);
}

static run_fn find_test(const char *name)
{
    if (num_tests == 0) {
        return NULL;
    } else if (strcmp(name, alltests[0].name) == 0) {
        return alltests[0].run;
    } else if (num_tests > 1 && strcmp(name, alltests[num_tests - 1].name) == 0) {
        return alltests[num_tests - 1].run;
    }

    /* binary search */
    int i1 = 0;
    int i2 = num_tests - 1;
    while (i1 <= i2) {
        int i3 = (i1 + i2) / 2;
        int ret = strcmp(name, alltests[i3].name);
        if (ret == 0) {
            return alltests[i3].run;
        } else if (ret < 0) {
            i2 = i3 - 1;
        } else {
            i1 = i3 + 1;
        }
    }

    return NULL;
}

#define skip_space(s) while (*s && isspace(*s)) s++
#define skip_nonspace(s) while (*s && !isspace(*s)) s++
#define check_null_return(s) if (!*s) return

static void split_cmd(char *cmd, const char **name_out, const char **args_out)
{
    char *s = cmd;

    *name_out = NULL;
    *args_out = NULL;

    skip_space(s);
    check_null_return(s);
    *name_out = s;

    skip_nonspace(s);
    check_null_return(s);
    *s++ = '\0';

    skip_space(s);
    check_null_return(s);
    *args_out = s;
}

static bool get_test(const char **name, const char **args, run_fn * fn)
{
    static char cmd[ARGS_MAX];
    if (wrank == 0) {
        if (!fgets(cmd, ARGS_MAX, test_in)) {
            cmd[0] = '\0';
        }
    }

    MPI_Bcast(cmd, ARGS_MAX, MPI_CHAR, 0, MPI_COMM_WORLD);

    if (cmd[0] == '\0') {
        return false;
    } else {
        split_cmd(cmd, name, args);
        if (*name) {
            *fn = find_test(*name);
        } else {
            *fn = NULL;
        }
        return true;
    }
}

#else /* !MULTI_TESTS */
static void concat_argv(char *buf, int max, int argc, char **argv)
{
    int n = 0;
    for (int i = 1; i < argc; i++) {
        int len = strlen(argv[i]);
        if (len > max - n - 1) {
            len = max - n - 1;
        }
        if (n > 0) {
            buf[n++] = ' ';
        }
        strncpy(buf + n, argv[i], len);
        n += len;
    }
    buf[n] = '\0';
}
#endif /* MULTI_TESTS */

int main(int argc, char **argv)
{
    int ret = 0;

#ifdef MULTI_TESTS
    init_multi_tests();
    test_in = stdin;
    test_out = stdout;
#else
    /* concat argv into args string */
    char args[ARGS_MAX];
    concat_argv(args, ARGS_MAX, argc, argv);
#endif

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

#ifdef MULTI_TESTS
    run_fn fn;
    const char *name;
    const char *args;
    /* multi_tests are driven by a script via stdin/stdout */
    while (get_test(&name, &args, &fn)) {
        if (fn) {
            int ret = fn(args);
            int toterrs;
            MPI_Reduce(&ret, &toterrs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
            if (wrank == 0) {
                fprintf(test_out, "%s: %d\n", name, toterrs);
            }
        } else {
            if (wrank == 0) {
                fprintf(test_out, "%s: not found\n", name);
            }
        }
    }
#else
    ret = run(args);
    int toterrs;
    MPI_Reduce(&ret, &toterrs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (wrank == 0) {
        if (toterrs == 0) {
            printf("No Errors\n");
        } else {
            printf("Found %d errors\n", toterrs);
        }
    }
#endif

    MPI_Finalize();
    return ret;
}
