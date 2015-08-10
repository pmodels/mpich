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

/***************************************************************************/
/*                                                                         */
/* allocate win_ptr (MPIDI_Win)                                            */
/* update win structure except for base address                            */
/*                                                                         */
/***************************************************************************/

int
MPIDI_Win_init( MPI_Aint length,
                int disp_unit,
                MPID_Win  **win_ptr,
                MPID_Info  *info,
                MPID_Comm *comm_ptr,
                int create_flavor,
                int model)
{
  int mpi_errno=MPI_SUCCESS;
  size_t rank, size;
  MPIDI_Win_info *winfo;
  static char FCNAME[] = "MPIDI_Win_init";

  /* ----------------------------------------- */
  /*  Setup the common sections of the window  */
  /* ----------------------------------------- */
  MPID_Win *win = (MPID_Win*)MPIU_Handle_obj_alloc(&MPID_Win_mem);

  MPIU_ERR_CHKANDSTMT(win == NULL, mpi_errno, MPI_ERR_NO_MEM,
                     return mpi_errno, "**nomem");

  *win_ptr = win;
  memset(&win->mpid, 0, sizeof(struct MPIDI_Win));
  win->comm_ptr = comm_ptr; MPIR_Comm_add_ref(comm_ptr);
  size = comm_ptr->local_size;
  rank = comm_ptr->rank;

  win->mpid.info = MPIU_Malloc(size * sizeof(struct MPIDI_Win_info));
  MPID_assert(win->mpid.info != NULL);
  memset((void *) win->mpid.info,0,(size * sizeof(struct MPIDI_Win_info)));
  winfo = &win->mpid.info[rank];
  win->errhandler          = NULL;
  win->base                = NULL;
  win->size                = length;
  win->disp_unit           = disp_unit;
  win->create_flavor       = create_flavor;
  win->model               = model;
  win->copyCreateFlavor    = 0;
  win->copyModel           = 0;
  win->attributes          = NULL;
  win->comm_ptr            = comm_ptr;
  if ((info != NULL) && ((int *)info != (int *) MPI_INFO_NULL)) {
      mpi_errno= MPIDI_Win_set_info(win, info);
      MPID_assert(mpi_errno == 0);
  }
  MPID_assert(mpi_errno == 0);


    /* Initialize the info (hint) flags per window */
  win->mpid.info_args.no_locks            = 0;
  win->mpid.info_args.accumulate_ordering =
      (MPIDI_ACCU_ORDER_RAR | MPIDI_ACCU_ORDER_RAW | MPIDI_ACCU_ORDER_WAR | MPIDI_ACCU_ORDER_WAW);
  win->mpid.info_args.accumulate_ops      = MPIDI_ACCU_SAME_OP_NO_OP; /*default */
  win->mpid.info_args.same_size           = 0;
  win->mpid.info_args.alloc_shared_noncontig = 0;

  win->copyDispUnit=0;
  win->copySize=0;
  winfo->memregion_used = 0;
  winfo->disp_unit = disp_unit;

  return mpi_errno;
}

/***************************************************************************/
/*                                                                         */
/* MPIDI_Win_allgather                                                     */
/*                                                                         */
/* registers memory with PAMI if possible                                  */
/* calls Allgather to gather the information from all members in win.      */
/*                                                                         */
/***************************************************************************/
int
MPIDI_Win_allgather( MPI_Aint size, MPID_Win **win_ptr )
{
  int mpi_errno = MPI_SUCCESS;
  MPIR_Errflag_t errflag=MPIR_ERR_NONE;
  MPID_Win *win;
  int rank;
  MPID_Comm *comm_ptr;
  size_t length_out = 0;
  pami_result_t rc;
  MPIDI_Win_info  *winfo;
  static char FCNAME[] = "MPIDI_Win_allgather";

  win = *win_ptr;
  comm_ptr = win->comm_ptr;
  rank = comm_ptr->rank;
  winfo = &win->mpid.info[rank];

  if (size != 0 && win->create_flavor != MPI_WIN_FLAVOR_SHARED)
    {
#ifndef USE_PAMI_RDMA
      if (!MPIDI_Process.mp_s_use_pami_get)
        {
#endif
          /* --------------------------------------- */
          /*  Setup the PAMI sections of the window  */
          /* --------------------------------------- */
          rc = PAMI_Memregion_create(MPIDI_Context[0], win->mpid.info[rank].base_addr, win->size, &length_out, &winfo->memregion);
#ifdef USE_PAMI_RDMA
          MPIU_ERR_CHKANDJUMP((rc != PAMI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**nomem");
          MPIU_ERR_CHKANDJUMP((win->size < length_out), mpi_errno, MPI_ERR_OTHER, "**nomem");
#else
          if (rc == PAMI_SUCCESS)
            {
              winfo->memregion_used = 1;
              MPID_assert(win->size == length_out);
            }
        }
#endif
    }

  mpi_errno = MPIR_Allgather_impl(MPI_IN_PLACE,
                                  0,
                                  MPI_DATATYPE_NULL,
                                  win->mpid.info,
                                  sizeof(struct MPIDI_Win_info),
                                  MPI_BYTE,
                                  comm_ptr,
                                  &errflag);

fn_fail:
   return mpi_errno;
}



/**
 * \brief MPI-PAMI glue for MPI_Win_create function
 *
 * Create a window object. Allocates a MPID_Win object and initializes it,
 * then allocates the collective info array, initalizes our entry, and
 * performs an Allgather to distribute/collect the rest of the array entries.
 *
 * ON first call, initializes (registers) protocol objects for locking,
 * get, and send operations to message layer. Also creates datatype to
 * represent the rma_sends element of the collective info array,
 * used later to synchronize epoch end events.
 *
 * \param[in] base	Local window buffer
 * \param[in] size	Local window size
 * \param[in] disp_unit	Displacement unit size
 * \param[in] info	Window hints (not used)
 * \param[in] comm_ptr	Communicator
 * \param[out] win_ptr	Window
 * \return MPI_SUCCESS, MPI_ERR_OTHER, or error returned from
 *	MPI_Comm_dup or MPI_Allgather.
 */
int
MPID_Win_create(void       * base,
                MPI_Aint     size,
                int          disp_unit,
                MPID_Info  * info,
                MPID_Comm  * comm_ptr,
                MPID_Win  ** win_ptr)
{
  int mpi_errno  = MPI_SUCCESS;
  MPIR_Errflag_t errflag = MPIR_ERR_NONE;
  int rc  = MPI_SUCCESS;
  MPID_Win *win;
  size_t  rank;
  MPIDI_Win_info *winfo;

  rc=MPIDI_Win_init(size,disp_unit,win_ptr, info, comm_ptr, MPI_WIN_FLAVOR_CREATE, MPI_WIN_UNIFIED);
  win = *win_ptr;
  win->base = base;
  rank = comm_ptr->rank;
  winfo = &win->mpid.info[rank];
  winfo->base_addr = base;
  winfo->win = win;
  winfo->disp_unit = disp_unit;

  rc= MPIDI_Win_allgather(size,win_ptr);
  if (rc != MPI_SUCCESS)
      return rc;


  mpi_errno = MPIR_Barrier_impl(comm_ptr, (MPIR_Errflag_t *) &errflag);

  return mpi_errno;
}
