/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "datatype.h"
#include "dataloop.h"
#include "mpir_typerep.h"

static void update_type_vector(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride,
                               MPI_Datatype oldtype, MPIR_Datatype * newtype, bool strideinbytes)
{
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    /* we only need calc eff_stride if count > 1 */
    MPI_Aint eff_stride = 0;

    if (HANDLE_IS_BUILTIN(oldtype)) {
        MPI_Aint el_sz = (MPI_Aint) MPIR_Datatype_get_basic_size(oldtype);

        old_lb = 0;
        old_true_lb = 0;
        old_ub = el_sz;
        old_true_ub = el_sz;
        old_extent = el_sz;

        newtype->size = count * blocklength * el_sz;

        newtype->alignsize = MPIR_Datatype_builtintype_alignment(oldtype);
        newtype->n_builtin_elements = count * blocklength;
        newtype->builtin_element_size = el_sz;
        newtype->basic_type = oldtype;

        if (count > 1) {
            eff_stride = (strideinbytes) ? stride : (stride * el_sz);
        }
    } else {    /* user-defined base type (oldtype) */

        MPIR_Datatype *old_dtp;

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        old_lb = old_dtp->lb;
        old_true_lb = old_dtp->true_lb;
        old_ub = old_dtp->ub;
        old_true_ub = old_dtp->true_ub;
        old_extent = old_dtp->extent;

        newtype->size = count * blocklength * old_dtp->size;

        newtype->alignsize = old_dtp->alignsize;
        newtype->n_builtin_elements = count * blocklength * old_dtp->n_builtin_elements;
        newtype->builtin_element_size = old_dtp->builtin_element_size;
        newtype->basic_type = old_dtp->basic_type;

        if (count > 1) {
            eff_stride = (strideinbytes) ? stride : (stride * old_dtp->extent);
        }
    }

    MPII_DATATYPE_VECTOR_LB_UB(count, eff_stride, blocklength,
                               old_lb, old_ub, old_extent, newtype->lb, newtype->ub);
    newtype->true_lb = newtype->lb + (old_true_lb - old_lb);
    newtype->true_ub = newtype->ub + (old_true_ub - old_ub);
    newtype->extent = newtype->ub - newtype->lb;
}

static void update_type_indexed(MPI_Aint count, const MPI_Aint * blocklength_array,
                                const MPI_Aint * displacement_array, MPI_Datatype oldtype,
                                MPIR_Datatype * newtype, bool dispinbytes)
{
    MPI_Aint i;
    MPI_Aint el_ct, old_ct, old_sz;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    MPI_Aint min_lb = 0, max_ub = 0, eff_disp;

    if (HANDLE_IS_BUILTIN(oldtype)) {
        /* builtins are handled differently than user-defined types because
         * they have no associated typerep or datatype structure.
         */
        MPI_Aint el_sz = MPIR_Datatype_get_basic_size(oldtype);
        old_sz = el_sz;
        el_ct = 1;

        old_lb = 0;
        old_true_lb = 0;
        old_ub = el_sz;
        old_true_ub = el_sz;
        old_extent = el_sz;

        newtype->alignsize = MPIR_Datatype_builtintype_alignment(oldtype);
        newtype->builtin_element_size = el_sz;
        newtype->basic_type = oldtype;
    } else {
        /* user-defined base type (oldtype) */
        MPIR_Datatype *old_dtp;

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        /* Ensure that "builtin_element_size" fits into an int datatype. */
        MPIR_Ensure_Aint_fits_in_int(old_dtp->builtin_element_size);

        old_sz = old_dtp->size;
        el_ct = old_dtp->n_builtin_elements;

        old_lb = old_dtp->lb;
        old_true_lb = old_dtp->true_lb;
        old_ub = old_dtp->ub;
        old_true_ub = old_dtp->true_ub;
        old_extent = old_dtp->extent;

        newtype->alignsize = old_dtp->alignsize;
        newtype->builtin_element_size = old_dtp->builtin_element_size;
        newtype->basic_type = old_dtp->basic_type;
    }

    /* find the first nonzero blocklength element */
    i = 0;
    while (i < count && blocklength_array[i] == 0)
        i++;

    MPIR_Assert(i < count);

    /* priming for loop */
    old_ct = blocklength_array[i];
    eff_disp = (dispinbytes) ? displacement_array[i] : displacement_array[i] * old_extent;

    MPII_DATATYPE_BLOCK_LB_UB(blocklength_array[i],
                              eff_disp, old_lb, old_ub, old_extent, min_lb, max_ub);

    /* determine min lb, max ub, and count of old types in remaining
     * nonzero size blocks
     */
    for (i++; i < count; i++) {
        MPI_Aint tmp_lb, tmp_ub;

        if (blocklength_array[i] > 0) {
            old_ct += blocklength_array[i];     /* add more oldtypes */

            eff_disp = (dispinbytes) ? displacement_array[i] : displacement_array[i] * old_extent;

            /* calculate ub and lb for this block */
            MPII_DATATYPE_BLOCK_LB_UB(blocklength_array[i],
                                      eff_disp, old_lb, old_ub, old_extent, tmp_lb, tmp_ub);

            if (tmp_lb < min_lb)
                min_lb = tmp_lb;
            if (tmp_ub > max_ub)
                max_ub = tmp_ub;
        }
    }

    newtype->size = old_ct * old_sz;

    newtype->lb = min_lb;
    newtype->ub = max_ub;
    newtype->true_lb = min_lb + (old_true_lb - old_lb);
    newtype->true_ub = max_ub + (old_true_ub - old_ub);
    newtype->extent = max_ub - min_lb;

    newtype->n_builtin_elements = old_ct * el_ct;
}

