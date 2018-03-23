/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpitest.h"

/* tests noncontiguous reads/writes using nonblocking collective I/O */

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
    int *buf, i, mynod, nprocs, len, b[3];
    int errs = 0, err = MPI_SUCCESS;
    MPI_Aint d[3];
    MPI_File fh;
    MPI_Request request;
    MPI_Status status;
    char *filename;
    MPI_Datatype typevec, newtype, t[3];

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &mynod);

    if (nprocs != 2) {
        fprintf(stderr, "Run this program on two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* process 0 broadcasts the file name to other processes */
    if (!mynod) {
        filename = "testfile";
        len = strlen(filename);
        MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(filename, len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
    } else {
        MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        filename = (char *) malloc(len + 1);
        MPI_Bcast(filename, len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    buf = (int *) malloc(SIZE * sizeof(int));

    MPI_Type_vector(SIZE / 2, 1, 2, MPI_INT, &typevec);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = mynod * sizeof(int);
    d[2] = SIZE * sizeof(int);
    t[0] = MPI_LB;
    t[1] = typevec;
    t[2] = MPI_UB;

    MPI_Type_struct(3, b, d, t, &newtype);
    MPI_Type_commit(&newtype);
    MPI_Type_free(&typevec);

    if (!mynod) {
#if VERBOSE
        fprintf(stderr, "\ntesting noncontiguous in memory, noncontiguous in "
                "file using collective I/O\n");
#endif
        MPI_File_delete(filename, MPI_INFO_NULL);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    err =
        MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL,
                      &fh);
    HANDLE_ERROR(err);

    err = MPI_File_set_view(fh, 0, MPI_INT, newtype, "native", MPI_INFO_NULL);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++)
        buf[i] = i + mynod * SIZE;
    err = MPI_File_iwrite_all(fh, buf, 1, newtype, &request);
    HANDLE_ERROR(err);

    MPI_Barrier(MPI_COMM_WORLD);
    err = MPI_Wait(&request, &status);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++)
        buf[i] = -1;

    err = MPI_File_iread_at_all(fh, 0, buf, 1, newtype, &request);
    HANDLE_ERROR(err);
    err = MPI_Wait(&request, &status);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++) {
        if (!mynod) {
            if ((i % 2) && (buf[i] != -1)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be -1\n", mynod, i, buf[i]);
            }
            if (!(i % 2) && (buf[i] != i)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n", mynod, i, buf[i], i);
            }
        } else {
            if ((i % 2) && (buf[i] != i + mynod * SIZE)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n",
                        mynod, i, buf[i], i + mynod * SIZE);
            }
            if (!(i % 2) && (buf[i] != -1)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be -1\n", mynod, i, buf[i]);
            }
        }
    }

    err = MPI_File_close(&fh);
    HANDLE_ERROR(err);

    MPI_Barrier(MPI_COMM_WORLD);

    if (!mynod) {
#if VERBOSE
        fprintf(stderr, "\ntesting noncontiguous in memory, contiguous in file "
                "using collective I/O\n");
#endif
        MPI_File_delete(filename, MPI_INFO_NULL);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    err =
        MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL,
                      &fh);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++)
        buf[i] = i + mynod * SIZE;
    err = MPI_File_iwrite_at_all(fh, mynod * (SIZE / 2) * sizeof(int), buf, 1, newtype, &request);
    HANDLE_ERROR(err);

    MPI_Barrier(MPI_COMM_WORLD);
    err = MPI_Wait(&request, &status);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++)
        buf[i] = -1;

    err = MPI_File_iread_at_all(fh, mynod * (SIZE / 2) * sizeof(int), buf, 1, newtype, &request);
    HANDLE_ERROR(err);
    err = MPI_Wait(&request, &status);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++) {
        if (!mynod) {
            if ((i % 2) && (buf[i] != -1)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be -1\n", mynod, i, buf[i]);
            }
            if (!(i % 2) && (buf[i] != i)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n", mynod, i, buf[i], i);
            }
        } else {
            if ((i % 2) && (buf[i] != i + mynod * SIZE)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n",
                        mynod, i, buf[i], i + mynod * SIZE);
            }
            if (!(i % 2) && (buf[i] != -1)) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be -1\n", mynod, i, buf[i]);
            }
        }
    }

    err = MPI_File_close(&fh);
    HANDLE_ERROR(err);

    MPI_Barrier(MPI_COMM_WORLD);

    if (!mynod) {
#if VERBOSE
        fprintf(stderr, "\ntesting contiguous in memory, noncontiguous in file "
                "using collective I/O\n");
#endif
        MPI_File_delete(filename, MPI_INFO_NULL);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    err =
        MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL,
                      &fh);
    HANDLE_ERROR(err);

    err = MPI_File_set_view(fh, 0, MPI_INT, newtype, "native", MPI_INFO_NULL);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++)
        buf[i] = i + mynod * SIZE;
    err = MPI_File_iwrite_all(fh, buf, SIZE, MPI_INT, &request);
    HANDLE_ERROR(err);

    MPI_Barrier(MPI_COMM_WORLD);
    err = MPI_Wait(&request, &status);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++)
        buf[i] = -1;

    err = MPI_File_iread_at_all(fh, 0, buf, SIZE, MPI_INT, &request);
    HANDLE_ERROR(err);
    err = MPI_Wait(&request, &status);
    HANDLE_ERROR(err);

    for (i = 0; i < SIZE; i++) {
        if (!mynod) {
            if (buf[i] != i) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n", mynod, i, buf[i], i);
            }
        } else {
            if (buf[i] != i + mynod * SIZE) {
                errs++;
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n",
                        mynod, i, buf[i], i + mynod * SIZE);
            }
        }
    }

    err = MPI_File_close(&fh);
    HANDLE_ERROR(err);

    MPI_Type_free(&newtype);
    free(buf);
    if (mynod)
        free(filename);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
