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

#define MPIDI_CH3_PKT_RELOAD_SEND 1
#define MPIDI_CH3_PKT_RELOAD_RECV 0

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/* 
 * Here are the two choices that we need to make to allow this 
 * file to be used by several channels.
 *
 * First, is it part of a multi-method channel (e.g., ssm) or part of a
 * single method channel (e.g. shm)?
 *
 * Define MPIDI_CH3_SHM_SHARES_PKTARRAY if it is shared
 *
 * Second, how are the receive queues arranged?  Are they
 * in a list (scalable, by active connection) or 
 * are they in an array of pointers?
 *
 * Define MPIDI_CH3_SHM_SCALABLE_READQUEUES if they are in a list
 */

/* shmem functions */

#ifdef MPIDI_CH3_SHM_SHARES_PKTARRAY
extern MPIDI_CH3_PktHandler_Fcn *MPIDI_pktArray[MPIDI_CH3_PKT_END_CH3+1];
#else
static MPIDI_CH3_PktHandler_Fcn *MPIDI_pktArray[MPIDI_CH3_PKT_END_CH3+1];

int MPIDI_CH3I_SHM_Progress_init(void)
{
    int mpi_errno;

    /* Initialize the code to handle incoming packets */
    mpi_errno = MPIDI_CH3_PktHandler_Init( MPIDI_pktArray, 
					   MPIDI_CH3_PKT_END_CH3+1 );

    return mpi_errno;
}
#endif

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
    MPIDI_CH3I_VC *vcch;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITE);

    vcch = (MPIDI_CH3I_VC *)vc->channel_private;

    writeq = vcch->write_shmq;
    index = writeq->tail_index;
    if (writeq->packet[index].avail == MPIDI_CH3I_PKT_FILLED)
    {
	*num_bytes_ptr = total;
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
	return MPI_SUCCESS;
    }
    
    while (len)
    {
	length = min(len, MPIDI_CH3I_PACKET_SIZE);
	/*writeq->packet[index].offset = 0; 
	  the reader guarantees this is reset to zero */
	writeq->packet[index].num_bytes = length;
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(writeq->packet[index].data, buf, length);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	MPIU_DBG_PRINTF(("shm_write: %d bytes in packet %d\n", 
			 writeq->packet[index].num_bytes, index));
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
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
	    return MPI_SUCCESS;
	}
    }

    writeq->tail_index = index;
    *num_bytes_ptr = total;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_writev(MPIDI_VC_t *vc, MPID_IOV *iov, int n, 
			  int *num_bytes_ptr)
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
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);

    writeq = vcch->write_shmq;
    index = writeq->tail_index;
    if (writeq->packet[index].avail == MPIDI_CH3I_PKT_FILLED)
    {
	*num_bytes_ptr = 0;
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
	return MPI_SUCCESS;
    }

    MPIU_DBG_PRINTF(("writing to write_shmq %p\n", writeq));
