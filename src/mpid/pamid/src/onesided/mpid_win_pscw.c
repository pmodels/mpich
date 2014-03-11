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
 * \file src/onesided/mpid_win_pscw.c
 * \brief ???
 */
#include "mpidi_onesided.h"


typedef struct
{
  MPID_Win          * win;

  volatile unsigned   done;
  pami_work_t         work;
} MPIDI_WinPSCW_info;


static pami_result_t
MPIDI_WinPost_post(pami_context_t   context,
                   void           * _info)
{
  MPIDI_WinPSCW_info * info = (MPIDI_WinPSCW_info*)_info;
  unsigned peer, index, pid,i;
  MPID_Group *group = info->win->mpid.sync.pw.group;
  MPID_assert(group != NULL);
  MPIDI_Win_control_t msg = {
    .type = MPIDI_WIN_MSGTYPE_POST,
  };

  for (index=0; index < group->size; ++index) {
      pid = group->lrank_to_lpid[index].lpid;
      for (i=0;i < ((info->win)->comm_ptr->local_size); i++) {
         if ((info->win)->comm_ptr->local_group->lrank_to_lpid[i].lpid == pid) {
             peer = ((info->win)->comm_ptr->local_group->lrank_to_lpid[i].lrank);
             break;
         }
      }
    MPIDI_WinCtrlSend(context, &msg, peer, info->win);
  }

  info->done = 1;
  return PAMI_SUCCESS;
}


void
MPIDI_WinPost_proc(pami_context_t              context,
                   const MPIDI_Win_control_t * info,
                   unsigned                    peer)
{
  ++info->win->mpid.sync.pw.count;
}


static pami_result_t
MPIDI_WinComplete_post(pami_context_t   context,
                       void           * _info)
{
  MPIDI_WinPSCW_info * info = (MPIDI_WinPSCW_info*)_info;
  unsigned peer, index,pid,i;
  MPID_Group *group = info->win->mpid.sync.sc.group;
  MPID_assert(group != NULL);
  MPIDI_Win_control_t msg = {
    .type = MPIDI_WIN_MSGTYPE_COMPLETE,
  };

  for (index=0; index < group->size; ++index) {
     pid = group->lrank_to_lpid[index].lpid;
     for (i=0;i < ((info->win)->comm_ptr->local_size); i++) {
         if ((info->win)->comm_ptr->local_group->lrank_to_lpid[i].lpid == pid) {
            peer = ((info->win)->comm_ptr->local_group->lrank_to_lpid[i].lrank);
            break;
         }
     }
     MPIDI_WinCtrlSend(context, &msg, peer, info->win);
  }

  info->done = 1;
  return PAMI_SUCCESS;
}


void
MPIDI_WinComplete_proc(pami_context_t              context,
                       const MPIDI_Win_control_t * info,
                       unsigned                    peer)
{
  ++info->win->mpid.sync.sc.count;
}


int
MPID_Win_start(MPID_Group *group,
               int         assert,
               MPID_Win   *win)
{
  int mpi_errno = MPI_SUCCESS;
  static char FCNAME[] = "MPID_Win_start";
  if(win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_NONE &&
    win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_REFENCE)
  {
    MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
  }

  MPIR_Group_add_ref(group);

  struct MPIDI_Win_sync* sync = &win->mpid.sync;
  MPID_PROGRESS_WAIT_WHILE(group->size != sync->pw.count);
  sync->pw.count = 0;

  MPIU_ERR_CHKANDJUMP((win->mpid.sync.sc.group != NULL), mpi_errno, MPI_ERR_GROUP, "**group");

  win->mpid.sync.sc.group = group;
  win->mpid.sync.origin_epoch_type = MPID_EPOTYPE_START;

fn_exit:
  return mpi_errno;
fn_fail:
  goto fn_exit;
}


