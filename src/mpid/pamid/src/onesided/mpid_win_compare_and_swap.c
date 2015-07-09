/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/onesided/mpid_win_compare_and_swap.c
 * \brief ???
 */
#include "mpidi_onesided.h"

extern pami_result_t
MPIDI_Atomic (pami_context_t   context,
	      void           * _req);

#ifndef __BGQ__
static pami_result_t
MPIDI_Compare_and_swap_using_pami_rmw(pami_context_t   context,
                                      void           * _req)
{
  MPIDI_Win_request *req = (MPIDI_Win_request*)_req;
  pami_result_t rc;
  int  target_rank;  

  MPID_assert(req != NULL);
  target_rank = req->target.rank;

  pami_rmw_t  params;
  params=zero_rmw_parms;
  params.dest=req->dest;
  params.cookie=(void *)req;
  params.done_fn=MPIDI_Win_DoneCB;
  params.type = req->pami_datatype;
  params.operation = PAMI_ATOMIC_FETCH_COMPARE_SET;
  params.local=req->user_buffer;  /*result*/
  params.remote=req->win->mpid.info[target_rank].base_addr + req->offset + (size_t)req->origin.dt.map[0].DLOOP_VECTOR_BUF;
  params.value=req->buffer;        /* replaced value with origin */
  params.test=req->compare_buffer;

  rc = PAMI_Rmw(context, &params);
  MPID_assert(rc == PAMI_SUCCESS);
  return rc;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_Compare_and_swap
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                          void *result_addr, MPI_Datatype datatype, int target_rank,
                          MPI_Aint target_disp, MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  MPIDI_Win_request *req;
  int shm_locked=0;

  if(win->mpid.sync.origin_epoch_type == win->mpid.sync.target_epoch_type &&
     win->mpid.sync.origin_epoch_type == MPID_EPOTYPE_REFENCE){
     win->mpid.sync.origin_epoch_type = MPID_EPOTYPE_FENCE;
     win->mpid.sync.target_epoch_type = MPID_EPOTYPE_FENCE;
  }

  if(win->mpid.sync.origin_epoch_type == MPID_EPOTYPE_NONE ||
     win->mpid.sync.origin_epoch_type == MPID_EPOTYPE_POST){
    MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
  }

  if (target_rank == MPI_PROC_NULL)
    {
      return MPI_SUCCESS;
    }
  /* Check if datatype is a C integer, Fortran Integer,
     logical, or byte, per the classes given on page 165. */
  MPIR_ERRTEST_TYPE_RMA_ATOMIC(datatype, mpi_errno);

  req = (MPIDI_Win_request *) MPIU_Calloc0(1, MPIDI_Win_request);
  req->win          = win;
  req->type         = MPIDI_WIN_REQUEST_COMPARE_AND_SWAP;

  req->offset = target_disp * win->mpid.info[target_rank].disp_unit;
#ifdef __BGQ__
  /* PAMI limitation as it doesnt permit VA of 0 to be passed into
   * memregion create, so we must pass base_va of heap computed from
   * an SPI call instead. So the target offset must be adjusted */
  if (req->win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC)
    req->offset -= (size_t)req->win->mpid.info[target_rank].base_addr;
#endif

  MPIDI_Win_datatype_basic(1, datatype, &req->origin.dt);

  if (req->origin.dt.size == 0)
    {
      MPIU_Free(req);
      return MPI_SUCCESS;
    }

  req->target.rank = target_rank;
    req->buffer           = (void *) ((uintptr_t) origin_addr + req->origin.dt.true_lb);
    req->user_buffer      = result_addr + req->origin.dt.true_lb;
    req->compare_buffer   = (void *) ((uintptr_t) compare_addr + req->origin.dt.true_lb);

    pami_result_t rc;
    pami_task_t task = MPID_VCR_GET_LPID(win->comm_ptr->vcr, target_rank);
    if (win->mpid.sync.origin_epoch_type == MPID_EPOTYPE_START &&
        !MPIDI_valid_group_rank(task, win->mpid.sync.sc.group))
    {
        MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                            return mpi_errno, "**rmasync");
    }

    rc = PAMI_Endpoint_create(MPIDI_Client, task, 0, &req->dest);
    MPID_assert(rc == PAMI_SUCCESS);

    MPIDI_Win_datatype_map(&req->origin.dt);
    win->mpid.sync.total += 1; 

    MPI_Datatype basic_type = MPI_DATATYPE_NULL;
    MPID_Datatype_get_basic_type(datatype, basic_type);
    MPID_assert(basic_type != MPI_DATATYPE_NULL);
    req->origin.datatype=basic_type;

    /* The pamid one-sided design requires context post in order to handle the
     * case where the number of pending rma operation exceeds the
     * 'PAMID_RMA_PENDING' threshold. When there are too many pending requests the
     * work function remains on the context post queue (by returning PAMI_EAGAIN)
     * so that the next time the context is advanced the work function will be
     * invoked again.
     *
     * TODO - When context post is not required it would be better to attempt a
     *        direct context operation and then fail over to using context post if
     *        the rma pending threshold has been reached. This would result in
     *        better latency for one-sided operations.
     */
    
#ifndef __BGQ__
  MPI_Op null_op=0;
  pami_data_function  pami_op;
  pami_type_t pami_type;
  if(MPIDI_Datatype_is_pami_rmw_supported(basic_type, &pami_type, null_op, &pami_op)  ) {
      req->pami_datatype = pami_type;
      PAMI_Context_post(MPIDI_Context[0], &req->post_request, MPIDI_Compare_and_swap_using_pami_rmw, req);
    } else 
#endif
      {
      PAMI_Context_post(MPIDI_Context[0], &req->post_request, MPIDI_Atomic, req);
    }

fn_fail:
  return mpi_errno;
}
