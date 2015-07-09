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
 * \file src/onesided/mpid_win_fetch_and_op.c.c
 * \brief ???
 */
#include "mpidi_onesided.h"

static pami_result_t
MPIDI_Fetch_and_op_using_pami_rmw(pami_context_t   context,
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
    params.operation = req->pami_op;
    params.local=req->user_buffer;  /*result*/
    params.remote=req->win->mpid.info[target_rank].base_addr + req->offset + (size_t)req->origin.dt.map[0].DLOOP_VECTOR_BUF;
    params.value=req->buffer;        /* replaced value with origin */

    rc = PAMI_Rmw(context, &params);
    MPID_assert(rc == PAMI_SUCCESS);
    return rc;
}


void
MPIDI_WinAtomicCB(pami_context_t    context,
		  void            * cookie,
		  const void      * _hdr,
		  size_t            size,
		  const void      * sndbuf,
		  size_t            sndlen,
		  pami_endpoint_t   sender,
		  pami_recv_t     * recv)
{
  MPIDI_AtomicHeader_t *ahdr = (MPIDI_AtomicHeader_t *) _hdr;
  MPID_assert (ahdr != NULL);
  MPID_assert (sizeof(MPIDI_AtomicHeader_t) == size);
  MPIDI_AtomicHeader_t ack_hdr = *ahdr;

  void *dest_addr = ahdr->remote_addr; 
  int len;       
  len = MPID_Datatype_get_basic_size (ahdr->datatype);

  if (ahdr->atomic_type == MPIDI_WIN_REQUEST_COMPARE_AND_SWAP) {

    //overwrite value with result in ack_hdr
    MPIU_Memcpy(ack_hdr.buf, dest_addr, len);
    
    if (MPIR_Compare_equal (&ahdr->test, dest_addr, ahdr->datatype))
      MPIU_Memcpy(dest_addr, ahdr->buf, len);      
  }    
  else if (ahdr->atomic_type == MPIDI_WIN_REQUEST_FETCH_AND_OP) {
    //overwrite value with result
    MPIU_Memcpy(ack_hdr.buf, dest_addr, len);

    MPI_User_function *uop;
    int one = 1;
    uop = MPIR_OP_HDL_TO_FN(ahdr->op);

    if (ahdr->op == MPI_REPLACE) 
      MPIU_Memcpy(dest_addr, ahdr->buf, len);
    else if (ahdr->op == MPI_NO_OP);
    else
      (*uop) ((void *)ahdr->buf, dest_addr, &one, &ahdr->datatype);
  }
  else
    MPID_abort();

  pami_send_immediate_t params = {
    .dispatch = MPIDI_Protocols_WinAtomicAck,
    .dest     = sender,
    .header   = {
      .iov_base = &ack_hdr,
      .iov_len  = sizeof(MPIDI_AtomicHeader_t),
    },
    .data     = {
       .iov_base = NULL,
       .iov_len  = 0,
     },
    .hints = {0}, 
  };
  
  pami_result_t rc = PAMI_Send_immediate(context, &params);  
  MPID_assert(rc == PAMI_SUCCESS);
}

void
MPIDI_WinAtomicAckCB(pami_context_t    context,
		     void            * cookie,
		     const void      * _hdr,
		     size_t            size,
		     const void      * sndbuf,
		     size_t            sndlen,
		     pami_endpoint_t   sender,
		     pami_recv_t     * recv)
{
  int len;       
  MPIDI_AtomicHeader_t *ahdr = (MPIDI_AtomicHeader_t *) _hdr;
  //We have a valid result addr
  if (ahdr->result_addr != NULL) {
    len = MPID_Datatype_get_basic_size (ahdr->datatype);
    MPIU_Memcpy(ahdr->result_addr, ahdr->buf, len);
  }
    
  MPIDI_Win_DoneCB(context, ahdr->request_addr, PAMI_SUCCESS);
}


