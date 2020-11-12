/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

/* -- Begin Profiling Symbol Block */
#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_get_position_shared = PMPI_File_get_position_shared
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_get_position_shared MPI_File_get_position_shared
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_get_position_shared as PMPI_File_get_position_shared
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_get_position_shared(MPI_File fh, MPI_Offset * offset)
    __attribute__ ((weak, alias("PMPI_File_get_position_shared")));
#endif
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

int MPI_File_get_position_shared(MPI_File fh, MPI_Offset * offset)
{
    QMPI_Context context;
    QMPI_File_get_position_shared_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_get_position_shared(context, 0, fh, offset);

    fn_ptr =
        (QMPI_File_get_position_shared_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_GET_POSITION_SHARED_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_GET_POSITION_SHARED_T], fh,
                      offset);
}

/*@
    MPI_File_get_position_shared - Returns the current position of the
               shared file pointer in etype units relative to the current view

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. offset - offset of shared file pointer (nonnegative integer)

.N fortran
@*/
int QMPI_File_get_position_shared(QMPI_Context context, int tool_id, MPI_File fh,
                                  MPI_Offset * offset)
{
    int error_code;
    ADIO_File adio_fh;
    static char myname[] = "MPI_FILE_GET_POSITION_SHARED";

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, myname, error_code);
    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, myname, error_code);
    MPIO_CHECK_FS_SUPPORTS_SHARED(adio_fh, myname, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_TEST_DEFERRED(adio_fh, myname, &error_code);

    ADIO_Get_shared_fp(adio_fh, 0, offset, &error_code);
    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
        error_code = MPIO_Err_return_file(adio_fh, error_code);
    /* --END ERROR HANDLING-- */

  fn_exit:
    return error_code;
}
