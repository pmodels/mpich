/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_p_scatterv
int run(const char *arg);
#endif

/*
   This is an example of using scatterv to send a matrix from one
   process to all others, with the matrix stored in Fortran order.
   Note the use of an explicit UB to enable the sources to overlap.

   This tests scatterv to make sure that it uses the datatype size
   and extent correctly.  It requires number of processors that
   can be split with MPI_Dims_create.

 */

static void SetData(double *sendbuf, double *recvbuf, int nx, int ny,
                    int myrow, int mycol, int nrow, int ncol)
{
    int coldim, i, j, m, k;
    double *p;

    if (myrow == 0 && mycol == 0) {
        coldim = nx * nrow;
        for (j = 0; j < ncol; j++) {
            for (i = 0; i < nrow; i++) {
                p = sendbuf + i * nx + j * (ny * coldim);
                for (m = 0; m < ny; m++) {
                    for (k = 0; k < nx; k++) {
                        p[k] = 1000 * j + 100 * i + m * nx + k;
                    }
                    p += coldim;
                }
            }
        }
    }
    for (i = 0; i < nx * ny; i++)
        recvbuf[i] = -1.0;
}

static int CheckData(double *recvbuf, int nx, int ny, int myrow, int mycol, int nrow,
                     int expect_no_value)
{
    int coldim, m, k;
    double *p, val;
    int errs = 0;

    coldim = nx;
    p = recvbuf;
    for (m = 0; m < ny; m++) {
        for (k = 0; k < nx; k++) {
            /* If expect_no_value is true then we assume that the pre-scatterv
             * value should remain in the recvbuf for our portion of the array.
             * This is the case for the root process when using MPI_IN_PLACE. */
            if (expect_no_value)
                val = -1.0;
            else
                val = 1000 * mycol + 100 * myrow + m * nx + k;

            if (p[k] != val) {
                errs++;
                if (errs < 10) {
                    printf("Error in (%d,%d) [%d,%d] location, got %f expected %f\n",
                           m, k, myrow, mycol, p[k], val);
                } else if (errs == 10) {
                    printf("Too many errors; suppressing printing\n");
                }
            }
        }
        p += coldim;
    }
    return errs;
}

int run(const char *arg)
{
    int rank, size, myrow, mycol, nx, ny, stride, cnt, i, j, errs, errs_in_place;
    double *sendbuf, *recvbuf;
    MPI_Datatype vec, block;
    int *scdispls;
    MPI_Comm comm2d;
    int dims[2], periods[2], coords[2], lcoords[2];
    int *sendcounts;

    MPI_Info info;
    MPI_Request req1, req2;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Info_create(&info);

    /* Get a 2-d decomposition of the processes */
    dims[0] = 0;
    dims[1] = 0;
    MPI_Dims_create(size, 2, dims);
    periods[0] = 0;
    periods[1] = 0;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &comm2d);
    MPI_Cart_get(comm2d, 2, dims, periods, coords);
    myrow = coords[0];
    mycol = coords[1];
/*
    if (rank == 0)
        printf("Decomposition is [%d x %d]\n", dims[0], dims[1]);
*/

    /* Get the size of the matrix */
    nx = 10;
    ny = 8;
    stride = nx * dims[0];

    recvbuf = (double *) malloc(nx * ny * sizeof(double));
    if (!recvbuf) {
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    sendbuf = 0;
    if (myrow == 0 && mycol == 0) {
        sendbuf = (double *) malloc(nx * ny * size * sizeof(double));
        if (!sendbuf) {
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    sendcounts = (int *) malloc(size * sizeof(int));
    scdispls = (int *) malloc(size * sizeof(int));

    MPI_Type_vector(ny, nx, stride, MPI_DOUBLE, &vec);
    MPI_Type_create_resized(vec, 0, nx * sizeof(double), &block);
    MPI_Type_free(&vec);
    MPI_Type_commit(&block);

    /* Set up the transfer */
    cnt = 0;
    for (i = 0; i < dims[1]; i++) {
        for (j = 0; j < dims[0]; j++) {
            sendcounts[cnt] = 1;
            /* Using Cart_coords makes sure that ranks (used by
             * sendrecv) matches the cartesian coordinates (used to
             * set data in the matrix) */
            MPI_Cart_coords(comm2d, cnt, 2, lcoords);
            scdispls[cnt++] = lcoords[0] + lcoords[1] * (dims[0] * ny);
        }
    }

    MPI_Scatterv_init(sendbuf, sendcounts, scdispls, block, recvbuf, nx * ny, MPI_DOUBLE, 0, comm2d,
                      info, &req1);
    for (i = 0; i < 10; ++i) {
        SetData(sendbuf, recvbuf, nx, ny, myrow, mycol, dims[0], dims[1]);

        MPI_Start(&req1);
        MPI_Wait(&req1, MPI_STATUS_IGNORE);

        if ((errs = CheckData(recvbuf, nx, ny, myrow, mycol, dims[0], 0))) {
            fprintf(stdout, "Failed to transfer data\n");
        }
    }
    MPI_Request_free(&req1);

    /* once more, but this time passing MPI_IN_PLACE for the root */
    MPI_Scatterv_init(sendbuf, sendcounts, scdispls, block, (rank == 0 ? MPI_IN_PLACE : recvbuf),
                      nx * ny, MPI_DOUBLE, 0, comm2d, info, &req2);
    for (i = 0; i < 2; ++i) {
        SetData(sendbuf, recvbuf, nx, ny, myrow, mycol, dims[0], dims[1]);

        MPI_Start(&req2);
        MPI_Wait(&req2, MPI_STATUS_IGNORE);

        errs_in_place = CheckData(recvbuf, nx, ny, myrow, mycol, dims[0], (rank == 0));
        if (errs_in_place) {
            fprintf(stdout, "Failed to transfer data (MPI_IN_PLACE)\n");
        }

        errs += errs_in_place;
    }
    MPI_Request_free(&req2);

    if (sendbuf)
        free(sendbuf);
    free(recvbuf);
    free(sendcounts);
    free(scdispls);
    MPI_Info_free(&info);
    MPI_Type_free(&block);
    MPI_Comm_free(&comm2d);

    return errs;
}