static void update_type_blockindexed(MPI_Aint count, MPI_Aint blocklength,
                                     const MPI_Aint displacement_array[],
                                     MPI_Datatype oldtype, MPIR_Datatype * newtype,
                                     bool dispinbytes)
{
    MPI_Aint i;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    MPI_Aint min_lb = 0, max_ub = 0, eff_disp;

    if (HANDLE_IS_BUILTIN(oldtype)) {
        MPI_Aint el_sz = MPIR_Datatype_get_basic_size(oldtype);

        old_lb = 0;
        old_true_lb = 0;
        old_ub = el_sz;
        old_true_ub = el_sz;
        old_extent = el_sz;

        newtype->size = count * blocklength * el_sz;

        newtype->alignsize = MPIR_Datatype_builtintype_alignment(oldtype);
        newtype->n_builtin_elements = count * blocklength;
        newtype->builtin_element_size = el_sz;
        newtype->basic_type = oldtype;
    } else {
        /* user-defined base type (oldtype) */
        MPIR_Datatype *old_dtp;

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        old_lb = old_dtp->lb;
        old_true_lb = old_dtp->true_lb;
        old_ub = old_dtp->ub;
        old_true_ub = old_dtp->true_ub;
        old_extent = old_dtp->extent;

        newtype->size = count * blocklength * old_dtp->size;

        newtype->alignsize = old_dtp->alignsize;
        newtype->n_builtin_elements = count * blocklength * old_dtp->n_builtin_elements;
        newtype->builtin_element_size = old_dtp->builtin_element_size;
        newtype->basic_type = old_dtp->basic_type;
    }

    /* priming for loop */
    eff_disp = (dispinbytes) ? displacement_array[0] : displacement_array[0] * old_extent;
    MPII_DATATYPE_BLOCK_LB_UB(blocklength, eff_disp, old_lb, old_ub, old_extent, min_lb, max_ub);

    /* determine new min lb and max ub */
    for (i = 1; i < count; i++) {
        MPI_Aint tmp_lb, tmp_ub;

        eff_disp = (dispinbytes) ? displacement_array[i] : displacement_array[i] * old_extent;
        MPII_DATATYPE_BLOCK_LB_UB(blocklength,
                                  eff_disp, old_lb, old_ub, old_extent, tmp_lb, tmp_ub);

        if (tmp_lb < min_lb)
            min_lb = tmp_lb;
        if (tmp_ub > max_ub)
            max_ub = tmp_ub;
    }

    newtype->lb = min_lb;
    newtype->ub = max_ub;
    newtype->true_lb = min_lb + (old_true_lb - old_lb);
    newtype->true_ub = max_ub + (old_true_ub - old_ub);
    newtype->extent = max_ub - min_lb;
}