int
MPID_Win_complete(MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  static char FCNAME[] = "MPID_Win_complete";
  if(win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_START){
     MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
  }

  struct MPIDI_Win_sync* sync = &win->mpid.sync;
  MPID_PROGRESS_WAIT_WHILE(sync->total != sync->complete);
  sync->total    = 0;
  sync->started  = 0;
  sync->complete = 0;

  MPIDI_WinPSCW_info info = {
    .done = 0,
    .win  = win,
  };
  MPIDI_Context_post(MPIDI_Context[0], &info.work, MPIDI_WinComplete_post, &info);
  MPID_PROGRESS_WAIT_WHILE(!info.done);

  if(win->mpid.sync.target_epoch_type == MPID_EPOTYPE_REFENCE)
  {
    win->mpid.sync.origin_epoch_type = MPID_EPOTYPE_REFENCE;
  }else{
    win->mpid.sync.origin_epoch_type = MPID_EPOTYPE_NONE;
  }

  MPIR_Group_release(sync->sc.group);
  sync->sc.group = NULL;
  return mpi_errno;
}


int
MPID_Win_post(MPID_Group *group,
              int         assert,
              MPID_Win   *win)
{
  int mpi_errno = MPI_SUCCESS;
  static char FCNAME[] = "MPID_Win_post";
  if(win->mpid.sync.target_epoch_type != MPID_EPOTYPE_NONE &&
     win->mpid.sync.target_epoch_type != MPID_EPOTYPE_REFENCE){
       MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
  }

  MPIR_Group_add_ref(group);

  MPIU_ERR_CHKANDJUMP((win->mpid.sync.pw.group != NULL), mpi_errno, MPI_ERR_GROUP, "**group");

  win->mpid.sync.pw.group = group;

  MPIDI_WinPSCW_info info = {
    .done = 0,
    .win  = win,
  };
  MPIDI_Context_post(MPIDI_Context[0], &info.work, MPIDI_WinPost_post, &info);
  MPID_PROGRESS_WAIT_WHILE(!info.done);

  win->mpid.sync.target_epoch_type = MPID_EPOTYPE_POST;

fn_fail:
  return mpi_errno;
}


int
MPID_Win_wait(MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  static char FCNAME[] = "MPID_Win_wait";
  struct MPIDI_Win_sync* sync = &win->mpid.sync;

  if(win->mpid.sync.target_epoch_type != MPID_EPOTYPE_POST){
    MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
  }

  MPID_Group *group = sync->pw.group;
  MPID_PROGRESS_WAIT_WHILE(group->size != sync->sc.count);
  sync->sc.count = 0;
  sync->pw.group = NULL;

  MPIR_Group_release(group);

  if(win->mpid.sync.origin_epoch_type == MPID_EPOTYPE_REFENCE){
    win->mpid.sync.target_epoch_type = MPID_EPOTYPE_REFENCE;
  }else{
    win->mpid.sync.target_epoch_type = MPID_EPOTYPE_NONE;
  }
  return mpi_errno;
}


int
MPID_Win_test(MPID_Win *win,
              int      *flag)
{
  int mpi_errno = MPI_SUCCESS;
  static char FCNAME[] = "MPID_Win_test";
  struct MPIDI_Win_sync* sync = &win->mpid.sync;

  if(win->mpid.sync.target_epoch_type != MPID_EPOTYPE_POST){
    MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
  }

  MPID_Group *group = sync->pw.group;
  if (group->size == sync->sc.count)
    {
      sync->sc.count = 0;
      sync->pw.group = NULL;
      *flag = 1;
      MPIR_Group_release(group);
      if(win->mpid.sync.origin_epoch_type == MPID_EPOTYPE_REFENCE){
        win->mpid.sync.target_epoch_type = MPID_EPOTYPE_REFENCE;
      }else{
        win->mpid.sync.target_epoch_type = MPID_EPOTYPE_NONE;
      }
    }
  else
    {
      *flag = 0;
      MPID_Progress_poke();
    }

  return mpi_errno;
}
