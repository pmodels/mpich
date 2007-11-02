/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"
#include "pmi.h"

#ifdef WITH_METHOD_IB

ib_uint32_t modifyQP( IB_Info *ib, Ib_qp_state qp_state )
{
    ib_uint32_t status;
    ib_qp_attr_list_t attrList;
    ib_address_vector_t av;
    attr_rec_t *attr_rec = NULL;

    if (qp_state == IB_QP_STATE_INIT)
    {
	if ((attr_rec = (attr_rec_t *)
	     malloc(sizeof (attr_rec_t) * 5)) == NULL )
	{
	    MPIU_DBG_PRINTF(("Malloc failed %d\n", __LINE__));
	    return IB_FAILURE;
	}
	    
	((attr_rec[0]).id) = IB_QP_ATTR_PRIMARY_PORT;
	((attr_rec[0]).data) = 1;
	((attr_rec[1]).id) = IB_QP_ATTR_PRIMARY_P_KEY_IX;
	((attr_rec[1]).data) = 0;
	((attr_rec[2]).id) = IB_QP_ATTR_RDMA_W_F;
	((attr_rec[2]).data) = 1;
	((attr_rec[3]).id) = IB_QP_ATTR_RDMA_R_F;
	((attr_rec[3]).data) = 1;
	((attr_rec[4]).id) = IB_QP_ATTR_ATOMIC_F;
	((attr_rec[4]).data) = 0;
	    
	attrList.attr_num = 5;
	attrList.attr_rec_p = &attr_rec[0];    
    }
    else if (qp_state == IB_QP_STATE_RTR) 
    {
	av.sl                         = 0;
	/*MPIU_DBG_PRINTF(("setting dest_lid to ib->m_dlid: %d\n", ib->m_dlid));*/
	av.dest_lid                   = (ib_uint16_t)ib->m_dlid;
	av.grh_f                      = 0;
	av.path_bits                  = 0;
	av.max_static_rate            = 1;
	av.global.flow_label          = 1;
	av.global.hop_limit           = 1;
	av.global.src_gid_index       = 0;
	av.global.traffic_class       = 1;
	    
	if ((attr_rec = (attr_rec_t *)
	     malloc(sizeof (attr_rec_t) * 6)) == NULL )
	{
	    MPIU_DBG_PRINTF(("Malloc failed %d\n", __LINE__));
	    return IB_FAILURE;
	}

	((attr_rec[0]).id) = IB_QP_ATTR_PRIMARY_ADDR;
	((attr_rec[0]).data) = (int)&av;
	((attr_rec[1]).id) = IB_QP_ATTR_DEST_QPN;
	((attr_rec[1]).data) = ib->m_dest_qp_num;
	((attr_rec[2]).id) = IB_QP_ATTR_RCV_PSN;
	((attr_rec[2]).data) = 0;
	((attr_rec[3]).id) = IB_QP_ATTR_MTU;
	((attr_rec[3]).data) = ib->m_mtu_size;
	((attr_rec[4]).id) = IB_QP_ATTR_RDMA_READ_LIMIT;
	((attr_rec[4]).data) = 4;
	((attr_rec[5]).id) = IB_QP_ATTR_RNR_NAK_TIMER;
	((attr_rec[5]).data) = 1;
	    
	attrList.attr_num = 6;
	attrList.attr_rec_p = &attr_rec[0];
    }
    else if (qp_state == IB_QP_STATE_RTS)
    {
	if ((attr_rec = (attr_rec_t *)
	     malloc(sizeof (attr_rec_t) * 5)) == NULL )
	{
	    MPIU_DBG_PRINTF(("Malloc failed %d\n", __LINE__));
	    return IB_FAILURE;
	}

	((attr_rec[0]).id)    = IB_QP_ATTR_SEND_PSN;
	((attr_rec[0]).data)  = 0; 
	((attr_rec[1]).id)    = IB_QP_ATTR_TIMEOUT;
	((attr_rec[1]).data)  = 0x7c;
	((attr_rec[2]).id)    = IB_QP_ATTR_RETRY_COUNT;
	((attr_rec[2]).data)  = 2048;
	((attr_rec[3]).id)    = IB_QP_ATTR_RNR_RETRY_COUNT;
	((attr_rec[3]).data)  = 2048;
	((attr_rec[4]).id)    = IB_QP_ATTR_DEST_RDMA_READ_LIMIT;
	((attr_rec[4]).data)  = 4;
	
	attrList.attr_num = 5; 
	attrList.attr_rec_p = &attr_rec[0];
    }
    else if (qp_state == IB_QP_STATE_RESET)
    {
	attrList.attr_num = 0;
	attrList.attr_rec_p = NULL;
    }
    else
    {
	return IB_FAILURE;
    }

    status = ib_qp_modify_us(IB_Process.hca_handle, 
			     ib->m_qp_handle, 
			     qp_state, 
			     &attrList );
    if (attr_rec)    
	free(attr_rec);
    if( status != IB_SUCCESS )
    {
	return status;
    }

    return IB_SUCCESS;
}

