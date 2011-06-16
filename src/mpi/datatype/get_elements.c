/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_elements */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_elements = PMPI_Get_elements
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_elements  MPI_Get_elements
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_elements as PMPI_Get_elements
#endif
/* -- End Profiling Symbol Block */

#ifndef MIN
#define MIN(__a, __b) (((__a) < (__b)) ? (__a) : (__b))
#endif

PMPI_LOCAL int MPIR_Type_get_basic_type_elements(int *bytes_p,
						 int count,
						 MPI_Datatype datatype);
PMPI_LOCAL int MPIR_Type_get_elements(int *bytes_p,
				      int count,
				      MPI_Datatype datatype);

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Get_elements
#define MPI_Get_elements PMPI_Get_elements

/* Note that all helper routines must be within the
 * MPICH_MPI_FROM_PMPI ifdef so that we don't get two copies of the
 * helper routines in the case where we don't have weak symbols. */

/* MPIR_Type_get_basic_type_elements()
 *
 * Arguments:
 * - bytes_p - input/output byte count
 * - count - maximum number of this type to subtract from the bytes; a count
 *           of -1 indicates use as many as we like
 * - datatype - input datatype
 *
 * Returns number of elements available given the two constraints of number of
 * bytes and count of types.  Also reduces the byte count by the amount taken
 * up by the types.
 *
 * Assumptions:
 * - the type passed to this function must be a basic *or* a pairtype
 *   (which aren't basic types)
 * - the count is not zero (otherwise we can't tell between a "no more
 *   complete types" case and a "zero count" case)
 *
 * As per section 4.9.3 of the MPI 1.1 specification, the two-part reduction
 * types are to be treated as structs of the constituent types.  So we have to
 * do something special to handle them correctly in here.
 *
 * As per section 3.12.5 get_count and get_elements report the same value for
 * basic datatypes; I'm currently interpreting this to *not* include these
 * reduction types, as they are considered structs.
 */
PMPI_LOCAL int MPIR_Type_get_basic_type_elements(int *bytes_p,
						 int count,
						 MPI_Datatype datatype)
{
    int elements, usable_bytes, used_bytes, type1_sz, type2_sz;

    if (count == 0) return 0;

    /* determine the maximum number of bytes we should take from the
     * byte count.
     */
    if (count < 0) {
	usable_bytes = *bytes_p;
    }
    else {
	usable_bytes = MIN(*bytes_p,
			   count * MPID_Datatype_get_basic_size(datatype));
    }

    switch (datatype) {
	/* we don't get valid fortran datatype handles in all cases... */
#ifdef HAVE_FORTRAN_BINDING
	case MPI_2REAL:
 	    type1_sz = type2_sz = MPID_Datatype_get_basic_size(MPI_REAL);
	    break;
	case MPI_2DOUBLE_PRECISION:
 	    type1_sz = type2_sz =
		MPID_Datatype_get_basic_size(MPI_DOUBLE_PRECISION);
	    break;
	case MPI_2INTEGER:
 	    type1_sz = type2_sz = MPID_Datatype_get_basic_size(MPI_INTEGER);
	    break;
#endif
	case MPI_2INT:
 	    type1_sz = type2_sz = MPID_Datatype_get_basic_size(MPI_INT);
	    break;
	case MPI_FLOAT_INT:
	    type1_sz = MPID_Datatype_get_basic_size(MPI_FLOAT);
	    type2_sz = MPID_Datatype_get_basic_size(MPI_INT);
	    break;
	case MPI_DOUBLE_INT:
	    type1_sz = MPID_Datatype_get_basic_size(MPI_DOUBLE);
	    type2_sz = MPID_Datatype_get_basic_size(MPI_INT);
	    break;
	case MPI_LONG_INT:
	    type1_sz = MPID_Datatype_get_basic_size(MPI_LONG);
	    type2_sz = MPID_Datatype_get_basic_size(MPI_INT);
	    break;
	case MPI_SHORT_INT:
	    type1_sz = MPID_Datatype_get_basic_size(MPI_SHORT);
	    type2_sz = MPID_Datatype_get_basic_size(MPI_INT);
	    break;
	case MPI_LONG_DOUBLE_INT:
	    type1_sz = MPID_Datatype_get_basic_size(MPI_LONG_DOUBLE);
	    type2_sz = MPID_Datatype_get_basic_size(MPI_INT);
	    break;
	default:
	    /* all other types.  this is more complicated than
	     * necessary for handling these types, but it puts us in the
	     * same code path for all the basics, so we stick with it.
	     */
	    type1_sz = type2_sz = MPID_Datatype_get_basic_size(datatype);
	    break;
    }

    /* determine the number of elements in the region */
    elements = 2 * (usable_bytes / (type1_sz + type2_sz));
    if (usable_bytes % (type1_sz + type2_sz) >= type1_sz) elements++;

    /* determine how many bytes we used up with those elements */
    used_bytes = ((elements / 2) * (type1_sz + type2_sz));
    if (elements % 2 == 1) used_bytes += type1_sz;

    *bytes_p -= used_bytes;

    return elements;
}


