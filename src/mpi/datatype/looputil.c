/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "datatype.h"
#include "mpir_dataloop.h"
#include "looputil.h"
#include "veccpy.h"

/* MPIR_Segment_piece_params
 *
 * This structure is used to pass function-specific parameters into our
 * segment processing function.  This allows us to get additional parameters
 * to the functions it calls without changing the prototype.
 */
struct MPIR_Segment_piece_params {
    union {
        struct {
            char *pack_buffer;
        } pack;
        struct {
            DLOOP_VECTOR *vectorp;
            int index;
            int length;
        } pack_vector;
        struct {
            int64_t *offp;
            DLOOP_Size *sizep; /* see notes in Segment_flatten header */
            int index;
            int length;
        } flatten;
        struct {
            char *last_loc;
            int count;
        } contig_blocks;
        struct {
            char *unpack_buffer;
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
static void setPrint( void ) {
    char *s = getenv( "MPICH_DATALOOP_PRINT" );
    if (s && (strcmp(s,"yes")==0 || strcmp(s,"YES") == 0)) {
        printSegment = 1;
    }
    else {
        printSegment = 0;
    }
}
#define DBG_SEGMENT(_a) do { if (printSegment < 0) setPrint(); \
        if (printSegment) { _a; } } while( 0 )
#else
#define DBG_SEGMENT(_a)
#endif

/* NOTE: bufp values are unused, ripe for removal */

int MPIR_Segment_contig_m2m(DLOOP_Offset *blocks_p,
                                       DLOOP_Type el_type,
                                       DLOOP_Offset rel_off,
                                       void *bufp,
                                       void *v_paramp);
int MPIR_Segment_vector_m2m(DLOOP_Offset *blocks_p,
                                       DLOOP_Count count,
                                       DLOOP_Count blksz,
                                       DLOOP_Offset stride,
                                       DLOOP_Type el_type,
                                       DLOOP_Offset rel_off,
                                       void *bufp,
                                       void *v_paramp);
int MPIR_Segment_blkidx_m2m(DLOOP_Offset *blocks_p,
                                       DLOOP_Count count,
                                       DLOOP_Count blocklen,
                                       DLOOP_Offset *offsetarray,
                                       DLOOP_Type el_type,
                                       DLOOP_Offset rel_off,
                                       void *bufp,
                                       void *v_paramp);
int MPIR_Segment_index_m2m(DLOOP_Offset *blocks_p,
                                      DLOOP_Count count,
                                      DLOOP_Count *blockarray,
                                      DLOOP_Offset *offsetarray,
                                      DLOOP_Type el_type,
                                      DLOOP_Offset rel_off,
                                      void *bufp,
                                      void *v_paramp);

/* prototypes of internal functions */
static int MPII_Segment_vector_pack_to_iov(DLOOP_Offset *blocks_p,
				       DLOOP_Count count,
				       DLOOP_Size blksz,
				       DLOOP_Offset stride,
				       DLOOP_Type el_type,
				       DLOOP_Offset rel_off,
				       void *bufp,
				       void *v_paramp);

static int MPII_Segment_contig_pack_to_iov(DLOOP_Offset *blocks_p,
                                           DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

static int MPII_Segment_contig_flatten(DLOOP_Offset *blocks_p,
				   DLOOP_Type el_type,
				   DLOOP_Offset rel_off,
				   void *bufp,
				   void *v_paramp);

static int MPII_Segment_vector_flatten(DLOOP_Offset *blocks_p,
				   DLOOP_Count count,
				   DLOOP_Size blksz,
				   DLOOP_Offset stride,
				   DLOOP_Type el_type,
				   DLOOP_Offset rel_off, /* offset into buffer */
				   void *bufp, /* start of buffer */
				   void *v_paramp);

static inline int is_float_type(DLOOP_Type el_type)
{
    return ((el_type == MPI_FLOAT) || (el_type == MPI_DOUBLE) ||
            (el_type == MPI_LONG_DOUBLE) ||
            (el_type == MPI_DOUBLE_PRECISION) ||
            (el_type == MPI_COMPLEX) || (el_type == MPI_DOUBLE_COMPLEX));
/*             (el_type == MPI_REAL4) || (el_type == MPI_REAL8) || */
/*             (el_type == MPI_REAL16)); */
}

static int external32_basic_convert(char *dest_buf,
                                    char *src_buf,
                                    int dest_el_size,
                                    int src_el_size,
                                    DLOOP_Offset count)
{
    char *src_ptr = src_buf, *dest_ptr = dest_buf;
    char *src_end = (char *)(src_buf + ((int)count * src_el_size));

    MPIR_Assert(dest_buf && src_buf);

    if (src_el_size == dest_el_size)
    {
        if (src_el_size == 2)
        {
            while(src_ptr != src_end)
            {
                BASIC_convert16((*(TWO_BYTE_BASIC_TYPE *)src_ptr),
                                (*(TWO_BYTE_BASIC_TYPE *)dest_ptr));

                src_ptr += src_el_size;
                dest_ptr += dest_el_size;
            }
        }
        else if (src_el_size == 4)
        {
            while(src_ptr != src_end)
            {
                BASIC_convert32((*(FOUR_BYTE_BASIC_TYPE *)src_ptr),
                                (*(FOUR_BYTE_BASIC_TYPE *)dest_ptr));

                src_ptr += src_el_size;
                dest_ptr += dest_el_size;
            }
        }
        else if (src_el_size == 8)
        {
            while(src_ptr != src_end)
            {
                BASIC_convert64(src_ptr, dest_ptr);

                src_ptr += src_el_size;
                dest_ptr += dest_el_size;
            }
        }
    }
    else
    {
        /* TODO */
        MPL_error_printf( "Conversion of types whose size is not the same as the size in external32 is not supported\n" );
        MPID_Abort( 0, MPI_SUCCESS, 1, "Aborting with internal error" );
        /* There is no way to return an error code, so an abort is the
           only choice (the return value of this routine is not
           an error code) */
    }
    return 0;
}

static int external32_float_convert(char *dest_buf,
                                    char *src_buf,
                                    int dest_el_size,
                                    int src_el_size,
                                    int count)
{
    char *src_ptr = src_buf, *dest_ptr = dest_buf;
    char *src_end = (char *)(src_buf + ((int)count * src_el_size));

    MPIR_Assert(dest_buf && src_buf);

    if (src_el_size == dest_el_size)
    {
        if (src_el_size == 4)
        {
            while(src_ptr != src_end)
            {
                FLOAT_convert((*(FOUR_BYTE_FLOAT_TYPE *)src_ptr),
                              (*(FOUR_BYTE_FLOAT_TYPE *)dest_ptr));

                src_ptr += src_el_size;
                dest_ptr += dest_el_size;
            }
        }
        else if (src_el_size == 8)
        {
            while(src_ptr != src_end)
            {
                FLOAT_convert((*(EIGHT_BYTE_FLOAT_TYPE *)src_ptr),
                              (*(EIGHT_BYTE_FLOAT_TYPE *)dest_ptr));

                src_ptr += src_el_size;
                dest_ptr += dest_el_size;
            }
        }
    }
    else
    {
        /* TODO */
        MPL_error_printf( "Conversion of types whose size is not the same as the size in external32 is not supported\n" );
        MPID_Abort( 0, MPI_SUCCESS, 1, "Aborting with internal error" );
        /* There is no way to return an error code, so an abort is the
           only choice (the return value of this routine is not
           an error code) */
    }
    return 0;
}

/* Segment_init
 *
 * buf    - datatype buffer location
 * count  - number of instances of the datatype in the buffer
 * handle - handle for datatype (could be derived or not)
 * segp   - pointer to previously allocated segment structure
 * flag   - flag indicating which optimizations are valid
 *          should be one of DLOOP_DATALOOP_HOMOGENEOUS, _HETEROGENEOUS,
 *          of _ALL_BYTES.
 *
 * Notes:
 * - Assumes that the segment has been allocated.
 * - Older MPICH code may pass "0" to indicate HETEROGENEOUS or "1" to
 *   indicate HETEROGENEOUS.
 *
 */
int MPIR_Segment_init(const DLOOP_Buffer buf,
                                 DLOOP_Count count,
                                 DLOOP_Handle handle,
                                 struct DLOOP_Segment *segp,
                                 int flag)
{
    DLOOP_Offset elmsize = 0;
    int i, depth = 0;
    int branch_detected = 0;

    struct DLOOP_Dataloop_stackelm *elmp;
    struct DLOOP_Dataloop *dlp = 0, *sblp = &segp->builtin_loop;

    DLOOP_Assert(flag == DLOOP_DATALOOP_HETEROGENEOUS ||
                 flag == DLOOP_DATALOOP_HOMOGENEOUS   ||
                 flag == DLOOP_DATALOOP_ALL_BYTES);

#ifdef DLOOP_DEBUG_MANIPULATE
    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE,VERBOSE,(MPL_DBG_FDEST,"DLOOP_Segment_init: count = %d, buf = %x\n",
                    count, buf));
#endif

    if (!DLOOP_Handle_hasloop_macro(handle)) {
        /* simplest case; datatype has no loop (basic) */

        DLOOP_Handle_get_size_macro(handle, elmsize);

        sblp->kind = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;
        sblp->loop_params.c_t.count = count;
        sblp->loop_params.c_t.dataloop = 0;
        sblp->el_size = elmsize;
        DLOOP_Handle_get_basic_type_macro(handle, sblp->el_type);
        DLOOP_Handle_get_extent_macro(handle, sblp->el_extent);

        dlp = sblp;
        depth = 1;
    }
    else if (count == 0) {
        /* only use the builtin */
        sblp->kind = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;
        sblp->loop_params.c_t.count = 0;
        sblp->loop_params.c_t.dataloop = 0;
        sblp->el_size = 0;
        sblp->el_extent = 0;

        dlp = sblp;
        depth = 1;
    }
    else if (count == 1) {
        /* don't use the builtin */
        DLOOP_Handle_get_loopptr_macro(handle, dlp, flag);
        DLOOP_Handle_get_loopdepth_macro(handle, depth, flag);
    }
    else {
        /* default: need to use builtin to handle contig; must check
         * loop depth first
         */
        DLOOP_Dataloop *oldloop; /* loop from original type, before new count */
        DLOOP_Offset type_size, type_extent;
        DLOOP_Type el_type;

        DLOOP_Handle_get_loopdepth_macro(handle, depth, flag);

        DLOOP_Handle_get_loopptr_macro(handle, oldloop, flag);
        DLOOP_Assert(oldloop != NULL);
        DLOOP_Handle_get_size_macro(handle, type_size);
        DLOOP_Handle_get_extent_macro(handle, type_extent);
        DLOOP_Handle_get_basic_type_macro(handle, el_type);

        if (depth == 1 && ((oldloop->kind & DLOOP_KIND_MASK) == DLOOP_KIND_CONTIG))
        {
            if (type_size == type_extent)
            {
                /* use a contig */
                sblp->kind                     = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;
                sblp->loop_params.c_t.count    = count * oldloop->loop_params.c_t.count;
                sblp->loop_params.c_t.dataloop = NULL;
                sblp->el_size                  = oldloop->el_size;
                sblp->el_extent                = oldloop->el_extent;
                sblp->el_type                  = oldloop->el_type;
            }
            else
            {
                /* use a vector, with extent of original type becoming the stride */
                sblp->kind                      = DLOOP_KIND_VECTOR | DLOOP_FINAL_MASK;
                sblp->loop_params.v_t.count     = count;
                sblp->loop_params.v_t.blocksize = oldloop->loop_params.c_t.count;
                sblp->loop_params.v_t.stride    = type_extent;
                sblp->loop_params.v_t.dataloop  = NULL;
                sblp->el_size                   = oldloop->el_size;
                sblp->el_extent                 = oldloop->el_extent;
                sblp->el_type                   = oldloop->el_type;
            }
        }
        else
        {
            /* general case */
            sblp->kind                     = DLOOP_KIND_CONTIG;
            sblp->loop_params.c_t.count    = count;
            sblp->loop_params.c_t.dataloop = oldloop;
            sblp->el_size                  = type_size;
            sblp->el_extent                = type_extent;
            sblp->el_type                  = el_type;

            depth++; /* we're adding to the depth with the builtin */
            DLOOP_Assert(depth < (DLOOP_MAX_DATATYPE_DEPTH));
        }

        dlp = sblp;
    }

    /* assert instead of return b/c dtype/dloop errorhandling code is inconsistent */
    DLOOP_Assert(depth < (DLOOP_MAX_DATATYPE_DEPTH));

    /* initialize the rest of the segment values */
    segp->handle = handle;
    segp->ptr = (DLOOP_Buffer) buf;
    segp->stream_off = 0;
    segp->cur_sp = 0;
    segp->valid_sp = 0;

    /* initialize the first stackelm in its entirety */
    elmp = &(segp->stackelm[0]);
    DLOOP_Stackelm_load(elmp, dlp, 0);
    branch_detected = elmp->may_require_reloading;

    /* Fill in parameters not set by DLOOP_Stackelm_load */
    elmp->orig_offset = 0;
    elmp->curblock    = elmp->orig_block;
    /* DLOOP_Stackelm_offset assumes correct orig_count, curcount, loop_p */
    elmp->curoffset   = /* elmp->orig_offset + */ DLOOP_Stackelm_offset(elmp);

    i = 1;
    while(!(dlp->kind & DLOOP_FINAL_MASK))
    {
        /* get pointer to next dataloop */
        switch (dlp->kind & DLOOP_KIND_MASK)
        {
            case DLOOP_KIND_CONTIG:
            case DLOOP_KIND_VECTOR:
            case DLOOP_KIND_BLOCKINDEXED:
            case DLOOP_KIND_INDEXED:
                dlp = dlp->loop_params.cm_t.dataloop;
                break;
            case DLOOP_KIND_STRUCT:
                dlp = dlp->loop_params.s_t.dataloop_array[0];
                break;
            default:
                /* --BEGIN ERROR HANDLING-- */
                DLOOP_Assert(0);
                break;
                /* --END ERROR HANDLING-- */
        }

        DLOOP_Assert(i < DLOOP_MAX_DATATYPE_DEPTH);

        /* loop_p, orig_count, orig_block, and curcount are all filled by us now.
         * the rest are filled in at processing time.
         */
        elmp = &(segp->stackelm[i]);

        DLOOP_Stackelm_load(elmp, dlp, branch_detected);
        branch_detected = elmp->may_require_reloading;
        i++;

    }

    segp->valid_sp = depth-1;

    return 0;
}

/* Segment_alloc
 *
 */
struct DLOOP_Segment * MPIR_Segment_alloc(void)
{
    return (struct DLOOP_Segment *) DLOOP_Malloc(sizeof(struct DLOOP_Segment));
}

/* Segment_free
 *
 * Input Parameters:
 * segp - pointer to segment
 */
void MPIR_Segment_free(struct DLOOP_Segment *segp)
{
    DLOOP_Free(segp);
    return;
}

void MPIR_Segment_pack(DLOOP_Segment *segp,
                                  DLOOP_Offset   first,
                                  DLOOP_Offset  *lastp,
                                  void *streambuf)
{
    struct MPIR_m2m_params params; /* defined in dataloop_parts.h */

    DBG_SEGMENT(printf( "Segment_pack...\n" ));
    /* experimenting with discarding buf value in the segment, keeping in
     * per-use structure instead. would require moving the parameters around a
     * bit.
     */
    params.userbuf   = segp->ptr;
    params.streambuf = streambuf;
    params.direction = DLOOP_M2M_FROM_USERBUF;

    MPIR_Segment_manipulate(segp, first, lastp,
                                       MPIR_Segment_contig_m2m,
                                       MPIR_Segment_vector_m2m,
                                       MPIR_Segment_blkidx_m2m,
                                       MPIR_Segment_index_m2m,
                                       NULL, /* size fn */
                                       &params);
    return;
}

void MPIR_Segment_unpack(DLOOP_Segment *segp,
                                    DLOOP_Offset   first,
                                    DLOOP_Offset  *lastp,
                                    void *streambuf)
{
    struct MPIR_m2m_params params;

    DBG_SEGMENT(printf( "Segment_unpack...\n" ));
    /* experimenting with discarding buf value in the segment, keeping in
     * per-use structure instead. would require moving the parameters around a
     * bit.
     */
    params.userbuf   = segp->ptr;
    params.streambuf = streambuf;
    params.direction = DLOOP_M2M_TO_USERBUF;

    MPIR_Segment_manipulate(segp, first, lastp,
                                       MPIR_Segment_contig_m2m,
                                       MPIR_Segment_vector_m2m,
                                       MPIR_Segment_blkidx_m2m,
                                       MPIR_Segment_index_m2m,
                                       NULL, /* size fn */
                                       &params);
    return;
}

/* PIECE FUNCTIONS BELOW */

int MPIR_Segment_contig_m2m(DLOOP_Offset *blocks_p,
                                       DLOOP_Type el_type,
                                       DLOOP_Offset rel_off,
                                       void *bufp ATTRIBUTE((unused)),
                                       void *v_paramp)
{
    DLOOP_Offset el_size; /* DLOOP_Count? */
    DLOOP_Offset size;
    struct MPIR_m2m_params *paramp = v_paramp;

    DLOOP_Handle_get_size_macro(el_type, el_size);
    size = *blocks_p * el_size;

    DBG_SEGMENT(printf( "element type = %lx\n", (long)el_type ));
    DBG_SEGMENT(printf( "contig m2m: elsize = %d, size = %d\n", (int)el_size, (int)size ));
#ifdef MPID_SU_VERBOSE
    dbg_printf("\t[contig unpack: do=" DLOOP_OFFSET_FMT_DEC_SPEC ", dp=%x, bp=%x, sz=" DLOOP_OFFSET_FMT_DEC_SPEC ", blksz=" DLOOP_OFFSET_FMT_DEC_SPEC "]\n",
               rel_off,
               (unsigned) bufp,
               (unsigned) paramp->u.unpack.unpack_buffer,
               el_size,
               *blocks_p);
#endif

    if (paramp->direction == DLOOP_M2M_TO_USERBUF) {
        /* Ensure that pointer increment fits in a pointer */
        /* userbuf is a pointer (not a displacement) since it is being
         * used on a memcpy */
        DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->userbuf)) + rel_off);
        DLOOP_Memcpy((char *) paramp->userbuf + rel_off, paramp->streambuf, size);
    }
    else {
        /* Ensure that pointer increment fits in a pointer */
        /* userbuf is a pointer (not a displacement) since it is being used on a memcpy */
        DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->userbuf)) + rel_off);
        DLOOP_Memcpy(paramp->streambuf, (char *) paramp->userbuf + rel_off, size);
    }
    /* Ensure that pointer increment fits in a pointer */
    /* streambuf is a pointer (not a displacement) since it was used on a memcpy */
    DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->streambuf)) + size);
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
int MPIR_Segment_vector_m2m(DLOOP_Offset *blocks_p,
                                       DLOOP_Count count ATTRIBUTE((unused)),
                                       DLOOP_Count blksz,
                                       DLOOP_Offset stride,
                                       DLOOP_Type el_type,
                                       DLOOP_Offset rel_off, /* offset into buffer */
                                       void *bufp ATTRIBUTE((unused)),
                                       void *v_paramp)
{
    DLOOP_Count i;
    DLOOP_Offset el_size, whole_count, blocks_left;
    struct MPIR_m2m_params *paramp = v_paramp;
    char *cbufp;

    /* Ensure that pointer increment fits in a pointer */
    /* userbuf is a pointer (not a displacement) since it is being used for a memory copy */
    DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->userbuf)) + rel_off);
    cbufp = (char*) paramp->userbuf + rel_off;
    DLOOP_Handle_get_size_macro(el_type, el_size);
    DBG_SEGMENT(printf( "vector m2m: elsize = %d, count = %d, stride = %d, blocksize = %d\n", (int)el_size, (int)count, (int)stride, (int)blksz ));

    whole_count = (DLOOP_Count)((blksz > 0) ? (*blocks_p / (DLOOP_Offset) blksz) : 0);
    blocks_left = (DLOOP_Count)((blksz > 0) ? (*blocks_p % (DLOOP_Offset) blksz) : 0);

    if (paramp->direction == DLOOP_M2M_TO_USERBUF) {
        if (el_size == 8
            MPIR_ALIGN8_TEST(paramp->streambuf,cbufp))
        {
            MPII_COPY_TO_VEC(paramp->streambuf, cbufp, stride,
                              int64_t, blksz, whole_count);
            MPII_COPY_TO_VEC(paramp->streambuf, cbufp, 0,
                              int64_t, blocks_left, 1);
        }
        else if (el_size == 4
                 MPIR_ALIGN4_TEST(paramp->streambuf,cbufp))
        {
            MPII_COPY_TO_VEC((paramp->streambuf), cbufp, stride,
                              int32_t, blksz, whole_count);
            MPII_COPY_TO_VEC(paramp->streambuf, cbufp, 0,
                              int32_t, blocks_left, 1);
        }
        else if (el_size == 2) {
            MPII_COPY_TO_VEC(paramp->streambuf, cbufp, stride,
                              int16_t, blksz, whole_count);
            MPII_COPY_TO_VEC(paramp->streambuf, cbufp, 0,
                              int16_t, blocks_left, 1);
        }
        else {
            for (i=0; i < whole_count; i++) {
                DLOOP_Memcpy(cbufp, paramp->streambuf, ((DLOOP_Offset) blksz) * el_size);
                DBG_SEGMENT(printf("vec: memcpy %p %p %d\n", cbufp,
                                   paramp->streambuf,
                                   (int)(blksz * el_size) ));
                /* Ensure that pointer increment fits in a pointer */
                /* streambuf is a pointer (not a displacement) since it is being used for a memory copy */
                DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->streambuf)) +
                                                 ((DLOOP_Offset) blksz) * el_size);
                paramp->streambuf += ((DLOOP_Offset) blksz) * el_size;

                DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (cbufp)) + stride);
                cbufp += stride;
            }
            if (blocks_left) {
                DLOOP_Memcpy(cbufp, paramp->streambuf, ((DLOOP_Offset) blocks_left) * el_size);
                DBG_SEGMENT(printf("vec(left): memcpy %p %p %d\n", cbufp,
                                   paramp->streambuf,
                                   (int)(blocks_left * el_size) ));
                /* Ensure that pointer increment fits in a pointer */
                /* streambuf is a pointer (not a displacement) since
                 * it is being used for a memory copy */
                DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->streambuf)) +
                                                 ((DLOOP_Offset) blocks_left) * el_size);
                paramp->streambuf += ((DLOOP_Offset) blocks_left) * el_size;
            }
        }
    }
    else /* M2M_FROM_USERBUF */ {
        if (el_size == 8
            MPIR_ALIGN8_TEST(cbufp,paramp->streambuf))
        {
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, stride,
                                int64_t, blksz, whole_count);
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, 0,
                                int64_t, blocks_left, 1);
        }
        else if (el_size == 4
                 MPIR_ALIGN4_TEST(cbufp,paramp->streambuf))
        {
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, stride,
                                int32_t, blksz, whole_count);
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, 0,
                                int32_t, blocks_left, 1);
        }
        else if (el_size == 2) {
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, stride,
                                int16_t, blksz, whole_count);
            MPII_COPY_FROM_VEC(cbufp, paramp->streambuf, 0,
                                int16_t, blocks_left, 1);
        }
        else {
            for (i=0; i < whole_count; i++) {
                DLOOP_Memcpy(paramp->streambuf, cbufp, (DLOOP_Offset) blksz * el_size);
                /* Ensure that pointer increment fits in a pointer */
                /* streambuf is a pointer (not a displacement) since
                 * it is being used for a memory copy */
                DBG_SEGMENT(printf("vec: memcpy %p %p %d\n",
                                   paramp->streambuf, cbufp,
                                   (int)(blksz * el_size) ));
                DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->streambuf)) +
                                                 (DLOOP_Offset) blksz * el_size);
                paramp->streambuf += (DLOOP_Offset) blksz * el_size;
                cbufp += stride;
            }
            if (blocks_left) {
                DLOOP_Memcpy(paramp->streambuf, cbufp, (DLOOP_Offset) blocks_left * el_size);
                DBG_SEGMENT(printf("vec(left): memcpy %p %p %d\n",
                                   paramp->streambuf, cbufp,
                                   (int)(blocks_left * el_size) ));
                /* Ensure that pointer increment fits in a pointer */
                /* streambuf is a pointer (not a displacement) since
                 * it is being used for a memory copy */
                DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->streambuf)) +
                                                 (DLOOP_Offset) blocks_left * el_size);
                paramp->streambuf += (DLOOP_Offset) blocks_left * el_size;
            }
        }
    }

    return 0;
}

/* MPIR_Segment_blkidx_m2m
 */
int MPIR_Segment_blkidx_m2m(DLOOP_Offset *blocks_p,
                                       DLOOP_Count count,
                                       DLOOP_Count blocklen,
                                       DLOOP_Offset *offsetarray,
                                       DLOOP_Type el_type,
                                       DLOOP_Offset rel_off,
                                       void *bufp ATTRIBUTE((unused)),
                                       void *v_paramp)
{
    DLOOP_Count curblock = 0;
    DLOOP_Offset el_size;
    DLOOP_Offset blocks_left = *blocks_p;
    char *cbufp;
    struct MPIR_m2m_params *paramp = v_paramp;

    DLOOP_Handle_get_size_macro(el_type, el_size);
    DBG_SEGMENT( printf("blkidx m2m: elsize = %ld, count = %ld, blocklen = %ld,"
                            " blocks_left = %ld\n",
                    el_size, count, blocklen, blocks_left ));

    while (blocks_left) {
        char *src, *dest;

        DLOOP_Assert(curblock < count);

        /* Ensure that pointer increment fits in a pointer */
        /* userbuf is a pointer (not a displacement) since it is being
         * used for a memory copy */
        DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->userbuf)) +
                                         rel_off + offsetarray[curblock]);
        cbufp = (char*) paramp->userbuf + rel_off + offsetarray[curblock];

        /* there was some casting going on here at one time but now all types
         * are promoted ot big values */
        if ( blocklen > blocks_left)
            blocklen = blocks_left;

        if (paramp->direction == DLOOP_M2M_TO_USERBUF) {
            src  = paramp->streambuf;
            dest = cbufp;
        }
        else {
            src  = cbufp;
            dest = paramp->streambuf;
        }

        /* note: macro modifies dest buffer ptr, so we must reset */
        if (el_size == 8
            MPIR_ALIGN8_TEST(src, dest))
        {
            MPII_COPY_FROM_VEC(src, dest, 0, int64_t, blocklen, 1);
        }
        else if (el_size == 4
                 MPIR_ALIGN4_TEST(src,dest))
        {
            MPII_COPY_FROM_VEC(src, dest, 0, int32_t, blocklen, 1);
        }
        else if (el_size == 2) {
            MPII_COPY_FROM_VEC(src, dest, 0, int16_t, blocklen, 1);
        }
        else {
            DLOOP_Memcpy(dest, src, (DLOOP_Offset) blocklen * el_size);
            DBG_SEGMENT(printf( "blkidx m3m:memcpy(%p,%p,%d)\n",dest,src,(int)(blocklen*el_size)));
        }

        /* Ensure that pointer increment fits in a pointer */
        /* streambuf is a pointer (not a displacement) since it is
         * being used for a memory copy */
        DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->streambuf)) +
                                         (DLOOP_Offset) blocklen * el_size);
        paramp->streambuf += (DLOOP_Offset) blocklen * el_size;
        blocks_left -= blocklen;
        curblock++;
    }

    return 0;
}

/* MPIR_Segment_index_m2m
 */
int MPIR_Segment_index_m2m(DLOOP_Offset *blocks_p,
                                      DLOOP_Count count,
                                      DLOOP_Count *blockarray,
                                      DLOOP_Offset *offsetarray,
                                      DLOOP_Type el_type,
                                      DLOOP_Offset rel_off,
                                      void *bufp ATTRIBUTE((unused)),
                                      void *v_paramp)
{
    int curblock = 0;
    DLOOP_Offset el_size;
    DLOOP_Offset cur_block_sz, blocks_left = *blocks_p;
    char *cbufp;
    struct MPIR_m2m_params *paramp = v_paramp;

    DLOOP_Handle_get_size_macro(el_type, el_size);
    DBG_SEGMENT(printf( "index m2m: elsize = %d, count = %d\n", (int)el_size, (int)count ));

    while (blocks_left) {
        char *src, *dest;

        DLOOP_Assert(curblock < count);
        cur_block_sz = blockarray[curblock];

        /* Ensure that pointer increment fits in a pointer */
        /* userbuf is a pointer (not a displacement) since it is being
         * used for a memory copy */
        DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->userbuf)) +
                                         rel_off + offsetarray[curblock]);
        cbufp = (char*) paramp->userbuf + rel_off + offsetarray[curblock];

        if (cur_block_sz > blocks_left) cur_block_sz = blocks_left;

        if (paramp->direction == DLOOP_M2M_TO_USERBUF) {
            src  = paramp->streambuf;
            dest = cbufp;
        }
        else {
            src  = cbufp;
            dest = paramp->streambuf;
        }

        /* note: macro modifies dest buffer ptr, so we must reset */
        if (el_size == 8
            MPIR_ALIGN8_TEST(src, dest))
        {
            MPII_COPY_FROM_VEC(src, dest, 0, int64_t, cur_block_sz, 1);
        }
        else if (el_size == 4
                 MPIR_ALIGN4_TEST(src,dest))
        {
            MPII_COPY_FROM_VEC(src, dest, 0, int32_t, cur_block_sz, 1);
        }
        else if (el_size == 2) {
            MPII_COPY_FROM_VEC(src, dest, 0, int16_t, cur_block_sz, 1);
        }
        else {
            DLOOP_Memcpy(dest, src, cur_block_sz * el_size);
        }

        /* Ensure that pointer increment fits in a pointer */
        /* streambuf is a pointer (not a displacement) since it is
         * being used for a memory copy */
        DLOOP_Ensure_Offset_fits_in_pointer((DLOOP_VOID_PTR_CAST_TO_OFFSET (paramp->streambuf)) +
                                         cur_block_sz * el_size);
        paramp->streambuf += cur_block_sz * el_size;
        blocks_left -= cur_block_sz;
        curblock++;
    }

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPII_Segment_contig_pack_external32_to_buf
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPII_Segment_contig_pack_external32_to_buf(DLOOP_Offset *blocks_p,
                                                      DLOOP_Type el_type,
                                                      DLOOP_Offset rel_off,
                                                      void *bufp,
                                                      void *v_paramp)
{
    int src_el_size, dest_el_size;
    struct MPIR_Segment_piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPIR_STATE_MPID_SEGMENT_CONTIG_PACK_EXTERNAL32_TO_BUF);

    MPIR_FUNC_VERBOSE_ENTER(MPIR_STATE_MPID_SEGMENT_CONTIG_PACK_EXTERNAL32_TO_BUF);

    src_el_size = MPIR_Datatype_get_basic_size(el_type);
    dest_el_size = MPII_Datatype_get_basic_size_external32(el_type);
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
               (unsigned) paramp->u.pack.pack_buffer,
               src_el_size,
               dest_el_size,
               (int) *blocks_p);
#endif

    /* TODO: DEAL WITH CASE WHERE ALL DATA DOESN'T FIT! */
    if ((src_el_size == dest_el_size) && (src_el_size == 1))
    {
        MPIR_Memcpy(paramp->u.pack.pack_buffer,
               ((char *) bufp) + rel_off, *blocks_p);
    }
    else if (is_float_type(el_type))
    {
        external32_float_convert(paramp->u.pack.pack_buffer,
                                 ((char *) bufp) + rel_off,
                                 dest_el_size, src_el_size, *blocks_p);
    }
    else
    {
        external32_basic_convert(paramp->u.pack.pack_buffer,
                                 ((char *) bufp) + rel_off,
                                 dest_el_size, src_el_size, *blocks_p);
    }
    paramp->u.pack.pack_buffer += (dest_el_size * (*blocks_p));

    MPIR_FUNC_VERBOSE_EXIT(MPIR_STATE_MPID_SEGMENT_CONTIG_PACK_EXTERNAL32_TO_BUF);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPII_Segment_contig_unpack_external32_to_buf
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPII_Segment_contig_unpack_external32_to_buf(DLOOP_Offset *blocks_p,
                                                        DLOOP_Type el_type,
                                                        DLOOP_Offset rel_off,
                                                        void *bufp,
                                                        void *v_paramp)
{
    int src_el_size, dest_el_size;
    struct MPIR_Segment_piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPIR_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF);

    MPIR_FUNC_VERBOSE_ENTER(MPIR_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF);

    src_el_size = MPIR_Datatype_get_basic_size(el_type);
    dest_el_size = MPII_Datatype_get_basic_size_external32(el_type);
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
               src_el_size,
               dest_el_size,
               (int) *blocks_p);
