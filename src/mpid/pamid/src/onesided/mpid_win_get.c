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
 * \file src/onesided/mpid_win_get.c
 * \brief ???
 */
#include "mpidi_onesided.h"


static inline int
MPIDI_Get_use_pami_rget(pami_context_t context, MPIDI_Win_request * req, int *freed)
__attribute__((__always_inline__));
#ifdef RDMA_FAILOVER
static inline int
MPIDI_Get_use_pami_get(pami_context_t context, MPIDI_Win_request * req, int *freed)
__attribute__((__always_inline__));
#endif

static pami_result_t
MPIDI_Get(pami_context_t   context,
          void           * _req)
{
  MPIDI_Win_request *req = (MPIDI_Win_request*)_req;
  pami_result_t rc;
  int  freed=0;

#ifdef USE_PAMI_RDMA
  rc = MPIDI_Get_use_pami_rget(context,req,&freed);
#else
  if( (req->origin.memregion_used) &&
      (req->win->mpid.info[req->target.rank].memregion_used) )
    {
      rc = MPIDI_Get_use_pami_rget(context,req,&freed);
    } else {
      rc = MPIDI_Get_use_pami_get(context,req,&freed);
    }
#endif
  if(rc == PAMI_EAGAIN)
    return rc;

  if (!freed)
      MPIDI_Win_datatype_unmap(&req->target.dt);

  return PAMI_SUCCESS;
}


static inline int
MPIDI_Get_use_pami_rget(pami_context_t context, MPIDI_Win_request * req, int *freed)
{
  pami_result_t rc;
  void  *map=NULL;

  pami_rget_simple_t params = {
    .rma = {
      .dest = req->dest,
      .hints = {
	.buffer_registered = PAMI_HINT_ENABLE,
	.use_rdma          = PAMI_HINT_ENABLE,
      },
      .bytes   = 0,
      .cookie  = req,
      .done_fn = MPIDI_Win_DoneCB,
    },
    .rdma = {
      .local = {
	.mr = &req->origin.memregion,
      },
      .remote = {
	.mr     = &req->win->mpid.info[req->target.rank].memregion,
	.offset = req->offset,
      },
    },
  };

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
    TRACE_ERR("  Sub     index=%u  bytes=%zu  l-offset=%zu  r-offset=%zu  buf=%p  *(int*)buf=0x%08x\n", req->state.index, params.rma.bytes, params.rdma.local.offset, params.rdma.remote.offset, buf, *buf);
      if (sync->total - sync->complete == 1) {
          map=NULL;
          if (req->target.dt.map != &req->target.dt.__map) {
              map=(void *) req->target.dt.map;
          }
          rc = PAMI_Rget(context, &params);
          MPID_assert(rc == PAMI_SUCCESS);
          if (map)
              MPIU_Free(map);
          *freed=1;
          return PAMI_SUCCESS;
      } else {
          rc = PAMI_Rget(context, &params);
          MPID_assert(rc == PAMI_SUCCESS);
          req->state.local_offset += params.rma.bytes;
          ++req->state.index;
      }
  }
  return PAMI_SUCCESS;
}


