/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_get_size = PMPI_File_get_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_get_size MPI_File_get_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_get_size as PMPI_File_get_size
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_get_size(MPI_File fh, MPI_Offset * size)
    __attribute__ ((weak, alias("PMPI_File_get_size")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_get_size - Returns the file size

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. size - size of the file in bytes (nonnegative integer)

.N fortran
@*/
int MPI_File_get_size(MPI_File fh, MPI_Offset * size)
{
    int error_code;

    error_code = MPIR_File_get_size_impl(fh, size);
    if (error_code) {
        goto fn_fail;
    }

  fn_exit:
    return error_code;
  fn_fail:
    error_code = MPIO_Err_return_file(fh, error_code);
    goto fn_exit;
}
