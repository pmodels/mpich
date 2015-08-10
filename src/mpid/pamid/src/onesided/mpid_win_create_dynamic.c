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
  MPIR_Errflag_t errflag = MPIR_ERR_NONE;
  int rc = MPI_SUCCESS;
  MPIDI_Win_info  *winfo;
  MPID_Win     *win;
  int          rank;  

  rc=MPIDI_Win_init(0,1,win_ptr, info, comm_ptr, MPI_WIN_FLAVOR_DYNAMIC, MPI_WIN_UNIFIED);
  win = *win_ptr;

  rank = comm_ptr->rank;
  win->base = MPI_BOTTOM;
  winfo = &win->mpid.info[rank];
  winfo->win = win;

#ifdef __BGQ__
  void *base = NULL;
  size_t length = 0;
  Kernel_MemoryRegion_t memregion;
  void *tmpbuf = MPIU_Malloc(sizeof(int));
  Kernel_CreateMemoryRegion(&memregion, tmpbuf, sizeof(int));
  //Reset base to base VA of local heap
  base = memregion.BaseVa;
  length = memregion.Bytes;
  MPIU_Free(tmpbuf);
  
  if (length != 0)
    {
      size_t length_out = 0;
      pami_result_t rc;
      rc = PAMI_Memregion_create(MPIDI_Context[0], base, length, &length_out, &winfo->memregion);
      MPID_assert(rc == PAMI_SUCCESS);
      MPID_assert(length == length_out);
    }
  win->base = base;
  winfo->base_addr = base;  
#endif

  rc= MPIDI_Win_allgather(0,win_ptr);
  if (rc != MPI_SUCCESS)
      return rc;

  mpi_errno = MPIR_Barrier_impl(comm_ptr, (MPIR_Errflag_t *) &errflag);

  return mpi_errno;
}
