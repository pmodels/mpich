/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 NEC Corporation
 *      Author: Masamichi Takagi
 *      See COPYRIGHT in top-level directory.
 */

#include "ib_impl.h"

//#define MPID_NEM_IB_DEBUG_FINALIZE
#ifdef dprintf  /* avoid redefinition with src/mpid/ch3/include/mpidimpl.h */
#undef dprintf
#endif
#ifdef MPID_NEM_IB_DEBUG_FINALIZE
#define dprintf printf
#else
#define dprintf(...)
#endif

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
#if 0
    int ibcom_errno;
    int i;
#endif

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_FINALIZE);

#if 0
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        ibcom_errno = MPID_nem_ib_com_close(MPID_nem_ib_conns[i].fd);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_close");

    }
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_FINALIZE);

  fn_exit:
    return mpi_errno;
    //fn_fail:
    goto fn_exit;
}
