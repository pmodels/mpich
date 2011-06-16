/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Pack */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Pack = PMPI_Pack
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Pack  MPI_Pack
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Pack as PMPI_Pack
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Pack
#define MPI_Pack PMPI_Pack

#undef FUNCNAME
#define FUNCNAME MPIR_Pack_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Pack_impl(void *inbuf,
                   int incount,
                   MPI_Datatype datatype,
                   void *outbuf,
                   int outcount,
                   int *position)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint first, last;
    MPID_Segment *segp;
    int contig;
    MPI_Aint dt_true_lb;
    MPI_Aint data_sz;

    if (incount == 0) {
	goto fn_exit;
    }

    /* Handle contig case quickly */
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
        contig     = TRUE;
        dt_true_lb = 0;
        data_sz    = incount * MPID_Datatype_get_basic_size(datatype);
    } else {
        MPID_Datatype *dt_ptr;
        MPID_Datatype_get_ptr(datatype, dt_ptr);
	contig     = dt_ptr->is_contig;
        dt_true_lb = dt_ptr->true_lb;
        data_sz    = incount * dt_ptr->size;
    }

    if (contig) {
        MPIU_Memcpy((char *) outbuf + *position, (char *)inbuf + dt_true_lb, data_sz);
        *position = (int)((MPI_Aint)*position + data_sz);
        goto fn_exit;
    }
    

    /* non-contig case */
    
    /* TODO: CHECK RETURN VALUES?? */
    /* TODO: SHOULD THIS ALL BE IN A MPID_PACK??? */
    segp = MPID_Segment_alloc();
    MPIU_ERR_CHKANDJUMP1(segp == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment");
    
    mpi_errno = MPID_Segment_init(inbuf, incount, datatype, segp, 0);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* NOTE: the use of buffer values and positions in MPI_Pack and in
     * MPID_Segment_pack are quite different.  See code or docs or something.
     */
    first = 0;
    last  = SEGMENT_IGNORE_LAST;

    /* Ensure that pointer increment fits in a pointer */
    MPID_Ensure_Aint_fits_in_pointer((MPI_VOID_PTR_CAST_TO_MPI_AINT outbuf) +
				     (MPI_Aint) *position);

    MPID_Segment_pack(segp,
		      first,
		      &last,
		      (void *) ((char *) outbuf + *position));

    /* Ensure that calculation fits into an int datatype. */
    MPID_Ensure_Aint_fits_in_int((MPI_Aint)*position + last);

    *position = (int)((MPI_Aint)*position + last);

    MPID_Segment_free(segp);
        
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Pack
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
    MPI_Pack - Packs a datatype into contiguous memory

   Input Parameters:
+  inbuf - input buffer start (choice)
.  incount - number of input data items (non-negative integer)
.  datatype - datatype of each input data item (handle)
.  outcount - output buffer size, in bytes (non-negative integer)
-  comm - communicator for packed message (handle)

  Output Parameter:
.  outbuf - output buffer start (choice)

  Input/Output Parameter:
.  position - current position in buffer, in bytes (integer)

  Notes (from the specifications):

  The input value of position is the first location in the output buffer to be
  used for packing.  position is incremented by the size of the packed message,
  and the output value of position is the first location in the output buffer
  following the locations occupied by the packed message.  The comm argument is
  the communicator that will be subsequently used for sending the packed
  message.


.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_OTHER
@*/
int MPI_Pack(void *inbuf,
	     int incount,
	     MPI_Datatype datatype,
	     void *outbuf,
	     int outcount,
	     int *position,
	     MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_PACK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_PACK);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(incount,mpi_errno);
	    MPIR_ERRTEST_COUNT(outcount,mpi_errno);
	    /* NOTE: inbuf could be null (MPI_BOTTOM) */
	    if (incount > 0) {
		MPIR_ERRTEST_ARGNULL(outbuf, "output buffer", mpi_errno);
	    }
	    MPIR_ERRTEST_ARGNULL(position, "position", mpi_errno);
            /* Validate comm_ptr */
	    /* If comm_ptr is not valid, it will be reset to null */
            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    if (mpi_errno == MPI_SUCCESS) {
		if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
		    MPID_Datatype *datatype_ptr = NULL;

		    MPID_Datatype_get_ptr(datatype, datatype_ptr);
		    MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		    MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
		}
	    }
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

#ifdef HAVE_ERROR_CHECKING /* IMPLEMENTATION-SPECIFIC ERROR CHECKS */
    {
	int tmp_sz;

	MPID_BEGIN_ERROR_CHECKS;
	/* Verify that there is space in the buffer to pack the type */
	MPID_Datatype_get_size_macro(datatype, tmp_sz);

	if (tmp_sz * incount > outcount - *position) {
	    if (*position < 0) {
		MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_ARG,
				     "**argposneg","**argposneg %d",
				     *position)
	    }
	    else if (outcount < 0) {
		MPIU_ERR_SETANDJUMP2(mpi_errno,MPI_ERR_ARG,"**argneg",
				     "**argneg %s %d","outcount",outcount);
	    }
	    else if (incount < 0) {
		MPIU_ERR_SETANDJUMP2(mpi_errno,MPI_ERR_ARG,"**argneg",
				     "**argneg %s %d","incount",incount);
	    }
	    else {
		MPIU_ERR_SETANDJUMP2(mpi_errno,MPI_ERR_ARG,"**argpackbuf",
				     "**argpackbuf %d %d", tmp_sz * incount,
				     outcount - *position);
	    }
	}
	MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPIR_Pack_impl(inbuf, incount, datatype, outbuf, outcount, position);
    if (mpi_errno) goto fn_fail;
    
   /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_pack",
	    "**mpi_pack %p %d %D %p %d %p %C", inbuf, incount, datatype, outbuf, outcount, position, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
