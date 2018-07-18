/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

static int DLOOP_Dataloop_create_struct_memory_error(void);
static int DLOOP_Dataloop_create_unique_type_struct(DLOOP_Count count,
                                                    const int *blklens,
                                                    const MPI_Aint * disps,
                                                    const DLOOP_Type * oldtypes,
                                                    int type_pos,
                                                    DLOOP_Dataloop ** dlp_p,
                                                    MPI_Aint * dlsz_p, int *dldepth_p, int flag);
static int DLOOP_Dataloop_create_basic_all_bytes_struct(DLOOP_Count count,
                                                        const int *blklens,
                                                        const MPI_Aint * disps,
                                                        const DLOOP_Type * oldtypes,
                                                        DLOOP_Dataloop ** dlp_p,
                                                        MPI_Aint * dlsz_p,
                                                        int *dldepth_p, int flag);
static int DLOOP_Dataloop_create_flattened_struct(DLOOP_Count count,
                                                  const int *blklens,
                                                  const MPI_Aint * disps,
                                                  const DLOOP_Type * oldtypes,
                                                  DLOOP_Dataloop ** dlp_p,
                                                  MPI_Aint * dlsz_p, int *dldepth_p, int flag);

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
- dlsz_p - pointer to address in which to place size of new dataloop

  Return Value:
  0 on success, -1 on failure.

  Notes:
  This function relies on others, like Dataloop_create_indexed, to create
  types in some cases. This call (like all the rest) takes int blklens
  and MPI_Aint displacements, so it's possible to overflow when working
  with a particularly large struct type in some cases. This isn't detected
  or corrected in this code at this time.

@*/
int MPIR_Dataloop_create_struct(DLOOP_Count count,
                                const int *blklens,
                                const MPI_Aint * disps,
                                const DLOOP_Type * oldtypes,
                                DLOOP_Dataloop ** dlp_p,
                                MPI_Aint * dlsz_p, int *dldepth_p, int flag)
{
    int err, i, nr_basics = 0, nr_derived = 0, type_pos = 0;

    DLOOP_Type first_basic = MPI_DATATYPE_NULL, first_derived = MPI_DATATYPE_NULL;

    /* if count is zero, handle with contig code, call it a int */
    if (count == 0) {
        err = MPIR_Dataloop_create_contiguous(0, MPI_INT, dlp_p, dlsz_p, dldepth_p, flag);
        return err;
    }

    /* browse the old types and characterize */
    for (i = 0; i < count; i++) {
        /* ignore type elements with a zero blklen */
        if (blklens[i] == 0)
            continue;

        if (oldtypes[i] != MPI_LB && oldtypes[i] != MPI_UB) {
            int is_builtin;

            is_builtin = (DLOOP_Handle_hasloop_macro(oldtypes[i])) ? 0 : 1;

            if (is_builtin) {
                if (nr_basics == 0) {
                    first_basic = oldtypes[i];
                    type_pos = i;
                } else if (oldtypes[i] != first_basic) {
                    first_basic = MPI_DATATYPE_NULL;
                }
                nr_basics++;
            } else {    /* derived type */

                if (nr_derived == 0) {
                    first_derived = oldtypes[i];
                    type_pos = i;
                } else if (oldtypes[i] != first_derived) {
                    first_derived = MPI_DATATYPE_NULL;
                }
                nr_derived++;
            }
        }
    }

    /* note on optimizations:
     *
     * because LB, UB, and extent calculations are handled as part of
     * the Datatype, we can safely ignore them in all our calculations
     * here.
     */

    /* optimization:
     *
     * if there were only MPI_LBs and MPI_UBs in the struct type,
     * treat it as a zero-element contiguous (just as count == 0).
     */
    if (nr_basics == 0 && nr_derived == 0) {
        err = MPIR_Dataloop_create_contiguous(0, MPI_INT, dlp_p, dlsz_p, dldepth_p, flag);
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
                                                oldtypes[type_pos], dlp_p, dlsz_p, dldepth_p, flag);

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
        return DLOOP_Dataloop_create_unique_type_struct(count,
                                                        blklens,
                                                        disps,
                                                        oldtypes,
                                                        type_pos, dlp_p, dlsz_p, dldepth_p, flag);
    }

    /* optimization:
     *
     * if there are no derived types, convert
     * everything to bytes and use an indexed type.
     */
    if (nr_derived == 0) {
        return DLOOP_Dataloop_create_basic_all_bytes_struct(count,
                                                            blklens,
                                                            disps,
                                                            oldtypes,
                                                            dlp_p, dlsz_p, dldepth_p, flag);
    }

    /* optimization:
     * flatten the type and store it as an indexed type so that
     * there are no branches in the dataloop tree.
     */
    return DLOOP_Dataloop_create_flattened_struct(count,
                                                  blklens,
                                                  disps, oldtypes, dlp_p, dlsz_p, dldepth_p, flag);
}

