/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpidu_dataloop.h>
#include <stdlib.h>

/*@
  MPIR_Type_flatten
 
Input Parameters:
. type - MPI Datatype (must have been committed)

Output Parameters:
. nr_blocks_p - pointer to int in which to store the number of contiguous blocks in the type


  Return Value:
  0 on success, -1 on failure.
@*/

int MPIR_Type_flatten(MPI_Datatype type,
		      MPI_Aint *off_array,
		      MPI_Aint *size_array,
		      MPI_Aint *array_len_p)
{
    int err;
    MPI_Aint first, last;
    MPIDU_Datatype *datatype_ptr ATTRIBUTE((unused));
    MPIDU_Segment *segp;

    if (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) {
	off_array[0] = 0;
	MPIDU_Datatype_get_size_macro(type, size_array[0]);
	*array_len_p = 1;
	return 0;
    }

    MPIDU_Datatype_get_ptr(type, datatype_ptr);
    MPIR_Assert(datatype_ptr->is_committed);
    MPIR_Assert(*array_len_p >= datatype_ptr->max_contig_blocks);

    segp = MPIDU_Segment_alloc();
    err = MPIDU_Segment_init(0, 1, type, segp, 0); /* first 0 is bufptr,
                                                   * 1 is count
                                                   * last 0 is homogeneous
                                                   */
    if (err) return err;

    first = 0;
    last  = SEGMENT_IGNORE_LAST;

    MPIDU_Segment_flatten(segp,
			 first,
			 &last,
			 off_array,
			 size_array,
			 array_len_p);

    MPIDU_Segment_free(segp);

    return 0;
}
