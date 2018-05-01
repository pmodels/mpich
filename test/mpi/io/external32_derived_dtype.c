/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

#define HANDLE_ERROR(err) \
    if (err != MPI_SUCCESS) { \
        char msg[MPI_MAX_ERROR_STRING]; \
        int resultlen; \
        MPI_Error_string(err, msg, &resultlen); \
        fprintf(stderr, "%s line %d: %s\n", __FILE__, __LINE__, msg); \
        MPI_Abort(MPI_COMM_WORLD, 1); \
    }

static void read_file(const char *name, void *buf, MPI_Datatype dt)
{
    int rank, err;
    MPI_File fh;
    char datarep[] = "external32";
    int amode = MPI_MODE_RDONLY;
    MPI_Status status;
    MPI_Offset offset;

    /* get our rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* open file */
    err = MPI_File_open(MPI_COMM_WORLD, (char *) name, amode, MPI_INFO_NULL, &fh);
    HANDLE_ERROR(err);

    /* set file view to be sequence of datatypes past header */
    err = MPI_File_set_view(fh, 0, dt, dt, datarep, MPI_INFO_NULL);
    HANDLE_ERROR(err);

    /* issue a collective read: In 3.2 and older the external32 code
     * path had a bug that would cause an overlapping memcopy and crash
     */
    offset = rank;
    err = MPI_File_read_at_all(fh, offset, buf, 1, dt, &status);
    HANDLE_ERROR(err);

    /* close file */
    err = MPI_File_close(&fh);
    HANDLE_ERROR(err);

    return;
}

static void write_file(const char *name, void *buf, MPI_Datatype dt)
{
    int rank, amode, err;
    char datarep[] = "external32";
    MPI_Status status;
    MPI_File fh;
    MPI_Offset offset;

    /* get our rank in job */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* open file */
    amode = MPI_MODE_WRONLY | MPI_MODE_CREATE;
    err = MPI_File_open(MPI_COMM_WORLD, (char *) name, amode, MPI_INFO_NULL, &fh);
    HANDLE_ERROR(err);

    /* truncate file to 0 bytes */
    err = MPI_File_set_size(fh, 0);
    HANDLE_ERROR(err);

    /* set file view to be sequence of datatypes past header */
    err = MPI_File_set_view(fh, 0, dt, dt, datarep, MPI_INFO_NULL);
    HANDLE_ERROR(err);

    /* collective write of file info */
    offset = rank;
    err = MPI_File_write_at_all(fh, offset, buf, 1, dt, &status);
    HANDLE_ERROR(err);

    /* close file */
    err = MPI_File_close(&fh);
    HANDLE_ERROR(err);

    return;
}

/* write and read a file in which each process writes one int
 * in rank order */
int main(int argc, char *argv[])
{

    char buf[2] = "a";
    MPI_Datatype dt;
    int blocks[2] = { 1, 1 };
    int disps[2] = { 0, 1 };

    MTest_Init(&argc, &argv);
    MPI_Type_indexed(2, blocks, disps, MPI_CHAR, &dt);
    MPI_Type_commit(&dt);

    write_file("testfile", buf, dt);

    read_file("testfile", buf, dt);

    MPI_Type_free(&dt);

    MTest_Finalize(0);

    return 0;
}
