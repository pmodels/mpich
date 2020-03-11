/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2018-2020 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "ad_daos.h"

void ADIOI_DAOS_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t * fcntl_struct, int *error_code)
{
    int rc;
    struct ADIO_DAOS_cont *cont = fd->fs_ptr;
    static char myname[] = "ADIOI_DAOS_FCNTL";

    switch (flag) {
        case ADIO_FCNTL_GET_FSIZE:
            {
                daos_size_t fsize;

                rc = dfs_get_size(cont->dfs, cont->obj, &fsize);
                if (rc != 0) {
                    *error_code = ADIOI_DAOS_err(myname, cont->obj_name, __LINE__, rc);
                    break;
                }
                *error_code = MPI_SUCCESS;
                fcntl_struct->fsize = (ADIO_Offset) fsize;
                break;
            }
        case ADIO_FCNTL_SET_DISKSPACE:
        case ADIO_FCNTL_SET_ATOMICITY:
        default:
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               MPI_ERR_ARG, "**flag", "**flag %d", flag);
    }
}
