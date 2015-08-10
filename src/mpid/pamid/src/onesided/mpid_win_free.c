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
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#ifdef USE_MMAP_SHM
#include <sys/mman.h>
#endif


int MPIDI_SHM_Win_free(MPID_Win **win_ptr)
{
  static char FCNAME[] = "MPID_SHM_Win_free";
  int    rc;
  int mpi_errno = MPI_SUCCESS;

  /* Free shared memory region */
  /* free shm_base_addrs that's only used for shared memory windows */
  if ((*win_ptr)->mpid.shm->allocated) {
    OPA_fetch_and_add_int((OPA_int_t *) &((*win_ptr)->mpid.shm->ctrl->shm_count),-1);
    while((*win_ptr)->mpid.shm->ctrl->shm_count !=0) MPIDI_QUICKSLEEP;
    if ((*win_ptr)->comm_ptr->rank == 0) {
      MPIDI_SHM_MUTEX_DESTROY(*win_ptr);
      }
#ifdef USE_SYSV_SHM
    mpi_errno = shmdt((*win_ptr)->mpid.shm->base_addr);
    if ((*win_ptr)->comm_ptr->rank == 0) {
	rc=shmctl((*win_ptr)->mpid.shm->shm_id,IPC_RMID,NULL);
	MPIU_ERR_CHKANDJUMP((rc == -1), errno,MPI_ERR_RMA_SHARED, "**shmctl");
    }
#elif USE_MMAP_SHM
    munmap ((*win_ptr)->mpid.shm->base_addr, (*win_ptr)->mpid.shm->segment_len);
    if (0 == (*win_ptr)->comm_ptr->rank) shm_unlink ((*win_ptr)->mpid.shm->shm_key);
#else
    MPID_Abort(NULL, MPI_ERR_RMA_SHARED, -1, "MPI_Win_free error");
#endif
  } else {/* one task on a node */
    MPIU_Free((*win_ptr)->mpid.shm->base_addr);
  }
  MPIU_Free((*win_ptr)->mpid.shm);
  (*win_ptr)->mpid.shm = NULL;

 fn_fail:
  return mpi_errno;
}

/**
 * \brief MPI-PAMI glue for MPI_Win_free function
 *
 * Release all references and free memory associated with window.
 *
 * \param[in,out] win  Window
 * \return MPI_SUCCESS or error returned from MPI_Barrier.
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_free
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPID_Win_free(MPID_Win **win_ptr)
{
  int mpi_errno = MPI_SUCCESS;

  MPID_Win *win = *win_ptr;
  size_t rank = win->comm_ptr->rank;
  MPIR_Errflag_t errflag = MPIR_ERR_NONE;

  if(win->mpid.sync.origin_epoch_type != win->mpid.sync.target_epoch_type ||
     (win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_NONE &&
      win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_REFENCE)){
    MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC, return mpi_errno, "**rmasync");
  }

  mpi_errno = MPIR_Barrier_impl(win->comm_ptr, &errflag);
  MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**mpi_bcast");

  if (win->create_flavor == MPI_WIN_FLAVOR_SHARED)
       mpi_errno=MPIDI_SHM_Win_free(win_ptr);



  if (win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE)
    MPIU_Free(win->base);

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
  if (win->mpid.work.msgQ) 
      MPIU_Free(win->mpid.work.msgQ);

  MPIR_Comm_release(win->comm_ptr, 0);

  MPIU_Handle_obj_free(&MPID_Win_mem, win);

fn_fail:
  return mpi_errno;
}
