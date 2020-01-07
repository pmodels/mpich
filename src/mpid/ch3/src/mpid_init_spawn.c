/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Init_spawn(void)
{
    int mpi_errno = MPI_SUCCESS;
    char * parent_port;


#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS

    /* FIXME: To allow just the "root" process to
       request the port and then use MPIR_Bcast_intra_auto to
       distribute it to the rest of the processes,
       we need to perform the Bcast after MPI is
       otherwise initialized.  We could do this
       by adding another MPID call that the MPI_Init(_thread)
       routine would make after the rest of MPI is
       initialized, but before MPI_Init returns.
       In fact, such a routine could be used to
       perform various checks, including parameter
       consistency value (e.g., all processes have the
       same environment variable values). Alternately,
       we could allow a few routines to operate with
       predefined parameter choices (e.g., bcast, allreduce)
       for the purposes of initialization. */
    mpi_errno = MPIDI_CH3_GetParentPort(&parent_port);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
                            "**ch3|get_parent_port");
    }
    MPL_DBG_MSG_S(MPIDI_CH3_DBG_CONNECT,VERBOSE,"Parent port is %s", parent_port);

    mpi_errno = MPID_Comm_connect(parent_port, NULL, 0, MPIR_Process.comm_world,
                                  &MPIR_Process.comm_parent);
    MPIR_ERR_CHKANDJUMP1(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**ch3|conn_parent",
                         "**ch3|conn_parent %s", parent_port);

    MPIR_Assert(MPIR_Process.comm_parent != NULL);
    MPL_strncpy(MPIR_Process.comm_parent->name, "MPI_COMM_PARENT", MPI_MAX_OBJECT_NAME);

    /* FIXME: Check that this intercommunicator gets freed in MPI_Finalize
       if not already freed.  */
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
