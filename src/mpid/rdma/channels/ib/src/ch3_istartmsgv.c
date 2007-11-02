/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*static MPID_Request * create_request(MPID_IOV * iov, int iov_count, int iov_offset, int nb)*/
#undef create_request
#define create_request(sreq, iov, count, offset, nb) \
{ \
    int i; \
    MPIDI_STATE_DECL(MPID_STATE_CREATE_REQUEST); \
    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_REQUEST); \
    sreq = MPIDI_CH3_Request_create(); \
    assert(sreq != NULL); \
    MPIU_Object_set_ref(sreq, 2); \
    sreq->kind = MPID_REQUEST_SEND; \
    /* memcpy(sreq->dev.iov, iov, iov_count * sizeof(MPID_IOV)); */ \
    for (i = 0; i < count; i++) \
    { \
	sreq->dev.iov[i] = iov[i]; \
    } \
    if (offset == 0) \
    { \
	/* memcpy(&sreq->ch.pkt, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN); */ \
	assert(iov[0].MPID_IOV_LEN == sizeof(MPIDI_CH3_Pkt_t)); \
	sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPID_IOV_BUF; \
	sreq->dev.iov[0].MPID_IOV_BUF = (void*)&sreq->ch.pkt; \
    } \
    (char *) sreq->dev.iov[offset].MPID_IOV_BUF += nb; \
    sreq->dev.iov[offset].MPID_IOV_LEN -= nb; \
    sreq->ch.iov_offset = offset; \
    sreq->dev.iov_count = count; \
    sreq->dev.ca = MPIDI_CH3_CA_COMPLETE; \
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST); \
}

/*
 * MPIDI_CH3_iStartMsgv() attempts to send the message immediately.  If the
 * entire message is successfully sent, then NULL is returned.  Otherwise a
 * request is allocated, the iovec and the first buffer pointed to by the
 * iovec (which is assumed to be a MPIDI_CH3_Pkt_t) are copied into the
 * request, and a pointer to the request is returned.  An error condition also
 * results in a request be allocated and the errror being returned in the
 * status field of the request.
 */

/* XXX - What do we do if MPIDI_CH3_Request_create() returns NULL???  If
   MPIDI_CH3_iStartMsgv() returns NULL, the calling code assumes the request
   completely successfully, but the reality is that we couldn't allocate the
   memory for a request.  This seems like a flaw in the CH3 API. */

/* NOTE - The completion action associated with a request created by
   CH3_iStartMsgv() is alway MPIDI_CH3_CA_COMPLETE.  This implies that
   CH3_iStartMsgv() can only be used when the entire message can be described
   by a single iovec of size MPID_IOV_LIMIT. */
    
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsgv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsgv(MPIDI_VC * vc, MPID_IOV * iov, int n_iov, MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreq = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
    MPIU_DBG_PRINTF(("ch3_istartmsgv\n"));

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
#ifdef MPICH_DBG_OUTPUT
    if (n_iov > MPID_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
	return mpi_errno;
    }
    if (iov[0].MPID_IOV_LEN > sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
	return mpi_errno;
    }
#endif

    /* The IB implementation uses a fixed length header, the size of which is
       the maximum of all possible packet headers */
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    
    /* Connection already formed.  If send queue is empty attempt to send
    data, queuing any unsent data. */
    if (MPIDI_CH3I_SendQ_empty(vc)) /* MT */
    {
	int nb;
	int offset = 0;

	/* MT - need some signalling to lock down our right to use the
	channel, thus insuring that the progress engine does also try to
	write */

	mpi_errno = (n_iov > 1) ?
	    ibu_writev(vc->ch.ibu, iov, n_iov, &nb) :
	    ibu_write(vc->ch.ibu, iov->MPID_IOV_BUF, iov->MPID_IOV_LEN, &nb);
	if (mpi_errno != MPI_SUCCESS)
	{
	    sreq = MPIDI_CH3_Request_create();
	    if (sreq == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
		return mpi_errno;
	    }
	    sreq->kind = MPID_REQUEST_SEND;
	    sreq->cc = 0;
	    /* TODO: Create an appropriate error message based on the return value */
	    sreq->status.MPI_ERROR = MPI_ERR_INTERN;
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ibwrite", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
	    return mpi_errno;
	}

	MPIU_DBG_PRINTF(("ch3_istartmsgv: ibu_post_writev returned %d bytes\n", nb));
	offset = 0;

	while (offset < n_iov)
	{
	    if (nb >= (int)iov[offset].MPID_IOV_LEN)
	    {
		nb -= iov[offset].MPID_IOV_LEN;
		offset++;
	    }
	    else
	    {
		MPIU_DBG_PRINTF(("ch3_istartmsgv: ibu_post_writev did not complete the send, allocating request\n"));
		create_request(sreq, iov, n_iov, offset, nb);
		MPIDI_CH3I_SendQ_enqueue_head(vc, sreq);
		vc->ch.send_active = sreq;
		break;
	    }
	}
    }
    else
    {
	create_request(sreq, iov, n_iov, 0, 0);
	MPIDI_CH3I_SendQ_enqueue(vc, sreq);
    }

    *sreq_ptr = sreq;

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
    return mpi_errno;
}
