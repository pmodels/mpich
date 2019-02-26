/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * GOVERNMENT LICENSE RIGHTS-OPEN SOURCE SOFTWARE
 * The Government's rights to use, modify, reproduce, release, perform, display,
 * or disclose this software are subject to the terms of the Apache License as
 * provided in Contract No. 8F-30005.
 * Any reproduction of computer software, computer software documentation, or
 * portions thereof marked with this legend must also reproduce the markings.
 */

#include "ad_daos.h"

void ADIOI_DAOS_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct,
                      int *error_code)
{
    int rc;
    struct ADIO_DAOS_cont *cont = fd->fs_ptr;
    static char myname[] = "ADIOI_DAOS_FCNTL";

    switch(flag) {
    case ADIO_FCNTL_GET_FSIZE:
    {
        daos_size_t fsize;

	rc = daos_array_get_size(cont->oh, DAOS_TX_NONE, &fsize, NULL);
	if (rc != 0) {
            *error_code = ADIOI_DAOS_err(myname, cont->obj_name, __LINE__, rc);
            break;
	}
        *error_code = MPI_SUCCESS;
        fcntl_struct->fsize = (ADIO_Offset)fsize;
        break;
    }
    case ADIO_FCNTL_SET_DISKSPACE:
    case ADIO_FCNTL_SET_ATOMICITY:
    default:
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   MPI_ERR_ARG,
					   "**flag", "**flag %d", flag);
    }
}
