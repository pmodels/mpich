/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Unpack_external */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Unpack_external = PMPI_Unpack_external
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Unpack_external  MPI_Unpack_external
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Unpack_external as PMPI_Unpack_external
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Unpack_external(const char datarep[], const void *inbuf, MPI_Aint insize,
                        MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype)
                        __attribute__((weak,alias("PMPI_Unpack_external")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Unpack_external
#define MPI_Unpack_external PMPI_Unpack_external

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Unpack_external

/*@
   MPI_Unpack_external - Unpack a buffer (packed with MPI_Pack_external)
   according to a datatype into contiguous memory

Input Parameters:
+ datarep - data representation (string)
. inbuf - input buffer start (choice)
. insize - input buffer size, in bytes (address integer)
. outcount - number of output data items (integer)
. datatype - datatype of output data item (handle)

Input/Output Parameters:
. position - current position in buffer, in bytes (address integer)

Output Parameters:
. outbuf - output buffer start (choice)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Unpack_external(const char datarep[],
			const void *inbuf,
			MPI_Aint insize,
			MPI_Aint *position,
			void *outbuf,
			int outcount,
			MPI_Datatype datatype)
{
    static const char FCNAME[] = "MPI_Unpack_external";
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint first, last;
    MPID_Segment *segp;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_UNPACK_EXTERNAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_UNPACK_EXTERNAL);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    if (insize > 0) {
		MPIR_ERRTEST_ARGNULL(inbuf, "input buffer", mpi_errno);
	    }
	    /* NOTE: outbuf could be MPI_BOTTOM; don't test for NULL */
	    MPIR_ERRTEST_COUNT(insize, mpi_errno);
	    MPIR_ERRTEST_COUNT(outcount, mpi_errno);

	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

	    if (datatype != MPI_DATATYPE_NULL && HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
		MPID_Datatype *datatype_ptr = NULL;

		MPID_Datatype_get_ptr(datatype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
	    }
		
	    /* If datatye_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    if (insize == 0) {
	goto fn_exit;
    }

    segp = MPID_Segment_alloc();
    MPIR_ERR_CHKANDJUMP1((segp == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
    mpi_errno = MPID_Segment_init(outbuf, outcount, datatype, segp, 1);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* NOTE: buffer values and positions in MPI_Unpack_external are used very
     * differently from use in MPID_Segment_unpack_external...
     */
    first = 0;
    last  = SEGMENT_IGNORE_LAST;

    /* Ensure that pointer increment fits in a pointer */
    MPIU_Ensure_Aint_fits_in_pointer((MPIU_VOID_PTR_CAST_TO_MPI_AINT inbuf) + *position);

    MPID_Segment_unpack_external32(segp,
				   first,
				   &last,
				   (void *) ((char *) inbuf + *position));

    *position += last;

    MPID_Segment_free(segp);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_UNPACK_EXTERNAL);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_unpack_external",
	    "**mpi_unpack_external %s %p %d %p %p %d %D", datarep, inbuf, insize, position, outbuf, outcount, datatype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
