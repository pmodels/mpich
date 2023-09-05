/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_set_info = PMPI_File_set_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_set_info MPI_File_set_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_set_info as PMPI_File_set_info
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_set_info(MPI_File fh, MPI_Info info)
    __attribute__ ((weak, alias("PMPI_File_set_info")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_set_info - Sets new values for the hints associated with a file

Input Parameters:
. fh - file handle (handle)
. info - info object (handle)

.N fortran
@*/
int MPI_File_set_info(MPI_File fh, MPI_Info info)
{
    int error_code;
    ROMIO_THREAD_CS_ENTER();

    error_code = MPIR_File_set_info_impl(fh, info);
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
