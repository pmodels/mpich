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
 * \file src/onesided/mpid_win_create.c
 * \brief ???
 */
#include "mpidi_onesided.h"

/**
 * \brief MPI-PAMI glue for MPI_Win_create_dynamic function
 *
 * Create a window object. Allocates a MPID_Win object and initializes it,
 * then allocates the collective info array, initalizes our entry, and
 * performs an Allgather to distribute/collect the rest of the array entries.
 * The function returns a window win without memory attached.
 *
 * Input Parameters:
 * \param[in] info      info argument
 * \param[in] comm      intra-Communicator (handle)
 * \param[out] win_ptr  window object returned by the call (handle)
 * \return MPI_SUCCESS, MPI_ERR_ARG, MPI_ERR_COMM, MPI_ERR_INFO. MPI_ERR_OTHER,
 *         MPI_ERR_SIZE
 */

int
MPID_Win_create_dynamic( MPID_Info  * info,
                  MPID_Comm  * comm_ptr,
                  MPID_Win  ** win_ptr)
{
  int mpi_errno  = MPI_SUCCESS;
  int rc = MPI_SUCCESS;
  static char FCNAME[] = "MPID_Win_allocate_dynamic";
  MPIDI_Win_info  *winfo;
  MPID_Win     *win;
  int          rank,i;  

  rc=MPIDI_Win_init(0,1,win_ptr, info, comm_ptr, MPI_WIN_FLAVOR_DYNAMIC, MPI_WIN_UNIFIED);
  win = *win_ptr;

  win->base = MPI_BOTTOM;
  rank = comm_ptr->rank;
  winfo = &win->mpid.info[rank];
  winfo->win = win;

  rc= MPIDI_Win_allgather(MPI_BOTTOM,0,win_ptr);
  if (rc != MPI_SUCCESS)
      return rc;

  mpi_errno = MPIR_Barrier_impl(comm_ptr, &mpi_errno);

  return mpi_errno;
}
