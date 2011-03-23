/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de           
 * Bordeaux. All rights reserved. Permission is hereby granted to use,          
 * reproduce, prepare derivative works, and to redistribute to others.          
 */



#include "mx_impl.h"
#include "my_papi_defs.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_probe
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_probe(MPIDI_VC_t *vc,  int source, int tag, MPID_Comm *comm, int context_offset, MPI_Status *status)
{
    uint64_t match_info = NEM_MX_MATCH_DIRECT;
    uint64_t match_mask = NEM_MX_MATCH_FULL_MASK;
    int      mpi_errno = MPI_SUCCESS;
    mx_return_t ret;
    mx_status_t mx_status;
    uint32_t result;

    NEM_MX_DIRECT_MATCH(match_info,0,source,comm->context_id + context_offset);
    if (tag == MPI_ANY_TAG)
    {
	NEM_MX_SET_ANYTAG(match_info);
	NEM_MX_SET_ANYTAG(match_mask);
    }
    else
        NEM_MX_SET_TAG(match_info,tag);

    
    ret = mx_probe(MPID_nem_mx_local_endpoint,MX_INFINITE,match_info,match_mask,&mx_status,&result);
    MPIU_Assert(ret == MX_SUCCESS);
    MPIU_Assert(result != 0);
    
    NEM_MX_MATCH_GET_RANK(mx_status.match_info,status->MPI_SOURCE);
    NEM_MX_MATCH_GET_TAG(mx_status.match_info,status->MPI_TAG);
    status->count = mx_status.xfer_length;
    
 fn_exit:
    return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_iprobe
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_iprobe(MPIDI_VC_t *vc,  int source, int tag, MPID_Comm *comm, int context_offset, int *flag, MPI_Status *status)
{
    uint64_t match_info = NEM_MX_MATCH_DIRECT;
    uint64_t match_mask = NEM_MX_MATCH_FULL_MASK;
    int      mpi_errno = MPI_SUCCESS;
    mx_return_t ret;
    mx_status_t mx_status;
    uint32_t result;

    NEM_MX_SET_CTXT(match_info,comm->context_id + context_offset);
    if( source  == MPI_ANY_SOURCE)
    {
	NEM_MX_SET_ANYSRC(match_info);
	NEM_MX_SET_ANYSRC(match_mask);	
    }
    else
	NEM_MX_SET_SRC(match_info,source);	
    if (tag == MPI_ANY_TAG)
    {
	NEM_MX_SET_ANYTAG(match_info);
	NEM_MX_SET_ANYTAG(match_mask);
    }
    else
        NEM_MX_SET_TAG(match_info,tag);
    
    ret = mx_iprobe(MPID_nem_mx_local_endpoint,match_info,match_mask,&mx_status,&result);
    MPIU_Assert(ret == MX_SUCCESS);

    if (result != 0)
    {    
	NEM_MX_MATCH_GET_RANK(mx_status.match_info,status->MPI_SOURCE);
	NEM_MX_MATCH_GET_TAG(mx_status.match_info,status->MPI_TAG);
	status->count = mx_status.xfer_length;	
	*flag = TRUE;	
    }
    else
	*flag = FALSE;
	
 fn_exit:
    return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
    goto fn_exit;
}




#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_anysource_iprobe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_mx_anysource_iprobe(int tag, MPID_Comm *comm, int context_offset, int *flag, MPI_Status *status)
{
    return MPID_nem_mx_iprobe(NULL, MPI_ANY_SOURCE, tag, comm, context_offset, flag, status);
}
