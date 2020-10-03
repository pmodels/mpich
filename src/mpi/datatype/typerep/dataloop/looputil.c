/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop_internal.h"
#include "datatype.h"
#include "mpir_typerep.h"
#include "looputil.h"
#include "veccpy.h"

#define M2M_TO_USERBUF   0
#define M2M_FROM_USERBUF 1

/* piece_params
 *
 * This structure is used to pass function-specific parameters into our
 * segment processing function.  This allows us to get additional parameters
 * to the functions it calls without changing the prototype.
 */
struct piece_params {
    union {
        struct {
            char *pack_buffer;
        } pack;
        struct {
            struct iovec *vectorp;
            int index;
            int length;
        } pack_vector;
        struct {
            int64_t *offp;
            MPI_Aint *sizep;    /* see notes in Segment_flatten header */
            int index;
            int length;
        } flatten;
        struct {
            char *last_loc;
            int count;
        } contig_blocks;
        struct {
            const char *unpack_buffer;
        } unpack;
        struct {
            int stream_off;
        } print;
    } u;
};

/* #define MPICH_DEBUG_SEGMENT_MOVE */
/* TODO: Consider integrating this with the general debug support. */
/* Note: This does not use the CVAR support for the environment variable
   because (a) this is a temporary code and (b) it is expert developer
   only */
#ifdef MPICH_DEBUG_SEGMENT_MOVE
static int printSegment = -1;
static void setPrint(void)
{
    char *s = getenv("MPICH_DATALOOP_PRINT");
    if (s && (strcmp(s, "yes") == 0 || strcmp(s, "YES") == 0)) {
        printSegment = 1;
    } else {
        printSegment = 0;
    }
}

#define DBG_SEGMENT(_a) do { if (printSegment < 0) setPrint(); \
        if (printSegment) { _a; } } while (0)
#else
#define DBG_SEGMENT(_a)
#endif

/* NOTE: bufp values are unused, ripe for removal */

static int contig_m2m(MPI_Aint * blocks_p,
                      MPI_Datatype el_type, MPI_Aint rel_off, void *bufp, void *v_paramp);
static int vector_m2m(MPI_Aint * blocks_p,
                      MPI_Aint count,
                      MPI_Aint blksz,
                      MPI_Aint stride,
                      MPI_Datatype el_type, MPI_Aint rel_off, void *bufp, void *v_paramp);
static int blkidx_m2m(MPI_Aint * blocks_p,
                      MPI_Aint count,
                      MPI_Aint blocklen,
                      MPI_Aint * offsetarray,
                      MPI_Datatype el_type, MPI_Aint rel_off, void *bufp, void *v_paramp);
static int index_m2m(MPI_Aint * blocks_p,
                     MPI_Aint count,
                     MPI_Aint * blockarray,
                     MPI_Aint * offsetarray,
                     MPI_Datatype el_type, MPI_Aint rel_off, void *bufp, void *v_paramp);

/* prototypes of internal functions */
static int vector_pack_to_iov(MPI_Aint * blocks_p,
                              MPI_Aint count,
                              MPI_Aint blksz,
                              MPI_Aint stride,
                              MPI_Datatype el_type, MPI_Aint rel_off, void *bufp, void *v_paramp);

static int contig_pack_to_iov(MPI_Aint * blocks_p,
                              MPI_Datatype el_type, MPI_Aint rel_off, void *bufp, void *v_paramp);

static inline int is_float_type(MPI_Datatype el_type)
{
    return ((el_type == MPI_FLOAT) || (el_type == MPI_DOUBLE) ||
            (el_type == MPI_LONG_DOUBLE) ||
            (el_type == MPI_DOUBLE_PRECISION) ||
            (el_type == MPI_COMPLEX) || (el_type == MPI_DOUBLE_COMPLEX));
/*             (el_type == MPI_REAL4) || (el_type == MPI_REAL8) || */
/*             (el_type == MPI_REAL16)); */
}

static int external32_basic_convert(char *dest_buf,
                                    const char *src_buf,
                                    int dest_el_size, int src_el_size, MPI_Aint count)
{
    const char *src_ptr = src_buf;
    char *dest_ptr = dest_buf;
    const char *src_end = src_buf + ((int) count * src_el_size);

    MPIR_Assert(dest_buf && src_buf);

    if (src_el_size == dest_el_size) {
        if (src_el_size == 2) {
            while (src_ptr != src_end) {
                BASIC_convert16((*(const TWO_BYTE_BASIC_TYPE *) src_ptr),
                                (*(TWO_BYTE_BASIC_TYPE *) dest_ptr));

                src_ptr += src_el_size;
                dest_ptr += dest_el_size;
            }
        } else if (src_el_size == 4) {
            while (src_ptr != src_end) {
                BASIC_convert32((*(const FOUR_BYTE_BASIC_TYPE *) src_ptr),
                                (*(FOUR_BYTE_BASIC_TYPE *) dest_ptr));

                src_ptr += src_el_size;
                dest_ptr += dest_el_size;
            }
        } else if (src_el_size == 8) {
            while (src_ptr != src_end) {
                BASIC_convert64(src_ptr, dest_ptr);

                src_ptr += src_el_size;
                dest_ptr += dest_el_size;
            }
        }
    } else {
        /* TODO */
        MPL_error_printf
            ("Conversion of types whose size is not the same as the size in external32 is not supported\n");
        MPID_Abort(0, MPI_SUCCESS, 1, "Aborting with internal error");
        /* There is no way to return an error code, so an abort is the
         * only choice (the return value of this routine is not
         * an error code) */
    }
    return 0;
}

