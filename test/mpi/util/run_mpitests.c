/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>

#define ARGS_MAX 1024
int wrank, wsize;

#ifdef MULTI_TESTS

#include "multi_tests.c"

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
            /* reset errhandler in case last test set it to something else */
            MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);

            int ret = fn(args);
            alarm(0);   /* cancel timeout */
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

#ifdef MULTI_TESTS
    finalize_multi_tests();
#endif
    MPI_Finalize();
    return ret;
}