#ifdef USE_IOV_LEN_2_SHORTCUT
    if (n == 2 && (iov[0].MPID_IOV_LEN + iov[1].MPID_IOV_LEN) < 
	MPIDI_CH3I_PACKET_SIZE)
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
	MPIU_DBG_PRINTF(("shm_writev - %d bytes in packet %d\n", writeq->packet[index].num_bytes, index));
	*num_bytes_ptr = total;
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
	    MPIU_DBG_PRINTF(("shm_writev: +%d=%d bytes in packet %d\n", iov[i].MPID_IOV_LEN, writeq->packet[index].num_bytes, index));
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
	    MPIU_DBG_PRINTF(("shm_writev: +%d=%d bytes in packet %d\n", dest_avail, writeq->packet[index].num_bytes, index));
	    MPID_WRITE_BARRIER();
	    writeq->packet[index].avail = MPIDI_CH3I_PKT_FILLED;
	    cur_pos = (unsigned char *)(iov[i].MPID_IOV_BUF) + dest_avail;
	    cur_avail = iov[i].MPID_IOV_LEN - dest_avail;
	    while (cur_avail)
	    {
		index = writeq->tail_index = 
		    (writeq->tail_index + 1) % MPIDI_CH3I_NUM_PACKETS;
		MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq tail = %d", writeq->tail_index));
		if (writeq->packet[index].avail == MPIDI_CH3I_PKT_FILLED)
		{
		    *num_bytes_ptr = total;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
		    return MPI_SUCCESS;
		}
		num_bytes = min(cur_avail, MPIDI_CH3I_PACKET_SIZE);
		writeq->packet[index].num_bytes = num_bytes;
		MPIDI_DBG_PRINTF((60, FCNAME, "writing %d bytes to write_shmq %08p packet[%d]", num_bytes, writeq, index));
		MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
		memcpy(writeq->packet[index].data, cur_pos, num_bytes);
		MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
		MPIU_DBG_PRINTF(("shm_writev: +%d=%d bytes in packet %d\n", num_bytes, writeq->packet[index].num_bytes, index));
		total += num_bytes;
		cur_pos += num_bytes;
		cur_avail -= num_bytes;
		if (cur_avail)
		{
		    MPID_WRITE_BARRIER();
		    writeq->packet[index].avail = MPIDI_CH3I_PKT_FILLED;
		}
	    }
	    dest_pos = (unsigned char *)
		(writeq->packet[index].data) + num_bytes;
	    dest_avail = MPIDI_CH3I_PACKET_SIZE - num_bytes;
	}
	if (dest_avail == 0)
	{
	    MPID_WRITE_BARRIER();
	    writeq->packet[index].avail = MPIDI_CH3I_PKT_FILLED;
	    index = writeq->tail_index = 
		(writeq->tail_index + 1) % MPIDI_CH3I_NUM_PACKETS;
	    MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq tail = %d", 
			      writeq->tail_index));
	    if (writeq->packet[index].avail == MPIDI_CH3I_PKT_FILLED)
	    {
		*num_bytes_ptr = total;
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
	MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq tail = %d", 
			  writeq->tail_index));
    }

    *num_bytes_ptr = total;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_WRITEV);
    return MPI_SUCCESS;
}

