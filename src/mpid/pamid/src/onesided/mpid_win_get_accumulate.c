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
 * \file src/onesided/mpid_win_accumulate.c
 * \brief ???
 */
#include "mpidi_onesided.h"

static void 
MPIDI_Fetch_data_op(const void   * origin_addr,
                    int            origin_count,
                    MPI_Datatype   origin_datatype,
                    void         * result_addr,
                    int            target_rank,
                    MPI_Aint       target_disp,
                    int            target_count,
                    MPI_Datatype   target_datatype,
                    MPI_Op         op,
                    MPID_Win      *win)
{
        static char FCNAME[] = "MPIDI_Fetch_data_op";
        int shm_locked=0;
        MPI_User_function *uop;
        void *base, *dest_addr;
        int disp_unit;
        int len, one;

           base = win->mpid.info[target_rank].base_addr;
           disp_unit = win->mpid.info[target_rank].disp_unit;
        dest_addr = (char *) base + disp_unit * target_disp;

        MPID_Datatype_get_size_macro(origin_datatype, len);
        MPIU_Memcpy(result_addr, dest_addr, len);
        if (op != MPI_NO_OP) {
            uop = MPIR_OP_HDL_TO_FN(op);
            one = 1;
            (*uop)((void *) origin_addr, dest_addr, &one, &origin_datatype);
        }

   fn_fail: return;
}


static void 
MPIDI_Win_GetAccSendAckDoneCB(pami_context_t   context,
			     void           * _info,
			     pami_result_t    result)
{
  MPIDI_Win_GetAccMsgInfo *msginfo = (MPIDI_Win_GetAccMsgInfo *) _info;
  MPIU_Free(msginfo->tptr);
  MPIU_Free(msginfo);
}

static void 
MPIDI_Win_GetAccumSendAck(pami_context_t   context,
			  void           * _info,
			  pami_result_t    result)
{
  static char FCNAME[] = "MPID_Win_GetAccumSendAck";
  MPIDI_Win_GetAccMsgInfo *msginfo = (MPIDI_Win_GetAccMsgInfo *) _info;  
  pami_result_t rc = PAMI_SUCCESS;

  //Copy from msginfo->addr to a contiguous buffer
  char *buffer = NULL;

  buffer      = MPIU_Malloc(msginfo->size);
  MPID_assert(buffer != NULL);
  
  if (msginfo->num_contig == 1)
    memcpy(buffer, msginfo->addr, msginfo->size);
  else
    {
      int mpi_errno = 0;
      mpi_errno = MPIR_Localcopy(msginfo->addr,
                                 msginfo->count,
                                 msginfo->type,
                                 buffer,
                                 msginfo->size,
                                 MPI_CHAR);
      MPID_assert(mpi_errno == MPI_SUCCESS);      
    }
  //Schedule sends to source to result buffer and trigger completion
  //callback there
  pami_send_t params = {
    .send = {
      .header = {
	 .iov_base = msginfo,
	 .iov_len = sizeof(MPIDI_Win_GetAccMsgInfo),
       },
      .dispatch = MPIDI_Protocols_WinGetAccumAck,
      .dest     = msginfo->src_endpoint,
    },
    .events = {
       .cookie   = msginfo,
       .local_fn = MPIDI_Win_GetAccSendAckDoneCB, //cleanup buffer
     },
  };

  msginfo->tptr = buffer;

  params.send.data.iov_len    = msginfo->size;
  params.send.data.iov_base   = buffer;
  
  rc = PAMI_Send(context, &params);
  MPID_assert(rc == PAMI_SUCCESS);
  fn_fail: return;
}

void
MPIDI_WinGetAccumCB(pami_context_t    context,
		    void            * cookie,
		    const void      * _msginfo,
		    size_t            msginfo_size,
		    const void      * sndbuf,
		    size_t            sndlen,
		    pami_endpoint_t   sender,
		    pami_recv_t     * recv)
{
  MPID_assert(recv   != NULL);
  MPID_assert(sndbuf == NULL);
  MPID_assert(msginfo_size == sizeof(MPIDI_Win_GetAccMsgInfo));
  MPID_assert(_msginfo != NULL);
  MPIDI_Win_GetAccMsgInfo * msginfo = (MPIDI_Win_GetAccMsgInfo *) 
    MPIU_Malloc(sizeof(MPIDI_Win_GetAccMsgInfo));

  *msginfo = *(MPIDI_Win_GetAccMsgInfo *)_msginfo;
  msginfo->src_endpoint = sender;

  int null=0;
  pami_type_t         pami_type;
  pami_data_function  pami_op;
  MPI_Op op = msginfo->op;

  MPIDI_Datatype_to_pami(msginfo->type, &pami_type, op, &pami_op, &null);
  
  recv->addr        = msginfo->addr;
  recv->type        = pami_type;
  recv->offset      = 0;
  recv->data_fn     = pami_op;
  recv->data_cookie = NULL;
  recv->local_fn    = NULL; 
  recv->cookie      = NULL; 
  
  if (msginfo->counter == 0) 
    //We will now allocate a tempbuf, copy local contents and start a
    //send
    MPIDI_Win_GetAccumSendAck (context, msginfo, PAMI_SUCCESS);  
  else 
    MPIU_Free(msginfo);
}

