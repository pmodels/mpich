/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "ibu.h"
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include "mpidi_ch3_impl.h"

#ifdef USE_IB_IBAL

#include "ibuimpl.ibal.h"

#undef FUNCNAME
#define FUNCNAME ibu_rdma_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_rdma_write(ibu_t ibu, void *sbuf, ibu_mem_t *smem, void *rbuf, ibu_mem_t *rmem, int len, int signalled, MPID_Request *sreq)
{
    ib_api_status_t status;
    ib_local_ds_t data;
    ib_send_wr_t work_req;
    ibu_work_id_handle_t *id_ptr;
    ibu_rdma_type_t entry_type; /* Added by Mellanox, dafna April 11th */
    MPIDI_STATE_DECL(MPID_STATE_IBU_RDMA_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_RDMA_WRITE);

    data.length = len;
    data.vaddr = (uint64_t)sbuf;
    data.lkey = smem->lkey;

    work_req.p_next = NULL;
    work_req.wr_type = WR_RDMA_WRITE;
    work_req.send_opt = signalled ? IB_SEND_OPT_SIGNALED : 0; /* IB_SEND_OPT_FENCE | IB_SEND_OPT_INLINE */
    work_req.num_ds = 1;
    work_req.ds_array = &data;
    work_req.immediate_data = 0;
    work_req.remote_ops.vaddr = (uint64_t)rbuf;
    work_req.remote_ops.rkey = rmem->rkey;

    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(IBU_Process.workAllocator); /* replaced g_workAllocator by Mellanox, dafna April 11th */
    *((ibu_work_id_handle_t**)&work_req.wr_id) = id_ptr;
    if (id_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_RDMA_WRITE);
	return IBU_FAIL;
    }
    id_ptr->ibu = ibu;
    id_ptr->mem = (void*)sreq;

    ibu->state |= IBU_RDMA_WRITING;

#ifdef MPICH_DBG_OUTPUT
    if (signalled)
    {
	MPIU_DBG_PRINTF(("signalled rdma write sreq: sreq=0x%x, rreq=0x%x\n", sreq->handle, sreq->dev.rdma_request));
    }