pami_result_t
MPIDI_Atomic (pami_context_t   context,
	      void           * _req)
{
  MPIDI_Win_request *req = (MPIDI_Win_request*)_req;
  pami_result_t rc;
  MPIDI_AtomicHeader_t atomic_hdr;
  int len;

  len = MPID_Datatype_get_basic_size (req->origin.datatype);
  assert(len <= MAX_ATOMIC_TYPE_SIZE);
  if (req->buffer)
    MPIU_Memcpy(atomic_hdr.buf, req->buffer, len);
  if (req->type == MPIDI_WIN_REQUEST_COMPARE_AND_SWAP)
    MPIU_Memcpy(atomic_hdr.test, req->compare_buffer, len);
  
  atomic_hdr.result_addr = req->user_buffer;
  atomic_hdr.remote_addr = req->win->mpid.info[req->target.rank].base_addr + req->offset;
  atomic_hdr.request_addr = req;
  atomic_hdr.datatype = req->origin.datatype;
  atomic_hdr.atomic_type = req->type;
  atomic_hdr.op = req->op;
  
  struct MPIDI_Win_sync* sync = &req->win->mpid.sync;
  MPID_assert (req->origin.dt.num_contig == 1);  
  ++sync->started;

  pami_send_immediate_t params = {
    .dispatch = MPIDI_Protocols_WinAtomic,
    .dest     = req->dest,
    .header   = {
      .iov_base = &atomic_hdr,
      .iov_len  = sizeof(MPIDI_AtomicHeader_t),
    },
    .data     = {
       .iov_base = NULL,
       .iov_len  = 0,
     },
    .hints = {0}, 
  };
  
  rc = PAMI_Send_immediate(context, &params);  
  MPID_assert(rc == PAMI_SUCCESS);
  return PAMI_SUCCESS;  
}


#define FUNCNAME MPIDI_Fetch_and_op
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Fetch_and_op(const void *origin_addr, void *result_addr,
                      MPI_Datatype datatype, int target_rank,
                      MPI_Aint target_disp, MPI_Op op, MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  MPIDI_Win_request *req;
  int good_for_rmw=0;
  int count = 1;
  int shm_locked = 0;

  if(win->mpid.sync.origin_epoch_type == win->mpid.sync.target_epoch_type &&
     win->mpid.sync.origin_epoch_type == MPID_EPOTYPE_REFENCE){
     win->mpid.sync.origin_epoch_type = MPID_EPOTYPE_FENCE; win->mpid.sync.target_epoch_type = MPID_EPOTYPE_FENCE;
  }

  if(win->mpid.sync.origin_epoch_type == MPID_EPOTYPE_NONE ||
     win->mpid.sync.origin_epoch_type == MPID_EPOTYPE_POST){
    MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
  }

  pami_type_t         pami_type;
  pami_atomic_t  pami_op;

  if (target_rank == MPI_PROC_NULL)
      return MPI_SUCCESS;

  MPI_Datatype basic_type = MPI_DATATYPE_NULL;
  MPID_Datatype_get_basic_type(datatype, basic_type);
  if ((datatype == MPI_FLOAT_INT)  ||
      (datatype == MPI_DOUBLE_INT) ||
      (datatype == MPI_LONG_INT)   ||
      (datatype == MPI_SHORT_INT)  ||
      (datatype == MPI_LONG_DOUBLE_INT))
    {
      MPID_assert(basic_type == MPI_DATATYPE_NULL);
      basic_type = datatype;
    }
    MPID_assert(basic_type != MPI_DATATYPE_NULL);

  if(MPIDI_Datatype_is_pami_rmw_supported(basic_type, &pami_type, op, &pami_op)  ) {
#ifndef __BGQ__
    good_for_rmw = 1; 
#endif
  } else {
     if((op == MPI_NO_OP) && (origin_addr == NULL) && (win->create_flavor != MPI_WIN_FLAVOR_SHARED) ) {
        /* essentially a MPI_Get to result buffer */
        MPID_Get(result_addr, 1, datatype, target_rank,
	 	 target_disp, 1, datatype, win);
	return 0;
    }  
  }

  req = (MPIDI_Win_request *) MPIU_Calloc0(1, MPIDI_Win_request);
  req->win          = win;
  req->type         = MPIDI_WIN_REQUEST_FETCH_AND_OP;

  req->offset = target_disp * win->mpid.info[target_rank].disp_unit;
#ifdef __BGQ__
  /* PAMI limitation as it doesnt permit VA of 0 to be passed into
   * memregion create, so we must pass base_va of heap computed from
   * an SPI call instead. So the target offset must be adjusted */
  if (req->win->create_flavor == MPI_WIN_FLAVOR_DYNAMIC)
    req->offset -= (size_t)req->win->mpid.info[target_rank].base_addr;
#endif

  if (datatype == MPI_DOUBLE_INT)
    {
      MPIDI_Win_datatype_basic(count*2,
                               MPI_DOUBLE,
                               &req->origin.dt);
    }
  else if (datatype == MPI_LONG_DOUBLE_INT)
    {
      MPIDI_Win_datatype_basic(count*2,
                               MPI_LONG_DOUBLE,
                               &req->origin.dt);
    }
  else if (datatype == MPI_LONG_INT)
    {
      MPIDI_Win_datatype_basic(count*2,
                               MPI_LONG,
                               &req->origin.dt);
    }
  else if (datatype == MPI_SHORT_INT)
    {
      MPIDI_Win_datatype_basic(count*2,
                               MPI_INT,
                               &req->origin.dt);
    }
  else
    {
      MPIDI_Win_datatype_basic(count,
                               datatype,
                               &req->origin.dt);
    }


  if (req->origin.dt.size == 0) 
    {
      MPIU_Free(req);
      return MPI_SUCCESS;
    }

  req->target.rank = target_rank;

    req->compare_buffer = NULL;
    req->pami_op = pami_op;
    req->op = op;
    req->pami_datatype = pami_type;
    /* MPI_Fetch_and_op only supports predefined datatype */
    req->buffer           = (void *) ((uintptr_t) origin_addr + req->origin.dt.true_lb);
    req->user_buffer      = result_addr + req->origin.dt.true_lb;

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
    win->mpid.sync.total += req->origin.dt.num_contig;
    req->origin.datatype= basic_type;

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
    if(good_for_rmw) {
      PAMI_Context_post(MPIDI_Context[0], &req->post_request, MPIDI_Fetch_and_op_using_pami_rmw, req);
    } else {
      PAMI_Context_post(MPIDI_Context[0], &req->post_request, MPIDI_Atomic, req);

    }

fn_fail:
  return mpi_errno;
}


