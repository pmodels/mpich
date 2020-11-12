/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpioimpl.h"

/* -- Begin Profiling Symbol Block */
#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_get_type_extent = PMPI_File_get_type_extent
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_get_type_extent MPI_File_get_type_extent
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_get_type_extent as PMPI_File_get_type_extent
/* end of weak pragmas */
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_File_get_type_extent(MPI_File fh, MPI_Datatype datatype, MPI_Aint * extent)
    __attribute__ ((weak, alias("PMPI_File_get_type_extent")));
#endif
/* -- End Profiling Symbol Block */

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

int MPI_File_get_type_extent(MPI_File fh, MPI_Datatype datatype, MPI_Aint * extent)
{
    QMPI_Context context;
    QMPI_File_get_type_extent_t *fn_ptr;

    context.storage_stack = NULL;

    if (MPIR_QMPI_num_tools == 0)
        return QMPI_File_get_type_extent(context, 0, fh, datatype, extent);

    fn_ptr = (QMPI_File_get_type_extent_t *) MPIR_QMPI_first_fn_ptrs[MPI_FILE_GET_TYPE_EXTENT_T];

    return (*fn_ptr) (context, MPIR_QMPI_first_tool_ids[MPI_FILE_GET_TYPE_EXTENT_T], fh, datatype,
                      extent);
}

/*@
    MPI_File_get_type_extent - Returns the extent of datatype in the file

Input Parameters:
. fh - file handle (handle)
. datatype - datatype (handle)

Output Parameters:
. extent - extent of the datatype (nonnegative integer)

.N fortran
@*/
int QMPI_File_get_type_extent(QMPI_Context context, int tool_id, MPI_File fh, MPI_Datatype datatype,
                              MPI_Aint * extent)
{
    int error_code;
    ADIO_File adio_fh;
    static char myname[] = "MPI_FILE_GET_TYPE_EXTENT";

    adio_fh = MPIO_File_resolve(fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(adio_fh, myname, error_code);
    MPIO_CHECK_DATATYPE(adio_fh, datatype, myname, error_code);
    /* --END ERROR HANDLING-- */

    /* FIXME: handle other file data representations */

    error_code = MPI_Type_extent(datatype, extent);

  fn_exit:
    return error_code;
}
