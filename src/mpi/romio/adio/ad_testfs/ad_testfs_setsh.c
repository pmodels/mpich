/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ad_testfs.h"
#include "adioi.h"

void ADIOI_TESTFS_Set_shared_fp(ADIO_File fd, ADIO_Offset offset, int *error_code)
{
    int myrank, nprocs;

    *error_code = MPI_SUCCESS;

    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);
    FPRINTF(stdout, "[%d/%d] ADIOI_TESTFS_Set_shared_fp called on %s\n",
            myrank, nprocs, fd->filename);
}