/* --BEGIN ERROR HANDLING-- */
static int DLOOP_Dataloop_create_struct_memory_error(void)
{
    return -1;
}

/* --END ERROR HANDLING-- */

static int DLOOP_Dataloop_create_unique_type_struct(DLOOP_Count count,
                                                    const int *blklens,
                                                    const MPI_Aint * disps,
                                                    const DLOOP_Type * oldtypes,
                                                    int type_pos,
                                                    DLOOP_Dataloop ** dlp_p,
                                                    MPI_Aint * dlsz_p, int *dldepth_p, int flag)
{
    /* the same type used more than once in the array; type_pos
     * indexes to the first of these.
     */
    int i, err, cur_pos = 0;
    DLOOP_Size *tmp_blklens;
    DLOOP_Offset *tmp_disps;

    /* count is an upper bound on number of type instances */
    tmp_blklens = (DLOOP_Size *) DLOOP_Malloc(count * sizeof(DLOOP_Size), MPL_MEM_DATATYPE);
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_blklens) {
        /* TODO: ??? */
        return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    tmp_disps = (DLOOP_Offset *)
        DLOOP_Malloc(count * sizeof(DLOOP_Offset), MPL_MEM_DATATYPE);
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_disps) {
        DLOOP_Free(tmp_blklens);
        /* TODO: ??? */
        return DLOOP_Dataloop_create_struct_memory_error();
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
                                       oldtypes[type_pos], dlp_p, dlsz_p, dldepth_p, flag);

    DLOOP_Free(tmp_blklens);
    DLOOP_Free(tmp_disps);

    return err;

}

static int DLOOP_Dataloop_create_basic_all_bytes_struct(DLOOP_Count count,
                                                        const int *blklens,
                                                        const MPI_Aint * disps,
                                                        const DLOOP_Type * oldtypes,
                                                        DLOOP_Dataloop ** dlp_p,
                                                        MPI_Aint * dlsz_p, int *dldepth_p, int flag)
{
    int i, err, cur_pos = 0;
    DLOOP_Size *tmp_blklens;
    MPI_Aint *tmp_disps;

    /* count is an upper bound on number of type instances */
    tmp_blklens = (DLOOP_Size *) DLOOP_Malloc(count * sizeof(DLOOP_Size), MPL_MEM_DATATYPE);

    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_blklens) {
        return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    tmp_disps = (MPI_Aint *) DLOOP_Malloc(count * sizeof(MPI_Aint), MPL_MEM_DATATYPE);

    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_disps) {
        DLOOP_Free(tmp_blklens);
        return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    for (i = 0; i < count; i++) {
        if (oldtypes[i] != MPI_LB && oldtypes[i] != MPI_UB && blklens[i] != 0) {
            DLOOP_Offset sz;

            DLOOP_Handle_get_size_macro(oldtypes[i], sz);
            tmp_blklens[cur_pos] = (int) sz *blklens[i];
            tmp_disps[cur_pos] = disps[i];
            cur_pos++;
        }
    }
    err = MPIR_Dataloop_create_indexed(cur_pos, tmp_blklens, tmp_disps, 1,      /* disp in bytes */
                                       MPI_BYTE, dlp_p, dlsz_p, dldepth_p, flag);

    DLOOP_Free(tmp_blklens);
    DLOOP_Free(tmp_disps);

    return err;
}

