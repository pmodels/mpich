/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   xfer_scatter_send_op - xfer_scatter_send_op

   Parameters:
+  MPID_Request *request_ptr - request
.  const void *buf - buffer
.  int count - count
.  MPI_Datatype dtype - datatype
.  int first - first
.  int last - last
-  int dest - destination

   Notes:
@*/
int xfer_scatter_send_op(MPID_Request *request_ptr, const void *buf, int count, MPI_Datatype dtype, int first, int last, int dest)
{
    MM_Car *pCar;
    MPID_Request *pRequest;

    /* Get a pointer to the current request.
       Either the first request, allocated by scatter_init,
       will be unused or we will allocate a new request at
       the end of the list */
    if (request_ptr->mm.op_valid == FALSE)
    {
	pRequest = request_ptr;
    }
    else
    {
	pRequest = request_ptr;
	while (pRequest->mm.next_ptr)
	    pRequest = pRequest->mm.next_ptr;
	pRequest->mm.next_ptr = mm_request_alloc();
	pRequest = pRequest->mm.next_ptr;
    }

    pRequest->mm.op_valid = TRUE;
    pRequest->cc_ptr = &request_ptr->cc;
    pRequest->mm.next_ptr = NULL;

    /* Save the mpi segment */
    pRequest->mm.sbuf = buf;
    pRequest->mm.count = count;
    pRequest->mm.dtype = dtype;
    pRequest->mm.first = first;
    pRequest->mm.last = last;

    /* allocate a write car */
    if (request_ptr->mm.write_list == NULL)
    {
	request_ptr->mm.write_list = &request_ptr->mm.wcar;
	pCar = &request_ptr->mm.wcar;
    }
    else
    {
	pCar = request_ptr->mm.write_list;
	while (pCar->next_ptr)
	    pCar = pCar->next_ptr;
	pCar->next_ptr = mm_car_alloc();
	pCar = pCar->next_ptr;
    }
    
    pCar->request_ptr = request_ptr;
    pCar->type = MM_UNBOUND_WRITE_CAR;
    pCar->vc_ptr = mm_get_vc(request_ptr->comm, dest);
    pCar->dest = dest;
    pCar->next_ptr = NULL;

    return MPI_SUCCESS;
}
