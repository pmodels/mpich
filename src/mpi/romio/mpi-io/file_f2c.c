/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_f2c = PMPI_File_f2c
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_f2c MPI_File_f2c
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_f2c as PMPI_File_f2c
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
MPI_File MPI_File_f2c(MPI_Fint fh) __attribute__ ((weak, alias("PMPI_File_f2c")));
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif
#include "adio_extern.h"

MPI_File MPI_File_f2c(MPI_Fint fh)
{
    QMPI_Context context;
    QMPI_File_f2c_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_f2c(context, 0, fh);

    fn_ptr = (QMPI_File_f2c_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_F2C_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_F2C_T], fh);
}

/*@
    MPI_File_f2c - Translates a Fortran file handle to a C file handle

Input Parameters:
. fh - Fortran file handle (integer)

Return Value:
  C file handle (handle)
@*/
MPI_File QMPI_File_f2c(QMPI_Context context, int tool_id, MPI_Fint fh)
{
    return MPIO_File_f2c(fh);
}