static int external32_float_convert(char *dest_buf,
                                    const char *src_buf, int dest_el_size, int src_el_size,
                                    int count)
{
    const char *src_ptr = src_buf;
    char *dest_ptr = dest_buf;
    const char *src_end = src_buf + ((int) count * src_el_size);

    MPIR_Assert(dest_buf && src_buf);

    if (src_el_size == dest_el_size) {
        if (src_el_size == 4) {
            while (src_ptr != src_end) {
                FLOAT_convert((*(const FOUR_BYTE_FLOAT_TYPE *) src_ptr),
                              (*(FOUR_BYTE_FLOAT_TYPE *) dest_ptr));

                src_ptr += src_el_size;
                dest_ptr += dest_el_size;
            }
        } else if (src_el_size == 8) {
            while (src_ptr != src_end) {
                FLOAT_convert((*(const EIGHT_BYTE_FLOAT_TYPE *) src_ptr),
                              (*(EIGHT_BYTE_FLOAT_TYPE *) dest_ptr));

                src_ptr += src_el_size;
                dest_ptr += dest_el_size;
            }
        }
    } else {
        /* TODO */
        MPL_error_printf
            ("Conversion of types whose size is not the same as the size in external32 is not supported\n");
        MPID_Abort(0, MPI_SUCCESS, 1, "Aborting with internal error");
        /* There is no way to return an error code, so an abort is the
         * only choice (the return value of this routine is not
         * an error code) */
    }
    return 0;
}

/* segment_init
 *
 * buf    - datatype buffer location
 * count  - number of instances of the datatype in the buffer
 * handle - handle for datatype (could be derived or not)
 * segp   - pointer to previously allocated segment structure
 *
 * Notes:
 * - Assumes that the segment has been allocated.
 *
 */
static inline void segment_init(const void *buf,
                                MPI_Aint count, MPI_Datatype handle, struct MPIR_Segment *segp)
{
    MPI_Aint elmsize = 0;
    int i, depth = 0;
    int branch_detected = 0;

    struct MPII_Dataloop_stackelm *elmp;
    MPII_Dataloop *dlp = 0, *sblp = &segp->builtin_loop;

#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                    (MPL_DBG_FDEST, "segment_init: count = %d, buf = %x\n", count, buf));
#endif

    if (!MPII_DATALOOP_HANDLE_HASLOOP(handle)) {
        /* simplest case; datatype has no loop (basic) */

        MPIR_Datatype_get_size_macro(handle, elmsize);

        sblp->kind = MPII_DATALOOP_KIND_CONTIG | MPII_DATALOOP_FINAL_MASK;
        sblp->loop_params.c_t.count = count;
        sblp->loop_params.c_t.dataloop = 0;
        sblp->el_size = elmsize;
        MPIR_Datatype_get_basic_type(handle, sblp->el_type);
        MPIR_Datatype_get_extent_macro(handle, sblp->el_extent);

        dlp = sblp;
        depth = 1;
    } else if (count == 0) {
        /* only use the builtin */
        sblp->kind = MPII_DATALOOP_KIND_CONTIG | MPII_DATALOOP_FINAL_MASK;
        sblp->loop_params.c_t.count = 0;
        sblp->loop_params.c_t.dataloop = 0;
        sblp->el_size = 0;
        sblp->el_extent = 0;

        dlp = sblp;
        depth = 1;
    } else if (count == 1) {
        /* don't use the builtin */
        MPIR_DATALOOP_GET_LOOPPTR(handle, dlp);
    } else {
        /* default: need to use builtin to handle contig; must check
         * loop depth first
         */
        MPII_Dataloop *oldloop; /* loop from original type, before new count */
        MPI_Aint type_size, type_extent;
        MPI_Datatype el_type;

        MPIR_DATALOOP_GET_LOOPPTR(handle, oldloop);
        MPIR_Assert(oldloop != NULL);
        MPIR_Datatype_get_size_macro(handle, type_size);
        MPIR_Datatype_get_extent_macro(handle, type_extent);
        MPIR_Datatype_get_basic_type(handle, el_type);

        if (depth == 1 && ((oldloop->kind & MPII_DATALOOP_KIND_MASK) == MPII_DATALOOP_KIND_CONTIG)) {
            if (type_size == type_extent) {
                /* use a contig */
                sblp->kind = MPII_DATALOOP_KIND_CONTIG | MPII_DATALOOP_FINAL_MASK;
                sblp->loop_params.c_t.count = count * oldloop->loop_params.c_t.count;
                sblp->loop_params.c_t.dataloop = NULL;
                sblp->el_size = oldloop->el_size;
                sblp->el_extent = oldloop->el_extent;
                sblp->el_type = oldloop->el_type;
            } else {
                /* use a vector, with extent of original type becoming the stride */
                sblp->kind = MPII_DATALOOP_KIND_VECTOR | MPII_DATALOOP_FINAL_MASK;
                sblp->loop_params.v_t.count = count;
                sblp->loop_params.v_t.blocksize = oldloop->loop_params.c_t.count;
                sblp->loop_params.v_t.stride = type_extent;
                sblp->loop_params.v_t.dataloop = NULL;
                sblp->el_size = oldloop->el_size;
                sblp->el_extent = oldloop->el_extent;
                sblp->el_type = oldloop->el_type;
            }
        } else {
            /* general case */
            sblp->kind = MPII_DATALOOP_KIND_CONTIG;
            sblp->loop_params.c_t.count = count;
            sblp->loop_params.c_t.dataloop = oldloop;
            sblp->el_size = type_size;
            sblp->el_extent = type_extent;
            sblp->el_type = el_type;

            depth++;    /* we're adding to the depth with the builtin */
            MPIR_Assert(depth < (MPII_DATALOOP_MAX_DATATYPE_DEPTH));
        }

        dlp = sblp;
    }

    /* assert instead of return b/c dtype/dloop errorhandling code is inconsistent */
    MPIR_Assert(depth < (MPII_DATALOOP_MAX_DATATYPE_DEPTH));

    /* initialize the rest of the segment values */
    segp->handle = handle;
    segp->ptr = (void *) buf;
    segp->stream_off = 0;
    segp->cur_sp = 0;
    segp->valid_sp = 0;

    /* initialize the first stackelm in its entirety */
    elmp = &(segp->stackelm[0]);
    MPII_Dataloop_stackelm_load(elmp, dlp, 0);
    branch_detected = elmp->may_require_reloading;

    /* Fill in parameters not set by MPII_Dataloop_stackelm_load */
    elmp->orig_offset = 0;
    elmp->curblock = elmp->orig_block;
    /* MPII_Dataloop_stackelm_offset assumes correct orig_count, curcount, loop_p */
    elmp->curoffset = /* elmp->orig_offset + */ MPII_Dataloop_stackelm_offset(elmp);

    i = 1;
    while (!(dlp->kind & MPII_DATALOOP_FINAL_MASK)) {
        /* get pointer to next dataloop */
        switch (dlp->kind & MPII_DATALOOP_KIND_MASK) {
            case MPII_DATALOOP_KIND_CONTIG:
            case MPII_DATALOOP_KIND_VECTOR:
            case MPII_DATALOOP_KIND_BLOCKINDEXED:
            case MPII_DATALOOP_KIND_INDEXED:
                dlp = dlp->loop_params.cm_t.dataloop;
                break;
            case MPII_DATALOOP_KIND_STRUCT:
                dlp = dlp->loop_params.s_t.dataloop_array[0];
                break;
            default:
                /* --BEGIN ERROR HANDLING-- */
                MPIR_Assert(0);
                break;
                /* --END ERROR HANDLING-- */
        }

        MPIR_Assert(i < MPII_DATALOOP_MAX_DATATYPE_DEPTH);

        /* loop_p, orig_count, orig_block, and curcount are all filled by us now.
         * the rest are filled in at processing time.
         */
        elmp = &(segp->stackelm[i]);

        MPII_Dataloop_stackelm_load(elmp, dlp, branch_detected);
        branch_detected = elmp->may_require_reloading;
        i++;

    }

    segp->valid_sp = depth - 1;
}

