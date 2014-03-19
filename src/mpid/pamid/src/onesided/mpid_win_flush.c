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
 * \file src/onesided/mpid_win_flush.c
 * \brief returns a new info object containing the hints of the window
 *        associated with win.                                          
 */
#include "mpidi_onesided.h"

/**
 * \brief MPI-PAMI glue for MPI_Win_flush function
 *
 * The funcion can be called only within passive target epochs such as
 * MPI_Win_lock, MPI_Win_unlock, MPI_Win_lock_all and MPI_Win_unlock_all.
 *
 * The function completes all outstanding RMA operations initialized by
 * the calling process to a specified target rank on the given window.
 * The operations are completed both at the origin and the target.
 *
 * \param[in] rank      rank of target window
 * \param[in] win       window object
 * \return MPI_SUCCESS, MPI_ERR_OTHER
 */


int
MPID_Win_flush(int       rank,
               MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  struct MPIDI_Win_sync* sync;
  static char FCNAME[] = "MPID_Win_flush";

  if((win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK) &&
     (win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK_ALL))
     {
      MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
     }
  sync = &win->mpid.sync;
  MPID_PROGRESS_WAIT_DO_WHILE(sync->total != sync->complete);
  sync->total    = 0;
  sync->started  = 0;
  sync->complete = 0;

  return mpi_errno;
}
/**
 * \brief MPI-PAMI glue for MPI_Win_flush_all function
 *
 * The funcion can be called only within passive target epochs such as
 * MPI_Win_lock, MPI_Win_unlock, MPI_Win_lock_all and MPI_Win_unlock_all.
 *
 * All RMA opertions issued by the calling process to any target on the
 * given window prior to this call and in the given window will have
 * completed both at the origin and the target when the call returns.
 *
 * \param[in] win       window object
 * \return MPI_SUCCESS, MPI_ERR_OTHER
 */


int
MPID_Win_flush_all(MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  struct MPIDI_Win_sync* sync;
  static char FCNAME[] = "MPID_Win_flush_all";

  if((win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK) &&
     (win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK_ALL))
     {
      MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
     }

  sync = &win->mpid.sync;
  MPID_PROGRESS_WAIT_WHILE(sync->total != sync->complete);
  sync->total    = 0;
  sync->started  = 0;
  sync->complete = 0;
  return mpi_errno;

}

/**
 * \brief MPI-PAMI glue for MPI_Win_flush_local function
 *
 * The funcion can be called only within passive target epochs such as
 * MPI_Win_lock, MPI_Win_unlock, MPI_Win_lock_all and MPI_Win_unlock_all.
 *
 * Locally completes at the origin all outstanding RMA operations initiated
 * the cng process to the target rank on the given window. The user may
 * reuse any buffers after this routine returns.
 *
 * It has been determined that the routine uses only counters for each window
 * and not for each rank because the overhead of tracking each rank could be great
 * if a window group contains a large number of ranks. 
 *
 * \param[in] rank      rank of target window
 * \param[in] win       window object
 * \return MPI_SUCCESS, MPI_ERR_OTHER
 */

int
MPID_Win_flush_local(int rank, MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  struct MPIDI_Win_sync* sync;
  static char FCNAME[] = "MPID_Win_flush_local";

  if((win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK) &&
     (win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK_ALL))
     {
      MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
     }
   sync = &win->mpid.sync;
   MPID_PROGRESS_WAIT_WHILE(sync->total != sync->complete);
   sync->total    = 0;
   sync->started  = 0;
   sync->complete = 0;

  return mpi_errno;
}

/**
 * \brief MPI-PAMI glue for MPI_Win_flush_local_all function
 *
 * The funcion can be called only within passive target epochs such as
 * MPI_Win_lock, MPI_Win_unlock, MPI_Win_lock_all and MPI_Win_unlock_all.
 *
 * All RMA operations issued to any target prior to this call in this window
 * will have copleted at the origin when this function returns.
 *
 * \param[in] win       window object
 * \return MPI_SUCCESS, MPI_ERR_OTHER
 */

int
MPID_Win_flush_local_all(MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  struct MPIDI_Win_sync* sync;
  static char FCNAME[] = "MPID_Win_flush";

  if((win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK) &&
     (win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK_ALL))
     {
      MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
     }
  sync = &win->mpid.sync;
  MPID_PROGRESS_WAIT_WHILE(sync->total != sync->complete);
  sync->total    = 0;
  sync->started  = 0;
  sync->complete = 0;

  return mpi_errno;
}





