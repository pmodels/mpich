/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
*  (C) 2001 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

#include <stdio.h>
#include <stdlib.h>

#include <mpiimpl.h>
#include <mpid_dataloop.h>

/*
 * Define these two names to enable debugging output.
 */
#undef MPID_SP_VERBOSE
#undef MPID_SU_VERBOSE

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

/* prototypes of internal functions */
static int MPID_Segment_vector_pack_to_iov(DLOOP_Offset *blocks_p,
				       int count,
				       int blksz,
				       DLOOP_Offset stride,
				       DLOOP_Type el_type,
				       DLOOP_Offset rel_off,
				       void *bufp,
				       void *v_paramp);

static int MPID_Segment_contig_pack_to_iov(DLOOP_Offset *blocks_p,
                                           DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

static int MPID_Segment_contig_flatten(DLOOP_Offset *blocks_p,
				   DLOOP_Type el_type,
				   DLOOP_Offset rel_off,
				   void *bufp,
				   void *v_paramp);

static int MPID_Segment_vector_flatten(DLOOP_Offset *blocks_p,
				   int count,
				   int blksz,
				   DLOOP_Offset stride,
				   DLOOP_Type el_type,
				   DLOOP_Offset rel_off, /* offset into buffer */
				   void *bufp, /* start of buffer */
				   void *v_paramp);

/********** EXTERNALLY VISIBLE FUNCTIONS FOR TYPE MANIPULATION **********/

#undef FUNCNAME
#define FUNCNAME MPID_Segment_pack_vector
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPID_Segment_pack_vector
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
void MPID_Segment_pack_vector(struct DLOOP_Segment *segp,
			  DLOOP_Offset first,
			  DLOOP_Offset *lastp,
			  DLOOP_VECTOR *vectorp,
			  int *lengthp)
{
struct MPID_Segment_piece_params packvec_params;
MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_PACK_VECTOR);

MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_PACK_VECTOR);

packvec_params.u.pack_vector.vectorp = vectorp;
packvec_params.u.pack_vector.index   = 0;
packvec_params.u.pack_vector.length  = *lengthp;

MPIU_Assert(*lengthp > 0);

MPID_Segment_manipulate(segp,
			first,
			lastp,
			MPID_Segment_contig_pack_to_iov,
			MPID_Segment_vector_pack_to_iov,
			NULL, /* blkidx fn */
			NULL, /* index fn */
			NULL,
			&packvec_params);

/* last value already handled by MPID_Segment_manipulate */
*lengthp = packvec_params.u.pack_vector.index;
MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_PACK_VECTOR);
return;
}

/* MPID_Segment_unpack_vector
*
* Q: Should this be any different from pack vector?
*/
#undef FUNCNAME
#define FUNCNAME MPID_Segment_unpack_vector
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_Segment_unpack_vector(struct DLOOP_Segment *segp,
			    DLOOP_Offset first,
			    DLOOP_Offset *lastp,
			    DLOOP_VECTOR *vectorp,
			    int *lengthp)
{
MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_UNPACK_VECTOR);
MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_UNPACK_VECTOR);
MPID_Segment_pack_vector(segp, first, lastp, vectorp, lengthp);
MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_UNPACK_VECTOR);
return;
}

#undef FUNCNAME
#define FUNCNAME MPID_Segment_flatten
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPID_Segment_flatten
*
* offp    - pointer to array to fill in with offsets
* sizep   - pointer to array to fill in with sizes
* lengthp - pointer to value holding size of arrays; # used is returned
*
* Internally, index is used to store the index of next array value to fill in.
*
* TODO: MAKE SIZES Aints IN ROMIO, CHANGE THIS TO USE INTS TOO.
*/
void MPID_Segment_flatten(struct DLOOP_Segment *segp,
		      DLOOP_Offset first,
		      DLOOP_Offset *lastp,
		      DLOOP_Offset *offp,
		      int *sizep,
		      DLOOP_Offset *lengthp)
{
struct MPID_Segment_piece_params packvec_params;
MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_FLATTEN);

MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_FLATTEN);

packvec_params.u.flatten.offp = (int64_t *) offp;
packvec_params.u.flatten.sizep = sizep;
packvec_params.u.flatten.index   = 0;
packvec_params.u.flatten.length  = *lengthp;

MPIU_Assert(*lengthp > 0);

MPID_Segment_manipulate(segp,
			first,
			lastp,
			MPID_Segment_contig_flatten,
			MPID_Segment_vector_flatten,
			NULL, /* blkidx fn */
			NULL,
			NULL,
			&packvec_params);

