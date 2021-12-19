/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This file is to be included in run_mpitests.c to provide utility routines
 * for running multi-tests.
 */

extern struct mpitest alltests[];
int num_tests;

FILE *test_in;
FILE *test_out;

static int cmp_testname(const void *a, const void *b)
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

    test_in = stdin;
    test_out = stdout;
}

static void finalize_multi_tests(void)
{
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
