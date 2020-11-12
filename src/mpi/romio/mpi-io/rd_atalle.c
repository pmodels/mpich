/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

/* -- Begin Profiling Symbol Block */
#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_read_at_all_end = PMPI_File_read_at_all_end
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_read_at_all_end MPI_File_read_at_all_end
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_read_at_all_end as PMPI_File_read_at_all_end
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_read_at_all_end(MPI_File fh, void *buf, MPI_Status * status)
    __attribute__ ((weak, alias("PMPI_File_read_at_all_end")));
#endif
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

int MPI_File_read_at_all_end(MPI_File fh, void *buf, MPI_Status * status)
{
    QMPI_Context context;
    QMPI_File_read_at_all_end_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_read_at_all_end(context, 0, fh, buf, status);

    fn_ptr = (QMPI_File_read_at_all_end_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_READ_AT_ALL_END_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_READ_AT_ALL_END_T], fh, buf,
                      status);
}

/*@
    MPI_File_read_at_all_end - Complete a split collective read using
    explicit offset

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. status - status object (Status)

.N fortran
@*/
int QMPI_File_read_at_all_end(QMPI_Context context, int tool_id, MPI_File fh, void *buf,
                              MPI_Status * status)
{
    int error_code;
    static char myname[] = "MPI_FILE_READ_AT_ALL_END";


    error_code = MPIOI_File_read_all_end(fh, buf, myname, status);

    return error_code;
}
