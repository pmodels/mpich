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
 * \brief MPI-PAMI glue for MPI_Win_allocate function
 *
 * Create a window object. Allocates a MPID_Win object and initializes it,
 * then allocates the collective info array, initalizes our entry, and
 * performs an Allgather to distribute/collect the rest of the array entries.
 * On each process, it allocates memory of at least size bytes, returns a
 * pointer to it, and returns a window object that can be used by all processes
 * in comm to * perform RMA operations. The returned memory consists of size
 * bytes local to each process, starting at address base_ptr and is associated
 * with the window as if the user called 'MPI_Win_create' on existing memory.
 * The size argument may be different at each process and size = 0 is valid;
 * however, a library might allocate and expose more memory in order to create
 * a fast, globally symmetric allocation.
 * Input Parameters:
 * \param[in] size      size of window in bytes (nonnegative integer)
 * \param[in] disp_unit local unit size for displacements, in bytes (positive integer)
 * \param[in] info      info argument (handle))
 * \param[in] comm_ptr  Communicator (handle)
 * \param[out] base_ptr - base address of the window in local memory
 * \param[out] win_ptr  window object returned by the call (handle)
 * \return MPI_SUCCESS, MPI_ERR_ARG, MPI_ERR_COMM, MPI_ERR_INFO. MPI_ERR_OTHER,
 *         MPI_ERR_SIZE
 */
int
MPID_Win_allocate(MPI_Aint     size,
                  int          disp_unit,
                  MPID_Info  * info,
                  MPID_Comm  * comm_ptr,
                  void *base_ptr,
                  MPID_Win  ** win_ptr)
{
  int mpi_errno  = MPI_SUCCESS;
  int rc = MPI_SUCCESS;
  MPIR_Errflag_t errflag = MPIR_ERR_NONE;
  void *baseP; 
  static char FCNAME[] = "MPID_Win_allocate";
  MPIDI_Win_info  *winfo;
  MPID_Win   *win;
  int        rank;

  rc=MPIDI_Win_init(size,disp_unit,win_ptr, info, comm_ptr, MPI_WIN_FLAVOR_ALLOCATE, MPI_WIN_UNIFIED);
  win = *win_ptr;

  if (size > 0) {
      baseP = MPIU_Malloc(size);
  #ifndef MPIDI_NO_ASSERT
      MPID_assert(baseP != NULL);
  #else
      MPIU_ERR_CHKANDJUMP((baseP == NULL), mpi_errno, MPI_ERR_BUFFER, "**bufnull");
  #endif

  } else if (size == 0) {
      baseP = NULL;
  } else {
      MPIU_ERR_CHKANDSTMT(size >=0 , mpi_errno, MPI_ERR_SIZE,
                          return mpi_errno, "**rmasize");
  }

  win->base = baseP;
  rank = comm_ptr->rank;
  winfo = &win->mpid.info[rank];
  winfo->base_addr = baseP;
  winfo->win = win;
  winfo->disp_unit = disp_unit;

  rc= MPIDI_Win_allgather(size,win_ptr);
  if (rc != MPI_SUCCESS)
      return rc;
  *(void**) base_ptr = (void *) win->base;
  mpi_errno = MPIR_Barrier_impl(comm_ptr, &errflag);

  fn_fail:
  return mpi_errno;
}

