/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "mpitest.h"

#define LARGE_CNT_CONTIG    550000000
#define LARGE_CNT_NONCONTIG 150000000

/*
static char MTEST_Descrip[] = "Get with Fence";
*/

static inline int test(MPI_Comm comm, int rank, int source, int dest,
                       MTestDatatype * sendtype, MTestDatatype * recvtype)
{
    int errs = 0, err;
    int disp_unit;
    MPI_Aint extent, lb;
    MPI_Win win;

    MTestPrintfMsg(1,
                   "Getting count = %ld of sendtype %s - count = %ld receive type %s\n",
                   sendtype->count, MTestGetDatatypeName(sendtype), recvtype->count,
                   MTestGetDatatypeName(recvtype));
    /* Make sure that everyone has a recv buffer */
    recvtype->InitBuf(recvtype);
    sendtype->InitBuf(sendtype);
    /* By default, print information about errors */
    recvtype->printErrors = 1;
    sendtype->printErrors = 1;

    MPI_Type_extent(sendtype->datatype, &extent);
    MPI_Type_lb(sendtype->datatype, &lb);
    disp_unit = extent < INT_MAX ? extent : 1;
    MPI_Win_create(sendtype->buf, sendtype->count * extent + lb, disp_unit, MPI_INFO_NULL, comm, &win);
    MPI_Win_fence(0, win);
    if (rank == source) {
        /* The source does not need to do anything besides the
         * fence */
        MPI_Win_fence(0, win);
    }
    else if (rank == dest) {
        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

        /* This should have the same effect, in terms of
         * transfering data, as a send/recv pair */
        err = MPI_Get(recvtype->buf, recvtype->count,
                      recvtype->datatype, source, 0, sendtype->count, sendtype->datatype, win);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }
        err = MPI_Win_fence(0, win);
        if (err) {
            errs++;
            if (errs < 10) {
                MTestPrintError(err);
            }
        }
        err = MTestCheckRecv(0, recvtype);
        if (err) {
            errs += err;
        }
    }
    else {
        MPI_Win_fence(0, win);
    }
    MPI_Win_free(&win);

    return errs;
}


int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, source, dest;
    int minsize = 2, count;
    MPI_Comm comm;
    MTestDatatype sendtype, recvtype;

    MTest_Init(&argc, &argv);

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        source = 0;
        dest = size - 1;

        MTEST_DATATYPE_FOR_EACH_COUNT(count) {
            while (MTestGetDatatypes(&sendtype, &recvtype, count)) {
                errs += test(comm, rank, source, dest, &sendtype, &recvtype);
                MTestFreeDatatype(&sendtype);
                MTestFreeDatatype(&recvtype);
            }
        }
        MTestFreeComm(&comm);
    }

    /* Part #2: simple large size test - contiguous and noncontiguous */
    if (sizeof(void *) > 4) {   /* Only if > 32-bit architecture */
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        source = 0;
        dest = size - 1;

        MTestGetDatatypes(&sendtype, &recvtype, LARGE_CNT_CONTIG);
        errs += test(MPI_COMM_WORLD, rank, source, dest, &sendtype, &recvtype);

        do {
            MTestFreeDatatype(&sendtype);
            MTestFreeDatatype(&recvtype);
            MTestGetDatatypes(&sendtype, &recvtype, LARGE_CNT_NONCONTIG);
        } while (strstr(MTestGetDatatypeName(&sendtype), "vector") == NULL);
        errs += test(MPI_COMM_WORLD, rank, source, dest, &sendtype, &recvtype);
        MTestFreeDatatype(&sendtype);
        MTestFreeDatatype(&recvtype);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
