/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mpitest.h"

/*
 * Test handling of truncated messages, including short and rendezvous
 */

static int testShort = 1;
static int ShortLen = 2;
static int testMid = 1;
static int MidLen = 64;
static int testLong = 1;
static int LongLen = 100000;

int checkTruncError(int err, const char *msg);
int checkOk(int err, const char *msg);

int main(int argc, char *argv[])
{
    MPI_Status status;
    MPI_Message message;
    int err, errs = 0, source, dest, rank, size;
    int *buf = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    source = 0;
    dest = size - 1;

    buf = (int *) malloc(LongLen * sizeof(int));
    MTEST_VG_MEM_INIT(buf, LongLen * sizeof(int));
    if (!buf) {
        fprintf(stderr, "Unable to allocate communication buffer of size %d\n", LongLen);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
#if MTEST_HAVE_MIN_MPI_VERSION(3,0)
    if (testShort) {
        if (rank == source) {
            err = MPI_Send(buf, ShortLen, MPI_INT, dest, 0, MPI_COMM_WORLD);
            errs += checkOk(err, "short");
        } else if (rank == dest) {
            err = MPI_Mprobe(source, 0, MPI_COMM_WORLD, &message, MPI_STATUS_IGNORE);
            errs += checkOk(err, "mprobe");
            err = MPI_Mrecv(buf, ShortLen - 1, MPI_INT, &message, &status);
            errs += checkTruncError(err, "short");
        }
    }
    if (testMid) {
        if (rank == source) {
            err = MPI_Send(buf, MidLen, MPI_INT, dest, 0, MPI_COMM_WORLD);
            errs += checkOk(err, "medium");
        } else if (rank == dest) {
            err = MPI_Mprobe(source, 0, MPI_COMM_WORLD, &message, MPI_STATUS_IGNORE);
            errs += checkOk(err, "mprobe");
            err = MPI_Mrecv(buf, MidLen - 1, MPI_INT, &message, &status);
            errs += checkTruncError(err, "medium");
        }
    }
    if (testLong) {
        if (rank == source) {
            err = MPI_Send(buf, LongLen, MPI_INT, dest, 0, MPI_COMM_WORLD);
            errs += checkOk(err, "long");
        } else if (rank == dest) {
            err = MPI_Mprobe(source, 0, MPI_COMM_WORLD, &message, MPI_STATUS_IGNORE);
            errs += checkOk(err, "mprobe");
            err = MPI_Mrecv(buf, LongLen - 1, MPI_INT, &message, &status);
            errs += checkTruncError(err, "long");
        }
    }
#endif

    free(buf);
    MTest_Finalize(errs);

    return 0;
}


int checkTruncError(int err, const char *msg)
{
    char errMsg[MPI_MAX_ERROR_STRING];
    int errs = 0, msgLen, errclass;

    if (!err) {
        errs++;
        fprintf(stderr, "MPI_Mrecv (%s) returned MPI_SUCCESS instead of truncated message\n", msg);
        fflush(stderr);
    } else {
        MPI_Error_class(err, &errclass);
        if (errclass != MPI_ERR_TRUNCATE) {
            errs++;
            MPI_Error_string(err, errMsg, &msgLen);
            fprintf(stderr, "MPI_Mrecv (%s) returned unexpected error message: %s\n", msg, errMsg);
            fflush(stderr);
        }
    }
    return errs;
}

int checkOk(int err, const char *msg)
{
    char errMsg[MPI_MAX_ERROR_STRING];
    int errs = 0, msgLen;
    if (err) {
        errs++;
        MPI_Error_string(err, errMsg, &msgLen);
        fprintf(stderr, "MPI_Send(%s) failed with %s\n", msg, errMsg);
        fflush(stderr);
    }

    return errs;
}
