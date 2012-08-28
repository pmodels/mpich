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
  unsigned peer, index;
  MPID_Group *group = info->win->mpid.sync.pw.group;
  MPID_assert(group != NULL);
  MPIDI_Win_control_t msg = {
    .type = MPIDI_WIN_MSGTYPE_POST,
  };

  for (index=0; index < group->size; ++index) {
    peer = group->lrank_to_lpid[index].lpid;
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
  unsigned peer, index;
  MPID_Group *group = info->win->mpid.sync.sc.group;
  MPID_assert(group != NULL);
  MPIDI_Win_control_t msg = {
    .type = MPIDI_WIN_MSGTYPE_COMPLETE,
  };

  for (index=0; index < group->size; ++index) {
    peer = group->lrank_to_lpid[index].lpid;
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
  MPIR_Group_add_ref(group);

  struct MPIDI_Win_sync* sync = &win->mpid.sync;
  MPID_PROGRESS_WAIT_WHILE(group->size != sync->pw.count);
  sync->pw.count = 0;

  MPID_assert(win->mpid.sync.sc.group == NULL);
  win->mpid.sync.sc.group = group;

  return mpi_errno;
}


int
MPID_Win_complete(MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;

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
  MPIR_Group_add_ref(group);

  MPID_assert(win->mpid.sync.pw.group == NULL);
  win->mpid.sync.pw.group = group;

  MPIDI_WinPSCW_info info = {
    .done = 0,
    .win  = win,
  };
  MPIDI_Context_post(MPIDI_Context[0], &info.work, MPIDI_WinPost_post, &info);
  MPID_PROGRESS_WAIT_WHILE(!info.done);

  return mpi_errno;
}


int
MPID_Win_wait(MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;

  struct MPIDI_Win_sync* sync = &win->mpid.sync;
  MPID_Group *group = sync->pw.group;
  MPID_PROGRESS_WAIT_WHILE(group->size != sync->sc.count);
  sync->sc.count = 0;
  sync->pw.group = NULL;

  MPIR_Group_release(group);
  return mpi_errno;
}


int
MPID_Win_test(MPID_Win *win,
              int      *flag)
{
  int mpi_errno = MPI_SUCCESS;

  struct MPIDI_Win_sync* sync = &win->mpid.sync;
  MPID_Group *group = sync->pw.group;
  if (group->size == sync->sc.count)
    {
      sync->sc.count = 0;
      sync->pw.group = NULL;
      *flag = 1;
      MPIR_Group_release(group);
    }
  else
    {
      MPID_Progress_poke();
    }

  return mpi_errno;
}
