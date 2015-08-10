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
 * \file src/onesided/mpid_win_fence.c
 * \brief ???
 */
#include "mpidi_onesided.h"


int
MPID_Win_fence(int       assert,
               MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  MPIR_Errflag_t errflag = MPIR_ERR_NONE;
  static char FCNAME[] = "MPID_Win_fence";

  if(win->mpid.sync.origin_epoch_type != win->mpid.sync.target_epoch_type){
       MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
  }

  if (!(assert & MPI_MODE_NOPRECEDE) &&
            win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_FENCE &&
            win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_REFENCE &&
            win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_NONE) {
        /* --BEGIN ERROR HANDLING-- */
        MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                return mpi_errno, "**rmasync");
        /* --END ERROR HANDLING-- */
  }


  struct MPIDI_Win_sync* sync = &win->mpid.sync;
  MPID_PROGRESS_WAIT_WHILE(sync->total != sync->complete);
  sync->total    = 0;
  sync->started  = 0;
  sync->complete = 0;

  if(assert & MPI_MODE_NOSUCCEED)
  {
    win->mpid.sync.origin_epoch_type = MPID_EPOTYPE_NONE;
    win->mpid.sync.target_epoch_type = MPID_EPOTYPE_NONE;
  }
  else{
    win->mpid.sync.origin_epoch_type = MPID_EPOTYPE_REFENCE;
    win->mpid.sync.target_epoch_type = MPID_EPOTYPE_REFENCE;
  }

  if (!(assert & MPI_MODE_NOPRECEDE))
    {
      mpi_errno = MPIR_Barrier_impl(win->comm_ptr, &errflag);
    }

  return mpi_errno;
}
