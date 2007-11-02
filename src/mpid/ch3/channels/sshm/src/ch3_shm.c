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
#ifdef MPIDI_CH3_CHANNEL_RNDV
    int mpi_errno = MPI_SUCCESS;
    int i;
    char *rbuf, *sbuf;
    int rbuf_len, riov_offset;
    int sbuf_len;
    int len;
#ifndef HAVE_WINDOWS_H
#define SIZE_T int
#endif
    SIZE_T num_written;
    MPID_IOV *send_iov, *recv_iov;
    int send_count, recv_count;
    int complete;
    MPIDI_CH3_Pkt_t pkt;
    MPIDI_CH3_Pkt_rdma_reload_t * reload_pkt = &pkt.reload;
    MPID_Request * reload_sreq;
#ifndef HAVE_WINDOWS_H
    int n;
    OFF_T uOffset;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);

    /* save the receiver's request to send back with the reload packet */
    reload_pkt->rreq = sreq->dev.rdma_request;
    reload_pkt->sreq = sreq->handle;

#ifndef HAVE_WINDOWS_H
    mpi_errno = MPIDI_SHM_AttachProc( vc->ch.nSharedProcessID );
    if (mpi_errno) {
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
	return mpi_errno;
    }
#endif
    for (;;)
    {
	send_iov = sreq->dev.iov;
	send_count = sreq->dev.iov_count;
	recv_iov = sreq->dev.rdma_iov;
	recv_count = sreq->dev.rdma_iov_count;

	/*
	printf("shm_rdma: writing %d send buffers into %d recv buffers.\n", send_count, recv_count);fflush(stdout);
	for (i=0; i<send_count; i++)
	{
	    printf("shm_rdma: send buf[%d] = %p, len = %d\n", i, send_iov[i].MPID_IOV_BUF, send_iov[i].MPID_IOV_LEN);
	}
	for (i=0; i<recv_count; i++)
	{
	    printf("shm_rdma: recv buf[%d] = %p, len = %d\n", i, recv_iov[i].MPID_IOV_BUF, recv_iov[i].MPID_IOV_LEN);
	}
	fflush(stdout);
	*/
	rbuf = recv_iov[0].MPID_IOV_BUF;
	rbuf_len = recv_iov[0].MPID_IOV_LEN;
	riov_offset = 0;
	for (i=sreq->ch.iov_offset; i<send_count; i++)
	{
	    sbuf = send_iov[i].MPID_IOV_BUF;
	    sbuf_len = send_iov[i].MPID_IOV_LEN;
	    while (sbuf_len)
	    {
		len = MPIDU_MIN(sbuf_len, rbuf_len);
		/*printf("writing %d bytes to remote process.\n", len);fflush(stdout);*/
#ifdef HAVE_WINDOWS_H
		if (!WriteProcessMemory(vc->ch.hSharedProcessHandle, rbuf, sbuf, len, &num_written))
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "WriteProcessMemory failed", GetLastError());
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
		    return mpi_errno;
		}
		if (num_written == -1)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "WriteProcessMemory returned -1 bytes written");
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
		    return mpi_errno;
		}
#else

#ifdef HAVE_PROC_RDMA_WRITE
		/* write is not implemented in the /proc device. It is considered a security hole.  You can recompile a Linux
		 * kernel with this function enabled and then define HAVE_PROC_RDMA_WRITE and this code will work.
		 */
		uOffset = lseek(vc->ch.nSharedProcessFileDescriptor, OFF_T_CAST(rbuf), SEEK_SET);
		if (uOffset != OFF_T_CAST(rbuf))
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "lseek failed", errno);
		    MPIDI_SHM_DetachProc( vc->ch.nSharedProcessID );
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
		    return mpi_errno;
		}

		num_written = write(vc->ch.nSharedProcessFileDescriptor, sbuf, len);
		if (num_written < 1)
		{
		    if (num_written == -1)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "write failed", errno);
			MPIDI_SHM_DetachProc( vc->ch.nSharedProcessID );
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
			return mpi_errno;
		    }
		    ptrace(PTRACE_PEEKDATA, vc->ch.nSharedProcessID, rbuf + len - num_written, 0);
		}
