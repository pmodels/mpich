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

void ADIOI_DAOS_Close(ADIO_File fd, int *error_code)
{
    int rank;
    struct ADIO_DAOS_cont *cont = (struct ADIO_DAOS_cont *)fd->fs_ptr;
    static char myname[] = "ADIOI_DAOS_CLOSE";
    int rc;

    MPI_Barrier(fd->comm);
    MPI_Comm_rank(fd->comm, &rank);

    if (rank == 0) {
        /* release the dfs object handle for the file. */
        rc = dfs_release(cont->obj);
        if (rc)
            PRINT_MSG(stderr, "dfs_release() failed (%d)\n", rc);
    }

    /* bcast the return code to the other ranks */
    MPI_Bcast(&rc, 1, MPI_INT, 0, fd->comm);
    if (rc != 0) {
        *error_code = ADIOI_DAOS_err(myname, cont->obj_name, __LINE__, rc);
        return;
    }

    /* array is closed on rank 0 in dfs_release(), close it on the other ranks */
    if (rank != 0) {
        rc = daos_array_close(cont->oh, NULL);
        if (rc != 0) {
            PRINT_MSG(stderr, "daos_array_close() failed (%d)\n", rc);
            *error_code = ADIOI_DAOS_err(myname, cont->obj_name, __LINE__, rc);
            return;
        }
    }

    /* decrement ref count on the container and pool in the hashtable. */
    adio_daos_coh_release(cont->c);
    cont->c = NULL;
    adio_daos_poh_release(cont->p);
    cont->p = NULL;

    ADIOI_Free(cont->obj_name);
    ADIOI_Free(cont->cont_name);
    ADIOI_Free(fd->fs_ptr);
    fd->fs_ptr = NULL;

    *error_code = MPI_SUCCESS;
}
/*
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */
