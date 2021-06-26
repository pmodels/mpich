/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_IMPL_H_INCLUDED
#define UCX_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include "ucx_types.h"
#include "mpidch4r.h"
#include "ch4_impl.h"

#include <ucs/type/status.h>

#define MPIDI_UCX_COMM(comm)     ((comm)->dev.ch4.netmod.ucx)
#define MPIDI_UCX_REQ(req)       ((req)->dev.ch4.netmod.ucx)
#define COMM_TO_INDEX(comm,rank) MPIDIU_comm_rank_to_pid(comm, rank, NULL, NULL)
#define MPIDI_UCX_COMM_TO_EP(comm,rank,vci_src,vci_dst) \
    MPIDI_UCX_AV(MPIDIU_comm_rank_to_av(comm, rank)).dest[vci_src][vci_dst]
#define MPIDI_UCX_AV_TO_EP(av,vci_src,vci_dst) MPIDI_UCX_AV((av)).dest[vci_src][vci_dst]

#define MPIDI_UCX_WIN(win) ((win)->dev.netmod.ucx)
#define MPIDI_UCX_WIN_INFO(win, rank) MPIDI_UCX_WIN(win).info_table[rank]

#define MPIDI_UCX_THREAD_CS_ENTER_VCI(vci) \
    do { \
        if (!MPIDI_VCI_IS_EXPLICIT(vci)) { \
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock); \
        } \
    } while (0)

#define MPIDI_UCX_THREAD_CS_EXIT_VCI(vci) \
    do { \
        if (!MPIDI_VCI_IS_EXPLICIT(vci)) { \
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock); \
        } \
    } while (0)

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_UCX_init_tag(MPIR_Context_id_t contextid, int source,
                                                     uint64_t tag)
{
    uint64_t ucp_tag = 0;
    ucp_tag = contextid;
    ucp_tag = (ucp_tag << MPIDI_UCX_RANK_BITS);
    ucp_tag |= source;
    ucp_tag = (ucp_tag << MPIDI_UCX_TAG_BITS);
    ucp_tag |= (MPIDI_UCX_TAG_MASK & tag);
    return ucp_tag;
}

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_UCX_tag_mask(int mpi_tag, int src)
{
    uint64_t tag_mask = 0xffffffffffffffff;
    MPIR_TAG_CLEAR_ERROR_BITS(tag_mask);
    if (mpi_tag == MPI_ANY_TAG)
        tag_mask &= ~MPIR_TAG_USABLE_BITS;

    if (src == MPI_ANY_SOURCE)
        tag_mask &= ~(MPIDI_UCX_RANK_MASK);

    return tag_mask;
}

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_UCX_recv_tag(int mpi_tag, int src,
                                                     MPIR_Context_id_t contextid)
{
    uint64_t ucp_tag = contextid;

    ucp_tag = (ucp_tag << MPIDI_UCX_RANK_BITS);
    if (src != MPI_ANY_SOURCE)
        ucp_tag |= (src & UCS_MASK(MPIDI_UCX_RANK_BITS));
    ucp_tag = ucp_tag << MPIDI_UCX_TAG_BITS;
    if (mpi_tag != MPI_ANY_TAG)
        ucp_tag |= (mpi_tag & UCS_MASK(MPIDI_UCX_TAG_BITS));
    return ucp_tag;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_get_tag(uint64_t match_bits)
{
    return ((int) (match_bits & MPIDI_UCX_TAG_MASK));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_get_source(uint64_t match_bits)
{
    return ((int) ((match_bits & MPIDI_UCX_RANK_MASK) >> MPIDI_UCX_TAG_BITS));
}

#define MPIDI_UCX_CHK_STATUS(STATUS)                                    \
    do {                                                                \
        MPIR_ERR_CHKANDJUMP4((STATUS!=UCS_OK && STATUS!=UCS_INPROGRESS), \
                             mpi_errno,                                 \
                             MPI_ERR_OTHER,                             \
                             "**ucx_nm_status",                         \
                             "**ucx_nm_status %s %d %s %s",             \
                             __SHORT_FILE__,                            \
                             __LINE__,                                  \
                             __func__,                                    \
                             ucs_status_string(STATUS));                \
    } while (0)

#define MPIDI_UCX_CHK_REQUEST(_req)                                     \
    do {                                                                \
        MPIR_ERR_CHKANDJUMP4(UCS_PTR_IS_ERR(_req),                      \
                             mpi_errno,                                 \
                             MPI_ERR_OTHER,                             \
                             "**ucx_nm_rq_error",                       \
                             "**ucx_nm_rq_error %s %d %s %s",           \
                             __SHORT_FILE__,                            \
                             __LINE__,                                  \
                             __func__,                                    \
                             ucs_status_string(UCS_PTR_STATUS(_req)));  \
    } while (0)

MPL_STATIC_INLINE_PREFIX bool MPIDI_UCX_is_reachable_target(int rank, MPIR_Win * win,
                                                            MPIDI_winattr_t winattr)
{
    /* unmapped win target does not have rkey. */
    return (winattr & MPIDI_WINATTR_NM_REACHABLE) || (MPIDI_UCX_WIN(win).info_table &&
                                                      MPIDI_UCX_WIN_INFO(win, rank).rkey != NULL);
}

/* Need both local and remote vci to be the same, or the synchronization call
 * may blocked at flushing the remote ep (due to missing remote progress) */
#define MPIDI_UCX_WIN_TO_EP(win,rank,vci,vci_target) \
    MPIDI_UCX_AV(MPIDIU_comm_rank_to_av(win->comm_ptr, rank)).dest[vci][vci_target]

#define MPIDI_UCX_WIN_AV_TO_EP(av, vci, vci_target) MPIDI_UCX_AV((av)).dest[vci][vci_target]

/* am handler for message sent by ucp_am_send_nb */
ucs_status_t MPIDI_UCX_am_handler(void *arg, void *data, size_t length, ucp_ep_h reply_ep,
                                  unsigned flags);
/* callback for ucp_am_send_nb, used in MPIDI_NM_am_isend */
void MPIDI_UCX_am_isend_callback(void *request, ucs_status_t status);
/* callback for ucp_am_send_nb, used in MPIDI_NM_am_send_hdr */
void MPIDI_UCX_am_send_callback(void *request, ucs_status_t status);

#endif /* UCX_IMPL_H_INCLUDED */
