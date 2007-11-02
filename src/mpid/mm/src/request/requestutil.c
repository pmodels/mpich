/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2001 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#ifndef MPID_REQUEST_PREALLOC
#define MPID_REQUEST_PREALLOC 8
#endif

MPID_Request MPID_Request_direct[MPID_REQUEST_PREALLOC];
MPIU_Object_alloc_t MPID_Request_mem = { 0, 0, 0, 0, 0,
					 sizeof(MPID_Request), MPID_Request_direct,
					 MPID_REQUEST_PREALLOC, };

MPID_Request * mm_request_alloc()
{
    MPID_Request *p;
    MPIDI_STATE_DECL(MPID_STATE_MM_REQUEST_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_REQUEST_ALLOC);

    p = MPIU_Handle_obj_alloc(&MPID_Request_mem);
    if (p == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MM_REQUEST_ALLOC);
	return p;
    }
    p->cc = 0;
    p->cc_ptr = &p->cc;
    /* prevent the built in cars from being freed */
    p->mm.rcar[0].freeme = FALSE;
    p->mm.rcar[1].freeme = FALSE;
    p->mm.wcar[0].freeme = FALSE;
    p->mm.wcar[1].freeme = FALSE;
    p->mm.next_ptr = NULL;

#ifdef MPICH_DEV_BUILD
    /* insert stuff like "ptr = INVALID_POINTER" here */
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MM_REQUEST_ALLOC);
    return p;
}

void mm_request_free(MPID_Request *request_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MM_REQUEST_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_REQUEST_FREE);

    /* insert reference count code here */

    /* free the buffer attached to this request */
    if (request_ptr->mm.release_buffers)
	request_ptr->mm.release_buffers(request_ptr);

    /* free the request */
    MPIU_Handle_obj_free(&MPID_Request_mem, request_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MM_REQUEST_FREE);
}

void MPID_Request_release(MPID_Request *request_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_REQUEST_RELEASE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_REQUEST_RELEASE);

    if (request_ptr == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MPID_REQUEST_RELEASE);
	return;
    }

    /* do a depth first traversal of the request structures to free them */
    if (request_ptr->mm.next_ptr)
	MPID_Request_release(request_ptr->mm.next_ptr);
    mm_request_free(request_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_REQUEST_RELEASE);
}

MPID_Request * MPID_Request_create(void)
{
    return mm_request_alloc();
}

void MPID_Request_set_completed(MPID_Request *req)
{
    *req->cc_ptr = 0;
}
