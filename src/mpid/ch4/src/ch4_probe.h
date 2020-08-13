/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_PROBE_H_INCLUDED
#define CH4_PROBE_H_INCLUDED

#include "ch4r_proc.h"
#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_iprobe(int source,
                                          int tag, MPIR_Comm * comm, int context_offset,
                                          MPIDI_av_entry_t * av, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPROBE);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    MPIDI_workq_vci_progress_unsafe();
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
#endif

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_iprobe(source, tag, comm, context_offset, av, flag, status);
#else
    if (unlikely(source == MPI_ANY_SOURCE)) {
        mpi_errno = MPIDI_SHM_mpi_iprobe(source, tag, comm, context_offset, flag, status);
        MPIR_ERR_CHECK(mpi_errno);
        if (!*flag)
            mpi_errno = MPIDI_NM_mpi_iprobe(source, tag, comm, context_offset, av, flag, status);
    } else if (MPIDI_rank_is_local(source, comm)) {
        mpi_errno = MPIDI_SHM_mpi_iprobe(source, tag, comm, context_offset, flag, status);
    } else {
        mpi_errno = MPIDI_NM_mpi_iprobe(source, tag, comm, context_offset, av, flag, status);
    }
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPROBE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_improbe(int source,
                                           int tag, MPIR_Comm * comm,
                                           int context_offset,
                                           MPIDI_av_entry_t * av,
                                           int *flag, MPIR_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IMPROBE);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    MPIDI_workq_vci_progress_unsafe();
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
#endif

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_improbe(source, tag, comm, context_offset, av, flag, message, status);
    MPIR_ERR_CHECK(mpi_errno);
#else
    if (unlikely(source == MPI_ANY_SOURCE)) {
        mpi_errno = MPIDI_SHM_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
        MPIR_ERR_CHECK(mpi_errno);
        if (*flag) {
            MPIDI_REQUEST(*message, is_local) = 1;
        } else {
            mpi_errno =
                MPIDI_NM_mpi_improbe(source, tag, comm, context_offset, av, flag, message, status);
            MPIR_ERR_CHECK(mpi_errno);
            if (*flag) {
                MPIDI_REQUEST(*message, is_local) = 0;
            }
        }
    } else if (MPIDI_av_is_local(av)) {
        mpi_errno = MPIDI_SHM_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
        MPIR_ERR_CHECK(mpi_errno);
        if (*flag)
            MPIDI_REQUEST(*message, is_local) = 1;
    } else {
        mpi_errno =
            MPIDI_NM_mpi_improbe(source, tag, comm, context_offset, av, flag, message, status);
        MPIR_ERR_CHECK(mpi_errno);
        if (*flag)
            MPIDI_REQUEST(*message, is_local) = 0;
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IMPROBE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Probe(int source,
                                        int tag, MPIR_Comm * comm, int context_offset,
                                        MPI_Status * status)
{
    int mpi_errno, flag = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_PROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_PROBE);

    MPIDI_av_entry_t *av = (source == MPI_ANY_SOURCE ? NULL : MPIDIU_comm_rank_to_av(comm, source));
    while (!flag) {
        mpi_errno = MPIDI_iprobe(source, tag, comm, context_offset, av, &flag, status);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPID_Progress_test(NULL);
        MPIR_ERR_CHECK(mpi_errno);
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}



MPL_STATIC_INLINE_PREFIX int MPID_Mprobe(int source,
                                         int tag,
                                         MPIR_Comm * comm,
                                         int context_offset, MPIR_Request ** message,
                                         MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS, flag = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_MPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_MPROBE);

    MPIDI_av_entry_t *av = (source == MPI_ANY_SOURCE ? NULL : MPIDIU_comm_rank_to_av(comm, source));
    while (!flag) {
        mpi_errno = MPIDI_improbe(source, tag, comm, context_offset, av, &flag, message, status);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPID_Progress_test(NULL);
        MPIR_ERR_CHECK(mpi_errno);
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_MPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Improbe(int source,
                                          int tag,
                                          MPIR_Comm * comm,
                                          int context_offset,
                                          int *flag, MPIR_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IMPROBE);

    *flag = 0;
    MPIDI_av_entry_t *av = (source == MPI_ANY_SOURCE ? NULL : MPIDIU_comm_rank_to_av(comm, source));

    mpi_errno = MPIDI_improbe(source, tag, comm, context_offset, av, flag, message, status);
    MPIR_ERR_CHECK(mpi_errno);

    if (!*flag) {
        mpi_errno = MPID_Progress_test(NULL);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IMPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iprobe(int source,
                                         int tag,
                                         MPIR_Comm * comm,
                                         int context_offset, int *flag, MPI_Status * status)
{

    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IPROBE);

    *flag = 0;
    MPIDI_av_entry_t *av = (source == MPI_ANY_SOURCE ? NULL : MPIDIU_comm_rank_to_av(comm, source));

    mpi_errno = MPIDI_iprobe(source, tag, comm, context_offset, av, flag, status);
    MPIR_ERR_CHECK(mpi_errno);

    if (!*flag) {
        mpi_errno = MPID_Progress_test(NULL);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_PROBE_H_INCLUDED */
