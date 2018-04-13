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
#define FUNCNAME MPIDI_iprobe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_iprobe(int transport,
                                          int source,
                                          int tag, MPIR_Comm * comm, int context_offset,
                                          MPIDI_av_entry_t * av, MPI_Status * status, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPROBE);

    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);

        /* Handoff branch: under the handoff model, this(main) thread sets the flag
         * 'processed' to false and submits an iprobe request to the handoff queue.
         * It then waits until the progress thread processes the request and sets
         * the flag 'processed' to true. */
        if (!cs_acq) {
            /* Submit iprobe request */
            MPIR_Request *req;
            OPA_int_t processed;
            /* Allocate request for enqueuing the task to the work queue, as pt2pt
             * enqueue function requires a request object which includes the command
             * structure.
             * This request is merely for enqueing, won't be used in netmod. */
            req = MPIR_Request_create(MPIR_REQUEST_KIND__MPROBE);
            MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
            OPA_store_int(&processed, 0);
            MPIDI_workq_pt2pt_enqueue(MPIDI_comm_vni_to_workq(comm, vni_idx),
                                      IPROBE, NULL /*send_buf */ , NULL /*buf */ , 0 /* count */ ,
                                      -1 /* datatype */ ,
                                      source, tag, comm, context_offset, av,
                                      status, req, flag, NULL /*message */ ,
                                      &processed);
            /* Busy loop to block until iprobe req is processed by the progress thread */
            while (!OPA_load_int(&processed));
            /* FIXME: alternatively may use yield, e.g., MPL_thread_yield if problem arises when
             * sharing hardware thread between the main/this thread and the progress thread */
            MPIR_Request_free(req);     /* Now it's safe to free the request */
            mpi_errno = MPI_SUCCESS;
        } else {        /* Non handoff branch */
            mpi_errno = MPIDI_NM_mpi_iprobe(source, tag, comm, context_offset, av, flag, status);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPROBE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

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
    /* Keep re-submitting iprobe query until a match is found */
    while (!flag) {
#ifdef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_iprobe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status, &flag);
#else
        if (unlikely(source == MPI_ANY_SOURCE)) {
            mpi_errno = MPIDI_SHM_mpi_iprobe(source, tag, comm, context_offset, &flag, status);
            if (!flag)
                mpi_errno =
                    MPIDI_iprobe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status,
                                 &flag);
        } else if (MPIDI_CH4_rank_is_local(source, comm)) {
            mpi_errno = MPIDI_SHM_mpi_iprobe(source, tag, comm, context_offset, &flag, status);
        } else {
            mpi_errno =
                MPIDI_iprobe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status,
                             &flag);
        }
#endif
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno = MPID_Progress_test();
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_PROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_improbe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_improbe(int transport,
                                           int source,
                                           int tag, MPIR_Comm * comm,
                                           int context_offset,
                                           MPIDI_av_entry_t * av,
                                           MPI_Status * status, int *flag, MPIR_Request ** message)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IMPROBE);

    if (transport == MPIDI_CH4R_NETMOD) {
        int vni_idx ATTRIBUTE((unused)) = 0, cs_acq = 0;
        MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_locks[vni_idx], cs_acq);

        /* Handoff branch: under the handoff model, this(main) thread sets the flag
         * 'processed' to false and submits an improbe request to the handoff queue.
         * It then waits until the progress thread processes the request and sets
         * the flag 'processed' to true. */
        if (!cs_acq) {
            /* Submit improbe request */
            MPIR_Request *req;
            OPA_int_t processed;
            req = MPIR_Request_create(MPIR_REQUEST_KIND__MPROBE);
            MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
            OPA_store_int(&processed, 0);
            MPIDI_workq_pt2pt_enqueue(MPIDI_comm_vni_to_workq(comm, vni_idx),
                                      IMPROBE, NULL /*send_buf */ , NULL /*buf */ , 0 /* count */ ,
                                      -1 /* datatype */ ,
                                      source, tag, comm, context_offset, av,
                                      status, req, flag, message, &processed);
            /* Busy loop to block until improbe req is completed by the progress thread */
            while (!OPA_load_int(&processed));
            /* FIXME: alternatively may use yield, e.g., MPL_thread_yield if problem arises when
             * sharing hardware thread between the main/this thread and the progress thread */
            MPIR_Request_free(req);     /* Now it's safe to free the request */
            mpi_errno = MPI_SUCCESS;
        } else {        /* Non handoff branch */
            mpi_errno =
                MPIDI_NM_mpi_improbe(source, tag, comm, context_offset, av, flag, message, status);
            MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IMPROBE);
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
    /* Keep re-submitting improbe query until a match is found */
    while (!flag) {
#ifdef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_improbe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status, &flag,
                          message);
