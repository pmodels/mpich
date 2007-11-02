/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ch3i_progress.h"

volatile unsigned int MPIDI_CH3I_progress_completion_count = 0;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connection_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;

    /* There is no post_close for shm connections so handle them as closed immediately. */
    MPIDI_CH3I_SHM_Remove_vc_references(vc);
    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Remove_vc_read_references
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void MPIDI_CH3I_SHM_Remove_vc_read_references(MPIDI_VC_t *vc)
{
    MPIDI_VC_t *iter, *trailer;

    /* remove vc from the reading list */
    iter = trailer = MPIDI_CH3I_Process.shm_reading_list;
    while (iter != NULL)
    {
	if (iter == vc)
	{
	    /* Mark the connection as NOT read connected so it won't be mistaken for active */
	    /* Since shm connections are uni-directional the following three states are considered active:
	     * MPIDI_VC_STATE_ACTIVE + ch.shm_read_connected = 0 - active in the write direction
	     * MPIDI_VC_STATE_INACTIVE + ch.shm_read_connected = 1 - active in the read direction
	     * MPIDI_VC_STATE_ACTIVE + ch.shm_read_connected = 1 - active in both directions
	     */
	    vc->ch.shm_read_connected = 0;
	    if (trailer != iter)
	    {
		/* remove the vc from the list */
		trailer->ch.shm_next_reader = iter->ch.shm_next_reader;
	    }
	    else
	    {
		/* remove the vc from the head of the list */
		MPIDI_CH3I_Process.shm_reading_list = MPIDI_CH3I_Process.shm_reading_list->ch.shm_next_reader;
	    }
	}
	if (trailer != iter)
	    trailer = trailer->ch.shm_next_reader;
	iter = iter->ch.shm_next_reader;
    }
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Remove_vc_write_references
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void MPIDI_CH3I_SHM_Remove_vc_write_references(MPIDI_VC_t *vc)
{
    MPIDI_VC_t *iter, *trailer;

    /* remove the vc from the writing list */
    iter = trailer = MPIDI_CH3I_Process.shm_writing_list;
    while (iter != NULL)
    {
	if (iter == vc)
	{
	    if (trailer != iter)
	    {
		/* remove the vc from the list */
		trailer->ch.shm_next_writer = iter->ch.shm_next_writer;
	    }
	    else
	    {
		/* remove the vc from the head of the list */
		MPIDI_CH3I_Process.shm_writing_list = MPIDI_CH3I_Process.shm_writing_list->ch.shm_next_writer;
	    }
	}
	if (trailer != iter)
	    trailer = trailer->ch.shm_next_writer;
	iter = iter->ch.shm_next_writer;
    }
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Remove_vc_references
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_SHM_Remove_vc_references(MPIDI_VC_t *vc)
{
    MPIDI_CH3I_SHM_Remove_vc_read_references(vc);
    MPIDI_CH3I_SHM_Remove_vc_write_references(vc);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Add_to_reader_list
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_SHM_Add_to_reader_list(MPIDI_VC_t *vc)
{
    MPIDI_CH3I_SHM_Remove_vc_read_references(vc);
    vc->ch.shm_next_reader = MPIDI_CH3I_Process.shm_reading_list;
    MPIDI_CH3I_Process.shm_reading_list = vc;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Add_to_writer_list
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_SHM_Add_to_writer_list(MPIDI_VC_t *vc)
{
    MPIDI_CH3I_SHM_Remove_vc_write_references(vc);
    vc->ch.shm_next_writer = MPIDI_CH3I_Process.shm_writing_list;
    MPIDI_CH3I_Process.shm_writing_list = vc;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Shm_connect(MPIDI_VC_t *vc, const char *business_card, int *flag)
{
    int mpi_errno;
    char hostname[256];
    char queue_name[100];
    MPIDI_CH3I_BootstrapQ queue;
    MPIDI_CH3I_Shmem_queue_info shm_info;
    int i;
#ifdef HAVE_SHARED_PROCESS_READ
    char pid_str[20];
#ifndef HAVE_WINDOWS_H
    char filename[256];
#endif
#endif

    /* get the host and queue from the business card */
    mpi_errno = MPIU_Str_get_string_arg(business_card, MPIDI_CH3I_SHM_HOST_KEY, hostname, 256);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	/*MPIU_DBG_PRINTF(("getstringarg(%s, %s) failed.\n", MPIDI_CH3I_SHM_HOST_KEY, business_card));*/
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_shmhost", 0);
	return mpi_errno;
    }

    mpi_errno = MPIU_Str_get_string_arg(business_card, MPIDI_CH3I_SHM_QUEUE_KEY, queue_name, 100);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_shmq", 0);
	return mpi_errno;
    }

#ifdef HAVE_SHARED_PROCESS_READ
    mpi_errno = MPIU_Str_get_string_arg(business_card, MPIDI_CH3I_SHM_PID_KEY, pid_str, 20);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_shmpid", 0);
	return mpi_errno;
    }
#endif

    /* compare this host's name with the business card host name */
    if (strcmp(MPIDI_Process.my_pg->ch.shm_hostname, hostname) != 0)
    {
	*flag = FALSE;
	/*MPIU_DBG_PRINTF(("%s != %s\n", MPIDI_Process.my_pg->ch.shm_hostname, hostname));*/
	return MPI_SUCCESS;
    }

    *flag = TRUE;
    /*MPIU_DBG_PRINTF(("%s == %s\n", MPIDI_Process.my_pg->ch.shm_hostname, hostname));*/

    MPIU_DBG_PRINTF(("attaching to queue: %s\n", queue_name));
    mpi_errno = MPIDI_CH3I_BootstrapQ_attach(queue_name, &queue);
    if (mpi_errno != MPI_SUCCESS)
    {
	*flag = FALSE;
	return MPI_SUCCESS;
    }

    /* create the write queue */
    mpi_errno = MPIDI_CH3I_SHM_Get_mem(sizeof(MPIDI_CH3I_SHM_Queue_t), &vc->ch.shm_write_queue_info);
    if (mpi_errno != MPI_SUCCESS)
    {
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmconnect_getmem", 0);
	return mpi_errno;
    }
    /* MPIU_DBG_PRINTF(("rank %d sending queue(%s) to rank %d\n", MPIR_Process.comm_world->rank, vc->ch.shm_write_queue_info.name,
       vc->ch.pg_rank)); */
    
    vc->ch.write_shmq = vc->ch.shm_write_queue_info.addr;
    vc->ch.write_shmq->head_index = 0;
    vc->ch.write_shmq->tail_index = 0;
    MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq head = 0"));
    MPIDI_DBG_PRINTF((60, FCNAME, "write_shmq tail = 0"));
    for (i=0; i<MPIDI_CH3I_NUM_PACKETS; i++)
    {
	vc->ch.write_shmq->packet[i].offset = 0;
	vc->ch.write_shmq->packet[i].avail = MPIDI_CH3I_PKT_EMPTY;
#ifdef MPICH_DBG_OUTPUT
	memset(vc->ch.write_shmq->packet[i].data, 0, MPIDI_CH3I_PACKET_SIZE);
#endif
    }

    /* send the queue connection information */
    /*MPIU_DBG_PRINTF(("write_shmq: %p, name - %s\n", vc->ch.write_shmq, vc->ch.shm_write_queue_info.key));*/
    shm_info.info = vc->ch.shm_write_queue_info;
    MPIU_Strncpy(shm_info.pg_id, MPIDI_Process.my_pg->id, 100);
    shm_info.pg_rank = MPIDI_Process.my_pg_rank;
    shm_info.pid = getpid();
    MPIU_DBG_PRINTF(("MPIDI_CH3I_Shm_connect: sending bootstrap queue info from rank %d to msg queue %s\n", MPIR_Process.comm_world->rank, queue_name));
    mpi_errno = MPIDI_CH3I_BootstrapQ_send_msg(queue, &shm_info, sizeof(shm_info));
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIDI_CH3I_SHM_Unlink_mem(&vc->ch.shm_write_queue_info);
	MPIDI_CH3I_SHM_Release_mem(&vc->ch.shm_write_queue_info);
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_send", 0);
	return mpi_errno;
    }

    /* MPIU_Free the queue resource */
    /*MPIU_DBG_PRINTF(("detaching from queue: %s\n", queue_name));*/
    mpi_errno = MPIDI_CH3I_BootstrapQ_detach(queue);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIDI_CH3I_SHM_Unlink_mem(&vc->ch.shm_write_queue_info);
	MPIDI_CH3I_SHM_Release_mem(&vc->ch.shm_write_queue_info);
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_detach", 0);
	return mpi_errno;
    }

#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
    /*MPIU_DBG_PRINTF(("Opening process[%d]: %d\n", i, pSharedProcess[i].nPid));*/
    vc->ch.hSharedProcessHandle =
	OpenProcess(STANDARD_RIGHTS_REQUIRED | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, 
	FALSE, atoi(pid_str));
    if (vc->ch.hSharedProcessHandle == NULL)
    {
	int err = GetLastError();
	MPIDI_CH3I_SHM_Unlink_mem(&vc->ch.shm_write_queue_info);
	MPIDI_CH3I_SHM_Release_mem(&vc->ch.shm_write_queue_info);
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**OpenProcess", "**OpenProcess %d %d", i, err);
	return mpi_errno;
    }
#else
    MPIU_Snprintf(filename, 256, "/proc/%s/mem", pid_str);
    vc->ch.nSharedProcessID = atoi(pid_str);
    vc->ch.nSharedProcessFileDescriptor = open(filename, O_RDWR/*O_RDONLY*/);
    if (vc->ch.nSharedProcessFileDescriptor == -1)
    {
	MPIDI_CH3I_SHM_Unlink_mem(&vc->ch.shm_write_queue_info);
	MPIDI_CH3I_SHM_Release_mem(&vc->ch.shm_write_queue_info);
	*flag = FALSE;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**open", "**open %s %d %d", filename, atoi(pid_str), errno);
	return mpi_errno;
    }
#endif
#endif

    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t * vc)
{
    char val[MPIDI_MAX_KVS_VALUE_LEN];
    int rc;
    int mpi_errno = MPI_SUCCESS;
    int connected;
    MPIDI_VC_t *iter;
    int count = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    if (vc->ch.state != MPIDI_CH3I_VC_STATE_UNCONNECTED)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**vc_state", "**vc_state %d", vc->ch.state);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }

    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;

    mpi_errno = MPIDI_PG_GetConnString( vc->pg, vc->pg_rank, val, sizeof(val));
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
	return mpi_errno;
    }

    MPIU_DBG_PRINTF(("%d: %s\n", vc->pg_rank, val));

    /* attempt to connect through shared memory */
    connected = FALSE;
    MPIU_DBG_PRINTF(("business card: <%d> = <%s>\n", vc->pg_rank, val));
    mpi_errno = MPIDI_CH3I_Shm_connect(vc, val, &connected);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**post_connect", "**post_connect %s", "MPIDI_CH3I_Shm_connect");
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }
    if (!connected)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to establish a shared memory queue connection");
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }

    MPIDI_CH3I_SHM_Add_to_writer_list(vc);

    /* If there are more shm connections than cpus, reduce the spin count to one. */
    /* This does not take into account connections between other processes on the same machine. */
    iter = MPIDI_CH3I_Process.shm_writing_list;
    while (iter)
    {
	count++;
	iter = iter->ch.shm_next_writer;
    }
    if (count >= MPIDI_CH3I_Process.num_cpus)
	MPIDI_Process.my_pg->ch.nShmWaitSpinCount = 1;

    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
    vc->ch.shm_reading_pkt = TRUE;
    vc->ch.send_active = MPIDI_CH3I_SendQ_head(vc); /* MT */

    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
}
