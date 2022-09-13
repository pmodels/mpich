/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop_internal.h"
#include "typerep_util.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#undef DEBUG_MEMORY

/* Dataloops
 *
 * The functions here are used for the creation, copying, update, and display
 * of MPII_Dataloop structures and trees of these structures.
 *
 * Currently we store trees of dataloops in contiguous regions of memory.  They
 * are stored in such a way that subtrees are also stored contiguously.  This
 * makes it somewhat easier to copy these subtrees around.  Keep this in mind
 * when looking at the functions below.
 *
 * The structures used in this file are defined in mpidu_datatype.h.  There is
 * no separate mpidu_dataloop_internal.h at this time.
 *
 * OPTIMIZATIONS:
 *
 * There are spots in the code with OPT tags that indicate where we could
 * optimize particular calculations or avoid certain checks.
 *
 * NOTES:
 *
 * Don't have locks in place at this time!
 */

/* Some functions in this file are responsible for allocation of space for
 * dataloops.  These structures include the dataloop structure itself
 * followed by a sequence of variable-sized arrays, depending on the loop
 * kind.  For example, a dataloop of kind MPII_DATALOOP_KIND_INDEXED has a
 * dataloop structure followed by an array of block sizes and then an array
 * of offsets.
 *
 * For efficiency and ease of cleanup (preserving a single free at
 * deallocation), we want to allocate this memory as a single large chunk.
 * However, we must perform some alignment of the components of this chunk
 * in order to obtain correct and efficient operation across all platforms.
 */


/*@
  Dataloop_free - deallocate the resources used to store a dataloop

Input/output Parameters:
. dataloop - pointer to dataloop structure
@*/
void MPIR_Dataloop_free(void **dataloop_)
{
    MPII_Dataloop **dataloop = (MPII_Dataloop **) dataloop_;

    if (*dataloop == NULL)
        return;

#ifdef DEBUG_MEMORY
    MPL_DBG_MSG_D(MPIR_DBG_DATATYPE, VERBOSE, "MPIR_Dataloop_free: freeing loop @ %x.\n",
                  (int) *dataloop);
#endif

    MPL_free(*dataloop);
    *dataloop = NULL;
    return;
}

struct dloop_flatten_hdr {
    MPI_Aint dloop_sz;
    MPII_Dataloop *dataloop_local_addr;
};

int MPIR_Dataloop_flatten_size(MPIR_Datatype * dtp, int *flattened_loop_size)
{
    MPII_Dataloop *dloop = (MPII_Dataloop *) dtp->typerep.handle;

    *flattened_loop_size = sizeof(struct dloop_flatten_hdr) + dloop->dloop_sz;

    return MPI_SUCCESS;
}

int MPIR_Dataloop_flatten(MPIR_Datatype * dtp, void *flattened_dataloop)
{
    struct dloop_flatten_hdr *dloop_flatten_hdr = (struct dloop_flatten_hdr *) flattened_dataloop;
    int mpi_errno = MPI_SUCCESS;
    MPII_Dataloop *dloop = (MPII_Dataloop *) dtp->typerep.handle;

    /*
     * Our flattened layout contains three elements:
     *    - The size of the dataloop
     *    - The local address of the dataloop (needed to update the dataloop pointers when we unflatten)
     *    - The actual dataloop itself
     */

    dloop_flatten_hdr->dloop_sz = dloop->dloop_sz;
    dloop_flatten_hdr->dataloop_local_addr = dloop;

    MPIR_Memcpy(((char *) flattened_dataloop + sizeof(struct dloop_flatten_hdr)),
                dloop, dloop->dloop_sz);

    return mpi_errno;
}

