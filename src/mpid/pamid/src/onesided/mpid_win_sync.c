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
 * \file src/onesided/mpid_win_sync.c
 * \brief returns a new info object containing the hints of the window
 *        associated with win.                                          
 */
#include "mpidi_onesided.h"

/**
 * \brief MPI-PAMI glue for MPI_Win_sync function
 *
 * The funcion synchronizes the private and public window copies of win.
 * For the purpose of synchronizing the private and public window,
 * MPI_Win_sync has the effect of ending and reopening an access and
 * exposure epoch on the window. (note that it does not actually end an
 * epoch or complete any pending MPI RMA operations).
 *
 * \param[in] win       window object
 * \return MPI_SUCCESS, MPI_ERR_RMA_SYNC
 */


int
MPID_Win_sync(MPID_Win *win)
{
  int mpi_errno = MPI_SUCCESS;
  static char FCNAME[] = "MPID_Win_flush";

  if((win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK) &&
     (win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK_ALL))
     {
      MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                        return mpi_errno, "**rmasync");
     }
  OPA_read_write_barrier();
  return mpi_errno;
}

