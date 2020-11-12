/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"
#include "adioi.h"      /* ADIOI_Get_byte_offset() prototype */

/* -- Begin Profiling Symbol Block */
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
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

int MPI_File_get_byte_offset(MPI_File fh, MPI_Offset offset, MPI_Offset * disp)
{
    QMPI_Context context;
    QMPI_File_get_byte_offset_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_get_byte_offset(context, 0, fh, offset, disp);

    fn_ptr = (QMPI_File_get_byte_offset_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_GET_BYTE_OFFSET_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_GET_BYTE_OFFSET_T], fh, offset,
                      disp);
}

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
int QMPI_File_get_byte_offset(QMPI_Context context, int tool_id, MPI_File fh, MPI_Offset offset,
                              MPI_Offset * disp)
{
    int error_code;
    ADIO_File adio_fh;
    static char myname[] = "MPI_FILE_GET_BYTE_OFFSET";

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, myname, error_code);

    if (offset < 0) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          myname, __LINE__, MPI_ERR_ARG, "**iobadoffset", 0);
        error_code = MPIO_Err_return_file(adio_fh, error_code);
        goto fn_exit;
    }

    MPIO_CHECK_NOT_SEQUENTIAL_MODE(adio_fh, myname, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_Get_byte_offset(adio_fh, offset, disp);

  fn_exit:

    return MPI_SUCCESS;
}
