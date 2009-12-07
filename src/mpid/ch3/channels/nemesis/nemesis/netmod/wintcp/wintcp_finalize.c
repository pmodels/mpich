/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "wintcp_impl.h"

extern sockconn_t MPID_nem_newtcp_module_g_lstn_sc;
/* FIXME: Move all externs to say socksm_globals.h */
extern MPIU_ExSetHandle_t MPID_nem_newtcp_module_ex_set_hnd;

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    
    /* FIXME: Why don't we have a finalize for sm - MPID_nem_newtcp_module_finalize_sm() - ? */
    /* FIXME: Shouldn't the order of finalize() be the reverse order of init() ? 
     * i.e., *finalize_sm(); *poll_finalize(); *send_finalize();
     */
    mpi_errno = MPID_nem_newtcp_module_send_finalize();
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    mpi_errno = MPID_nem_newtcp_module_poll_finalize();
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    
    mpi_errno =  MPID_nem_newtcp_module_sm_finalize();
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
     
    if(MPIU_SOCKW_Sockfd_is_valid(MPID_nem_newtcp_module_g_lstn_sc.fd))
    {
        /* Since the listen sc is global we don't need to post a close and
         * invoke the EX handlers
         */
        MPIU_OSW_RETRYON_INTR((mpi_errno != MPI_SUCCESS), (mpi_errno = MPIU_SOCKW_Sock_close(MPID_nem_newtcp_module_g_lstn_sc.fd)));
        if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }

    /* Close the newtcp module executive set */
    MPIU_ExCloseSet(MPID_nem_newtcp_module_ex_set_hnd);

    mpi_errno = MPIU_ExFinalize();
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    mpi_errno = MPIU_SOCKW_Finalize();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
       
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_ckpt_shutdown()
{
    return MPID_nem_newtcp_module_finalize();
}