static int DLOOP_Dataloop_create_flattened_struct(DLOOP_Count count,
                                                  const int *blklens,
                                                  const MPI_Aint * disps,
                                                  const DLOOP_Type * oldtypes,
                                                  DLOOP_Dataloop ** dlp_p,
                                                  MPI_Aint * dlsz_p, int *dldepth_p, int flag)
{
    /* arbitrary types, convert to bytes and use indexed */
    int i, err, nr_blks = 0;
    DLOOP_Size *tmp_blklens;
    MPI_Aint *tmp_disps;        /* since we're calling another fn that takes
                                 * this type as an input parameter */
    DLOOP_Offset bytes;
    DLOOP_Segment *segp;

    int first_ind;
    DLOOP_Size last_ind;

    segp = MPIR_Segment_alloc();
    /* --BEGIN ERROR HANDLING-- */
    if (!segp) {
        return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    /* use segment code once to count contiguous regions */
    for (i = 0; i < count; i++) {
        int is_basic;

        /* ignore type elements with a zero blklen */
        if (blklens[i] == 0)
            continue;

        is_basic = (DLOOP_Handle_hasloop_macro(oldtypes[i])) ? 0 : 1;

        if (is_basic && (oldtypes[i] != MPI_LB && oldtypes[i] != MPI_UB)) {
            nr_blks++;
        } else {        /* derived type; get a count of contig blocks */

            DLOOP_Count tmp_nr_blks, sz;

            DLOOP_Handle_get_size_macro(oldtypes[i], sz);

            /* if the derived type has some data to contribute,
             * add to flattened representation */
            if (sz > 0) {
                err = MPIR_Segment_init(NULL, (DLOOP_Count) blklens[i], oldtypes[i], segp);
                if (err)
                    return err;

                bytes = SEGMENT_IGNORE_LAST;

                MPIR_Segment_count_contig_blocks(segp, 0, &bytes, &tmp_nr_blks);

                nr_blks += tmp_nr_blks;
            }
        }
    }

    /* it's possible for us to get to this point only to realize that
     * there isn't any data in this type. in that case do what we always
     * do: store a simple contig of zero ints and call it done.
     */
    if (nr_blks == 0) {
        MPIR_Segment_free(segp);
        err = MPIR_Dataloop_create_contiguous(0, MPI_INT, dlp_p, dlsz_p, dldepth_p, flag);
        return err;

    }

    nr_blks += 2;       /* safety measure */

    tmp_blklens = (DLOOP_Size *) DLOOP_Malloc(nr_blks * sizeof(DLOOP_Size), MPL_MEM_DATATYPE);
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_blklens) {
        MPIR_Segment_free(segp);
        return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */


    tmp_disps = (MPI_Aint *) DLOOP_Malloc(nr_blks * sizeof(MPI_Aint), MPL_MEM_DATATYPE);
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_disps) {
        DLOOP_Free(tmp_blklens);
        MPIR_Segment_free(segp);
        return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    /* use segment code again to flatten the type */
    first_ind = 0;
    for (i = 0; i < count; i++) {
        int is_basic;
        DLOOP_Count sz = -1;

        is_basic = (DLOOP_Handle_hasloop_macro(oldtypes[i])) ? 0 : 1;
        if (!is_basic)
            DLOOP_Handle_get_size_macro(oldtypes[i], sz);

        /* we're going to use the segment code to flatten the type.
         * we put in our displacement as the buffer location, and use
         * the blocklength as the count value to get N contiguous copies
         * of the type.
         *
         * Note that we're going to get back values in bytes, so that will
         * be our new element type.
         */
        if (oldtypes[i] != MPI_UB &&
            oldtypes[i] != MPI_LB && blklens[i] != 0 && (is_basic || sz > 0)) {
            err = MPIR_Segment_init((char *) DLOOP_OFFSET_CAST_TO_VOID_PTR disps[i],
                                    (DLOOP_Count) blklens[i], oldtypes[i], segp);
            if (err)
                return err;

            last_ind = nr_blks - first_ind;
            bytes = SEGMENT_IGNORE_LAST;
            MPIR_Segment_mpi_flatten(segp,
                                     0,
                                     &bytes,
                                     &tmp_blklens[first_ind], &tmp_disps[first_ind], &last_ind);
            first_ind += last_ind;
        }
    }
    nr_blks = first_ind;

#if 0
    if (MPL_DBG_SELECTED(MPIR_DBG_DATATYPE, VERBOSE)) {
        MPL_DBG_OUT(MPIR_DBG_DATATYPE, "--- start of flattened type ---");
        for (i = 0; i < nr_blks; i++) {
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                "a[%d] = (%d, " DLOOP_OFFSET_FMT_DEC_SPEC ")", i,
                                                tmp_blklens[i], tmp_disps[i]));
        }
        MPL_DBG_OUT(MPIR_DBG_DATATYPE, "--- end of flattened type ---");
    }
#endif

    MPIR_Segment_free(segp);

    err = MPIR_Dataloop_create_indexed(nr_blks, tmp_blklens, tmp_disps, 1,      /* disp in bytes */
                                       MPI_BYTE, dlp_p, dlsz_p, dldepth_p, flag);

    DLOOP_Free(tmp_blklens);
    DLOOP_Free(tmp_disps);

    return err;
}
