/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
*  (C) 2001 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

#include <stdio.h>
#include <stdlib.h>

#include <mpiimpl.h>
#include <mpidu_dataloop.h>

/*
 * Define these two names to enable debugging output.
 */
#undef MPID_SP_VERBOSE
#undef MPID_SU_VERBOSE

/* MPIDU_Segment_piece_params
*
* This structure is used to pass function-specific parameters into our
* segment processing function.  This allows us to get additional parameters
* to the functions it calls without changing the prototype.
*/
struct MPIDU_Segment_piece_params {
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
	    MPI_Aint length;
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
static int MPIDU_Segment_vector_pack_to_iov(DLOOP_Offset *blocks_p,
				       DLOOP_Count count,
				       DLOOP_Size blksz,
				       DLOOP_Offset stride,
				       DLOOP_Type el_type,
				       DLOOP_Offset rel_off,
				       void *bufp,
				       void *v_paramp);

static int MPIDU_Segment_contig_pack_to_iov(DLOOP_Offset *blocks_p,
                                           DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

static int MPIDU_Segment_contig_flatten(DLOOP_Offset *blocks_p,
				   DLOOP_Type el_type,
				   DLOOP_Offset rel_off,
				   void *bufp,
				   void *v_paramp);

static int MPIDU_Segment_vector_flatten(DLOOP_Offset *blocks_p,
				   DLOOP_Count count,
				   DLOOP_Size blksz,
				   DLOOP_Offset stride,
				   DLOOP_Type el_type,
				   DLOOP_Offset rel_off, /* offset into buffer */
				   void *bufp, /* start of buffer */
				   void *v_paramp);

/********** EXTERNALLY VISIBLE FUNCTIONS FOR TYPE MANIPULATION **********/

#undef FUNCNAME
#define FUNCNAME MPIDU_Segment_pack_vector
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIDU_Segment_pack_vector
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
void MPIDU_Segment_pack_vector(struct DLOOP_Segment *segp,
			  DLOOP_Offset first,
			  DLOOP_Offset *lastp,
			  DLOOP_VECTOR *vectorp,
			  int *lengthp)
{
struct MPIDU_Segment_piece_params packvec_params;
MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEGMENT_PACK_VECTOR);

MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEGMENT_PACK_VECTOR);

packvec_params.u.pack_vector.vectorp = vectorp;
packvec_params.u.pack_vector.index   = 0;
packvec_params.u.pack_vector.length  = *lengthp;

MPIR_Assert(*lengthp > 0);

MPIDU_Segment_manipulate(segp,
			first,
			lastp,
			MPIDU_Segment_contig_pack_to_iov,
			MPIDU_Segment_vector_pack_to_iov,
			NULL, /* blkidx fn */
			NULL, /* index fn */
			NULL,
			&packvec_params);

/* last value already handled by MPIDU_Segment_manipulate */
*lengthp = packvec_params.u.pack_vector.index;
MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEGMENT_PACK_VECTOR);
return;
}

/* MPIDU_Segment_unpack_vector
*
* Q: Should this be any different from pack vector?
*/
#undef FUNCNAME
#define FUNCNAME MPIDU_Segment_unpack_vector
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDU_Segment_unpack_vector(struct DLOOP_Segment *segp,
			    DLOOP_Offset first,
			    DLOOP_Offset *lastp,
			    DLOOP_VECTOR *vectorp,
			    int *lengthp)
{
MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEGMENT_UNPACK_VECTOR);
MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEGMENT_UNPACK_VECTOR);
MPIDU_Segment_pack_vector(segp, first, lastp, vectorp, lengthp);
MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEGMENT_UNPACK_VECTOR);
return;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Segment_flatten
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIDU_Segment_flatten
*
* offp    - pointer to array to fill in with offsets
* sizep   - pointer to array to fill in with sizes
* lengthp - pointer to value holding size of arrays; # used is returned
*
* Internally, index is used to store the index of next array value to fill in.
*
* TODO: MAKE SIZES Aints IN ROMIO, CHANGE THIS TO USE INTS TOO.
*/
void MPIDU_Segment_flatten(struct DLOOP_Segment *segp,
		      DLOOP_Offset first,
		      DLOOP_Offset *lastp,
		      DLOOP_Offset *offp,
		      DLOOP_Size *sizep,
		      DLOOP_Offset *lengthp)
{
struct MPIDU_Segment_piece_params packvec_params;
MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEGMENT_FLATTEN);

MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEGMENT_FLATTEN);

packvec_params.u.flatten.offp = (int64_t *) offp;
packvec_params.u.flatten.sizep = sizep;
packvec_params.u.flatten.index   = 0;
packvec_params.u.flatten.length  = *lengthp;

MPIR_Assert(*lengthp > 0);

MPIDU_Segment_manipulate(segp,
			first,
			lastp,
			MPIDU_Segment_contig_flatten,
			MPIDU_Segment_vector_flatten,
			NULL, /* blkidx fn */
			NULL,
			NULL,
			&packvec_params);

/* last value already handled by MPIDU_Segment_manipulate */
*lengthp = packvec_params.u.flatten.index;
MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEGMENT_FLATTEN);
return;
}


/*
* EVERYTHING BELOW HERE IS USED ONLY WITHIN THIS FILE
*/

/********** FUNCTIONS FOR CREATING AN IOV DESCRIBING BUFFER **********/

#undef FUNCNAME
#define FUNCNAME MPIDU_Segment_contig_pack_to_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIDU_Segment_contig_pack_to_iov
*/
static int MPIDU_Segment_contig_pack_to_iov(DLOOP_Offset *blocks_p,
                                           DLOOP_Type el_type,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp)
{
    int el_size, last_idx;
    DLOOP_Offset size;
    char *last_end = NULL;
    struct MPIDU_Segment_piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);

    el_size = MPIDU_Datatype_get_basic_size(el_type);
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
	MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Segment_vector_pack_to_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIDU_Segment_vector_pack_to_iov
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
static int MPIDU_Segment_vector_pack_to_iov(DLOOP_Offset *blocks_p,
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
    struct MPIDU_Segment_piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);

    basic_size = (DLOOP_Offset) MPIDU_Datatype_get_basic_size(el_type);
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
	    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV);
    return 0;
}

/********** FUNCTIONS FOR FLATTENING A TYPE **********/

#undef FUNCNAME
#define FUNCNAME MPIDU_Segment_contig_flatten
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIDU_Segment_contig_flatten
 */
static int MPIDU_Segment_contig_flatten(DLOOP_Offset *blocks_p,
				       DLOOP_Type el_type,
				       DLOOP_Offset rel_off,
				       void *bufp,
				       void *v_paramp)
{
    int idx, el_size;
    DLOOP_Offset size;
    struct MPIDU_Segment_piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);

    el_size = MPIDU_Datatype_get_basic_size(el_type);
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
	    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);
	    return 1;
	}
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Segment_vector_flatten
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIDU_Segment_vector_flatten
 *
 * Notes:
 * - this is only called when the starting position is at the beginning
 *   of a whole block in a vector type.
 * - this was a virtual copy of MPIDU_Segment_pack_to_iov; now it has improvements
 *   that MPIDU_Segment_pack_to_iov needs.
 * - we return the number of blocks that we did process in region pointed to by
 *   blocks_p.
 */
static int MPIDU_Segment_vector_flatten(DLOOP_Offset *blocks_p,
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
    struct MPIDU_Segment_piece_params *paramp = v_paramp;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);

    basic_size = (DLOOP_Offset) MPIDU_Datatype_get_basic_size(el_type);
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
	    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);
	    return 1;
	}
	rel_off += stride;

    }
    /* --BEGIN ERROR HANDLING-- */
    MPIR_Assert(blocks_left == 0);
    /* --END ERROR HANDLING-- */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN);
    return 0;
}
