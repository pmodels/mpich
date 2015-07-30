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
static char MTEST_Descrip[] = "Test Request_get_status";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, source, dest;
    int buf[2], flag, count;
    MPI_Comm comm;
    MPI_Status status, status2;
    MPI_Request req;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    /* Determine the sender and receiver */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    source = 0;
    dest = size - 1;


    /* Handling MPI_REQUEST_NULL in MPI_Request_get_status was only required
     * starting with MPI-2.2. */
#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    MPI_Request_get_status(MPI_REQUEST_NULL, &flag, &status);
    if (!flag) {
        errs++;
        fprintf(stderr, "flag not true for MPI_REQUEST_NULL, flag=%d\n", flag);
    }
    if ((status.MPI_SOURCE != MPI_ANY_SOURCE) ||
        (status.MPI_TAG != MPI_ANY_TAG) || (status.MPI_ERROR != MPI_SUCCESS)) {
        errs++;
        fprintf(stderr, "non-empty MPI_Status returned for MPI_REQUEST_NULL\n");
    }

    /* also pass MPI_STATUS_IGNORE to make sure the implementation doesn't
     * blow up when it is passed as the status argument */
    MPI_Request_get_status(MPI_REQUEST_NULL, &flag, MPI_STATUS_IGNORE);
    if (!flag) {
        errs++;
        fprintf(stderr, "flag not true for MPI_REQUEST_NULL with MPI_STATUS_IGNORE, flag=%d\n",
                flag);
    }
#endif

    if (rank == source) {
        buf[0] = size;
        buf[1] = 3;
        MPI_Ssend(buf, 2, MPI_INT, dest, 10, comm);
    }
    if (rank == dest) {
        MPI_Irecv(buf, 2, MPI_INT, source, 10, comm, &req);
    }
    MPI_Barrier(comm);
    /* At this point, we know that the receive has at least started,
     * because of the Ssend.  Check the status on the request */
    if (rank == dest) {
        status.MPI_SOURCE = -1;
        status.MPI_TAG = -1;
        MPI_Request_get_status(req, &flag, &status);
        if (flag) {
            if (status.MPI_TAG != 10) {
                errs++;
                fprintf(stderr, "Tag value %d should be 10\n", status.MPI_TAG);
            }
            if (status.MPI_SOURCE != source) {
                errs++;
                fprintf(stderr, "Source value %d should be %d\n", status.MPI_SOURCE, source);
            }
            MPI_Get_count(&status, MPI_INT, &count);
            if (count != 2) {
                errs++;
                fprintf(stderr, "Count value %d should be 2\n", count);
            }
        }
        else {
            errs++;
            fprintf(stderr, "Unexpected flag value from get_status\n");
        }
        /* Now, complete the request */
        MPI_Wait(&req, &status2);
        /* Check that the status is correct */
        if (status2.MPI_TAG != 10) {
            errs++;
            fprintf(stderr, "(wait)Tag value %d should be 10\n", status2.MPI_TAG);
        }
        if (status2.MPI_SOURCE != source) {
            errs++;
            fprintf(stderr, "(wait)Source value %d should be %d\n", status2.MPI_SOURCE, source);
        }
        MPI_Get_count(&status2, MPI_INT, &count);
        if (count != 2) {
            errs++;
            fprintf(stderr, "(wait)Count value %d should be 2\n", count);
        }
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
