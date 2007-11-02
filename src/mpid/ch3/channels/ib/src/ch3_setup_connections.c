/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "ibu.h"
#ifdef USE_IB_VAPI
#include "ibuimpl.vapi.h"
#else
#include "ibuimpl.ibal.h"
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Setup_connections
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Setup_connections(MPIDI_PG_t *pg, int pg_rank)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno = PMI_SUCCESS;
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    int i, dlid;
    int qp_num, dest_qp_num;
#ifdef USE_IB_VAPI
    VAPI_lkey_t dest_rdma_buf_rkey;
    ibu_rdma_buf_t *RDMA_buf, *pdest_rdma_buf;
    VAPI_rkey_t rkey;
#else
    /* Mellanox added by Dafna July 7th 2004*/
    uint32_t dest_rdma_buf_rkey;
    ibu_rdma_buf_t *RDMA_buf, *pdest_rdma_buf;
    uint32_t rkey;
#endif
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);

    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
	    __LINE__, MPI_ERR_OTHER, "**fail",
	    "**fail %d", pmi_errno);
	return mpi_errno;
    }
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
	    __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (pmi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
	    __LINE__, MPI_ERR_OTHER, "**fail",
	    "**fail %d", pmi_errno);
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
    for (i=0; i<MPIDI_PG_Get_size(pg); i++)
    {
	MPIDI_PG_Get_vc(pg, i, &vc);

	if (vc->pg_rank == pg_rank)
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
	ibu_set_vc_ptr(vc->ch.ibu, vc);
	mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d:%d-qp", pg_rank, i);
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**snprintf", "**snprintf %d",
		mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = MPIU_Snprintf(val, val_max_sz, "%d", qp_num);
	if (mpi_errno < 0 || mpi_errno > val_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**snprintf", "**snprintf %d",
		mpi_errno);
	    return mpi_errno;
	}
	MPIU_DBG_PRINTF(("putting qp_num (%s => %s)\n", key, val));
	pmi_errno = PMI_KVS_Put(pg->ch.kvs_name, key, val);
	if (pmi_errno != PMI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**pmi_kvs_put", "**pmi_kvs_put %d",
		pmi_errno);
	    return mpi_errno;
	}
	/*MPIU_DBG_PRINTF(("Published qp_num = %d\n", qp_num));*/

	/* Mellanox by Dafna July 7th 2004 - applies to both IBAL and VAPI */
	RDMA_buf = ibui_RDMA_buf_init(vc->ch.ibu, &rkey);
	if (RDMA_buf == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**nomem", 0);
	    return mpi_errno;
	}

	mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d:%d-RDMA_buf", pg_rank,i);
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**snprintf", "**snprintf %d",
		mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = MPIU_Snprintf(val, val_max_sz, "%p", RDMA_buf);
	if (mpi_errno < 0 || mpi_errno > val_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**snprintf", "**snprintf %d",
		mpi_errno);
	    return mpi_errno;
	}
	MPIU_DBG_PRINTF(("putting rdma buf (%s => %s)\n", key, val));
	pmi_errno = PMI_KVS_Put(pg->ch.kvs_name, key, val);
	if (pmi_errno != PMI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**pmi_kvs_put", "**pmi_kvs_put %d",
		pmi_errno);
	    return mpi_errno;
	}
	mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d:%d-RDMA_buf_rkey", pg_rank,i);
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**snprintf", "**snprintf %d",
		mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = MPIU_Snprintf(val, val_max_sz, "%d", rkey);
	if (mpi_errno < 0 || mpi_errno > val_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**snprintf", "**snprintf %d",
		mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = MPI_SUCCESS;
	MPIU_DBG_PRINTF(("putting rdma buf rkeys (%s => %s)\n", key, val));
	pmi_errno = PMI_KVS_Put(pg->ch.kvs_name, key, val);
	if (pmi_errno != PMI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**pmi_kvs_put", "**pmi_kvs_put %d",
		pmi_errno);
	    return mpi_errno;
	}
	/*MPIU_DBG_PRINTF(("Published qp_num = %d\n", qp_num));*/
    }



    /* commit the qp_num puts and barrier so gets can happen */
    pmi_errno = PMI_KVS_Commit(pg->ch.kvs_name);
    if (pmi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
	    __LINE__, MPI_ERR_OTHER,
	    "**pmi_kvs_commit", "**pmi_kvs_commit %d",
	    pmi_errno);
	return mpi_errno;
    }
    pmi_errno = PMI_Barrier();
    if (pmi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
	    __LINE__, MPI_ERR_OTHER,
	    "**pmi_barrier", "**pmi_barrier %d",
	    pmi_errno);
	return mpi_errno;
    }

    /* complete the queue pairs and post the receives */
    for (i=0; i<MPIDI_PG_Get_size(pg); i++)
    {
	MPIDI_PG_Get_vc(pg, i, &vc);
	if (vc->pg_rank == pg_rank)
	    continue;

	/* get the destination lid from the pmi database */
	mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-lid", vc->pg_rank);
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**snprintf", "**snprintf %d",
		mpi_errno);
	    return mpi_errno;
	}
	
	pmi_errno = PMI_KVS_Get(pg->ch.kvs_name, key, val, val_max_sz);
	if (pmi_errno != PMI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**pmi_kvs_get", "**pmi_kvs_get %d",
		pmi_errno);
	    return mpi_errno;
	}
	dlid = atoi(val);
