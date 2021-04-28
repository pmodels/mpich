/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDI_CH3_IMPL_H_INCLUDED
#define MPIDI_CH3_IMPL_H_INCLUDED

#include "mpidimpl.h"
#include "mpidu_generic_queue.h"
#include "utlist.h"

#if defined(HAVE_ASSERT_H)
#include <assert.h>
#endif

extern void *MPIDI_CH3_packet_buffer;
extern int MPIDI_CH3I_my_rank;

typedef GENERIC_Q_DECL(struct MPIR_Request) MPIDI_CH3I_shm_sendq_t;
extern MPIDI_CH3I_shm_sendq_t MPIDI_CH3I_shm_sendq;
extern struct MPIR_Request *MPIDI_CH3I_shm_active_send;

/* Send queue macros */
/* MT - not thread safe! */
#define MPIDI_CH3I_Sendq_empty(q) GENERIC_Q_EMPTY (q)
#define MPIDI_CH3I_Sendq_head(q) GENERIC_Q_HEAD (q)
#define MPIDI_CH3I_Sendq_enqueue(qp, ep) do {                                           \
        /* add refcount so req doesn't get freed before it's dequeued */                \
        MPIR_Request_add_ref(ep);                                                       \
        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL, VERBOSE, (MPL_DBG_FDEST,                         \
                          "MPIDI_CH3I_Sendq_enqueue req=%p (handle=%#x), queue=%p",     \
                          ep, (ep)->handle, qp));                                       \
        GENERIC_Q_ENQUEUE (qp, ep, dev.next);                                           \
    } while (0)
#define MPIDI_CH3I_Sendq_dequeue(qp, ep)  do {                                          \
        GENERIC_Q_DEQUEUE (qp, ep, dev.next);                                           \
        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL, VERBOSE, (MPL_DBG_FDEST,                         \
                          "MPIDI_CH3I_Sendq_dequeuereq=%p (handle=%#x), queue=%p",      \
                          *(ep), *(ep) ? (*(ep))->handle : -1, qp));                    \
        MPIR_Request_free(*(ep));                                                    \
    } while (0)
#define MPIDI_CH3I_Sendq_enqueue_multiple_no_refcount(qp, ep0, ep1)             \
    /* used to move reqs from one queue to another, so we don't update */       \
    /* the refcounts */                                                         \
    GENERIC_Q_ENQUEUE_MULTIPLE(qp, ep0, ep1, dev.next)

int MPIDI_CH3I_Shm_supported(void);
int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);
int MPIDI_CH3I_Shm_send_progress(void);
int MPIDI_CH3I_Complete_sendq_with_error(MPIDI_VC_t * vc);

int MPIDI_CH3I_SendNoncontig( MPIDI_VC_t *vc, MPIR_Request *sreq, void *header, intptr_t hdr_sz,
                              struct iovec *hdr_iov, int n_hdr_iov);

int MPID_nem_lmt_shm_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *rts_pkt, MPIR_Request *req);
int MPID_nem_lmt_shm_start_recv(MPIDI_VC_t *vc, MPIR_Request *req, struct iovec s_cookie);
int MPID_nem_lmt_shm_start_send(MPIDI_VC_t *vc, MPIR_Request *req, struct iovec r_cookie);
int MPID_nem_lmt_shm_handle_cookie(MPIDI_VC_t *vc, MPIR_Request *req, struct iovec cookie);
int MPID_nem_lmt_shm_done_send(MPIDI_VC_t *vc, MPIR_Request *req);
int MPID_nem_lmt_shm_done_recv(MPIDI_VC_t *vc, MPIR_Request *req);
int MPID_nem_lmt_shm_vc_terminated(MPIDI_VC_t *vc);

int MPID_nem_lmt_dma_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *rts_pkt, MPIR_Request *req);
int MPID_nem_lmt_dma_start_recv(MPIDI_VC_t *vc, MPIR_Request *req, struct iovec s_cookie);
int MPID_nem_lmt_dma_start_send(MPIDI_VC_t *vc, MPIR_Request *req, struct iovec r_cookie);
int MPID_nem_lmt_dma_handle_cookie(MPIDI_VC_t *vc, MPIR_Request *req, struct iovec cookie);
int MPID_nem_lmt_dma_done_send(MPIDI_VC_t *vc, MPIR_Request *req);
int MPID_nem_lmt_dma_done_recv(MPIDI_VC_t *vc, MPIR_Request *req);
int MPID_nem_lmt_dma_vc_terminated(MPIDI_VC_t *vc);

