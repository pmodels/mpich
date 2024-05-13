/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
		int rank;

		MPI_Comm_rank(fd->comm, &rank);
		MPI_Barrier(fd->comm);

		if (rank == fd->hints->ranklist[0])
			rc = dfs_get_size(cont->dfs, cont->obj, &fsize);

		MPI_Bcast(&rc, 1, MPI_INT, fd->hints->ranklist[0], fd->comm);
		if (rc != 0) {
			*error_code = ADIOI_DAOS_err(myname, cont->obj_name, __LINE__, rc);
			break;
		}

		MPI_Bcast(&fsize, 1, MPI_UINT64_T, fd->hints->ranklist[0], fd->comm);
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