#endif

    /* TODO: DEAL WITH CASE WHERE ALL DATA DOESN'T FIT! */
    if ((src_el_size == dest_el_size) && (src_el_size == 1))
    {
        MPIR_Memcpy(((char *)bufp) + rel_off,
               paramp->u.unpack.unpack_buffer, *blocks_p);
    }
    else if (is_float_type(el_type))
    {
        external32_float_convert(((char *) bufp) + rel_off,
                                 paramp->u.unpack.unpack_buffer,
                                 dest_el_size, src_el_size, *blocks_p);
    }
    else
    {
        external32_basic_convert(((char *) bufp) + rel_off,
                                 paramp->u.unpack.unpack_buffer,
                                 dest_el_size, src_el_size, *blocks_p);
    }
    paramp->u.unpack.unpack_buffer += (dest_el_size * (*blocks_p));

    MPIR_FUNC_VERBOSE_EXIT(MPIR_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Segment_pack_external32
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Segment_pack_external32(struct DLOOP_Segment *segp,
                                  DLOOP_Offset first,
                                  DLOOP_Offset *lastp,
                                  void *pack_buffer)
{
    struct MPIR_Segment_piece_params pack_params;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPIR_STATE_MPID_SEGMENT_PACK_EXTERNAL);

    MPIR_FUNC_VERBOSE_ENTER(MPIR_STATE_MPID_SEGMENT_PACK_EXTERNAL);

    pack_params.u.pack.pack_buffer = (DLOOP_Buffer)pack_buffer;
    MPIR_Segment_manipulate(segp,
                            first,
                            lastp,
                            MPII_Segment_contig_pack_external32_to_buf,
                            NULL, /* MPIR_Segment_vector_pack_external32_to_buf, */
                            NULL, /* blkidx */
                            NULL, /* MPIR_Segment_index_pack_external32_to_buf, */
                            MPII_Datatype_get_basic_size_external32,
                            &pack_params);

    MPIR_FUNC_VERBOSE_EXIT(MPIR_STATE_MPID_SEGMENT_PACK_EXTERNAL);
    return;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Segment_unpack_external32
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Segment_unpack_external32(struct DLOOP_Segment *segp,
                                    DLOOP_Offset first,
                                    DLOOP_Offset *lastp,
                                    DLOOP_Buffer unpack_buffer)
{
    struct MPIR_Segment_piece_params pack_params;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPIR_STATE_MPID_SEGMENT_UNPACK_EXTERNAL32);

    MPIR_FUNC_VERBOSE_ENTER(MPIR_STATE_MPID_SEGMENT_UNPACK_EXTERNAL32);

    pack_params.u.unpack.unpack_buffer = unpack_buffer;
    MPIR_Segment_manipulate(segp,
                            first,
                            lastp,
                            MPII_Segment_contig_unpack_external32_to_buf,
                            NULL, /* MPIR_Segment_vector_unpack_external32_to_buf, */
                            NULL, /* blkidx */
                            NULL, /* MPIR_Segment_index_unpack_external32_to_buf, */
                            MPII_Datatype_get_basic_size_external32,
                            &pack_params);

    MPIR_FUNC_VERBOSE_EXIT(MPIR_STATE_MPID_SEGMENT_UNPACK_EXTERNAL32);
    return;
}

