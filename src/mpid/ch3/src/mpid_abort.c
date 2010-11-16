/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME: Who uses/sets MPIDI_DEV_IMPLEMENTS_ABORT? */
#ifdef MPIDI_DEV_IMPLEMENTS_ABORT
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif
static int MPIDI_CH3I_PMI_Abort(int exit_code, const char *error_msg);
#endif

/* FIXME: We should move this into a header file so that we don't
   need the ifdef.  Also, don't use exit (add to coding check) since
   not safe in windows.  To avoid confusion, define a RobustExit? or
   MPIU_Exit? */
#ifdef HAVE_WINDOWS_H
/* exit can hang if libc fflushes output while in/out/err buffers are locked
   (this must be a bug in exit?).  ExitProcess does not hang (what does this
   mean about the state of the locked buffers?). */
#define exit(_e) ExitProcess(_e)
#endif

/* FIXME: This routine *or* MPI_Abort should provide abort callbacks,
   similar to the support in MPI_Finalize */

#undef FUNCNAME
#define FUNCNAME MPID_Abort
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Abort(MPID_Comm * comm, int mpi_errno, int exit_code, 
	       const char *error_msg)
{
    int rank;
    char msg[MPI_MAX_ERROR_STRING] = "";
    char error_str[MPI_MAX_ERROR_STRING + 100];
    MPIDI_STATE_DECL(MPID_STATE_MPID_ABORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_ABORT);

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
	    if (MPIR_Process.comm_world != NULL)
	    {
		rank = MPIR_Process.comm_world->rank;
	    }
	    else
	    {
		rank = -1;
	    }
	}

	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIR_Err_get_string(mpi_errno, msg, MPI_MAX_ERROR_STRING, NULL);
	    /* FIXME: Not internationalized */
	    MPIU_Snprintf(error_str, sizeof(error_str), "internal ABORT - process %d: %s", rank, msg);
	}
	else
	{
	    /* FIXME: Not internationalized */
	    MPIU_Snprintf(error_str, sizeof(error_str), "internal ABORT - process %d", rank);
	}
    }
    
    MPIDU_Ftb_publish_me(MPIDU_FTB_EV_ABORT);
    MPIDU_Ftb_finalize();
    
#ifdef HAVE_DEBUGGER_SUPPORT
    MPIR_DebuggerSetAborting( error_msg );
#endif

    /* FIXME: This should not use an ifelse chain. Either define the function
       by name or set a function pointer */
#ifdef MPIDI_CH3_IMPLEMENTS_ABORT
    MPIDI_CH3_Abort(exit_code, error_msg);
#elif defined(MPIDI_DEV_IMPLEMENTS_ABORT)
    MPIDI_CH3I_PMI_Abort(exit_code, error_msg);
#else
    MPIU_Error_printf("%s\n", error_msg);
    fflush(stderr);
#endif

    /* ch3_abort should not return but if it does, exit here.  If it does,
       add the function exit code before calling the final exit.  */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_ABORT);
    MPIU_Exit(exit_code);
    
    return MPI_ERR_INTERN;
}

#ifdef MPIDI_DEV_IMPLEMENTS_ABORT
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_PMI_Abort
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_PMI_Abort(int exit_code, const char *error_msg)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PMI_ABORT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PMI_ABORT);

    /* Dumping the error message in MPICH2 and passing the same
     * message to the PM as well. This might cause duplicate messages,
     * but it is better to have two messages than none. Note that the
     * PM is in a better position to throw the message (e.g., in case
     * where the stdout/stderr pipes from MPICH2 to the PM are
     * broken), but not all PMs might display respect the message
     * (this problem was noticed with SLURM). */
    MPIU_Error_printf("%s\n", error_msg);
    fflush(stderr);

    /* FIXME: What is the scope for PMI_Abort?  Shouldn't it be one or more
       process groups?  Shouldn't abort of a communicator abort either the
       process groups of the communicator or only the current process?
       Should PMI_Abort have a parameter for which of these two cases to
       perform? */
#ifdef USE_PMI2_API
    PMI2_Abort(TRUE, error_msg);
#else
    PMI_Abort(exit_code, error_msg);
#endif

    /* if abort returns for some reason, exit here */
    exit(exit_code);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PMI_ABORT);
    return MPI_ERR_INTERN;    
}
#endif
