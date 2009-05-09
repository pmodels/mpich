/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_abort.c
 * \brief Handle general operations assosicated with erroneous job termination
 */
#include "mpidimpl.h"
#include "mpix.h"

/**
 * \brief The central parts of the MPID_Abort() call
 *
 * \param[in] comm      The communicator associated with the failure (can be null).
 * \param[in] mpi_errno The MPI error associated with the failure (can be zero).
 * \param[in] exit_code The requested exit code, however BG features imply that exit(1) will always be used.
 * \param[in] error_msg The message to display (may be NULL_
 *
 * This is the majority of the call to MPID_Abort().  The only
 * difference is that it does not call exit.  That allows it to be
 * used as a test function to ensure that the output is what you would
 * expect.
 *
 * MPID_Abort_core() simply uses the same params from MPID_Abort().
 */
void MPID_Abort_core(MPID_Comm * comm, int mpi_errno, int exit_code, const char *user_str)
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
  MPIU_Error_printf("%s", error_str);

  if (MPIDI_Process.verbose)
    MPIX_Dump_stacks();

  fflush(stderr);  fflush(stdout);
}

/**
 * \brief The central parts of the MPID_Abort call
 * \param[in] comm      The communicator associated with the failure (can be null).
 * \param[in] mpi_errno The MPI error associated with the failure (can be zero).
 * \param[in] exit_code The requested exit code, however BG features imply that exit(1) will always be used.
 * \param[in] error_msg The message to display (may be NULL_
 * \returns MPI_ERR_INTERN
 *
 * This function MUST NEVER return.
 */
int MPID_Abort(MPID_Comm * comm, int mpi_errno, int exit_code, const char *error_msg)
{
  char* env     = getenv("DCMF_COREONMPIDABORT");
  if (!env) env = getenv("DCMF_COREONMPIABORT");
  if (!env) env = getenv("DCMF_COREONABORT");
  MPID_Abort_core(comm, mpi_errno, exit_code, error_msg);

  if (env != NULL)
    if ( (strncasecmp("no", env, 2)==0) || (strncasecmp("exit", env, 4)==0) || (strncmp("0", env, 1)==0) )
      exit(1);

  abort();
}
