/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"



#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_lmt_initiate_lmt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_lmt_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *rts_pkt, MPID_Request *req)
{
    /* Nothing to do here, but has to be defined for CH3 to follow the right path to
       MPID_nem_ptl_lmt_start_recv */
    return MPI_SUCCESS;
}



/* The following function is implemented in ptl_recv.c to make use of the handlers defined there */
/* int MPID_nem_ptl_lmt_start_recv(MPIDI_VC_t *vc,  MPID_Request *rreq, MPL_IOV s_cookie) */



#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_lmt_start_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_lmt_start_send(MPIDI_VC_t *vc, MPID_Request *sreq, MPL_IOV r_cookie)
{
    MPIU_Assertp(0 && "This function shouldn't be called.");
    return MPI_ERR_INTERN;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_lmt_handle_cookie
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_lmt_handle_cookie(MPIDI_VC_t *vc, MPID_Request *req, MPL_IOV s_cookie)
{
    MPIU_Assertp(0 && "This function shouldn't be called.");
    return MPI_ERR_INTERN;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_lmt_done_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_lmt_done_send(MPIDI_VC_t *vc, MPID_Request *req)
{
    MPIU_Assertp(0 && "This function shouldn't be called.");
    return MPI_ERR_INTERN;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_lmt_done_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_lmt_done_recv(MPIDI_VC_t *vc, MPID_Request *req)
{
    MPIU_Assertp(0 && "This function shouldn't be called.");
    return MPI_ERR_INTERN;
}
