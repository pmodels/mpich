/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <limits.h>

#undef MPID_STRUCT_FLATTEN_DEBUG
#undef MPID_STRUCT_DEBUG

static MPI_Aint MPID_Type_struct_alignsize(int count,
				      const MPI_Datatype *oldtype_array,
				      const MPI_Aint *displacement_array);

/* MPID_Type_struct_alignsize
 *
 * This function guesses at how the C compiler would align a structure
 * with the given components.
 *
 * It uses these configure-time defines to do its magic:
 * - HAVE_MAX_INTEGER_ALIGNMENT - maximum byte alignment of integers
 * - HAVE_MAX_FP_ALIGNMENT      - maximum byte alignment of floating points
 * - HAVE_MAX_LONG_DOUBLE_FP_ALIGNMENT - maximum byte alignment with long
 *                                   doubles (if different from FP_ALIGNMENT)
 * - HAVE_MAX_DOUBLE_FP_ALIGNMENT - maximum byte alignment with doubles (if
 *                                  long double is different from FP_ALIGNMENT)
 * - HAVE_DOUBLE_POS_ALIGNMENT  - indicates that structures with doubles
 *                                are aligned differently if double isn't
 *                                at displacement 0 (e.g. PPC32/64).
 * - HAVE_LLINT_POS_ALIGNMENT   - same as above, for MPI_LONG_LONG_INT
 *
 * The different FP, DOUBLE, LONG_DOUBLE alignment case are necessary for
 * Cygwin on X86 (because long_double is 12 bytes, so double and long double
 * have different natural alignments).  Linux on X86, however, does not have
 * different rules for this case.
 */
static MPI_Aint MPID_Type_struct_alignsize(int count,
				      const MPI_Datatype *oldtype_array,
				      const MPI_Aint *displacement_array)
{
    int i;
    MPI_Aint max_alignsize = 0, tmp_alignsize, derived_alignsize = 0;

    for (i=0; i < count; i++)
    {
	/* shouldn't be called with an LB or UB, but we'll handle it nicely */
	if (oldtype_array[i] == MPI_LB || oldtype_array[i] == MPI_UB) continue;
	else if (HANDLE_GET_KIND(oldtype_array[i]) == HANDLE_KIND_BUILTIN)
	{
	    tmp_alignsize = MPID_Datatype_get_basic_size(oldtype_array[i]);

#ifdef HAVE_DOUBLE_ALIGNMENT_EXCEPTION
	    if (oldtype_array[i] == MPI_DOUBLE) {
		tmp_alignsize = HAVE_DOUBLE_ALIGNMENT_EXCEPTION;
	    }
#endif

	    switch(oldtype_array[i])
	    {
		case MPI_FLOAT:
		case MPI_DOUBLE:
		case MPI_LONG_DOUBLE:
#if defined(HAVE_MAX_LONG_DOUBLE_FP_ALIGNMENT) && \
    defined(HAVE_MAX_DOUBLE_FP_ALIGNMENT)
		    if (oldtype_array[i] == MPI_LONG_DOUBLE) {
			if (tmp_alignsize > HAVE_MAX_LONG_DOUBLE_FP_ALIGNMENT)
			    tmp_alignsize = HAVE_MAX_LONG_DOUBLE_FP_ALIGNMENT;
		    }
		    else if (oldtype_array[i] == MPI_DOUBLE) {
			if (tmp_alignsize > HAVE_MAX_DOUBLE_FP_ALIGNMENT)
			    tmp_alignsize = HAVE_MAX_DOUBLE_FP_ALIGNMENT;
		    }
		    else {
			/* HAVE_MAX_FP_ALIGNMENT may not be defined, hence commented */
				/*
			if (tmp_alignsize > HAVE_MAX_FP_ALIGNMENT)
			    tmp_alignsize = HAVE_MAX_FP_ALIGNMENT;
				*/
		    }
#elif defined(HAVE_MAX_FP_ALIGNMENT)
		    if (tmp_alignsize > HAVE_MAX_FP_ALIGNMENT)
			tmp_alignsize = HAVE_MAX_FP_ALIGNMENT;
#endif
#ifdef HAVE_DOUBLE_POS_ALIGNMENT
		    /* sort of a hack, but so is this rule */
		    if (oldtype_array[i] == MPI_DOUBLE &&
			displacement_array[i] != (MPI_Aint) 0)
		    {
			tmp_alignsize = 4;
		    }
#endif
		    break;
		default:
#ifdef HAVE_MAX_INTEGER_ALIGNMENT
		    if (tmp_alignsize > HAVE_MAX_INTEGER_ALIGNMENT)
			tmp_alignsize = HAVE_MAX_INTEGER_ALIGNMENT;
#endif
		    break;
#ifdef HAVE_LLINT_POS_ALIGNMENT
		    if (oldtype_array[i] == MPI_LONG_LONG_INT &&
			displacement_array[i] != (MPI_Aint) 0)
		    {
			tmp_alignsize = 4;
		    }
#endif
	    }
	}
	else
	{
	    MPID_Datatype *dtp;

	    MPID_Datatype_get_ptr(oldtype_array[i], dtp);
	    tmp_alignsize = dtp->alignsize;
	    if (derived_alignsize < tmp_alignsize)
		derived_alignsize = tmp_alignsize;
	}
	if (max_alignsize < tmp_alignsize) max_alignsize = tmp_alignsize;

    }

    return max_alignsize;
}


