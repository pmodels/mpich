/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adio.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

void ADIOI_GEN_Delete(const char *filename, int *error_code)
{
    int err;
    static char myname[] = "ADIOI_GEN_DELETE";

    err = unlink(filename);
    if (err == -1) {
        *error_code = ADIOI_Err_create_code(myname, filename, errno);
    } else
        *error_code = MPI_SUCCESS;
}
