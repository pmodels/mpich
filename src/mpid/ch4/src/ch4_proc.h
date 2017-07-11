/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_PROC_H_INCLUDED
#define CH4_PROC_H_INCLUDED

#include "ch4_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4_rank_is_local
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_rank_is_local(int rank, MPIR_Comm * comm)
{
    int ret;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4_RANK_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4_RANK_IS_LOCAL);

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    /* Ask the netmod for locality information. If it decided not to build it,
     * it will call back up to the CH4U function to get the infomration. */
    ret = MPIDI_NM_rank_is_local(rank, comm);
#else
    ret = MPIDI_CH4U_rank_is_local(rank, comm);
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4_RANK_IS_LOCAL);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_av_is_local
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_av_is_local(MPIDI_av_entry_t *av)
{
   int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4_AV_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4_AV_IS_LOCAL);

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    /* Ask the netmod for locality information. If it decided not to build it,
     * it will call back up to the CH4U function to get the infomration. */
    ret = MPIDI_NM_av_is_local(av);
#else
    ret = MPIDI_CH4U_av_is_local(av);
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4_AV_IS_LOCAL);
    return ret;
}

#endif /* CH4_PROC_H_INCLUDED */
