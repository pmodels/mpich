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
 * \file src/onesided/mpid_win_shared_query.c
 * \brief queries the process-local address for remote memory segments
 *        created with MPI_Win_allocate_shared.                       
 */
#include "mpidi_onesided.h"

/**
 * \brief MPI-PAMI glue for MPI_Win_shared_query function
 * 
 * Query the size and base pointer for a patch of a shared memory window
 * 
 * \param[in]  win       shared memory window object
 * \param[in]  rank      rank in the group of window win or MPI_PROC_NULL
 * \param[out] size      size of the window segment (non-negative integer)
 * \param[out] disp_unit local unit size for displacements, in bytes  
 * \param[out] base_ptr   address for load/store access to window segment
 * \return MPI_SUCCESS, MPI_ERR_OTHER, or error returned from
 */

int
MPID_Win_shared_query(MPID_Win *win, int rank, MPI_Aint *size,
                           int *disp_unit, void *base_ptr)
{
    int i, mpi_errno = MPI_SUCCESS;
    static char FCNAME[] = "MPID_Win_shared_query";
    MPIU_ERR_CHKANDSTMT((win->create_flavor != MPI_WIN_FLAVOR_SHARED), mpi_errno,
                         MPI_ERR_RMA_FLAVOR, return mpi_errno, "**rmaflavor");

    if (rank == MPI_PROC_NULL) {
      for (i=0; i<win->comm_ptr->local_size; ++i) {
          if (win->mpid.info[i].base_size != 0) {
              *size                 = win->mpid.info[i].base_size;
              *disp_unit            = win->mpid.info[i].disp_unit;
              break;
          }
      }
      *((void **) base_ptr) = win->base; /* return the begin. address of shared segment */
    } else {
        *((void **) base_ptr) = win->mpid.info[rank].base_addr; /* return the begin. address of the rank */
        *size                 = win->mpid.info[rank].base_size;
        *disp_unit            = win->mpid.info[rank].disp_unit;
    }


    return mpi_errno;
}