/* MPIR_Type_get_elements
 *
 * Arguments:
 * - bytes_p - input/output byte count
 * - count - maximum number of this type to subtract from the bytes; a count
 *           of -1 indicates use as many as we like
 * - datatype - input datatype
 *
 * Returns number of elements available given the two constraints of number of
 * bytes and count of types.  Also reduces the byte count by the amount taken
 * up by the types.
 *
 * This is called from MPI_Get_elements() when it sees a type with multiple
 * element types (datatype_ptr->element_sz = -1).  This function calls itself too.
 */
PMPI_LOCAL int MPIR_Type_get_elements(int *bytes_p,
				      int count,
				      MPI_Datatype datatype)
{
    MPID_Datatype *datatype_ptr = NULL;

    MPID_Datatype_get_ptr(datatype, datatype_ptr); /* invalid if builtin */

    /* if we have gotten down to a type with only one element type,
     * call MPIR_Type_get_basic_type_elements() and return.
     */
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN ||
	datatype == MPI_FLOAT_INT ||
	datatype == MPI_DOUBLE_INT ||
	datatype == MPI_LONG_INT ||
	datatype == MPI_SHORT_INT ||
	datatype == MPI_LONG_DOUBLE_INT)
    {
	return MPIR_Type_get_basic_type_elements(bytes_p, count, datatype);
    }
    else if (datatype_ptr->element_size >= 0) {
	return MPIR_Type_get_basic_type_elements(bytes_p,
						 count * datatype_ptr->n_elements,
						 datatype_ptr->eltype);
    }
    else {
	/* we have bytes left and still don't have a single element size; must
         * recurse.
	 */
	int i, j, typecount = 0, *ints, nr_elements = 0, last_nr_elements;
	MPI_Aint *aints;
	MPI_Datatype *types;

        /* Establish locations of arrays */
        MPID_Type_access_contents(datatype_ptr->handle, &ints, &aints, &types);
        if (!ints || !aints || !types)
            return MPI_ERR_TYPE;

	switch (datatype_ptr->contents->combiner) {
	    case MPI_COMBINER_NAMED:
	    case MPI_COMBINER_DUP:
	    case MPI_COMBINER_RESIZED:
		return MPIR_Type_get_elements(bytes_p, count, *types);
		break;
	    case MPI_COMBINER_CONTIGUOUS:
	    case MPI_COMBINER_VECTOR:
	    case MPI_COMBINER_HVECTOR_INTEGER:
	    case MPI_COMBINER_HVECTOR:
		/* count is first in ints array */
		return MPIR_Type_get_elements(bytes_p, count * (*ints), *types);
		break;
	    case MPI_COMBINER_INDEXED_BLOCK:
		/* count is first in ints array, blocklength is second */
		return MPIR_Type_get_elements(bytes_p,
					      count * ints[0] * ints[1],
					      *types);
		break;
	    case MPI_COMBINER_INDEXED:
	    case MPI_COMBINER_HINDEXED_INTEGER:
	    case MPI_COMBINER_HINDEXED:
		for (i=0; i < (*ints); i++) {
		    /* add up the blocklengths to get a max. # of the next type */
		    typecount += ints[i+1];
		}
		return MPIR_Type_get_elements(bytes_p, count * typecount, *types);
		break;
	    case MPI_COMBINER_STRUCT_INTEGER:
	    case MPI_COMBINER_STRUCT:
		/* In this case we can't simply multiply the count of the next
		 * type by the count of the current type, because we need to
		 * cycle through the types just as the struct would.  thus the
		 * nested loops.
		 *
		 * We need to keep going until we see a "0" elements returned
		 * or we run out of bytes.
		 */


		last_nr_elements = 1; /* seed value */
		for (j=0;
		     (count == -1 || j < count) &&
			 *bytes_p > 0 && last_nr_elements > 0;
		     j++)
		{
		    /* recurse on each type; bytes are reduced in calls */
		    for (i=0; i < (*ints); i++) {
			/* skip zero-count elements of the struct */
			if (ints[i+1] == 0) continue;

			last_nr_elements = MPIR_Type_get_elements(bytes_p,
								  ints[i+1],
								  types[i]);
			nr_elements += last_nr_elements;

			MPIU_Assert(last_nr_elements >= 0);

			if (last_nr_elements == 0) break;
		    }
		}
		return nr_elements;
		break;
	    case MPI_COMBINER_SUBARRAY:
	    case MPI_COMBINER_DARRAY:
	    case MPI_COMBINER_F90_REAL:
	    case MPI_COMBINER_F90_COMPLEX:
	    case MPI_COMBINER_F90_INTEGER:
	    default:
		/* --BEGIN ERROR HANDLING-- */
		MPIU_Assert(0);
		return -1;
		break;
		/* --END ERROR HANDLING-- */
	}
    }
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Get_elements
#undef FCNAME
#define FCNAME "MPI_Get_elements"