static void
MPIDI_Win_GetAccDoneCB(pami_context_t  context,
		       void          * cookie,
		       pami_result_t   result)
{
  MPIDI_Win_request *req = (MPIDI_Win_request*)cookie;
  ++req->win->mpid.sync.complete;
  ++req->origin.completed;

  if (req->origin.completed == req->target.dt.num_contig + 1)
    {
      MPID_Request * req_handle = req->req_handle;

      if (req->buffer_free) {
          MPIU_Free(req->buffer);
          req->buffer_free = 0;
      }
      if (req->accum_headers)
          MPIU_Free(req->accum_headers);

      MPIDI_Win_datatype_unmap(&req->target.dt);
      MPIDI_Win_datatype_unmap(&req->result.dt);      

      if( req->type != MPIDI_WIN_REQUEST_RGET_ACCUMULATE ) {
          if (req_handle) {
              req_handle->mpid.win_req = NULL;
          }
          MPIU_Free(req);
      }
      /* The instant this completion counter is set to zero another thread
       * may notice the change and begin freeing request resources. The
       * thread executing the code in this function must not touch any
       * portion of the request structure after decrementing the completion
       * counter.
       *
       * See MPID_Request_release_inline()
       */
      if(req_handle)
          MPID_cc_set(req_handle->cc_ptr, 0);
    }
  MPIDI_Progress_signal();
}

void
MPIDI_Win_GetAccAckDoneCB(pami_context_t   context,
			  void           * _msginfo,
			  pami_result_t    result)
{
  MPIDI_Win_GetAccMsgInfo * msginfo =(MPIDI_Win_GetAccMsgInfo *)_msginfo;
  MPIDI_Win_request *req = (MPIDI_Win_request *) msginfo->request;

  if (req->result_num_contig > 1) {
    MPIR_Localcopy(req->result.addr,
		   req->result.count,
		   req->result.datatype,
		   msginfo->tptr,
		   msginfo->size,
		   MPI_CHAR);
    MPIU_Free(msginfo->tptr);
  }
  MPIU_Free(msginfo);
  
  MPIDI_Win_GetAccDoneCB(context, req, result);
}


void
MPIDI_WinGetAccumAckCB(pami_context_t    context,
		       void            * cookie,
		       const void      * _msginfo,
		       size_t            msginfo_size,
		       const void      * sndbuf,
		       size_t            sndlen,
		       pami_endpoint_t   sender,
		       pami_recv_t     * recv)
{
  MPID_assert(recv   != NULL);
  MPID_assert(sndbuf == NULL);
  MPID_assert(_msginfo != NULL);
  MPIDI_Win_GetAccMsgInfo * msginfo =MPIU_Malloc(sizeof(MPIDI_Win_GetAccMsgInfo));
  *msginfo = *(const MPIDI_Win_GetAccMsgInfo *)_msginfo;
  MPIDI_Win_request *req = (MPIDI_Win_request *) msginfo->request;
  
  msginfo->tptr = NULL;
  recv->addr = req->result.addr;
  if (req->result_num_contig > 1)
    recv->addr = msginfo->tptr = MPIU_Malloc(msginfo->size); 
  
  recv->type        = PAMI_TYPE_BYTE;
  recv->offset      = 0;
  recv->data_fn     = PAMI_DATA_COPY;
  recv->data_cookie = NULL;
  recv->local_fn    = MPIDI_Win_GetAccAckDoneCB;
  recv->cookie      = msginfo;
}