int MPIR_Dataloop_unflatten(MPIR_Datatype * dtp, void *flattened_dataloop)
{
    struct dloop_flatten_hdr *dloop_flatten_hdr = (struct dloop_flatten_hdr *) flattened_dataloop;
    MPI_Aint ptrdiff;
    int mpi_errno = MPI_SUCCESS;

    dtp->typerep.handle = MPL_malloc(dloop_flatten_hdr->dloop_sz, MPL_MEM_DATATYPE);
    MPIR_ERR_CHKANDJUMP1(dtp->typerep.handle == NULL, mpi_errno, MPI_ERR_INTERN, "**nomem",
                         "**nomem %s", "dataloop flatten hdr");

    MPIR_Memcpy(dtp->typerep.handle, (char *) flattened_dataloop + sizeof(struct dloop_flatten_hdr),
                dloop_flatten_hdr->dloop_sz);

    ptrdiff =
        (MPI_Aint) ((char *) (dtp->typerep.handle) -
                    (char *) dloop_flatten_hdr->dataloop_local_addr);
    MPII_Dataloop_update(dtp->typerep.handle, ptrdiff);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


/*@
  Dataloop_copy - Copy an arbitrary dataloop structure, updating
  pointers as necessary

Input Parameters:
+ dest   - pointer to destination region
. src    - pointer to original dataloop structure
- size   - size of dataloop structure

  This routine parses the dataloop structure as it goes in order to
  determine what exactly it needs to update.

  Notes:
  It assumes that the source dataloop was allocated in our usual way;
  this means that the entire dataloop is in a contiguous region and that
  the root of the tree is first in the array.

  This has some implications:
+ we can use a contiguous copy mechanism to copy the majority of the
  structure
- all pointers in the region are relative to the start of the data region
  the first dataloop in the array is the root of the tree
@*/
static void dloop_copy(void *dest, void *src, MPI_Aint size)
{
    MPI_Aint ptrdiff;

#ifdef DEBUG_MEMORY
    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                    (MPL_DBG_FDEST, "dloop_copy: copying from %x to %x (%z bytes).\n",
                     (int) src, (int) dest, (size_t) size));
#endif

    /* copy region first */
    MPIR_Memcpy(dest, src, size);

    /* Calculate difference in starting locations. MPII_Dataloop_update()
     * then traverses the new structure and updates internal pointers by
     * adding this difference to them. This way we can just copy the
     * structure, including pointers, in one big block.
     */
    ptrdiff = (MPI_Aint) ((char *) dest - (char *) src);

    /* traverse structure updating pointers */
    MPII_Dataloop_update(dest, ptrdiff);

    return;
}