struct MPIR_Segment *MPIR_Segment_alloc(const void *buf, MPI_Aint count, MPI_Datatype handle)
{
    struct MPIR_Segment *segp;

    segp = (struct MPIR_Segment *) MPL_malloc(sizeof(struct MPIR_Segment), MPL_MEM_DATATYPE);
    if (segp)
        segment_init(buf, count, handle, segp);

    return segp;
}

/* Segment_free
 *
 * Input Parameters:
 * segp - pointer to segment
 */
void MPIR_Segment_free(struct MPIR_Segment *segp)
{
    MPL_free(segp);
    return;
}

void MPIR_Segment_pack(MPIR_Segment * segp, MPI_Aint first, MPI_Aint * lastp, void *streambuf)
{
    struct MPII_Dataloop_m2m_params params;     /* defined in dataloop_parts.h */

    DBG_SEGMENT(printf("Segment_pack...\n"));
    /* experimenting with discarding buf value in the segment, keeping in
     * per-use structure instead. would require moving the parameters around a
     * bit.
     */
    params.userbuf = segp->ptr;
    params.streambuf = streambuf;
    params.direction = M2M_FROM_USERBUF;

    MPII_Segment_manipulate(segp, first, lastp, contig_m2m, vector_m2m, blkidx_m2m, index_m2m, NULL,    /* size fn */
                            &params);
    return;
}

void MPIR_Segment_unpack(MPIR_Segment * segp, MPI_Aint first, MPI_Aint * lastp,
                         const void *streambuf)
{
    struct MPII_Dataloop_m2m_params params;

    DBG_SEGMENT(printf("Segment_unpack...\n"));
    /* experimenting with discarding buf value in the segment, keeping in
     * per-use structure instead. would require moving the parameters around a
     * bit.
     */
    params.userbuf = segp->ptr;
    params.streambuf = (void *) streambuf;
    params.direction = M2M_TO_USERBUF;

    MPII_Segment_manipulate(segp, first, lastp, contig_m2m, vector_m2m, blkidx_m2m, index_m2m, NULL,    /* size fn */
                            &params);
    return;
}

/* PIECE FUNCTIONS BELOW */