#ifdef RDMA_FAILOVER
static inline int
MPIDI_Get_use_pami_get(pami_context_t context, MPIDI_Win_request * req, int *freed)
{
  pami_result_t rc;
  void  *map=NULL;

  pami_get_simple_t params = {
    .rma = {
      .dest = req->dest,
      .hints = {
	.use_rdma          = PAMI_HINT_DEFAULT,
#ifndef OUT_OF_ORDER_HANDLING
	.no_long_header= 1,
#endif
      },
      .bytes   = 0,
      .cookie  = req,
      .done_fn = MPIDI_Win_DoneCB,
    },
    .addr = {
      .local   = req->buffer,
      .remote  = req->win->mpid.info[req->target.rank].base_addr,
    },
  };

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
    unsigned* buf = (unsigned*)(req->buffer + params.rdma.local.offset);
#endif
    TRACE_ERR("  Sub     index=%u  bytes=%zu  l-offset=%zu  r-offset=%zu  buf=%p  *(int*)buf=0x%08x\n",
	      req->state.index, params.rma.bytes, params.rdma.local.offset, params.rdma.remote.offset, buf, *buf);
    if (sync->total - sync->complete == 1) {
        map=NULL;
        if (req->target.dt.map != &req->target.dt.__map) {
            map=(void *) req->target.dt.map;
        }
        rc = PAMI_Get(context, &params);
        MPID_assert(rc == PAMI_SUCCESS);
        if (map)
            MPIU_Free(map);
        *freed=1;
        return PAMI_SUCCESS;
    } else {
        rc = PAMI_Get(context, &params);
        MPID_assert(rc == PAMI_SUCCESS);
        req->state.local_offset += params.rma.bytes;
        ++req->state.index;
    }
  }
  return PAMI_SUCCESS;
}
#endif


/**
 * \brief MPI-PAMI glue for MPI_GET function
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
int
MPID_Get(void         *origin_addr,
         int           origin_count,
         MPI_Datatype  origin_datatype,
         int           target_rank,
         MPI_Aint      target_disp,
         int           target_count,
         MPI_Datatype  target_datatype,
         MPID_Win     *win)
{
  MPIDI_Win_request *req = MPIU_Calloc0(1, MPIDI_Win_request);
  req->win          = win;
  req->type         = MPIDI_WIN_REQUEST_GET;

  req->offset = target_disp * win->mpid.info[target_rank].disp_unit;

  MPIDI_Win_datatype_basic(origin_count,
                           origin_datatype,
                           &req->origin.dt);
  MPIDI_Win_datatype_basic(target_count,
                           target_datatype,
                           &req->target.dt);
  #ifndef MPIDI_NO_ASSERT
     MPID_assert(req->origin.dt.size == req->target.dt.size);
  #else
  /* temp fix, should be fixed as part of error injection for one sided comm.*/
  /* by 10/12                                                                */
  if (req->origin.dt.size != req->target.dt.size) {
       exit(1);
  }
  #endif

  if ( (req->origin.dt.size == 0) ||
       (target_rank == MPI_PROC_NULL))
    {
      MPIU_Free(req);
      return MPI_SUCCESS;
    }

  /* If the get is a local operation, do it here */
  if (target_rank == win->comm_ptr->rank)
    {
      size_t offset = req->offset;
      MPIU_Free(req);
      return MPIR_Localcopy(win->base + offset,
                            target_count,
                            target_datatype,
                            origin_addr,
                            origin_count,
                            origin_datatype);
    }
  req->target.rank = target_rank;


  if (req->origin.dt.contig)
    {
      req->buffer_free = 0;
      req->buffer      = origin_addr + req->origin.dt.true_lb;
    }
  else
    {
      req->buffer_free = 1;
      req->buffer      = MPIU_Malloc(req->origin.dt.size);
      MPID_assert(req->buffer != NULL);

      MPID_Datatype_add_ref(req->origin.dt.pointer);
      req->origin.addr  = origin_addr;
      req->origin.count = origin_count;
      req->origin.datatype = origin_datatype;
      req->origin.completed = 0;




    }


  pami_result_t rc;
  pami_task_t task = MPID_VCR_GET_LPID(win->comm_ptr->vcr, target_rank);
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
      if(rc == PAMI_SUCCESS)
	{
	  req->origin.memregion_used = 1;
	  MPID_assert(req->origin.dt.size == length_out);
	}
    }
#endif


  MPIDI_Win_datatype_map(&req->target.dt);
  win->mpid.sync.total += req->target.dt.num_contig;


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
  PAMI_Context_post(MPIDI_Context[0], &req->post_request, MPIDI_Get, req);


  return MPI_SUCCESS;
}
