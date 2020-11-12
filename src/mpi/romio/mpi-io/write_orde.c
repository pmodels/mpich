/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

/* -- Begin Profiling Symbol Block */
#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_write_ordered_end = PMPI_File_write_ordered_end
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_write_ordered_end MPI_File_write_ordered_end
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_write_ordered_end as PMPI_File_write_ordered_end
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_write_ordered_end(MPI_File fh, const void *buf, MPI_Status * status)
    __attribute__ ((weak, alias("PMPI_File_write_ordered_end")));
#endif
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

int MPI_File_write_ordered_end(MPI_File fh, ROMIO_CONST void *buf, MPI_Status * status)
{
    QMPI_Context context;
    QMPI_File_write_ordered_end_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_write_ordered_end(context, 0, fh, buf, status);

    fn_ptr =
        (QMPI_File_write_ordered_end_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_WRITE_ORDERED_END_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_WRITE_ORDERED_END_T], fh, buf,
                      status);
}

/*@
    MPI_File_write_ordered_end - Complete a split collective write using shared file pointer

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. status - status object (Status)

.N fortran
@*/
int QMPI_File_write_ordered_end(QMPI_Context context, int tool_id, MPI_File fh,
                                ROMIO_CONST void *buf, MPI_Status * status)
{
    int error_code;
    static char myname[] = "MPI_FILE_WRITE_ORDERED_END";
    ADIO_File adio_fh;

    MPL_UNREFERENCED_ARG(buf);

    ROMIO_THREAD_CS_ENTER();

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, myname, error_code);

    if (!(adio_fh->split_coll_count)) {
        error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                          myname, __LINE__, MPI_ERR_IO, "**iosplitcollnone", 0);
        error_code = MPIO_Err_return_file(adio_fh, error_code);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

#ifdef HAVE_STATUS_SET_BYTES
    if (status != MPI_STATUS_IGNORE)
        *status = adio_fh->split_status;
#endif
    adio_fh->split_coll_count = 0;


  fn_exit:
    ROMIO_THREAD_CS_EXIT();
    return MPI_SUCCESS;
}
