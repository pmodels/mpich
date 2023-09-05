/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_get_amode = PMPI_File_get_amode
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_get_amode MPI_File_get_amode
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_get_amode as PMPI_File_get_amode
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_get_amode(MPI_File fh, int *amode)
    __attribute__ ((weak, alias("PMPI_File_get_amode")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_get_amode - Returns the file access mode

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. amode - access mode (integer)

.N fortran
@*/
int MPI_File_get_amode(MPI_File fh, int *amode)
{
    int error_code;

    error_code = MPIR_File_get_amode_impl(fh, amode);
    if (error_code) {
        goto fn_fail;
    }

  fn_exit:
    return error_code;
  fn_fail:
    error_code = MPIO_Err_return_file(fh, error_code);
    goto fn_exit;
}
