/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>

/*@
  MPIR_Type_get_contig_blocks
 
  Input Parameters:
. type - MPI Datatype (must have been committed)

  Output Parameters:
. nr_blocks_p - pointer to int in which to store the number of contiguous blocks in the type


  Return Value:
  0 on success, -1 on failure.
@*/

int MPIR_Type_get_contig_blocks(MPI_Datatype type,
				int *nr_blocks_p)
{
    MPID_Datatype *datatype_ptr;

    if (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) {
	*nr_blocks_p = 1;
	return 0;
    }

    MPID_Datatype_get_ptr(type, datatype_ptr);
    MPIU_Assert(datatype_ptr->is_committed);

    *nr_blocks_p = datatype_ptr->max_contig_blocks;
    return 0;
}
