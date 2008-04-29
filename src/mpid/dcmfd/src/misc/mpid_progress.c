/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_progress.c
 * \brief Maintain the state and rules of the MPI progress semantics
 */
#include "mpidimpl.h"

/**
 * \defgroup MPID_PROGRESS MPID Progress engine
 *
 * Maintain the state and rules of the MPI progress semantics
 */


/**
 * \brief A counter to allow the detection of changes to message state.
 *
 * It is theoretically possible to miss an event : if exactly 2^32 (4
 * Billion) events complete in a singal call to
 * DCMF_Messager_advance(), the comparison would still be true.  We
 * assume that this will not happen.
 */
static volatile unsigned _requests;

/**
 * \brief Unused, provided since MPI calls it.
 * \param[in] state Unused
 */
void MPID_Progress_start(MPID_Progress_state * state)
{
}


/**
 * \brief Unused, provided since MPI calls it.
 * \param[in] state Unused
 */
void MPID_Progress_end(MPID_Progress_state * state)
{
}

/**
 * \brief This function blocks until a request completes
 * \param[in] state Unused
 *
 * It does not check what has completed, only that the counter
 * changes.  That happens whenever there is a call to
 * MPID_Progress_signal().  It is therefore important that the ADI
 * layer include a call to MPID_Progress_signal() whenever something
 * occurs that a node might be waiting on.
 *
 */
int MPID_Progress_wait(MPID_Progress_state * state)
{
  int x = _requests;
  while (x == _requests) {
    DCMF_Messager_advance();
    /*  The point of locking and unlocking here is that it allows
     *  other threads to access the MPI routines when we are blocked.
     *  If we do not release the lock we are holding, no other thread
     *  will ever enter MPI--that violates the THREAD_MULTIPLE
     *  requirements.
     */
    MPID_CS_CYCLE();
  }
  return MPI_SUCCESS;
}

/**
 * \brief This function advances the connection manager.
 *
 * It gets called when progress is desired (e.g. MPI_Iprobe), but
 * nobody wants to block for it.
 */
int MPID_Progress_poke()
{
  DCMF_Messager_advance();
  return MPI_SUCCESS;
}

/**
 * \brief The same as MPID_Progress_poke()
 */
int MPID_Progress_test()
{
  DCMF_Messager_advance();
  return MPI_SUCCESS;
}

/**
 * \brief Signal MPID_Progress_wait() that something is done/changed
 *
 * It is therefore important that the ADI layer include a call to
 * MPID_Progress_signal() whenever something occurs that a node might
 * be waiting on.
 */
void MPID_Progress_signal()
{
  _requests++;
}
