/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop.h"
#include "typerep_internal.h"

#include <stdlib.h>
#include <limits.h>

#define PAIRTYPE_CONTENTS(mt1_,ut1_,mt2_,ut2_)                          \
    {                                                                   \
        struct { ut1_ a; ut2_ b; } foo;                                 \
        disps[0] = 0;                                                   \
        disps[1] = (MPI_Aint) ((char *) &foo.b - (char *) &foo.a);      \
        types[0] = mt1_;                                                \
        types[1] = mt2_;                                                \
    }

#define PAIRTYPE_GET_NUM_CONTIG_BLOCKS(ctype1, ctype2, num_contig_blocks)        \
    do {                                                                \
        struct {                                                        \
            ctype1 x;                                                   \
            ctype2 y;                                                   \
        } z;                                                            \
        num_contig_blocks = (sizeof(z.x) + sizeof(z.y) == sizeof(z)) ? 1 : 2; \
    } while (0)

/*@
create_pairtype - create dataloop for a pairtype

   Arguments:
+  MPI_Datatype type - the pairtype

.N Errors
.N Returns 0 on success, -1 on failure.

   Note:
   This function simply creates the appropriate input parameters for
   use with Dataloop_create_struct and then calls that function.

   This same function could be used to create dataloops for any type
   that actually consists of two distinct elements.
@*/
static int create_pairtype(MPI_Datatype type)
{
    MPI_Aint blocks[2] = { 1, 1 };
    MPI_Aint disps[2];
    MPI_Datatype types[2];

    MPIR_Assert(type == MPI_FLOAT_INT || type == MPI_DOUBLE_INT ||
                type == MPI_LONG_INT || type == MPI_SHORT_INT ||
                type == MPI_LONG_DOUBLE_INT || type == MPI_2INT);

    MPIR_Datatype *typeptr;
    MPIR_Datatype_get_ptr(type, typeptr);

    if (type == MPI_FLOAT_INT) {
        PAIRTYPE_CONTENTS(MPI_FLOAT, float, MPI_INT, int);
        PAIRTYPE_GET_NUM_CONTIG_BLOCKS(float, int, typeptr->typerep.num_contig_blocks);
    } else if (type == MPI_DOUBLE_INT) {
        PAIRTYPE_CONTENTS(MPI_DOUBLE, double, MPI_INT, int);
        PAIRTYPE_GET_NUM_CONTIG_BLOCKS(double, int, typeptr->typerep.num_contig_blocks);
    } else if (type == MPI_LONG_INT) {
        PAIRTYPE_CONTENTS(MPI_LONG, long, MPI_INT, int);
        PAIRTYPE_GET_NUM_CONTIG_BLOCKS(long, int, typeptr->typerep.num_contig_blocks);
    } else if (type == MPI_SHORT_INT) {
        PAIRTYPE_CONTENTS(MPI_SHORT, short, MPI_INT, int);
        PAIRTYPE_GET_NUM_CONTIG_BLOCKS(short, int, typeptr->typerep.num_contig_blocks);
    } else if (type == MPI_LONG_DOUBLE_INT) {
        PAIRTYPE_CONTENTS(MPI_LONG_DOUBLE, long double, MPI_INT, int);
        PAIRTYPE_GET_NUM_CONTIG_BLOCKS(long double, int, typeptr->typerep.num_contig_blocks);
    } else if (type == MPI_2INT) {
        PAIRTYPE_CONTENTS(MPI_INT, int, MPI_INT, int);
        PAIRTYPE_GET_NUM_CONTIG_BLOCKS(int, int, typeptr->typerep.num_contig_blocks);
    }

    return MPIR_Dataloop_create_struct(2, blocks, disps, types, (void **) &typeptr->typerep.handle);
}

static void create_named(MPI_Datatype type);

