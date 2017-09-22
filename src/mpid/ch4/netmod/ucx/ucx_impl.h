/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
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
#define MPIDI_UCX_COMM_TO_EP(comm,rank) \
    MPIDI_UCX_AV(MPIDIU_comm_rank_to_av(comm, rank)).dest
#define MPIDI_UCX_AV_TO_EP(av) MPIDI_UCX_AV((av)).dest

#define MPIDI_UCX_WIN(win) ((win)->dev.netmod.ucx)
#define MPIDI_UCX_WIN_INFO(win, rank) MPIDI_UCX_WIN(win).info_table[rank]

static inline uint64_t MPIDI_UCX_init_tag(MPIR_Context_id_t contextid, int source, uint64_t tag)
{
    uint64_t ucp_tag = 0;
    ucp_tag = contextid;
    ucp_tag = (ucp_tag << MPIDI_UCX_SOURCE_SHIFT);
    ucp_tag |= source;
    ucp_tag = (ucp_tag << MPIDI_UCX_TAG_SHIFT);
    ucp_tag |= (MPIDI_UCX_TAG_MASK & tag);
    return ucp_tag;
}

#ifndef MPIR_TAG_ERROR_BIT
#define MPIR_TAG_ERROR_BIT (1 << 30)
#endif
#ifndef  MPIR_TAG_PROC_FAILURE_BIT
#define MPIR_TAG_PROC_FAILURE_BIT (1 << 29)
#endif

static inline uint64_t MPIDI_UCX_tag_mask(int mpi_tag, int src)
{
    uint64_t tag_mask;
    tag_mask = ~(MPIR_TAG_PROC_FAILURE_BIT | MPIR_TAG_ERROR_BIT);
    if (mpi_tag == MPI_ANY_TAG)
        tag_mask &= ~MPIDI_UCX_TAG_MASK;

    if (src == MPI_ANY_SOURCE)
        tag_mask &= ~(MPIDI_UCX_SOURCE_MASK);

    return tag_mask;
}

static inline uint64_t MPIDI_UCX_recv_tag(int mpi_tag, int src, MPIR_Context_id_t contextid)
{
    uint64_t ucp_tag = contextid;

    ucp_tag = (ucp_tag << MPIDI_UCX_SOURCE_SHIFT);
    if (src != MPI_ANY_SOURCE)
        ucp_tag |= (src & UCS_MASK(MPIDI_UCX_CONTEXT_RANK_BITS));
    ucp_tag = ucp_tag << MPIDI_UCX_TAG_SHIFT;
    if (mpi_tag != MPI_ANY_TAG)
        ucp_tag |= (MPIDI_UCX_TAG_MASK & mpi_tag);
    return ucp_tag;
}

static inline int MPIDI_UCX_get_tag(uint64_t match_bits)
{
    return ((int) (match_bits & MPIDI_UCX_TAG_MASK));
}

static inline int MPIDI_UCX_get_source(uint64_t match_bits)
{
    return ((int) ((match_bits & MPIDI_UCX_SOURCE_MASK) >> MPIDI_UCX_TAG_SHIFT));
}



#define MPIDI_UCX_CHK_STATUS(STATUS)                \
  do {								\
    MPIR_ERR_CHKANDJUMP4((STATUS!=UCS_OK && STATUS!=UCS_INPROGRESS),\
			  mpi_errno,				\
			  MPI_ERR_OTHER,			\
			  "**ucx_nm_status",                  \
			  "**ucx_nm_status %s %d %s %s",    \
			  __SHORT_FILE__,			\
			  __LINE__,				\
			  FCNAME,				\
			  ucs_status_string(STATUS));		\
    } while (0)


#define MPIDI_UCX_PMI_ERROR(_errno)				\
  do									\
    {									\
       MPIR_ERR_CHKANDJUMP4(_errno!=PMI_SUCCESS,			\
			    mpi_errno,					\
			    MPI_ERR_OTHER,				\
			    "**ucx_nm_pmi_error",			\
			    "**ucx_nm_pmi_error %s %d %s %s",	\
			    __SHORT_FILE__,				\
			    __LINE__,					\
			    FCNAME,					\
			    "pmi_error");					\
    } while (0)

#define MPIDI_UCX_MPI_ERROR(_errno)				     \
  do								     \
    {								     \
      if (unlikely(_errno!=MPI_SUCCESS)) MPIR_ERR_POP(mpi_errno);    \
    } while (0)

#define MPIDI_UCX_STR_ERRCHK(_errno)				\
  do									\
    {									\
       MPIR_ERR_CHKANDJUMP4(_errno!=MPL_STR_SUCCESS,			\
			    mpi_errno,					\
			    MPI_ERR_OTHER,				\
			    "**ucx_nm_str_error",			\
			    "**ucx_nm_str_error %s %d %s %s",		\
			    __SHORT_FILE__,				\
			    __LINE__,					\
			    FCNAME,					\
			    "strng_error");					\
    } while (0)



#define MPIDI_CH4_UCX_REQUEST(_req)				\
  do {									\
   MPIR_ERR_CHKANDJUMP4(UCS_PTR_IS_ERR(_req),				\
			  mpi_errno,					\
			  MPI_ERR_OTHER,				\
			  "**ucx_nm_rq_error",				\
			  "**ucx_nm_rq_error %s %d %s %s",		\
			  __SHORT_FILE__,				\
			  __LINE__,					\
			  FCNAME,					\
			  ucs_status_string(UCS_PTR_STATUS(_req)));	\
  } while (0)

#endif /* UCX_IMPL_H_INCLUDED */
