/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include <stdio.h>

/* STATES:NO WARNINGS */

/*#undef USE_IOV_LEN_2_SHORTCUT*/
#define USE_IOV_LEN_2_SHORTCUT

#define SHM_READING_BIT     0x0008

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/* shmem functions */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_write(MPIDI_VC_t * vc, void *buf, int len, int *num_bytes_ptr)
{
    int total = 0;
    int length;
    int index;
    MPIDI_CH3I_SHM_Queue_t * writeq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    writeq = vc->ch.write_shmq;
    index = writeq->tail_index;
    if (writeq->packet[index].avail == MPIDI_CH3I_PKT_FILLED)
    {
	*num_bytes_ptr = total;
	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
	return MPI_SUCCESS;
    }
    
    while (len)
    {
	length = min(len, MPIDI_CH3I_PACKET_SIZE);
	/*writeq->packet[index].offset = 0; the reader guarantees this is reset to zero */
	writeq->packet[index].num_bytes = length;
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(writeq->packet[index].data, buf, length);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	MPID_WRITE_BARRIER();
	writeq->packet[index].avail = MPIDI_CH3I_PKT_FILLED;
	buf = (char *) buf + length;
	total += length;
	len -= length;

	index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
	if (writeq->packet[index].avail == MPIDI_CH3I_PKT_FILLED)
	{
	    writeq->tail_index = index;
	    *num_bytes_ptr = total;
	    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
	    return MPI_SUCCESS;
	}
    }

    writeq->tail_index = index;
    *num_bytes_ptr = total;
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_writev(MPIDI_VC_t *vc, MPID_IOV *iov, int n, int *num_bytes_ptr)
{
#ifdef MPICH_DBG_OUTPUT
    int mpi_errno;
#endif
    int i;
    unsigned int total = 0;
    unsigned int num_bytes = 0;
    unsigned int cur_avail, dest_avail;
    unsigned char *cur_pos, *dest_pos;
    int index;
    MPIDI_CH3I_SHM_Queue_t * writeq;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    writeq = vc->ch.write_shmq;
    index = writeq->tail_index;
    if (writeq->packet[index].avail == MPIDI_CH3I_PKT_FILLED)
    {
	*num_bytes_ptr = 0;
	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
	return MPI_SUCCESS;
    }

    MPIU_DBG_PRINTF(("writing to write_shmq %p\n", writeq));
#ifdef USE_IOV_LEN_2_SHORTCUT
    if (n == 2 && (iov[0].MPID_IOV_LEN + iov[1].MPID_IOV_LEN) < MPIDI_CH3I_PACKET_SIZE)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "writing %d bytes to write_shmq %08p packet[%d]", iov[0].MPID_IOV_LEN + iov[1].MPID_IOV_LEN, writeq, index));
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(writeq->packet[index].data, 
	       iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(&writeq->packet[index].data[iov[0].MPID_IOV_LEN],
	       iov[1].MPID_IOV_BUF, iov[1].MPID_IOV_LEN);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	writeq->packet[index].num_bytes = 
	    iov[0].MPID_IOV_LEN + iov[1].MPID_IOV_LEN;
	total = writeq->packet[index].num_bytes;
	MPID_WRITE_BARRIER();
	writeq->packet[index].avail = MPIDI_CH3I_PKT_FILLED;
#ifdef MPICH_DBG_OUTPUT
	/*MPIU_Assert(index == writeq->tail_index);*/
	if (index != writeq->tail_index)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmq_index", "**shmq_index %d %d", index, writeq->tail_index);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
	    return mpi_errno;
	}
#endif
	writeq->tail_index =
	    (writeq->tail_index + 1) % MPIDI_CH3I_NUM_PACKETS;
	MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq tail = %d", writeq->tail_index));
	*num_bytes_ptr = total;
	MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
	return MPI_SUCCESS;
    }
