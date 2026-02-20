/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

/* Test MPI_Bsend_iflush. */

/* NOTE: this test send self messages for more predictable behavior. */

#define MSG_SIZE 100000

int main(int argc, char *argv[])
{
    int errs = 0;
    char send_buf[MSG_SIZE];
    char send_buf2[MSG_SIZE];
    char recv_buf[MSG_SIZE];
    char recv_buf2[MSG_SIZE];

    MPI_Comm comm = MPI_COMM_WORLD;

    MTest_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(comm, &rank);

    int bufsize = MPI_BSEND_OVERHEAD + MSG_SIZE;
    void *buf = (char *) malloc(bufsize);
    MPI_Buffer_attach(buf, bufsize);

    MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

    int errno, err_class;
    int tag = 0;

    errno = MPI_Bsend(send_buf, MSG_SIZE, MPI_BYTE, rank, tag, comm);
    if (errno != MPI_SUCCESS) {
        printf("First Bsend failed but we expect it to succeed.\n");
        errs++;
    }

    errno = MPI_Bsend(send_buf2, MSG_SIZE, MPI_BYTE, rank, tag, comm);
    MPI_Error_class(errno, &err_class);
    if (err_class != MPI_ERR_BUFFER) {
        printf("We expect the 2nd Bsend to fail with MPI_ERR_BUFFER,"
               "but got error code %d (class %d)\n", errno, err_class);
        errs++;
    }

    MPI_Request request;
    errno = MPI_Buffer_iflush(&request);
    if (errno) {
        printf("Failed MPI_Buffer_iflush, error code = %d\n", errno);
        errs++;
    }

    /* post a Recv or the flush won't complete */
    errno = MPI_Recv(recv_buf, MSG_SIZE, MPI_BYTE, rank, tag, comm, MPI_STATUS_IGNORE);
    if (errno) {
        printf("Failed the first MPI_Recv, error code = %d\n", errno);
        errs++;
    }

    /* wait for flush to complete */
    errno = MPI_Wait(&request, MPI_STATUS_IGNORE);
    if (errno) {
        printf("Failed MPI_Wait, error code = %d\n", errno);
        errs++;
    }

    /* now the 2nd bsend should succeed */
    errno = MPI_Bsend(send_buf2, MSG_SIZE, MPI_BYTE, rank, tag, comm);
    if (errno != MPI_SUCCESS) {
        printf("The 2nd Bsend failed but we expect it to succeed.\n");
        errs++;
    }

    /* recv the 2nd */
    errno = MPI_Recv(recv_buf2, MSG_SIZE, MPI_BYTE, rank, tag, comm, MPI_STATUS_IGNORE);
    if (errno) {
        printf("Failed the second MPI_Recv, error code = %d\n", errno);
        errs++;
    }

    void *bbuf;
    int bsize;
    errno = MPI_Buffer_detach(&bbuf, &bsize);
    if (errno) {
        printf("Failed MPI_Buffer_detach, error code = %d\n", errno);
        errs++;
    }
    free(buf);

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