#else
		/* Do not use this code.  Using PTRACE_POKEDATA for rdma writes gives horrible performance.
		 * This code is only provided for correctness to show that the put model will run.
		 */
		do
                {
		    int *rbuf2;
		    rbuf2 = (int*)rbuf;
		    for (n=0; n<len/sizeof(int); n++)
		    {
			if (ptrace(PTRACE_POKEDATA, vc->ch.nSharedProcessID, rbuf2, &((int*)sbuf)[n]) != 0)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "ptrace pokedata failed", errno);
			    /* printf("EPERM = %d, ESRCH = %d, EIO = %d, EFAULT = %d\n", EPERM, ESRCH, EIO, EFAULT);fflush(stdout); */
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
			    return mpi_errno;
			}
		    }
		    n = len % sizeof(int);
		    if (n > 0)
		    {
			if (len > sizeof(int))
			{
			    if (ptrace(PTRACE_POKEDATA, vc->ch.nSharedProcessID, rbuf + len - sizeof(int), ((int*)(sbuf + len - sizeof(int)))) != 0)
			    {
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "ptrace pokedata failed", errno);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
				return mpi_errno;
			    }
			}
			else
			{
			    int data;
			    data = ptrace(PTRACE_PEEKDATA, vc->ch.nSharedProcessID, rbuf + len - n, 0);
			    if (data == -1 && errno != 0)
			    {
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "ptrace peekdata failed", errno);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
				return mpi_errno;
			    }
                            /* mask in the new bits */
			    /* printf("FIXME!");fflush(stdout); */
			    if (ptrace(PTRACE_POKEDATA, vc->ch.nSharedProcessID, rbuf + len - n, &data) != 0)
			    {
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "ptrace pokedata failed", errno);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
				return mpi_errno;
			    }
			}
		    }
		} while (0);
		num_written = len;
#endif

#endif
		/*printf("wrote %d bytes to remote process\n", num_written);fflush(stdout);*/
		if (num_written < (SIZE_T)rbuf_len)
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
		}
		sbuf = sbuf + num_written;
		sbuf_len = sbuf_len - num_written;
		if (riov_offset == recv_count)
		{
		    if ( (i != (send_count - 1)) || (sbuf_len != 0) )
		    {
#ifndef HAVE_WINDOWS_H
			mpi_errno = MPIDI_SHM_DetachProc( vc->ch.nSharedProcessID );
			if (mpi_errno) {
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
			    return mpi_errno;
			}
#endif
			/* the recv iov needs to be reloaded */
			if (sbuf_len != 0)
			{
			    sreq->dev.iov[i].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)sbuf;
			    sreq->dev.iov[i].MPID_IOV_LEN = sbuf_len;
			}
			sreq->ch.iov_offset = i;

			/* send the reload packet to the receiver */
			MPIDI_Pkt_init(reload_pkt, MPIDI_CH3_PKT_RELOAD);
			reload_pkt->send_recv = MPIDI_CH3_PKT_RELOAD_RECV;
			mpi_errno = MPIDI_CH3_iStartMsg(vc, reload_pkt, sizeof(*reload_pkt), &reload_sreq);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */
			if (reload_sreq != NULL)
			{
			    /* The sender doesn't need to know when the packet has been sent.  So release the request immediately */
			    MPID_Request_release(reload_sreq);
			}
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
			return MPI_SUCCESS;
		    }
		}
	    }
	}

#ifndef HAVE_WINDOWS_H
	mpi_errno = MPIDI_SHM_DetachProc( vc->ch.nSharedProcessID );
	if (mpi_errno) {
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
	    return mpi_errno;
	}
