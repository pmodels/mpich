/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

static void read_file(const char *name, void *buf, MPI_Datatype dt)
{
    int rank, rc;
    MPI_File fh;
    char datarep[] = "external32";
    int amode = MPI_MODE_RDONLY;
    MPI_Status status;
    MPI_Offset offset;

    /* get our rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* open file */
    rc = MPI_File_open(MPI_COMM_WORLD, (char *) name,
	    amode, MPI_INFO_NULL, &fh);
    if (rc != MPI_SUCCESS) {
        printf("Rank %d: Failed to open file %s\n", rank, name);
        fflush(stdout);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return;
    }

    /* set file view to be sequence of datatypes past header */
    MPI_File_set_view(fh, 0, dt, dt, datarep, MPI_INFO_NULL);

    /* issue a collective read: In 3.2 and older the external32 code
     * path had a bug that would cause an overlapping memcopy and crash
     */
    offset = rank;
    MPI_File_read_at_all(fh, offset, buf, 1, dt, &status);

    /* close file */
    MPI_File_close(&fh);

    return;
}

static void write_file(const char *name, void *buf, MPI_Datatype dt)
{
    int rank, amode;
    char datarep[] = "external32";
    MPI_Status status;
    MPI_File fh;
    MPI_Offset offset;

    /* get our rank in job */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* open file */
    amode = MPI_MODE_WRONLY | MPI_MODE_CREATE;
    MPI_File_open(MPI_COMM_WORLD, (char *) name, amode, MPI_INFO_NULL, &fh);

    /* truncate file to 0 bytes */
    MPI_File_set_size(fh, 0);

    /* set file view to be sequence of datatypes past header */
    MPI_File_set_view(fh, 0, dt, dt, datarep, MPI_INFO_NULL);

    /* collective write of file info */
    offset = rank;
    MPI_File_write_at_all(fh, offset, buf, 1, dt, &status);

    /* close file */
    MPI_File_close(&fh);

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

    MPI_Init(&argc, &argv);
    MPI_Type_indexed(2, blocks, disps, MPI_CHAR, &dt);
    MPI_Type_commit(&dt);

    write_file("testfile", buf, dt);

    read_file("testfile", buf, dt);

    MPI_Type_free(&dt);

    /* if we get this far, then we've passed.  No verification in this test at
     * this time. */
    fprintf(stdout, " No Errors\n");

    MPI_Finalize();

    return 0;
}
