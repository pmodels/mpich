/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_iprobe.c
 * \brief ???
 */
#include "mpidimpl.h"

static inline int
MPID_Iprobe_rsm(int source,
                int tag,
                MPID_Comm * comm,
                int context_offset,
                int *flag,
                MPI_Status * status)
{
  MPID_Request * rreq;
  const int context = comm->recvcontext_id + context_offset;

  if (source == MPI_PROC_NULL)
    {
      MPIR_Status_set_procnull(status);
      /* We set the flag to true because an MPI_Recv with this rank will
       * return immediately */
      *flag = TRUE;
      return MPI_SUCCESS;
    }
  rreq = MPIDI_Recvq_FU(source, tag, context);
  if (rreq != NULL)
    {
      if (status != MPI_STATUS_IGNORE) *status = rreq->status;
      MPID_Request_release(rreq);
      *flag = TRUE;
      return MPI_SUCCESS;
    }
  else
    {
      MPID_Progress_poke();
      *flag = FALSE;
    }
  return MPI_SUCCESS;
}


static inline int
MPID_Iprobe_ssm(int source,
                int tag,
                MPID_Comm * comm,
                int context_offset,
                int *flag,
                MPI_Status * status)
{
  return SSM_ABORT();
}


int
MPID_Iprobe(int source,
            int tag,
            MPID_Comm * comm,
            int context_offset,
            int *flag,
            MPI_Status * status)
{
  if (MPIDI_Process.use_ssm)
    return MPID_Iprobe_ssm(source, tag, comm, context_offset, flag, status);
  else
    return MPID_Iprobe_rsm(source, tag, comm, context_offset, flag, status);
}
