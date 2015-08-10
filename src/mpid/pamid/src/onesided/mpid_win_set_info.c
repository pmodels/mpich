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
 * \file src/onesided/mpid_win_set_info.c
 * \brief sets new values for the hints of the window of the window 
 *        associated with win.                                      
 */
#include "mpidi_onesided.h"

/**
 * \brief MPI-PAMI glue for MPI_WIN_SET_INFO function
 *
 * \param[in] win              Window
 * \param[in] info             Info hint
 * \return MPI_SUCCESS
 */

int MPIDI_Win_set_info(MPID_Win *win, MPID_Info *info)
{

    int mpi_errno = MPI_SUCCESS;
    MPID_Info *curr_ptr;
    char *value, *token;
    char *savePtr;
    curr_ptr = info->next;
    uint    save_ordering;

    while (curr_ptr) {
        if (!strcmp(curr_ptr->key,"no_locks")) {
            if (!strcmp(curr_ptr->value,"true")) {
                win->mpid.info_args.no_locks=1;
            } else 
                win->mpid.info_args.no_locks=0;
        } else if (!strcmp(curr_ptr->key,"accumulate_ordering"))
        {
              save_ordering=(uint) win->mpid.info_args.accumulate_ordering;
              win->mpid.info_args.accumulate_ordering=0;
              value = curr_ptr->value;
              token = (char *) strtok_r(value,"," , &savePtr);
              while (token) {
                if (!memcmp(token,"rar",3)) 
                {
                     win->mpid.info_args.accumulate_ordering =
                     (win->mpid.info_args.accumulate_ordering | MPIDI_ACCU_ORDER_RAR);
                 } else if (!memcmp(token,"raw",3)) 
                 {
                     win->mpid.info_args.accumulate_ordering =
                     (win->mpid.info_args.accumulate_ordering | MPIDI_ACCU_ORDER_RAW);
                  } else if (!memcmp(token,"war",3))
                  {
                     win->mpid.info_args.accumulate_ordering =
                     (win->mpid.info_args.accumulate_ordering | MPIDI_ACCU_ORDER_WAR);
                  } else if (!memcmp(token,"waw",3))
                  {
                     win->mpid.info_args.accumulate_ordering =
                     (win->mpid.info_args.accumulate_ordering | MPIDI_ACCU_ORDER_WAW);
                  } else
                      MPID_assert_always(0);
                  token = (char *) strtok_r(NULL,"," , &savePtr);
               }
               if (win->mpid.info_args.accumulate_ordering == 0) {
                   win->mpid.info_args.accumulate_ordering=
                      (MPIDI_Win_info_accumulate_ordering) save_ordering;
               }
        } else if (!strcmp(curr_ptr->key,"accumulate_ops"))
         {
              /* the default setting is MPIDI_ACCU_SAME_OP_NO_OP */
              if (!strcmp(curr_ptr->value,"same_op"))
                   win->mpid.info_args.accumulate_ops = MPIDI_ACCU_SAME_OP;
         }
        curr_ptr = curr_ptr->next;
    }

    return mpi_errno;
}

int
MPID_Win_set_info(MPID_Win     *win, MPID_Info    *info)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    mpi_errno = MPIDI_Win_set_info(win, info);
    MPID_assert(mpi_errno == MPI_SUCCESS);
    mpi_errno = MPIR_Barrier_impl(win->comm_ptr, &errflag);
    return mpi_errno;
}
