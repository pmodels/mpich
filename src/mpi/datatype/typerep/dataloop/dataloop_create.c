/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop_internal.h"

#include <stdlib.h>
#include <limits.h>

#define PAIRTYPE_CONTENTS(mt1_,ut1_,mt2_,ut2_)                          \
    {                                                                   \
        struct { ut1_ a; ut2_ b; } foo;                                 \
        disps[0] = 0;                                                   \
        disps[1] = (MPI_Aint) ((char *) &foo.b - (char *) &foo.a); \
        types[0] = mt1_;                                                \
        types[1] = mt2_;                                                \
    }

/*@
create_pairtype - create dataloop for a pairtype

   Arguments:
+  MPI_Datatype type - the pairtype
.  MPII_Dataloop **output_dataloop_ptr

.N Errors
.N Returns 0 on success, -1 on failure.

   Note:
   This function simply creates the appropriate input parameters for
   use with Dataloop_create_struct and then calls that function.

   This same function could be used to create dataloops for any type
   that actually consists of two distinct elements.
@*/
static int create_pairtype(MPI_Datatype type, MPII_Dataloop ** dlp_p)
{
    int blocks[2] = { 1, 1 };
    MPI_Aint disps[2];
    MPI_Datatype types[2];

    MPIR_Assert(type == MPI_FLOAT_INT || type == MPI_DOUBLE_INT ||
                type == MPI_LONG_INT || type == MPI_SHORT_INT ||
                type == MPI_LONG_DOUBLE_INT || type == MPI_2INT);

    if (type == MPI_FLOAT_INT)
        PAIRTYPE_CONTENTS(MPI_FLOAT, float, MPI_INT, int);
    if (type == MPI_DOUBLE_INT)
        PAIRTYPE_CONTENTS(MPI_DOUBLE, double, MPI_INT, int);
    if (type == MPI_LONG_INT)
        PAIRTYPE_CONTENTS(MPI_LONG, long, MPI_INT, int);
    if (type == MPI_SHORT_INT)
        PAIRTYPE_CONTENTS(MPI_SHORT, short, MPI_INT, int);
    if (type == MPI_LONG_DOUBLE_INT)
        PAIRTYPE_CONTENTS(MPI_LONG_DOUBLE, long double, MPI_INT, int);
    if (type == MPI_2INT)
        PAIRTYPE_CONTENTS(MPI_INT, int, MPI_INT, int);

    return MPII_Dataloop_create_struct(2, blocks, disps, types, dlp_p);
}

static void create_named(MPI_Datatype type, MPII_Dataloop ** dlp_p);