#ifdef USE_RDMA_READV
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
#endif /* USE_RDMA_READV */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_read_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_read_progress(MPIDI_VC_t *vc, int millisecond_timeout, 
				 MPIDI_VC_t **vc_pptr, int *num_bytes_ptr, 
				 shm_wait_t *shm_out)
{
    int mpi_errno = MPI_SUCCESS;
    void *mem_ptr;
    char *iter_ptr;
    int num_bytes;
    unsigned int offset;
    MPIDI_VC_t *recv_vc_ptr;
    MPIDI_CH3I_VC *recv_vcch;
    MPIDI_CH3I_SHM_Packet_t *pkt_ptr;
    MPIDI_CH3I_SHM_Queue_t *shm_ptr;
    register int index, working;
#ifndef MPIDI_CH3_SHM_SCALABLE_READQUEUES
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);

    for (;;) 
    {
	working = FALSE;
#ifdef MPIDI_CH3_SHM_SCALABLE_READQUEUES
	for (recv_vc_ptr=vc; recv_vc_ptr; 
	     recv_vc_ptr = recv_vcch->shm_next_reader )
	{
	    recv_vcch = (MPIDI_CH3I_VC *)recv_vc_ptr->channel_private;
	    shm_ptr = recv_vcch->read_shmq;
	    if (shm_ptr == NULL)
		continue;
#else
	for (i=0; i<MPIDI_PG_Get_size(vc->pg); i++)
	{
	    /* skip over the vc to myself */
	    if (vc->pg_rank == i)
		continue;
	    recv_vcch = (MPIDI_CH3I_VC *)vc->channel_private;

	    shm_ptr = &recv_vcch->shm[i];
	    MPIDI_PG_Get_vc(vc->pg, i, &recv_vc_ptr);
	    recv_vcch = (MPIDI_CH3I_VC *)recv_vc_ptr->channel_private;
#endif
	    index = shm_ptr->head_index;
	    pkt_ptr = &shm_ptr->packet[index];

	    /* if the packet at the head index is available, the queue is 
	       empty */
	    if (pkt_ptr->avail == MPIDI_CH3I_PKT_EMPTY)
		continue;
	    MPID_READ_BARRIER(); /* no loads after this line can occur before 
				    the avail flag has been read */

	    working = TRUE;
	    MPIU_DBG_PRINTF(("MPIDI_CH3I_SHM_read_progress: reading from queue %p\n", shm_ptr));

	    mem_ptr = (void*)(pkt_ptr->data + pkt_ptr->offset);
	    num_bytes = pkt_ptr->num_bytes;

	    if (recv_vcch->shm_reading_pkt)
	    {
		MPIU_DBG_PRINTF(("shm_read_progress: reading %d byte header from shm packet %d offset %d size %d\n", sizeof(MPIDI_CH3_Pkt_t), index, pkt_ptr->offset, num_bytes));
		{
		    {
                        MPIDI_msg_sz_t buflen = sizeof (MPIDI_CH3_Pkt_t);
                        MPIDI_CH3_Pkt_t *pkt = (MPIDI_CH3_Pkt_t*)mem_ptr;
			mpi_errno = MPIDI_pktArray[pkt->type]( 
			    recv_vc_ptr, pkt, &buflen, &recv_vcch->recv_active);
                        MPIU_Assert(mpi_errno || buflen == sizeof (MPIDI_CH3_Pkt_t));
		    }
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "shared memory read progress unable to handle incoming packet");
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			return mpi_errno;
		    }
		}

		if (recv_vcch->recv_active == NULL)
		{
		    recv_vcch->shm_reading_pkt = TRUE;
		}
		else
		{
		    mpi_errno = MPIDI_CH3I_SHM_post_readv(recv_vc_ptr, recv_vcch->recv_active->dev.iov, recv_vcch->recv_active->dev.iov_count, NULL);
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
		    continue;
		}
		if (recv_vcch->recv_active == NULL)
		    continue;
	    }

	    MPIDI_DBG_PRINTF((60, FCNAME, "read %d bytes\n", num_bytes));
	    if (!(recv_vcch->shm_state & SHM_READING_BIT))
	    {
		continue;
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "read update, total = %d + %d = %d\n", recv_vcch->read.total, num_bytes, recv_vcch->ch.read.total + num_bytes));
	    if (recv_vcch->read.use_iov)
	    {
		iter_ptr = mem_ptr;
		while (num_bytes && recv_vcch->read.iovlen > 0)
		{
		    if ((int)recv_vcch->read.iov[recv_vcch->read.index].MPID_IOV_LEN <= num_bytes)
		    {
			/* copy the received data */
			MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
			memcpy(recv_vcch->read.iov[recv_vcch->read.index].MPID_IOV_BUF, iter_ptr,
			    recv_vcch->read.iov[recv_vcch->read.index].MPID_IOV_LEN);
			MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
			MPIU_DBG_PRINTF(("a:shm_read_progress: %d bytes read from packet %d offset %d\n",
			    recv_vcch->read.iov[recv_vcch->read.index].MPID_IOV_LEN, index,
			    pkt_ptr->offset + (int)((char*)iter_ptr - (char*)mem_ptr)));
			iter_ptr += recv_vcch->read.iov[recv_vcch->read.index].MPID_IOV_LEN;
			/* update the iov */
			num_bytes -= recv_vcch->read.iov[recv_vcch->read.index].MPID_IOV_LEN;
			recv_vcch->read.index++;
			recv_vcch->read.iovlen--;
		    }
		    else
		    {
			/* copy the received data */
			MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
			memcpy(recv_vcch->read.iov[recv_vcch->read.index].MPID_IOV_BUF, iter_ptr, num_bytes);
			MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
			MPIU_DBG_PRINTF(("b:shm_read_progress: %d bytes read from packet %d offset %d\n", num_bytes, index,
			    pkt_ptr->offset + (int)((char*)iter_ptr - (char*)mem_ptr)));
			iter_ptr += num_bytes;
			/* update the iov */
			recv_vcch->read.iov[recv_vcch->read.index].MPID_IOV_LEN -= num_bytes;
			recv_vcch->read.iov[recv_vcch->read.index].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)(
			    (char*)(recv_vcch->read.iov[recv_vcch->read.index].MPID_IOV_BUF) + num_bytes);
			num_bytes = 0;
		    }
		}
		offset = (unsigned int)((unsigned char*)iter_ptr - (unsigned char*)mem_ptr);
		recv_vcch->read.total += offset;
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
		if (recv_vcch->read.iovlen == 0)
		{
		    if (recv_vcch->recv_active->kind < MPID_LAST_REQUEST_KIND)
		    {
			recv_vcch->shm_state &= ~SHM_READING_BIT;
			*num_bytes_ptr = recv_vcch->read.total;
			*vc_pptr = recv_vc_ptr;
			*shm_out = SHM_WAIT_READ;
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			return MPI_SUCCESS;
		    }
		    else
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "invalid request type", recv_vcch->recv_active->kind);
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			return mpi_errno;
		    }
		}
	    }
	    else
	    {
		if ((unsigned int)num_bytes > recv_vcch->read.bufflen)
		{
		    /* copy the received data */
		    MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", recv_vcch->read.bufflen, shm_ptr, index));
		    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
		    memcpy(recv_vcch->read.buffer, mem_ptr, recv_vcch->read.bufflen);
		    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
		    recv_vcch->read.total = recv_vcch->read.bufflen;
		    pkt_ptr->offset += recv_vcch->read.bufflen;
		    pkt_ptr->num_bytes = num_bytes - recv_vcch->read.bufflen;
		    recv_vcch->read.bufflen = 0;
		}
		else
		{
		    /* copy the received data */
		    MPIDI_DBG_PRINTF((60, FCNAME, "reading %d bytes from read_shmq %08p packet[%d]", num_bytes, shm_ptr, index));
		    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
		    memcpy(recv_vcch->read.buffer, mem_ptr, num_bytes);
		    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
		    recv_vcch->read.total += num_bytes;
		    /* advance the user pointer */
		    recv_vcch->read.buffer = (char*)(recv_vcch->read.buffer) + num_bytes;
		    recv_vcch->read.bufflen -= num_bytes;
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
		if (recv_vcch->read.bufflen == 0)
		{
		    MPIU_Assert(recv_vcch->recv_active->kind < MPID_LAST_REQUEST_KIND);
		    recv_vcch->shm_state &= ~SHM_READING_BIT;
		    *num_bytes_ptr = recv_vcch->read.total;
		    *vc_pptr = recv_vc_ptr;
		    *shm_out = SHM_WAIT_READ;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
		    return MPI_SUCCESS;
		}
	    }
	} /* that for on recv_vc_ptr, way, way above */

	if (millisecond_timeout == 0 && !working)
	{
	    *shm_out = SHM_WAIT_TIMEOUT;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
	    return MPI_SUCCESS;
	}
    } /* while forever */

    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
    return mpi_errno;
}