int MPID_nem_lmt_vmsplice_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *rts_pkt, MPIR_Request *req);
int MPID_nem_lmt_vmsplice_start_recv(MPIDI_VC_t *vc, MPIR_Request *req, struct iovec s_cookie);
int MPID_nem_lmt_vmsplice_start_send(MPIDI_VC_t *vc, MPIR_Request *req, struct iovec r_cookie);
int MPID_nem_lmt_vmsplice_handle_cookie(MPIDI_VC_t *vc, MPIR_Request *req, struct iovec cookie);
int MPID_nem_lmt_vmsplice_done_send(MPIDI_VC_t *vc, MPIR_Request *req);
int MPID_nem_lmt_vmsplice_done_recv(MPIDI_VC_t *vc, MPIR_Request *req);
int MPID_nem_lmt_vmsplice_vc_terminated(MPIDI_VC_t *vc);

int MPID_nem_handle_pkt(MPIDI_VC_t *vc, char *buf, intptr_t buflen);

/* Nemesis-provided RMA implementation */
int MPIDI_CH3_SHM_Win_shared_query(MPIR_Win *win_ptr, int target_rank, MPI_Aint *size, int *disp_unit, void *baseptr);
int MPIDI_CH3_SHM_Win_free(MPIR_Win **win_ptr);

/* Shared memory window atomic/accumulate mutex implementation */

#if !defined(HAVE_WINDOWS_H)
#define MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr)                                              \
    do {                                                                                \
        int pt_err = pthread_mutex_lock((win_ptr)->shm_mutex);                          \
        MPIR_Assert(pt_err == 0); \
    } while (0)

#define MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr)                                            \
    do {                                                                                \
        int pt_err = pthread_mutex_unlock((win_ptr)->shm_mutex);                        \
        MPIR_Assert(pt_err == 0); \
    } while (0)

#define MPIDI_CH3I_SHM_MUTEX_INIT(win_ptr)                                              \
    do {                                                                                \
        int pt_err;                                                                     \
        pthread_mutexattr_t attr;                                                       \
                                                                                        \
        pt_err = pthread_mutexattr_init(&attr);                                         \
        MPIR_ERR_CHKANDJUMP1(pt_err, mpi_errno, MPI_ERR_OTHER, "**pthread_mutex",       \
                             "**pthread_mutex %s", strerror(pt_err));                   \
        pt_err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);           \
        MPIR_ERR_CHKANDJUMP1(pt_err, mpi_errno, MPI_ERR_OTHER, "**pthread_mutex",       \
                             "**pthread_mutex %s", strerror(pt_err));                   \
        pt_err = pthread_mutex_init((win_ptr)->shm_mutex, &attr);                       \
        MPIR_ERR_CHKANDJUMP1(pt_err, mpi_errno, MPI_ERR_OTHER, "**pthread_mutex",       \
                             "**pthread_mutex %s", strerror(pt_err));                   \
        pt_err = pthread_mutexattr_destroy(&attr);                                      \
        MPIR_ERR_CHKANDJUMP1(pt_err, mpi_errno, MPI_ERR_OTHER, "**pthread_mutex",       \
                             "**pthread_mutex %s", strerror(pt_err));                   \
    } while (0);

#define MPIDI_CH3I_SHM_MUTEX_DESTROY(win_ptr)                                           \
    do {                                                                                \
        int pt_err = pthread_mutex_destroy((win_ptr)->shm_mutex);                       \
        MPIR_ERR_CHKANDJUMP1(pt_err, mpi_errno, MPI_ERR_OTHER, "**pthread_mutex",       \
                             "**pthread_mutex %s", strerror(pt_err));                   \
    } while (0);
