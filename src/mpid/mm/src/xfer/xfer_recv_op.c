/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static void get_xfer_recv_request(MPID_Request *request_ptr, int src, MPID_Request **ppRequest, BOOL *pbNeedHeader)
{
    MM_Car *pCarIter;
    MPID_Request *pRequestIter;

    /* *pbNeedHeader must be set to TRUE before calling this function */

    /* allocate a new request structure */
    *ppRequest = mm_request_alloc();

    /* Find out if the read car in this request needs to be put at the end of a car chain or not.
     * If there already is a receive operation from the same source then this recv_op gets put at the
     * end of that chain.  Otherwise this recv_op starts a new chain and pbNeedHeader is set to TRUE
     */
    pRequestIter = request_ptr;
    /* check the receive car in the first request */
    pCarIter = pRequestIter->mm.rcar;
    if (pCarIter->type & MM_HEAD_CAR)
    {
	if (pCarIter->src == src)
	{
	    /* the source matches so add this recv_op to the end of the matched chain */
	    while (pCarIter->next_ptr)
	    {
		pCarIter = pCarIter->next_ptr;
	    }
	    pCarIter->next_ptr = (*ppRequest)->mm.rcar;
	    *pbNeedHeader = FALSE;
	}
    }
    /* iterate through the rest of the request list */
    while (pRequestIter->mm.next_ptr)
    {
	pRequestIter = pRequestIter->mm.next_ptr;
	/* if the car has been matched (*pbNeedHeader == FALSE), skip this matching block and 
	   simply iterate to the end of the list of requests */
	if (*pbNeedHeader)
	{
	    pCarIter = pRequestIter->mm.rcar;
	    if (pCarIter->type & MM_HEAD_CAR)
	    {
		if (pCarIter->src == src)
		{
		    /* the source matches so add this recv_op to the end of the matched chain */
		    while (pCarIter->next_ptr)
		    {
			pCarIter = pCarIter->next_ptr;
		    }
		    pCarIter->next_ptr = (*ppRequest)->mm.rcar;
		    *pbNeedHeader = FALSE;
		}
	    }
	}
    }

    /* add the newly allocated request to the end of the list */
    pRequestIter->mm.next_ptr = (*ppRequest);
}

/*@
   xfer_recv_op - xfer_recv_op

   Parameters:
+  MPID_Request *request_ptr - request
.  void *buf - buffer
.  int count - count
.  MPI_Datatype dtype - datatype
.  int first - first
.  int last - last
-  int src - source

   Notes:
@*/
int xfer_recv_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src)
{
    MM_Car *pCar;
    MPID_Request *pRequest;
    BOOL bNeedHeader = TRUE;
    long dtype_sz;
    MPIDI_STATE_DECL(MPID_STATE_XFER_RECV_OP);

    MPIDI_FUNC_ENTER(MPID_STATE_XFER_RECV_OP);
    dbg_printf("xfer_recv_op\n");

    /* Get a pointer to the current unused request, allocating if necessary. */
    if (request_ptr->mm.op_valid == FALSE)
    {
	/* This is the common case */
	pRequest = request_ptr;
    }
    else
    {
	/* This is the case for collective operations */
	/* Does putting the uncommon case in a function call make the 
	 * common case faster?  The code used to be inline here.  Having a
	 * function call makes the else jump shorter.
	 * Does this prevent speculative branch execution down the else block?
	 * Does this do nothing for the common case and therefore only slow
	 * down the collective case?
	 */
	get_xfer_recv_request(request_ptr, src, &pRequest, &bNeedHeader);
    }

    pRequest->mm.op_valid = TRUE;
    pRequest->comm = request_ptr->comm;
    pRequest->cc_ptr = &request_ptr->cc;
    pRequest->mm.next_ptr = NULL;

    /* Save the mpi segment */
    /* These fields may not be necessary since we have MPID_Segment_init */
    pRequest->mm.user_buf.recv = buf;
    pRequest->mm.count = count;
    pRequest->mm.dtype = dtype;
    pRequest->mm.first = first;
    MPID_Datatype_get_size_macro(dtype, dtype_sz);
    /*if (dtype_sz == 0) err_printf("Help the russians are coming!!! dtype_sz == 0.\n");*/
    pRequest->mm.size = count * dtype_sz;
    /*printf("recv(%d): count(%d) * size(%d) = %d\n", pRequest->mm.tag, count, dtype_sz, pRequest->mm.size);fflush(stdout);*/
    pRequest->mm.last = (last == MPID_DTYPE_END) ? pRequest->mm.size : last;

    MPID_Segment_init(buf, count, dtype, &pRequest->mm.segment, 0);

    /* set up the read car */
    if (bNeedHeader)
    {
	pRequest->mm.rcar[0].type = MM_HEAD_CAR | MM_READ_CAR;
	pRequest->mm.rcar[0].src = src;
	pRequest->mm.rcar[0].request_ptr = pRequest;
	pRequest->mm.rcar[0].vc_ptr = mm_vc_from_communicator(request_ptr->comm, src);
	pRequest->mm.rcar[0].buf_ptr = &pRequest->mm.rcar[0].msg_header.buf;
	pRequest->mm.rcar[0].msg_header.pkt.u.type = MPID_EAGER_PKT; /* this should be set by the method */
	pRequest->mm.rcar[0].msg_header.pkt.u.hdr.context = request_ptr->comm->context_id;
	pRequest->mm.rcar[0].msg_header.pkt.u.hdr.tag = request_ptr->mm.tag;
	pRequest->mm.rcar[0].msg_header.pkt.u.hdr.src = src;
	pRequest->mm.rcar[0].msg_header.pkt.u.hdr.size = 0;
	pRequest->mm.rcar[0].opnext_ptr = &pRequest->mm.rcar[1];
	pRequest->mm.rcar[0].next_ptr = &pRequest->mm.rcar[1];
	pRequest->mm.rcar[0].qnext_ptr = NULL;
	/*printf("inc cc: read head car\n");fflush(stdout);*/
	mm_inc_cc(pRequest);

	pCar = &pRequest->mm.rcar[1];
	pCar->vc_ptr = pRequest->mm.rcar[0].vc_ptr;
    }
    else
    {
	pCar = pRequest->mm.rcar;
	pCar->vc_ptr = mm_vc_from_communicator(request_ptr->comm, src);
    }

    pCar->type = MM_READ_CAR;
    pCar->request_ptr = pRequest;
    pCar->buf_ptr = &pRequest->mm.buf;
    pCar->src = src;
    pCar->next_ptr = NULL;
    pCar->opnext_ptr = NULL;
    pCar->qnext_ptr = NULL;
    /*printf("inc cc: read data car\n");fflush(stdout);*/
    mm_inc_cc(pRequest);

    /* setup a write car for unpacking */
    pRequest->mm.write_list = &pRequest->mm.wcar[0];
    pCar = pRequest->mm.wcar;
    pCar->type = MM_WRITE_CAR | MM_UNPACKER_CAR;
    pCar->request_ptr = pRequest;
    pCar->buf_ptr = &pRequest->mm.buf;
    pCar->vc_ptr = MPID_Process.unpacker_vc_ptr;
    pCar->next_ptr = NULL;
    pCar->opnext_ptr = NULL;
    pCar->qnext_ptr = NULL;
    /*printf("inc cc: recv unpack car\n");fflush(stdout);*/
    mm_inc_cc(pRequest);

    MPIDI_FUNC_EXIT(MPID_STATE_XFER_RECV_OP);
    return MPI_SUCCESS;
}
