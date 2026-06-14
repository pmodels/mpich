/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adio.h"

/* Generic implementation of ReadDone/WriteDone simply sets the
 * bytes field in the status structure and frees the request.
 *
 * Same function is used for both reads and writes.
 */
int ADIOI_FAKE_IODone(ADIO_Request * request, ADIO_Status * status, int *error_code)
{
    /* should not ever get called now */
    return 1;
}
