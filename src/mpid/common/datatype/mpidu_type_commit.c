/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpidu_dataloop.h>
#include <stdlib.h>
#ifdef HAVE_EXT_COLL
#include "mpir_coll_impl.h"
#endif


/*@
  MPIDU_Type_commit
 
Input Parameters:
. datatype_p - pointer to MPI datatype

Output Parameters:

  Return Value:
  0 on success, -1 on failure.
@*/

int MPIDU_Type_commit(MPI_Datatype *datatype_p)
{
    int           mpi_errno=MPI_SUCCESS;
    MPIDU_Datatype *datatype_ptr;

    MPIR_Assert(HANDLE_GET_KIND(*datatype_p) != HANDLE_KIND_BUILTIN);

    MPIDU_Datatype_get_ptr(*datatype_p, datatype_ptr);

    if (datatype_ptr->is_committed == 0) {
       datatype_ptr->is_committed = 1;

#ifdef MPID_NEEDS_DLOOP_ALL_BYTES
        /* If MPID implementation needs use to reduce everything to
           a byte stream, do that. */
	MPIDU_Dataloop_create(*datatype_p,
			     &datatype_ptr->dataloop,
			     &datatype_ptr->dataloop_size,
			     &datatype_ptr->dataloop_depth,
			     MPIDU_DATALOOP_ALL_BYTES);
#else
	MPIDU_Dataloop_create(*datatype_p,
			     &datatype_ptr->dataloop,
			     &datatype_ptr->dataloop_size,
			     &datatype_ptr->dataloop_depth,
			     MPIDU_DATALOOP_HOMOGENEOUS);
#endif

	/* create heterogeneous dataloop */
	MPIDU_Dataloop_create(*datatype_p,
			     &datatype_ptr->hetero_dloop,
			     &datatype_ptr->hetero_dloop_size,
			     &datatype_ptr->hetero_dloop_depth,
			     MPIDU_DATALOOP_HETEROGENEOUS);

	MPL_DBG_MSG_D(MPIR_DBG_DATATYPE,TERSE,"# contig blocks = %d\n",
                       (int) datatype_ptr->max_contig_blocks);

#if 0
        MPIDI_Dataloop_dot_printf(datatype_ptr->dataloop, 0, 1);
#endif

#ifdef MPID_Type_commit_hook
    //   MPID_Type_commit_hook(datatype_ptr);
#endif /* MPID_Type_commit_hook */
#ifdef HAVE_EXT_COLL
    MPIC_dt_init(datatype_ptr);
#endif
    }
    return mpi_errno;
}