static pami_result_t
MPIDI_Get_accumulate(pami_context_t   context,
		     void           * _req)
{
  MPIDI_Win_request *req = (MPIDI_Win_request*)_req;
  pami_result_t rc;

  pami_send_t params = {
    .send = {
      .header = {
        .iov_len = sizeof(MPIDI_Win_GetAccMsgInfo),
      },
      .dispatch = MPIDI_Protocols_WinGetAccum,
      .dest     = req->dest,
    },
    .events = {
      .cookie    = req,
      .remote_fn = MPIDI_Win_GetAccDoneCB, //When all accumulates have
					   //completed remotely
					   //complete accumulate
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

    params.send.header.iov_base = &(((MPIDI_Win_GetAccMsgInfo *)req->accum_headers)[req->state.index]);
    params.send.data.iov_len    = req->target.dt.map[req->state.index].DLOOP_VECTOR_LEN;
    params.send.data.iov_base   = req->buffer + req->state.local_offset;

    if (req->target.dt.num_contig - req->state.index == 1) {
      rc = PAMI_Send(context, &params);
      MPID_assert(rc == PAMI_SUCCESS);
      return PAMI_SUCCESS;
    } else {
      rc = PAMI_Send(context, &params);
      MPID_assert(rc == PAMI_SUCCESS);
      req->state.local_offset += params.send.data.iov_len;
      ++req->state.index;
    }
  }
  return PAMI_SUCCESS;
}


/**
 * \brief MPI-PAMI glue for MPI_GET_ACCUMULATE function
 *
 * According to the MPI Specification:
 *
 *        Each datatype argument must be a predefined datatype or
 *        a derived datatype, where all basic components are of the
 *        same predefined datatype. Both datatype arguments must be
 *        constructed from the same predefined datatype.
 *
 * \param[in] origin_addr      Source buffer
 * \param[in] origin_count     Number of datatype elements
 * \param[in] origin_datatype  Source datatype
 * \param[in] target_rank      Destination rank (target)
 * \param[in] target_disp      Displacement factor in target buffer
 * \param[in] target_count     Number of target datatype elements
 * \param[in] target_datatype  Destination datatype
 * \param[in] op               Operand to perform
 * \param[in] win              Window
 * \return MPI_SUCCESS
 */
#undef FUNCNAME
#define FUNCNAME MPID_Get_accumulate
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int 
MPID_Get_accumulate(const void   * origin_addr, 
		    int            origin_count,
		    MPI_Datatype   origin_datatype, 
		    void         * result_addr, 
		    int            result_count,
		    MPI_Datatype   result_datatype, 
		    int            target_rank, 
		    MPI_Aint       target_disp,
		    int            target_count, 
		    MPI_Datatype   target_datatype, 
		    MPI_Op         op, 
		    MPID_Win      *win)
{
  int mpi_errno = MPI_SUCCESS;

  if (op == MPI_NO_OP) {//we just need to fetch data    
     if (win->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        win->mpid.sync.total++;
        MPIDI_Fetch_data_op(origin_addr, origin_count, origin_datatype,
                           result_addr, target_rank, target_disp,
                           target_count, target_datatype, op, win);
        ++win->mpid.sync.complete;
     } else {
        mpi_errno = MPID_Get(result_addr,
			 result_count,
			 result_datatype,
			 target_rank,
			 target_disp,
			 target_count,
			 target_datatype,
			 win);  
    }
    return mpi_errno;
  }
  
  MPIDI_Win_request *req;
  req = MPIU_Calloc0(1, MPIDI_Win_request);
  req->win      = win;
  if(win->mpid.request_based != 1) 
    req->type         = MPIDI_WIN_REQUEST_GET_ACCUMULATE;
  else {
    req->req_handle   = win->mpid.rreq;
    req->type         = MPIDI_WIN_REQUEST_RGET_ACCUMULATE;
    req->req_handle->mpid.win_req = req;
  }
  req->offset   = target_disp*win->mpid.info[target_rank].disp_unit;
#ifdef __BGQ__
  /* PAMI limitation as it doesnt permit VA of 0 to be passed into
   * memregion create, so we must pass base_va of heap computed from
   * an SPI call instead. So the target offset must be adjusted */
  if (req->win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC)
    req->offset -= (size_t)req->win->mpid.info[target_rank].base_addr;
#endif

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

  if (origin_datatype == MPI_DOUBLE_INT)    {
      MPIDI_Win_datatype_basic(origin_count*2,
                               MPI_DOUBLE,
                               &req->origin.dt);
      MPIDI_Win_datatype_basic(target_count*2,
                               MPI_DOUBLE,
                               &req->target.dt);
    }
  else if (origin_datatype == MPI_LONG_DOUBLE_INT)
    {
      MPIDI_Win_datatype_basic(origin_count*2,
                               MPI_LONG_DOUBLE,
                               &req->origin.dt);
      MPIDI_Win_datatype_basic(target_count*2,
                               MPI_LONG_DOUBLE,
                               &req->target.dt);
    }
  else if (origin_datatype == MPI_LONG_INT)
    {
      MPIDI_Win_datatype_basic(origin_count*2,
                               MPI_LONG,
                               &req->origin.dt);
      MPIDI_Win_datatype_basic(target_count*2,
                               MPI_LONG,
                               &req->target.dt);
    }
  else if (origin_datatype == MPI_SHORT_INT)
    {
      MPIDI_Win_datatype_basic(origin_count*2,
                               MPI_INT,
                               &req->origin.dt);
      MPIDI_Win_datatype_basic(target_count*2,
                               MPI_INT,
                               &req->target.dt);
    }
  else
    {
      MPIDI_Win_datatype_basic(origin_count,
                               origin_datatype,
                               &req->origin.dt);
      MPIDI_Win_datatype_basic(target_count,
                               target_datatype,
                               &req->target.dt);
    }

  MPID_assert(req->origin.dt.size == req->target.dt.size);

  if ( (req->origin.dt.size == 0) ||
       (target_rank == MPI_PROC_NULL))
    {
      if(req->req_handle)
         MPID_cc_set(req->req_handle->cc_ptr, 0);
      else
         MPIU_Free(req);
      return MPI_SUCCESS;
    }

  req->target.rank = target_rank;


  if (req->origin.dt.contig)
    {
      req->buffer_free = 0;
      req->buffer      = (char*)origin_addr + req->origin.dt.true_lb;
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


  MPIDI_Win_datatype_map(&req->target.dt);
  req->result.addr = result_addr;
  req->result.count = result_count;
  req->result.datatype = result_datatype;
  MPIDI_Win_datatype_basic(result_count, result_datatype, &req->result.dt);
  MPIDI_Win_datatype_map(&req->result.dt);
  req->result_num_contig = req->result.dt.num_contig;
  if (win->create_flavor == MPI_WIN_FLAVOR_SHARED)
   {
        win->mpid.sync.total++;
        MPIDI_Fetch_data_op(origin_addr, origin_count, origin_datatype,
                           result_addr, target_rank, target_disp,
                           target_count, target_datatype, op, win);
        ++win->mpid.sync.complete;

       if (req->buffer_free) {
           MPIU_Free(req->buffer);
           MPIU_Free(req->user_buffer);
           req->buffer_free = 0;
       }
       MPIDI_Win_datatype_unmap(&req->target.dt);
       MPIDI_Win_datatype_unmap(&req->result.dt);

       if(req->req_handle) {
          MPID_cc_set(req->req_handle->cc_ptr, 0);
       } else { 
           MPIU_Free(req);
       }
   } else {    /* non-shared  */
  //We wait for #messages depending on target and result_datatype
  win->mpid.sync.total += (1 + req->target.dt.num_contig);
  {
    MPI_Datatype basic_type = MPI_DATATYPE_NULL;
    MPID_Datatype_get_basic_type(origin_datatype, basic_type);
    /* MPID_Datatype_get_basic_type() doesn't handle the struct types */
    if ((origin_datatype == MPI_FLOAT_INT)  ||
        (origin_datatype == MPI_DOUBLE_INT) ||
        (origin_datatype == MPI_LONG_INT)   ||
        (origin_datatype == MPI_SHORT_INT)  ||
        (origin_datatype == MPI_LONG_DOUBLE_INT))
      {
        MPID_assert(basic_type == MPI_DATATYPE_NULL);
        basic_type = origin_datatype;
      }
    MPID_assert(basic_type != MPI_DATATYPE_NULL);

    unsigned index;
    MPIDI_Win_GetAccMsgInfo * headers = MPIU_Calloc0(req->target.dt.num_contig, MPIDI_Win_GetAccMsgInfo);
    req->accum_headers = headers;
    for (index=0; index < req->target.dt.num_contig; ++index) {
     headers[index].addr = win->mpid.info[target_rank].base_addr + req->offset +
                           (size_t)req->target.dt.map[index].DLOOP_VECTOR_BUF;
     headers[index].req  = req;
     headers[index].win  = win;
     headers[index].type = basic_type;
     headers[index].op   = op;
     headers[index].count            = target_count;
     headers[index].counter          = index;
     headers[index].num_contig       = req->target.dt.num_contig;
     headers[index].size             = req->target.dt.size;
     headers[index].request          = req;
    }

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
  PAMI_Context_post(MPIDI_Context[0], &req->post_request, MPIDI_Get_accumulate, req);
 }
fn_fail:
  return mpi_errno;
}
