/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* 
 * This file contains code that is no longer used.  This code may have been 
 * used for some experiments.  Any documentation on this code is included in 
 * this file.
 */
#ifdef USE_SHM_UNEX

#undef FUNCNAME
#define FUNCNAME shmi_buffer_unex_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int shmi_buffer_unex_read(MPIDI_VC_t *vc_ptr, 
				 MPIDI_CH3I_SHM_Packet_t *pkt_ptr, 
				 void *mem_ptr, unsigned int offset, 
				 unsigned int num_bytes)
{
    MPIDI_CH3I_SHM_Unex_read_t *p;
    MPIDI_STATE_DECL(MPID_STATE_SHMI_BUFFER_UNEX_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_SHMI_BUFFER_UNEX_READ);

    MPIDI_DBG_PRINTF((60, FCNAME, "%d bytes\n", num_bytes));

    p = (MPIDI_CH3I_SHM_Unex_read_t *)
	MPIU_Malloc(sizeof(MPIDI_CH3I_SHM_Unex_read_t));
    p->pkt_ptr = pkt_ptr;
    p->buf = (unsigned char *)mem_ptr + offset;
    p->length = num_bytes;
    p->next = vc_ptr->ch.unex_list;
    vc_ptr->ch.unex_list = p;

    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_BUFFER_UNEX_READ);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME shmi_read_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int shmi_read_unex(MPIDI_VC_t *vc_ptr)
{
    unsigned int len;
    MPIDI_CH3I_SHM_Unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_SHMI_READ_UNEX);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_SHMI_READ_UNEX);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    MPIU_Assert(vc_ptr->ch.unex_list);

    /* copy the received data */
    while (vc_ptr->ch.unex_list)
    {
	len = min(vc_ptr->ch.unex_list->length, vc_ptr->ch.read.bufflen);
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	memcpy(vc_ptr->ch.read.buffer, vc_ptr->ch.unex_list->buf, len);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	/* advance the user pointer */
	vc_ptr->ch.read.buffer = (char*)(vc_ptr->ch.read.buffer) + len;
	vc_ptr->ch.read.bufflen -= len;
	vc_ptr->ch.read.total += len;
	if (len != vc_ptr->ch.unex_list->length)
	{
	    vc_ptr->ch.unex_list->length -= len;
	    vc_ptr->ch.unex_list->buf += len;
	}
	else
	{
	    /* put the receive packet back in the pool */
	    MPIU_Assert(vc_ptr->ch.unex_list->pkt_ptr != NULL);
	    vc_ptr->ch.unex_list->pkt_ptr->cur_pos = 
		vc_ptr->ch.unex_list->pkt_ptr->data;
	    vc_ptr->ch.unex_list->pkt_ptr->avail = MPIDI_CH3I_PKT_EMPTY;
	    /* MPIU_Free the unexpected data node */
	    temp = vc_ptr->ch.unex_list;
	    vc_ptr->ch.unex_list = vc_ptr->ch.unex_list->next;
	    MPIU_Free(temp);
	}
	/* check to see if the entire message was received */
	if (vc_ptr->ch.read.bufflen == 0)
	{
	    /* place this vc_ptr in the finished list so it will be 
	       completed by shm_wait */
	    vc_ptr->ch.shm_state &= ~SHM_READING_BIT;
	    vc_ptr->ch.unex_finished_next = 
		MPIDI_CH3I_Process.unex_finished_list;
	    MPIDI_CH3I_Process.unex_finished_list = vc_ptr;
	    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READ_UNEX);
	    return MPI_SUCCESS;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READ_UNEX);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME shmi_readv_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int shmi_readv_unex(MPIDI_VC_t *vc_ptr)
{
    unsigned int num_bytes;
    MPIDI_CH3I_SHM_Unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_SHMI_READV_UNEX);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_SHMI_READV_UNEX);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    while (vc_ptr->ch.unex_list)
    {
	while (vc_ptr->ch.unex_list->length && vc_ptr->ch.read.iovlen)
	{
	    num_bytes = min(vc_ptr->ch.unex_list->length, 
		    vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_LEN);
	    MPIDI_DBG_PRINTF((60, FCNAME, "copying %d bytes\n", num_bytes));
	    /* copy the received data */
	    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy(vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_BUF, 
		   vc_ptr->ch.unex_list->buf, num_bytes);
	    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
	    vc_ptr->ch.read.total += num_bytes;
	    vc_ptr->ch.unex_list->buf += num_bytes;
	    vc_ptr->ch.unex_list->length -= num_bytes;
	    /* update the iov */
	    vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_LEN -= 
		num_bytes;
	    vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_BUF = 
		(MPID_IOV_BUF_CAST)((char*)
		 (vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_BUF) + 
				    num_bytes);
	    if (vc_ptr->ch.read.iov[vc_ptr->ch.read.index].MPID_IOV_LEN == 0)
	    {
		vc_ptr->ch.read.index++;
		vc_ptr->ch.read.iovlen--;
	    }
	}

	if (vc_ptr->ch.unex_list->length == 0)
	{
	    /* put the receive packet back in the pool */
	    MPIU_Assert(vc_ptr->ch.unex_list->pkt_ptr != NULL);
	    vc_ptr->ch.unex_list->pkt_ptr->cur_pos = 
		vc_ptr->ch.unex_list->pkt_ptr->data;
	    vc_ptr->ch.unex_list->pkt_ptr->avail = MPIDI_CH3I_PKT_EMPTY;
	    /* MPIU_Free the unexpected data node */
	    temp = vc_ptr->ch.unex_list;
	    vc_ptr->ch.unex_list = vc_ptr->ch.unex_list->next;
	    MPIU_Free(temp);
	}
	
	if (vc_ptr->ch.read.iovlen == 0)
	{
	    vc_ptr->ch.shm_state &= ~SHM_READING_BIT;
	    vc_ptr->ch.unex_finished_next = 
		MPIDI_CH3I_Process.unex_finished_list;
	    MPIDI_CH3I_Process.unex_finished_list = vc_ptr;
	    MPIDI_DBG_PRINTF((60, FCNAME, 
            "finished read saved in MPIDI_CH3I_Process.unex_finished_list\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READV_UNEX);
	    return MPI_SUCCESS;
	}
    }
    MPIDI_FUNC_EXIT(MPID_STATE_SHMI_READV_UNEX);
    return MPI_SUCCESS;
}

#endif /* USE_SHM_UNEX */

/* This block is from read_progress in the first (for (;;)) loop */
#ifdef USE_SHM_UNEX
	MPIDI_VC_t *temp_vc_ptr;
	if (MPIDI_CH3I_Process.unex_finished_list)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, 
			"returning previously received %d bytes", 
			MPIDI_CH3I_Process.unex_finished_list->ch.read.total));

	    *num_bytes_ptr = 
		MPIDI_CH3I_Process.unex_finished_list->ch.read.total;
	    *vc_pptr = MPIDI_CH3I_Process.unex_finished_list;
	    /* remove this vc from the finished list */
	    temp_vc_ptr = MPIDI_CH3I_Process.unex_finished_list;
	    MPIDI_CH3I_Process.unex_finished_list = 
		MPIDI_CH3I_Process.unex_finished_list->ch.unex_finished_next;
	    temp_vc_ptr->ch.unex_finished_next = NULL;

	    *shm_out = SHM_WAIT_READ;
	    return MPI_SUCCESS;
	}
#endif /* USE_SHM_UNEX */

/* dead code */
#ifdef USE_SHM_UNEX
		/* Should we buffer unexpected messages or leave them in the 
		   shmem queue? */
		/*shmi_buffer_unex_read(recv_vc_ptr, pkt_ptr, mem_ptr, 0, 
		  num_bytes);*/
#endif

#ifdef USE_SHM_UNEX
    if (vc->ch.unex_list)
	shmi_read_unex(vc);
#endif

#ifdef USE_SHM_UNEX
    if (vc->ch.unex_list)
	shmi_readv_unex(vc);
#endif
