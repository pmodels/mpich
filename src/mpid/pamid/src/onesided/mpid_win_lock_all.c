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
 * \file src/onesided/mpid_win_lock_all.c
 * \brief ???
 */
#include "mpidi_onesided.h"


static pami_result_t
MPIDI_WinLockAllReq_post(pami_context_t   context,
                         void           * _info)
{
  MPIDI_WinLock_info* info = (MPIDI_WinLock_info*)_info;
  MPIDI_Win_control_t msg = {
    .type       = MPIDI_WIN_MSGTYPE_LOCKALLREQ,
    .flagAddr   = info,
    .data       = {
      .lock       = {
        .type = info->lock_type,
      },
    },
  };

  MPIDI_WinCtrlSend(context, &msg, info->peer, info->win);
  return PAMI_SUCCESS;
}


static pami_result_t
MPIDI_WinUnlockAllReq_post(pami_context_t   context,
                        void           * _info)
{
  MPIDI_WinLock_info* info = (MPIDI_WinLock_info*)_info;
  MPIDI_Win_control_t msg = {
  .type       = MPIDI_WIN_MSGTYPE_UNLOCKALL,
  .flagAddr   = info,
  };
  MPIDI_WinCtrlSend(context, &msg, info->peer, info->win);
  return PAMI_SUCCESS;
}

void
MPIDI_WinUnlockAll_proc(pami_context_t              context,
                        const MPIDI_Win_control_t * info,
                        unsigned                    peer)
{
  MPID_Win *win = info->win;
  --win->mpid.sync.lock.local.count;
  MPID_assert((int)win->mpid.sync.lock.local.count >= 0);
  MPIDI_WinLockAdvance(context, win);
}

void
MPIDI_WinLockAllAck_post(pami_context_t   context,
                      unsigned         peer,
                      MPID_Win       * win)
{
  MPIDI_Win_control_t info = {
  .type       = MPIDI_WIN_MSGTYPE_LOCKALLACK,
  };
  MPIDI_WinCtrlSend(context, &info, peer, win);
}


int
MPID_Win_lock_all(int      assert,
                  MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  int i,size;
  MPIDI_WinLock_info *lockQ;
  char *cp=NULL;
  int  nMask,index;
  struct MPIDI_Win_sync_lock* slock = &win->mpid.sync.lock;
  static char FCNAME[] = "MPID_Win_lock_all";

  if(win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_NONE &&
     win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_REFENCE){
    MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
   }
   size = (MPIR_Comm_size(win->comm_ptr));
   win->mpid.max_ctrlsends = MAX_NUM_CTRLSEND;
   nMask= win->mpid.max_ctrlsends - 1;
   if ( (cp=getenv("MP_MAX_NUM_CTRLSEND")) ) {
       win->mpid.max_ctrlsends = atoi(cp);
   }
   nMask=(win->mpid.max_ctrlsends - 1);
   if (!win->mpid.work.msgQ) {
       if (size < (win->mpid.max_ctrlsends)) {
           win->mpid.work.msgQ = (void *) MPIU_Calloc0(size, MPIDI_WinLock_info);
       }  else {
           win->mpid.work.msgQ = (void *) MPIU_Calloc0((win->mpid.max_ctrlsends), MPIDI_WinLock_info);
       }
       MPID_assert(win->mpid.work.msgQ != NULL);
       win->mpid.work.count=0;
   }
   lockQ = (MPIDI_WinLock_info *) win->mpid.work.msgQ;
   if (size < win->mpid.max_ctrlsends) {
      for (i = 0; i < size; i++) {
           lockQ[i].done=0;
           lockQ[i].peer=i;
           lockQ[i].win=win;
           lockQ[i].lock_type=MPI_LOCK_SHARED;
           MPIDI_Context_post(MPIDI_Context[0], &lockQ[i].work, MPIDI_WinLockAllReq_post, &lockQ[i]);
      }
    } else {
      for (i = 0; i < size; i++) {
           if (i < win->mpid.max_ctrlsends)
               index=i;
           else {
               index = i & nMask;
               if (!lockQ[index].done) {
                   MPID_PROGRESS_WAIT_WHILE(lockQ[index].done == 0);
               }
           }
           lockQ[index].done=0;
           lockQ[index].peer=i;
           lockQ[index].win=win;
           lockQ[index].lock_type=MPI_LOCK_SHARED;
           MPIDI_Context_post(MPIDI_Context[0], &lockQ[index].work, MPIDI_WinLockAllReq_post, &lockQ[index]);
      }
    }
    /* wait for the lock is granted for all tasks in the window */
   MPID_PROGRESS_WAIT_WHILE(size != slock->remote.allLocked);

   win->mpid.sync.origin_epoch_type = MPID_EPOTYPE_LOCK_ALL;

  return mpi_errno;
}


int
MPID_Win_unlock_all(MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  int i,size;
  MPIDI_WinLock_info *lockQ;
  int  nMask,index;
  struct MPIDI_Win_sync* sync;
  static char FCNAME[] = "MPID_Win_unlock_all";

  if (win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK_ALL) {   
      MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
   }

  sync = &win->mpid.sync;
  MPID_PROGRESS_WAIT_WHILE(sync->total != sync->complete);
  sync->total    = 0;
  sync->started  = 0;
  sync->complete = 0;
  MPID_assert(win->mpid.work.msgQ != NULL);
  lockQ = (MPIDI_WinLock_info *) win->mpid.work.msgQ;
  size = MPIR_Comm_size(win->comm_ptr);
  nMask = (win->mpid.max_ctrlsends - 1);
  if (size < win->mpid.max_ctrlsends) {
      for (i = 0; i < size; i++) {
           lockQ[i].done=0;
           lockQ[i].peer=i;
           lockQ[i].win=win;
           MPIDI_Context_post(MPIDI_Context[0], &lockQ[i].work, MPIDI_WinUnlockAllReq_post, &lockQ[i]);
      }
   } else {
      for (i = 0; i < size; i++) {
           if (i < win->mpid.max_ctrlsends)
               index=i;
           else {
               index = (i & nMask);
               if (!lockQ[index].done) {
                  MPID_PROGRESS_WAIT_WHILE(lockQ[index].done == 0);
               }
           }
           lockQ[index].done=0;
           lockQ[index].peer=i;
           lockQ[index].win=win;
           MPIDI_Context_post(MPIDI_Context[0], &lockQ[index].work, MPIDI_WinUnlockAllReq_post, &lockQ[index]);
      }
  }
  
  MPID_PROGRESS_WAIT_WHILE(sync->lock.remote.allLocked);

  if(win->mpid.sync.target_epoch_type == MPID_EPOTYPE_REFENCE)
  {
    win->mpid.sync.origin_epoch_type = MPID_EPOTYPE_REFENCE;
  }else{
    win->mpid.sync.origin_epoch_type = MPID_EPOTYPE_NONE;
  }
  return mpi_errno;
}
