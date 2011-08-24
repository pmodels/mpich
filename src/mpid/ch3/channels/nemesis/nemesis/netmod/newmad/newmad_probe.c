/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de
 * Bordeaux. All rights reserved. Permission is hereby granted to use,
 * reproduce, prepare derivative works, and to redistribute to others.
 */

#include "newmad_impl.h"
#include "my_papi_defs.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_probe
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_probe(MPIDI_VC_t *vc,  int source, int tag, MPID_Comm *comm, int context_offset, MPI_Status *status)
{
    nm_tag_t  match_info = 0;
    nm_tag_t  match_mask = NEM_NMAD_MATCH_FULL_MASK;
    nm_gate_t out_gate;
    nm_gate_t in_gate;
    nm_tag_t  out_tag;
    int size;   
    int mpi_errno = MPI_SUCCESS;
    int ret;

    NEM_NMAD_SET_CTXT(match_info,comm->context_id + context_offset);
    if( source  == MPI_ANY_SOURCE)
    {
       NEM_NMAD_SET_ANYSRC(match_info);
       NEM_NMAD_SET_ANYSRC(match_mask);
       in_gate = NM_ANY_GATE;
    }
    else
    { 
	NEM_NMAD_SET_SRC(match_info,source);
	in_gate = VC_FIELD(vc,p_gate);
    }
   
    if (tag != MPI_ANY_TAG)
    {
        NEM_NMAD_SET_TAG(match_info,tag);
    }
    else
    {
       NEM_NMAD_SET_ANYTAG(match_info);
       NEM_NMAD_SET_ANYTAG(match_mask);
    }

    do {
	ret = nm_sr_probe(mpid_nem_newmad_session,in_gate,&out_gate,
			  match_info,match_mask,&out_tag,&size);
    }
    while (ret != NM_ESUCCESS);

   if (source != MPI_ANY_SOURCE)
     status->MPI_SOURCE = source;
   else 
     {	
	MPIDI_VC_t *vc;
	int         index;
	vc = (MPIDI_VC_t *)nm_gate_ref_get(out_gate);
	for(index = 0 ; index < comm->local_size ; index ++)
	  if (vc == comm->vcr[index])
	    break;
	status->MPI_SOURCE = index;
     }
   
   if (tag != MPI_ANY_TAG)
     status->MPI_TAG = tag;
   else
     NEM_NMAD_MATCH_GET_TAG(out_tag,status->MPI_TAG);
   
   status->count = size;
   
 fn_exit:
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_iprobe
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_iprobe(MPIDI_VC_t *vc,  int source, int tag, MPID_Comm *comm, int context_offset, int *flag, MPI_Status *status)
{
    nm_tag_t  match_info = 0;
    nm_tag_t  match_mask = NEM_NMAD_MATCH_FULL_MASK;
    nm_gate_t out_gate;
    nm_gate_t in_gate;
    nm_tag_t out_tag;
    int size;
    int mpi_errno = MPI_SUCCESS;
    int ret;

    NEM_NMAD_SET_CTXT(match_info,comm->context_id + context_offset);
    if( source  == MPI_ANY_SOURCE)
    {
       NEM_NMAD_SET_ANYSRC(match_info);
       NEM_NMAD_SET_ANYSRC(match_mask);
       in_gate = NM_ANY_GATE;
    }
    else
    {
	NEM_NMAD_SET_SRC(match_info,source);
	in_gate = VC_FIELD(vc,p_gate);
    }
   
    if (tag != MPI_ANY_TAG)
    {
        NEM_NMAD_SET_TAG(match_info,tag);
    }
    else
    {
       NEM_NMAD_SET_ANYTAG(match_info);
       NEM_NMAD_SET_ANYTAG(match_mask);
    }

    ret = nm_sr_probe(mpid_nem_newmad_session,in_gate,&out_gate,
		      match_info,match_mask,&out_tag,&size);
    if (ret == NM_ESUCCESS)
    {   
       if (source != MPI_ANY_SOURCE)
	 status->MPI_SOURCE = source;
       else 
	 {	    
	    MPIDI_VC_t *vc;
	    int         index;
	    vc = (MPIDI_VC_t *)nm_gate_ref_get(out_gate);
	    for(index = 0 ; index < comm->local_size ; index ++)
	      if (vc == comm->vcr[index])
		break;
	    status->MPI_SOURCE = index;
	 }
       
       if (tag != MPI_ANY_TAG)
	 status->MPI_TAG = tag;
       else
	 NEM_NMAD_MATCH_GET_TAG(out_tag,status->MPI_TAG);  
       
       status->count = size;
       *flag = TRUE;
    }
    else
     *flag = FALSE;
 
 fn_exit:
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_anysource_iprobe
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_newmad_anysource_iprobe(int tag, MPID_Comm *comm, int context_offset, int *flag, MPI_Status *status)
{   
    return MPID_nem_newmad_iprobe(NULL, MPI_ANY_SOURCE, tag, comm, context_offset, flag, status);
}