int MPIDI_Datatype_is_pami_rmw_supported(MPI_Datatype datatype, pami_type_t *pami_type, MPI_Op op, pami_atomic_t *pami_op)
{
  int null=0;
  MPI_Op null_op=0;
  int rc = FALSE;
  pami_data_function pami_data_fn;

  MPIDI_Datatype_to_pami(datatype, pami_type, op, &pami_data_fn, &null);

  if(*pami_type == PAMI_TYPE_SIGNED_INT || 
     *pami_type == PAMI_TYPE_UNSIGNED_INT ||
     *pami_type == PAMI_TYPE_SIGNED_LONG || 
     *pami_type == PAMI_TYPE_UNSIGNED_LONG ||
     *pami_type == PAMI_TYPE_SIGNED_LONG_LONG || 
     *pami_type == PAMI_TYPE_SIGNED_LONG_LONG) { 
     if(op == null_op) {
	rc = TRUE;
     } else if (op == MPI_SUM) {
        *pami_op = PAMI_ATOMIC_FETCH_ADD;
	rc = TRUE;
     } else if (op == MPI_BOR) {
        *pami_op = PAMI_ATOMIC_FETCH_OR;
        rc = TRUE;
     } else if (op == MPI_BAND) {
        *pami_op = PAMI_ATOMIC_FETCH_AND;
	rc = TRUE;
     } else if (op == MPI_BXOR) {
        *pami_op = PAMI_ATOMIC_FETCH_XOR;
	rc = TRUE;
     } else if (op == MPI_REPLACE) {
        *pami_op = PAMI_ATOMIC_FETCH_SET;
	rc = TRUE;
     } else if (op == MPI_NO_OP) {
        *pami_op = PAMI_ATOMIC_FETCH;
	rc = TRUE;
     }
  }
  return rc;
}
