/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_elements_x */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_elements_x = PMPI_Get_elements_x
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_elements_x  MPI_Get_elements_x
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_elements_x as PMPI_Get_elements_x
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Get_elements_x(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count) __attribute__((weak,alias("PMPI_Get_elements_x")));
#endif
/* -- End Profiling Symbol Block */

/* Internal helper routines.  If you want to get the number of elements from
 * within the MPI library, call MPIR_Get_elements_x_impl instead. */
PMPI_LOCAL MPI_Count MPIR_Type_get_basic_type_elements(MPI_Count *bytes_p,
                                                 MPI_Count count,
                                                 MPI_Datatype datatype);
PMPI_LOCAL MPI_Count MPIR_Type_get_elements(MPI_Count *bytes_p,
                                      MPI_Count count,
                                      MPI_Datatype datatype);

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Get_elements_x
#define MPI_Get_elements_x PMPI_Get_elements_x

/* any non-MPI functions go here, especially non-static ones */

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
PMPI_LOCAL MPI_Count MPIR_Type_get_basic_type_elements(MPI_Count *bytes_p,
                                                       MPI_Count count,
                                                       MPI_Datatype datatype)
{
    MPI_Count elements, usable_bytes, used_bytes, type1_sz, type2_sz;

    if (count == 0) return 0;

    /* determine the maximum number of bytes we should take from the
     * byte count.
     */
    if (count < 0) {
        usable_bytes = *bytes_p;
    }
    else {
        usable_bytes = MPIR_MIN(*bytes_p,
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
 *           of <0 indicates use as many as we like
 * - datatype - input datatype
 *
 * Returns number of elements available given the two constraints of number of
 * bytes and count of types.  Also reduces the byte count by the amount taken
 * up by the types.
 *
 * This is called from MPI_Get_elements() when it sees a type with multiple
 * element types (datatype_ptr->element_sz = -1).  This function calls itself too.
 */
PMPI_LOCAL MPI_Count MPIR_Type_get_elements(MPI_Count *bytes_p,
                                            MPI_Count count,
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
    else if (datatype_ptr->builtin_element_size >= 0) {
        MPI_Datatype basic_type = MPI_DATATYPE_NULL;
        MPID_Datatype_get_basic_type(datatype_ptr->basic_type, basic_type);
        return MPIR_Type_get_basic_type_elements(bytes_p,
                                                 count * datatype_ptr->n_builtin_elements,
                                                 basic_type);
    }
    else {
        /* we have bytes left and still don't have a single element size; must
         * recurse.
         */
        int i, j, *ints;
        MPI_Count typecount = 0, nr_elements = 0, last_nr_elements;
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
            case MPI_COMBINER_HINDEXED_BLOCK:
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
                 * We need to keep going until we get less elements than expected
                 * or we run out of bytes.
                 */


                last_nr_elements = 1; /* seed value */
                for (j=0;
                     (count < 0 || j < count) &&
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

                        if (last_nr_elements < ints[i+1]) break;
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

#undef FUNCNAME
#define FUNCNAME MPIR_Get_elements_x_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Get_elements_x_impl(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *elements)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPI_Count byte_count;

    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
        MPID_Datatype_get_ptr(datatype, datatype_ptr);
    }

    /* three cases:
     * - nice, simple, single element type
     * - derived type with a zero size
     * - type with multiple element types (nastiest)
     */
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN ||
        (datatype_ptr->builtin_element_size != -1 && datatype_ptr->size > 0))
    {
        byte_count = MPIR_STATUS_GET_COUNT(*status);

        /* QUESTION: WHAT IF SOMEONE GAVE US AN MPI_UB OR MPI_LB???
         */

        /* in both cases we do not limit the number of types that might
         * be in bytes
         */
        if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
            MPI_Datatype basic_type = MPI_DATATYPE_NULL;
            MPID_Datatype_get_basic_type(datatype_ptr->basic_type, basic_type);
            *elements = MPIR_Type_get_basic_type_elements(&byte_count,
                                                          -1,
                                                          basic_type);
        }
        else {
            /* Behaves just like MPI_Get_Count in the predefined case */
            MPI_Count size;
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
        if (MPIR_STATUS_GET_COUNT(*status) > 0) {
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
        MPIU_Assert(datatype_ptr->builtin_element_size == -1);

        byte_count = MPIR_STATUS_GET_COUNT(*status);
        *elements = MPIR_Type_get_elements(&byte_count, -1, datatype);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Get_elements_x
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* N.B. "count" is the name mandated by the MPI-3 standard, but it should
 * probably be called "elements" instead and is handled that way in the _impl
 * routine [goodell@ 2012-11-05 */
/*@
MPI_Get_elements_x - Returns the number of basic elements
                     in a datatype

Input Parameters:
+ status - return status of receive operation (Status)
- datatype - datatype used by receive operation (handle)

Output Parameters:
. count - number of received basic elements (integer)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Get_elements_x(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GET_ELEMENTS_X);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET_ELEMENTS_X);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            /* TODO more checks may be appropriate */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype *datatype_ptr = NULL;
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
            }

            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Get_elements_x_impl(status, datatype, count);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_ELEMENTS_X);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_get_elements_x", "**mpi_get_elements_x %p %D %p", status, datatype, count);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