#endif

    dest_pos = (unsigned char *)(writeq->packet[index].data);
    dest_avail = MPIDI_CH3I_PACKET_SIZE;
    writeq->packet[index].num_bytes = 0;
    for (i=0; i<n; i++)
    {
	if (iov[i].MPID_IOV_LEN <= dest_avail)
	{
	    total += iov[i].MPID_IOV_LEN;
	    writeq->packet[index].num_bytes += iov[i].MPID_IOV_LEN;
	    dest_avail -= iov[i].MPID_IOV_LEN;
	    MPIDI_DBG_PRINTF((60, FCNAME, "writing %d bytes to write_shmq %08p packet[%d]", iov[i].MPID_IOV_LEN, writeq, index));
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(dest_pos, iov[i].MPID_IOV_BUF, iov[i].MPID_IOV_LEN);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	    dest_pos += iov[i].MPID_IOV_LEN;
	}
	else
	{
	    total += dest_avail;
	    writeq->packet[index].num_bytes = MPIDI_CH3I_PACKET_SIZE;
	    MPIDI_DBG_PRINTF((60, FCNAME, "writing %d bytes to write_shmq %08p packet[%d]", dest_avail, writeq, index));
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(dest_pos, iov[i].MPID_IOV_BUF, dest_avail);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	    MPID_WRITE_BARRIER();
	    writeq->packet[index].avail = MPIDI_CH3I_PKT_FILLED;
	    cur_pos = (unsigned char *)iov[i].MPID_IOV_BUF + dest_avail;
	    cur_avail = iov[i].MPID_IOV_LEN - dest_avail;
	    while (cur_avail)
	    {
		index = writeq->tail_index = 
		    (writeq->tail_index + 1) % MPIDI_CH3I_NUM_PACKETS;
		MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq tail = %d", writeq->tail_index));
		if (writeq->packet[index].avail == MPIDI_CH3I_PKT_FILLED)
		{
		    *num_bytes_ptr = total;
		    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
		    return MPI_SUCCESS;
		}
		num_bytes = min(cur_avail, MPIDI_CH3I_PACKET_SIZE);
		writeq->packet[index].num_bytes = num_bytes;
		MPIDI_DBG_PRINTF((60, FCNAME, "writing %d bytes to write_shmq %08p packet[%d]", num_bytes, writeq, index));
		MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
		memcpy(writeq->packet[index].data, cur_pos, num_bytes);
		MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
		total += num_bytes;
		cur_pos += num_bytes;
		cur_avail -= num_bytes;
		if (cur_avail)
		{
		    MPID_WRITE_BARRIER();
		    writeq->packet[index].avail = MPIDI_CH3I_PKT_FILLED;
		}
	    }
	    dest_pos = (unsigned char *)(writeq->packet[index].data) + num_bytes;
	    dest_avail = MPIDI_CH3I_PACKET_SIZE - num_bytes;
	}
	if (dest_avail == 0)
	{
	    MPID_WRITE_BARRIER();
	    writeq->packet[index].avail = MPIDI_CH3I_PKT_FILLED;
	    index = writeq->tail_index = 
		(writeq->tail_index + 1) % MPIDI_CH3I_NUM_PACKETS;
	    MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq tail = %d", writeq->tail_index));
	    if (writeq->packet[index].avail == MPIDI_CH3I_PKT_FILLED)
	    {
		*num_bytes_ptr = total;
		MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
		return MPI_SUCCESS;
	    }
	    dest_pos = (unsigned char *)(writeq->packet[index].data);
	    dest_avail = MPIDI_CH3I_PACKET_SIZE;
	    writeq->packet[index].num_bytes = 0;
	}
    }
    if (dest_avail < MPIDI_CH3I_PACKET_SIZE)
    {
	MPID_WRITE_BARRIER();
	writeq->packet[index].avail = MPIDI_CH3I_PKT_FILLED;
	writeq->tail_index = 
	    (writeq->tail_index + 1) % MPIDI_CH3I_NUM_PACKETS;
	MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq tail = %d", writeq->tail_index));
    }

    *num_bytes_ptr = total;
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_rdma_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_rdma_writev(MPIDI_VC_t *vc, MPID_Request *sreq)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_rdma_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_rdma_readv(MPIDI_VC_t *vc, MPID_Request *rreq)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_read(MPIDI_VC_t * recv_vc_ptr, void *buf, int len, int *num_bytes_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    void *mem_ptr;
    int num_bytes;
    MPIDI_CH3I_SHM_Packet_t *pkt_ptr;
    MPIDI_CH3I_SHM_Queue_t *shm_ptr;
    register int index;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_READ);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_READ);

    shm_ptr = recv_vc_ptr->ch.read_shmq;
    if (shm_ptr == NULL)
    {
	*num_bytes_ptr = 0;
	goto fn_exit;
    }
    index = shm_ptr->head_index;
    pkt_ptr = &shm_ptr->packet[index];

    /* if the packet at the head index is available, the queue is empty */
    if (pkt_ptr->avail == MPIDI_CH3I_PKT_EMPTY)
    {
	*num_bytes_ptr = 0;
	goto fn_exit;
    }
    MPID_READ_BARRIER(); /* no loads after this line can occur before the avail flag has been read */

    MPIU_DBG_PRINTF(("MPIDI_CH3I_SHM_read_progress: reading from queue %p\n", shm_ptr));

    mem_ptr = (void*)(pkt_ptr->data + pkt_ptr->offset);
    num_bytes = pkt_ptr->num_bytes;

    MPIDI_DBG_PRINTF((60, FCNAME, "read %d bytes", num_bytes));
    if (num_bytes > len)
    {
	/* copy the received data */
	MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", recv_vc_ptr->ch.read.bufflen, shm_ptr, index));
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(buf, mem_ptr, len);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	*num_bytes_ptr = len;
	pkt_ptr->offset += len;
	pkt_ptr->num_bytes = num_bytes - len;
    }
    else
    {
	/* copy the received data */
	MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", num_bytes, shm_ptr, index));
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(buf, mem_ptr, num_bytes);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	*num_bytes_ptr = num_bytes;
	/* put the shmem buffer back in the queue */
	pkt_ptr->offset = 0;
	MPID_READ_WRITE_BARRIER(); /* the writing of the flag cannot occur before the reading of the last piece of data */
	pkt_ptr->avail = MPIDI_CH3I_PKT_EMPTY;
#ifdef MPICH_DBG_OUTPUT
	/*MPIU_Assert(&shm_ptr->packet[index] == pkt_ptr);*/
	if (&shm_ptr->packet[index] != pkt_ptr)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pkt_ptr", "**pkt_ptr %p %p", &shm_ptr->packet[index], pkt_ptr);
	    goto fn_exit;
	}