void MPIR_Type_access_contents(MPI_Datatype type,
                               int **ints_p,
                               MPI_Aint **aints_p,
                               MPI_Datatype **types_p)
{
    int nr_ints, nr_aints, nr_types, combiner;
    int types_sz, struct_sz, ints_sz, epsilon, align_sz = 8;
    MPIR_Datatype *dtp;
    MPIR_Datatype_contents *cp;

    MPIR_Type_get_envelope(type, &nr_ints, &nr_aints, &nr_types, &combiner);

    /* hardcoded handling of MPICH contents format... */
    MPIR_Datatype_get_ptr(type, dtp);
    DLOOP_Assert(dtp != NULL);

    cp = dtp->contents;
    DLOOP_Assert(cp != NULL);

#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    if (align_sz > HAVE_MAX_STRUCT_ALIGNMENT) {
        align_sz = HAVE_MAX_STRUCT_ALIGNMENT;
    }
#endif

    struct_sz = sizeof(MPIR_Datatype_contents);
    types_sz  = nr_types * sizeof(MPI_Datatype);
    ints_sz   = nr_ints * sizeof(int);

    if ((epsilon = struct_sz % align_sz)) {
        struct_sz += align_sz - epsilon;
    }
    if ((epsilon = types_sz % align_sz)) {
        types_sz += align_sz - epsilon;
    }
    if ((epsilon = ints_sz % align_sz)) {
        ints_sz += align_sz - epsilon;
    }
    *types_p = (MPI_Datatype *) (((char *) cp) + struct_sz);
    *ints_p  = (int *) (((char *) (*types_p)) + types_sz);
    *aints_p = (MPI_Aint *) (((char *) (*ints_p)) + ints_sz);
    /* end of hardcoded handling of MPICH contents format */

    return;
}

/* FIXME: Is this routine complete?  Why is it needed? If it is needed, it
   must have a comment that describes why it is needed and the arguments
   must have ATTRIBUTE((unused)) */
void MPIR_Type_release_contents(MPI_Datatype type,
                                int **ints_p,
                                MPI_Aint **aints_p,
                                MPI_Datatype **types_p)
{
    return;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Segment_pack_vector
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIR_Segment_pack_vector
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
void MPIR_Segment_pack_vector(struct DLOOP_Segment *segp,
                              DLOOP_Offset first,
                              DLOOP_Offset *lastp,
                              DLOOP_VECTOR *vectorp,
                              int *lengthp)
{
    struct MPIR_Segment_piece_params packvec_params;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SEGMENT_PACK_VECTOR);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SEGMENT_PACK_VECTOR);

    packvec_params.u.pack_vector.vectorp = vectorp;
    packvec_params.u.pack_vector.index   = 0;
    packvec_params.u.pack_vector.length  = *lengthp;

    MPIR_Assert(*lengthp > 0);

    MPIR_Segment_manipulate(segp,
                            first,
                            lastp,
                            MPII_Segment_contig_pack_to_iov,
                            MPII_Segment_vector_pack_to_iov,
                            NULL, /* blkidx fn */
                            NULL, /* index fn */
                            NULL,
                            &packvec_params);

    /* last value already handled by MPIR_Segment_manipulate */
    *lengthp = packvec_params.u.pack_vector.index;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_PACK_VECTOR);
    return;
}


