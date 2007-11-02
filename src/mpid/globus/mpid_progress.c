/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

mpig_pe_count_t mpig_pe_total_ops_count = 0;
int mpig_pe_active_ops_count = 0;
int mpig_pe_active_ras_ops_count = 0;
int mpig_pe_polling_required_count = 0;


/*
 * MPID_Progress_start()
 */
#if !defined (HAVE_MPID_PROGRESS_START_MACRO)
#undef FUNCNAME
#define FUNCNAME MPID_Progress_start
void MPID_Progress_start(MPID_Progress_state * state)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_START);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_START);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));
    
#   error MPID_Progress_start() should be a macro
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_START);
}
/* MPID_Progress_start() */
#endif


/*
 * MPID_Progress_end()
 */
#if !defined (HAVE_MPID_PROGRESS_END_MACRO)
#undef FUNCNAME
#define FUNCNAME MPID_Progress_end
void MPID_Progress_end(MPID_Progress_state * state)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_END);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_END);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

#   error MPID_Progress_end() should be a macro
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_END);
}
/* MPID_Progress_end() */
#endif


/*
 * MPID_Progress_wait()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Progress_wait
int MPID_Progress_wait(MPID_Progress_state * state)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_WAIT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_WAIT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    MPIU_Assert(mpig_pe_active_ops_count > 0 || state->dev.count != mpig_pe_total_ops_count);

    while (TRUE)
    {
	/* NOTE: a CM pe_wait() routine must not block if the total number of active operations is greater than the number of
	   active operations in that module or if an operation has completed since the previous call to MPID_Progress_start or
	   MPID_Progress_wait.  a CM can determine if it safe to block by checking two things.  first, the CM must confirm that
	   it owns all active operations by calling mpig_pe_cm_owns_all_active_ops().  second, the CM must call
	   mpig_pe_op_has_completed() to verify that no operations have completed since the previous call to a MPID_Progress
	   routine. */

#if defined(MPIG_VMPI)
	mpi_errno = mpig_cm_vmpi_pe_wait(state);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_pe_wait", "**globus|cm_pe_wait %s", "VMPI");
#endif

#if defined(HAVE_GLOBUS_XIO_MODULE)
	mpi_errno = mpig_cm_xio_pe_wait(state);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_pe_wait", "**globus|cm_pe_wait %s", "XIO");
#endif
	
        if (mpig_pe_op_has_completed(state)) break;

	/* XXX: should we really be performing a thread yield here if no progress was made.  the primary reason for doing so is
	   to allow the XIO communication threads some CPU time to make progress, which suggests the yield should be performed in
	   the CM XIO module.  However, other threads could theoretically exist which are in need of a timeslice, so the yield
	   has been placed here for now to prevent multiple modules from yielding.  a mechanism may be needed to determine when
	   yielding is really necessary so that performance is not sacrificed unnecessarily. */
	mpig_thread_yield();
    }
    
    state->dev.count = mpig_pe_total_ops_count;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_WAIT);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
/* MPID_Progress_wait() */
}


/*
 * MPID_Progress_test()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Progress_test
int MPID_Progress_test(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_TEST);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_TEST);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

#if defined(MPIG_VMPI)
    mpi_errno = mpig_cm_vmpi_pe_test();
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_pe_test", "**globus|cm_pe_test %s", "VMPI");
#endif

#if defined(HAVE_GLOBUS_XIO_MODULE)
    mpi_errno = mpig_cm_xio_pe_test();
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_pe_test", "**globus|cm_pe_test %s", "XIO");
#endif
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_TEST);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Progress_test() */


/*
 * MPID_Progress_poke()
 */
#if !defined (HAVE_MPID_PROGRESS_POKE_MACRO)
#undef FUNCNAME
#define FUNCNAME MPID_Progress_poke
int MPID_Progress_poke(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_POKE);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_POKE);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

#   error MPID_Progress_poke() should be a macro
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_POKE);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Progress_poke() */
#endif