#endif
	shm_ptr->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
	MPIDI_DBG_PRINTF((60, FCNAME, "read_shmq head = %d", shm_ptr->head_index));
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_read_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_read_progress(MPIDI_VC_t *recv_vc_ptr, int millisecond_timeout, MPIDI_VC_t **vc_pptr, int *num_bytes_ptr)
{
    int mpi_errno;
    void *mem_ptr;
    char *iter_ptr;
    int num_bytes;
    unsigned int offset;
    MPIDI_CH3I_SHM_Packet_t *pkt_ptr;
    MPIDI_CH3I_SHM_Queue_t *shm_ptr;
    register int index, working;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);

    for (;;) 
    {
	working = FALSE;

	while (recv_vc_ptr)
	{
	    shm_ptr = recv_vc_ptr->ch.read_shmq;
	    if (shm_ptr == NULL)
	    {
		recv_vc_ptr = recv_vc_ptr->ch.shm_next_reader;
		continue;
	    }
	    index = shm_ptr->head_index;
	    pkt_ptr = &shm_ptr->packet[index];

	    /* if the packet at the head index is available, the queue is empty */
	    if (pkt_ptr->avail == MPIDI_CH3I_PKT_EMPTY)
	    {
		recv_vc_ptr = recv_vc_ptr->ch.shm_next_reader;
		continue;
	    }
	    MPID_READ_BARRIER(); /* no loads after this line can occur before the avail flag has been read */

	    working = TRUE;
	    MPIU_DBG_PRINTF(("MPIDI_CH3I_SHM_read_progress: reading from queue %p\n", shm_ptr));

	    mem_ptr = (void*)(pkt_ptr->data + pkt_ptr->offset);
	    num_bytes = pkt_ptr->num_bytes;

	    if (recv_vc_ptr->ch.shm_reading_pkt)
	    {
		MPIDI_DBG_PRINTF((60, FCNAME, "reading header(%d bytes) from read_shmq %08p packet[%d]", sizeof(MPIDI_CH3_Pkt_t), shm_ptr, index));
		if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3I_PKT_SC_CONN_ACCEPT)
		{
		    int port_name_tag = ((MPIDI_CH3_Pkt_t*)mem_ptr)->sc_conn_accept.port_name_tag;
		    MPIDI_CH3I_Acceptq_enqueue(recv_vc_ptr,port_name_tag);
		    MPIDI_CH3I_progress_completion_count++;
		    MPIDI_CH3I_SHM_Remove_vc_references(recv_vc_ptr);
		    recv_vc_ptr = NULL;
		    if (num_bytes > sizeof(MPIDI_CH3_Pkt_t))
		    {
			pkt_ptr->offset += sizeof(MPIDI_CH3_Pkt_t);
			num_bytes -= sizeof(MPIDI_CH3_Pkt_t);
			pkt_ptr->num_bytes = num_bytes;
			mem_ptr = (char*)mem_ptr + sizeof(MPIDI_CH3_Pkt_t);
		    }
		    else
		    {
			pkt_ptr->offset = 0;
			MPID_READ_WRITE_BARRIER(); /* the writing of the flag cannot occur before the reading of the last piece of data */
			pkt_ptr->avail = MPIDI_CH3I_PKT_EMPTY;
			shm_ptr->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
			MPIDI_DBG_PRINTF((60, FCNAME, "read_shmq head = %d", shm_ptr->head_index));
		    }
		    continue;
		}
		else
		{
		    mpi_errno = MPIDI_CH3U_Handle_recv_pkt(recv_vc_ptr, (MPIDI_CH3_Pkt_t*)mem_ptr, &recv_vc_ptr->ch.recv_active);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "shared memory read progress unable to handle incoming packet");
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			return mpi_errno;
		    }
		}

		if (recv_vc_ptr->ch.recv_active == NULL)
		{
		    recv_vc_ptr->ch.shm_reading_pkt = TRUE;
		}
		else
		{
		    mpi_errno = MPIDI_CH3I_SHM_post_readv(recv_vc_ptr, recv_vc_ptr->ch.recv_active->dev.iov, recv_vc_ptr->ch.recv_active->dev.iov_count, NULL);
		}
		if (num_bytes > sizeof(MPIDI_CH3_Pkt_t))
		{
		    pkt_ptr->offset += sizeof(MPIDI_CH3_Pkt_t);
		    num_bytes -= sizeof(MPIDI_CH3_Pkt_t);
		    pkt_ptr->num_bytes = num_bytes;
		    mem_ptr = (char*)mem_ptr + sizeof(MPIDI_CH3_Pkt_t);
		}
		else
		{
		    pkt_ptr->offset = 0;
		    MPID_READ_WRITE_BARRIER(); /* the writing of the flag cannot occur before the reading of the last piece of data */
		    pkt_ptr->avail = MPIDI_CH3I_PKT_EMPTY;
		    shm_ptr->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
		    MPIDI_DBG_PRINTF((60, FCNAME, "read_shmq head = %d", shm_ptr->head_index));
		    
		    recv_vc_ptr = recv_vc_ptr->ch.shm_next_reader;
		    continue;
		}
		if (recv_vc_ptr->ch.recv_active == NULL)
		{
		    recv_vc_ptr = recv_vc_ptr->ch.shm_next_reader;
		    continue;
		}
	    }

	    MPIDI_DBG_PRINTF((60, FCNAME, "read %d bytes", num_bytes));
	    /*MPIDI_DBG_PRINTF((60, FCNAME, "shm_wait(recv finished %d bytes)", num_bytes));*/
	    if (!(recv_vc_ptr->ch.shm_state & SHM_READING_BIT))
	    {
		recv_vc_ptr = recv_vc_ptr->ch.shm_next_reader;
		continue;
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "read update, total = %d + %d = %d", recv_vc_ptr->ch.read.total, num_bytes, recv_vc_ptr->ch.read.total + num_bytes));
	    if (recv_vc_ptr->ch.read.use_iov)
	    {
		iter_ptr = mem_ptr;
		while (num_bytes && recv_vc_ptr->ch.read.iovlen > 0)
		{
		    if ((int)recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN <= num_bytes)
		    {
			/* copy the received data */
			MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN, shm_ptr, index));
			MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
			memcpy(recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_BUF, iter_ptr,
			    recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN);
			MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
			iter_ptr += recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN;
			/* update the iov */
			num_bytes -= recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN;
			recv_vc_ptr->ch.read.index++;
			recv_vc_ptr->ch.read.iovlen--;
		    }
		    else
		    {
			/* copy the received data */
			MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", num_bytes, shm_ptr, index));
			MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
			memcpy(recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_BUF, iter_ptr, num_bytes);
			MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
			iter_ptr += num_bytes;
			/* update the iov */
			recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_LEN -= num_bytes;
			recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)(
			    (char*)(recv_vc_ptr->ch.read.iov[recv_vc_ptr->ch.read.index].MPID_IOV_BUF) + num_bytes);
			num_bytes = 0;
		    }
		}
		offset = (unsigned int)((unsigned char*)iter_ptr - (unsigned char*)mem_ptr);
		recv_vc_ptr->ch.read.total += offset;
		if (num_bytes == 0)
		{
		    /* put the shmem buffer back in the queue */
		    pkt_ptr->offset = 0;
		    MPID_READ_WRITE_BARRIER(); /* the writing of the flag cannot occur before the reading of the last piece of data */
		    pkt_ptr->avail = MPIDI_CH3I_PKT_EMPTY;
#ifdef MPICH_DBG_OUTPUT
		    /*MPIU_Assert(&shm_ptr->packet[index] == pkt_ptr);*/
		    if (&shm_ptr->packet[index] != pkt_ptr)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pkt_ptr", "**pkt_ptr %p %p", &shm_ptr->packet[index], pkt_ptr);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			return mpi_errno;
		    }
