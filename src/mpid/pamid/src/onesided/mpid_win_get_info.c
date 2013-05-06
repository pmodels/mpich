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
 * \file src/onesided/mpid_win_get_info.c
 * \brief ???
 */
#include "mpidi_onesided.h"

/**
 * \brief MPI-PAMI glue for MPI_WIN_GET_INFO function
 *
 * \param[in] win              Window
 * \param[in] info_p_p         Info hint
 * \return MPI_SUCCESS
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_get_info
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPID_Win_get_info(MPID_Win     *win,
                  MPID_Info   **info_p_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* Allocate an empty info object */
    mpi_errno = MPIU_Info_alloc(info_p_p);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
