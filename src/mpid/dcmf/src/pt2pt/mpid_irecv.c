/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpid_irecv.c
 * \brief ADI level implemenation of MPI_Irecv()
 */
#include "mpidimpl.h"

/**
 * \brief ADI level implemenation of MPI_Irecv()
 *
 * \param[in]  buf            The buffer to receive into
 * \param[in]  count          Number of expected elements in the buffer
 * \param[in]  datatype       The datatype of each element
 * \param[in]  rank           The sending rank
 * \param[in]  tag            The message tag
 * \param[in]  comm           Pointer to the communicator
 * \param[in]  context_offset Offset from the communicator context ID
 *
 * \param[out] request        Return a pointer to the new request object
 *
 * \returns An MPI Error code
 */
int MPID_Irecv(void          * buf,
               int             count,
               MPI_Datatype    datatype,
               int             rank,
               int             tag,
               MPID_Comm     * comm,
               int             context_offset,

               MPID_Request ** request)
{
  int mpi_errno = MPI_SUCCESS;
  int found;
  MPID_Request * rreq;

  /* ---------------------------------------- */
  /* NULL rank means empty request            */
  /* ---------------------------------------- */
  if (rank == MPI_PROC_NULL)
    {
      rreq = MPID_Request_create();
      if (!rreq)
        return MPIR_Err_create_code(MPI_SUCCESS,
                                    MPIR_ERR_FATAL,
                                    "MPID_Irecv",
                                    __LINE__,
                                    MPI_ERR_OTHER,
                                    "**nomem",
                                    0);
      MPIU_Object_set_ref(rreq, 1);
      rreq->cc               = 0;
      rreq->kind             = MPID_REQUEST_RECV;
      MPIR_Status_set_procnull(&rreq->status);
      rreq->comm             = comm;
      MPIR_Comm_add_ref(comm);
      MPID_Request_setMatch(rreq, tag, rank, comm->recvcontext_id+context_offset);
      rreq->dcmf.userbuf      = buf;
      rreq->dcmf.userbufcount = count;
      rreq->dcmf.datatype     = datatype;
      *request = rreq;
      return MPI_SUCCESS;
    }

  /* ---------------------------------------- */
  /* find our request in the unexpected queue */
  /* or allocate one in the posted queue      */
  /* ---------------------------------------- */
  rreq = MPIDI_Recvq_FDU_or_AEP(rank,
                                tag,
                                comm->recvcontext_id + context_offset,
                                &found);

  if (rreq == NULL)
    {
      *request = rreq;
      mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                       MPIR_ERR_FATAL,
                                       "MPID_Irecv",
                                       __LINE__,
                                       MPI_ERR_NO_MEM,
                                       "**nomem",
                                       0);
      return mpi_errno;
    }

  /* ----------------------------------------------------------------- */
  /* populate request with our data                                    */
  /* We can do this because this is not a multithreaded implementation */
  /* ----------------------------------------------------------------- */

  rreq->comm              = comm;     MPIR_Comm_add_ref (comm);
  rreq->dcmf.userbuf      = buf;
  rreq->dcmf.userbufcount = count;
  rreq->dcmf.datatype     = datatype;
  rreq->dcmf.ca           = MPIDI_DCMF_CA_COMPLETE;

  if (found)
    {
      /* ------------------------------------------------------------ */
      /* message was found in unexpected queue                        */
      /* ------------------------------------------------------------ */
      /* We must acknowledge synchronous send requests                */
      /* The recvnew callback will acknowledge the posted messages    */
      /* Recv functions will ack the messages that are unexpected     */
      /* ------------------------------------------------------------ */

      if (MPID_Request_isSelf(rreq))
        {
          /* ---------------------- */
          /* "SELF" request         */
          /* ---------------------- */
          MPID_Request * const sreq = rreq->partner_request;
          MPID_assert(sreq != NULL);
          MPIDI_DCMF_Buffer_copy(sreq->dcmf.userbuf,
                                sreq->dcmf.userbufcount,
                                sreq->dcmf.datatype,
                                &sreq->status.MPI_ERROR,
                                buf,
                                count,
                                datatype,
                                (MPIDI_msg_sz_t*)&rreq->status.count,
                                &rreq->status.MPI_ERROR);
          MPID_Request_set_completed(sreq);
          MPID_Request_release(sreq);
          /* no other thread can possibly be waiting on rreq,
             so it is safe to reset ref_count and cc */
          rreq->cc = 0;
          MPIU_Object_set_ref(rreq, 1);
          *request = rreq;
          return rreq->status.MPI_ERROR;
        }

      else if (MPID_Request_isRzv(rreq))
        {
          /* -------------------------------------------------------- */
          /* Received an unexpected flow-control rendezvous RTS.      */
          /*     This is very similar to the found/incolplete case    */
          /* -------------------------------------------------------- */
          if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
            {
              MPID_Datatype_get_ptr(datatype, rreq->dcmf.datatype_ptr);
              MPID_Datatype_add_ref(rreq->dcmf.datatype_ptr);
            }
          MPIDI_DCMF_RendezvousTransfer (rreq);

          *request = rreq;
          return mpi_errno;
        }

      else if (*rreq->cc_ptr == 0)
        {
          /* -------------------------------- */
          /* request is complete              */
          /* if sync request, need to ack it. */
          /* -------------------------------- */
          if (MPID_Request_isSync(rreq))
            MPIDI_DCMF_postSyncAck(rreq);

          int smpi_errno;
          MPID_assert(rreq->dcmf.uebuf != NULL);
          if(rreq->status.cancelled == FALSE)
            {
              MPIDI_DCMF_Buffer_copy(rreq->dcmf.uebuf,
                                     rreq->dcmf.uebuflen,
                                     MPI_CHAR,
                                     &smpi_errno,
                                     buf,
                                     count,
                                     datatype,
                                     (MPIDI_msg_sz_t*)&rreq->status.count,
                                     &rreq->status.MPI_ERROR);
            }
          mpi_errno = rreq->status.MPI_ERROR;
          MPIU_Free(rreq->dcmf.uebuf); rreq->dcmf.uebuf = NULL;
/* JRATT : The following were not here originally, but are copied from mpid_recv.c */
/*           if (status != MPI_STATUS_IGNORE) { *status = rreq->status; } */

/*           MPID_Request_release(rreq); */
/*           rreq = NULL; */
          *request = rreq;
          return mpi_errno;
        }

      else
        {
          /* ----------------------- */
          /* request is incomplete.  */
          /* ----------------------- */
          if (MPID_Request_isSync(rreq))
            MPIDI_DCMF_postSyncAck(rreq);

          if(rreq->status.cancelled == FALSE)
            {
              if (rreq->dcmf.uebuf) /* we have an unexpected buffer */
                rreq->dcmf.ca = MPIDI_DCMF_CA_UNPACK_UEBUF_AND_COMPLETE;
              else /* no unexpected buffer; must be a resend */
                // MPIDI_DCMF_postFC (rreq, 0); /* send a NAK */
                MPID_abort();
            }
          if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
            {
              MPID_Datatype_get_ptr(datatype, rreq->dcmf.datatype_ptr);
              MPID_Datatype_add_ref(rreq->dcmf.datatype_ptr);
            }
          *request = rreq;
          return mpi_errno;
        }
    }
  else
    {
      /* ----------------------------------------------------------- */
      /* request not found in unexpected queue, allocated and posted */
      /* ----------------------------------------------------------- */

      if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
        {
          MPID_Datatype_get_ptr(datatype, rreq->dcmf.datatype_ptr);
          MPID_Datatype_add_ref(rreq->dcmf.datatype_ptr);
        }
      *request = rreq;
      return mpi_errno;
    }
  return mpi_errno;
}
