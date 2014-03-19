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
 * \brief returns a new info object containing the hints of the window
 *        associated with win.                                          
 */
#include "mpidi_onesided.h"
#include "mpiinfo.h"

/**
 * \brief MPI-PAMI glue for MPI_WIN_GET_INFO function
 *
 * \param[in] win              Window
 * \param[in] info_p_p         Info hint
 * \return MPI_SUCCESS
 */
int MPIDI_Win_get_info(MPID_Win *win, MPID_Info **info_used) 
{
    int mpi_errno = MPI_SUCCESS;

    
    /* Populate the predefined info keys */
    if (win->mpid.info_args.no_locks)
        mpi_errno = MPIR_Info_set_impl(*info_used, "no_locks", "true");
    else
        mpi_errno = MPIR_Info_set_impl(*info_used, "no_locks", "false");
    MPID_assert(mpi_errno == MPI_SUCCESS);

    {
#define BUFSIZE 32
        char buf[BUFSIZE];
        int c = 0;
        if (win->mpid.info_args.accumulate_ordering & MPIDI_ACCU_ORDER_RAR)  
            c += snprintf(buf+c, BUFSIZE-c, "%srar", (c > 0) ? "," : "");
        if (win->mpid.info_args.accumulate_ordering & MPIDI_ACCU_ORDER_RAW)
            c += snprintf(buf+c, BUFSIZE-c, "%sraw", (c > 0) ? "," : "");
        if (win->mpid.info_args.accumulate_ordering & MPIDI_ACCU_ORDER_WAR)
            c += snprintf(buf+c, BUFSIZE-c, "%swar", (c > 0) ? "," : "");
        if (win->mpid.info_args.accumulate_ordering & MPIDI_ACCU_ORDER_WAW)
            c += snprintf(buf+c, BUFSIZE-c, "%swaw", (c > 0) ? "," : "");

        if (c == 0) {
            memcpy(&buf[0],"not set   ",10); 
        }
        MPIR_Info_set_impl(*info_used, "accumulate_ordering", buf);
        MPID_assert(mpi_errno == MPI_SUCCESS);
#undef BUFSIZE
    }
    if (win->mpid.info_args.accumulate_ops == (unsigned) MPIDI_ACCU_OPS_SAME_OP)
        mpi_errno = MPIR_Info_set_impl(*info_used, "accumulate_ops", "same_op");
    else
        mpi_errno = MPIR_Info_set_impl(*info_used, "accumulate_ops", "same_op_no_op");

    MPID_assert(mpi_errno == MPI_SUCCESS);

    if (win->create_flavor == MPI_WIN_FLAVOR_SHARED) {
        if (win->mpid.info_args.alloc_shared_noncontig)
            mpi_errno = MPIR_Info_set_impl(*info_used, "alloc_shared_noncontig", "true");
        else
            mpi_errno = MPIR_Info_set_impl(*info_used, "alloc_shared_noncontig", "false");

        MPID_assert(mpi_errno == MPI_SUCCESS);
    }
    else if (win->create_flavor == MPI_WIN_FLAVOR_ALLOCATE) {
        if (win->mpid.info_args.same_size)
            mpi_errno = MPIR_Info_set_impl(*info_used, "same_size", "true");
        else
            mpi_errno = MPIR_Info_set_impl(*info_used, "same_size", "false");

        MPID_assert(mpi_errno == MPI_SUCCESS);
    }
    return mpi_errno;
}


int
MPID_Win_get_info(MPID_Win     *win,
                  MPID_Info   **info_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* Allocate an empty info object */
    mpi_errno = MPIU_Info_alloc(info_p);
    MPID_assert(mpi_errno == MPI_SUCCESS);
    mpi_errno = MPIDI_Win_get_info(win, info_p);
    MPID_assert(mpi_errno == MPI_SUCCESS);
    return MPI_SUCCESS;
}
