/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

#define MAX_BUF_SIZE_LG 22
#define NUM_MSGS_PER_BUF_SIZE 5
char buf[1 << MAX_BUF_SIZE_LG];

/*
 * This program verifies that MPI_Probe() is operating properly in the face of
 * unexpected messages arriving after MPI_Probe() has
 * been called.  This program may hang if MPI_Probe() does not return when the
 * message finally arrives (see req #375).
 */
int main(int argc, char **argv)
{
    int p_size;
    int p_rank;
    int msg_size_lg;
    int errs = 0;
    int mpi_errno;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &p_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &p_rank);
    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);


    for (msg_size_lg = 0; msg_size_lg <= MAX_BUF_SIZE_LG; msg_size_lg++) {
        const int msg_size = 1 << msg_size_lg;
        int msg_cnt;

        MTestPrintfMsg(2, "testing messages of size %d\n", msg_size);
        for (msg_cnt = 0; msg_cnt < NUM_MSGS_PER_BUF_SIZE; msg_cnt++) {
            MPI_Status status;
            const int tag = msg_size_lg * NUM_MSGS_PER_BUF_SIZE + msg_cnt;

            MTestPrintfMsg(2, "Message count %d\n", msg_cnt);
            if (p_rank == 0) {
                int p;

                for (p = 1; p < p_size; p++) {
                    /* Wait for synchronization message */
                    mpi_errno = MPI_Recv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE,
                                         tag, MPI_COMM_WORLD, &status);
                    if (mpi_errno != MPI_SUCCESS && errs++ < 10) {
                        MTestPrintError(mpi_errno);
                    }

                    if (status.MPI_TAG != tag && errs++ < 10) {
                        printf
                            ("ERROR: unexpected message tag from MPI_Recv(): lp=0, rp=%d, expected=%d, actual=%d, count=%d\n",
                             status.MPI_SOURCE, status.MPI_TAG, tag, msg_cnt);
                    }

#		    if defined(VERBOSE)
                    {
                        printf("sending message: p=%d s=%d c=%d\n",
                               status.MPI_SOURCE, msg_size, msg_cnt);
                    }
#		    endif

                    /* Send unexpected message which hopefully MPI_Probe() is
                     * already waiting for at the remote process */
                    mpi_errno = MPI_Send(buf, msg_size, MPI_BYTE,
                                         status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD);
                    if (mpi_errno != MPI_SUCCESS && errs++ < 10) {
                        MTestPrintError(mpi_errno);
                    }
                }
            }
            else {
                int incoming_msg_size;

                /* Send synchronization message */
                mpi_errno = MPI_Send(NULL, 0, MPI_BYTE, 0, tag, MPI_COMM_WORLD);
                if (mpi_errno != MPI_SUCCESS && errs++ < 10) {
                    MTestPrintError(mpi_errno);
                }

                /* Perform probe, hopefully before the master process can
                 * send its reply */
                mpi_errno = MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                if (mpi_errno != MPI_SUCCESS && errs++ < 10) {
                    MTestPrintError(mpi_errno);
                }
                mpi_errno = MPI_Get_count(&status, MPI_BYTE, &incoming_msg_size);
                if (mpi_errno != MPI_SUCCESS && errs++ < 10) {
                    MTestPrintError(mpi_errno);
                }
                if (status.MPI_SOURCE != 0 && errs++ < 10) {
                    printf
                        ("ERROR: unexpected message source from MPI_Probe(): p=%d, expected=0, actual=%d, count=%d\n",
                         p_rank, status.MPI_SOURCE, msg_cnt);
                }
                if (status.MPI_TAG != tag && errs++ < 10) {
                    printf
                        ("ERROR: unexpected message tag from MPI_Probe(): p=%d, expected=%d, actual=%d, count=%d\n",
                         p_rank, tag, status.MPI_TAG, msg_cnt);
                }
                if (incoming_msg_size != msg_size && errs++ < 10) {
                    printf
                        ("ERROR: unexpected message size from MPI_Probe(): p=%d, expected=%d, actual=%d, count=%d\n",
                         p_rank, msg_size, incoming_msg_size, msg_cnt);
                }

                /* Receive the probed message from the master process */
                mpi_errno = MPI_Recv(buf, msg_size, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
                if (mpi_errno != MPI_SUCCESS && errs++ < 10) {
                    MTestPrintError(mpi_errno);
                }
                mpi_errno = MPI_Get_count(&status, MPI_BYTE, &incoming_msg_size);
                if (mpi_errno != MPI_SUCCESS && errs++ < 10) {
                    MTestPrintError(mpi_errno);
                }
                if (status.MPI_SOURCE != 0 && errs++ < 10) {
                    printf
                        ("ERROR: unexpected message source from MPI_Recv(): p=%d, expected=0, actual=%d, count=%d\n",
                         p_rank, status.MPI_SOURCE, msg_cnt);
                }
                if (status.MPI_TAG != tag && errs++ < 10) {
                    printf
                        ("ERROR: unexpected message tag from MPI_Recv(): p=%d, expected=%d, actual=%d, count=%d\n",
                         p_rank, tag, status.MPI_TAG, msg_cnt);
                }
                if (incoming_msg_size != msg_size && errs++ < 10) {
                    printf
                        ("ERROR: unexpected message size from MPI_Recv(): p=%d, expected=%d, actual=%d, count=%d\n",
                         p_rank, msg_size, incoming_msg_size, msg_cnt);
                }
            }
        }
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