void MPIR_Typerep_commit(MPI_Datatype type)
{
    MPI_Datatype *types;
    int *ints;
    MPI_Aint *aints;
    MPI_Aint *counts;

    void *old_dlp;

    MPIR_Datatype *typeptr;
    MPIR_Datatype_get_ptr(type, typeptr);
    void **dlp_p = (void **) &typeptr->typerep.handle;

    if (type == MPI_FLOAT_INT || type == MPI_DOUBLE_INT || type == MPI_LONG_INT ||
        type == MPI_SHORT_INT || type == MPI_LONG_DOUBLE_INT || type == MPI_2INT) {
        create_pairtype(type);
        return;
    }

    int combiner = MPIR_Type_get_combiner(type);

    /* some named types do need dataloops; handle separately. */
    if (combiner == MPI_COMBINER_NAMED) {
        create_named(type);
        return;
    } else if (combiner == MPI_COMBINER_F90_REAL || combiner == MPI_COMBINER_F90_COMPLEX ||
               combiner == MPI_COMBINER_F90_INTEGER) {
        MPI_Datatype f90basetype;
        MPIR_Datatype_get_basic_type(type, f90basetype);
        MPIR_Dataloop_create_contiguous(1, f90basetype, (void **) dlp_p);
        return;
    }

    MPIR_DATALOOP_GET_LOOPPTR(type, old_dlp);
    if (old_dlp != NULL) {
        /* dataloop already created; just return it. */
        *dlp_p = old_dlp;
        return;
    }

    MPIR_Datatype_contents *cp = typeptr->contents;
    MPIR_Datatype_access_contents(cp, &ints, &aints, &counts, &types);

    /* first check for zero count on types where that makes sense */
    switch (combiner) {
        case MPI_COMBINER_CONTIGUOUS:
        case MPI_COMBINER_VECTOR:
        case MPI_COMBINER_HVECTOR:
        case MPI_COMBINER_INDEXED_BLOCK:
        case MPI_COMBINER_HINDEXED_BLOCK:
        case MPI_COMBINER_INDEXED:
        case MPI_COMBINER_HINDEXED:
        case MPI_COMBINER_STRUCT:
            /* check ints[0] for normal types and counts[0] for large count types */
            if ((cp->nr_ints > 0 && ints[0] == 0) || (cp->nr_counts > 0 && counts[0] == 0)) {
                MPIR_Dataloop_create_contiguous(0, MPI_INT, (void **) dlp_p);
                goto clean_exit;
            }
            break;
        case MPI_COMBINER_HVECTOR_INTEGER:
        case MPI_COMBINER_HINDEXED_INTEGER:
        case MPI_COMBINER_STRUCT_INTEGER:
            MPIR_Assert(0 && "wrong combiner");
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
    int type0_combiner = MPIR_Type_get_combiner(types[0]);
    if (type0_combiner != MPI_COMBINER_NAMED) {
        MPIR_DATALOOP_GET_LOOPPTR(types[0], old_dlp);
        if (old_dlp == NULL) {
            /* no dataloop already present; create and store one */
            MPIR_Typerep_commit(types[0]);
            MPIR_DATALOOP_GET_LOOPPTR(types[0], old_dlp);
            assert(old_dlp);
        }
    }

    switch (combiner) {
        case MPI_COMBINER_DUP:
            if (type0_combiner != MPI_COMBINER_NAMED) {
                MPIR_Dataloop_dup(old_dlp, (void **) dlp_p);
            } else if (cp->nr_counts == 0) {
                MPIR_Dataloop_create_contiguous(1, types[0], (void **) dlp_p);
            } else {
                MPIR_Assert(0);
            }
            break;
        case MPI_COMBINER_RESIZED:
            if (type0_combiner != MPI_COMBINER_NAMED) {
                MPIR_Dataloop_dup(old_dlp, (void **) dlp_p);
            } else if (cp->nr_counts == 0) {
                MPIR_Dataloop_create_resized(types[0], aints[1], (void **) dlp_p);
            } else {
                MPIR_Dataloop_create_resized(types[0], counts[1], (void **) dlp_p);
            }
            break;
        case MPI_COMBINER_CONTIGUOUS:
            if (cp->nr_counts == 0) {
                MPIR_Dataloop_create_contiguous(ints[0], types[0], (void **) dlp_p);
            } else {
                MPIR_Dataloop_create_contiguous(counts[0], types[0], (void **) dlp_p);
            }
            break;
        case MPI_COMBINER_VECTOR:
            if (cp->nr_counts == 0) {
                MPIR_Dataloop_create_vector(ints[0], ints[1], ints[2], 0, types[0],
                                            (void **) dlp_p);
            } else {
                MPIR_Dataloop_create_vector(counts[0], counts[1], counts[2], 0, types[0],
                                            (void **) dlp_p);
            }
            break;
        case MPI_COMBINER_HVECTOR:
            if (cp->nr_counts == 0) {
                MPIR_Dataloop_create_vector(ints[0], ints[1], aints[0], 1, types[0],
                                            (void **) dlp_p);
            } else {
                MPIR_Dataloop_create_vector(counts[0], counts[1], counts[2], 1, types[0],
                                            (void **) dlp_p);
            }
            break;
        case MPI_COMBINER_INDEXED_BLOCK:
            if (cp->nr_counts == 0) {
                MPI_Aint *disps = MPL_malloc(ints[0] * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
                MPIR_Assert(disps);
                for (int i = 0; i < ints[0]; i++) {
                    disps[i] = ints[2 + i];
                }
                MPIR_Dataloop_create_blockindexed(ints[0], ints[1], disps, 0, types[0],
                                                  (void **) dlp_p);
                MPL_free(disps);
            } else {
                MPIR_Dataloop_create_blockindexed(counts[0], counts[1], &counts[2], 0, types[0],
                                                  (void **) dlp_p);
            }
            break;
        case MPI_COMBINER_HINDEXED_BLOCK:
            if (cp->nr_counts == 0) {
                MPIR_Dataloop_create_blockindexed(ints[0], ints[1], aints, 1, types[0],
                                                  (void **) dlp_p);
            } else {
                MPIR_Dataloop_create_blockindexed(counts[0], counts[1], &counts[2], 1, types[0],
                                                  (void **) dlp_p);
            }
            break;
        case MPI_COMBINER_INDEXED:
            if (cp->nr_counts == 0) {
                int n = ints[0];
                MPI_Aint *blkls = MPL_malloc(n * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
                MPI_Aint *disps = MPL_malloc(n * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
                for (int i = 0; i < n; i++) {
                    blkls[i] = ints[1 + i];
                    disps[i] = ints[1 + n + i];
                }
                MPIR_Dataloop_create_indexed(n, blkls, disps, 0, types[0], (void **) dlp_p);
                MPL_free(blkls);
                MPL_free(disps);
            } else {
                int n = counts[0];
                MPI_Aint *blkls = counts + 1;
                MPI_Aint *disps = counts + 1 + n;
                MPIR_Dataloop_create_indexed(n, blkls, disps, 0, types[0], (void **) dlp_p);
            }
            break;
        case MPI_COMBINER_HINDEXED:
            if (cp->nr_counts == 0) {
                int n = ints[0];
                MPI_Aint *blkls = MPL_malloc(n * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
                MPI_Aint *disps = aints;
                for (int i = 0; i < n; i++) {
                    blkls[i] = ints[1 + i];
                }
                MPIR_Dataloop_create_indexed(n, blkls, disps, 1, types[0], (void **) dlp_p);

                MPL_free(blkls);
            } else {
                int n = counts[0];
                MPI_Aint *blkls = counts + 1;
                MPI_Aint *disps = counts + 1 + n;
                MPIR_Dataloop_create_indexed(n, blkls, disps, 1, types[0], (void **) dlp_p);
            }
            break;
        case MPI_COMBINER_STRUCT:
            {
                int n;
                if (cp->nr_counts == 0) {
                    n = ints[0];
                } else {
                    n = counts[0];
                }
                /* commit each subtypes */
                for (MPI_Aint i = 1; i < n; i++) {
                    int type_combiner = MPIR_Type_get_combiner(types[i]);

                    if (type_combiner != MPI_COMBINER_NAMED) {
                        MPIR_DATALOOP_GET_LOOPPTR(types[i], old_dlp);
                        if (old_dlp == NULL) {
                            MPIR_Typerep_commit(types[i]);
                            MPIR_DATALOOP_GET_LOOPPTR(types[i], old_dlp);
                            assert(old_dlp);
                        }
                    }
                }
                if (cp->nr_counts == 0) {
                    MPI_Aint *blkls = MPL_malloc(n * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
                    for (int i = 0; i < n; i++) {
                        blkls[i] = ints[1 + i];
                    }
                    MPI_Aint *disps = aints;
                    int err = MPIR_Dataloop_create_struct(n, blkls, disps, types, (void **) dlp_p);
                    MPIR_Assertp(0 == err);
                    MPL_free(blkls);
                } else {
                    MPI_Aint *blkls = counts + 1;
                    MPI_Aint *disps = counts + 1 + n;
                    int err = MPIR_Dataloop_create_struct(n, blkls, disps, types, (void **) dlp_p);
                    MPIR_Assertp(0 == err);
                }
            }
            break;
        case MPI_COMBINER_SUBARRAY:
            {
                MPI_Datatype tmptype;
                if (cp->nr_counts == 0) {
                    int n = ints[0];
                    MPI_Aint *p_sizes = MPL_malloc(n * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
                    MPI_Aint *p_subsizes = MPL_malloc(n * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
                    MPI_Aint *p_starts = MPL_malloc(n * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
                    for (int i = 0; i < n; i++) {
                        p_sizes[i] = ints[1 + i];
                        p_subsizes[i] = ints[1 + n + i];
                        p_starts[i] = ints[1 + 2 * n + i];
                    }
                    MPII_Typerep_convert_subarray(n, p_sizes, p_subsizes, p_starts,
                                                  ints[1 + 3 * n], types[0], &tmptype);
                    MPL_free(p_sizes);
                    MPL_free(p_subsizes);
                    MPL_free(p_starts);
                } else {
                    int n = ints[0];
                    MPI_Aint *p_sizes = counts;
                    MPI_Aint *p_subsizes = counts + n;
                    MPI_Aint *p_starts = counts + n * 2;
                    MPII_Typerep_convert_subarray(n, p_sizes, p_subsizes, p_starts,
                                                  ints[1], types[0], &tmptype);
                }
                MPIR_Typerep_commit(tmptype);

                {
                    void *tmploop;
                    MPIR_DATALOOP_GET_LOOPPTR(tmptype, tmploop);
                    MPIR_Dataloop_dup(tmploop, dlp_p);
                }

                MPIR_Type_free_impl(&tmptype);
            }
            break;
        case MPI_COMBINER_DARRAY:
            {
                MPI_Datatype tmptype;
                if (cp->nr_counts == 0) {
                    int n = ints[2];
                    MPI_Aint *p_gsizes = MPL_malloc(n * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
                    for (int i = 0; i < n; i++) {
                        p_gsizes[i] = ints[3 + i];
                    }
                    MPII_Typerep_convert_darray(ints[0], ints[1], n, p_gsizes, &ints[3 + n],
                                                &ints[3 + 2 * n], &ints[3 + 3 * n],
                                                ints[3 + 4 * n], types[0], &tmptype);
                    MPL_free(p_gsizes);
                } else {
                    int n = ints[2];
                    MPII_Typerep_convert_darray(ints[0], ints[1], n, counts, &ints[3],
                                                &ints[3 + n], &ints[3 + 2 * n],
                                                ints[3 + 3 * n], types[0], &tmptype);
                }
                MPIR_Typerep_commit(tmptype);

                {
                    void *tmploop;
                    MPIR_DATALOOP_GET_LOOPPTR(tmptype, tmploop);
                    MPIR_Dataloop_dup(tmploop, dlp_p);
                }

                MPIR_Type_free_impl(&tmptype);
            }
            break;
        default:
            MPIR_Assert(0);
            break;
    }

  clean_exit:
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
static void create_named(MPI_Datatype type)
{
    /* special case: pairtypes need dataloops too.
     *
     * note: not dealing with MPI_2INT because size == extent
     *       in all cases for that type.
     *
     * note: MPICH always precreates these, so we will never call
     *       create_pairtype() from here in the MPICH
     *       case.
     */
    if (type == MPI_FLOAT_INT || type == MPI_DOUBLE_INT || type == MPI_LONG_INT ||
        type == MPI_SHORT_INT || type == MPI_LONG_DOUBLE_INT) {
        void *dlp;
        MPIR_DATALOOP_GET_LOOPPTR(type, dlp);
        if (dlp != NULL) {
            /* dataloop already created */
        } else {
            create_pairtype(type);
        }
        return;
    }
    /* no other combiners need dataloops; exit. */
    else {
        return;
    }
}

void MPIR_Typerep_free(MPIR_Datatype * typeptr)
{
    MPIR_Dataloop_free(&typeptr->typerep.handle);
}