static int contig_m2m(MPI_Aint * blocks_p,
                      MPI_Datatype el_type,
                      MPI_Aint rel_off, void *bufp ATTRIBUTE((unused)), void *v_paramp)
{
    MPI_Aint el_size;           /* MPI_Aint? */
    MPI_Aint size;
    struct MPII_Dataloop_m2m_params *paramp = v_paramp;

    MPIR_Datatype_get_size_macro(el_type, el_size);
    size = *blocks_p * el_size;

    DBG_SEGMENT(printf("element type = %lx\n", (long) el_type));
    DBG_SEGMENT(printf("contig m2m: elsize = %d, size = %d\n", (int) el_size, (int) size));
#ifdef MPID_SU_VERBOSE
    dbg_printf("\t[contig unpack: do=" MPI_AINT_FMT_DEC_SPEC ", dp=%x, bp=%x, sz="
               MPI_AINT_FMT_DEC_SPEC ", blksz=" MPI_AINT_FMT_DEC_SPEC "]\n", rel_off,
               (unsigned) bufp, (unsigned) paramp->u.unpack.unpack_buffer, el_size, *blocks_p);
#endif

    if (paramp->direction == M2M_TO_USERBUF) {
        MPIR_Memcpy((char *) paramp->userbuf + rel_off, paramp->streambuf, size);
    } else {
        MPIR_Memcpy(paramp->streambuf, (char *) paramp->userbuf + rel_off, size);
    }
    paramp->streambuf += size;
    return 0;
}

/* Segment_vector_m2m
 *
 * Note: this combines both packing and unpacking functionality.
 *
 * Note: this is only called when the starting position is at the beginning
 * of a whole block in a vector type.
 */
static int vector_m2m(MPI_Aint * blocks_p, MPI_Aint count ATTRIBUTE((unused)), MPI_Aint blksz, MPI_Aint stride, MPI_Datatype el_type, MPI_Aint rel_off, /* offset into buffer */
                      void *bufp ATTRIBUTE((unused)), void *v_paramp)
{
    MPI_Aint i;
    MPI_Aint el_size, whole_count, blocks_left;
    struct MPII_Dataloop_m2m_params *paramp = v_paramp;
    char *cbufp;

    cbufp = (char *) paramp->userbuf + rel_off;
    MPIR_Datatype_get_size_macro(el_type, el_size);
    DBG_SEGMENT(printf
                ("vector m2m: elsize = %d, count = %d, stride = %d, blocksize = %d\n",
                 (int) el_size, (int) count, (int) stride, (int) blksz));

    whole_count = (MPI_Aint) ((blksz > 0) ? (*blocks_p / (MPI_Aint) blksz) : 0);
    blocks_left = (MPI_Aint) ((blksz > 0) ? (*blocks_p % (MPI_Aint) blksz) : 0);

    if (paramp->direction == M2M_TO_USERBUF) {
        if (el_size == 8 MPIR_ALIGN8_TEST(paramp->streambuf, cbufp)) {
            MPII_COPY_TO_VEC(paramp->streambuf, cbufp, stride, int64_t, blksz, whole_count);
            MPII_COPY_TO_VEC(paramp->streambuf, cbufp, 0, int64_t, blocks_left, 1);
        } else if (el_size == 4 MPIR_ALIGN4_TEST(paramp->streambuf, cbufp)) {
            MPII_COPY_TO_VEC((paramp->streambuf), cbufp, stride, int32_t, blksz, whole_count);
            MPII_COPY_TO_VEC(paramp->streambuf, cbufp, 0, int32_t, blocks_left, 1);
        } else if (el_size == 2) {
            MPII_COPY_TO_VEC(paramp->streambuf, cbufp, stride, int16_t, blksz, whole_count);
            MPII_COPY_TO_VEC(paramp->streambuf, cbufp, 0, int16_t, blocks_left, 1);
        } else {
            for (i = 0; i < whole_count; i++) {
                MPIR_Memcpy(cbufp, paramp->streambuf, ((MPI_Aint) blksz) * el_size);
                DBG_SEGMENT(printf("vec: memcpy %p %p %d\n", cbufp,
                                   paramp->streambuf, (int) (blksz * el_size)));
                paramp->streambuf += ((MPI_Aint) blksz) * el_size;

                cbufp += stride;
            }
            if (blocks_left) {
                MPIR_Memcpy(cbufp, paramp->streambuf, ((MPI_Aint) blocks_left) * el_size);
                DBG_SEGMENT(printf("vec(left): memcpy %p %p %d\n", cbufp,
                                   paramp->streambuf, (int) (blocks_left * el_size)));
                paramp->streambuf += ((MPI_Aint) blocks_left) * el_size;
            }
        }
    } else {    /* M2M_FROM_USERBUF */

        if (el_size == 8 MPIR_ALIGN8_TEST(cbufp, paramp->streambuf)) {
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, stride, int64_t, blksz, whole_count);
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, 0, int64_t, blocks_left, 1);
        } else if (el_size == 4 MPIR_ALIGN4_TEST(cbufp, paramp->streambuf)) {
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, stride, int32_t, blksz, whole_count);
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, 0, int32_t, blocks_left, 1);
        } else if (el_size == 2) {
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, stride, int16_t, blksz, whole_count);
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, 0, int16_t, blocks_left, 1);
        } else {
            for (i = 0; i < whole_count; i++) {
                MPIR_Memcpy(paramp->streambuf, cbufp, (MPI_Aint) blksz * el_size);
                DBG_SEGMENT(printf("vec: memcpy %p %p %d\n",
                                   paramp->streambuf, cbufp, (int) (blksz * el_size)));
                paramp->streambuf += (MPI_Aint) blksz *el_size;
                cbufp += stride;
            }
            if (blocks_left) {
                MPIR_Memcpy(paramp->streambuf, cbufp, (MPI_Aint) blocks_left * el_size);
                DBG_SEGMENT(printf("vec(left): memcpy %p %p %d\n",
                                   paramp->streambuf, cbufp, (int) (blocks_left * el_size)));
                paramp->streambuf += (MPI_Aint) blocks_left *el_size;
            }
        }
    }

    return 0;
}