static MPI_Aint struct_alignsize(MPI_Aint count, const MPI_Datatype * oldtype_array)
{
    MPI_Aint max_alignsize = 0, tmp_alignsize;

    for (MPI_Aint i = 0; i < count; i++) {
        if (HANDLE_IS_BUILTIN(oldtype_array[i])) {
            tmp_alignsize = MPIR_Datatype_builtintype_alignment(oldtype_array[i]);
        } else {
            MPIR_Datatype *dtp;
            MPIR_Datatype_get_ptr(oldtype_array[i], dtp);
            tmp_alignsize = dtp->alignsize;
        }
        if (max_alignsize < tmp_alignsize)
            max_alignsize = tmp_alignsize;
    }

    return max_alignsize;
}

/* end static functions */

int MPIR_Typerep_create_vector(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride,
                               MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    update_type_vector(count, blocklength, stride, oldtype, newtype, 0);

    return MPI_SUCCESS;
}

int MPIR_Typerep_create_hvector(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride,
                                MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    update_type_vector(count, blocklength, stride, oldtype, newtype, 1);

    return MPI_SUCCESS;
}

int MPIR_Typerep_create_contig(MPI_Aint count, MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    if (HANDLE_IS_BUILTIN(oldtype)) {
        MPI_Aint el_sz = MPIR_Datatype_get_basic_size(oldtype);

        newtype->size = count * el_sz;
        newtype->true_lb = 0;
        newtype->lb = 0;
        newtype->true_ub = count * el_sz;
        newtype->ub = newtype->true_ub;
        newtype->extent = newtype->ub - newtype->lb;

        newtype->alignsize = MPIR_Datatype_builtintype_alignment(oldtype);
        newtype->n_builtin_elements = count;
        newtype->builtin_element_size = el_sz;
        newtype->basic_type = oldtype;
    } else {
        /* user-defined base type (oldtype) */
        MPIR_Datatype *old_dtp;

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        newtype->size = count * old_dtp->size;

        MPII_DATATYPE_CONTIG_LB_UB(count, old_dtp->lb, old_dtp->ub, old_dtp->extent,
                                   newtype->lb, newtype->ub);

        /* easiest to calc true lb/ub relative to lb/ub; doesn't matter
         * if there are sticky lb/ubs or not when doing this.
         */
        newtype->true_lb = newtype->lb + (old_dtp->true_lb - old_dtp->lb);
        newtype->true_ub = newtype->ub + (old_dtp->true_ub - old_dtp->ub);
        newtype->extent = newtype->ub - newtype->lb;

        newtype->alignsize = old_dtp->alignsize;
        newtype->n_builtin_elements = count * old_dtp->n_builtin_elements;
        newtype->builtin_element_size = old_dtp->builtin_element_size;
        newtype->basic_type = old_dtp->basic_type;
    }

    return MPI_SUCCESS;
}

int MPIR_Typerep_create_dup(MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_Datatype *dtp;

    MPIR_Datatype_get_ptr(oldtype, dtp);

    newtype->size = dtp->size;
    newtype->extent = dtp->extent;
    newtype->ub = dtp->ub;
    newtype->lb = dtp->lb;
    newtype->true_ub = dtp->true_ub;
    newtype->true_lb = dtp->true_lb;
    newtype->alignsize = dtp->alignsize;

    newtype->n_builtin_elements = dtp->n_builtin_elements;
    newtype->builtin_element_size = dtp->builtin_element_size;
    newtype->basic_type = dtp->basic_type;

    return MPI_SUCCESS;
}

int MPIR_Typerep_create_indexed_block(MPI_Aint count, MPI_Aint blocklength,
                                      const MPI_Aint array_of_displacements[],
                                      MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    update_type_blockindexed(count, blocklength, array_of_displacements, oldtype, newtype, 0);

    return MPI_SUCCESS;
}

int MPIR_Typerep_create_hindexed_block(MPI_Aint count, MPI_Aint blocklength,
                                       const MPI_Aint * array_of_displacements,
                                       MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    update_type_blockindexed(count, blocklength, array_of_displacements, oldtype, newtype, 1);

    return MPI_SUCCESS;
}