#endif
		    shm_ptr->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
		    MPIDI_DBG_PRINTF((60, FCNAME, "read_shmq head = %d", shm_ptr->head_index));
		}
		else
		{
		    /* update the head of the shmem queue */
		    pkt_ptr->offset += (pkt_ptr->num_bytes - num_bytes);
		    pkt_ptr->num_bytes = num_bytes;
		}
		if (recv_vc_ptr->ch.read.iovlen == 0)
		{
		    if (recv_vc_ptr->ch.recv_active->kind < MPID_LAST_REQUEST_KIND)
		    {
			recv_vc_ptr->ch.shm_state &= ~SHM_READING_BIT;
			*num_bytes_ptr = recv_vc_ptr->ch.read.total;
			*vc_pptr = recv_vc_ptr;
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			return MPI_SUCCESS;
		    }
		    else
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "invalid request type", recv_vc_ptr->ch.recv_active->kind);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			return mpi_errno;
		    }
		}
	    }
	    else
	    {
		if ((unsigned int)num_bytes > recv_vc_ptr->ch.read.bufflen)
		{
		    /* copy the received data */
		    MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", recv_vc_ptr->ch.read.bufflen, shm_ptr, index));
		    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
		    memcpy(recv_vc_ptr->ch.read.buffer, mem_ptr, recv_vc_ptr->ch.read.bufflen);
		    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
		    recv_vc_ptr->ch.read.total = recv_vc_ptr->ch.read.bufflen;
		    pkt_ptr->offset += recv_vc_ptr->ch.read.bufflen;
		    pkt_ptr->num_bytes = num_bytes - recv_vc_ptr->ch.read.bufflen;
		    recv_vc_ptr->ch.read.bufflen = 0;
		}
		else
		{
		    /* copy the received data */
		    MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", num_bytes, shm_ptr, index));
		    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
		    memcpy(recv_vc_ptr->ch.read.buffer, mem_ptr, num_bytes);
		    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
		    recv_vc_ptr->ch.read.total += num_bytes;
		    /* advance the user pointer */
		    recv_vc_ptr->ch.read.buffer = (char*)(recv_vc_ptr->ch.read.buffer) + num_bytes;
		    recv_vc_ptr->ch.read.bufflen -= num_bytes;
		    /* put the shmem buffer back in the queue */
		    pkt_ptr->offset = 0;
		    MPID_READ_WRITE_BARRIER(); /* the writing of the flag cannot occur before the reading of the last piece of data */
		    pkt_ptr->avail = MPIDI_CH3I_PKT_EMPTY;
#ifdef MPICH_DBG_OUTPUT
		    /*MPIU_Assert(&shm_ptr->packet[index] == pkt_ptr);*/
		    if (&shm_ptr->packet[index] != pkt_ptr)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pkt_ptr", "**pkt_ptr %p %p", &shm_ptr->packet[index], pkt_ptr);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			return mpi_errno;
		    }
#endif
		    shm_ptr->head_index = (index + 1) % MPIDI_CH3I_NUM_PACKETS;
		    MPIDI_DBG_PRINTF((60, FCNAME, "read_shmq head = %d", shm_ptr->head_index));
		}
		if (recv_vc_ptr->ch.read.bufflen == 0)
		{
		    recv_vc_ptr->ch.shm_state &= ~SHM_READING_BIT;
		    *num_bytes_ptr = recv_vc_ptr->ch.read.total;
		    *vc_pptr = recv_vc_ptr;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
		    return MPI_SUCCESS;
		}
	    }
	    recv_vc_ptr = recv_vc_ptr->ch.shm_next_reader;
	}

	if (millisecond_timeout == 0 && !working)
	{
	    /* FIXME: This is wrong because SHM_WAIT_TIMEOUT might conflict with a real mpi error code */
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
	    return SHM_WAIT_TIMEOUT;
	}
    }

    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
    return mpi_errno;
}