static int blkidx_m2m(MPI_Aint * blocks_p,
                      MPI_Aint count,
                      MPI_Aint blocklen,
                      MPI_Aint * offsetarray,
                      MPI_Datatype el_type,
                      MPI_Aint rel_off, void *bufp ATTRIBUTE((unused)), void *v_paramp)
{
    MPI_Aint curblock = 0;
    MPI_Aint el_size;
    MPI_Aint blocks_left = *blocks_p;
    char *cbufp;
    struct MPII_Dataloop_m2m_params *paramp = v_paramp;

    MPIR_Datatype_get_size_macro(el_type, el_size);
    DBG_SEGMENT(printf("blkidx m2m: elsize = %ld, count = %ld, blocklen = %ld,"
                       " blocks_left = %ld\n", el_size, count, blocklen, blocks_left));

    while (blocks_left) {
        char *src, *dest;

        MPIR_Assert(curblock < count);

        cbufp = (char *) paramp->userbuf + rel_off + offsetarray[curblock];

        /* there was some casting going on here at one time but now all types
         * are promoted ot big values */
        if (blocklen > blocks_left)
            blocklen = blocks_left;

        if (paramp->direction == M2M_TO_USERBUF) {
            src = paramp->streambuf;
            dest = cbufp;
        } else {
            src = cbufp;
            dest = paramp->streambuf;
        }

        /* note: macro modifies dest buffer ptr, so we must reset */
        if (el_size == 8 MPIR_ALIGN8_TEST(src, dest)) {
            MPII_COPY_FROM_VEC(src, dest, 0, int64_t, blocklen, 1);
        } else if (el_size == 4 MPIR_ALIGN4_TEST(src, dest)) {
            MPII_COPY_FROM_VEC(src, dest, 0, int32_t, blocklen, 1);
        } else if (el_size == 2) {
            MPII_COPY_FROM_VEC(src, dest, 0, int16_t, blocklen, 1);
        } else {
            MPIR_Memcpy(dest, src, (MPI_Aint) blocklen * el_size);
            DBG_SEGMENT(printf
                        ("blkidx m3m:memcpy(%p,%p,%d)\n", dest, src, (int) (blocklen * el_size)));
        }

        paramp->streambuf += (MPI_Aint) blocklen *el_size;
        blocks_left -= blocklen;
        curblock++;
    }

    return 0;
}

static int index_m2m(MPI_Aint * blocks_p,
                     MPI_Aint count,
                     MPI_Aint * blockarray,
                     MPI_Aint * offsetarray,
                     MPI_Datatype el_type,
                     MPI_Aint rel_off, void *bufp ATTRIBUTE((unused)), void *v_paramp)
{
    int curblock = 0;
    MPI_Aint el_size;
    MPI_Aint cur_block_sz, blocks_left = *blocks_p;
    char *cbufp;
    struct MPII_Dataloop_m2m_params *paramp = v_paramp;

    MPIR_Datatype_get_size_macro(el_type, el_size);
    DBG_SEGMENT(printf("index m2m: elsize = %d, count = %d\n", (int) el_size, (int) count));

    while (blocks_left) {
        char *src, *dest;

        MPIR_Assert(curblock < count);
        cur_block_sz = blockarray[curblock];

        cbufp = (char *) paramp->userbuf + rel_off + offsetarray[curblock];

        if (cur_block_sz > blocks_left)
            cur_block_sz = blocks_left;

        if (paramp->direction == M2M_TO_USERBUF) {
            src = paramp->streambuf;
            dest = cbufp;
        } else {
            src = cbufp;
            dest = paramp->streambuf;
        }

        /* note: macro modifies dest buffer ptr, so we must reset */
        if (el_size == 8 MPIR_ALIGN8_TEST(src, dest)) {
            MPII_COPY_FROM_VEC(src, dest, 0, int64_t, cur_block_sz, 1);
        } else if (el_size == 4 MPIR_ALIGN4_TEST(src, dest)) {
            MPII_COPY_FROM_VEC(src, dest, 0, int32_t, cur_block_sz, 1);
        } else if (el_size == 2) {
            MPII_COPY_FROM_VEC(src, dest, 0, int16_t, cur_block_sz, 1);
        } else {
            MPIR_Memcpy(dest, src, cur_block_sz * el_size);
        }

        paramp->streambuf += cur_block_sz * el_size;
        blocks_left -= cur_block_sz;
        curblock++;
    }

    return 0;
}

