/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test the routines to control error handlers on windows";
*/

static int calls = 0;
static int errs = 0;
static MPI_Win mywin;
static int expected_err_class = MPI_ERR_OTHER;

static int w1Called = 0;
static int w2Called = 0;

void weh1(MPI_Win * win, int *err, ...);
void weh1(MPI_Win * win, int *err, ...)
{
    int errclass;
    w1Called++;
    MPI_Error_class(*err, &errclass);
    if (errclass != expected_err_class) {
        errs++;
        printf("Unexpected error code (class = %d)\n", errclass);
    }
    if (*win != mywin) {
        errs++;
        printf("Unexpected window (got %x expected %x)\n", (int) *win, (int) mywin);
    }
    calls++;
    return;
}

void weh2(MPI_Win * win, int *err, ...);
void weh2(MPI_Win * win, int *err, ...)
{
    int errclass;
    w2Called++;
    MPI_Error_class(*err, &errclass);
    if (errclass != expected_err_class) {
        errs++;
        printf("Unexpected error code (class = %d)\n", errclass);
    }
    if (*win != mywin) {
        errs++;
        printf("Unexpected window (got %x expected %x)\n", (int) *win, (int) mywin);
    }
    calls++;
    return;
}

int main(int argc, char *argv[])
{
    int err;
    int buf[2];
    MPI_Win win;
    MPI_Comm comm;
    MPI_Errhandler newerr1, newerr2, olderr;


    MTest_Init(&argc, &argv);
    comm = MPI_COMM_WORLD;
    MPI_Win_create_errhandler(weh1, &newerr1);
    MPI_Win_create_errhandler(weh2, &newerr2);

    MPI_Win_create(buf, 2 * sizeof(int), sizeof(int), MPI_INFO_NULL, comm, &win);

    mywin = win;
    MPI_Win_get_errhandler(win, &olderr);
    if (olderr != MPI_ERRORS_ARE_FATAL) {
        errs++;
        printf("Expected errors are fatal\n");
    }

    MPI_Win_set_errhandler(win, newerr1);
    /* We should be able to free the error handler now since the window is
     * using it */
    MPI_Errhandler_free(&newerr1);

    expected_err_class = MPI_ERR_RANK;
    err = MPI_Put(buf, 1, MPI_INT, -5, 0, 1, MPI_INT, win);
    if (w1Called != 1) {
        errs++;
        printf("newerr1 not called\n");
        w1Called = 1;
    }
    expected_err_class = MPI_ERR_OTHER;
    MPI_Win_call_errhandler(win, MPI_ERR_OTHER);
    if (w1Called != 2) {
        errs++;
        printf("newerr1 not called (2)\n");
    }

    if (w1Called != 2 || w2Called != 0) {
        errs++;
        printf("Error handler weh1 not called the expected number of times\n");
    }

    /* Try another error handler.  This should allow the MPI implementation to
     * free the first error handler */
    MPI_Win_set_errhandler(win, newerr2);
    MPI_Errhandler_free(&newerr2);

    expected_err_class = MPI_ERR_RANK;
    err = MPI_Put(buf, 1, MPI_INT, -5, 0, 1, MPI_INT, win);
    if (w2Called != 1) {
        errs++;
        printf("newerr2 not called\n");
        calls = 1;
    }
    expected_err_class = MPI_ERR_OTHER;
    MPI_Win_call_errhandler(win, MPI_ERR_OTHER);
    if (w2Called != 2) {
        errs++;
        printf("newerr2 not called (2)\n");
    }
    if (w1Called != 2 || w2Called != 2) {
        errs++;
        printf("Error handler weh1 not called the expected number of times\n");
    }

    MPI_Win_free(&win);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