/*@
  MPII_Dataloop_update - update pointers after a copy operation

Input Parameters:
+ dataloop - pointer to loop to update
- ptrdiff - value indicating offset between old and new pointer values

  This function is used to recursively update all the pointers in a
  dataloop tree.
@*/
void MPII_Dataloop_update(MPII_Dataloop * dataloop, MPI_Aint ptrdiff)
{
    /* OPT: only declare these variables down in the Struct case */
    int i;
    MPII_Dataloop **looparray;

    switch (dataloop->kind & MPII_DATALOOP_KIND_MASK) {
        case MPII_DATALOOP_KIND_CONTIG:
        case MPII_DATALOOP_KIND_VECTOR:
            /*
             * All these really ugly assignments are really of the form:
             *
             * ((char *) dataloop->loop_params.c_t.loop) += ptrdiff;
             *
             * However, some compilers spit out warnings about casting on the
             * LHS, so we get this much nastier form instead (using common
             * struct for contig and vector):
             */

            if (!(dataloop->kind & MPII_DATALOOP_FINAL_MASK)) {
                MPIR_Assert(dataloop->loop_params.cm_t.dataloop);

                dataloop->loop_params.cm_t.dataloop = (MPII_Dataloop *) (void *)
                    ((MPI_Aint) (char *) dataloop->loop_params.cm_t.dataloop + ptrdiff);

                MPII_Dataloop_update(dataloop->loop_params.cm_t.dataloop, ptrdiff);
            }
            break;

        case MPII_DATALOOP_KIND_BLOCKINDEXED:
            MPIR_Assert(dataloop->loop_params.bi_t.offset_array);

            dataloop->loop_params.bi_t.offset_array = (MPI_Aint *) (void *)
                ((MPI_Aint) (char *) dataloop->loop_params.bi_t.offset_array + ptrdiff);

            if (!(dataloop->kind & MPII_DATALOOP_FINAL_MASK)) {
                MPIR_Assert(dataloop->loop_params.bi_t.dataloop);

                dataloop->loop_params.bi_t.dataloop = (MPII_Dataloop *) (void *)
                    ((MPI_Aint) (char *) dataloop->loop_params.bi_t.dataloop + ptrdiff);

                MPII_Dataloop_update(dataloop->loop_params.bi_t.dataloop, ptrdiff);
            }
            break;

        case MPII_DATALOOP_KIND_INDEXED:
            MPIR_Assert(dataloop->loop_params.i_t.blocksize_array);

            dataloop->loop_params.i_t.blocksize_array = (MPI_Aint *) (void *)
                ((MPI_Aint) (char *) dataloop->loop_params.i_t.blocksize_array + ptrdiff);

            MPIR_Assert(dataloop->loop_params.i_t.offset_array);

            dataloop->loop_params.i_t.offset_array = (MPI_Aint *) (void *)
                ((MPI_Aint) (char *) dataloop->loop_params.i_t.offset_array + ptrdiff);

            if (!(dataloop->kind & MPII_DATALOOP_FINAL_MASK)) {
                MPIR_Assert(dataloop->loop_params.i_t.dataloop);

                dataloop->loop_params.i_t.dataloop = (MPII_Dataloop *) (void *)
                    ((MPI_Aint) (char *) dataloop->loop_params.i_t.dataloop + ptrdiff);

                MPII_Dataloop_update(dataloop->loop_params.i_t.dataloop, ptrdiff);
            }
            break;

        case MPII_DATALOOP_KIND_STRUCT:
            MPIR_Assert(dataloop->loop_params.s_t.blocksize_array);

            dataloop->loop_params.s_t.blocksize_array = (MPI_Aint *) (void *)
                ((MPI_Aint) (char *) dataloop->loop_params.s_t.blocksize_array + ptrdiff);

            MPIR_Assert(dataloop->loop_params.s_t.offset_array);

            dataloop->loop_params.s_t.offset_array = (MPI_Aint *) (void *)
                ((MPI_Aint) (char *) dataloop->loop_params.s_t.offset_array + ptrdiff);

            if (dataloop->kind & MPII_DATALOOP_FINAL_MASK)
                break;

            MPIR_Assert(dataloop->loop_params.s_t.dataloop_array);

            dataloop->loop_params.s_t.dataloop_array = (MPII_Dataloop **) (void *)
                ((MPI_Aint) (char *) dataloop->loop_params.s_t.dataloop_array + ptrdiff);

            /* fix the N dataloop pointers too */
            looparray = dataloop->loop_params.s_t.dataloop_array;
            for (i = 0; i < dataloop->loop_params.s_t.count; i++) {
                MPIR_Assert(looparray[i]);

                looparray[i] = (MPII_Dataloop *) (void *)
                    ((MPI_Aint) (char *) looparray[i] + ptrdiff);
            }

            for (i = 0; i < dataloop->loop_params.s_t.count; i++) {
                MPII_Dataloop_update(looparray[i], ptrdiff);
            }
            break;
        default:
            /* --BEGIN ERROR HANDLING-- */
            MPIR_Assert(0);
            break;
            /* --END ERROR HANDLING-- */
    }
    return;
}

/*@
  Dataloop_alloc - allocate the resources used to store a dataloop with
                   no old loops associated with it.

Input Parameters:
+ kind          - kind of dataloop to allocate
. count         - number of elements in dataloop (kind dependent)
. new_loop_p    - address at which to store new dataloop pointer

  Notes:
  The count parameter passed into this function will often be different
  from the count passed in at the MPI layer due to optimizations.
@*/
void MPII_Dataloop_alloc(int kind, MPI_Aint count, MPII_Dataloop ** new_loop_p)
{
    MPII_Dataloop_alloc_and_copy(kind, count, NULL, new_loop_p);
    return;
}