/* non-blocking functions */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_post_read(MPIDI_VC_t *vc, void *buf, int len, 
			     int (*rfn)(int, void*))
{
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);
    MPIDI_DBG_PRINTF((60, FCNAME, "posting a read of %d bytes", len));
    vcch->read.total = 0;
    vcch->read.buffer = buf;
    vcch->read.bufflen = len;
    vcch->read.use_iov = FALSE;
    vcch->shm_state |= SHM_READING_BIT;
    vcch->shm_reading_pkt = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_POST_READ);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_post_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_post_readv(MPIDI_VC_t *vc, MPID_IOV *iov, int n, 
			      int (*rfn)(int, void*))
{
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
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
	int i;
	for (i=0; i<n; i++)
	{
	    MPIU_DBG_PRINTF(("iov[%d].len = %d\n", i, iov[i].MPID_IOV_LEN));
	}
    }
#endif
    vcch->read.total = 0;
#ifdef USE_SHM_IOV_COPY
    /* This isn't necessary if we require the iov to be valid for the 
       duration of the operation */
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
    memcpy(vcch->read.iov, iov, sizeof(MPID_IOV) * n);
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
#else
    vcch->read.iov = iov;
#endif
    vcch->read.iovlen = n;
    vcch->read.index = 0;
    vcch->read.use_iov = TRUE;
    vcch->shm_state |= SHM_READING_BIT;
    vcch->shm_reading_pkt = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_POST_READV);
    return MPI_SUCCESS;
}


/* -------------------------------------------------------------------------- */
/* Routines to create/open shared memory                                      */
/* -------------------------------------------------------------------------- */
/* Create a name that may be used for a shared-memory region.
   "str" must have at least maxlen characters, and it is recommended 
   that maxlen be at least MPIDI_MAX_SHM_NAME_LENGTH
   Note that the name may include a random number; this is to 
   help create unique names which is needed in some users (e.g., 
   a separate shared memory region for each pair of processes)
 */
void MPIDI_Generate_shm_string(char *str, int maxlen)
{
#ifdef USE_WINDOWS_SHM
    UUID guid;
    UuidCreate(&guid);
    MPIU_Snprintf(str, maxlen,
	"%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
	guid.Data1, guid.Data2, guid.Data3,
	guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    MPIU_DBG_PRINTF(("GUID = %s\n", str));
#elif defined (USE_POSIX_SHM)
    MPIU_Snprintf(str, maxlen, "/mpich_shm_%d_%d", rand(), getpid());
#elif defined (USE_SYSV_SHM)
    MPIU_Snprintf(str, maxlen, "%d", getpid());
#else
#error No shared memory subsystem defined
#endif
}