static int contig_pack_external32_to_buf(MPI_Aint * blocks_p,
                                         MPI_Datatype el_type,
                                         MPI_Aint rel_off, void *bufp, void *v_paramp)
{
    int src_el_size, dest_el_size;
    struct piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONTIG_PACK_EXTERNAL32_TO_BUF);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONTIG_PACK_EXTERNAL32_TO_BUF);

    src_el_size = MPIR_Datatype_get_basic_size(el_type);
    dest_el_size = MPII_Dataloop_get_basic_size_external32(el_type);
    MPIR_Assert(dest_el_size);

    /*
     * h  = handle value
     * do = datatype buffer offset
     * dp = datatype buffer pointer
     * bp = pack buffer pointer (current location, incremented as we go)
     * sz = size of datatype (guess we could get this from handle value if
     *      we wanted...)
     */
#ifdef MPID_SP_VERBOSE
    dbg_printf("\t[contig pack [external32]: do=%d, dp=%x, bp=%x, "
               "src_el_sz=%d, dest_el_sz=%d, blksz=%d]\n",
               rel_off,
               (unsigned) bufp,
               (unsigned) paramp->u.pack.pack_buffer, src_el_size, dest_el_size, (int) *blocks_p);
#endif

    /* TODO: DEAL WITH CASE WHERE ALL DATA DOESN'T FIT! */
    if ((src_el_size == dest_el_size) && (src_el_size == 1)) {
        MPIR_Memcpy(paramp->u.pack.pack_buffer, ((char *) bufp) + rel_off, *blocks_p);
    } else if (is_float_type(el_type)) {
        external32_float_convert(paramp->u.pack.pack_buffer,
                                 ((char *) bufp) + rel_off, dest_el_size, src_el_size, *blocks_p);
    } else {
        external32_basic_convert(paramp->u.pack.pack_buffer,
                                 ((char *) bufp) + rel_off, dest_el_size, src_el_size, *blocks_p);
    }
    paramp->u.pack.pack_buffer += (dest_el_size * (*blocks_p));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONTIG_PACK_EXTERNAL32_TO_BUF);
    return 0;
}

static int contig_unpack_external32_to_buf(MPI_Aint * blocks_p,
                                           MPI_Datatype el_type,
                                           MPI_Aint rel_off, void *bufp, void *v_paramp)
{
    int src_el_size, dest_el_size;
    struct piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONTIG_UNPACK_EXTERNAL32_TO_BUF);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONTIG_UNPACK_EXTERNAL32_TO_BUF);

    src_el_size = MPIR_Datatype_get_basic_size(el_type);
    dest_el_size = MPII_Dataloop_get_basic_size_external32(el_type);
    MPIR_Assert(dest_el_size);

    /*
     * h  = handle value
     * do = datatype buffer offset
     * dp = datatype buffer pointer
     * up = unpack buffer pointer (current location, incremented as we go)
     * sz = size of datatype (guess we could get this from handle value if
     *      we wanted...)
     */
#ifdef MPID_SP_VERBOSE
    dbg_printf("\t[contig unpack [external32]: do=%d, dp=%x, up=%x, "
               "src_el_sz=%d, dest_el_sz=%d, blksz=%d]\n",
               rel_off,
               (unsigned) bufp,
               (unsigned) paramp->u.unpack.unpack_buffer,
               src_el_size, dest_el_size, (int) *blocks_p);
#endif

    /* TODO: DEAL WITH CASE WHERE ALL DATA DOESN'T FIT! */
    if ((src_el_size == dest_el_size) && (src_el_size == 1)) {
        MPIR_Memcpy(((char *) bufp) + rel_off, paramp->u.unpack.unpack_buffer, *blocks_p);
    } else if (is_float_type(el_type)) {
        external32_float_convert(((char *) bufp) + rel_off,
                                 paramp->u.unpack.unpack_buffer,
                                 dest_el_size, src_el_size, *blocks_p);
    } else {
        external32_basic_convert(((char *) bufp) + rel_off,
                                 paramp->u.unpack.unpack_buffer,
                                 dest_el_size, src_el_size, *blocks_p);
    }
    paramp->u.unpack.unpack_buffer += (dest_el_size * (*blocks_p));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONTIG_UNPACK_EXTERNAL32_TO_BUF);
    return 0;
}

void MPIR_Segment_pack_external32(struct MPIR_Segment *segp,
                                  MPI_Aint first, MPI_Aint * lastp, void *pack_buffer)
{
    struct piece_params pack_params;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SEGMENT_PACK_EXTERNAL32);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SEGMENT_PACK_EXTERNAL32);

    pack_params.u.pack.pack_buffer = (void *) pack_buffer;
    MPII_Segment_manipulate(segp, first, lastp, contig_pack_external32_to_buf, NULL,    /* MPIR_Segment_vector_pack_external32_to_buf, */
                            NULL,       /* blkidx */
                            NULL,       /* MPIR_Segment_index_pack_external32_to_buf, */
                            MPII_Dataloop_get_basic_size_external32, &pack_params);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_PACK_EXTERNAL32);
    return;
}

void MPIR_Segment_unpack_external32(struct MPIR_Segment *segp,
                                    MPI_Aint first, MPI_Aint * lastp, const void *unpack_buffer)
{
    struct piece_params pack_params;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SEGMENT_UNPACK_EXTERNAL32);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SEGMENT_UNPACK_EXTERNAL32);

    pack_params.u.unpack.unpack_buffer = unpack_buffer;
    MPII_Segment_manipulate(segp, first, lastp, contig_unpack_external32_to_buf, NULL,  /* MPIR_Segment_vector_unpack_external32_to_buf, */
                            NULL,       /* blkidx */
                            NULL,       /* MPIR_Segment_index_unpack_external32_to_buf, */
                            MPII_Dataloop_get_basic_size_external32, &pack_params);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_UNPACK_EXTERNAL32);
    return;
}