int MPIR_Typerep_create_indexed(MPI_Aint count, const MPI_Aint * array_of_blocklengths,
                                const MPI_Aint * array_of_displacements, MPI_Datatype oldtype,
                                MPIR_Datatype * newtype)
{
    update_type_indexed(count, array_of_blocklengths, array_of_displacements, oldtype, newtype, 0);

    return MPI_SUCCESS;
}

int MPIR_Typerep_create_hindexed(MPI_Aint count, const MPI_Aint * array_of_blocklengths,
                                 const MPI_Aint * array_of_displacements, MPI_Datatype oldtype,
                                 MPIR_Datatype * newtype)
{
    update_type_indexed(count, array_of_blocklengths, array_of_displacements, oldtype, newtype, 1);

    return MPI_SUCCESS;
}

int MPIR_Typerep_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                                MPIR_Datatype * newtype)
{
    if (HANDLE_IS_BUILTIN(oldtype)) {
        int oldsize = MPIR_Datatype_get_basic_size(oldtype);

        newtype->size = oldsize;
        newtype->true_lb = 0;
        newtype->lb = lb;
        newtype->true_ub = oldsize;
        newtype->ub = lb + extent;
        newtype->extent = extent;
        newtype->alignsize = MPIR_Datatype_builtintype_alignment(oldtype);
        newtype->n_builtin_elements = 1;
        newtype->builtin_element_size = oldsize;
        newtype->basic_type = oldtype;
    } else {
        MPIR_Datatype *old_dtp;
        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        newtype->size = old_dtp->size;
        newtype->true_lb = old_dtp->true_lb;
        newtype->lb = lb;
        newtype->true_ub = old_dtp->true_ub;
        newtype->ub = lb + extent;
        newtype->extent = extent;
        newtype->alignsize = old_dtp->alignsize;
        newtype->n_builtin_elements = old_dtp->n_builtin_elements;
        newtype->builtin_element_size = old_dtp->builtin_element_size;
        newtype->basic_type = old_dtp->basic_type;
    }

    return MPI_SUCCESS;
}