/* last value already handled by MPID_Segment_manipulate */
*lengthp = packvec_params.u.flatten.index;
MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_FLATTEN);
return;
}


/*
* EVERYTHING BELOW HERE IS USED ONLY WITHIN THIS FILE
*/

/********** FUNCTIONS FOR CREATING AN IOV DESCRIBING BUFFER **********/

#undef FUNCNAME
#define FUNCNAME MPID_Segment_contig_pack_to_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPID_Segment_contig_pack_to_iov
*/
static int MPID_Segment_contig_pack_to_iov(DLOOP_Offset *blocks_p,
                                           DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp)
{
    int el_size, last_idx;
    DLOOP_Offset size;
    char *last_end = NULL;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);

    el_size = MPID_Datatype_get_basic_size(el_type);
    size = *blocks_p * (DLOOP_Offset) el_size;

    MPIU_DBG_MSG_FMT(DATATYPE,VERBOSE,(MPIU_DBG_FDEST,
             "\t[contig to vec: do=" MPI_AINT_FMT_DEC_SPEC ", dp=%p, ind=%d, sz=%d, blksz=" MPI_AINT_FMT_DEC_SPEC "]\n",
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

    MPID_Ensure_Aint_fits_in_pointer((MPI_VOID_PTR_CAST_TO_MPI_AINT (bufp)) + rel_off);
    if ((last_idx == paramp->u.pack_vector.length-1) &&
	(last_end != ((char *) bufp + rel_off)))
    {
	/* we have used up all our entries, and this region doesn't fit on
	 * the end of the last one.  setting blocks to 0 tells manipulation
	 * function that we are done (and that we didn't process any blocks).
	 */
	*blocks_p = 0;
	MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);
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
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_Segment_vector_pack_to_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPID_Segment_vector_pack_to_iov
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
static int MPID_Segment_vector_pack_to_iov(DLOOP_Offset *blocks_p,
					   int count,
					   int blksz,
					   DLOOP_Offset stride,
					   DLOOP_Type el_type,
					   DLOOP_Offset rel_off, /* offset into buffer */
					   void *bufp, /* start of buffer */
					   void *v_paramp)
{
    int i;
    DLOOP_Offset size, blocks_left, basic_size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);

    basic_size = (DLOOP_Offset) MPID_Datatype_get_basic_size(el_type);
    blocks_left = *blocks_p;

    MPIU_DBG_MSG_FMT(DATATYPE,VERBOSE,(MPIU_DBG_FDEST,
             "\t[vector to vec: do=" MPI_AINT_FMT_DEC_SPEC
             ", dp=%p"
             ", len=%d, ind=%d, ct=%d, blksz=%d"
             ", str=" MPI_AINT_FMT_DEC_SPEC
             ", blks=" MPI_AINT_FMT_DEC_SPEC
             "]\n",
		    (MPI_Aint) rel_off,
		    bufp,
		    paramp->u.pack_vector.length,
		    paramp->u.pack_vector.index,
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

	MPID_Ensure_Aint_fits_in_pointer((MPI_VOID_PTR_CAST_TO_MPI_AINT (bufp)) + rel_off);
	if ((last_idx == paramp->u.pack_vector.length-1) &&
	    (last_end != ((char *) bufp + rel_off)))
	{
	    /* we have used up all our entries, and this one doesn't fit on
	     * the end of the last one.
	     */
	    *blocks_p -= (blocks_left + (size / basic_size));
#ifdef MPID_SP_VERBOSE
	    MPIU_dbg_printf("\t[vector to vec exiting (1): next ind = %d, " MPI_AINT_FMT_DEC_SPEC " blocks processed.\n",
			    paramp->u.pack_vector.index,
			    (MPI_Aint) *blocks_p);
#endif
	    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);
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
    MPIU_dbg_printf("\t[vector to vec exiting (2): next ind = %d, " MPI_AINT_FMT_DEC_SPEC " blocks processed.\n",
		    paramp->u.pack_vector.index,
		    (MPI_Aint) *blocks_p);
#endif

    /* if we get here then we processed ALL the blocks; don't need to update
     * blocks_p
     */
    MPIU_Assert(blocks_left == 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);
    return 0;
}

/********** FUNCTIONS FOR FLATTENING A TYPE **********/

#undef FUNCNAME
#define FUNCNAME MPID_Segment_contig_flatten
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPID_Segment_contig_flatten
 */
static int MPID_Segment_contig_flatten(DLOOP_Offset *blocks_p,
				       DLOOP_Type el_type,
				       DLOOP_Offset rel_off,
				       void *bufp,
				       void *v_paramp)
{
    int index, el_size;
    DLOOP_Offset size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);

    el_size = MPID_Datatype_get_basic_size(el_type);
    size = *blocks_p * (DLOOP_Offset) el_size;
    index = paramp->u.flatten.index;

#ifdef MPID_SP_VERBOSE
    MPIU_dbg_printf("\t[contig flatten: index = %d, loc = (" MPI_AINT_FMT_HEX_SPEC " + " MPI_AINT_FMT_HEX_SPEC ") = " MPI_AINT_FMT_HEX_SPEC ", size = " MPI_AINT_FMT_DEC_SPEC "]\n",
		    index,
		    MPI_VOID_PTR_CAST_TO_MPI_AINT bufp,
		    (MPI_Aint) rel_off,
		    MPI_VOID_PTR_CAST_TO_MPI_AINT bufp + rel_off,
		    (MPI_Aint) size);
#endif

    if (index > 0 && ((DLOOP_Offset) MPI_VOID_PTR_CAST_TO_MPI_AINT bufp + rel_off) ==
	((paramp->u.flatten.offp[index - 1]) +
	 (DLOOP_Offset) paramp->u.flatten.sizep[index - 1]))
    {
	/* add this size to the last vector rather than using up another one */
	paramp->u.flatten.sizep[index - 1] += size;
    }
    else {
	paramp->u.flatten.offp[index] =  ((int64_t) MPI_VOID_PTR_CAST_TO_MPI_AINT bufp) + (int64_t) rel_off;
	paramp->u.flatten.sizep[index] = size;

	paramp->u.flatten.index++;
	/* check to see if we have used our entire vector buffer, and if so
	 * return 1 to stop processing
	 */
	if (paramp->u.flatten.index == paramp->u.flatten.length)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);
	    return 1;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_Segment_vector_flatten
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* MPID_Segment_vector_flatten
 *
 * Notes:
 * - this is only called when the starting position is at the beginning
 *   of a whole block in a vector type.
 * - this was a virtual copy of MPID_Segment_pack_to_iov; now it has improvements
 *   that MPID_Segment_pack_to_iov needs.
 * - we return the number of blocks that we did process in region pointed to by
 *   blocks_p.
 */
static int MPID_Segment_vector_flatten(DLOOP_Offset *blocks_p,
				       int count,
				       int blksz,
				       DLOOP_Offset stride,
				       DLOOP_Type el_type,
				       DLOOP_Offset rel_off, /* offset into buffer */
				       void *bufp, /* start of buffer */
				       void *v_paramp)
{
    int i;
    DLOOP_Offset size, blocks_left, basic_size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);

    basic_size = (DLOOP_Offset) MPID_Datatype_get_basic_size(el_type);
    blocks_left = *blocks_p;

    for (i=0; i < count && blocks_left > 0; i++) {
	int index = paramp->u.flatten.index;

	if (blocks_left > (DLOOP_Offset) blksz) {
	    size = ((DLOOP_Offset) blksz) * basic_size;
	    blocks_left -= (DLOOP_Offset) blksz;
	}
	else {
	    /* last pass */
	    size = blocks_left * basic_size;
	    blocks_left = 0;
	}

	if (index > 0 && ((DLOOP_Offset) MPI_VOID_PTR_CAST_TO_MPI_AINT bufp + rel_off) ==
	    ((paramp->u.flatten.offp[index - 1]) + (DLOOP_Offset) paramp->u.flatten.sizep[index - 1]))
	{
	    /* add this size to the last region rather than using up another one */
	    paramp->u.flatten.sizep[index - 1] += size;
	}
	else if (index < paramp->u.flatten.length) {
	    /* take up another region */
	    paramp->u.flatten.offp[index]  = (DLOOP_Offset) MPI_VOID_PTR_CAST_TO_MPI_AINT bufp + rel_off;
	    paramp->u.flatten.sizep[index] = size;
	    paramp->u.flatten.index++;
	}
	else {
	    /* we tried to add to the end of the last region and failed; add blocks back in */
	    *blocks_p = *blocks_p - blocks_left + (size / basic_size);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);
	    return 1;
	}
	rel_off += stride;

    }
    /* --BEGIN ERROR HANDLING-- */
    MPIU_Assert(blocks_left == 0);
    /* --END ERROR HANDLING-- */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);
    return 0;
}