/*@
  Dataloop_alloc_and_copy - allocate the resources used to store a
                            dataloop and copy in old dataloop as
                            appropriate

Input Parameters:
+ kind          - kind of dataloop to allocate
. count         - number of elements in dataloop (kind dependent)
. old_loop      - pointer to old dataloop (or NULL for none)
. new_loop_p    - address at which to store new dataloop pointer

  Notes:
  The count parameter passed into this function will often be different
  from the count passed in at the MPI layer.
@*/
void MPII_Dataloop_alloc_and_copy(int kind,
                                  MPI_Aint count,
                                  MPII_Dataloop * old_loop, MPII_Dataloop ** new_loop_p)
{
    MPI_Aint new_loop_sz = 0;
    int epsilon;
    MPI_Aint loop_sz = sizeof(MPII_Dataloop);
    MPI_Aint off_sz = 0, blk_sz = 0, ptr_sz = 0, extent_sz = 0;

    char *pos;
    MPII_Dataloop *new_loop;
    MPI_Aint old_loop_sz = old_loop ? old_loop->dloop_sz : 0;

    if (old_loop != NULL) {
        MPIR_Assert((old_loop_sz % MAX_ALIGNMENT) == 0);
    }

    /* calculate the space that we actually need for everything */
    switch (kind) {
        case MPII_DATALOOP_KIND_STRUCT:
            /* need space for dataloop pointers and extents */
            ptr_sz = count * sizeof(MPII_Dataloop *);
            extent_sz = count * sizeof(MPI_Aint);
            /* fall through */
        case MPII_DATALOOP_KIND_INDEXED:
            /* need space for block sizes */
            blk_sz = count * sizeof(MPI_Aint);
            /* fall through */
        case MPII_DATALOOP_KIND_BLOCKINDEXED:
            /* need space for block offsets */
            off_sz = count * sizeof(MPI_Aint);
        case MPII_DATALOOP_KIND_CONTIG:
        case MPII_DATALOOP_KIND_VECTOR:
            break;
        default:
            MPIR_Assert(0);
    }

    /* pad everything that we're going to allocate */
    epsilon = loop_sz % MAX_ALIGNMENT;
    if (epsilon)
        loop_sz += MAX_ALIGNMENT - epsilon;

    epsilon = off_sz % MAX_ALIGNMENT;
    if (epsilon)
        off_sz += MAX_ALIGNMENT - epsilon;

    epsilon = blk_sz % MAX_ALIGNMENT;
    if (epsilon)
        blk_sz += MAX_ALIGNMENT - epsilon;

    epsilon = ptr_sz % MAX_ALIGNMENT;
    if (epsilon)
        ptr_sz += MAX_ALIGNMENT - epsilon;

    epsilon = extent_sz % MAX_ALIGNMENT;
    if (epsilon)
        extent_sz += MAX_ALIGNMENT - epsilon;

    new_loop_sz += loop_sz + off_sz + blk_sz + ptr_sz + extent_sz + old_loop_sz;

    /* allocate space */
    new_loop = (MPII_Dataloop *) MPL_malloc(new_loop_sz, MPL_MEM_DATATYPE);
    if (new_loop == NULL) {
        *new_loop_p = NULL;
        return;
    }
#ifdef DEBUG_MEMORY
    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                    (MPL_DBG_FDEST,
                     "MPII_Dataloop_alloc_and_copy: new loop @ %x (tot sz = %z, loop = %z, off = %z, blk = %z, ptr = %z, extent = %z, old = %z)\n",
                     (int) new_loop, new_loop_sz, loop_sz, off_sz, blk_sz, ptr_sz, extent_sz,
                     old_loop_sz));