ib_uint32_t createQP(IB_Info *ib)
{
    ib_uint32_t status;
    ib_uint32_t qp_num;
    ib_qp_attr_list_t attrList;
    attr_rec_t attr_rec[] = {
	{IB_QP_ATTR_SERVICE_TYPE, IB_ST_RELIABLE_CONNECTION},
	{IB_QP_ATTR_SEND_CQ, 0},
	{IB_QP_ATTR_RCV_CQ, 0},
	{IB_QP_ATTR_SEND_REQ_MAX, 0},
	{IB_QP_ATTR_RCV_REQ_MAX, 0},
	{IB_QP_ATTR_SEND_SGE_MAX, 8},
	{IB_QP_ATTR_RCV_SGE_MAX, 8},
	{IB_QP_ATTR_SIGNALING_TYPE, QP_SIGNAL_ALL}
    };

    attr_rec[1].data = (int)IB_Process.cq_handle;
    attr_rec[2].data = (int)IB_Process.cq_handle;
    attr_rec[3].data = ib->m_max_wqes;
    attr_rec[4].data = ib->m_max_wqes;

    attrList.attr_num = sizeof(attr_rec)/sizeof(attr_rec[0]);
    attrList.attr_rec_p = &attr_rec[0];

    status = ib_qp_create_us(
	IB_Process.hca_handle,
	IB_Process.pd_handle,
	&attrList, 
	&ib->m_qp_handle, 
	&qp_num, 
	NULL);
    if (status != IBA_OK)
	return status;
    ib->m_dest_qp_num = qp_num;
    return IB_SUCCESS;
}

static ib_mr_handle_t s_mr_handle;
static ib_uint32_t    s_lkey;
void *ib_malloc_register(size_t size)
{
    ib_uint32_t status;
    void *ptr;
    ib_uint32_t rkey;

    ptr = malloc(size);
    if (ptr == NULL)
    {
	MPIU_DBG_PRINTF(("malloc(%d) failed.\n", size));
	return NULL;
    }
    status = ib_mr_register_us(
	IB_Process.hca_handle,
	(ib_uint8_t*)ptr,
	size,
	IB_Process.pd_handle,
	IB_ACCESS_LOCAL_WRITE,
	&s_mr_handle,
	&s_lkey, &rkey);
    if (status != IB_SUCCESS)
    {
	MPIU_DBG_PRINTF(("ib_mr_register_us failed, error %d\n", status));
	return NULL;
    }

    return ptr;
}

void ib_free_deregister(void *p)
{
    free(p);
}