/* MPIR_Segment_unpack_vector
*
* Q: Should this be any different from pack vector?
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Segment_unpack_vector
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Segment_unpack_vector(struct DLOOP_Segment *segp,
                                DLOOP_Offset first,
                                DLOOP_Offset *lastp,
                                DLOOP_VECTOR *vectorp,
                                int *lengthp)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SEGMENT_UNPACK_VECTOR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SEGMENT_UNPACK_VECTOR);

    MPIR_Segment_pack_vector(segp, first, lastp, vectorp, lengthp);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_UNPACK_VECTOR);
    return;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Segment_flatten
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIR_Segment_flatten
*
* offp    - pointer to array to fill in with offsets
* sizep   - pointer to array to fill in with sizes
* lengthp - pointer to value holding size of arrays; # used is returned
*
* Internally, index is used to store the index of next array value to fill in.
*
* TODO: MAKE SIZES Aints IN ROMIO, CHANGE THIS TO USE INTS TOO.
*/
void MPIR_Segment_flatten(struct DLOOP_Segment *segp,
                          DLOOP_Offset first,
                          DLOOP_Offset *lastp,
                          DLOOP_Offset *offp,
                          DLOOP_Size *sizep,
                          DLOOP_Offset *lengthp)
{
    struct MPIR_Segment_piece_params packvec_params;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SEGMENT_FLATTEN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SEGMENT_FLATTEN);

    packvec_params.u.flatten.offp = (int64_t *) offp;
    packvec_params.u.flatten.sizep = sizep;
    packvec_params.u.flatten.index   = 0;
    packvec_params.u.flatten.length  = *lengthp;

    MPIR_Assert(*lengthp > 0);

    MPIR_Segment_manipulate(segp,
                            first,
                            lastp,
                            MPII_Segment_contig_flatten,
                            MPII_Segment_vector_flatten,
                            NULL, /* blkidx fn */
                            NULL,
                            NULL,
                            &packvec_params);

    /* last value already handled by MPIR_Segment_manipulate */
    *lengthp = packvec_params.u.flatten.index;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_FLATTEN);
    return;
}

/*
* EVERYTHING BELOW HERE IS USED ONLY WITHIN THIS FILE
*/

/********** FUNCTIONS FOR CREATING AN IOV DESCRIBING BUFFER **********/

#undef FUNCNAME
#define FUNCNAME MPII_Segment_contig_pack_to_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPII_Segment_contig_pack_to_iov
*/
static int MPII_Segment_contig_pack_to_iov(DLOOP_Offset *blocks_p,
                                           DLOOP_Type el_type,
                                           DLOOP_Offset rel_off,
                                           void *bufp,
                                           void *v_paramp)
{
    int el_size, last_idx;
    DLOOP_Offset size;
    char *last_end = NULL;
    struct MPIR_Segment_piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SEGMENT_CONTIG_PACK_TO_IOV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SEGMENT_CONTIG_PACK_TO_IOV);

    el_size = MPIR_Datatype_get_basic_size(el_type);
    size = *blocks_p * (DLOOP_Offset) el_size;

    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE,VERBOSE,(MPL_DBG_FDEST,
                                               "    contig to vec: do=" MPI_AINT_FMT_DEC_SPEC ", dp=%p, ind=%d, sz=%d, blksz=" MPI_AINT_FMT_DEC_SPEC,
                                               (MPI_Aint) rel_off,
                                               bufp,
                                               paramp->u.pack_vector.index,
                                               el_size,
                                               (MPI_Aint) *blocks_p));

    last_idx = paramp->u.pack_vector.index - 1;
    if (last_idx >= 0) {
        last_end = ((char *) paramp->u.pack_vector.vectorp[last_idx].DLOOP_VECTOR_BUF) +
            paramp->u.pack_vector.vectorp[last_idx].DLOOP_VECTOR_LEN;
    }

    MPIR_Ensure_Aint_fits_in_pointer((MPIR_VOID_PTR_CAST_TO_MPI_AINT (bufp)) + rel_off);
    if ((last_idx == paramp->u.pack_vector.length-1) &&
            (last_end != ((char *) bufp + rel_off)))
    {
        /* we have used up all our entries, and this region doesn't fit on
         * the end of the last one.  setting blocks to 0 tells manipulation
         * function that we are done (and that we didn't process any blocks).
         */
        *blocks_p = 0;
        MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_CONTIG_PACK_TO_IOV);
        return 1;
    }
    else if (last_idx >= 0 && (last_end == ((char *) bufp + rel_off)))
    {
        /* add this size to the last vector rather than using up another one */
        paramp->u.pack_vector.vectorp[last_idx].DLOOP_VECTOR_LEN += size;
    }
    else {
        paramp->u.pack_vector.vectorp[last_idx+1].DLOOP_VECTOR_BUF = (char *) bufp + rel_off;
        paramp->u.pack_vector.vectorp[last_idx+1].DLOOP_VECTOR_LEN = size;
        paramp->u.pack_vector.index++;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_CONTIG_PACK_TO_IOV);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPII_Segment_vector_pack_to_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPII_Segment_vector_pack_to_iov
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
static int MPII_Segment_vector_pack_to_iov(DLOOP_Offset *blocks_p,
                                           DLOOP_Count count,
                                           DLOOP_Size blksz,
                                           DLOOP_Offset stride,
                                           DLOOP_Type el_type,
                                           DLOOP_Offset rel_off, /* offset into buffer */
                                           void *bufp, /* start of buffer */
                                           void *v_paramp)
{
    int i;
    DLOOP_Offset size, blocks_left, basic_size;
    struct MPIR_Segment_piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SEGMENT_VECTOR_PACK_TO_IOV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SEGMENT_VECTOR_PACK_TO_IOV);

    basic_size = (DLOOP_Offset) MPIR_Datatype_get_basic_size(el_type);
    blocks_left = *blocks_p;

    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE,VERBOSE,(MPL_DBG_FDEST,
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
                                               blksz,
                                               (MPI_Aint) stride,
                                               (MPI_Aint) *blocks_p));

    for (i=0; i < count && blocks_left > 0; i++) {
        int last_idx;
        char *last_end = NULL;

        if (blocks_left > (DLOOP_Offset) blksz) {
            size = ((DLOOP_Offset) blksz) * basic_size;
            blocks_left -= (DLOOP_Offset) blksz;
        }
        else {
            /* last pass */
            size = blocks_left * basic_size;
            blocks_left = 0;
        }

        last_idx = paramp->u.pack_vector.index - 1;
        if (last_idx >= 0) {
            last_end = ((char *) paramp->u.pack_vector.vectorp[last_idx].DLOOP_VECTOR_BUF) +
                paramp->u.pack_vector.vectorp[last_idx].DLOOP_VECTOR_LEN;
        }

        MPIR_Ensure_Aint_fits_in_pointer((MPIR_VOID_PTR_CAST_TO_MPI_AINT (bufp)) + rel_off);
        if ((last_idx == paramp->u.pack_vector.length-1) &&
                (last_end != ((char *) bufp + rel_off)))
        {
            /* we have used up all our entries, and this one doesn't fit on
             * the end of the last one.
             */
            *blocks_p -= (blocks_left + (size / basic_size));
#ifdef MPID_SP_VERBOSE
            MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE,VERBOSE,(MPL_DBG_FDEST,"\t[vector to vec exiting (1): next ind = %d, " MPI_AINT_FMT_DEC_SPEC " blocks processed.\n",
                        paramp->u.pack_vector.index,
                        (MPI_Aint) *blocks_p));
#endif
            MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_VECTOR_PACK_TO_IOV);
            return 1;
        }
        else if (last_idx >= 0 && (last_end == ((char *) bufp + rel_off)))
        {
            /* add this size to the last vector rather than using up new one */
            paramp->u.pack_vector.vectorp[last_idx].DLOOP_VECTOR_LEN += size;
        }
        else {
            paramp->u.pack_vector.vectorp[last_idx+1].DLOOP_VECTOR_BUF =
                (char *) bufp + rel_off;
            paramp->u.pack_vector.vectorp[last_idx+1].DLOOP_VECTOR_LEN = size;
            paramp->u.pack_vector.index++;
        }

        rel_off += stride;

    }