#else
#define HANDLE_WIN_MUTEX_ERROR()                                                        \
    do {                                                                                \
        HLOCAL str;                                                                     \
        char error_msg[MPIR_STRERROR_BUF_SIZE];                                         \
        DWORD error = GetLastError();                                                   \
        int num_bytes = FormatMessage(                                                  \
        FORMAT_MESSAGE_FROM_SYSTEM |                                                    \
        FORMAT_MESSAGE_ALLOCATE_BUFFER,                                                 \
        0,                                                                              \
        error,                                                                          \
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),                                    \
        (LPTSTR) &str,                                                                  \
        0,0);                                                                           \
                                                                                        \
        if (num_bytes != 0) {                                                           \
            int pt_err = 1;                                                             \
            int mpi_errno = MPI_ERR_OTHER;                                              \
            MPL_strncpy(error_msg, str, MPIR_STRERROR_BUF_SIZE);                       \
            LocalFree(str);                                                             \
            strtok(error_msg, "\r\n");                                                  \
            MPIR_ERR_CHKANDJUMP1(pt_err, mpi_errno, MPI_ERR_OTHER, "**windows_mutex",   \
                                 "**windows_mutex %s", error_msg);                      \
        }                                                                               \
    } while (0);

#define MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr)                                              \
    do {                                                                                \
        DWORD result = WaitForSingleObject(*((win_ptr)->shm_mutex), INFINITE);          \
        if (result == WAIT_FAILED) {                                                    \
            HANDLE_WIN_MUTEX_ERROR();                                                   \
        }                                                                               \
    } while (0);

#define MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr)                                            \
    do {                                                                                \
        BOOL result = ReleaseMutex(*((win_ptr)->shm_mutex));                            \
        if (!result) {                                                                  \
            HANDLE_WIN_MUTEX_ERROR();                                                   \
        }                                                                               \
    } while (0);

#define MPIDI_CH3I_SHM_MUTEX_INIT(win_ptr)                                              \
    do {                                                                                \
        *((win_ptr)->shm_mutex) = CreateMutex(NULL, FALSE, NULL);                       \
        if (*((win_ptr)->shm_mutex) == NULL) {                                          \
            HANDLE_WIN_MUTEX_ERROR();                                                   \
        }                                                                               \
    } while (0);

#define MPIDI_CH3I_SHM_MUTEX_DESTROY(win_ptr)                                           \
    do {                                                                                \
        BOOL result = CloseHandle(*((win_ptr)->shm_mutex));                             \
        if (!result) {                                                                  \
            HANDLE_WIN_MUTEX_ERROR();                                                   \
        }                                                                               \
    } while (0);
#endif /* !defined(HAVE_WINDOWS_H) */


/* Starting of shared window list */

typedef struct MPIDI_SHM_Win {
    struct MPIDI_SHM_Win *prev;
    struct MPIDI_SHM_Win *next;
    MPIR_Win *win;
} MPIDI_SHM_Win_t;

typedef MPIDI_SHM_Win_t *MPIDI_SHM_Wins_list_t;

extern MPIDI_SHM_Wins_list_t shm_wins_list;

#define MPIDI_SHM_Wins_next_and_continue(elem) {elem = elem->next; continue;}

static inline int MPIDI_CH3I_SHM_Wins_append(MPIDI_SHM_Wins_list_t * list, MPIR_Win * win)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_SHM_Win_t *tmp_ptr;
    MPIR_CHKPMEM_DECL(1);

    /* FIXME: We should use a pool allocator here */
    MPIR_CHKPMEM_MALLOC(tmp_ptr, MPIDI_SHM_Win_t *, sizeof(MPIDI_SHM_Win_t),
                        mpi_errno, "SHM window entry", MPL_MEM_SHM);

    tmp_ptr->next = NULL;
    tmp_ptr->win = win;

    DL_APPEND(*list, tmp_ptr);

  fn_exit:
    MPIR_CHKPMEM_COMMIT();
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

/* Unlink an element from the SHM window list
 *
 * @param IN    list      Pointer to the SHM window list
 * @param IN    elem      Pointer to the element to be unlinked
 */
static inline void MPIDI_CH3I_SHM_Wins_unlink(MPIDI_SHM_Wins_list_t * list, MPIR_Win * shm_win)
{
    MPIDI_SHM_Win_t *elem = NULL;
    MPIDI_SHM_Win_t *tmp_elem = NULL;

    LL_SEARCH_SCALAR(*list, elem, win, shm_win);
    if (elem != NULL) {
        tmp_elem = elem;
        DL_DELETE(*list, elem);
        MPL_free(tmp_elem);
    }
}

#endif /* MPIDI_CH3_IMPL_H_INCLUDED */
