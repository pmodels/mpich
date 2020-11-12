/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

/* -- Begin Profiling Symbol Block */
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
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

int MPI_File_get_amode(MPI_File fh, int *amode)
{
    QMPI_Context context;
    QMPI_File_get_amode_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_get_amode(context, 0, fh, amode);

    fn_ptr = (QMPI_File_get_amode_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_GET_AMODE_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_GET_AMODE_T], fh, amode);
}

/*@
    MPI_File_get_amode - Returns the file access mode

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. amode - access mode (integer)

.N fortran
@*/
int QMPI_File_get_amode(QMPI_Context context, int tool_id, MPI_File fh, int *amode)
{
    int error_code = MPI_SUCCESS;
    static char myname[] = "MPI_FILE_GET_AMODE";
    ADIO_File adio_fh;

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, myname, error_code);
    /* --END ERROR HANDLING-- */

    *amode = adio_fh->orig_access_mode;

  fn_exit:
    return error_code;
}