/*@
  MPID_Type_struct - create a struct datatype

Input Parameters:
+ count - number of blocks in vector
. blocklength_array - number of elements in each block
. displacement_array - offsets of blocks from start of type in bytes
- oldtype_array - types (using handle) of datatypes on which vector is based

Output Parameters:
. newtype - handle of new struct datatype

  Return Value:
  MPI_SUCCESS on success, MPI errno on failure.
@*/
int MPID_Type_struct(int count,
		     const int *blocklength_array,
		     const MPI_Aint *displacement_array,
		     const MPI_Datatype *oldtype_array,
		     MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int i, old_are_contig = 1, definitely_not_contig = 0;
    int found_sticky_lb = 0, found_sticky_ub = 0, found_true_lb = 0,
	found_true_ub = 0, found_el_type = 0, found_lb=0, found_ub=0;
    MPI_Aint el_sz = 0;
    MPI_Aint size = 0;
    MPI_Datatype el_type = MPI_DATATYPE_NULL;
    MPI_Aint true_lb_disp = 0, true_ub_disp = 0, sticky_lb_disp = 0,
	sticky_ub_disp = 0, lb_disp = 0, ub_disp = 0;

    MPID_Datatype *new_dtp;

    if (count == 0) return MPID_Type_zerolen(newtype);

#ifdef MPID_STRUCT_DEBUG
    MPIDI_Datatype_printf(oldtype_array[0], 1, displacement_array[0],
			  blocklength_array[0], 1);
    for (i=1; i < count; i++)
    {
	MPIDI_Datatype_printf(oldtype_array[i], 1, displacement_array[i],
			      blocklength_array[i], 0);
    }
#endif

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_struct",
					 __LINE__, MPI_ERR_OTHER,
					 "**nomem", 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = NULL;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = NULL;

    new_dtp->dataloop       = NULL;
    new_dtp->dataloop_size  = -1;
    new_dtp->dataloop_depth = -1;
    new_dtp->hetero_dloop       = NULL;
    new_dtp->hetero_dloop_size  = -1;
    new_dtp->hetero_dloop_depth = -1;

    /* check for junk struct with all zero blocks */
    for (i=0; i < count; i++) if (blocklength_array[i] != 0) break;

    if (i == count)
    {
	MPIU_Handle_obj_free(&MPID_Datatype_mem, new_dtp);
	return MPID_Type_zerolen(newtype);
    }

    new_dtp->max_contig_blocks = 0;
    for (i=0; i < count; i++)
    {
	int is_builtin =
	    (HANDLE_GET_KIND(oldtype_array[i]) == HANDLE_KIND_BUILTIN);
	MPI_Aint tmp_lb, tmp_ub, tmp_true_lb, tmp_true_ub;
	MPI_Aint tmp_el_sz;
	MPI_Datatype tmp_el_type;
	MPID_Datatype *old_dtp = NULL;

	/* Interpreting typemap to not include 0 blklen things, including
	 * MPI_LB and MPI_UB. -- Rob Ross, 10/31/2005
	 */
	if (blocklength_array[i] == 0) continue;

	if (is_builtin)
	{
	    tmp_el_sz   = MPID_Datatype_get_basic_size(oldtype_array[i]);
	    tmp_el_type = oldtype_array[i];

	    MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint)(blocklength_array[i]),
				      displacement_array[i],
				      0,
				      tmp_el_sz,
				      tmp_el_sz,
				      tmp_lb,
				      tmp_ub);
	    tmp_true_lb = tmp_lb;
	    tmp_true_ub = tmp_ub;

	    size += tmp_el_sz * blocklength_array[i];

	    new_dtp->max_contig_blocks++;
	}
	else
	{
	    MPID_Datatype_get_ptr(oldtype_array[i], old_dtp);

	    /* Ensure that "builtin_element_size" fits into an int datatype. */
	    MPIU_Ensure_Aint_fits_in_int(old_dtp->builtin_element_size);

	    tmp_el_sz   = old_dtp->builtin_element_size;
	    tmp_el_type = old_dtp->basic_type;

	    MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[i],
				      displacement_array[i],
				      old_dtp->lb,
				      old_dtp->ub,
				      old_dtp->extent,
				      tmp_lb,
				      tmp_ub);
	    tmp_true_lb = tmp_lb + (old_dtp->true_lb - old_dtp->lb);
	    tmp_true_ub = tmp_ub + (old_dtp->true_ub - old_dtp->ub);

	    size += old_dtp->size * blocklength_array[i];

	    new_dtp->max_contig_blocks += old_dtp->max_contig_blocks;
	}

	/* element size and type */
	if (oldtype_array[i] != MPI_LB && oldtype_array[i] != MPI_UB)
	{
	    if (found_el_type == 0)
	    {
		el_sz         = tmp_el_sz;
		el_type       = tmp_el_type;
		found_el_type = 1;
	    }
	    else if (el_sz != tmp_el_sz)
	    {
		el_sz = -1;
		el_type = MPI_DATATYPE_NULL;
	    }
	    else if (el_type != tmp_el_type)
	    {
		/* Q: should we set el_sz = -1 even though the same? */
		el_type = MPI_DATATYPE_NULL;
	    }
	}

	/* keep lowest sticky lb */
	if ((oldtype_array[i] == MPI_LB) ||
	    (!is_builtin && old_dtp->has_sticky_lb))
	{
	    if (!found_sticky_lb)
	    {
		found_sticky_lb = 1;
		sticky_lb_disp  = tmp_lb;
	    }
	    else if (sticky_lb_disp > tmp_lb)
	    {
		sticky_lb_disp = tmp_lb;
	    }
	}

	/* keep highest sticky ub */
	if ((oldtype_array[i] == MPI_UB) ||
	    (!is_builtin && old_dtp->has_sticky_ub))
	{
	    if (!found_sticky_ub)
	    {
		found_sticky_ub = 1;
		sticky_ub_disp  = tmp_ub;
	    }
	    else if (sticky_ub_disp < tmp_ub)
	    {
		sticky_ub_disp = tmp_ub;
	    }
	}

	/* keep lowest lb/true_lb and highest ub/true_ub
	 *
	 * note: checking for contiguity at the same time, to avoid
	 *       yet another pass over the arrays
	 */
	if (oldtype_array[i] != MPI_UB && oldtype_array[i] != MPI_LB)
	{
	    if (!found_true_lb)
	    {
		found_true_lb = 1;
		true_lb_disp  = tmp_true_lb;
	    }
	    else if (true_lb_disp > tmp_true_lb)
	    {
		/* element starts before previous */
		true_lb_disp = tmp_true_lb;
		definitely_not_contig = 1;
	    }

	    if (!found_lb)
	    {
		found_lb = 1;
		lb_disp  = tmp_lb;
	    }
	    else if (lb_disp > tmp_lb)
	    {
		/* lb before previous */
		lb_disp = tmp_lb;
		definitely_not_contig = 1;
	    }

	    if (!found_true_ub)
	    {
		found_true_ub = 1;
		true_ub_disp  = tmp_true_ub;
	    }
	    else if (true_ub_disp < tmp_true_ub)
	    {
		true_ub_disp = tmp_true_ub;
	    }
	    else {
		/* element ends before previous ended */
		definitely_not_contig = 1;
	    }

	    if (!found_ub)
	    {
		found_ub = 1;
		ub_disp  = tmp_ub;
	    }
	    else if (ub_disp < tmp_ub)
	    {
		ub_disp = tmp_ub;
	    }
	    else {
		/* ub before previous */
		definitely_not_contig = 1;
	    }
	}

	if (!is_builtin && !old_dtp->is_contig)
	{
	    old_are_contig = 0;
	}
    }

    new_dtp->n_builtin_elements = -1; /* TODO */
    new_dtp->builtin_element_size = el_sz;
    new_dtp->basic_type = el_type;

    new_dtp->has_sticky_lb = found_sticky_lb;
    new_dtp->true_lb       = true_lb_disp;
    new_dtp->lb = (found_sticky_lb) ? sticky_lb_disp : lb_disp;

    new_dtp->has_sticky_ub = found_sticky_ub;
    new_dtp->true_ub       = true_ub_disp;
    new_dtp->ub = (found_sticky_ub) ? sticky_ub_disp : ub_disp;

    new_dtp->alignsize = MPID_Type_struct_alignsize(count,
						    oldtype_array,
						    displacement_array);

    new_dtp->extent = new_dtp->ub - new_dtp->lb;
    if ((!found_sticky_lb) && (!found_sticky_ub))
    {
	/* account for padding */
	MPI_Aint epsilon = (new_dtp->alignsize > 0) ?
	    new_dtp->extent % ((MPI_Aint)(new_dtp->alignsize)) : 0;

	if (epsilon)
	{
	    new_dtp->ub    += ((MPI_Aint)(new_dtp->alignsize) - epsilon);
	    new_dtp->extent = new_dtp->ub - new_dtp->lb;
	}
    }

    new_dtp->size = size;

    /* new type is contig for N types if its size and extent are the
     * same, and the old type was also contiguous, and we didn't see
     * something noncontiguous based on true ub/ub.
     */
    if (((MPI_Aint)(new_dtp->size) == new_dtp->extent) &&
	old_are_contig && (! definitely_not_contig))
    {
	new_dtp->is_contig = 1;
    }
    else
    {
	new_dtp->is_contig = 0;
    }

    *newtype = new_dtp->handle;
    return mpi_errno;
}


