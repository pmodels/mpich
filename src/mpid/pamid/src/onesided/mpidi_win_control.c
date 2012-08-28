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
 * \file src/onesided/mpidi_win_control.c
 * \brief ???
 */
#include "mpidi_onesided.h"


void
MPIDI_WinCtrlSend(pami_context_t       context,
                  MPIDI_Win_control_t *control,
                  pami_task_t          peer,
                  MPID_Win            *win)
{
  control->win = win->mpid.info[peer].win;

  pami_endpoint_t dest;
  pami_result_t rc;
  rc = PAMI_Endpoint_create(MPIDI_Client, peer, 0, &dest);
  MPID_assert(rc == PAMI_SUCCESS);

  pami_send_immediate_t params = {
    .dispatch = MPIDI_Protocols_WinCtrl,
    .dest     = dest,
    .header   = {
      .iov_base = control,
      .iov_len  = sizeof(MPIDI_Win_control_t),
    },
    .data     = {
      .iov_base = NULL,
      .iov_len  = 0,
    },
  };

  rc = PAMI_Send_immediate(context, &params);
  MPID_assert(rc == PAMI_SUCCESS);
}


void
MPIDI_WinControlCB(pami_context_t    context,
                   void            * cookie,
                   const void      * _control,
                   size_t            size,
                   const void      * sndbuf,
                   size_t            sndlen,
                   pami_endpoint_t   sender,
                   pami_recv_t     * recv)
{
  MPID_assert(recv == NULL);
  MPID_assert(sndlen == 0);
  MPID_assert(_control != NULL);
  MPID_assert(size == sizeof(MPIDI_Win_control_t));
  const MPIDI_Win_control_t *control = (const MPIDI_Win_control_t *)_control;
  pami_task_t senderrank = PAMIX_Endpoint_query(sender);

  switch (control->type)
    {
    case MPIDI_WIN_MSGTYPE_LOCKREQ:
      MPIDI_WinLockReq_proc(context, control, senderrank);
      break;
    case MPIDI_WIN_MSGTYPE_LOCKACK:
      MPIDI_WinLockAck_proc(context, control, senderrank);
      break;
    case MPIDI_WIN_MSGTYPE_UNLOCK:
      MPIDI_WinUnlock_proc(context, control, senderrank);
      break;

    case MPIDI_WIN_MSGTYPE_COMPLETE:
      MPIDI_WinComplete_proc(context, control, senderrank);
      break;
    case MPIDI_WIN_MSGTYPE_POST:
      MPIDI_WinPost_proc(context, control, senderrank);
      break;

    default:
      fprintf(stderr, "Bad win-control type: 0x%08x  %d\n",
              control->type,
              control->type);
      MPID_abort();
    }

  MPIDI_Progress_signal();
}
