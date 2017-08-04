/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#include "ad_daos.h"

void ADIOI_DAOS_Resize(ADIO_File fd, ADIO_Offset size, int *error_code)
{
    int ret, rank;
    struct ADIO_DAOS_cont *cont = fd->fs_ptr;
    static char myname[] = "ADIOI_DAOS_RESIZE";

    *error_code = MPI_SUCCESS;

    MPI_Comm_rank(fd->comm, &rank);

    if (rank == fd->hints->ranklist[0]) {
	ret = daos_array_set_size(cont->oh, cont->epoch, size, NULL);
	MPI_Bcast(&ret, 1, MPI_INT, fd->hints->ranklist[0], fd->comm);
    } else  {
	MPI_Bcast(&ret, 1, MPI_INT, fd->hints->ranklist[0], fd->comm);
    }
    /* --BEGIN ERROR HANDLING-- */
    if (ret != 0) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   ADIOI_DAOS_error_convert(ret),
					   "Error in daos_array_set_size", 0);
	return;
    }
    /* --END ERROR HANDLING-- */
}
