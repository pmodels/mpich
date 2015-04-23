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
 * \file src/onesided/mpid_win_put.c
 * \brief ???
 */
#include "mpidi_onesided.h"
#include "mpidi_util.h"


static inline int
MPIDI_Put_use_pami_rput(pami_context_t context, MPIDI_Win_request * req)
__attribute__((__always_inline__));
#ifdef RDMA_FAILOVER
static inline int
MPIDI_Put_use_pami_put(pami_context_t   context, MPIDI_Win_request * req)
__attribute__((__always_inline__));
#endif


static pami_result_t
MPIDI_Put(pami_context_t   context,
          void           * _req)
{
  MPIDI_Win_request *req = (MPIDI_Win_request*)_req;
  pami_result_t rc;

#ifdef USE_PAMI_RDMA
  rc = MPIDI_Put_use_pami_rput(context, req);
#else
  if( (req->origin.memregion_used) && (req->win->mpid.info[req->target.rank].memregion_used) )
    {
      rc = MPIDI_Put_use_pami_rput(context, req);
    } else {
      rc = MPIDI_Put_use_pami_put(context, req);
    }
#endif
  if( rc == PAMI_EAGAIN)
    return rc;


  return PAMI_SUCCESS;
}


static inline int
MPIDI_Put_use_pami_rput(pami_context_t context, MPIDI_Win_request * req)
{
  int use_typed_rdma = 0;
  if (!req->target.dt.contig || !req->origin.dt.contig) {
    use_typed_rdma = 0;
    if (MPIDI_Process.typed_onesided == 1)
      use_typed_rdma = 1;
  }

  if (use_typed_rdma) {
    pami_result_t rc;
    pami_rput_typed_t params;
    /* params need to zero out to avoid passing garbage to PAMI */
    params=zero_rput_typed_parms;

    params.rma.dest=req->dest;
    params.rma.hints.buffer_registered = PAMI_HINT_ENABLE;
    params.rma.hints.use_rdma          = PAMI_HINT_ENABLE;
    params.rma.bytes   = req->target.dt.size;
    params.rma.cookie  = req;
    params.rma.done_fn = NULL;
    params.rdma.local.mr=&req->origin.memregion;
    params.rdma.remote.mr=&req->win->mpid.info[req->target.rank].memregion;
    params.rdma.remote.offset= req->offset;
    params.rdma.local.offset  = req->state.local_offset;
    params.put.rdone_fn= MPIDI_Win_DoneCB;

    params.type.local = *(pami_type_t *)(req->origin.dt.pointer->device_datatype);
    params.type.remote = *(pami_type_t *)(req->target.dt.pointer->device_datatype);

    rc = PAMI_Rput_typed(context, &params);
    MPID_assert(rc == PAMI_SUCCESS);

  }
  else {
  pami_result_t rc;
  pami_rput_simple_t params;
  /* params need to zero out to avoid passing garbage to PAMI */
  params=zero_rput_parms;

  params.rma.dest=req->dest;
  params.rma.hints.buffer_registered = PAMI_HINT_ENABLE;
  params.rma.hints.use_rdma          = PAMI_HINT_ENABLE;
  params.rma.bytes   = 0;
  params.rma.cookie  = req;
  params.rma.done_fn = NULL;
  params.rdma.local.mr=&req->origin.memregion;
  params.rdma.remote.mr=&req->win->mpid.info[req->target.rank].memregion;
  params.rdma.remote.offset= req->offset;
  params.put.rdone_fn= MPIDI_Win_DoneCB;

  struct MPIDI_Win_sync* sync = &req->win->mpid.sync;
  TRACE_ERR("Start       index=%u/%d  l-addr=%p  r-base=%p  r-offset=%zu (sync->started=%u  sync->complete=%u)\n",
	    req->state.index, req->target.dt.num_contig, req->buffer, req->win->mpid.info[req->target.rank].base_addr, req->offset, sync->started, sync->complete);
  while (req->state.index < req->target.dt.num_contig) {
    if (sync->started > sync->complete + MPIDI_Process.rma_pending)
      {
	TRACE_ERR("Bailing out;  index=%u/%d  sync->started=%u  sync->complete=%u\n",
		  req->state.index, req->target.dt.num_contig, sync->started, sync->complete);
	return PAMI_EAGAIN;
      }
    ++sync->started;


    params.rma.bytes          =                       req->target.dt.map[req->state.index].DLOOP_VECTOR_LEN;
    params.rdma.remote.offset = req->offset + (size_t)req->target.dt.map[req->state.index].DLOOP_VECTOR_BUF;
    params.rdma.local.offset  = req->state.local_offset;
#ifdef TRACE_ON
    unsigned* buf = (unsigned*)(req->buffer + params.rdma.local.offset);
#endif
    TRACE_ERR("  Sub     index=%u  bytes=%zu  l-offset=%zu  r-offset=%zu  buf=%p  *(int*)buf=0x%08x\n",
	      req->state.index, params.rma.bytes, params.rdma.local.offset, params.rdma.remote.offset, buf, *buf);

    /** sync->total will be updated with every RMA and the complete
	will not change till that RMA has completed. In the meanwhile
	the rest of the RMAs will have memory leaks */
    if (req->target.dt.num_contig - req->state.index == 1) {
         rc = PAMI_Rput(context, &params);
         MPID_assert(rc == PAMI_SUCCESS);
         return PAMI_SUCCESS;
    } else {
          rc = PAMI_Rput(context, &params);
          MPID_assert(rc == PAMI_SUCCESS);
          req->state.local_offset += params.rma.bytes;
          ++req->state.index;
    }
  }
  }
  return PAMI_SUCCESS;
}

