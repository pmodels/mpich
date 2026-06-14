/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ad_nfs.h"

int ADIOI_NFS_ReadDone(ADIO_Request * request, ADIO_Status * status, int *error_code)
{
    *error_code = MPI_SUCCESS;
    return 1;
}

int ADIOI_NFS_WriteDone(ADIO_Request * request, ADIO_Status * status, int *error_code)
{
    return ADIOI_NFS_ReadDone(request, status, error_code);
}
