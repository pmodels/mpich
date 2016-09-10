/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpir_nsched.h"

int MPIR_Nsched_create(MPIR_Nsched ** s, struct MPIR_Request **req)
{
    *s = (MPIR_Nsched *) MPL_malloc(sizeof(struct MPIR_Nsched));

    (*s)->elm_list_head = NULL;
    (*s)->elm_list_tail = NULL;
    (*s)->req = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    MPIR_Request_add_ref(((*s)->req));
    (*s)->next = NULL;

    *req = (*s)->req;

    return MPI_SUCCESS;
}

int MPIR_Nsched_enqueue_sync(MPIR_Nsched * s)
{
    MPII_Nsched_elm *e;

    e = (MPII_Nsched_elm *) MPL_malloc(sizeof(MPII_Nsched_elm));
    e->type = MPII_NSCHED_ELM_TYPE__SYNC;
    e->issued = 0;
    e->req = NULL;
    e->next = NULL;

    MPII_NSCHED_ELM_ENQUEUE(s, e);

    return MPI_SUCCESS;
}

int MPIR_Nsched_enqueue_isend(const void *buf, int count, MPI_Datatype datatype,
                              int dest, int tag, MPIR_Comm * comm_ptr, MPIR_Nsched * s)
{
    MPII_Nsched_elm *e;

    e = (MPII_Nsched_elm *) MPL_malloc(sizeof(MPII_Nsched_elm));
    e->type = MPII_NSCHED_ELM_TYPE__ISEND;
    e->u.isend.buf = buf;
    e->u.isend.count = count;
    e->u.isend.datatype = datatype;
    e->u.isend.dest = dest;
    e->u.isend.tag = tag;
    e->u.isend.comm_ptr = comm_ptr;

    e->issued = 0;
    e->req = NULL;
    e->next = NULL;

    MPII_NSCHED_ELM_ENQUEUE(s, e);

    return MPI_SUCCESS;
}

int MPIR_Nsched_enqueue_irecv(void *buf, int count, MPI_Datatype datatype,
                              int dest, int tag, MPIR_Comm * comm_ptr, MPIR_Nsched * s)
{
    MPII_Nsched_elm *e;

    e = (MPII_Nsched_elm *) MPL_malloc(sizeof(MPII_Nsched_elm));
    e->type = MPII_NSCHED_ELM_TYPE__IRECV;
    e->u.irecv.buf = buf;
    e->u.irecv.count = count;
    e->u.irecv.datatype = datatype;
    e->u.irecv.dest = dest;
    e->u.irecv.tag = tag;
    e->u.irecv.comm_ptr = comm_ptr;

    e->issued = 0;
    e->req = NULL;
    e->next = NULL;

    MPII_NSCHED_ELM_ENQUEUE(s, e);

    return MPI_SUCCESS;
}
