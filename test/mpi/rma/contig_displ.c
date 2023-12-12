/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_contig_displ
int run(const char *arg);
#endif

/* Run with 1 process.

   This program does an MPI_Get with an indexed datatype. The datatype
   comprises a single integer at an initial displacement of 1 integer.
   That is, the first integer in the array is to be skipped.

   This program found a bug in IBM's MPI in which MPI_Get ignored the
   displacement and got the first integer instead of the second.
*/

int run(const char *arg)
{
    int rank, nprocs, mpi_err, *array;
    int getval, disp, errs = 0;
    MPI_Win win;
    MPI_Datatype type;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (rank == 0) {
        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);

        /* create an indexed datatype that points to the second integer
         * in an array (the first integer is skipped). */
        disp = 1;
        mpi_err = MPI_Type_create_indexed_block(1, 1, &disp, MPI_INT, &type);
        if (mpi_err != MPI_SUCCESS)
            goto err_return;
        mpi_err = MPI_Type_commit(&type);
        if (mpi_err != MPI_SUCCESS)
            goto err_return;

        /* allocate window of size 2 integers */
        mpi_err = MPI_Alloc_mem(2 * sizeof(int), MPI_INFO_NULL, &array);
        if (mpi_err != MPI_SUCCESS)
            goto err_return;

        /* create window object */
        mpi_err =
            MPI_Win_create(array, 2 * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_SELF, &win);
        if (mpi_err != MPI_SUCCESS)
            goto err_return;

        /* initialize array */
        array[0] = 100;
        array[1] = 200;

        getval = 0;

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

        mpi_err = MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
        if (mpi_err != MPI_SUCCESS)
            goto err_return;

        /* get the current value of element array[1] */
        mpi_err = MPI_Get(&getval, 1, MPI_INT, 0, 0, 1, type, win);
        if (mpi_err != MPI_SUCCESS)
            goto err_return;

        mpi_err = MPI_Win_unlock(0, win);
        if (mpi_err != MPI_SUCCESS)
            goto err_return;

        /* getval should contain the value of array[1] */
        if (getval != array[1]) {
            errs++;
            printf("getval=%d, should be %d\n", getval, array[1]);
        }

        MPI_Free_mem(array);
        MPI_Win_free(&win);
        MPI_Type_free(&type);
    }

    return errs;

  err_return:
    printf("MPI function error returned an error\n");
    MTestPrintError(mpi_err);
    errs++;
    return errs;
}