#endif

	/* update the sender's request */
	mpi_errno = MPIDI_CH3U_Handle_send_req(vc, sreq, &complete);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update request after rdma write");
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
	    return mpi_errno;
	}

	if (complete || (riov_offset == recv_count))
	{
	    /* send the reload/done packet to the receiver */
	    MPIDI_Pkt_init(reload_pkt, MPIDI_CH3_PKT_RELOAD);
	    reload_pkt->send_recv = MPIDI_CH3_PKT_RELOAD_RECV;
	    mpi_errno = MPIDI_CH3_iStartMsg(vc, reload_pkt, sizeof(*reload_pkt), &reload_sreq);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
		return mpi_errno;
	    }
	    /* --END ERROR HANDLING-- */
	    if (reload_sreq != NULL)
	    {
		/* The sender doesn't need to know when the packet has been sent.  So release the request immediately */
		MPID_Request_release(reload_sreq);
	    }
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
	    return MPI_SUCCESS;
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
    return mpi_errno;
#else
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_WRITEV);
    return mpi_errno;
#endif
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_rdma_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_rdma_readv(MPIDI_VC_t *vc, MPID_Request *rreq)
{
#ifdef MPIDI_CH3_CHANNEL_RNDV
    int mpi_errno = MPI_SUCCESS;
    int i;
    char *rbuf, *sbuf;
    int rbuf_len;
    int sbuf_len, siov_offset;
    int len;
#ifndef HAVE_WINDOWS_H
#define SIZE_T int
#endif
    SIZE_T num_read;
    MPID_IOV *send_iov, *recv_iov;
    int send_count, recv_count;
    int complete;
    MPIDI_CH3_Pkt_t pkt;
    MPIDI_CH3_Pkt_rdma_reload_t * reload_pkt = &pkt.reload;
    MPID_Request * reload_rreq;
#ifndef HAVE_WINDOWS_H
    int n;
    OFF_T uOffset;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);

    /* save the sender's request to send back with the reload packet */
    /*reload_pkt->req = rreq->dev.rdma_request;*/
    reload_pkt->sreq = rreq->dev.sender_req_id;
    reload_pkt->rreq = rreq->handle;

#ifndef HAVE_WINDOWS_H
    mpi_errno = MPIDI_SHM_AttachProc( vc->ch.nSharedProcessID );
    if (mpi_errno) {
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
	return mpi_errno;
    }
#endif
    for (;;)
    {
	recv_iov = rreq->dev.iov;
	recv_count = rreq->dev.iov_count;
	send_iov = rreq->dev.rdma_iov;
	send_count = rreq->dev.rdma_iov_count;
	siov_offset = rreq->dev.rdma_iov_offset;

	/*
	printf("shm_rdma: reading %d send buffers into %d recv buffers.\n", send_count, recv_count);fflush(stdout);
	for (i=siov_offset; i<send_count; i++)
	{
	    printf("shm_rdma: send buf[%d] = %p, len = %d\n", i, send_iov[i].MPID_IOV_BUF, send_iov[i].MPID_IOV_LEN);
	}
	for (i=0; i<recv_count; i++)
	{
	    printf("shm_rdma: recv buf[%d] = %p, len = %d\n", i, recv_iov[i].MPID_IOV_BUF, recv_iov[i].MPID_IOV_LEN);
	}
	fflush(stdout);
	*/
	sbuf = send_iov[siov_offset].MPID_IOV_BUF;
	sbuf_len = send_iov[siov_offset].MPID_IOV_LEN;
	for (i=rreq->ch.iov_offset; i<recv_count; i++)
	{
	    rbuf = recv_iov[i].MPID_IOV_BUF;
	    rbuf_len = recv_iov[i].MPID_IOV_LEN;
	    while (rbuf_len)
	    {
		len = MPIDU_MIN(rbuf_len, sbuf_len);
		/*printf("reading %d bytes from the remote process.\n", len);fflush(stdout);*/
#ifdef HAVE_WINDOWS_H
		if (!ReadProcessMemory(vc->ch.hSharedProcessHandle, sbuf, rbuf, len, &num_read))
		{
		    /*printf("ReadProcessMemory failed, error %d\n", GetLastError());*/
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "ReadProcessMemory failed", GetLastError());
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
		    return mpi_errno;
		}
		if (num_read == -1)
		{
		    /*printf("ReadProcessMemory read -1 bytes.\n");fflush(stdout);*/
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "ReadProcessMemory returned -1 bytes written");
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
		    return mpi_errno;
		}
#else
		uOffset = lseek(vc->ch.nSharedProcessFileDescriptor, OFF_T_CAST(sbuf), SEEK_SET);
		if (uOffset != OFF_T_CAST(sbuf))
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "lseek failed", errno);
		    MPIDI_SHM_DetachProc( vc->ch.nSharedProcessID );
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
		    return mpi_errno;
		}

		num_read = read(vc->ch.nSharedProcessFileDescriptor, rbuf, len);
		if (num_read < 1)
		{
		    if (num_read == -1)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "read failed", errno);
			MPIDI_SHM_DetachProc( vc->ch.nSharedProcessID );
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
			return mpi_errno;
		    }
		    ptrace(PTRACE_PEEKDATA, vc->ch.nSharedProcessID, sbuf + len - num_read, 0);
		}
