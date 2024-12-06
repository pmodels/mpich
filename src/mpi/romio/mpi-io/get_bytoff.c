/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"
#include "adioi.h"      /* ADIOI_Get_byte_offset() prototype */

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_get_byte_offset = PMPI_File_get_byte_offset
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_get_byte_offset MPI_File_get_byte_offset
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_get_byte_offset as PMPI_File_get_byte_offset
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_get_byte_offset(MPI_File fh, MPI_Offset offset, MPI_Offset * disp)
    __attribute__ ((weak, alias("PMPI_File_get_byte_offset")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_get_byte_offset - Returns the absolute byte position in
                the file corresponding to "offset" etypes relative to
                the current view

Input Parameters:
. fh - file handle (handle)
. offset - offset (nonnegative integer)

Output Parameters:
. disp - absolute byte position of offset (nonnegative integer)

.N fortran
@*/
int MPI_File_get_byte_offset(MPI_File fh, MPI_Offset offset, MPI_Offset * disp)
{
    int error_code;

    error_code = MPIR_File_get_byte_offset_impl(fh, offset, disp);
    if (error_code) {
        goto fn_fail;
    }

  fn_exit:
    return error_code;
  fn_fail:
    error_code = MPIO_Err_return_file(fh, error_code);
    goto fn_exit;
}
