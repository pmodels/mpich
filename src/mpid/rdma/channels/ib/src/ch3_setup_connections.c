/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "ibu.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Setup_connections
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Setup_connections()
{
    int mpi_errno;
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    int i, dlid;
    int qp_num, dest_qp_num;
    MPIDI_CH3I_Process_group_t * pg;
    int pg_rank;
    MPIDI_VC *vc;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);

    pg_rank = MPIR_Process.comm_world->rank;
    pg = MPIDI_CH3I_Process.pg;

    mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**fail",
					 "**fail %d", mpi_errno);
	return mpi_errno;
    }
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
				       __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**fail",
					 "**fail %d", mpi_errno);
	return mpi_errno;
    }
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
				       __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    /* create a queue pair for each process */
    for (i=0; i<MPIDI_CH3I_Process.pg->size; i++)
    {
	vc = &MPIDI_CH3I_Process.pg->vc_table[i];

	if (vc->ch.pg_rank == pg_rank)
	    continue;

	/* create the qp */
	MPIU_DBG_PRINTF(("calling ibu_start_qp\n"));
	vc->ch.ibu = ibu_start_qp(MPIDI_CH3I_Process.set, &qp_num);
	if (vc->ch.ibu == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**nomem", 0);
	    return mpi_errno;
	}
	/* set the user pointer to be a pointer to the VC */
	ibu_set_vc_ptr(vc->ch.ibu, &MPIDI_CH3I_Process.pg->vc_table[i]);
	mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d:%d-qp", pg_rank, i);
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**snprintf", "**snprintf %d",
					     mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = snprintf(val, val_max_sz, "%d", qp_num);
	if (mpi_errno < 0 || mpi_errno > val_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**snprintf", "**snprintf %d",
					     mpi_errno);
	    return mpi_errno;
	}
	MPIU_DBG_PRINTF(("putting qp_num (%s => %s)\n", key, val));
	mpi_errno = PMI_KVS_Put(pg->kvs_name, key, val);
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**pmi_kvs_put", "**pmi_kvs_put %d",
					     mpi_errno);
	    return mpi_errno;
	}
	/*MPIU_DBG_PRINTF(("Published qp_num = %d\n", qp_num));*/
    }

    /* commit the qp_num puts and barrier so gets can happen */
    mpi_errno = PMI_KVS_Commit(pg->kvs_name);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**pmi_kvs_commit", "**pmi_kvs_commit %d",
					 mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Barrier();
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**pmi_barrier", "**pmi_barrier %d",
					 mpi_errno);
	return mpi_errno;
    }

    /* complete the queue pairs and post the receives */
    for (i=0; i<MPIDI_CH3I_Process.pg->size; i++)
    {
	vc = &MPIDI_CH3I_Process.pg->vc_table[i];

	if (vc->ch.pg_rank == pg_rank)
	    continue;

	/* get the destination lid from the pmi database */
	mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-lid", vc->ch.pg_rank);
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**snprintf", "**snprintf %d",
					     mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = PMI_KVS_Get(vc->ch.pg->kvs_name, key, val, val_max_sz);
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**pmi_kvs_get", "**pmi_kvs_get %d",
					     mpi_errno);
	    return mpi_errno;
	}
	dlid = atoi(val);
	if (dlid < 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**arg", 0);
	    return mpi_errno;
	}

	/* get the destination qp from the pmi database */
	mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d:%d-qp", vc->ch.pg_rank, pg_rank);
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**snprintf", "**snprintf %d",
					     mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = PMI_KVS_Get(vc->ch.pg->kvs_name, key, val, val_max_sz);
	if (mpi_errno != 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**pmi_kvs_get", "**pmi_kvs_get %d",
					     mpi_errno);
	    return mpi_errno;
	}
	dest_qp_num = atoi(val);
	if (dest_qp_num < 0)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					     FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**arg", 0);
	    return mpi_errno;
	}
	/* finish the queue pair connection */
	MPIU_DBG_PRINTF(("calling ibu_finish_qp(%d:%d)\n", dlid, dest_qp_num));
	/*printf("calling ibu_finish_qp(%d:%d)\n", dlid, dest_qp_num);fflush(stdout);*/
	mpi_errno = ibu_finish_qp(vc->ch.ibu, dlid, dest_qp_num);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					     __LINE__, MPI_ERR_OTHER,
					     "**finish_qp", 0);
	    return mpi_errno;
	}

	/* set the state to connected */
	vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
	/* post a read of the first packet */
	post_pkt_recv(vc);
#if 0
	MPIU_DBG_PRINTF(("posting first packet receive of %d bytes\n", sizeof(MPIDI_CH3_Pkt_t)));
	vc->ch.req->dev.iov[0].MPID_IOV_BUF = (void *)&vc->ch.req->ch.pkt;
	vc->ch.req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
	vc->ch.req->dev.iov_count = 1;
	vc->ch.req->ch.iov_offset = 0;
	vc->ch.req->dev.ca = MPIDI_CH3I_CA_HANDLE_PKT;
	vc->ch.recv_active = vc->ch.req;
	mpi_errno = ibu_post_read(vc->ch.ibu, &vc->ch.req->ch.pkt, sizeof(MPIDI_CH3_Pkt_t));
#endif
    }

    PMI_Barrier();

    MPIU_Free(val);
    MPIU_Free(key);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);
    return 0;
}
