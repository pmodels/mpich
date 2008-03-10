/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/barrier/mpido_barrier.c
 * \brief ???
 */

#include "mpido_coll.h"

#pragma weak PMPIDO_Barrier = MPIDO_Barrier

/**
 * **************************************************************************
 * \brief "Done" callback for barrier messages.
 * **************************************************************************
 */

static void
cb_done (void *clientdata) {
  volatile unsigned *work_left = (unsigned *) clientdata;
  *work_left = 0;
  MPID_Progress_signal();
  return;
}

static volatile unsigned mpid_globalbarrier_active = 0;// global active field for global barriers
static DCMF_Request_t mpid_globalbarrier_request;
static unsigned mpid_globalbarrier_restart = 0;


/**
 * **************************************************************************
 * \brief General MPIDO_Barrier() implementation
 * **************************************************************************
 */

int MPIDO_Barrier(MPID_Comm *comm_ptr)
{
  volatile unsigned active; // local (thread safe) active field for non-global barriers
  int rc;
  MPID_Comm *comm_world;
  MPID_Comm_get_ptr(MPI_COMM_WORLD, comm_world);
  DCMF_Callback_t callback = { cb_done, (void *) &mpid_globalbarrier_active }; // use global active field by default
  if(comm_ptr == comm_world && MPIDI_CollectiveProtocols.barrier.usegi)
  {
    mpid_globalbarrier_active = 1; // initialize global active field

    if (mpid_globalbarrier_restart)
    {
      rc = DCMF_Restart (&mpid_globalbarrier_request);
    }
    else
    {
      mpid_globalbarrier_restart = 1;
      rc = DCMF_GlobalBarrier(&MPIDI_Protocols.globalbarrier, &mpid_globalbarrier_request, callback);
    }
  }
  else
  {
    callback.clientdata = (void*) &active;  // use local (thread safe) active field
    active = 1;  // initialize local (thread safe) active field

    /* geometry sets up proper barrier for the geometry at init time */
    rc = DCMF_Barrier (&comm_ptr->dcmf.geometry,
                       callback,
                       DCMF_MATCH_CONSISTENCY);
  }

  if (rc == DCMF_SUCCESS)
  {
    MPID_PROGRESS_WAIT_WHILE(*(int*)callback.clientdata); // use local or global active field - whichever was set
  }
  else
  {
    rc = MPIR_Barrier(comm_ptr);
  }

  return rc;
}