/*@
   MPI_Get_elements - Returns the number of basic elements
                      in a datatype

Input Parameters:
+ status - return status of receive operation (Status)
- datatype - datatype used by receive operation (handle)

Output Parameter:
. count - number of received basic elements (integer)

   Notes:

 If the size of the datatype is zero and the amount of data returned as
 determined by 'status' is also zero, this routine will return a count of
 zero.  This is consistent with a clarification made by the MPI Forum.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Get_elements(MPI_Status *status, MPI_Datatype datatype, int *elements)
{
    int mpi_errno = MPI_SUCCESS, byte_count;
    MPID_Datatype *datatype_ptr = NULL;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GET_ELEMENTS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET_ELEMENTS);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* Convert MPI object handles to object pointers */
    MPID_Datatype_get_ptr(datatype, datatype_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(elements, "elements", mpi_errno);
            /* Validate datatype_ptr */
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
		MPID_Datatype_get_ptr(datatype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		if (mpi_errno == MPI_SUCCESS) {
		    MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
		}
	    }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* ... body of routine ...  */

    /* three cases:
     * - nice, simple, single element type
     * - derived type with a zero size
     * - type with multiple element types (nastiest)
     */
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN ||
	(datatype_ptr->element_size != -1 && datatype_ptr->size > 0))
    {
	byte_count = status->count;

	/* QUESTION: WHAT IF SOMEONE GAVE US AN MPI_UB OR MPI_LB???
	 */

	/* in both cases we do not limit the number of types that might
	 * be in bytes
	 */
	if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
	    *elements = MPIR_Type_get_basic_type_elements(&byte_count,
							  -1,
							  datatype_ptr->eltype);
	}
	else {
	    /* Behaves just like MPI_Get_Count in the predefined case */
	    int size;
	    MPID_Datatype_get_size_macro(datatype, size);
            if ((byte_count % size) != 0)
                *elements = MPI_UNDEFINED;
            else
                *elements = MPIR_Type_get_basic_type_elements(&byte_count,
							      -1,
							      datatype);
	}
	MPIU_Assert(byte_count >= 0);
    }
    else if (datatype_ptr->size == 0) {
	if (status->count > 0) {
	    /* --BEGIN ERROR HANDLING-- */

	    /* datatype size of zero and count > 0 should never happen. */

	    (*elements) = MPI_UNDEFINED;
	    /* --END ERROR HANDLING-- */
	}
	else {
	    /* This is ambiguous.  However, discussions on MPI Forum
	     * reached a consensus that this is the correct return
	     * value
	     */
	    (*elements) = 0;
	}
    }
    else /* derived type with weird element type or weird size */ {
	MPIU_Assert(datatype_ptr->element_size == -1);

	byte_count = status->count;
	*elements = MPIR_Type_get_elements(&byte_count, -1, datatype);
    }

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_ELEMENTS);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_get_elements",
	    "**mpi_get_elements %p %D %p", status, datatype, elements);
    }
    mpi_errno = MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