#endif
		/*printf("read %d bytes from the remote process\n", num_read);fflush(stdout);*/
		if (num_read < (SIZE_T)sbuf_len)
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
#ifndef HAVE_WINDOWS_H
			mpi_errno = MPIDI_SHM_DetachProc( vc->ch.nSharedProcessID );
			if (mpi_errno) {
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
			    return mpi_errno;
			}
#endif
			/* the send iov needs to be reloaded */
			if (rbuf_len != 0)
			{
			    rreq->dev.iov[i].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rbuf;
			    rreq->dev.iov[i].MPID_IOV_LEN = rbuf_len;
			}
			rreq->ch.iov_offset = i;

			/* send the reload packet to the sender */
			/*printf("sending reload packet to the sender.\n");fflush(stdout);*/
			MPIDI_Pkt_init(reload_pkt, MPIDI_CH3_PKT_RELOAD);
			reload_pkt->send_recv = MPIDI_CH3_PKT_RELOAD_SEND;
			mpi_errno = MPIDI_CH3_iStartMsg(vc, reload_pkt, sizeof(*reload_pkt), &reload_rreq);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */
			if (reload_rreq != NULL)
			{
			    /* The sender doesn't need to know when the packet has been sent.  So release the request immediately */
			    MPID_Request_release(reload_rreq);
			}
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
			return MPI_SUCCESS;
		    }
		}
	    }
	}

#ifndef HAVE_WINDOWS_H
	mpi_errno = MPIDI_SHM_DetachProc( vc->ch.nSharedProcessID );
	if (mpi_errno) {
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
	    return mpi_errno;
	}
#endif

	/* update the sender's request */
	mpi_errno = MPIDI_CH3U_Handle_recv_req(vc, rreq, &complete);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update request after rdma read");
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
	    return mpi_errno;
	}

	if (complete || (siov_offset == send_count))
	{
	    /*printf("sending reload send packet.\n");fflush(stdout);*/
	    /* send the reload/done packet to the sender */
	    MPIDI_Pkt_init(reload_pkt, MPIDI_CH3_PKT_RELOAD);
	    reload_pkt->send_recv = MPIDI_CH3_PKT_RELOAD_SEND;
	    mpi_errno = MPIDI_CH3_iStartMsg(vc, reload_pkt, sizeof(*reload_pkt), &reload_rreq);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
		return mpi_errno;
	    }
	    /* --END ERROR HANDLING-- */
	    if (reload_rreq != NULL)
	    {
		/* The sender doesn't need to know when the packet has been sent.  So release the request immediately */
		MPID_Request_release(reload_rreq);
	    }
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
	    return MPI_SUCCESS;
	}

	rreq->dev.rdma_iov_offset = siov_offset;
	rreq->dev.rdma_iov[siov_offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)sbuf;
	rreq->dev.rdma_iov[siov_offset].MPID_IOV_LEN = sbuf_len;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
    return mpi_errno;
#else
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RDMA_READV);
    return mpi_errno;