#endif

    /* set all the pointers in the new dataloop structure */
    switch (kind) {
        case MPII_DATALOOP_KIND_STRUCT:
            /* order is:
             * - pointers
             * - blocks
             * - offsets
             * - extents
             */
            new_loop->loop_params.s_t.dataloop_array =
                (MPII_Dataloop **) (((char *) new_loop) + loop_sz);
            new_loop->loop_params.s_t.blocksize_array =
                (MPI_Aint *) (((char *) new_loop) + loop_sz + ptr_sz);
            new_loop->loop_params.s_t.offset_array =
                (MPI_Aint *) (((char *) new_loop) + loop_sz + ptr_sz + blk_sz);
            new_loop->loop_params.s_t.el_extent_array =
                (MPI_Aint *) (((char *) new_loop) + loop_sz + ptr_sz + blk_sz + off_sz);
            break;
        case MPII_DATALOOP_KIND_INDEXED:
            /* order is:
             * - blocks
             * - offsets
             */
            new_loop->loop_params.i_t.blocksize_array =
                (MPI_Aint *) (((char *) new_loop) + loop_sz);
            new_loop->loop_params.i_t.offset_array =
                (MPI_Aint *) (((char *) new_loop) + loop_sz + blk_sz);
            if (old_loop == NULL) {
                new_loop->loop_params.i_t.dataloop = NULL;
            } else {
                new_loop->loop_params.i_t.dataloop =
                    (MPII_Dataloop *) (((char *) new_loop) + (new_loop_sz - old_loop_sz));
            }
            break;
        case MPII_DATALOOP_KIND_BLOCKINDEXED:
            new_loop->loop_params.bi_t.offset_array = (MPI_Aint *) (((char *) new_loop) + loop_sz);
            if (old_loop == NULL) {
                new_loop->loop_params.bi_t.dataloop = NULL;
            } else {
                new_loop->loop_params.bi_t.dataloop =
                    (MPII_Dataloop *) (((char *) new_loop) + (new_loop_sz - old_loop_sz));
            }
            break;
        case MPII_DATALOOP_KIND_CONTIG:
            if (old_loop == NULL) {
                new_loop->loop_params.c_t.dataloop = NULL;
            } else {
                new_loop->loop_params.c_t.dataloop =
                    (MPII_Dataloop *) (((char *) new_loop) + (new_loop_sz - old_loop_sz));
            }
            break;
        case MPII_DATALOOP_KIND_VECTOR:
            if (old_loop == NULL) {
                new_loop->loop_params.v_t.dataloop = NULL;
            } else {
                new_loop->loop_params.v_t.dataloop =
                    (MPII_Dataloop *) (((char *) new_loop) + (new_loop_sz - old_loop_sz));
            }
            break;
        default:
            MPIR_Assert(0);
    }

    pos = ((char *) new_loop) + (new_loop_sz - old_loop_sz);
    if (old_loop != NULL) {
        dloop_copy(pos, old_loop, old_loop_sz);
    }

    *new_loop_p = new_loop;
    new_loop->dloop_sz = new_loop_sz;
    return;
}

/*@
  Dataloop_dup - make a copy of a dataloop

  Returns 0 on success, -1 on failure.
@*/
void MPIR_Dataloop_dup(void *old_loop_, void **new_loop_p_)
{
    MPII_Dataloop *new_loop;
    MPII_Dataloop **new_loop_p = (MPII_Dataloop **) new_loop_p_;
    MPII_Dataloop *old_loop = (MPII_Dataloop *) old_loop_;

    MPIR_Assert(old_loop != NULL);

    MPI_Aint old_loop_sz = old_loop->dloop_sz;
    MPIR_Assert(old_loop_sz > 0);

    new_loop = (MPII_Dataloop *) MPL_malloc(old_loop_sz, MPL_MEM_DATATYPE);
    if (new_loop == NULL) {
        *new_loop_p = NULL;
        return;
    }

    dloop_copy(new_loop, old_loop, old_loop_sz);
    *new_loop_p = new_loop;
    return;
}

void MPIR_Dataloop_create_resized(MPI_Datatype oldtype, MPI_Aint extent, void **new_loop_p_)
{
    MPII_Dataloop *new_loop;
    MPIR_Dataloop_create_contiguous(1, oldtype, (void **) &new_loop);
    new_loop->el_extent = extent;

    *new_loop_p_ = new_loop;
}

MPI_Aint MPIR_Dataloop_size_external32(MPI_Datatype type)
{
    if (HANDLE_IS_BUILTIN(type)) {
        return MPII_Typerep_get_basic_size_external32(type);
    } else {
        MPII_Dataloop *dlp = NULL;

        MPIR_DATALOOP_GET_LOOPPTR(type, dlp);
        MPIR_Assert(dlp != NULL);

        return MPII_Dataloop_stream_size(dlp, MPII_Typerep_get_basic_size_external32);
    }
}

