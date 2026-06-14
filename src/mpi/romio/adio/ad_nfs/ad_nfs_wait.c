/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ad_nfs.h"

void ADIOI_NFS_ReadComplete(ADIO_Request * request, ADIO_Status * status, int *error_code)
{
    return;
}


void ADIOI_NFS_WriteComplete(ADIO_Request * request, ADIO_Status * status, int *error_code)
{
    ADIOI_NFS_ReadComplete(request, status, error_code);
}