int MPIR_Typerep_create_struct(MPI_Aint count, const MPI_Aint * array_of_blocklengths,
                               const MPI_Aint * array_of_displacements,
                               const MPI_Datatype * array_of_types, MPIR_Datatype * newtype)
{
    int found_true_lb = 0, found_true_ub = 0, found_el_type = 0, found_lb = 0, found_ub = 0;
    MPI_Aint el_sz = 0;
    MPI_Aint size = 0;
    MPI_Datatype el_type = MPI_DATATYPE_NULL;
    MPI_Aint true_lb_disp = 0, true_ub_disp = 0, lb_disp = 0, ub_disp = 0;

    for (MPI_Aint i = 0; i < count; i++) {
        MPI_Aint tmp_lb, tmp_ub, tmp_true_lb, tmp_true_ub;
        MPI_Aint tmp_el_sz;
        MPI_Datatype tmp_el_type;
        MPIR_Datatype *old_dtp = NULL;

        /* Interpreting typemap to not include 0 blklen things. -- Rob
         * Ross, 10/31/2005
         */
        if (array_of_blocklengths[i] == 0)
            continue;

        if (HANDLE_IS_BUILTIN(array_of_types[i])) {
            tmp_el_sz = MPIR_Datatype_get_basic_size(array_of_types[i]);
            tmp_el_type = array_of_types[i];

            MPII_DATATYPE_BLOCK_LB_UB(array_of_blocklengths[i],
                                      array_of_displacements[i],
                                      0, tmp_el_sz, tmp_el_sz, tmp_lb, tmp_ub);
            tmp_true_lb = tmp_lb;
            tmp_true_ub = tmp_ub;

            size += tmp_el_sz * array_of_blocklengths[i];
        } else {
            MPIR_Datatype_get_ptr(array_of_types[i], old_dtp);

            /* Ensure that "builtin_element_size" fits into an int datatype. */
            MPIR_Ensure_Aint_fits_in_int(old_dtp->builtin_element_size);

            tmp_el_sz = old_dtp->builtin_element_size;
            tmp_el_type = old_dtp->basic_type;

            MPII_DATATYPE_BLOCK_LB_UB(array_of_blocklengths[i],
                                      array_of_displacements[i],
                                      old_dtp->lb, old_dtp->ub, old_dtp->extent, tmp_lb, tmp_ub);
            tmp_true_lb = tmp_lb + (old_dtp->true_lb - old_dtp->lb);
            tmp_true_ub = tmp_ub + (old_dtp->true_ub - old_dtp->ub);

            size += old_dtp->size * array_of_blocklengths[i];
        }

        /* element size and type */
        if (found_el_type == 0) {
            el_sz = tmp_el_sz;
            el_type = tmp_el_type;
            found_el_type = 1;
        } else if (el_sz != tmp_el_sz) {
            el_sz = -1;
            el_type = MPI_DATATYPE_NULL;
        } else if (el_type != tmp_el_type) {
            /* Q: should we set el_sz = -1 even though the same? */
            el_type = MPI_DATATYPE_NULL;
        }

        /* keep lowest lb/true_lb and highest ub/true_ub
         *
         * note: checking for contiguity at the same time, to avoid
         *       yet another pass over the arrays
         */
        if (!found_true_lb) {
            found_true_lb = 1;
            true_lb_disp = tmp_true_lb;
        } else if (true_lb_disp > tmp_true_lb) {
            /* element starts before previous */
            true_lb_disp = tmp_true_lb;
        }

        if (!found_lb) {
            found_lb = 1;
            lb_disp = tmp_lb;
        } else if (lb_disp > tmp_lb) {
            /* lb before previous */
            lb_disp = tmp_lb;
        }

        if (!found_true_ub) {
            found_true_ub = 1;
            true_ub_disp = tmp_true_ub;
        } else if (true_ub_disp < tmp_true_ub) {
            true_ub_disp = tmp_true_ub;
        } else {
            /* element ends before previous ended */
        }

        if (!found_ub) {
            found_ub = 1;
            ub_disp = tmp_ub;
        } else if (ub_disp < tmp_ub) {
            ub_disp = tmp_ub;
        } else {
            /* ub before previous */
        }
    }

    newtype->n_builtin_elements = -1;   /* TODO */
    newtype->builtin_element_size = el_sz;
    newtype->basic_type = el_type;

    newtype->true_lb = true_lb_disp;
    newtype->lb = lb_disp;

    newtype->true_ub = true_ub_disp;
    newtype->ub = ub_disp;

    newtype->alignsize = struct_alignsize(count, array_of_types);

    newtype->extent = newtype->ub - newtype->lb;
    /* account for padding */
    MPI_Aint epsilon = (newtype->alignsize > 0) ?
        newtype->extent % ((MPI_Aint) (newtype->alignsize)) : 0;

    if (epsilon) {
        newtype->ub += ((MPI_Aint) (newtype->alignsize) - epsilon);
        newtype->extent = newtype->ub - newtype->lb;
    }

    newtype->size = size;

    return MPI_SUCCESS;
}

int MPIR_Typerep_create_pairtype(MPI_Datatype type, MPIR_Datatype * newtype)
{
    int alignsize;
    switch (type) {
        case MPI_FLOAT_INT:
            alignsize = MPL_MAX(ALIGNOF_FLOAT, ALIGNOF_INT);
            break;
        case MPI_DOUBLE_INT:
            alignsize = MPL_MAX(ALIGNOF_DOUBLE, ALIGNOF_INT);
            break;
        case MPI_LONG_INT:
            alignsize = ALIGNOF_LONG;
            break;
        case MPI_SHORT_INT:
            alignsize = ALIGNOF_INT;
            break;
        case MPI_LONG_DOUBLE_INT:
            alignsize = MPL_MAX(ALIGNOF_LONG_DOUBLE, ALIGNOF_INT);
            break;
        default:
            /* --BEGIN ERROR HANDLING-- */
            return MPIR_Err_create_code(MPI_SUCCESS,
                                        MPIR_ERR_RECOVERABLE,
                                        "MPIR_Type_create_pairtype",
                                        __LINE__, MPI_ERR_OTHER, "**dtype", 0);
            /* --END ERROR HANDLING-- */
    }
    newtype->alignsize = alignsize;
    return MPI_SUCCESS;
}