#ifdef USE_IB_VAPI	
	if (dlid < 0) 
#else
	if (cl_ntoh32(dlid) < 0) /* Mellanox added by Dafna July 7th 2004. IBAL's endianess requires */
#endif
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**arg", 0);
	    return mpi_errno;
	}

	/* get the destination qp from the pmi database */
	mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d:%d-qp", vc->pg_rank, pg_rank);
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**snprintf", "**snprintf %d",
		mpi_errno);
	    return mpi_errno;
	}
	pmi_errno = PMI_KVS_Get(pg->ch.kvs_name, key, val, val_max_sz);
	if (pmi_errno != PMI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**pmi_kvs_get", "**pmi_kvs_get %d",
		pmi_errno);
	    return mpi_errno;
	}
	
	dest_qp_num = atoi(val);

#ifdef USE_IB_VAPI	
	if (dest_qp_num < 0)
#else
	if (cl_ntoh32(dest_qp_num) < 0) /* Mellanox added by Dafna July 7th 2004 - endianess of IBAL requires */
#endif
	{		
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**arg", 0);
	    return mpi_errno;
	}
	/* finish the queue pair connection */
	MPIU_DBG_PRINTF(("calling ibu_finish_qp(%d:%d)\n", dlid, dest_qp_num));
	mpi_errno = ibu_finish_qp(vc->ch.ibu, dlid, dest_qp_num);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
		__LINE__, MPI_ERR_OTHER,
		"**finish_qp", 0);
	    return mpi_errno;
	}

	/* Mellanox by Dafna July 7th 2004 - applies for both IBAL and VAPI */
	/* get the destination RDMA bufeer address and keys from the pmi database */
	mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d:%d-RDMA_buf", vc->pg_rank, pg_rank);
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**snprintf", "**snprintf %d",
		mpi_errno);
	    return mpi_errno;
	}
	pmi_errno = PMI_KVS_Get(pg->ch.kvs_name, key, val, val_max_sz);
	if (pmi_errno != PMI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**pmi_kvs_get", "**pmi_kvs_get %d",
		pmi_errno);
	    return mpi_errno;
	}
	sscanf((val), "%p", &pdest_rdma_buf);

	mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d:%d-RDMA_buf_rkey", vc->pg_rank, pg_rank);
	if (mpi_errno < 0 || mpi_errno > key_max_sz)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**snprintf", "**snprintf %d",
		mpi_errno);
	    return mpi_errno;
	}
	mpi_errno = MPI_SUCCESS;
	pmi_errno = PMI_KVS_Get(pg->ch.kvs_name, key, val, val_max_sz);
	if (pmi_errno != PMI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
		FCNAME, __LINE__, MPI_ERR_OTHER,
		"**pmi_kvs_get", "**pmi_kvs_get %d",
		pmi_errno);
	    return mpi_errno;
	}
#ifdef USE_IB_VAPI 
	dest_rdma_buf_rkey = (VAPI_rkey_t)atoi(val); /* Mellanox added by Dafna July 7th 2004*/
#else
	dest_rdma_buf_rkey = (uint32_t)atoi(val);
#endif

	ibui_update_remote_RDMA_buf(vc->ch.ibu, pdest_rdma_buf, dest_rdma_buf_rkey);

	/* set the state to connected */
	vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
	/* post a read of the first packet */
	post_pkt_recv(vc);
    }

    pmi_errno = PMI_Barrier();
    if (pmi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
	return mpi_errno;
    }

    MPIU_Free(val);
    MPIU_Free(key);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS);
    return mpi_errno;
}