void MPIR_Dataloop_create(MPI_Datatype type, void **dlp_p_)
{
    int i;
    int err ATTRIBUTE((unused));

    int nr_ints, nr_aints, nr_types, combiner;
    MPI_Datatype *types;
    int *ints;
    MPI_Aint *aints;

    MPII_Dataloop *old_dlp;

    int dummy1, dummy2, dummy3, type0_combiner, ndims;
    MPI_Datatype tmptype;

    MPI_Aint stride;
    MPI_Aint *disps;
    MPI_Aint *blklen;
    MPII_Dataloop **dlp_p = (MPII_Dataloop **) dlp_p_;

    if (type == MPI_FLOAT_INT || type == MPI_DOUBLE_INT ||
        type == MPI_LONG_INT || type == MPI_SHORT_INT ||
        type == MPI_LONG_DOUBLE_INT || type == MPI_2INT) {
        create_pairtype(type, dlp_p);
        return;
    }

    MPIR_Type_get_envelope(type, &nr_ints, &nr_aints, &nr_types, &combiner);

    /* some named types do need dataloops; handle separately. */
    if (combiner == MPI_COMBINER_NAMED) {
        create_named(type, dlp_p);
        return;
    } else if (combiner == MPI_COMBINER_F90_REAL ||
               combiner == MPI_COMBINER_F90_COMPLEX || combiner == MPI_COMBINER_F90_INTEGER) {
        MPI_Datatype f90basetype;
        MPIR_Datatype_get_basic_type(type, f90basetype);
        MPII_Dataloop_create_contiguous(1 /* count */ ,
                                        f90basetype, dlp_p);
        return;
    }

    /* Q: should we also check for "hasloop", or is the COMBINER
     *    check above enough to weed out everything that wouldn't
     *    have a loop?
     */
    MPII_DATALOOP_GET_LOOPPTR(type, old_dlp);
    if (old_dlp != NULL) {
        /* dataloop already created; just return it. */
        *dlp_p = old_dlp;
        return;
    }

    MPIR_Type_access_contents(type, &ints, &aints, &types);

    /* first check for zero count on types where that makes sense */
    switch (combiner) {
        case MPI_COMBINER_CONTIGUOUS:
        case MPI_COMBINER_VECTOR:
        case MPI_COMBINER_HVECTOR_INTEGER:
        case MPI_COMBINER_HVECTOR:
        case MPI_COMBINER_INDEXED_BLOCK:
        case MPI_COMBINER_HINDEXED_BLOCK:
        case MPI_COMBINER_INDEXED:
        case MPI_COMBINER_HINDEXED_INTEGER:
        case MPI_COMBINER_HINDEXED:
        case MPI_COMBINER_STRUCT_INTEGER:
        case MPI_COMBINER_STRUCT:
            if (ints[0] == 0) {
                MPII_Dataloop_create_contiguous(0, MPI_INT, dlp_p);
                goto clean_exit;
            }
            break;
        default:
            break;
    }

    /* recurse, processing types "below" this one before processing
     * this one, if those type don't already have dataloops.
     *
     * note: in the struct case below we'll handle any additional
     *       types "below" the current one.
     */
    MPIR_Type_get_envelope(types[0], &dummy1, &dummy2, &dummy3, &type0_combiner);
    if (type0_combiner != MPI_COMBINER_NAMED) {
        MPII_DATALOOP_GET_LOOPPTR(types[0], old_dlp);
        if (old_dlp == NULL) {
            /* no dataloop already present; create and store one */
            MPIR_Dataloop_create(types[0], (void **) &old_dlp);

            MPII_DATALOOP_SET_LOOPPTR(types[0], old_dlp);
        }
    }

    switch (combiner) {
        case MPI_COMBINER_DUP:
            if (type0_combiner != MPI_COMBINER_NAMED) {
                MPIR_Dataloop_dup(old_dlp, (void **) dlp_p);
            } else {
                MPII_Dataloop_create_contiguous(1, types[0], dlp_p);
            }
            break;
        case MPI_COMBINER_RESIZED:
            if (type0_combiner != MPI_COMBINER_NAMED) {
                MPIR_Dataloop_dup(old_dlp, (void **) dlp_p);
            } else {
                MPII_Dataloop_create_contiguous(1, types[0], dlp_p);

                (*dlp_p)->el_extent = aints[1]; /* extent */
            }
            break;
        case MPI_COMBINER_CONTIGUOUS:
            MPII_Dataloop_create_contiguous(ints[0] /* count */ ,
                                            types[0] /* oldtype */ ,
                                            dlp_p);
            break;
        case MPI_COMBINER_VECTOR:
            MPII_Dataloop_create_vector(ints[0] /* count */ ,
                                        ints[1] /* blklen */ ,
                                        ints[2] /* stride */ ,
                                        0 /* stride not bytes */ ,
                                        types[0] /* oldtype */ ,
                                        dlp_p);
            break;
        case MPI_COMBINER_HVECTOR_INTEGER:
        case MPI_COMBINER_HVECTOR:
            /* fortran hvector has integer stride in bytes */
            if (combiner == MPI_COMBINER_HVECTOR_INTEGER) {
                stride = (MPI_Aint) ints[2];
            } else {
                stride = aints[0];
            }

            MPII_Dataloop_create_vector(ints[0] /* count */ ,
                                        ints[1] /* blklen */ ,
                                        stride, 1 /* stride in bytes */ ,
                                        types[0] /* oldtype */ ,
                                        dlp_p);
            break;
        case MPI_COMBINER_INDEXED_BLOCK:
            MPII_Dataloop_create_blockindexed(ints[0] /* count */ ,
                                              ints[1] /* blklen */ ,
                                              &ints[2] /* disps */ ,
                                              0 /* disp not bytes */ ,
                                              types[0] /* oldtype */ ,
                                              dlp_p);
            break;
        case MPI_COMBINER_HINDEXED_BLOCK:
            disps = (MPI_Aint *) MPL_malloc(ints[0] * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
            for (i = 0; i < ints[0]; i++)
                disps[i] = aints[i];
            MPII_Dataloop_create_blockindexed(ints[0] /* count */ ,
                                              ints[1] /* blklen */ ,
                                              disps /* disps */ ,
                                              1 /* disp is bytes */ ,
                                              types[0] /* oldtype */ ,
                                              dlp_p);
            MPL_free(disps);
            break;
        case MPI_COMBINER_INDEXED:
            blklen = (MPI_Aint *) MPL_malloc(ints[0] * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
            for (i = 0; i < ints[0]; i++)
                blklen[i] = ints[1 + i];
            MPII_Dataloop_create_indexed(ints[0] /* count */ ,
                                         blklen /* blklens */ ,
                                         &ints[ints[0] + 1] /* disp */ ,
                                         0 /* disp not in bytes */ ,
                                         types[0] /* oldtype */ ,
                                         dlp_p);
            MPL_free(blklen);
            break;
        case MPI_COMBINER_HINDEXED_INTEGER:
        case MPI_COMBINER_HINDEXED:
            if (combiner == MPI_COMBINER_HINDEXED_INTEGER) {
                disps = (MPI_Aint *) MPL_malloc(ints[0] * sizeof(MPI_Aint), MPL_MEM_DATATYPE);

                for (i = 0; i < ints[0]; i++) {
                    disps[i] = (MPI_Aint) ints[ints[0] + 1 + i];
                }
            } else {
                disps = aints;
            }

            blklen = (MPI_Aint *) MPL_malloc(ints[0] * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
            for (i = 0; i < ints[0]; i++)
                blklen[i] = (MPI_Aint) ints[1 + i];
            MPII_Dataloop_create_indexed(ints[0] /* count */ ,
                                         blklen /* blklens */ ,
                                         disps, 1 /* disp in bytes */ ,
                                         types[0] /* oldtype */ ,
                                         dlp_p);

            if (combiner == MPI_COMBINER_HINDEXED_INTEGER) {
                MPL_free(disps);
            }
            MPL_free(blklen);

            break;
        case MPI_COMBINER_STRUCT_INTEGER:
        case MPI_COMBINER_STRUCT:
            for (i = 1; i < ints[0]; i++) {
                int type_combiner;
                MPIR_Type_get_envelope(types[i], &dummy1, &dummy2, &dummy3, &type_combiner);

                if (type_combiner != MPI_COMBINER_NAMED) {
                    MPII_DATALOOP_GET_LOOPPTR(types[i], old_dlp);
                    if (old_dlp == NULL) {
                        MPIR_Dataloop_create(types[i], (void **) &old_dlp);

                        MPII_DATALOOP_SET_LOOPPTR(types[i], old_dlp);
                    }
                }
            }
            if (combiner == MPI_COMBINER_STRUCT_INTEGER) {
                disps = (MPI_Aint *) MPL_malloc(ints[0] * sizeof(MPI_Aint), MPL_MEM_DATATYPE);

                for (i = 0; i < ints[0]; i++) {
                    disps[i] = (MPI_Aint) ints[ints[0] + 1 + i];
                }
            } else {
                disps = aints;
            }

            err = MPII_Dataloop_create_struct(ints[0] /* count */ ,
                                              &ints[1] /* blklens */ ,
                                              disps, types /* oldtype array */ ,
                                              dlp_p);
            /* TODO if/when this function returns error codes, propagate this failure instead */
            MPIR_Assert(0 == err);
            /* if (err) return err; */

            if (combiner == MPI_COMBINER_STRUCT_INTEGER) {
                MPL_free(disps);
            }
            break;
        case MPI_COMBINER_SUBARRAY:
            ndims = ints[0];
            MPII_Dataloop_convert_subarray(ndims, &ints[1] /* sizes */ ,
                                           &ints[1 + ndims] /* subsizes */ ,
                                           &ints[1 + 2 * ndims] /* starts */ ,
                                           ints[1 + 3 * ndims] /* order */ ,
                                           types[0], &tmptype);

            MPIR_Dataloop_create(tmptype, (void **) dlp_p);

            MPIR_Type_free_impl(&tmptype);
            break;
        case MPI_COMBINER_DARRAY:
            ndims = ints[2];
            MPII_Dataloop_convert_darray(ints[0] /* size */ ,
                                         ints[1] /* rank */ ,
                                         ndims, &ints[3] /* gsizes */ ,
                                         &ints[3 + ndims] /*distribs */ ,
                                         &ints[3 + 2 * ndims] /* dargs */ ,
                                         &ints[3 + 3 * ndims] /* psizes */ ,
                                         ints[3 + 4 * ndims] /* order */ ,
                                         types[0], &tmptype);

            MPIR_Dataloop_create(tmptype, (void **) dlp_p);

            MPIR_Type_free_impl(&tmptype);
            break;
        default:
            MPIR_Assert(0);
            break;
    }

  clean_exit:

    MPIR_Type_release_contents(type, &ints, &aints, &types);

    /* for now we just leave the intermediate dataloops in place.
     * could remove them to save space if we wanted.
     */

    return;
}

/*@
  create_named - create a dataloop for a "named" type
  if necessary.

  "named" types are ones for which MPI_Type_get_envelope() returns a
  combiner of MPI_COMBINER_NAMED. some types that fit this category,
  such as MPI_SHORT_INT, have multiple elements with potential gaps
  and padding. these types need dataloops for correct processing.
@*/
static void create_named(MPI_Datatype type, MPII_Dataloop ** dlp_p)
{
    MPII_Dataloop *dlp;

    /* special case: pairtypes need dataloops too.
     *
     * note: not dealing with MPI_2INT because size == extent
     *       in all cases for that type.
     *
     * note: MPICH always precreates these, so we will never call
     *       create_pairtype() from here in the MPICH
     *       case.
     */
    if (type == MPI_FLOAT_INT || type == MPI_DOUBLE_INT ||
        type == MPI_LONG_INT || type == MPI_SHORT_INT || type == MPI_LONG_DOUBLE_INT) {
        MPII_DATALOOP_GET_LOOPPTR(type, dlp);
        if (dlp != NULL) {
            /* dataloop already created; just return it. */
            *dlp_p = dlp;
        } else {
            create_pairtype(type, dlp_p);
        }
        return;
    }
    /* no other combiners need dataloops; exit. */
    else {
        *dlp_p = NULL;
        return;
    }
}
