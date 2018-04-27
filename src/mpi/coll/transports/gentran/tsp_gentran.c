/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"
#include "tsp_gentran_types.h"
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "utlist.h"

/* UT_icd helper structure for vtx_t utarray */
UT_icd vtx_t_icd = {
    sizeof(MPII_Genutil_vtx_t),
    NULL,
    MPII_Genutil_vtx_copy,
    MPII_Genutil_vtx_dtor
};

#undef FUNCNAME
#define FUNCNAME MPII_Genutil_sched_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Genutil_sched_create(MPII_Genutil_sched_t * sched, int tag)
{
    sched->total_vtcs = 0;
    sched->completed_vtcs = 0;
    sched->tag = tag;

    /* initialize array for storing vertices */
    utarray_new(sched->vtcs, &vtx_t_icd, MPL_MEM_COLL);

    sched->issued_head = NULL;
    sched->issued_tail = NULL;

    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPII_Genutil_sched_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Genutil_sched_isend(const void *buf,
                             int count,
                             MPI_Datatype dt,
                             int dest,
                             int tag,
                             MPIR_Comm * comm_ptr,
                             MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs)
{
    vtx_t *vtxp;
    int vtx_id;

    /* assign a new vertex */
    vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);
    sched->tag = tag;
    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__ISEND;
    MPII_Genutil_vtx_add_dependencies(sched, vtx_id, n_in_vtcs, in_vtcs);

    /* store the arguments */
    vtxp->u.isend.buf = buf;
    vtxp->u.isend.count = count;
    vtxp->u.isend.dt = dt;
    vtxp->u.isend.dest = dest;
    vtxp->u.isend.comm = comm_ptr;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] isend\n", vtx_id));

    return vtx_id;
}


#undef FUNCNAME
#define FUNCNAME MPII_Genutil_sched_imcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Genutil_sched_imcast(const void *buf,
                              int count,
                              MPI_Datatype dt,
                              UT_array * dests,
                              int num_dests,
                              int tag,
                              MPIR_Comm * comm_ptr,
                              MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs)
{
    vtx_t *vtxp;
    int vtx_id;
    int *dest, i;

    /* assign a new vertex */
    vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);
    sched->tag = tag;
    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__IMCAST;
    MPII_Genutil_vtx_add_dependencies(sched, vtx_id, n_in_vtcs, in_vtcs);

    /* store the arguments */
    vtxp->u.imcast.buf = (void *) buf;
    vtxp->u.imcast.count = count;
    vtxp->u.imcast.dt = dt;
    vtxp->u.imcast.num_dests = num_dests;
    utarray_new(vtxp->u.imcast.dests, &ut_int_icd, MPL_MEM_COLL);
    utarray_concat(vtxp->u.imcast.dests, dests, MPL_MEM_COLL);
    vtxp->u.imcast.comm = comm_ptr;
    vtxp->u.imcast.req =
        (struct MPIR_Request **) MPL_malloc(sizeof(struct MPIR_Request *) * num_dests,
                                            MPL_MEM_COLL);
    vtxp->u.imcast.last_complete = -1;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] imcast\n", vtx_id));
    return vtx_id;
}


#undef FUNCNAME
#define FUNCNAME MPII_Genutil_sched_irecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Genutil_sched_irecv(void *buf,
                             int count,
                             MPI_Datatype dt,
                             int source,
                             int tag,
                             MPIR_Comm * comm_ptr,
                             MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs)
{
    vtx_t *vtxp;
    int vtx_id;

    /* assign a new vertex */
    vtx_id = MPII_Genutil_vtx_create(sched, &vtxp);

    sched->tag = tag;
    vtxp->vtx_kind = MPII_GENUTIL_VTX_KIND__IRECV;
    MPII_Genutil_vtx_add_dependencies(sched, vtx_id, n_in_vtcs, in_vtcs);

    /* record the arguments */
    vtxp->u.irecv.buf = buf;
    vtxp->u.irecv.count = count;
    vtxp->u.irecv.dt = dt;
    vtxp->u.irecv.src = source;
    vtxp->u.irecv.comm = comm_ptr;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Gentran: schedule [%d] irecv\n", vtx_id));

    return vtx_id;
}


#undef FUNCNAME
#define FUNCNAME MPII_Genutil_sched_start
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Genutil_sched_start(MPII_Genutil_sched_t * sched, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    int is_complete;
    int made_progress;
    MPIR_Request *reqp;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPII_GENUTIL_SCHED_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPII_GENUTIL_SCHED_START);

    /* Create a request */
    reqp = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!reqp)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    *req = reqp;
    MPIR_Request_add_ref(reqp);

    /* Make some progress */
    mpi_errno = MPII_Genutil_sched_poke(sched, &is_complete, &made_progress);
    if (is_complete) {
        MPID_Request_complete(reqp);
        goto fn_exit;
    }

    /* Enqueue schedule and activate progress hook if not already activated */
    reqp->u.nbc.coll.sched = (void *) sched;
    if (coll_queue.head == NULL)
        MPID_Progress_activate_hook(MPII_Genutil_progress_hook_id);
    DL_APPEND(coll_queue.head, &(reqp->u.nbc.coll));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPII_GENUTIL_SCHED_START);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
