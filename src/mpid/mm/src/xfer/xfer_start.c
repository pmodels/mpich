/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_start - xfer_start

   Parameters:
+  MPID_Request *request_ptr - request

   Notes:
@*/
int xfer_start(MPID_Request *request_ptr)
{
    int mpi_errno;
    MPID_Request *pRequest;
    MM_Car *pCar, *pCarIter;
    MPIDI_STATE_DECL(MPID_STATE_XFER_START);

    MPIDI_FUNC_ENTER(MPID_STATE_XFER_START);

    assert(request_ptr);

    /* choose the buffers scheme to satisfy each segment */
    pRequest = request_ptr;
    do
    {
	/* choose the buffer */
	mpi_errno = mm_choose_buffer(pRequest);
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_XFER_START);
	    return mpi_errno;
	}
	/* reset the cars */
	mpi_errno = mm_reset_cars(pRequest);
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_XFER_START);
	    return mpi_errno;
	}
	pRequest = pRequest->mm.next_ptr;
    } while (pRequest);

    /* enqueue the head cars */
    pRequest = request_ptr;
    do
    {
	if (pRequest->mm.rcar[0].type & MM_HEAD_CAR)
	{
	    /* add up the size of the message and put it in the packet */
	    pRequest->mm.rcar[0].msg_header.pkt.u.hdr.size = 0;
	    pCarIter = pRequest->mm.rcar->next_ptr;
#ifdef MPICH_DEV_BUILD
	    if (pCarIter == NULL) err_printf("Error: empty receive posted.\n");
#endif
	    while (pCarIter)
	    {
		pRequest->mm.rcar[0].msg_header.pkt.u.hdr.size += pCarIter->request_ptr->mm.size;
		pCarIter = pCarIter->next_ptr;
	    }
	    /* post the recv */
#ifdef MPICH_DEV_BUILD
	    if (pRequest->mm.rcar[0].msg_header.pkt.u.hdr.size == 0) err_printf("Error: posting empty receive.\n");
#endif
	    mm_post_recv(pRequest->mm.rcar);
	}
	/* check if this is a packer car */
	/* packer cars are not matched */
	if (pRequest->mm.rcar[0].type & MM_PACKER_CAR)
	{
	    packer_post_read(MPID_Process.packer_vc_ptr, &pRequest->mm.rcar[0]);
	}
	
	pCar = pRequest->mm.write_list;
	while (pCar)
	{
	    if (pCar->type & MM_HEAD_CAR)
	    {
		/* add up the size of the message and put it in the packet */
		pCar->msg_header.pkt.u.hdr.size = 0;
		/* figure out the total size by adding up the size fields of the data cars */
		pCarIter = pCar->next_ptr; /* skip the header car when calculating the size */
		while (pCarIter)
		{
		    pCar->msg_header.pkt.u.hdr.size += pCarIter->request_ptr->mm.size;
		    pCarIter = pCarIter->next_ptr;
		}
		/* enqueue the send */
		mm_post_send(pCar);
	    }
	    if (pCar->type & MM_UNPACKER_CAR)
		pCar->vc_ptr->fn.post_write(pCar->vc_ptr, pCar);

	    pCar = pCar->opnext_ptr;
	}
	pRequest = pRequest->mm.next_ptr;
    } while (pRequest);

    MPIDI_FUNC_EXIT(MPID_STATE_XFER_START);
    return MPI_SUCCESS;
}
