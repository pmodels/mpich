/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "mpitest.h"

#define DATA_SIZE 324*4
#define PAD 256
#define HEADER 144
#define BLK_COUNT 3

static void handle_error(int errcode, char *str)
{
    char msg[MPI_MAX_ERROR_STRING];
    int resultlen;
    MPI_Error_string(errcode, msg, &resultlen);
    fprintf(stderr, "%s: %s\n", str, msg);
    MPI_Abort(MPI_COMM_WORLD, 1);
}

#define CHECK(fn) { int errcode; errcode = (fn); if (errcode != MPI_SUCCESS) handle_error(errcode, #fn); }

int main(int argc, char **argv)
{

    MPI_File fh;
    MPI_Datatype file_type, mem_type;
    int *data = NULL;
    int *verify = NULL;
    int data_size = DATA_SIZE;
    int i, j, k, errs = 0;
    MPI_Aint disp[BLK_COUNT];
    char *filename = "unnamed.dat";

    MTest_Init(&argc, &argv);
    disp[0] = (MPI_Aint) (PAD);
    disp[1] = (MPI_Aint) (data_size * 1 + PAD);
    disp[2] = (MPI_Aint) (data_size * 2 + PAD);

    data = malloc(data_size);
    verify = malloc(data_size * BLK_COUNT + HEADER + PAD);
    for (i = 0; i < data_size / sizeof(int); i++)
        data[i] = i;

    MPI_Type_create_hindexed_block(BLK_COUNT, data_size, disp, MPI_BYTE, &file_type);
    MPI_Type_commit(&file_type);

    MPI_Type_create_hvector(BLK_COUNT, data_size, 0, MPI_BYTE, &mem_type);
    MPI_Type_commit(&mem_type);

    if (1 < argc)
        filename = argv[1];

    CHECK(MPI_File_open(MPI_COMM_WORLD, filename,
                        MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_DELETE_ON_CLOSE,
                        MPI_INFO_NULL, &fh) != 0);

    CHECK(MPI_File_set_view(fh, HEADER, MPI_BYTE, file_type, "native", MPI_INFO_NULL));

    /* write everything */
    CHECK(MPI_File_write_at_all(fh, 0, data, 1, mem_type, MPI_STATUS_IGNORE));
    /* verify */
    CHECK(MPI_File_set_view(fh, 0, MPI_BYTE, MPI_BYTE, "native", MPI_INFO_NULL));
    CHECK(MPI_File_read_at_all(fh, 0,
                               verify, (HEADER + PAD + BLK_COUNT * DATA_SIZE) / sizeof(int),
                               MPI_INT, MPI_STATUS_IGNORE));

    /* header and block padding should have no data */
    for (i = 0; i < (HEADER + PAD) / sizeof(int); i++) {
        if (verify[i] != 0) {
            errs++;
            fprintf(stderr, "expected 0, read %d\n", verify[i]);
        }
    }
    /* blocks are replicated */
    for (j = 0; j < BLK_COUNT; j++) {
        for (k = 0; k < (DATA_SIZE / sizeof(int)); k++) {
            if (verify[(HEADER + PAD) / sizeof(int) + k + j * (DATA_SIZE / sizeof(int))] != data[k]) {
                errs++;
                fprintf(stderr, "expcted %d, read %d\n", data[k],
                        verify[(HEADER + PAD) / sizeof(int) + k + j * (DATA_SIZE / sizeof(int))]);
            }
            i++;
        }
    }

    MPI_File_close(&fh);

    MPI_Type_free(&mem_type);
    MPI_Type_free(&file_type);

    MTest_Finalize(errs);

    free(data);
    free(verify);
    return MTestReturnValue(errs);
}