#endif
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
#ifdef MPIDI_CH3_CHANNEL_RNDV
    MPID_Request *sreq, *rreq;
    int complete;
    int i;
#endif
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
#ifdef MPIDI_CH3_CHANNEL_RNDV
		if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type > MPIDI_CH3I_PKT_SC_CLOSE/*MPIDI_CH3_PKT_END_CH3*/)
		{
		    if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_RTS_IOV)
		    {
			/*printf("received rts packet.\n");fflush(stdout);*/
			rreq = MPID_Request_create();
			if (rreq == NULL)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			    return mpi_errno;
			}
			MPIU_Object_set_ref(rreq, 1);
			rreq->kind = MPIDI_CH3I_RTS_IOV_READ_REQUEST;
			rreq->dev.rdma_request = ((MPIDI_CH3_Pkt_rdma_rts_iov_t*)mem_ptr)->sreq;
			rreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_rts_iov_t*)mem_ptr)->iov_len;
			rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->dev.rdma_iov;
			rreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			rreq->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->ch.pkt;
			rreq->dev.iov[1].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
			rreq->dev.iov_count = 2;
			rreq->ch.req = NULL;
			recv_vc_ptr->ch.recv_active = rreq;
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_CTS_IOV)
		    {
			/*printf("received cts packet.\n");fflush(stdout);*/
			MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->sreq, sreq);
			sreq->dev.rdma_request = ((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->rreq;
			sreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->iov_len;
			rreq = MPID_Request_create();
			if (rreq == NULL)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			    return mpi_errno;
			}
			MPIU_Object_set_ref(rreq, 1);
			rreq->kind = MPIDI_CH3I_IOV_WRITE_REQUEST;
			rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->dev.rdma_iov;
			rreq->dev.iov[0].MPID_IOV_LEN = sreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			rreq->dev.iov_count = 1;
			rreq->ch.req = sreq;
			recv_vc_ptr->ch.recv_active = rreq;
			/*MPIDI_CH3I_SHM_post_read(recv_vc_ptr, &sreq->ch.rdma_iov, sreq->ch.rdma_iov_count * sizeof(MPID_IOV), NULL);*/
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_IOV)
		    {
			if ( ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_SEND )
			{
			    /*printf("received sender's iov packet, posting a read of %d iovs.\n", ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len);fflush(stdout);*/
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->req, sreq);
			    sreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len;
			    rreq = MPID_Request_create();
			    if (rreq == NULL)
			    {
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
				return mpi_errno;
			    }
			    MPIU_Object_set_ref(rreq, 1);
			    rreq->kind = MPIDI_CH3I_IOV_READ_REQUEST;
			    rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->dev.rdma_iov;
			    rreq->dev.iov[0].MPID_IOV_LEN = sreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			    rreq->dev.iov_count = 1;
			    rreq->ch.req = sreq;
			    recv_vc_ptr->ch.recv_active = rreq;
			}
			else if ( ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_RECV )
			{
			    /*printf("received receiver's iov packet, posting a read of %d iovs.\n", ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len);fflush(stdout);*/
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->req, rreq);
			    rreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len;
			    sreq = MPID_Request_create();
			    if (sreq == NULL)
			    {
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
				return mpi_errno;
			    }
			    MPIU_Object_set_ref(sreq, 1);
			    sreq->kind = MPIDI_CH3I_IOV_WRITE_REQUEST;
			    sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->dev.rdma_iov;
			    sreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			    sreq->dev.iov_count = 1;
			    sreq->ch.req = rreq;
			    recv_vc_ptr->ch.recv_active = sreq;
			}
			else
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "received invalid MPIDI_CH3_PKT_IOV packet");
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			    return mpi_errno;
			}
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_RELOAD)
		    {
			if (((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_SEND)
			{
			    /*printf("received reload send packet.\n");fflush(stdout);*/
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->sreq, sreq);
			    mpi_errno = MPIDI_CH3U_Handle_send_req(recv_vc_ptr, sreq, &complete);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update send request after receiving a reload packet");
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
				return mpi_errno;
			    }
			    if (!complete)
			    {
				/* send a new iov */
				MPID_Request * rts_sreq;
				MPIDI_CH3_Pkt_t pkt;

				/*printf("sending reloaded send iov of length %d\n", sreq->dev.iov_count);fflush(stdout);*/
				MPIDI_Pkt_init(&pkt.iov, MPIDI_CH3_PKT_IOV);
				pkt.iov.send_recv = MPIDI_CH3_PKT_RELOAD_SEND;
				pkt.iov.req = ((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->rreq;
				pkt.iov.iov_len = sreq->dev.iov_count;

				sreq->dev.rdma_iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&pkt;
				sreq->dev.rdma_iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
				sreq->dev.rdma_iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)sreq->dev.iov;
				sreq->dev.rdma_iov[1].MPID_IOV_LEN = sreq->dev.iov_count * sizeof(MPID_IOV);

				mpi_errno = MPIDI_CH3_iStartMsgv(recv_vc_ptr, sreq->dev.rdma_iov, 2, &rts_sreq);
				/* --BEGIN ERROR HANDLING-- */
				if (mpi_errno != MPI_SUCCESS)
				{
				    MPIU_Object_set_ref(sreq, 0);
				    MPIDI_CH3_Request_destroy(sreq);
				    sreq = NULL;
				    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
				    return mpi_errno;
				}
				/* --END ERROR HANDLING-- */
				if (rts_sreq != NULL)
				{
				    /* The sender doesn't need to know when the message has been sent.  So release the request immediately */
				    MPID_Request_release(rts_sreq);
				}
			    }
			}
			else if (((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_RECV)
			{
			    /*printf("received reload recv packet.\n");fflush(stdout);*/
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->rreq, rreq);
			    mpi_errno = MPIDI_CH3U_Handle_recv_req(recv_vc_ptr, rreq, &complete);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update request after receiving a reload packet");
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
				return mpi_errno;
			    }
			    if (!complete)
			    {
				/* send a new iov */
				MPID_Request * cts_sreq;
				MPIDI_CH3_Pkt_t pkt;

				/*printf("sending reloaded recv iov of length %d\n", rreq->dev.iov_count);fflush(stdout);*/
				MPIDI_Pkt_init(&pkt.iov, MPIDI_CH3_PKT_IOV);
				pkt.iov.send_recv = MPIDI_CH3_PKT_RELOAD_RECV;
				pkt.iov.req = ((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->sreq;
				pkt.iov.iov_len = rreq->dev.iov_count;

				rreq->dev.rdma_iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&pkt;
				rreq->dev.rdma_iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
				rreq->dev.rdma_iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rreq->dev.iov;
				rreq->dev.rdma_iov[1].MPID_IOV_LEN = rreq->dev.iov_count * sizeof(MPID_IOV);

				mpi_errno = MPIDI_CH3_iStartMsgv(recv_vc_ptr, rreq->dev.rdma_iov, 2, &cts_sreq);
				/* --BEGIN ERROR HANDLING-- */
				if (mpi_errno != MPI_SUCCESS)
				{
				    /* This destruction probably isn't correct. */
				    /* I think it needs to save the error in the request, complete the request and return */
				    MPIU_Object_set_ref(rreq, 0);
				    MPIDI_CH3_Request_destroy(rreq);
				    rreq = NULL;
				    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|ctspkt", 0);
				    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
				    return mpi_errno;
				}
				/* --END ERROR HANDLING-- */
				if (cts_sreq != NULL)
				{
				    /* The sender doesn't need to know when the message has been sent.  So release the request immediately */
				    MPID_Request_release(cts_sreq);
				}
			    }
			}
			else
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			    return mpi_errno;
			}

			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.shm_reading_pkt = TRUE;
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
			}
			/* return from the wait */
			*num_bytes_ptr = 0;
			*vc_pptr = recv_vc_ptr;
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			return MPI_SUCCESS;
		    }
		    else
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "shared memory read progress unable to handle unknown rdma packet");
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			return mpi_errno;
		    }
		}
		else