#ifdef RDMA_FAILOVER
static inline int
MPIDI_Put_use_pami_put(pami_context_t   context, MPIDI_Win_request * req)
{
  pami_result_t rc;
  pami_put_simple_t params;

  params = zero_put_parms;

  params.rma.dest=req->dest;
  params.rma.hints.use_rdma          = PAMI_HINT_DEFAULT;
#ifndef OUT_OF_ORDER_HANDLING
  params.rma.hints.no_long_header= 1,
#endif
  params.rma.bytes   = 0;
  params.rma.cookie  = req;
  params.rma.done_fn = NULL;
  params.addr.local=req->buffer;
  params.addr.remote=req->win->mpid.info[req->target.rank].base_addr;
  params.put.rdone_fn= MPIDI_Win_DoneCB;

  struct MPIDI_Win_sync* sync = &req->win->mpid.sync;
  TRACE_ERR("Start       index=%u/%d  l-addr=%p  r-base=%p  r-offset=%zu (sync->started=%u  sync->complete=%u)\n",
	    req->state.index, req->target.dt.num_contig, req->buffer, req->win->mpid.info[req->target.rank].base_addr, req->offset, sync->started, sync->complete);
  while (req->state.index < req->target.dt.num_contig) {
    if (sync->started > sync->complete + MPIDI_Process.rma_pending)
      {
	TRACE_ERR("Bailing out;  index=%u/%d  sync->started=%u  sync->complete=%u\n",
		  req->state.index, req->target.dt.num_contig, sync->started, sync->complete);
	return PAMI_EAGAIN;
      }
    ++sync->started;


    params.rma.bytes          =                       req->target.dt.map[req->state.index].DLOOP_VECTOR_LEN;
    params.addr.local          = req->buffer+req->state.local_offset;
    params.addr.remote         = req->win->mpid.info[req->target.rank].base_addr+ req->offset + (size_t)req->target.dt.map[req->state.index].DLOOP_VECTOR_BUF;
#ifdef TRACE_ON
    unsigned* buf = (unsigned*)(req->buffer + req->state.local_offset);
#endif
    TRACE_ERR("  Sub     index=%u  bytes=%zu  l-offset=%zu  r-offset=%zu  buf=%p  *(int*)buf=0x%08x\n",
	      req->state.index, params.rma.bytes, params.addr.local, params.addr.remote, buf, *buf);

    /** sync->total will be updated with every RMA and the complete
	will not change till that RMA has completed. In the meanwhile
	the rest of the RMAs will have memory leaks */
    if (req->target.dt.num_contig - req->state.index == 1) {
        rc = PAMI_Put(context, &params);
        MPID_assert(rc == PAMI_SUCCESS);
        return PAMI_SUCCESS;
    } else {
        rc = PAMI_Put(context, &params);
        MPID_assert(rc == PAMI_SUCCESS);
        req->state.local_offset += params.rma.bytes;
        ++req->state.index;
    }
  }
  return PAMI_SUCCESS;
}
#endif


