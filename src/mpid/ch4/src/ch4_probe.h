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
#ifndef CH4_PROBE_H_INCLUDED
#define CH4_PROBE_H_INCLUDED

#include "ch4_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Probe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Probe(int source,
                                         int tag, MPIR_Comm * comm, int context_offset,
                                         MPI_Status * status)
{
    int mpi_errno, flag = 0;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROBE);

    if (unlikely(source == MPI_PROC_NULL)) {
        MPIR_Status_set_procnull(status);
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(comm, source);
    while (!flag) {
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
        mpi_errno = MPIDI_NM_mpi_iprobe(source, tag, comm, context_offset, av, &flag, status);
#else
        if (unlikely(source == MPI_ANY_SOURCE)) {
            mpi_errno = MPIDI_SHM_mpi_iprobe(source, tag, comm, context_offset, &flag, status);
            if (!flag)
                mpi_errno = MPIDI_NM_mpi_iprobe(source, tag, comm, context_offset, av, &flag, status);
        }
        else if (MPIDI_av_is_local(av))
            mpi_errno = MPIDI_SHM_mpi_iprobe(source, tag, comm, context_offset, &flag, status);
        else
            mpi_errno = MPIDI_NM_mpi_iprobe(source, tag, comm, context_offset, av, &flag, status);
#endif
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
        MPID_Progress_test();
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Mprobe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Mprobe(int source,
                                          int tag,
                                          MPIR_Comm * comm,
                                          int context_offset, MPIR_Request ** message,
                                          MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS, flag = 0;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_MPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_MPROBE);

    if (source == MPI_PROC_NULL) {
        MPIR_Status_set_procnull(status);
        *message = NULL;        /* should be interpreted as MPI_MESSAGE_NO_PROC */
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(comm, source);
    while (!flag) {
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
        mpi_errno = MPIDI_NM_mpi_improbe(source, tag, comm, context_offset, av, &flag, message, status);
#else
        if (unlikely(source == MPI_ANY_SOURCE)) {
            mpi_errno =
                MPIDI_SHM_mpi_improbe(source, tag, comm, context_offset, &flag, message, status);
            if (flag) {
                MPIDI_CH4I_REQUEST(*message, is_local) = 1;
            } else {
                mpi_errno =
                    MPIDI_NM_mpi_improbe(source, tag, comm, context_offset, av, &flag, message, status);
                if (flag)
                    MPIDI_CH4I_REQUEST(*message, is_local) = 0;
            }
        }
        else if (MPIDI_av_is_local(av)) {
            mpi_errno =
                MPIDI_SHM_mpi_improbe(source, tag, comm, context_offset, &flag, message, status);
            if (flag)
                MPIDI_CH4I_REQUEST(*message, is_local) = 1;
        }
        else {
            mpi_errno =
                MPIDI_NM_mpi_improbe(source, tag, comm, context_offset, av, &flag, message, status);
            if (flag)
                MPIDI_CH4I_REQUEST(*message, is_local) = 0;
        }
#endif
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
        MPID_Progress_test();
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_MPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Improbe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Improbe(int source,
                                           int tag,
                                           MPIR_Comm * comm,
                                           int context_offset,
                                           int *flag, MPIR_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IMPROBE);

    if (source == MPI_PROC_NULL) {
        MPIR_Status_set_procnull(status);
        *flag = 1;
        *message = NULL;        /* should be interpreted as MPI_MESSAGE_NO_PROC */
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(comm, source);
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_improbe(source, tag, comm, context_offset, av, flag, message, status);
#else
    if (unlikely(source == MPI_ANY_SOURCE)) {
        mpi_errno = MPIDI_SHM_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
        if (*flag) {
            MPIDI_CH4I_REQUEST(*message, is_local) = 1;
        } else {
            mpi_errno =
                MPIDI_NM_mpi_improbe(source, tag, comm, context_offset, av, flag, message, status);
            if (*flag) {
                MPIDI_CH4I_REQUEST(*message, is_local) = 0;
            }
        }
    }
    else if (MPIDI_av_is_local(av)) {
        mpi_errno = MPIDI_SHM_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
        if (*flag)
            MPIDI_CH4I_REQUEST(*message, is_local) = 1;
    }
    else {
        mpi_errno = MPIDI_NM_mpi_improbe(source, tag, comm, context_offset, av, flag, message, status);
        if (*flag)
            MPIDI_CH4I_REQUEST(*message, is_local) = 0;
    }
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
    if (!*flag)
        MPID_Progress_test();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IMPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Iprobe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Iprobe(int source,
                                          int tag,
                                          MPIR_Comm * comm,
                                          int context_offset, int *flag, MPI_Status * status)
{

    int mpi_errno;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IPROBE);

    if (unlikely(source == MPI_PROC_NULL)) {
        MPIR_Status_set_procnull(status);
        *flag = 1;
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(comm, source);
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_iprobe(source, tag, comm, context_offset, av, flag, status);
#else
    if (unlikely(source == MPI_ANY_SOURCE)) {
        mpi_errno = MPIDI_SHM_mpi_iprobe(source, tag, comm, context_offset, flag, status);
        if (!*flag)
            mpi_errno = MPIDI_NM_mpi_iprobe(source, tag, comm, context_offset, av, flag, status);
    }
    else if (MPIDI_av_is_local(av))
        mpi_errno = MPIDI_SHM_mpi_iprobe(source, tag, comm, context_offset, flag, status);
    else
        mpi_errno = MPIDI_NM_mpi_iprobe(source, tag, comm, context_offset, av, flag, status);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
    if (!*flag)
        MPID_Progress_test();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_PROBE_H_INCLUDED */
