/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

/* Test MPI_File_set_view with zero-len filetype on some of the processes */

/* The test is contributed Eric Chamberland. Reference issue #6222 */

int main(int argc, char *argv[])
{
    int errs = 0;
    MTest_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size != 3) {
        printf("Please run with 3 processes.\n");
        MPI_Finalize();
        return 1;
    }

    int displacement[3];
    int *buffer = 0;

    int lTailleBuf = 0;
    if (rank == 0) {
        lTailleBuf = 3;
        displacement[0] = 0;
        displacement[1] = 4;
        displacement[2] = 5;
        buffer = malloc(sizeof(int) * lTailleBuf);
        for (int i = 0; i < lTailleBuf; i++) {
            buffer[i] = 10 * (i + 1);
        }
    }
    if (rank == 1) {
        lTailleBuf = 3;
        displacement[0] = 1;
        displacement[1] = 2;
        displacement[2] = 3;

        buffer = malloc(sizeof(int) * lTailleBuf);
        for (int i = 0; i < lTailleBuf; i++) {
            buffer[i] = -(i + 1);
        }
    }
    if (rank == 2) {
        // A rank without any "element" is ok normally:
        lTailleBuf = 0;
    }

    MPI_File lFile;
    MPI_File_open(MPI_COMM_WORLD, "testfile", MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL,
                  &lFile);

    MPI_Datatype lTypeIndexIntWithExtent, lTypeIndexIntWithoutExtent;

    MPI_Type_create_indexed_block(lTailleBuf, 1, displacement, MPI_INT,
                                  &lTypeIndexIntWithoutExtent);
    MPI_Type_commit(&lTypeIndexIntWithoutExtent);

    // Here we compute the total number of int to write to resize the type:
    int lTailleGlobale = 0;
    MPI_Allreduce(&lTailleBuf, &lTailleGlobale, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    //We now modify the extent of the type "type_without_extent"
    MPI_Type_create_resized(lTypeIndexIntWithoutExtent, 0, lTailleGlobale * sizeof(int),
                            &lTypeIndexIntWithExtent);
    MPI_Type_commit(&lTypeIndexIntWithExtent);

    MPI_File_set_view(lFile, 0, MPI_INT, lTypeIndexIntWithExtent, "native", MPI_INFO_NULL);

    for (int i = 0; i < 2; ++i) {
        MPI_File_write_all(lFile, buffer, lTailleBuf, MPI_INT, MPI_STATUS_IGNORE);
        MPI_Offset lOffset, lSharedOffset;
        MPI_File_get_position(lFile, &lOffset);
        MPI_File_get_position_shared(lFile, &lSharedOffset);
        MTestPrintfMsg(1, "[%d] Offset after write : %d int: Local: %lld Shared: %lld \n",
                       rank, lTailleBuf, lOffset, lSharedOffset);
    }

    MPI_File_close(&lFile);

    MPI_Type_free(&lTypeIndexIntWithExtent);
    MPI_Type_free(&lTypeIndexIntWithoutExtent);

    if (buffer) {
        free(buffer);
    }

    if (rank == 0) {
        MPI_File_delete((char *) "testfile", MPI_INFO_NULL);
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