#endif
    MPIDI_DBG_PRINTF((60, FCNAME, "calling rdma ib_post_send(%d bytes)", len));

    status = ib_post_send( ibu->qp_handle, &work_req, NULL);
    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("%s: Error: failed to post ib rdma send, status = %s\n", FCNAME, ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_RDMA_WRITE);
	return IBU_FAIL;
    }

    /* Added by Mellanox, dafna April 11th: push entry to send_wqe_fifo */
    entry_type = (signalled)? IBU_RDMA_RDNV_SIGNALED : IBU_RDMA_RNDV_UNSIGNALED;
    send_wqe_info_fifo_push(ibu, entry_type, sreq, len);

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_RDMA_WRITE);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_rdma_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_rdma_read(ibu_t ibu, void *rbuf, ibu_mem_t *rmem, void *sbuf, ibu_mem_t *smem, int len, int signalled, MPID_Request *rreq)
{
    ib_api_status_t status;
    ib_local_ds_t data;
    ib_send_wr_t work_req;
    ibu_work_id_handle_t *id_ptr;
    ibu_rdma_type_t entry_type; /* Added by Mellanox, dafna April 11th */
    MPIDI_STATE_DECL(MPID_STATE_IBU_RDMA_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_RDMA_READ);

    data.length = len;
    data.vaddr  = (uint64_t)rbuf;
    data.lkey = rmem->lkey;

    work_req.p_next = NULL;
    work_req.wr_type = WR_RDMA_READ;
    work_req.send_opt = signalled ? IB_SEND_OPT_SIGNALED : 0; /* IB_SEND_OPT_FENCE | IB_SEND_OPT_INLINE */
    work_req.num_ds = 1;
    work_req.ds_array = &data;
    work_req.immediate_data = 0;
    work_req.remote_ops.vaddr = (uint64_t)sbuf;
    work_req.remote_ops.rkey = smem->rkey;

    id_ptr = (ibu_work_id_handle_t*)ibuBlockAlloc(IBU_Process.workAllocator); /* replaced g_workAllocator by Mellanox, dafna April 11th */
    *((ibu_work_id_handle_t**)&work_req.wr_id) = id_ptr;
    if (id_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlocAlloc returned NULL"));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_RDMA_READ);
	return IBU_FAIL;
    }
    id_ptr->ibu = (void*)ibu;
    id_ptr->mem = (void*)rreq;
    ibu->state |= IBU_RDMA_READING;

#ifdef MPICH_DBG_OUTPUT
    if (signalled)
    {
	MPIU_DBG_PRINTF(("signalled rdma read rreq: sreq=0x%x, rreq=0x%x\n", rreq->handle, rreq->dev.rdma_request));
	fflush(stdout);
    }
#endif
    MPIDI_DBG_PRINTF((60, FCNAME, "calling rdma ib_post_send(%d bytes)", len));
    status = ib_post_send( ibu->qp_handle, &work_req, NULL);
    if (status != IB_SUCCESS)
    {
	MPIU_Internal_error_printf("%s: Error: failed to post ib rdma send, status = %s\n", FCNAME, ib_get_err_str(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_RDMA_READ);
	return IBU_FAIL;
    }

    /* Added by Mellanox, dafna April 11th : push entry to send_wqe_fifo */
    entry_type = (signalled)? IBU_RDMA_RDNV_SIGNALED : IBU_RDMA_RNDV_UNSIGNALED;
    send_wqe_info_fifo_push(ibu, entry_type, rreq, len);

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_RDMA_READ);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_rdma_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_rdma_writev(MPIDI_VC_t *vc, MPID_Request *sreq)
{
#ifdef MPIDI_CH3_CHANNEL_RNDV
    int mpi_errno = MPI_SUCCESS;
    int i;
    char *rbuf, *sbuf;
    int rbuf_len, riov_offset;
    int sbuf_len;
    int len;
    int num_written;
    MPID_IOV *send_iov, *recv_iov;
    int send_count, recv_count;
    MPIDI_CH3_Pkt_t pkt;
    MPIDI_CH3_Pkt_rdma_reload_t * reload_pkt = &pkt.reload;
    int signalled = 1;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_RDMA_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_RDMA_WRITEV);

    /* save the receiver's request to send back with the reload packet */
    reload_pkt->rreq = sreq->dev.rdma_request;
    reload_pkt->sreq = sreq->handle;

    send_iov = sreq->dev.iov;
    send_count = sreq->dev.iov_count;
    recv_iov = sreq->dev.rdma_iov;
    recv_count = sreq->dev.rdma_iov_count;
    riov_offset = sreq->ch.riov_offset;

#ifdef MPICH_DBG_OUTPUT
    MPIU_DBG_PRINTF(("ibu_rdma: sreq = 0x%x, rreq = 0x%x.\n", reload_pkt->sreq, reload_pkt->sreq));
    MPIU_DBG_PRINTF(("ibu_rdma: writing %d send buffers into %d recv buffers.\n", send_count, recv_count));
    for (i=0; i<send_count; i++)
    {
	MPIU_DBG_PRINTF(("ibu_rdma: send buf[%d] = %p, len = %d\n",
	    i, send_iov[i].MPID_IOV_BUF, send_iov[i].MPID_IOV_LEN));
    }
    for (i=0; i<recv_count; i++)
    {
	MPIU_DBG_PRINTF(("ibu_rdma: recv buf[%d] = %p, len = %d\n",
	    i, recv_iov[i].MPID_IOV_BUF, recv_iov[i].MPID_IOV_LEN));
    }
#endif

    rbuf = recv_iov[0].MPID_IOV_BUF;
    rbuf_len = recv_iov[0].MPID_IOV_LEN;
    for (i=sreq->ch.iov_offset; i<send_count; i++)
    {
	sbuf = send_iov[i].MPID_IOV_BUF;
	sbuf_len = send_iov[i].MPID_IOV_LEN;
	while (sbuf_len)
	{
	    len = MPIDU_MIN(sbuf_len, rbuf_len);
	    MPIU_DBG_PRINTF(("posting write of %d bytes to remote process.\n", len));

	    if ( ((i == send_count - 1) && (sbuf_len == len)) ||
		((riov_offset == recv_count - 1) && (rbuf_len == len)) )
	    {
		signalled = 1;
	    }
	    else
	    {
		signalled = 0;
	    }
	    mpi_errno = ibu_rdma_write(vc->ch.ibu,
		sbuf, &sreq->ch.local_iov_mem[i],
		rbuf, &sreq->ch.remote_iov_mem[riov_offset],
		len, signalled, sreq);
	    if (mpi_errno != IBU_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "ibu_rdma_write failed", mpi_errno);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RDMA_WRITEV);
		return mpi_errno;
	    }
	    num_written = len;

	    MPIU_DBG_PRINTF(("wrote %d bytes to remote process\n", num_written));
	    if (num_written < rbuf_len)
	    {
		rbuf = rbuf + num_written;
		rbuf_len = rbuf_len - num_written;
	    }
	    else
	    {
		riov_offset = riov_offset + 1;
		if (riov_offset < recv_count)
		{
		    rbuf = recv_iov[riov_offset].MPID_IOV_BUF;
		    rbuf_len = recv_iov[riov_offset].MPID_IOV_LEN;
		}
		else
		{
		    rbuf_len = 0;
		}
	    }
	    sbuf = sbuf + num_written;
	    sbuf_len = sbuf_len - num_written;
	    if (riov_offset == recv_count)
	    {
		if ( (i != (send_count - 1)) || (sbuf_len != 0) )
		{
		    /* partial send, the recv iov needs to be reloaded */

		    if (sbuf_len != 0)
		    {
			sreq->dev.iov[i].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)sbuf;
			sreq->dev.iov[i].MPID_IOV_LEN = sbuf_len;
		    }
		    sreq->ch.iov_offset = i;

		    /* send the reload receiver message after the rdma writes have completed */
		    sreq->ch.reload_state = MPIDI_CH3I_RELOAD_RECEIVER;

		    MPIU_DBG_PRINTF(("ibu_rdma: on exit 1 - sreq = 0x%x, rreq = 0x%x.\n", reload_pkt->sreq, reload_pkt->sreq));
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RDMA_WRITEV);
		    return MPI_SUCCESS;
		}
	    }
	}
    }

    sreq->ch.reload_state = 0;
    if (rbuf_len != 0 && riov_offset < recv_count)
    {
	sreq->dev.rdma_iov[riov_offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rbuf;
	sreq->dev.rdma_iov[riov_offset].MPID_IOV_LEN = rbuf_len;
    }
    if (riov_offset == recv_count && rbuf_len == 0)
    {
	sreq->ch.reload_state |= MPIDI_CH3I_RELOAD_RECEIVER;
    }
    sreq->ch.riov_offset = riov_offset;
    sreq->ch.reload_state |= MPIDI_CH3I_RELOAD_SENDER;

    MPIU_DBG_PRINTF(("ibu_rdma: on exit 2 - sreq = 0x%x, rreq = 0x%x.\n", reload_pkt->sreq, reload_pkt->sreq));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RDMA_WRITEV);
    return mpi_errno;
