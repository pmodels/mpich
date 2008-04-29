/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_abort.c
 * \brief Handle general operations assosicated with erroneous job termination
 */
#include "mpidimpl.h"

extern int backtrace(void **buffer, int size); /**< GlibC backtrace support */
extern char **backtrace_symbols(void *const *buffer, int size); /**< GlibC backtrace support */

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
    MPID_Dump_stacks();

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
 * This function SHOULD NEVER return.
 */
int MPID_Abort(MPID_Comm * comm, int mpi_errno, int exit_code, const char *error_msg)
{
  MPID_Abort_core(comm, mpi_errno, exit_code, error_msg);
  abort();
  return MPI_ERR_INTERN;
}


/**
 * \brief Print the current system stack
 *
 * The first frame (this function) is discarded to make the trace look nicer.
 */
void MPID_Dump_stacks()
{
  void *array[32];
  size_t i;
  size_t size    = backtrace(array, 32);
  char **strings = backtrace_symbols(array, size);
  fprintf(stderr, "Dumping %zd frames:\n", size - 1);
  for (i = 1; i < size; i++)
    {
      if (strings != NULL)
        fprintf(stderr, "\tFrame %d: %p: %s\n", i, array[i], strings[i]);
      else
        fprintf(stderr, "\tFrame %d: %p\n", i, array[i]);
    }

  free(strings); /* Since this is not allocated by MPIU_Malloc, do not use MPIU_Free */
}