#ifdef MPID_SP_VERBOSE
    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE,VERBOSE,(MPL_DBG_FDEST,"\t[vector to vec exiting (2): next ind = %d, " MPI_AINT_FMT_DEC_SPEC " blocks processed.\n",
                paramp->u.pack_vector.index,
                (MPI_Aint) *blocks_p));
#endif

    /* if we get here then we processed ALL the blocks; don't need to update
     * blocks_p
     */
    MPIR_Assert(blocks_left == 0);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_VECTOR_PACK_TO_IOV);
    return 0;
}

/********** FUNCTIONS FOR FLATTENING A TYPE **********/

#undef FUNCNAME
#define FUNCNAME MPII_Segment_contig_flatten
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPII_Segment_contig_flatten
 */
static int MPII_Segment_contig_flatten(DLOOP_Offset *blocks_p,
                                       DLOOP_Type el_type,
                                       DLOOP_Offset rel_off,
                                       void *bufp,
                                       void *v_paramp)
{
    int idx, el_size;
    DLOOP_Offset size;
    struct MPIR_Segment_piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SEGMENT_CONTIG_FLATTEN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SEGMENT_CONTIG_FLATTEN);

    el_size = MPIR_Datatype_get_basic_size(el_type);
    size = *blocks_p * (DLOOP_Offset) el_size;
    idx = paramp->u.flatten.index;

#ifdef MPID_SP_VERBOSE
    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE,VERBOSE,(MPL_DBG_FDEST,"\t[contig flatten: idx = %d, loc = (" MPI_AINT_FMT_HEX_SPEC " + " MPI_AINT_FMT_HEX_SPEC ") = " MPI_AINT_FMT_HEX_SPEC ", size = " MPI_AINT_FMT_DEC_SPEC "]\n",
                                                             idx,
                                                             MPIR_VOID_PTR_CAST_TO_MPI_AINT bufp,
                                                             (MPI_Aint) rel_off,
                                                             MPIR_VOID_PTR_CAST_TO_MPI_AINT bufp + rel_off,
                                                             (MPI_Aint) size));
#endif

    if (idx > 0 && ((DLOOP_Offset) MPIR_VOID_PTR_CAST_TO_MPI_AINT bufp + rel_off) ==
            ((paramp->u.flatten.offp[idx - 1]) +
             (DLOOP_Offset) paramp->u.flatten.sizep[idx - 1]))
    {
        /* add this size to the last vector rather than using up another one */
        paramp->u.flatten.sizep[idx - 1] += size;
    }
    else {
        paramp->u.flatten.offp[idx] =  ((int64_t) MPIR_VOID_PTR_CAST_TO_MPI_AINT bufp) + (int64_t) rel_off;
        paramp->u.flatten.sizep[idx] = size;

        paramp->u.flatten.index++;
        /* check to see if we have used our entire vector buffer, and if so
         * return 1 to stop processing
         */
        if (paramp->u.flatten.index == paramp->u.flatten.length)
        {
            MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_CONTIG_FLATTEN);
            return 1;
        }
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_CONTIG_FLATTEN);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPII_Segment_vector_flatten
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPII_Segment_vector_flatten
 *
 * Notes:
 * - this is only called when the starting position is at the beginning
 *   of a whole block in a vector type.
 * - this was a virtual copy of MPIR_Segment_pack_to_iov; now it has improvements
 *   that MPIR_Segment_pack_to_iov needs.
 * - we return the number of blocks that we did process in region pointed to by
 *   blocks_p.
 */
static int MPII_Segment_vector_flatten(DLOOP_Offset *blocks_p,
                                       DLOOP_Count count,
                                       DLOOP_Size blksz,
                                       DLOOP_Offset stride,
                                       DLOOP_Type el_type,
                                       DLOOP_Offset rel_off, /* offset into buffer */
                                       void *bufp, /* start of buffer */
                                       void *v_paramp)
{
    int i;
    DLOOP_Offset size, blocks_left, basic_size;
    struct MPIR_Segment_piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SEGMENT_VECTOR_FLATTEN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SEGMENT_VECTOR_FLATTEN);

    basic_size = (DLOOP_Offset) MPIR_Datatype_get_basic_size(el_type);
    blocks_left = *blocks_p;

    for (i=0; i < count && blocks_left > 0; i++) {
        int idx = paramp->u.flatten.index;

        if (blocks_left > (DLOOP_Offset) blksz) {
            size = ((DLOOP_Offset) blksz) * basic_size;
            blocks_left -= (DLOOP_Offset) blksz;
        }
        else {
            /* last pass */
            size = blocks_left * basic_size;
            blocks_left = 0;
        }

        if (idx > 0 && ((DLOOP_Offset) MPIR_VOID_PTR_CAST_TO_MPI_AINT bufp + rel_off) ==
                ((paramp->u.flatten.offp[idx - 1]) + (DLOOP_Offset) paramp->u.flatten.sizep[idx - 1]))
        {
            /* add this size to the last region rather than using up another one */
            paramp->u.flatten.sizep[idx - 1] += size;
        }
        else if (idx < paramp->u.flatten.length) {
            /* take up another region */
            paramp->u.flatten.offp[idx]  = (DLOOP_Offset) MPIR_VOID_PTR_CAST_TO_MPI_AINT bufp + rel_off;
            paramp->u.flatten.sizep[idx] = size;
            paramp->u.flatten.index++;
        }
        else {
            /* we tried to add to the end of the last region and failed; add blocks back in */
            *blocks_p = *blocks_p - blocks_left + (size / basic_size);
            MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_VECTOR_FLATTEN);
            return 1;
        }
        rel_off += stride;

    }
    /* --BEGIN ERROR HANDLING-- */
    MPIR_Assert(blocks_left == 0);
    /* --END ERROR HANDLING-- */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SEGMENT_VECTOR_FLATTEN);
    return 0;
}
