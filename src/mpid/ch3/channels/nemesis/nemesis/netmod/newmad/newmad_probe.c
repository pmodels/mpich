/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
    int mpi_errno = MPI_SUCCESS;
    int ret;

    NEM_NMAD_SET_CTXT(match_info,comm->context_id + context_offset);
    if( source  == MPI_ANY_SOURCE)
    {
       NEM_NMAD_SET_ANYSRC(match_info);
       NEM_NMAD_SET_ANYSRC(match_mask);
    }
   else
     NEM_NMAD_SET_SRC(match_info,source);
 
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
	ret = nm_sr_probe(mpid_nem_newmad_session,VC_FIELD(vc,p_gate),&out_gate,match_info,match_mask);
    }
    while (ret != NM_ESUCCESS);

    status->MPI_SOURCE = source;
    status->MPI_TAG = tag;
    status->count = 0; /* FIXME */

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
    int mpi_errno = MPI_SUCCESS;
    int ret;

    NEM_NMAD_SET_CTXT(match_info,comm->context_id + context_offset);
    if( source  == MPI_ANY_SOURCE)
    {
       NEM_NMAD_SET_ANYSRC(match_info);
       NEM_NMAD_SET_ANYSRC(match_mask);
    }
   else
     NEM_NMAD_SET_SRC(match_info,source);
   
    if (tag != MPI_ANY_TAG)
    {
        NEM_NMAD_SET_TAG(match_info,tag);
    }
    else
    {
       NEM_NMAD_SET_ANYTAG(match_info);
       NEM_NMAD_SET_ANYTAG(match_mask);
    }

    ret = nm_sr_probe(mpid_nem_newmad_session,VC_FIELD(vc,p_gate),&out_gate,match_info,match_mask);
    if (ret == NM_ESUCCESS)
    {   
	/*
	size_t size;
	nm_sr_get_size(mpid_nem_newmad_session, p_out_req, &size);
	*/

	status->MPI_SOURCE = source;
	status->MPI_TAG = tag;
	status->count = 0; /* FIXME */
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