#else
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_RDMA_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_RDMA_WRITEV);
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RDMA_WRITEV);
    return mpi_errno;
#endif
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_rdma_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_rdma_readv(MPIDI_VC_t *vc, MPID_Request *rreq)
{
#ifdef MPIDI_CH3_CHANNEL_RNDV
    int mpi_errno = MPI_SUCCESS;
    int i;
    char *rbuf, *sbuf;
    int rbuf_len;
    int sbuf_len, siov_offset;
    int len;
    int num_read;
    MPID_IOV *send_iov, *recv_iov;
    int send_count, recv_count;
    MPIDI_CH3_Pkt_t pkt;
    MPIDI_CH3_Pkt_rdma_reload_t * reload_pkt = &pkt.reload;
    int signalled = 1;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_RDMA_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_RDMA_READV);

    /* save the sender's request to send back with the reload packet */
    /*reload_pkt->req = rreq->dev.rdma_request;*/
    reload_pkt->sreq = rreq->dev.sender_req_id;
    reload_pkt->rreq = rreq->handle;

    recv_iov = rreq->dev.iov;
    recv_count = rreq->dev.iov_count;
    send_iov = rreq->dev.rdma_iov;
    send_count = rreq->dev.rdma_iov_count;
    siov_offset = rreq->dev.rdma_iov_offset;

