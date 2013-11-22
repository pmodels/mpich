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
 * \file src/onesided/mpid_win_detach.c
 * \brief detaches a previously attached memory region beginning at    
 *        base 
 */
#include "mpidi_onesided.h"

/**
 * \brief MPI-PAMI glue for MPI_Win_detach function
 *
 * Detaches a previously attached memory beginning at base.
 * The arguments base and win must match the arguments passed
 * to a previous call to MPI_Win_attach.
 * \param[in]  win       window object
 * \param[in]  base      initial address of emmory to be detached
 * \return MPI_SUCCESS, MPI_ERR_RMA_FLAVOR
 *
 */

int
MPID_Win_detach(MPID_Win *win, const void *base)
{
    int mpi_errno = MPI_SUCCESS;
    static char FCNAME[] = "MPID_Win_detach";
    MPIU_ERR_CHKANDSTMT((win->create_flavor != MPI_WIN_FLAVOR_DYNAMIC), mpi_errno,
                         MPI_ERR_RMA_FLAVOR, return mpi_errno, "**rmaflavor");


    /* no op, all memory is exposed           */

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

