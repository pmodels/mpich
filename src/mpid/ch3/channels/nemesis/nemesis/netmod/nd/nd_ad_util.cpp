/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "nd_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_ad_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_Nem_nd_ad_init(MPID_Nem_nd_dev_hnd_t hnd, MPIU_ExSetHandle_t ex_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    WSADATA wsa_data;
    SIZE_T cq_sz;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_AD_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_AD_INIT);

    MPIU_Assert(MPID_NEM_ND_DEV_HND_IS_INIT(hnd));

    /* Initialize WinSock dll */
    if(WSAStartup(MAKEWORD(2,2), &wsa_data) != 0){
        int err = MPIU_OSW_Get_errno();
        MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER,
            "**wsastartup", "**wsastartup %s %d",
            MPIU_OSW_Strerror(err), err);
    }

    /* Initialize ND provider */
    mpi_errno = MPID_Nem_nd_provider_hnd_init();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Open the adapter */
    mpi_errno = MPID_Nem_nd_open_ad(hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Create a ND completion queue */
    /* FIXME: We currently support only a fixed number of connections */
    cq_sz = MPID_NEM_ND_CONN_NUM_MAX * (hnd->ad_info.MaxInboundRequests + hnd->ad_info.MaxOutboundRequests);
    cq_sz = min(hnd->ad_info.MaxCqEntries, cq_sz);
    hr = hnd->p_ad->CreateCompletionQueue(cq_sz, &(hnd->p_cq));
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_ad_open", "**nd_ad_open %s %d",
        _com_error(hr).ErrorMessage(), hr);

    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "Successfully created an ND CQ (sz=%d)", cq_sz);
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST,
        "ND CQ : size = " MPIR_UPINT_FMT_DEC_SPEC ", mcq = " MPIR_UPINT_FMT_DEC_SPEC
        ", mir = " MPIR_UPINT_FMT_DEC_SPEC ", mor = " MPIR_UPINT_FMT_DEC_SPEC
        ", mirl = " MPIR_UPINT_FMT_DEC_SPEC ", morl = " MPIR_UPINT_FMT_DEC_SPEC
        ", mol = " MPIR_UPINT_FMT_DEC_SPEC ", mreg_sz = " MPIR_UPINT_FMT_DEC_SPEC
        ", lreq_thres = " MPIR_UPINT_FMT_DEC_SPEC,
        cq_sz, hnd->ad_info.MaxCqEntries,
        hnd->ad_info.MaxInboundRequests, hnd->ad_info.MaxOutboundRequests,
        hnd->ad_info.MaxInboundReadLimit, hnd->ad_info.MaxOutboundReadLimit,
        hnd->ad_info.MaxOutboundLength, hnd->ad_info.MaxRegistrationSize,
        hnd->ad_info.LargeRequestThreshold));

    /* Associate the adapter with the Executive */
    MPIU_ExAttachHandle(ex_hnd, MPIU_EX_GENERIC_COMP_PROC_KEY, hnd->p_ad->GetFileHandle());

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_AD_INIT);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_ad_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_Nem_nd_ad_finalize(MPID_Nem_nd_dev_hnd_t hnd)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_AD_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_AD_FINALIZE);

    MPIU_Assert(MPID_NEM_ND_DEV_HND_IS_INIT(hnd));
    /* We don't bail out on error in finalize*/
    if(hnd->p_listen){
        /* Listen finalize */
        hnd->p_listen->Release();
    }
    if(hnd->p_cq){
        /* Completion queue finalize */
        hnd->p_cq->Release();
    }
    if(hnd->p_ad){    
        /* Adapter finalize */
        hnd->p_ad->Release();
    }

    MPID_Nem_nd_provider_hnd_finalize();

    /* Cleanup winsock dll */
    WSACleanup();

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_AD_FINALIZE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_dev_hnd_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_dev_hnd_init(MPID_Nem_nd_dev_hnd_t *phnd, MPIU_ExSetHandle_t ex_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_DEV_HND_INIT);
    MPIU_CHKPMEM_DECL(1);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_DEV_HND_INIT);

    MPIU_Assert(phnd != NULL);

    /* Allocate memory for dev handle */
    MPIU_CHKPMEM_MALLOC((*phnd), MPID_Nem_nd_dev_hnd_t , sizeof(MPID_Nem_nd_dev_hnd_), mpi_errno, "ND Dev handle");

    /* Initialize adapter */
    mpi_errno = MPID_Nem_nd_ad_init(*phnd, ex_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    (*phnd)->npending_rds = 0;
    (*phnd)->zcp_pending = 0;

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_DEV_HND_INIT);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_dev_hnd_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_dev_hnd_finalize(MPID_Nem_nd_dev_hnd_t *phnd)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_DEV_HND_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_DEV_HND_FINALIZE);

    MPIU_Assert(phnd != NULL);
    if(*phnd){
        MPID_Nem_nd_ad_finalize(*phnd);
        MPIU_Free(*phnd);
    }

    *phnd = MPID_NEM_ND_DEV_HND_INVALID;
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_DEV_HND_FINALIZE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}
