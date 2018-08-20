/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 * Copyright (C) 1997 University of Chicago.
 * See COPYRIGHT notice in top-level directory.
 *
 * Copyright (C) 2018 Intel Corporation
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

#if 0
    {
        char uuid_str[37];
        uuid_unparse(cont->uuid, uuid_str);
        fprintf(stderr, "File Close %s %s\n", fd->filename, uuid_str);
    }
#endif
    if (cont->amode == DAOS_COO_RW) {
        daos_epoch_t max_epoch;

        MPI_Allreduce(&cont->epoch, &max_epoch, 1, MPI_UINT64_T, MPI_MAX,
                      fd->comm);
        cont->epoch = max_epoch;
    } else
        MPI_Barrier(fd->comm);

    MPI_Comm_rank(fd->comm, &rank);

    if (rank == 0) {
        daos_epoch_t epoch;

        rc = dfs_release(cont->obj);
        if (rc) {
            PRINT_MSG(stderr, "dfs_release() failed (%d)\n", rc);
            goto bcast_rc;
        }

        if (cont->amode == DAOS_COO_RW) {
            rc = dfs_get_epoch(cont->dfs, &epoch);
            if (rc) {
                PRINT_MSG(stderr, "dfs_get_epoch() failed (%d)\n", rc);
                goto bcast_rc;
            }

            if (epoch > cont->epoch)
                cont->epoch = epoch;

            rc = daos_epoch_commit(cont->coh, cont->epoch, NULL, NULL);
            if (rc) {
                PRINT_MSG(stderr, "daos_epoch_commit() failed (%d)\n", rc);
                goto bcast_rc;
            }
        }

        rc = dfs_umount(cont->dfs, false);
        if (rc) {
            PRINT_MSG(stderr, "dfs_umount() failed (%d)\n", rc);
            goto bcast_rc;
        }
    }

bcast_rc:
    MPI_Bcast(&rc, 1, MPI_INT, 0, fd->comm);
    if (rc != 0) {
        PRINT_MSG(stderr, "Failed to close file (%d)\n", rc);
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(rc),
                                           "Epoch Commit failed", 0);
        return;
    }

    /* array is closed on rank 0 in dfs_release() */
    if (rank != 0) {
        rc = daos_array_close(cont->oh, NULL);
        if (rc != 0) {
            PRINT_MSG(stderr, "daos_array_close() failed (%d)\n", rc);
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               ADIOI_DAOS_error_convert(rc),
                                               "Container Close failed", 0);
            return;
        }
    }

    rc = daos_cont_close(cont->coh, NULL);
    if (rc != 0) {
        PRINT_MSG(stderr, "daos_cont_close() failed (%d)\n", rc);
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(rc),
                                           "Container Close failed", 0);
        return;
    }

    ADIOI_Free(fd->fs_ptr);
    fd->fs_ptr = NULL;

    *error_code = MPI_SUCCESS;
}
/*
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */
