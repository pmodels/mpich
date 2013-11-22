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
 * \file src/onesided/mpid_win_attach.c
 * \brief attaches a local memory region beginning at base for remote 
 *        access within the given window.
 */
#include "mpidi_onesided.h"

/**
 * \brief MPI-PAMI glue for MPI_Win_attach function
 *
 * Attaches a local memory region beginning at base for remote access
 * within the given window.
 *
 * \param[in]  win       shared memory window object
 * \param[in]  base      initial address of memory to be attached
 * \param[in]  length       length of memory to be attached in bytes
 * \return MPI_SUCCESS, MPI_ERR_RMA_FLAVOR
 *
 */

int
MPID_Win_attach(MPID_Win *win, void *base, MPI_Aint size)
{
    int mpi_errno = MPI_SUCCESS;
    static char FCNAME[] = "MPID_Win_attach";
    MPIU_ERR_CHKANDSTMT((win->create_flavor != MPI_WIN_FLAVOR_DYNAMIC), mpi_errno, 
                         MPI_ERR_RMA_FLAVOR, return mpi_errno, "**rmaflavor");


    /* no op, all memory is exposed, the user is responsible for */
    /* ensuring that MPI_WIN_ATTACH at the target has returned   */
    /* before a process attempts to target that memory with an   */
    /* RMA call                                                  */

    
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

