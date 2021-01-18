/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop_internal.h"

static int create_struct_memory_error(void);
static int create_unique_type_struct(MPI_Aint count,
                                     const MPI_Aint * blklens,
                                     const MPI_Aint * disps,
                                     const MPI_Datatype * oldtypes,
                                     int type_pos, MPII_Dataloop ** dlp_p);
static int create_basic_all_bytes_struct(MPI_Aint count,
                                         const MPI_Aint * blklens,
                                         const MPI_Aint * disps,
                                         const MPI_Datatype * oldtypes, MPII_Dataloop ** dlp_p);
static int create_flattened_struct(MPI_Aint count,
                                   const MPI_Aint * blklens,
                                   const MPI_Aint * disps,
                                   const MPI_Datatype * oldtypes, MPII_Dataloop ** dlp_p);

/*@
  Dataloop_create_struct - create the dataloop representation for a
  struct datatype

Input Parameters:
+ count - number of blocks in vector
. blklens - number of elements in each block
. disps - offsets of blocks from start of type in bytes
- oldtypes - types (using handle) of datatypes on which vector is based

Output Parameters:
+ dlp_p - pointer to address in which to place pointer to new dataloop

  Return Value:
  0 on success, -1 on failure.

  Notes:
  This function relies on others, like Dataloop_create_indexed, to create
  types in some cases. This call (like all the rest) takes int blklens
  and MPI_Aint displacements, so it's possible to overflow when working
  with a particularly large struct type in some cases. This isn't detected
  or corrected in this code at this time.

@*/
int MPIR_Dataloop_create_struct(MPI_Aint count,
                                const MPI_Aint * blklens,
                                const MPI_Aint * disps, const MPI_Datatype * oldtypes, void **dlp_p)
{
    int err, i, nr_basics = 0, nr_derived = 0, type_pos = 0;

    MPI_Datatype first_basic = MPI_DATATYPE_NULL, first_derived = MPI_DATATYPE_NULL;

    /* if count is zero, handle with contig code, call it a int */
    if (count == 0) {
        err = MPIR_Dataloop_create_contiguous(0, MPI_INT, (void **) dlp_p);
        return err;
    }

    /* browse the old types and characterize */
    for (i = 0; i < count; i++) {
        /* ignore type elements with a zero blklen */
        if (blklens[i] == 0)
            continue;

        int is_builtin;

        is_builtin = (MPII_DATALOOP_HANDLE_HASLOOP(oldtypes[i])) ? 0 : 1;

        if (is_builtin) {
            if (nr_basics == 0) {
                first_basic = oldtypes[i];
                type_pos = i;
            } else if (oldtypes[i] != first_basic) {
                first_basic = MPI_DATATYPE_NULL;
            }
            nr_basics++;
        } else {        /* derived type */

            if (nr_derived == 0) {
                first_derived = oldtypes[i];
                type_pos = i;
            } else if (oldtypes[i] != first_derived) {
                first_derived = MPI_DATATYPE_NULL;
            }
            nr_derived++;
        }
    }

    /* note on optimizations:
     *
     * because LB, UB, and extent calculations are handled as part of
     * the Datatype, we can safely ignore them in all our calculations
     * here.
     */

    if (nr_basics == 0 && nr_derived == 0) {
        err = MPIR_Dataloop_create_contiguous(0, MPI_INT, (void **) dlp_p);
        return err;
    }

    /* optimization:
     *
     * if there is only one unique instance of a type in the struct, treat it
     * as a blockindexed type.
     *
     * notes:
     *
     * if the displacement happens to be zero, the blockindexed code will
     * optimize this into a contig.
     */
    if (nr_basics + nr_derived == 1) {
        /* type_pos is index to only real type in array */
        err = MPIR_Dataloop_create_blockindexed(1,      /* count */
                                                blklens[type_pos], &disps[type_pos], 1, /* displacement in bytes */
                                                oldtypes[type_pos], (void **) dlp_p);

        return err;
    }

    /* optimization:
     *
     * if there only one unique type (more than one instance) in the
     * struct, treat it as an indexed type.
     *
     * notes:
     *
     * this will apply to a single type with an LB/UB, as those
     * are handled elsewhere.
     *
     */
    if (((nr_derived == 0) && (first_basic != MPI_DATATYPE_NULL)) ||
        ((nr_basics == 0) && (first_derived != MPI_DATATYPE_NULL))) {
        return create_unique_type_struct(count, blklens, disps, oldtypes, type_pos,
                                         (MPII_Dataloop **) dlp_p);
    }

    /* optimization:
     *
     * if there are no derived types, convert
     * everything to bytes and use an indexed type.
     */
    if (nr_derived == 0) {
        return create_basic_all_bytes_struct(count, blklens, disps, oldtypes,
                                             (MPII_Dataloop **) dlp_p);
    }

    /* optimization:
     * flatten the type and store it as an indexed type so that
     * there are no branches in the dataloop tree.
     */
    return create_flattened_struct(count, blklens, disps, oldtypes, (MPII_Dataloop **) dlp_p);
}

