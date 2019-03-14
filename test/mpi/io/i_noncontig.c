/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpitest.h"
#include "test_io.h"

/* tests noncontiguous reads/writes using nonblocking I/O */

/*
static char MTEST_Descrip[] = "Test nonblocking I/O";
*/

#define SIZE 5000

#define VERBOSE 0

#define HANDLE_ERROR(err) \
    if (err != MPI_SUCCESS) { \
        char msg[MPI_MAX_ERROR_STRING]; \
        int resultlen; \
        MPI_Error_string(err, msg, &resultlen); \
        fprintf(stderr, "%s line %d: %s\n", __FILE__, __LINE__, msg); \
        MPI_Abort(MPI_COMM_WORLD, 1); \
    }

int main(int argc, char **argv)
{
    int *buf, i, rank, nprocs, b[3];
    int err, errs = 0;
    MPI_Aint d[3];
    MPI_File fh;
    MPI_Status status;
    MPI_Datatype typevec, newtype, t[3];
    MPI_Request req;
    INIT_FILENAME;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    GET_TEST_FILENAME;

    if (nprocs != 2) {
        fprintf(stderr, "Run this program on two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    buf = (int *) malloc(SIZE * sizeof(int));

    MPI_Type_vector(SIZE / 2, 1, 2, MPI_INT, &typevec);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = rank * sizeof(int);
    d[2] = SIZE * sizeof(int);
    t[0] = MPI_LB;
    t[1] = typevec;
    t[2] = MPI_UB;

    MPI_Type_struct(3, b, d, t, &newtype);
    MPI_Type_commit(&newtype);
    MPI_Type_free(&typevec);

    if (!rank) {
#if VERBOSE
        fprintf(stderr,
                "\ntesting noncontiguous in memory, noncontiguous in file using nonblocking I/O\n");
#endif
        MPI_File_delete(filename, MPI_INFO_NULL);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    err =
        MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL,
                      &fh);
    HANDLE_ERROR(err);

    err = MPI_File_set_view(fh, 0, MPI_INT, newtype, (char *) "native", MPI_INFO_NULL);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++)
        buf[i] = i + rank * SIZE;
    err = MPI_File_iwrite(fh, buf, 1, newtype, &req);
    HANDLE_ERROR(err);
    err = MPI_Wait(&req, &status);
    HANDLE_ERROR(err);

    MPI_Barrier(MPI_COMM_WORLD);

    for (i = 0; i < SIZE; i++)
        buf[i] = -1;

    err = MPI_File_iread_at(fh, 0, buf, 1, newtype, &req);
    HANDLE_ERROR(err);
    err = MPI_Wait(&req, &status);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++) {
        if (!rank) {
            if ((i % 2) && (buf[i] != -1)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be -1\n", rank, i, buf[i]);
            }
            if (!(i % 2) && (buf[i] != i)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n", rank, i, buf[i], i);
            }
        } else {
            if ((i % 2) && (buf[i] != i + rank * SIZE)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n",
                        rank, i, buf[i], i + rank * SIZE);
            }
            if (!(i % 2) && (buf[i] != -1)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be -1\n", rank, i, buf[i]);
            }
        }
    }

    err = MPI_File_close(&fh);
    HANDLE_ERROR(err);

    MPI_Barrier(MPI_COMM_WORLD);

    if (!rank) {
#if VERBOSE
        fprintf(stderr,
                "\ntesting noncontiguous in memory, contiguous in file using nonblocking I/O\n");
#endif
        MPI_File_delete(filename, MPI_INFO_NULL);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    err =
        MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL,
                      &fh);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++)
        buf[i] = i + rank * SIZE;
    err = MPI_File_iwrite_at(fh, rank * (SIZE / 2) * sizeof(int), buf, 1, newtype, &req);
    HANDLE_ERROR(err);
    err = MPI_Wait(&req, &status);
    HANDLE_ERROR(err);

    MPI_Barrier(MPI_COMM_WORLD);

    for (i = 0; i < SIZE; i++)
        buf[i] = -1;

    err = MPI_File_iread_at(fh, rank * (SIZE / 2) * sizeof(int), buf, 1, newtype, &req);
    HANDLE_ERROR(err);
    err = MPI_Wait(&req, &status);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++) {
        if (!rank) {
            if ((i % 2) && (buf[i] != -1)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be -1\n", rank, i, buf[i]);
            }
            if (!(i % 2) && (buf[i] != i)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n", rank, i, buf[i], i);
            }
        } else {
            if ((i % 2) && (buf[i] != i + rank * SIZE)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n",
                        rank, i, buf[i], i + rank * SIZE);
            }
            if (!(i % 2) && (buf[i] != -1)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be -1\n", rank, i, buf[i]);
            }
        }
    }

    err = MPI_File_close(&fh);
    HANDLE_ERROR(err);

    MPI_Barrier(MPI_COMM_WORLD);

    if (!rank) {
#if VERBOSE
        fprintf(stderr,
                "\ntesting contiguous in memory, noncontiguous in file using nonblocking I/O\n");
#endif
        MPI_File_delete(filename, MPI_INFO_NULL);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    err =
        MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL,
                      &fh);
    HANDLE_ERROR(err);

    err = MPI_File_set_view(fh, 0, MPI_INT, newtype, (char *) "native", MPI_INFO_NULL);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++)
        buf[i] = i + rank * SIZE;
    err = MPI_File_iwrite(fh, buf, SIZE, MPI_INT, &req);
    HANDLE_ERROR(err);
    err = MPI_Wait(&req, &status);
    HANDLE_ERROR(err);

    MPI_Barrier(MPI_COMM_WORLD);

    for (i = 0; i < SIZE; i++)
        buf[i] = -1;

    err = MPI_File_iread_at(fh, 0, buf, SIZE, MPI_INT, &req);
    HANDLE_ERROR(err);
    err = MPI_Wait(&req, &status);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++) {
        if (!rank) {
            if (buf[i] != i) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n", rank, i, buf[i], i);
            }
        } else {
            if (buf[i] != i + rank * SIZE) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n",
                        rank, i, buf[i], i + rank * SIZE);
            }
        }
    }

    err = MPI_File_close(&fh);
    HANDLE_ERROR(err);

    MPI_Type_free(&newtype);
    free(buf);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
