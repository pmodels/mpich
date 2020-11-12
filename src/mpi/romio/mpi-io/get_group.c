/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

/* -- Begin Profiling Symbol Block */
#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_get_group = PMPI_File_get_group
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_get_group MPI_File_get_group
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_get_group as PMPI_File_get_group
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_get_group(MPI_File fh, MPI_Group * group)
    __attribute__ ((weak, alias("PMPI_File_get_group")));
#endif
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

int MPI_File_get_group(MPI_File fh, MPI_Group * group)
{
    QMPI_Context context;
    QMPI_File_get_group_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_get_group(context, 0, fh, group);

    fn_ptr = (QMPI_File_get_group_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_GET_GROUP_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_GET_GROUP_T], fh, group);
}

/*@
    MPI_File_get_group - Returns the group of processes that
                         opened the file

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. group - group that opened the file (handle)

.N fortran
@*/
int QMPI_File_get_group(QMPI_Context context, int tool_id, MPI_File fh, MPI_Group * group)
{
    int error_code;
    ADIO_File adio_fh;
    static char myname[] = "MPI_FILE_GET_GROUP";

    ROMIO_THREAD_CS_ENTER();

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, myname, error_code);
    /* --END ERROR HANDLING-- */


    /* note: this will return the group of processes that called open, but
     * with deferred open this might not be the group of processes that
     * actually opened the file from the file system's perspective
     */
    error_code = MPI_Comm_group(adio_fh->comm, group);

  fn_exit:
    ROMIO_THREAD_CS_EXIT();
    return error_code;
}
