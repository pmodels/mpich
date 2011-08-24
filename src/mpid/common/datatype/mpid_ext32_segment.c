/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <mpichconf.h>
#include <mpiimpl.h>
#include <mpid_dataloop.h>

#undef MPID_SP_VERBOSE
#undef MPID_SU_VERBOSE

#include "mpid_ext32_segment.h"

/* MPID_Segment_piece_params
 *
 * This structure is used to pass function-specific parameters into our 
 * segment processing function.  This allows us to get additional parameters
 * to the functions it calls without changing the prototype.
 */
struct MPID_Segment_piece_params {
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
	    int *sizep; /* see notes in Segment_flatten header */
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

    MPIU_Assert(dest_buf && src_buf);

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
	MPIU_Error_printf( "Conversion of types whose size is not the same as the size in external32 is not supported\n" );
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

    MPIU_Assert(dest_buf && src_buf);

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
	MPIU_Error_printf( "Conversion of types whose size is not the same as the size in external32 is not supported\n" );
	MPID_Abort( 0, MPI_SUCCESS, 1, "Aborting with internal error" );
	/* There is no way to return an error code, so an abort is the 
	   only choice (the return value of this routine is not 
	   an error code) */
    }
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_Segment_contig_pack_external32_to_buf
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_Segment_contig_pack_external32_to_buf(DLOOP_Offset *blocks_p,
                                                      DLOOP_Type el_type,
                                                      DLOOP_Offset rel_off,
                                                      void *bufp,
                                                      void *v_paramp)
{
    int src_el_size, dest_el_size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_EXTERNAL32_TO_BUF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_EXTERNAL32_TO_BUF);

    src_el_size = MPID_Datatype_get_basic_size(el_type);
    dest_el_size = MPIDI_Datatype_get_basic_size_external32(el_type);
    MPIU_Assert(dest_el_size);

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
        MPIU_Memcpy(paramp->u.pack.pack_buffer,
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

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_EXTERNAL32_TO_BUF);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_Segment_contig_unpack_external32_to_buf
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_Segment_contig_unpack_external32_to_buf(DLOOP_Offset *blocks_p,
                                                        DLOOP_Type el_type,
                                                        DLOOP_Offset rel_off,
                                                        void *bufp,
                                                        void *v_paramp)
{
    int src_el_size, dest_el_size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF);

    src_el_size = MPID_Datatype_get_basic_size(el_type);
    dest_el_size = MPIDI_Datatype_get_basic_size_external32(el_type);
    MPIU_Assert(dest_el_size);

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
        MPIU_Memcpy(((char *)bufp) + rel_off,
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

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_Segment_pack_external32
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_Segment_pack_external32(struct DLOOP_Segment *segp,
				  DLOOP_Offset first,
				  DLOOP_Offset *lastp, 
				  void *pack_buffer)
{
    struct MPID_Segment_piece_params pack_params;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_PACK_EXTERNAL);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_PACK_EXTERNAL);

    pack_params.u.pack.pack_buffer = (DLOOP_Buffer)pack_buffer;
    MPID_Segment_manipulate(segp,
			    first,
			    lastp,
			    MPID_Segment_contig_pack_external32_to_buf,
                            NULL, /* MPID_Segment_vector_pack_external32_to_buf, */
			    NULL, /* blkidx */
                            NULL, /* MPID_Segment_index_pack_external32_to_buf, */
                            MPIDI_Datatype_get_basic_size_external32,
			    &pack_params);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_PACK_EXTERNAL);
    return;
}

#undef FUNCNAME
#define FUNCNAME MPID_Segment_unpack_external32
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_Segment_unpack_external32(struct DLOOP_Segment *segp,
				    DLOOP_Offset first,
				    DLOOP_Offset *lastp,
				    DLOOP_Buffer unpack_buffer)
{
    struct MPID_Segment_piece_params pack_params;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_UNPACK_EXTERNAL32);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_UNPACK_EXTERNAL32);

    pack_params.u.unpack.unpack_buffer = unpack_buffer;
    MPID_Segment_manipulate(segp,
			    first,
			    lastp,
			    MPID_Segment_contig_unpack_external32_to_buf,
                            NULL, /* MPID_Segment_vector_unpack_external32_to_buf, */
			    NULL, /* blkidx */
                            NULL, /* MPID_Segment_index_unpack_external32_to_buf, */
                            MPIDI_Datatype_get_basic_size_external32,
			    &pack_params);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_UNPACK_EXTERNAL32);
    return;
}
