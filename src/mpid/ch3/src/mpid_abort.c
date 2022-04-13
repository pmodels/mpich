/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/* FIXME: This routine *or* MPI_Abort should provide abort callbacks,
   similar to the support in MPI_Finalize */

int MPID_Abort(MPIR_Comm * comm, int mpi_errno, int exit_code,
	       const char *error_msg)
{
    int rank;
    char msg[MPI_MAX_ERROR_STRING] = "";
    char error_str[MPI_MAX_ERROR_STRING + 100];

    MPIR_FUNC_ENTER;

    if (error_msg == NULL) {
	/* Create a default error message */
	error_msg = error_str;
	/* FIXME: Do we want the rank of the input communicator here 
	   or the rank of comm world?  The message gives the rank but not the 
	   communicator, so using other than the rank in comm world does not 
	   identify the process, as the message suggests */
	if (comm)
	{
	    rank = comm->rank;
	}
	else
	{
            rank = MPIR_Process.rank;
	}

	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIR_Err_get_string(mpi_errno, msg, MPI_MAX_ERROR_STRING, NULL);
	    /* FIXME: Not internationalized */
	    MPL_snprintf(error_str, sizeof(error_str), "internal ABORT - process %d: %s", rank, msg);
	}
	else
	{
	    /* FIXME: Not internationalized */
	    MPL_snprintf(error_str, sizeof(error_str), "internal ABORT - process %d", rank);
	}
    }
    
#ifdef HAVE_DEBUGGER_SUPPORT
    MPIR_Debugger_set_aborting( error_msg );
#endif

    /* Dumping the error message in MPICH and passing the same
     * message to the PM as well. This might cause duplicate messages,
     * but it is better to have two messages than none. Note that the
     * PM is in a better position to throw the message (e.g., in case
     * where the stdout/stderr pipes from MPICH to the PM are
     * broken), but not all PMs might display respect the message
     * (this problem was noticed with SLURM). */
    MPL_error_printf("%s\n", error_msg);
    fflush(stderr);

    if (MPIR_CVAR_COREDUMP_ON_ABORT) {
        abort();
    }

    MPIR_pmi_abort(exit_code, error_msg);

    /* pmi_abort should not return but if it does, exit here.  If it does,
       add the function exit code before calling the final exit.  */
    MPIR_FUNC_EXIT;
    MPL_exit(exit_code);

    return MPI_ERR_INTERN;
}
