/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_probe.c
 * \brief ???
 */
#include "mpidimpl.h"

static inline int
MPID_Probe_rsm(int source,
               int tag,
               MPID_Comm * comm,
               int context_offset,
               MPI_Status * status)
{
  int found;
  MPID_Progress_state state;
  const int context = comm->recvcontext_id + context_offset;

  if (source == MPI_PROC_NULL)
    {
        MPIR_Status_set_procnull(status);
        return MPI_SUCCESS;
    }
  for(;;)
    {
      found = MPIDI_Recvq_FU(source, tag, context, status);
      if (found)
        return MPI_SUCCESS;
      MPID_Progress_wait(&state);
    }
  return MPI_SUCCESS;
}


static inline int
MPID_Probe_ssm(int source,
               int tag,
               MPID_Comm * comm,
               int context_offset,
               MPI_Status * status)
{
  return SSM_ABORT();
}


int
MPID_Probe(int source,
            int tag,
            MPID_Comm * comm,
            int context_offset,
            MPI_Status * status)
{
  if (MPIDI_Process.use_ssm)
    return MPID_Probe_ssm(source, tag, comm, context_offset, status);
  else
    return MPID_Probe_rsm(source, tag, comm, context_offset, status);
}
