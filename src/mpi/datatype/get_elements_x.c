/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Internal helper routines.  If you want to get the number of elements from
 * within the MPI library, call MPIR_Get_elements_x_impl instead. */

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
static MPI_Count MPIR_Type_get_basic_type_elements(MPI_Count * bytes_p,
                                                   MPI_Count count, MPI_Datatype datatype)
{
    MPI_Count elements, usable_bytes, used_bytes, type1_sz, type2_sz;

    if (count == 0)
        return 0;

    /* determine the maximum number of bytes we should take from the
     * byte count.
     */
    if (count < 0) {
        usable_bytes = *bytes_p;
    } else {
        usable_bytes = MPL_MIN(*bytes_p, count * MPIR_Datatype_get_basic_size(datatype));
    }

    switch (datatype) {
            /* we don't get valid fortran datatype handles in all cases... */
#ifdef HAVE_FORTRAN_BINDING
        case MPI_2REAL:
            type1_sz = type2_sz = MPIR_Datatype_get_basic_size(MPI_REAL);
            break;
        case MPI_2DOUBLE_PRECISION:
            type1_sz = type2_sz = MPIR_Datatype_get_basic_size(MPI_DOUBLE_PRECISION);
            break;
        case MPI_2INTEGER:
            type1_sz = type2_sz = MPIR_Datatype_get_basic_size(MPI_INTEGER);
            break;
#endif
        case MPI_2INT:
            type1_sz = type2_sz = MPIR_Datatype_get_basic_size(MPI_INT);
            break;
        case MPI_FLOAT_INT:
            type1_sz = MPIR_Datatype_get_basic_size(MPI_FLOAT);
            type2_sz = MPIR_Datatype_get_basic_size(MPI_INT);
            break;
        case MPI_DOUBLE_INT:
            type1_sz = MPIR_Datatype_get_basic_size(MPI_DOUBLE);
            type2_sz = MPIR_Datatype_get_basic_size(MPI_INT);
            break;
        case MPI_LONG_INT:
            type1_sz = MPIR_Datatype_get_basic_size(MPI_LONG);
            type2_sz = MPIR_Datatype_get_basic_size(MPI_INT);
            break;
        case MPI_SHORT_INT:
            type1_sz = MPIR_Datatype_get_basic_size(MPI_SHORT);
            type2_sz = MPIR_Datatype_get_basic_size(MPI_INT);
            break;
        case MPI_LONG_DOUBLE_INT:
            type1_sz = MPIR_Datatype_get_basic_size(MPI_LONG_DOUBLE);
            type2_sz = MPIR_Datatype_get_basic_size(MPI_INT);
            break;
        default:
            /* all other types.  this is more complicated than
             * necessary for handling these types, but it puts us in the
             * same code path for all the basics, so we stick with it.
             */
            type1_sz = type2_sz = MPIR_Datatype_get_basic_size(datatype);
            break;
    }

    /* determine the number of elements in the region */
    elements = 2 * (usable_bytes / (type1_sz + type2_sz));
    if (usable_bytes % (type1_sz + type2_sz) >= type1_sz)
        elements++;

    /* determine how many bytes we used up with those elements */
    used_bytes = ((elements / 2) * (type1_sz + type2_sz));
    if (elements % 2 == 1)
        used_bytes += type1_sz;

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
static MPI_Count MPIR_Type_get_elements(MPI_Count * bytes_p, MPI_Count count, MPI_Datatype datatype)
{
    MPIR_Datatype *datatype_ptr = NULL;

    MPIR_Datatype_get_ptr(datatype, datatype_ptr);      /* invalid if builtin */

    /* if we have gotten down to a type with only one element type,
     * call MPIR_Type_get_basic_type_elements() and return.
     */
    if (MPIR_DATATYPE_IS_PREDEFINED(datatype)) {
        return MPIR_Type_get_basic_type_elements(bytes_p, count, datatype);
    } else if (datatype_ptr->builtin_element_size >= 0) {
        MPI_Datatype basic_type = MPI_DATATYPE_NULL;
        MPIR_Datatype_get_basic_type(datatype_ptr->basic_type, basic_type);
        return MPIR_Type_get_basic_type_elements(bytes_p,
                                                 count * datatype_ptr->n_builtin_elements,
                                                 basic_type);
    } else {
        /* we have bytes left and still don't have a single element size; must
         * recurse.
         */
        int *ints;
        MPI_Aint *aints;
        MPI_Aint *counts;
        MPI_Datatype *types;

        /* Establish locations of arrays */
        MPIR_Datatype_contents *cp = datatype_ptr->contents;
        MPIR_Datatype_access_contents(cp, &ints, &aints, &counts, &types);
        if (!ints || !aints || !types)
            return MPI_ERR_TYPE;

        switch (datatype_ptr->contents->combiner) {
            case MPI_COMBINER_NAMED:
            case MPI_COMBINER_DUP:
            case MPI_COMBINER_RESIZED:
                return MPIR_Type_get_elements(bytes_p, count, *types);
            case MPI_COMBINER_CONTIGUOUS:
            case MPI_COMBINER_VECTOR:
            case MPI_COMBINER_HVECTOR:
            case MPI_COMBINER_SUBARRAY:
                if (cp->nr_counts == 0) {
                    /* count is first in ints array */
                    return MPIR_Type_get_elements(bytes_p, count * ints[0], *types);
                } else {
                    return MPIR_Type_get_elements(bytes_p, count * counts[0], *types);
                }
            case MPI_COMBINER_INDEXED_BLOCK:
            case MPI_COMBINER_HINDEXED_BLOCK:
                if (cp->nr_counts == 0) {
                    /* count is first in ints array, blocklength is second */
                    return MPIR_Type_get_elements(bytes_p, count * ints[0] * ints[1], *types);
                } else {
                    return MPIR_Type_get_elements(bytes_p, count * counts[0] * counts[1], *types);
                }
            case MPI_COMBINER_INDEXED:
            case MPI_COMBINER_HINDEXED:
                {
                    MPI_Aint typecount = 0;     /* total number of subtypes */
                    if (cp->nr_counts == 0) {
                        for (int i = 0; i < ints[0]; i++) {
                            typecount += ints[i + 1];
                        }
                    } else {
                        for (MPI_Aint i = 0; i < counts[0]; i++) {
                            typecount += counts[i + 1];
                        }
                    }
                    return MPIR_Type_get_elements(bytes_p, count * typecount, *types);
                }
            case MPI_COMBINER_STRUCT:
                /* In this case we can't simply multiply the count of the next
                 * type by the count of the current type, because we need to
                 * cycle through the types just as the struct would.  thus the
                 * nested loops.
                 *
                 * We need to keep going until we get less elements than expected
                 * or we run out of bytes.
                 */
                if (cp->nr_counts == 0) {
                    MPI_Count nr_elements = 0;
                    MPI_Count last_nr_elements = 1;     /* seed value */
                    for (MPI_Aint j = 0;
                         (count < 0 || j < count) && *bytes_p > 0 && last_nr_elements > 0; j++) {
                        /* recurse on each type; bytes are reduced in calls */
                        for (int i = 0; i < ints[0]; i++) {
                            /* skip zero-count elements of the struct */
                            if (ints[i + 1] == 0)
                                continue;

                            last_nr_elements =
                                MPIR_Type_get_elements(bytes_p, ints[i + 1], types[i]);
                            nr_elements += last_nr_elements;

                            MPIR_Assert(last_nr_elements >= 0);

                            if (last_nr_elements < ints[i + 1])
                                break;
                        }
                    }
                    return nr_elements;
                } else {
                    MPI_Count nr_elements = 0;
                    MPI_Count last_nr_elements = 1;     /* seed value */
                    for (MPI_Aint j = 0;
                         (count < 0 || j < count) && *bytes_p > 0 && last_nr_elements > 0; j++) {
                        /* recurse on each type; bytes are reduced in calls */
                        for (int i = 0; i < counts[0]; i++) {
                            /* skip zero-count elements of the struct */
                            if (counts[i + 1] == 0)
                                continue;

                            last_nr_elements =
                                MPIR_Type_get_elements(bytes_p, counts[i + 1], types[i]);
                            nr_elements += last_nr_elements;

                            MPIR_Assert(last_nr_elements >= 0);

                            if (last_nr_elements < counts[i + 1])
                                break;
                        }
                    }
                    return nr_elements;
                }
            case MPI_COMBINER_DARRAY:
            case MPI_COMBINER_F90_REAL:
            case MPI_COMBINER_F90_COMPLEX:
            case MPI_COMBINER_F90_INTEGER:
            case MPI_COMBINER_HVECTOR_INTEGER:
            case MPI_COMBINER_HINDEXED_INTEGER:
            case MPI_COMBINER_STRUCT_INTEGER:
            default:
                /* --BEGIN ERROR HANDLING-- */
                MPIR_Assert(0);
                return -1;
                /* --END ERROR HANDLING-- */
        }
    }
}

/* MPIR_Get_elements_x_impl
 *
 * Arguments:
 * - byte_count - input/output byte count
 * - datatype - input datatype
 * - elements - Number of basic elements this byte_count would contain
 *
 * Returns number of elements available given the two constraints of number of
 * bytes and count of types.  Also reduces the byte count by the amount taken
 * up by the types.
 */
int MPIR_Get_elements_x_impl(MPI_Count * byte_count, MPI_Datatype datatype, MPI_Count * elements)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *datatype_ptr = NULL;

    if (!HANDLE_IS_BUILTIN(datatype)) {
        MPIR_Datatype_get_ptr(datatype, datatype_ptr);
    }

    /* three cases:
     * - nice, simple, single element type
     * - derived type with a zero size
     * - type with multiple element types (nastiest)
     */
    if (HANDLE_IS_BUILTIN(datatype) ||
        (datatype_ptr->builtin_element_size != -1 && datatype_ptr->size > 0)) {
        /* in both cases we do not limit the number of types that might
         * be in bytes
         */
        if (!HANDLE_IS_BUILTIN(datatype)) {
            MPI_Datatype basic_type = MPI_DATATYPE_NULL;
            MPIR_Datatype_get_basic_type(datatype_ptr->basic_type, basic_type);
            *elements = MPIR_Type_get_basic_type_elements(byte_count, -1, basic_type);
        } else {
            /* Behaves just like MPI_Get_Count in the predefined case */
            MPI_Count size;
            MPIR_Datatype_get_size_macro(datatype, size);
            if ((*byte_count % size) != 0)
                *elements = MPI_UNDEFINED;
            else
                *elements = MPIR_Type_get_basic_type_elements(byte_count, -1, datatype);
        }
        MPIR_Assert(*byte_count >= 0);
    } else if (datatype_ptr->size == 0) {
        if (*byte_count > 0) {
            /* --BEGIN ERROR HANDLING-- */

            /* datatype size of zero and count > 0 should never happen. */

            (*elements) = MPI_UNDEFINED;
            /* --END ERROR HANDLING-- */
        } else {
            /* This is ambiguous.  However, discussions on MPI Forum
             * reached a consensus that this is the correct return
             * value
             */
            (*elements) = 0;
        }
    } else {    /* derived type with weird element type or weird size */

        MPIR_Assert(datatype_ptr->builtin_element_size == -1);

        *elements = MPIR_Type_get_elements(byte_count, -1, datatype);
    }

    return mpi_errno;
}