void MPIR_Type_access_contents(MPI_Datatype type,
                               int **ints_p, MPI_Aint ** aints_p, MPI_Datatype ** types_p)
{
    int nr_ints, nr_aints, nr_types, combiner;
    int types_sz, struct_sz, ints_sz, epsilon;
    MPIR_Datatype *dtp;
    MPIR_Datatype_contents *cp;

    MPIR_Type_get_envelope(type, &nr_ints, &nr_aints, &nr_types, &combiner);

    /* hardcoded handling of MPICH contents format... */
    MPIR_Datatype_get_ptr(type, dtp);
    MPIR_Assert(dtp != NULL);

    cp = dtp->contents;
    MPIR_Assert(cp != NULL);

    struct_sz = sizeof(MPIR_Datatype_contents);
    types_sz = nr_types * sizeof(MPI_Datatype);
    ints_sz = nr_ints * sizeof(int);

    if ((epsilon = struct_sz % MAX_ALIGNMENT)) {
        struct_sz += MAX_ALIGNMENT - epsilon;
    }
    if ((epsilon = types_sz % MAX_ALIGNMENT)) {
        types_sz += MAX_ALIGNMENT - epsilon;
    }
    if ((epsilon = ints_sz % MAX_ALIGNMENT)) {
        ints_sz += MAX_ALIGNMENT - epsilon;
    }
    *types_p = (MPI_Datatype *) (((char *) cp) + struct_sz);
    *ints_p = (int *) (((char *) (*types_p)) + types_sz);
    *aints_p = (MPI_Aint *) (((char *) (*ints_p)) + ints_sz);
    /* end of hardcoded handling of MPICH contents format */

    return;
}

/* FIXME: Is this routine complete?  Why is it needed? If it is needed, it
   must have a comment that describes why it is needed and the arguments
   must have ATTRIBUTE((unused)) */
void MPIR_Type_release_contents(MPI_Datatype type,
                                int **ints_p, MPI_Aint ** aints_p, MPI_Datatype ** types_p)
{
    return;
}

/* MPIR_Segment_to_iov
*
* Parameters:
* segp    - pointer to segment structure
* first   - first byte in segment to pack
* lastp   - in/out parameter describing last byte to pack (and afterwards
*           the last byte _actually_ packed)
*           NOTE: actually returns index of byte _after_ last one packed
* vectorp - pointer to (off, len) pairs to fill in
* lengthp - in/out parameter describing length of array (and afterwards
*           the amount of the array that has actual data)
*/
void MPIR_Segment_to_iov(struct MPIR_Segment *segp,
                         MPI_Aint first, MPI_Aint * lastp, struct iovec *vectorp, int *lengthp)
{
    struct piece_params packvec_params;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SEGMENT_TO_IOV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SEGMENT_TO_IOV);

    packvec_params.u.pack_vector.vectorp = vectorp;
    packvec_params.u.pack_vector.index = 0;
    packvec_params.u.pack_vector.length = *lengthp;

    MPIR_Assert(*lengthp > 0);

    MPII_Segment_manipulate(segp, first, lastp, contig_pack_to_iov, vector_pack_to_iov, NULL,   /* blkidx fn */
                            NULL,       /* index fn */
                            NULL, &packvec_params);

    /* last value already handled by MPII_Segment_manipulate */
    *lengthp = packvec_params.u.pack_vector.index;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_TO_IOV);
    return;
}


/*
* EVERYTHING BELOW HERE IS USED ONLY WITHIN THIS FILE
*/

/********** FUNCTIONS FOR CREATING AN IOV DESCRIBING BUFFER **********/

static int contig_pack_to_iov(MPI_Aint * blocks_p,
                              MPI_Datatype el_type, MPI_Aint rel_off, void *bufp, void *v_paramp)
{
    int el_size, last_idx;
    MPI_Aint size;
    intptr_t last_end = 0;
    struct piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONTIG_PACK_TO_IOV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONTIG_PACK_TO_IOV);

    el_size = MPIR_Datatype_get_basic_size(el_type);
    size = *blocks_p * (MPI_Aint) el_size;

    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE, (MPL_DBG_FDEST,
                                                 "    contig to vec: do=" MPI_AINT_FMT_DEC_SPEC
                                                 ", dp=%p, ind=%d, sz=%d, blksz="
                                                 MPI_AINT_FMT_DEC_SPEC, (MPI_Aint) rel_off, bufp,
                                                 paramp->u.pack_vector.index, el_size,
                                                 (MPI_Aint) * blocks_p));

    last_idx = paramp->u.pack_vector.index - 1;
    if (last_idx >= 0) {
        last_end = ((intptr_t) paramp->u.pack_vector.vectorp[last_idx].iov_base) +
            paramp->u.pack_vector.vectorp[last_idx].iov_len;
    }

    if ((last_idx == paramp->u.pack_vector.length - 1) && (last_end != ((intptr_t) bufp + rel_off))) {
        /* we have used up all our entries, and this region doesn't fit on
         * the end of the last one.  setting blocks to 0 tells manipulation
         * function that we are done (and that we didn't process any blocks).
         */
        *blocks_p = 0;
        MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONTIG_PACK_TO_IOV);
        return 1;
    } else if (last_idx >= 0 && (last_end == ((intptr_t) bufp + rel_off))) {
        /* add this size to the last vector rather than using up another one */
        paramp->u.pack_vector.vectorp[last_idx].iov_len += size;
    } else {
        paramp->u.pack_vector.vectorp[last_idx + 1].iov_base = (void *) ((intptr_t) bufp + rel_off);
        paramp->u.pack_vector.vectorp[last_idx + 1].iov_len = size;
        paramp->u.pack_vector.index++;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONTIG_PACK_TO_IOV);
    return 0;
}