#ifdef MPICH_DBG_OUTPUT
    MPIU_DBG_PRINTF(("ibu_rdma: reading %d send buffers into %d recv buffers.\n", send_count, recv_count));
    for (i=siov_offset; i<send_count; i++)
    {
	MPIU_DBG_PRINTF(("ibu_rdma: send buf[%d] = %p, len = %d\n",
	    i, send_iov[i].MPID_IOV_BUF, send_iov[i].MPID_IOV_LEN));
    }
    for (i=0; i<recv_count; i++)
    {
	MPIU_DBG_PRINTF(("ibu_rdma: recv buf[%d] = %p, len = %d\n",
	    i, recv_iov[i].MPID_IOV_BUF, recv_iov[i].MPID_IOV_LEN));
    }
#endif

    sbuf = send_iov[siov_offset].MPID_IOV_BUF;
    sbuf_len = send_iov[siov_offset].MPID_IOV_LEN;
    for (i=rreq->ch.iov_offset; i<recv_count; i++)
    {
	rbuf = recv_iov[i].MPID_IOV_BUF;
	rbuf_len = recv_iov[i].MPID_IOV_LEN;
	while (rbuf_len)
	{
	    len = MPIDU_MIN(rbuf_len, sbuf_len);
	    MPIU_DBG_PRINTF(("reading %d bytes from the remote process.\n", len));

	    if ( ((i == recv_count - 1) && (rbuf_len == len)) ||
		((siov_offset == send_count - 1) && (sbuf_len == len)) )
	    {
		signalled = 1;
	    }
	    else
	    {
		signalled = 0;
	    }
	    mpi_errno = ibu_rdma_read(vc->ch.ibu,
		rbuf, &rreq->ch.local_iov_mem[i],
		sbuf, &rreq->ch.remote_iov_mem[siov_offset],
		len, signalled, rreq);
	    if (mpi_errno != IBU_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "ibu_rdma_read failed", mpi_errno);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RDMA_READV);
		return mpi_errno;
	    }
	    num_read = len;

	    MPIU_DBG_PRINTF(("read %d bytes from the remote process\n", num_read));
	    if (num_read < sbuf_len)
	    {
		sbuf = sbuf + num_read;
		sbuf_len = sbuf_len - num_read;
	    }
	    else
	    {
		siov_offset = siov_offset + 1;
		if (siov_offset < send_count)
		{
		    sbuf = send_iov[siov_offset].MPID_IOV_BUF;
		    sbuf_len = send_iov[siov_offset].MPID_IOV_LEN;
		}
		else
		{
		    sbuf_len = 0;
		}
	    }
	    rbuf = rbuf + num_read;
	    rbuf_len = rbuf_len - num_read;
	    if (siov_offset == send_count)
	    {
		if ( (i != (recv_count - 1)) || (rbuf_len != 0) )
		{
		    /* partial read, the send iov needs to be reloaded */
		    MPIU_DBG_PRINTF(("partial read, the send iov needs to be reloaded.\n"));

		    if (rbuf_len != 0)
		    {
			rreq->dev.iov[i].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rbuf;
			rreq->dev.iov[i].MPID_IOV_LEN = rbuf_len;
		    }
		    rreq->ch.iov_offset = i;

		    /* send the reload sender message after the rdma reads have completed */
		    rreq->ch.reload_state = MPIDI_CH3I_RELOAD_SENDER;

		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RDMA_READV);
		    return MPI_SUCCESS;
		}
	    }
	}
    }

    rreq->ch.reload_state = 0;
    if (sbuf_len != 0 && siov_offset < send_count)
    {
	rreq->dev.rdma_iov[siov_offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)sbuf;
	rreq->dev.rdma_iov[siov_offset].MPID_IOV_LEN = sbuf_len;
    }
    if (siov_offset == send_count && sbuf_len == 0)
    {
	MPIU_DBG_PRINTF(("reload sender state set.\n"));
	rreq->ch.reload_state |= MPIDI_CH3I_RELOAD_SENDER;
    }
    rreq->ch.siov_offset = siov_offset;
    MPIU_DBG_PRINTF(("reload receiver state set.\n"));
    rreq->ch.reload_state |= MPIDI_CH3I_RELOAD_RECEIVER;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RDMA_READV);
    return mpi_errno;
#else
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_RDMA_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_RDMA_READV);
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RDMA_READV);
    return mpi_errno;
#endif
}

#endif /* USE_IB_IBAL */
