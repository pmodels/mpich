/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_seek_shared = PMPI_File_seek_shared
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_seek_shared MPI_File_seek_shared
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_seek_shared as PMPI_File_seek_shared
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_seek_shared(MPI_File fh, MPI_Offset offset, int whence)
    __attribute__ ((weak, alias("PMPI_File_seek_shared")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_seek_shared - Updates the shared file pointer

Input Parameters:
. fh - file handle (handle)
. offset - file offset (integer)
. whence - update mode (state)

.N fortran
@*/
int MPI_File_seek_shared(MPI_File fh, MPI_Offset offset, int whence)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_File_seek_shared_impl(fh, offset, whence);
    if (error_code) {
        goto fn_fail;
    }

  fn_exit:
    ROMIO_THREAD_CS_EXIT();
    return error_code;
  fn_fail:
    error_code = MPIO_Err_return_file(fh, error_code);
    goto fn_exit;
}
