/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME: This routine needs to be factored into finalize actions per module,
   In addition, we should consider registering callbacks for those actions
   rather than direct routine calls.
 */

#undef FUNCNAME
#define FUNCNAME MPID_Finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_FINALIZE);

    /*
     * Wait for all posted receives to complete.  For now we are not doing 
     * this since it will cause invalid programs to hang.
     * The side effect of not waiting is that posted any source receives 
     * may erroneous blow up.
     *
     * For now, we are placing a warning at the end of MPID_Finalize() to 
     * inform the user if any outstanding posted receives exist.
     */
     /* FIXME: The correct action here is to begin a shutdown protocol
      * that lets other processes know that this process is now
      * in finalize.  
      *
      * Note that only requests that have been freed with MPI_Request_free
      * are valid at this point; other pending receives can be ignored 
      * since a valid program should wait or test for them before entering
      * finalize.  
      * 
      * The easist fix is to allow an MPI_Barrier over comm_world (and 
      * any connected processes in the MPI-2 case).  Once the barrier
      * completes, all processes are in finalize and any remaining 
      * unmatched receives will never be matched (by a correct program; 
      * a program with a send in a separate thread that continues after
      * some thread calls MPI_Finalize is erroneous).
      * 
      * Avoiding the barrier is hard.  Consider this sequence of steps:
      * Send in-finalize message to all connected processes.  Include
      * information on whether there are pending receives.
      *   (Note that a posted receive with any source is a problem)
      *   (If there are many connections, then this may take longer than
      *   the barrier)
      * Allow connection requests from anyone who has not previously
      * connected only if there is an possible outstanding receive; 
      * reject others with a failure (causing the source process to 
      * fail).
      * Respond to an in-finalize message with the number of posted receives
      * remaining.  If both processes have no remaining receives, they 
      * can both close the connection.
      * 
      * Processes with no pending receives and no connections can exit, 
      * calling PMI_Finalize to let the process manager know that they
      * are in a controlled exit.  
      *
      * Processes that still have open connections must then try to contact
      * the remaining processes.
      *
      * August 2010:
      *
      * The barrier has been removed so that finalize won't hang when
      * another processes has died.  This allows processes to finalize
      * and exit while other processes are still executing.  This has
      * the following consequences:
      *
      *  * If a process tries to send a message to a process that has
      *    exited before completing the matching receive, it will now
      *    get an error.  This is an erroneous program.
      *
      *  * If a process finalizes before completing a nonblocking
      *    send, the message might not be sent.  Similarly, if it
      *    finalizes before completing all receives, the sender may
      *    get an error.  These are erroneous programs.
      *
      *  * A process may isend to another process that has already
      *    terminated, then cancel the send.  The program is not
      *    erroneous in this case, but this will result in an error.
      *    This can be fixed by not returning an error until the app
      *    completes the send request.  If the app cancels the
      *    request, we need to to search the pending send queue and
      *    cancel it, in which case an error shouldn't be generated.
      */
    
    /* Re-enabling the close step because many tests are failing
     * without it, particularly under gforker */
#if 1
    /* FIXME: The close actions should use the same code as the other
       connection close code */
    mpi_errno = MPIDI_PG_Close_VCs();
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    /*
     * Wait for all VCs to finish the close protocol
     */
    mpi_errno = MPIDI_CH3U_VC_WaitForClose();
    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
#endif

    /* Note that the CH3I_Progress_finalize call has been removed; the
       CH3_Finalize routine should call it */
    mpi_errno = MPIDI_CH3_Finalize();
    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }

#ifdef MPID_NEEDS_ICOMM_WORLD
    mpi_errno = MPIR_Comm_release_always(MPIR_Process.icomm_world);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif

    mpi_errno = MPIR_Comm_release_always(MPIR_Process.comm_self);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Comm_release_always(MPIR_Process.comm_world);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* Tell the process group code that we're done with the process groups.
       This will notify PMI (with PMI_Finalize) if necessary.  It
       also frees all PG structures, including the PG for COMM_WORLD, whose 
       pointer is also saved in MPIDI_Process.my_pg */
    mpi_errno = MPIDI_PG_Finalize();
    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }

#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
    MPIDI_CH3_FreeParentPort();
#endif

    /* Release any SRbuf pool storage */
    if (MPIDI_CH3U_SRBuf_pool) {
	MPIDI_CH3U_SRBuf_element_t *p, *pNext;
	p = MPIDI_CH3U_SRBuf_pool;
	while (p) {
	    pNext = p->next;
	    MPIU_Free(p);
	    p = pNext;
	}
    }

    MPIDI_RMA_finalize();
    
    MPIU_Free(MPIDI_failed_procs_string);

    MPIDU_Ftb_finalize();

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