/* vector_pack_to_iov
 *
 * Input Parameters:
 * blocks_p - [inout] pointer to a count of blocks (total, for all noncontiguous pieces)
 * count    - # of noncontiguous regions
 * blksz    - size of each noncontiguous region
 * stride   - distance in bytes from start of one region to start of next
 * el_type - elemental type (e.g. MPI_INT)
 * ...
 *
 * Note: this is only called when the starting position is at the beginning
 * of a whole block in a vector type.
 */
static int vector_pack_to_iov(MPI_Aint * blocks_p, MPI_Aint count, MPI_Aint blksz, MPI_Aint stride, MPI_Datatype el_type, MPI_Aint rel_off,     /* offset into buffer */
                              void *bufp,       /* start of buffer */
                              void *v_paramp)
{
    int i;
    MPI_Aint size, blocks_left, basic_size;
    struct piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_VECTOR_PACK_TO_IOV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_VECTOR_PACK_TO_IOV);

    basic_size = (MPI_Aint) MPIR_Datatype_get_basic_size(el_type);
    blocks_left = *blocks_p;

    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE, (MPL_DBG_FDEST,
                                                 "    vector to vec: do=" MPI_AINT_FMT_DEC_SPEC
                                                 ", dp=%p"
                                                 ", len=" MPI_AINT_FMT_DEC_SPEC
                                                 ", ind=" MPI_AINT_FMT_DEC_SPEC
                                                 ", ct=" MPI_AINT_FMT_DEC_SPEC
                                                 ", blksz=" MPI_AINT_FMT_DEC_SPEC
                                                 ", str=" MPI_AINT_FMT_DEC_SPEC
                                                 ", blks=" MPI_AINT_FMT_DEC_SPEC,
                                                 (MPI_Aint) rel_off,
                                                 bufp,
                                                 (MPI_Aint) paramp->u.pack_vector.length,
                                                 (MPI_Aint) paramp->u.pack_vector.index,
                                                 count,
                                                 blksz, (MPI_Aint) stride, (MPI_Aint) * blocks_p));

    for (i = 0; i < count && blocks_left > 0; i++) {
        int last_idx;
        intptr_t last_end = 0;

        if (blocks_left > (MPI_Aint) blksz) {
            size = ((MPI_Aint) blksz) * basic_size;
            blocks_left -= (MPI_Aint) blksz;
        } else {
            /* last pass */
            size = blocks_left * basic_size;
            blocks_left = 0;
        }

        last_idx = paramp->u.pack_vector.index - 1;
        if (last_idx >= 0) {
            last_end = ((intptr_t) paramp->u.pack_vector.vectorp[last_idx].iov_base) +
                paramp->u.pack_vector.vectorp[last_idx].iov_len;
        }

        if ((last_idx == paramp->u.pack_vector.length - 1) &&
            (last_end != ((intptr_t) bufp + rel_off))) {
            /* we have used up all our entries, and this one doesn't fit on
             * the end of the last one.
             */
            *blocks_p -= (blocks_left + (size / basic_size));
#ifdef MPID_SP_VERBOSE
            MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                            (MPL_DBG_FDEST,
                             "\t[vector to vec exiting (1): next ind = %d, " MPI_AINT_FMT_DEC_SPEC
                             " blocks processed.\n", paramp->u.pack_vector.index,
                             (MPI_Aint) * blocks_p));
#endif
            MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_VECTOR_PACK_TO_IOV);
            return 1;
        } else if (last_idx >= 0 && (last_end == ((intptr_t) bufp + rel_off))) {
            /* add this size to the last vector rather than using up new one */
            paramp->u.pack_vector.vectorp[last_idx].iov_len += size;
        } else {
            paramp->u.pack_vector.vectorp[last_idx + 1].iov_base =
                (void *) ((intptr_t) bufp + rel_off);
            paramp->u.pack_vector.vectorp[last_idx + 1].iov_len = size;
            paramp->u.pack_vector.index++;
        }

        rel_off += stride;

    }

#ifdef MPID_SP_VERBOSE
    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                    (MPL_DBG_FDEST,
                     "\t[vector to vec exiting (2): next ind = %d, " MPI_AINT_FMT_DEC_SPEC
                     " blocks processed.\n", paramp->u.pack_vector.index, (MPI_Aint) * blocks_p));
#endif

    /* if we get here then we processed ALL the blocks; don't need to update
     * blocks_p
     */
    MPIR_Assert(blocks_left == 0);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_VECTOR_PACK_TO_IOV);
    return 0;
}
