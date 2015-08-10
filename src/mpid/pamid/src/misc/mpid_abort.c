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
 * \file src/misc/mpid_abort.c
 * \brief Handle general operations assosicated with erroneous job termination
 */
#include <mpidimpl.h>
#include <mpix.h>

/**
 * \brief The central parts of the MPID_Abort() call
 *
 * \param[in] comm      The communicator associated with the failure (can be null).
 * \param[in] mpi_errno The MPI error associated with the failure (can be zero).
 * \param[in] exit_code The requested exit code.
 * \param[in] error_msg The message to display (may be NULL)
 *
 * This is the majority of the call to MPID_Abort().  The only
 * difference is that it does not call exit.  That allows it to be
 * used as a test function to ensure that the output is what you would
 * expect.
 *
 * MPIDI_Abort_core() simply uses the same params from MPID_Abort().
 */
void MPIDI_Abort_core(MPID_Comm * comm, int mpi_errno, int exit_code, const char *user_str)
{
  char sys_str[MPI_MAX_ERROR_STRING+5] = "";
  char comm_str[MPI_MAX_ERROR_STRING] = "";
  char world_str[MPI_MAX_ERROR_STRING] = "";
  char error_str[2*MPI_MAX_ERROR_STRING + 128];

  if (MPIR_Process.comm_world)
    {
      int rank = MPIR_Process.comm_world->rank;
      snprintf(world_str, sizeof(world_str), " on node %d", rank);
    }
  if (comm)
    {
      int rank   = comm->rank;
      int handle = comm->handle;
      snprintf(comm_str, sizeof(comm_str), " (rank %d in comm %d)", rank, handle);
    }
  if (!user_str)
    user_str = "Internal error";
  if (mpi_errno != MPI_SUCCESS)
    {
      char msg[MPI_MAX_ERROR_STRING] = "";
      MPIR_Err_get_string(mpi_errno, msg, MPI_MAX_ERROR_STRING, NULL);
      snprintf(sys_str, sizeof(msg), " (%s)", msg);
    }

  snprintf(error_str, sizeof(error_str), "Abort(%d)%s%s: %s%s\n", exit_code, world_str, comm_str, user_str, sys_str);
  MPL_error_printf("%s", error_str);

  fflush(stderr);  fflush(stdout);
}

/**
 * \brief The central parts of the MPID_Abort call
 * \param[in] comm      The communicator associated with the failure (can be null).
 * \param[in] mpi_errno The MPI error associated with the failure (can be zero).
 * \param[in] exit_code The requested exit code.
 * \param[in] error_msg The message to display (may be NULL)
 * \return MPI_ERR_INTERN
 *
 * This function MUST NEVER return.
 */
int MPID_Abort(MPID_Comm * comm, int mpi_errno, int exit_code, const char *error_msg)
{
  MPIDI_Abort_core(comm, mpi_errno, exit_code, error_msg);

#ifdef DYNAMIC_TASKING
  extern int mpidi_dynamic_tasking;
  if (mpidi_dynamic_tasking)
      return PMI2_Abort(1,error_msg);
#endif

  /* The POE and BGQ control systems both catch the exit value for additional
   * processing. If a process exits with '1' then all processes in the job
   * are terminated. The requested error code is lost in this process however
   * this is acceptable, but not desirable, behavior according to the MPI
   * standard.
   *
   * On BGQ, the user may force the process (rank) that exited with '1' to core
   * dump by setting the environment variable 'BG_COREDUMPONERROR=1'.
   */
  exit(1);
}
