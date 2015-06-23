/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test lock contention while streaming ACC-like operations";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size;
    int target = 1;
    int minsize = 2, count;
    MPI_Comm comm;
    MPI_Win win;
    MPI_Aint lb, extent;
    MTestDatatype sendtype, recvtype;

    MTest_Init(&argc, &argv);

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;

        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        MTEST_DATATYPE_FOR_EACH_COUNT(count) {
            while (MTestGetDatatypes(&sendtype, &recvtype, count)) {
                recvtype.printErrors = 1;
                recvtype.InitBuf(&recvtype);
                MPI_Type_get_extent(recvtype.datatype, &lb, &extent);

                MPI_Win_create(recvtype.buf, lb + recvtype.count * extent,
                               (int) extent, MPI_INFO_NULL, comm, &win);
                if (rank != target) {
                    sendtype.InitBuf(&sendtype);

                    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
                    MPI_Accumulate(sendtype.buf, sendtype.count,
                                   sendtype.datatype, target, 0,
                                   recvtype.count, recvtype.datatype, MPI_REPLACE, win);
                    MPI_Win_unlock(target, win);
                    MPI_Barrier(comm);

                    char *resbuf = (char *) calloc(lb + extent * recvtype.count, sizeof(char));

                    /*wait for the destination to finish checking and reinitializing the buffer */
                    MPI_Barrier(comm);

                    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
                    MPI_Get_accumulate(sendtype.buf, sendtype.count,
                                       sendtype.datatype, resbuf, recvtype.count, recvtype.datatype,
                                       target, 0, recvtype.count, recvtype.datatype, MPI_REPLACE,
                                       win);
                    MPI_Win_unlock(target, win);
                    MPI_Barrier(comm);
                    free(resbuf);
                }
                else {  /* Target checks the result */
                    int err;
                    MPI_Barrier(comm);
                    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
                    err = MTestCheckRecv(0, &recvtype);
                    if (err)
                        errs++;
                    recvtype.InitBuf(&recvtype);
                    MPI_Win_unlock(rank, win);

                    /*signal the source that checking and reinitialization is done */
                    MPI_Barrier(comm);

                    MPI_Barrier(comm);
                    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
                    err = MTestCheckRecv(0, &recvtype);
                    if (err)
                        errs++;
                    MPI_Win_unlock(rank, win);
                }

                MPI_Win_free(&win);
                MTestFreeDatatype(&sendtype);
                MTestFreeDatatype(&recvtype);
            }
        }
        MTestFreeComm(&comm);
    }
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