/* non-blocking functions */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_post_read(MPIDI_VC_t *vc, void *buf, int len, int (*rfn)(int, void*))
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);
    MPIDI_DBG_PRINTF((60, FCNAME, "posting read of %d bytes", len));
    /*printf("...\n");fflush(stdout);*/
    vc->ch.read.total = 0;
    vc->ch.read.buffer = buf;
    vc->ch.read.bufflen = len;
    vc->ch.read.use_iov = FALSE;
    vc->ch.shm_state |= SHM_READING_BIT;
    vc->ch.shm_reading_pkt = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_post_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_post_readv(MPIDI_VC_t *vc, MPID_IOV *iov, int n, int (*rfn)(int, void*))
{
#ifdef MPICH_DBG_OUTPUT
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_POST_READV);
#ifdef USE_SHM_IOV_COPY
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);
#endif

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_POST_READV);

    /* FIXME */
    /* Remove this stripping code after the updated segment code no longer
       produces iov's with empty buffers */
    /* strip any trailing empty buffers */
    /*
    while (n && iov[n-1].MPID_IOV_LEN == 0)
	n--;
    */
    MPIU_Assert(iov[n-1].MPID_IOV_LEN > 0);

    MPIDI_DBG_PRINTF((60, FCNAME, "posting read of iov[%d]", n));
    /*printf(".x.x.\n");fflush(stdout);*/
#ifdef MPICH_DBG_OUTPUT
    if (n > 1)
    {
	for (i=0; i<n; i++)
	{
	    MPIU_DBG_PRINTF(("iov[%d].len = %d\n", i, iov[i].MPID_IOV_LEN));
	}
    }
#endif
    vc->ch.read.total = 0;
#ifdef USE_SHM_IOV_COPY
    /* This isn't necessary if we require the iov to be valid for the duration of the operation */
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    memcpy(vc->ch.read.iov, iov, sizeof(MPID_IOV) * n);
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
#else
    vc->ch.read.iov = iov;
#endif
    vc->ch.read.iovlen = n;
    vc->ch.read.index = 0;
    vc->ch.read.use_iov = TRUE;
    vc->ch.shm_state |= SHM_READING_BIT;
    vc->ch.shm_reading_pkt = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_POST_READV);
    return MPI_SUCCESS;
}