int ib_setup_connections()
{
    MPID_Comm *comm_ptr;
    int mpi_errno;
    MPIDI_VC *vc_ptr;
    int i, j, len;
    char *key, *value;
    IB_Info *ib;
    ib_uint32_t status;
    MPIDI_STATE_DECL(MPID_STATE_IB_SETUP_CONNECTIONS);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_SETUP_CONNECTIONS);

    len = PMI_KVS_Get_key_length_max();
    key = (char*)malloc(len * sizeof(char));
    len = PMI_KVS_Get_value_length_max();
    value = (char*)malloc(len * sizeof(char));

    /* setup the vc's on comm_world */
    comm_ptr = MPIR_Process.comm_world;

    /* allocate a vc reference table */
    mpi_errno = MPID_VCRT_Create(comm_ptr->remote_size, &comm_ptr->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_DBG_PRINTF(("MPID_VCRT_Create failed, error %d\n", mpi_errno));
	free(key);
	free(value);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	return -1;
    }
    /* get an alias to the array of vc pointers */
    mpi_errno = MPID_VCRT_Get_ptr(comm_ptr->vcrt, &comm_ptr->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_DBG_PRINTF(("MPID_VCRT_Get_ptr failed, error %d\n", mpi_errno));
	free(key);
	free(value);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	return -1;
    }

    for (i=0; i<comm_ptr->remote_size; i++)
    {
	if ( i == comm_ptr->rank)
	    continue; /* don't make a connection to myself */
	/*MPIU_DBG_PRINTF(("setting up VC connection to rank %d\n", i));*/
	vc_ptr = comm_ptr->vcr[i];
	if (vc_ptr == NULL)
	{
	    comm_ptr->vcr[i] = vc_ptr = mm_vc_alloc(MM_IB_METHOD);
	    /* copy the kvs name and rank into the vc. 
	       this may not be necessary */
	    vc_ptr->pmi_kvsname = comm_ptr->mm.pmi_kvsname;
	    vc_ptr->rank = i;
	}
	sprintf(key, "ib_lid_%d", i);
	PMI_KVS_Get(vc_ptr->pmi_kvsname, key, value);
	/*MPIU_DBG_PRINTF(("PMI_KVS_Get %s:<%s, %s>\n", 
			vc_ptr->pmi_kvsname, key, value));*/
	ib = &vc_ptr->data.ib.info;
	ib->m_dlid = atoi(value);
	ib->m_allocator = BlockAllocInit(IB_PACKET_SIZE, IB_PACKET_COUNT, IB_PACKET_COUNT, ib_malloc_register, ib_free_deregister);
	ib->m_mr_handle = s_mr_handle; /* Not thread safe. This handle is reset every time ib_malloc_register is called. */
	ib->m_polling = TRUE;
	ib->m_max_wqes = 255;
	ib->m_mtu_size = 3; /* 3 = 2048 */

	/* save the lkey for posting sends and receives */
	ib->m_lkey = s_lkey;

	/*MPIU_DBG_PRINTF(("creating the queue pair\n"));*/
	/* Create the queue pair */
	status = createQP(ib);
	if (status != IB_SUCCESS)
	{
	    MPIU_DBG_PRINTF(("createQP failed, error %d\n", status));
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}

	/*MPIU_DBG_PRINTF(("modifyQP(INIT)\n"));*/
	status = modifyQP(ib, IB_QP_STATE_INIT);
	if (status != IB_SUCCESS)
	{
	    MPIU_DBG_PRINTF(("modifyQP(INIT) failed, error %d\n", status));
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}
	/*MPIU_DBG_PRINTF(("modifyQP(RTR)\n"));*/
	status = modifyQP(ib, IB_QP_STATE_RTR);
	if (status != IB_SUCCESS)
	{
	    MPIU_DBG_PRINTF(("modifyQP(RTR) failed, error %d\n", status));
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}
	/*MPIU_DBG_PRINTF(("modifyQP(RTS)\n"));*/
	status = modifyQP(ib, IB_QP_STATE_RTS);
	if (status != IB_SUCCESS)
	{
	    MPIU_DBG_PRINTF(("modifyQP(RTS) failed, error %d\n", status));
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}

	/* pre post some receives on each connection */
	for (j=0; j<IB_NUM_PREPOSTED_RECEIVES; j++)
	{
	    ibr_post_receive(vc_ptr);
	}
    }

    /*MPIU_DBG_PRINTF(("calling PMI_Barrier\n"));*/
    PMI_Barrier();
    /*MPIU_DBG_PRINTF(("PMI_Barrier returned\n"));*/

    free(key);
    free(value);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
    return MPI_SUCCESS;
}

#endif