/* --BEGIN ERROR HANDLING-- */
static int create_struct_memory_error(void)
{
    return -1;
}

/* --END ERROR HANDLING-- */

static int create_unique_type_struct(MPI_Aint count,
                                     const MPI_Aint * blklens,
                                     const MPI_Aint * disps,
                                     const MPI_Datatype * oldtypes,
                                     int type_pos, MPII_Dataloop ** dlp_p)
{
    /* the same type used more than once in the array; type_pos
     * indexes to the first of these.
     */
    int i, err, cur_pos = 0;
    MPI_Aint *tmp_blklens;
    MPI_Aint *tmp_disps;

    /* count is an upper bound on number of type instances */
    tmp_blklens = (MPI_Aint *) MPL_malloc(count * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_blklens) {
        /* TODO: ??? */
        return create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    tmp_disps = (MPI_Aint *)
        MPL_malloc(count * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_disps) {
        MPL_free(tmp_blklens);
        /* TODO: ??? */
        return create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    for (i = type_pos; i < count; i++) {
        if (oldtypes[i] == oldtypes[type_pos] && blklens != 0) {
            tmp_blklens[cur_pos] = blklens[i];
            tmp_disps[cur_pos] = disps[i];
            cur_pos++;
        }
    }

    err = MPIR_Dataloop_create_indexed(cur_pos, tmp_blklens, tmp_disps, 1,      /* disp in bytes */
                                       oldtypes[type_pos], (void **) dlp_p);

    MPL_free(tmp_blklens);
    MPL_free(tmp_disps);

    return err;

}

static int create_basic_all_bytes_struct(MPI_Aint count,
                                         const MPI_Aint * blklens,
                                         const MPI_Aint * disps,
                                         const MPI_Datatype * oldtypes, MPII_Dataloop ** dlp_p)
{
    int i, err, cur_pos = 0;
    MPI_Aint *tmp_blklens;
    MPI_Aint *tmp_disps;

    /* count is an upper bound on number of type instances */
    tmp_blklens = (MPI_Aint *) MPL_malloc(count * sizeof(MPI_Aint), MPL_MEM_DATATYPE);

    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_blklens) {
        return create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    tmp_disps = (MPI_Aint *) MPL_malloc(count * sizeof(MPI_Aint), MPL_MEM_DATATYPE);

    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_disps) {
        MPL_free(tmp_blklens);
        return create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    for (i = 0; i < count; i++) {
        if (blklens[i] != 0) {
            MPI_Aint sz;

            MPIR_Datatype_get_size_macro(oldtypes[i], sz);
            tmp_blklens[cur_pos] = (int) sz *blklens[i];
            tmp_disps[cur_pos] = disps[i];
            cur_pos++;
        }
    }
    err = MPIR_Dataloop_create_indexed(cur_pos, tmp_blklens, tmp_disps, 1,      /* disp in bytes */
                                       MPI_BYTE, (void **) dlp_p);

    MPL_free(tmp_blklens);
    MPL_free(tmp_disps);

    return err;
}

static int create_flattened_struct(MPI_Aint count,
                                   const MPI_Aint * blklens,
                                   const MPI_Aint * disps,
                                   const MPI_Datatype * oldtypes, MPII_Dataloop ** dlp_p)
{
    /* arbitrary types, convert to bytes and use indexed */
    int i, err, nr_blks = 0;
    MPI_Aint *tmp_blklens;
    MPI_Aint *tmp_disps;        /* since we're calling another fn that takes
                                 * this type as an input parameter */
    MPI_Aint bytes;
    MPIR_Segment *segp;

    int first_ind;
    MPI_Aint last_ind;

    /* use segment code once to count contiguous regions */
    for (i = 0; i < count; i++) {
        int is_basic;

        /* ignore type elements with a zero blklen */
        if (blklens[i] == 0)
            continue;

        is_basic = (MPII_DATALOOP_HANDLE_HASLOOP(oldtypes[i])) ? 0 : 1;

        if (is_basic) {
            nr_blks++;
        } else {        /* derived type; get a count of contig blocks */

            MPI_Aint tmp_nr_blks, sz;

            MPIR_Datatype_get_size_macro(oldtypes[i], sz);

            /* if the derived type has some data to contribute,
             * add to flattened representation */
            if (sz > 0) {
                segp = MPIR_Segment_alloc(NULL, (MPI_Aint) blklens[i], oldtypes[i]);
                if (!segp) {
                    return create_struct_memory_error();
                }

                bytes = MPIR_SEGMENT_IGNORE_LAST;

                MPIR_Segment_count_contig_blocks(segp, 0, &bytes, &tmp_nr_blks);
                MPIR_Segment_free(segp);

                nr_blks += tmp_nr_blks;
            }
        }
    }

    /* it's possible for us to get to this point only to realize that
     * there isn't any data in this type. in that case do what we always
     * do: store a simple contig of zero ints and call it done.
     */
    if (nr_blks == 0) {
        err = MPIR_Dataloop_create_contiguous(0, MPI_INT, (void **) dlp_p);
        return err;

    }

    nr_blks += 2;       /* safety measure */

    tmp_blklens = (MPI_Aint *) MPL_malloc(nr_blks * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_blklens) {
        return create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */


    tmp_disps = (MPI_Aint *) MPL_malloc(nr_blks * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_disps) {
        MPL_free(tmp_blklens);
        return create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    /* use segment code again to flatten the type */
    first_ind = 0;
    for (i = 0; i < count; i++) {
        int is_basic;
        MPI_Aint sz = -1;

        is_basic = (MPII_DATALOOP_HANDLE_HASLOOP(oldtypes[i])) ? 0 : 1;
        if (!is_basic)
            MPIR_Datatype_get_size_macro(oldtypes[i], sz);

        /* we're going to use the segment code to flatten the type.
         * we put in our displacement as the buffer location, and use
         * the blocklength as the count value to get N contiguous copies
         * of the type.
         *
         * Note that we're going to get back values in bytes, so that will
         * be our new element type.
         */
        if (blklens[i] != 0 && (is_basic || sz > 0)) {
            segp = MPIR_Segment_alloc((char *) disps[i], (MPI_Aint) blklens[i], oldtypes[i]);

            last_ind = nr_blks - first_ind;
            bytes = MPIR_SEGMENT_IGNORE_LAST;
            MPII_Dataloop_segment_flatten(segp,
                                          0,
                                          &bytes,
                                          &tmp_blklens[first_ind], &tmp_disps[first_ind],
                                          &last_ind);
            MPIR_Segment_free(segp);
            first_ind += last_ind;
        }
    }
    nr_blks = first_ind;

#if 0
    if (MPL_DBG_SELECTED(MPIR_DBG_DATATYPE, VERBOSE)) {
        MPL_DBG_OUT(MPIR_DBG_DATATYPE, "--- start of flattened type ---");
        for (i = 0; i < nr_blks; i++) {
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                "a[%d] = (%d, " MPI_AINT_FMT_DEC_SPEC ")", i,
                                                tmp_blklens[i], tmp_disps[i]));
        }
        MPL_DBG_OUT(MPIR_DBG_DATATYPE, "--- end of flattened type ---");
    }
#endif

    err = MPIR_Dataloop_create_indexed(nr_blks, tmp_blklens, tmp_disps, 1,      /* disp in bytes */
                                       MPI_BYTE, (void **) dlp_p);

    MPL_free(tmp_blklens);
    MPL_free(tmp_disps);

    return err;
}
