/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adio.h"

/* Generic implementation of ReadComplete/WriteComplete simply sets the
 * bytes field in the status structure and frees the request.
 *
 * Same function is used for both reads and writes.
 */
void ADIOI_FAKE_IOComplete(ADIO_Request * request, ADIO_Status * status, int *error_code)
{
    *error_code = MPI_SUCCESS;
    return;
}
