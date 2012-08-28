/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/onesided/mpid_win_lock.c
 * \brief ???
 */
#include "mpidi_onesided.h"


typedef struct
{
  unsigned            peer;
  int                 lock_type;
  MPID_Win          * win;

  volatile unsigned   done;
  pami_work_t         work;
} MPIDI_WinLock_info;


static inline void
MPIDI_WinLockAck_post(pami_context_t   context,
                      unsigned         peer,
                      MPID_Win       * win);


static inline void
MPIDI_WinLockAdvance(pami_context_t   context,
                     MPID_Win       * win)
{
  struct MPIDI_Win_sync_lock* slock = &win->mpid.sync.lock;
  struct MPIDI_Win_queue*     q     = &slock->local.requested;

  if (
      (q->head != NULL ) &&
      ( (slock->local.count == 0) ||
        (
         (slock->local.type == MPI_LOCK_SHARED) &&
         (q->head->type     == MPI_LOCK_SHARED)
         )
        )
      )
    {
      struct MPIDI_Win_lock* lock = q->head;
      q->head = lock->next;
      if (q->head == NULL)
        q->tail = NULL;

      ++slock->local.count;
      slock->local.type = lock->type;

      MPIDI_WinLockAck_post(context, lock->rank, win);
      MPIU_Free(lock);
      MPIDI_WinLockAdvance(context, win);
    }
}


static pami_result_t
MPIDI_WinLockReq_post(pami_context_t   context,
                      void           * _info)
{
  MPIDI_WinLock_info* info = (MPIDI_WinLock_info*)_info;
  MPIDI_Win_control_t msg = {
    .type       = MPIDI_WIN_MSGTYPE_LOCKREQ,
    .data       = {
      .lock       = {
        .type = info->lock_type,
      },
    },
  };

  MPIDI_WinCtrlSend(context, &msg, info->peer, info->win);
  return PAMI_SUCCESS;
}


void
MPIDI_WinLockReq_proc(pami_context_t              context,
                      const MPIDI_Win_control_t * info,
                      unsigned                    peer)
{
  MPID_Win * win = info->win;
  struct MPIDI_Win_lock* lock = MPIU_Calloc0(1, struct MPIDI_Win_lock);
  lock->rank = peer;
  lock->type = info->data.lock.type;

  struct MPIDI_Win_queue* q = &win->mpid.sync.lock.local.requested;
  MPID_assert( (q->head != NULL) ^ (q->tail == NULL) );
  if (q->tail == NULL)
    q->head = lock;
  else
    q->tail->next = lock;
  q->tail = lock;

  MPIDI_WinLockAdvance(context, win);
}


static inline void
MPIDI_WinLockAck_post(pami_context_t   context,
                      unsigned         peer,
                      MPID_Win       * win)
{
  MPIDI_Win_control_t info = {
  .type       = MPIDI_WIN_MSGTYPE_LOCKACK,
  };
  MPIDI_WinCtrlSend(context, &info, peer, win);
}


void
MPIDI_WinLockAck_proc(pami_context_t              context,
                      const MPIDI_Win_control_t * info,
                      unsigned                    peer)
{
  info->win->mpid.sync.lock.remote.locked = 1;
}


static pami_result_t
MPIDI_WinUnlock_post(pami_context_t   context,
                     void           * _info)
{
  MPIDI_WinLock_info* info = (MPIDI_WinLock_info*)_info;
  MPIDI_Win_control_t msg = {
  .type       = MPIDI_WIN_MSGTYPE_UNLOCK,
  };
  MPIDI_WinCtrlSend(context, &msg, info->peer, info->win);
  info->done = 1;
  return PAMI_SUCCESS;
}


void
MPIDI_WinUnlock_proc(pami_context_t              context,
                     const MPIDI_Win_control_t * info,
                     unsigned                    peer)
{
  MPID_Win *win = info->win;
  --win->mpid.sync.lock.local.count;
  MPID_assert((int)win->mpid.sync.lock.local.count >= 0);
  MPIDI_WinLockAdvance(context, win);
}


int
MPID_Win_lock(int       lock_type,
              int       rank,
              int       assert,
              MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  struct MPIDI_Win_sync_lock* slock = &win->mpid.sync.lock;

  MPIDI_WinLock_info info = {
  .done = 0,
  .peer = rank,
  .win  = win,
  .lock_type = lock_type
  };

  MPIDI_Context_post(MPIDI_Context[0], &info.work, MPIDI_WinLockReq_post, &info);
  MPID_PROGRESS_WAIT_WHILE(!slock->remote.locked);

  return mpi_errno;
}


int
MPID_Win_unlock(int       rank,
                MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;

  struct MPIDI_Win_sync* sync = &win->mpid.sync;
  MPID_PROGRESS_WAIT_WHILE(sync->total != sync->complete);
  sync->total    = 0;
  sync->started  = 0;
  sync->complete = 0;

  MPIDI_WinLock_info info = {
  .done = 0,
  .peer = rank,
  .win  = win,
  };
  MPIDI_Context_post(MPIDI_Context[0], &info.work, MPIDI_WinUnlock_post, &info);
  MPID_PROGRESS_WAIT_WHILE(!info.done);
  sync->lock.remote.locked = 0;
  return mpi_errno;
}