#else
        if (unlikely(source == MPI_ANY_SOURCE)) {
            mpi_errno =
                MPIDI_SHM_mpi_improbe(source, tag, comm, context_offset, &flag, message, status);
            if (flag) {
                MPIDI_CH4I_REQUEST(*message, is_local) = 1;
            } else {
                mpi_errno =
                    MPIDI_improbe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status,
                                  &flag, message);
                if (flag)
                    MPIDI_CH4I_REQUEST(*message, is_local) = 0;
            }
        } else if (MPIDI_av_is_local(av)) {
            mpi_errno =
                MPIDI_SHM_mpi_improbe(source, tag, comm, context_offset, &flag, message, status);
            if (flag)
                MPIDI_CH4I_REQUEST(*message, is_local) = 1;
        } else {
            mpi_errno =
                MPIDI_improbe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status,
                              &flag, message);
            if (flag)
                MPIDI_CH4I_REQUEST(*message, is_local) = 0;
        }
#endif
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno = MPID_Progress_test();
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
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

    *flag = 0;
    av = MPIDIU_comm_rank_to_av(comm, source);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_improbe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status, flag,
                      message);
#else
    if (unlikely(source == MPI_ANY_SOURCE)) {
        mpi_errno = MPIDI_SHM_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
        if (*flag) {
            MPIDI_CH4I_REQUEST(*message, is_local) = 1;
        } else {
            mpi_errno =
                MPIDI_improbe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status,
                              flag, message);
            if (*flag) {
                MPIDI_CH4I_REQUEST(*message, is_local) = 0;
            }
        }
    } else if (MPIDI_av_is_local(av)) {
        mpi_errno = MPIDI_SHM_mpi_improbe(source, tag, comm, context_offset, flag, message, status);
        if (*flag)
            MPIDI_CH4I_REQUEST(*message, is_local) = 1;
    } else {
        mpi_errno =
            MPIDI_improbe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status, flag,
                          message);
        if (*flag)
            MPIDI_CH4I_REQUEST(*message, is_local) = 0;
    }
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    if (!*flag) {
        mpi_errno = MPID_Progress_test();
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

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

    *flag = 0;
    av = MPIDIU_comm_rank_to_av(comm, source);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_iprobe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status, flag);
#else
    if (unlikely(source == MPI_ANY_SOURCE)) {
        mpi_errno = MPIDI_SHM_mpi_iprobe(source, tag, comm, context_offset, flag, status);
        if (!*flag)
            mpi_errno =
                MPIDI_iprobe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status,
                             flag);
    } else if (MPIDI_CH4_rank_is_local(source, comm)) {
        mpi_errno = MPIDI_SHM_mpi_iprobe(source, tag, comm, context_offset, flag, status);
    } else {
        mpi_errno =
            MPIDI_iprobe(MPIDI_CH4R_NETMOD, source, tag, comm, context_offset, av, status, flag);
    }
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    if (!*flag) {
        mpi_errno = MPID_Progress_test();
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IPROBE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_PROBE_H_INCLUDED */