/**
 * \brief MPI-PAMI glue for MPI_PUT function
 *
 * \param[in] origin_addr      Source buffer
 * \param[in] origin_count     Number of datatype elements
 * \param[in] origin_datatype  Source datatype
 * \param[in] target_rank      Destination rank (target)
 * \param[in] target_disp      Displacement factor in target buffer
 * \param[in] target_count     Number of target datatype elements
 * \param[in] target_datatype  Destination datatype
 * \param[in] win              Window
 * \return MPI_SUCCESS
 */
#undef FUNCNAME
#define FUNCNAME MPID_Put
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPID_Put(const void   *origin_addr,
         int           origin_count,
         MPI_Datatype  origin_datatype,
         int           target_rank,
         MPI_Aint      target_disp,
         int           target_count,
         MPI_Datatype  target_datatype,
         MPID_Win     *win)
{
  int mpi_errno = MPI_SUCCESS;
  int shm_locked=0;
  void * target_addr;
  MPIDI_Win_request *req = MPIU_Calloc0(1, MPIDI_Win_request);
  req->win          = win;
  if(win->mpid.request_based != 1) 
    req->type         = MPIDI_WIN_REQUEST_PUT;
  else {
    req->req_handle   = win->mpid.rreq;
    req->type         = MPIDI_WIN_REQUEST_RPUT;
    req->req_handle->mpid.win_req = req;
  }

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

  req->offset = target_disp * win->mpid.info[target_rank].disp_unit;
#ifdef __BGQ__
  /* PAMI limitation as it doesnt permit VA of 0 to be passed into
   * memregion create, so we must pass base_va of heap computed from
   * an SPI call instead. So the target offset must be adjusted */
  if (req->win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC)
    req->offset -= (size_t)req->win->mpid.info[target_rank].base_addr;
#endif

  MPIDI_Win_datatype_basic(origin_count,
                           origin_datatype,
                           &req->origin.dt);
  MPIDI_Win_datatype_basic(target_count,
                           target_datatype,
                           &req->target.dt);
  #ifndef MPIDI_NO_ASSERT
     MPID_assert(req->origin.dt.size == req->target.dt.size);
  #else
     MPIU_ERR_CHKANDJUMP((req->origin.dt.size != req->target.dt.size), mpi_errno, MPI_ERR_SIZE, "**rmasize");
  #endif



  if ( (req->origin.dt.size == 0) ||
       (target_rank == MPI_PROC_NULL))
    {
      if(req->req_handle)
       MPID_cc_set(req->req_handle->cc_ptr, 0);
      else
       MPIU_Free(req);
      return MPI_SUCCESS;
    }


  if ((target_rank == win->comm_ptr->rank) || (win->create_flavor == MPI_WIN_FLAVOR_SHARED))
     {
       size_t offset = req->offset;
       if (target_rank == win->comm_ptr->rank)
           target_addr = win->base + req->offset;
       else
           target_addr = win->mpid.info[target_rank].base_addr + req->offset;

       mpi_errno = MPIR_Localcopy(origin_addr,
                                  origin_count,
                                  origin_datatype,
                                  target_addr,
                                  target_count,
                                  target_datatype);

      /* The instant this completion counter is set to zero another thread
       * may notice the change and begin freeing request resources. The
       * thread executing the code in this function must not touch any
       * portion of the request structure after decrementing the completion
       * counter.
       *
       * See MPID_Request_release_inline()
       */
       if(req->req_handle)
         MPID_cc_set(req->req_handle->cc_ptr, 0);
       else
         MPIU_Free(req);
       return mpi_errno;
     }
  req->target.rank = target_rank;


  /* Only pack the origin data if the origin is non-contiguous and we are using the simple PAMI_Rput.
   * If we are using the typed PAMI_Rput_typed use the origin address as-is, if we are using the simple
   * PAMI_Rput with contiguous data use the origin address with the lower-bound adjustment.
   */
  if (req->origin.dt.contig || (!req->origin.dt.contig && (MPIDI_Process.typed_onesided == 1)))
    {
      req->buffer_free = 0;
      if ((req->origin.dt.contig && req->target.dt.contig && (MPIDI_Process.typed_onesided == 1)) || (!(MPIDI_Process.typed_onesided == 1)))
        req->buffer      = (void *) ((uintptr_t) origin_addr + req->origin.dt.true_lb);
      else
        req->buffer      = (void *) ((uintptr_t) origin_addr);
    }
  else
    {
      req->buffer_free = 1;
      req->buffer      = MPIU_Malloc(req->origin.dt.size);
      MPID_assert(req->buffer != NULL);

      int mpi_errno = 0;
      mpi_errno = MPIR_Localcopy(origin_addr,
                                 origin_count,
                                 origin_datatype,
                                 req->buffer,
                                 req->origin.dt.size,
                                 MPI_CHAR);
      MPID_assert(mpi_errno == MPI_SUCCESS);
    }


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

#ifdef USE_PAMI_RDMA
  size_t length_out;
  rc = PAMI_Memregion_create(MPIDI_Context[0],
			     req->buffer,
			     req->origin.dt.size,
			     &length_out,
			     &req->origin.memregion);
  MPID_assert(rc == PAMI_SUCCESS);
  MPID_assert(req->origin.dt.size == length_out);
#else
  if(!MPIDI_Process.mp_s_use_pami_get)
    {
      size_t length_out;
      rc = PAMI_Memregion_create(MPIDI_Context[0],
				 req->buffer,
				 req->origin.dt.size,
				 &length_out,
				 &req->origin.memregion);
      if(rc == PAMI_SUCCESS) {
	MPID_assert(req->origin.dt.size == length_out);
	req->origin.memregion_used = 1;
      }
    }
#endif


  if ((!req->target.dt.contig || !req->origin.dt.contig) && (MPIDI_Process.typed_onesided == 1))
    /* If the datatype is non-contiguous and the PAMID typed_onesided optimization
     * is enabled then we will be using the typed interface and will only make 1 call.
     */
    win->mpid.sync.total = 1;
  else {
    MPIDI_Win_datatype_map(&req->target.dt);
    win->mpid.sync.total += req->target.dt.num_contig;
  }
  if ((MPIDI_Process.typed_onesided == 1) && (!req->target.dt.contig || !req->origin.dt.contig)) {
    /* We will use the PAMI_Rput_typed call so we need to make sure any MPI_Type_free before the context
     * executes the put does not free the MPID_Datatype, which would also free the associated PAMI datatype which
     * is still needed for communication -- decrement the ref in the callback to allow the MPID_Datatype
     * to be freed once the PAMI communication has completed.
     */
    MPID_Datatype_add_ref(req->origin.dt.pointer);
    MPID_Datatype_add_ref(req->target.dt.pointer);
  }
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
  PAMI_Context_post(MPIDI_Context[0], &req->post_request, MPIDI_Put, req);  

fn_fail:
  return mpi_errno;
}