/*@
MPII_Dataloop_stream_size - return the size of the data described by the dataloop

Input Parameters:
+ dl_p   - pointer to dataloop for which we will return the size
- sizefn - function for determining size of types in the corresponding stream
           (passing NULL will instead result in el_size values being used)

@*/
MPI_Aint MPII_Dataloop_stream_size(MPII_Dataloop * dl_p, MPI_Aint(*sizefn) (MPI_Datatype el_type))
{
    MPI_Aint tmp_sz, tmp_ct = 1;

    for (;;) {
        if ((dl_p->kind & MPII_DATALOOP_KIND_MASK) == MPII_DATALOOP_KIND_STRUCT) {
            int i;

            tmp_sz = 0;
            for (i = 0; i < dl_p->loop_params.s_t.count; i++) {
                tmp_sz += (MPI_Aint) (dl_p->loop_params.s_t.blocksize_array[i]) *
                    MPII_Dataloop_stream_size(dl_p->loop_params.s_t.dataloop_array[i], sizefn);
            }
            return tmp_sz * tmp_ct;
        }

        switch (dl_p->kind & MPII_DATALOOP_KIND_MASK) {
            case MPII_DATALOOP_KIND_CONTIG:
                tmp_ct *= (MPI_Aint) (dl_p->loop_params.c_t.count);
#ifdef MPII_DATALOOP_DEBUG_SIZE
                MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "stream_size: contig: ct = %d; new tot_ct = "
                                 MPI_AINT_FMT_DEC_SPEC "\n", (int) dl_p->loop_params.c_t.count,
                                 (MPI_Aint) tmp_ct));
#endif
                break;
            case MPII_DATALOOP_KIND_VECTOR:
                tmp_ct *= (MPI_Aint) (dl_p->loop_params.v_t.count) *
                    (MPI_Aint) (dl_p->loop_params.v_t.blocksize);
#ifdef MPII_DATALOOP_DEBUG_SIZE
                MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "stream_size: vector: ct = %d; blk = %d; new tot_ct = "
                                 MPI_AINT_FMT_DEC_SPEC "\n", (int) dl_p->loop_params.v_t.count,
                                 (int) dl_p->loop_params.v_t.blocksize, (MPI_Aint) tmp_ct));
#endif
                break;
            case MPII_DATALOOP_KIND_BLOCKINDEXED:
                tmp_ct *= (MPI_Aint) (dl_p->loop_params.bi_t.count) *
                    (MPI_Aint) (dl_p->loop_params.bi_t.blocksize);
#ifdef MPII_DATALOOP_DEBUG_SIZE
                MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "stream_size: blkindexed: blks = %d; new tot_ct = "
                                 MPI_AINT_FMT_DEC_SPEC "\n",
                                 (int) dl_p->loop_params.bi_t.count *
                                 (int) dl_p->loop_params.bi_t.blocksize, (MPI_Aint) tmp_ct));
#endif
                break;
            case MPII_DATALOOP_KIND_INDEXED:
                tmp_ct *= (MPI_Aint) (dl_p->loop_params.i_t.total_blocks);
#ifdef MPII_DATALOOP_DEBUG_SIZE
                MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "stream_size: contig: blks = %d; new tot_ct = "
                                 MPI_AINT_FMT_DEC_SPEC "\n",
                                 (int) dl_p->loop_params.i_t.total_blocks, (MPI_Aint) tmp_ct));
#endif
                break;
            default:
                /* --BEGIN ERROR HANDLING-- */
                MPIR_Assert(0);
                break;
                /* --END ERROR HANDLING-- */
        }

        if (dl_p->kind & MPII_DATALOOP_FINAL_MASK)
            break;
        else {
            MPIR_Assert(dl_p->loop_params.cm_t.dataloop != NULL);
            dl_p = dl_p->loop_params.cm_t.dataloop;
        }
    }

    /* call fn for size using bottom type, or use size if fnptr is NULL */
    tmp_sz = ((sizefn) ? sizefn(dl_p->el_type) : dl_p->el_size);

    return tmp_sz * tmp_ct;
}

/* dataloop doesn't maintain modified LB and UB (e.g. struct, resized).
 * Update and overwrite is_contig from upper level after dataloop is created.
 */
void MPIR_Dataloop_update_contig(void *dataloop, MPI_Aint extent, MPI_Aint typesize)
{
    MPII_Dataloop *dlp = (MPII_Dataloop *) dataloop;
    /* the type is only contiguous if extent is equal to size */
    if (dlp->is_contig) {
        if (extent != typesize) {
            dlp->is_contig = 0;
        }
    }
}

void MPIR_Dataloop_get_contig(void *dataloop, int *is_contig, MPI_Aint * num_contig)
{
    MPII_Dataloop *dlp = (MPII_Dataloop *) dataloop;
    *is_contig = dlp->is_contig;
    *num_contig = dlp->num_contig;
}