#endif /* MPIDI_CH3_CHANNEL_RNDV */
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
#ifdef MPIDI_CH3_CHANNEL_RNDV
		    else if (recv_vc_ptr->ch.recv_active->kind == MPIDI_CH3I_RTS_IOV_READ_REQUEST)
		    {
			int found;
			/*printf("received rts iov_read.\n");fflush(stdout);*/

			mpi_errno = MPIDI_CH3U_Handle_recv_rndv_pkt(recv_vc_ptr,
								    &recv_vc_ptr->ch.recv_active->ch.pkt,
								    &rreq, &found);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "shared memory read progress unable to handle incoming rts(get) packet");
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

			for (i=0; i<recv_vc_ptr->ch.recv_active->dev.rdma_iov_count; i++)
			{
			    rreq->dev.rdma_iov[i].MPID_IOV_BUF = recv_vc_ptr->ch.recv_active->dev.rdma_iov[i].MPID_IOV_BUF;
			    rreq->dev.rdma_iov[i].MPID_IOV_LEN = recv_vc_ptr->ch.recv_active->dev.rdma_iov[i].MPID_IOV_LEN;
			}
			rreq->dev.rdma_iov_count = recv_vc_ptr->ch.recv_active->dev.rdma_iov_count;

			if (found)
			{
			    if (rreq->dev.recv_data_sz == 0) {
				MPIDI_CH3U_Request_complete(rreq);
				rreq = NULL;
			    }
			    else {
				mpi_errno = MPIDI_CH3U_Post_data_receive_found(rreq);
				/* --BEGIN ERROR HANDLING-- */
				if (mpi_errno != MPI_SUCCESS)
				    {
					MPIU_ERR_SET1(mpi_errno,MPI_ERR_OTHER,
						      "**ch3|postrecv",
						      "**ch3|postrecv %s",
				      "MPIDI_CH3_PKT_RNDV_REQ_TO_SEND");
					MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
					return mpi_errno;
				    }
			    }
			    /* --END ERROR HANDLING-- */
			    mpi_errno = MPIDI_CH3_iStartRndvTransfer(recv_vc_ptr, rreq);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL,
				    FCNAME, __LINE__,
				    MPI_ERR_OTHER,
				    "**ch3|ctspkt", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
				return mpi_errno;
			    }
			    /* --END ERROR HANDLING-- */
			}

			rreq = recv_vc_ptr->ch.recv_active;
			/* free the request used to receive the rts packet and iov data */
			MPIU_Object_set_ref(rreq, 0);
			MPIDI_CH3_Request_destroy(rreq);

			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.shm_reading_pkt = TRUE;
		    }
		    else if (recv_vc_ptr->ch.recv_active->kind == MPIDI_CH3I_IOV_READ_REQUEST)
		    {
			/*printf("received iov_read.\n");fflush(stdout);*/
			rreq = recv_vc_ptr->ch.recv_active;

			mpi_errno = MPIDI_CH3_iStartRndvTransfer(recv_vc_ptr, rreq->ch.req);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "shared memory read progress unable to handle incoming rts(get) iov");
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.shm_reading_pkt = TRUE;

			/* free the request used to receive the iov data */
			MPIU_Object_set_ref(rreq, 0);
			MPIDI_CH3_Request_destroy(rreq);
		    }
		    else if (recv_vc_ptr->ch.recv_active->kind == MPIDI_CH3I_IOV_WRITE_REQUEST)
		    {
			/*printf("received iov_write.\n");fflush(stdout);*/
			mpi_errno = MPIDI_CH3I_SHM_rdma_writev(recv_vc_ptr, recv_vc_ptr->ch.recv_active->ch.req);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

			/* return from the wait */
			MPID_Request_release(recv_vc_ptr->ch.recv_active);
			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.shm_reading_pkt = TRUE;
			*num_bytes_ptr = 0;
			*vc_pptr = recv_vc_ptr;
			MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_READ_PROGRESS);
			return MPI_SUCCESS;
		    }
#endif /* MPIDI_CH3_CHANNEL_RNDV */
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
