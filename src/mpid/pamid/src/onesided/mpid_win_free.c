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
 * \file src/onesided/mpid_win_free.c
 * \brief ???
 */
#include "mpidi_onesided.h"


/**
 * \brief MPI-PAMI glue for MPI_Win_free function
 *
 * Release all references and free memory associated with window.
 *
 * \param[in,out] win  Window
 * \return MPI_SUCCESS or error returned from MPI_Barrier.
 */
int
MPID_Win_free(MPID_Win **win_ptr)
{
  int mpi_errno = MPI_SUCCESS;

  MPID_Win *win = *win_ptr;
  size_t rank = win->comm_ptr->rank;

  mpi_errno = MPIR_Barrier_impl(win->comm_ptr, &mpi_errno);
  if (mpi_errno != MPI_SUCCESS)
    return mpi_errno;

  struct MPIDI_Win_info *winfo = &win->mpid.info[rank];
#ifdef USE_PAMI_RDMA
  if (win->size != 0)
    {
      pami_result_t rc;
      rc = PAMI_Memregion_destroy(MPIDI_Context[0], &winfo->memregion);
      MPID_assert(rc == PAMI_SUCCESS);
    }
#else
  if ( (!MPIDI_Process.mp_s_use_pami_get) && (win->size != 0) && (winfo->memregion_used) )
    {
      pami_result_t rc;
      rc = PAMI_Memregion_destroy(MPIDI_Context[0], &winfo->memregion);
      MPID_assert(rc == PAMI_SUCCESS);
    }
#endif

  MPIU_Free(win->mpid.info);

  MPIR_Comm_release(win->comm_ptr, 0);

  MPIU_Handle_obj_free(&MPID_Win_mem, win);

  return mpi_errno;
}
