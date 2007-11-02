/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"
#ifdef HAVE_STDIO_H
#include <stdio.h> /* snprintf */
#endif

/*@
   mm_vc_init - initialize vc stuff

   Notes:
@*/
void mm_vc_init()
{
    MPIDI_STATE_DECL(MPID_STATE_MM_VC_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_VC_INIT);

    MPID_Process.VCTable_allocator = BlockAllocInit(sizeof(MPIDI_VCRT), 100, 100, malloc, free);
    MPID_Process.VC_allocator = BlockAllocInit(sizeof(MPIDI_VC), 100, 100, malloc, free);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_INIT);
}

/*@
   mm_vc_finalize - finalize vc stuff

   Notes:
@*/
void mm_vc_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_MM_VC_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_VC_FINALIZE);
    
    BlockAllocFinalize(&MPID_Process.VCTable_allocator);
    BlockAllocFinalize(&MPID_Process.VC_allocator);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_FINALIZE);
}

/*@
   MPID_VCRT_Create - create a vc reference table

   Parameters:
+  int size - size
-  MPID_VCRT *vcrt_ptr - pointer to a reference table

   Notes:
@*/
int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{
    MPID_VCRT p;

    p = BlockAlloc(MPID_Process.VCTable_allocator);
    p->ref_count = 1;
    p->table_ptr = (MPIDI_VC**)MPIU_Malloc(sizeof(MPIDI_VC*) * size);
    memset(p->table_ptr, 0, sizeof(MPIDI_VC*) * size);
    p->lid_ptr = (int*)MPIU_Malloc(sizeof(int) * size);

    *vcrt_ptr = p;

    return MPI_SUCCESS;
}

/*@
   MPID_VCRT_Add_ref - add reference count

   Parameters:
+  MPID_VCRT vcrt - vc reference table

   Notes:
@*/
int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    if (vcrt == NULL)
	return MPI_ERR_ARG;

    vcrt->ref_count++;

    return MPI_SUCCESS;
}

/*@
   MPID_VCRT_Release - release vc reference table

   Parameters:
+  MPID_VCRT vcrt - vc reference table

   Notes:
@*/
int MPID_VCRT_Release(MPID_VCRT vcrt)
{
    if (vcrt == NULL)
	return MPI_ERR_ARG;

    vcrt->ref_count--;
    if (vcrt->ref_count == 0)
    {
	if (vcrt->table_ptr != NULL)
	    MPIU_Free(vcrt->table_ptr);
	BlockFree(MPID_Process.VCTable_allocator, vcrt);
    }

    return MPI_SUCCESS;
}

/*@
   MPID_VCRT_Get_ptr - get pointer to the array of vc's in the reference table

   Parameters:
+  MPID_VCRT vcrt - vc reference table
-  MPID_VCR **vc_pptr - pointer to the vc array pointer

   Notes:
@*/
int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    if (vcrt == NULL)
	return MPI_ERR_ARG;

    *vc_pptr = vcrt->table_ptr;

    return MPI_SUCCESS;
}

int MPID_VCR_Get_lpid(MPID_VCR vcr, int * lpid_ptr)
{
    return MPI_SUCCESS;
}

int MPID_VCR_Dup(MPID_VCR orig_vcr, MPID_VCR *new_vcr)
{
    return MPI_SUCCESS;
}

/*@
   mm_vc_alloc - allocate a virtual connection

   Parameters:
+  MM_METHOD method - method

   Notes:
@*/
MPIDI_VC * mm_vc_alloc(MM_METHOD method)
{
    MPIDI_VC *vc_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MM_VC_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_VC_ALLOC);
    dbg_printf("mm_vc_alloc\n");

    vc_ptr = (MPIDI_VC*)BlockAlloc(MPID_Process.VC_allocator);
    vc_ptr->method = method;
    vc_ptr->ref_count = 1;
    MPID_Thread_lock_init(&vc_ptr->lock);
    vc_ptr->readq_head = NULL;
    vc_ptr->readq_tail = NULL;
    vc_ptr->writeq_head = NULL;
    vc_ptr->writeq_tail = NULL;
#ifdef MPICH_DEV_BUILD
    vc_ptr->pmi_kvsname = INVALID_POINTER;
    vc_ptr->rank = -1;
    vc_ptr->read_next_ptr = INVALID_POINTER;
    vc_ptr->write_next_ptr = INVALID_POINTER;
#endif
    switch (method)
    {
    case MM_NULL_METHOD:
	break;
    case MM_UNBOUND_METHOD:
	memset(&vc_ptr->data, 0, sizeof(vc_ptr->data));
	break;
    case MM_PACKER_METHOD:
	vc_ptr->fn = g_packer_vc_functions;
	break;
    case MM_UNPACKER_METHOD:
	vc_ptr->fn = g_unpacker_vc_functions;
	break;
#ifdef WITH_METHOD_SHM
    case MM_SHM_METHOD:
	/* data members */
	vc_ptr->data.shm.shm_ptr = NULL;
	/* function pointers */
	vc_ptr->fn = g_shm_vc_functions;
	break;
#endif
#ifdef WITH_METHOD_TCP
    case MM_TCP_METHOD:
	/* data members */
	vc_ptr->data.tcp.bfd = BFD_INVALID_SOCKET;
	vc_ptr->data.tcp.connected = FALSE;
	vc_ptr->data.tcp.connecting = FALSE;
	/* function pointers */
	/* mm required functions */
	vc_ptr->fn = g_tcp_vc_functions;
	/* tcp specific functions */
	vc_ptr->data.tcp.read = tcp_read_connecting;
	vc_ptr->pkt_car.type = MM_HEAD_CAR | MM_READ_CAR; /* static car used to read headers */
	vc_ptr->pkt_car.vc_ptr = vc_ptr;
	vc_ptr->pkt_car.next_ptr = NULL;
	vc_ptr->pkt_car.vcqnext_ptr = NULL;
	vc_ptr->pkt_car.freeme = FALSE;
	vc_ptr->pkt_car.request_ptr = NULL;
	vc_ptr->pkt_car.buf_ptr = &vc_ptr->pkt_car.msg_header.buf;
	vc_ptr->pkt_car.msg_header.buf.type = MM_SIMPLE_BUFFER;
	vc_ptr->pkt_car.msg_header.buf.simple.buf = &vc_ptr->pkt_car.msg_header.pkt;
	vc_ptr->pkt_car.msg_header.buf.simple.len = sizeof(MPID_Packet);
	break;
#endif
#ifdef WITH_METHOD_SOCKET
    case MM_SOCKET_METHOD:
	/* data members */
	vc_ptr->data.socket.sock = NULL; /*SOCK_INVALID_SOCKET;*/
	vc_ptr->data.socket.state = 0; /*SOCKET_INVALID_STATE;*/
	vc_ptr->data.socket.connect_vc_ptr = NULL;
	/* function pointers */
	/* mm required functions */
	vc_ptr->fn = g_socket_vc_functions;
	/* socket specific */
	vc_ptr->pkt_car.type = MM_HEAD_CAR | MM_READ_CAR; /* static car used to read headers */
	vc_ptr->pkt_car.vc_ptr = vc_ptr;
	vc_ptr->pkt_car.next_ptr = NULL;
	vc_ptr->pkt_car.vcqnext_ptr = NULL;
	vc_ptr->pkt_car.freeme = FALSE;
	vc_ptr->pkt_car.request_ptr = NULL;
	vc_ptr->pkt_car.buf_ptr = &vc_ptr->pkt_car.msg_header.buf;
	vc_ptr->pkt_car.msg_header.buf.type = MM_SIMPLE_BUFFER;
	vc_ptr->pkt_car.msg_header.buf.simple.buf = &vc_ptr->pkt_car.msg_header.pkt;
	vc_ptr->pkt_car.msg_header.buf.simple.len = sizeof(MPID_Packet);
	break;
#endif
#ifdef WITH_METHOD_IB
    case MM_IB_METHOD:
	/* mm required functions */
	vc_ptr->fn = g_ib_vc_functions;
	/* ib specific */
	vc_ptr->data.ib.reading_header = TRUE;
#ifdef MPICH_DEV_BUILD
	memset(&vc_ptr->data.ib.info, 0, sizeof(vc_ptr->data.ib.info));
#endif
	vc_ptr->pkt_car.type = MM_HEAD_CAR | MM_READ_CAR; /* static car used to read headers */
	vc_ptr->pkt_car.vc_ptr = vc_ptr;
	vc_ptr->pkt_car.next_ptr = NULL;
	vc_ptr->pkt_car.vcqnext_ptr = NULL;
	vc_ptr->pkt_car.freeme = FALSE;
	vc_ptr->pkt_car.request_ptr = NULL;
	vc_ptr->pkt_car.buf_ptr = &vc_ptr->pkt_car.msg_header.buf;
	vc_ptr->pkt_car.msg_header.buf.type = MM_SIMPLE_BUFFER;
	vc_ptr->pkt_car.msg_header.buf.simple.buf = &vc_ptr->pkt_car.msg_header.pkt;
	vc_ptr->pkt_car.msg_header.buf.simple.len = sizeof(MPID_Packet);
	break;
#endif
#ifdef WITH_METHOD_VIA
    case MM_VIA_METHOD:
	/* data members */
	/*
	via_init_info_struct(&vc_ptr->data.via.info);
	vc_ptr->data.via.info.hVi = NULL;
	...
	*/
	/* function pointers */
	vc_ptr->fn = g_via_vc_functions;
	break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    case MM_VIA_RDMA_METHOD:
	/* data members */
	/*via_init_info_struct(&vc_ptr->data.via.info);*/
	/* function pointers */
	vc_ptr->fn = g_via_rdma_vc_functions;
	break;
#endif
#ifdef WITH_METHOD_NEW
    case MM_NEW_METHOD:
	vc_ptr->fn = g_new_vc_functions;
	break;
#endif
    default:
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_ALLOC);
    return vc_ptr;
}

/*@
   mm_vc_connect_alloc - allocate a new vc and post a connect to its method

   Parameters:
+  MPID_Comm *comm_ptr - communicator
-  int rank - rank

   Notes:
@*/
MPIDI_VC * mm_vc_connect_alloc(MPID_Comm *comm_ptr, int rank)
{
    MPIDI_VC *vc_ptr;
    char *kvs_name;
    char key[100];
    char *value;
    int value_len;
    char *methods;
#ifdef WITH_METHOD_VIA
    char *temp;
#endif
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MM_VC_CONNECT_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_MM_VC_CONNECT_ALLOC);
    dbg_printf("mm_vc_connect_alloc(rank:%d)\n", rank);
    
    kvs_name = comm_ptr->mm.pmi_kvsname;

    /*dbg_printf("+PMI_KVS_Get_value_length_max");*/
    value_len = PMI_KVS_Get_value_length_max();
    /*dbg_printf("-\n");*/
    value = (char*)MPIU_Malloc(value_len);
    methods = (char*)MPIU_Malloc(value_len);

    /*dbg_printf("A:remote_rank: %d\n", rank);*/
    snprintf(key, 100, "businesscard:%d", rank);
    /*dbg_printf("+PMI_KVS_Get(%s):", key);*/
    PMI_KVS_Get(kvs_name, key, methods);
    /*dbg_printf("%s-\n", methods);*/
    
    /* choose method */
    
    for (i=0; i<MPID_Process.num_ordered_methods; i++)
    {
	switch (MPID_Process.method_order[i])
	{
#ifdef WITH_METHOD_SHM
	case MM_SHM_METHOD:
	    if (strstr(methods, "shm"))
	    {
		/* get the shm method business card */
		snprintf(key, 100, "business_card_shm:%d", rank);
		PMI_KVS_Get(kvs_name, key, value);
		
		/* check to see if we can connect with this business card */
		if (shm_can_connect(value))
		{
		    /* allocate a vc for this method */
		    vc_ptr = mm_vc_alloc(MM_SHM_METHOD);
		    /* copy the kvs name and rank into the vc. this may not be necessary */
		    vc_ptr->pmi_kvsname = kvs_name;
		    vc_ptr->rank = rank;
		    /* post a connection request to the method */
		    shm_post_connect(vc_ptr, value);
		    
		    MPIU_Free(value);
		    MPIU_Free(methods);
		    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_CONNECT_ALLOC);
		    return vc_ptr;
		}
	    }
	    break;
#endif
    
#ifdef WITH_METHOD_VIA_RDMA
	case MM_VIA_RDMA_METHOD:
	    if (strstr(methods, "via_rdma"))
	    {
		/* get the via method business card */
		snprintf(key, 100, "business_card_via_rdma:%d", rank);
		PMI_KVS_Get(kvs_name, key, value);
		
		/* check to see if we can connect with this business card */
		if (via_rdma_can_connect(value))
		{
		    /* allocate a vc for this method */
		    vc_ptr = mm_vc_alloc(MM_VIA_RDMA_METHOD);
		    /* copy the kvs name and rank into the vc. this may not be necessary */
		    vc_ptr->pmi_kvsname = kvs_name;
		    vc_ptr->rank = rank;
		    /* post a connection request to the method */
		    via_rdma_post_connect(vc_ptr, value);
		    
		    MPIU_Free(value);
		    MPIU_Free(methods);
		    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_CONNECT_ALLOC);
		    return vc_ptr;
		}
	    }
	    break;
#endif
    
#ifdef WITH_METHOD_VIA
	case MM_VIA_METHOD:
	    /* check for a false match with the via_rdma method */
	    temp = strstr(methods, "via_rdma");
	    if (temp != NULL)
		*temp = 'x';
	    if (strstr(methods, "via"))
	    {
		/* get the via rdma method business card */
		snprintf(key, 100, "business_card_via:%d", rank);
		PMI_KVS_Get(kvs_name, key, value);
		
		/* check to see if we can connect with this business card */
		if (via_can_connect(value))
		{
		    /* allocate a vc for this method */
		    vc_ptr = mm_vc_alloc(MM_VIA_METHOD);
		    /* copy the kvs name and rank into the vc. this may not be necessary */
		    vc_ptr->pmi_kvsname = kvs_name;
		    vc_ptr->rank = rank;
		    /* post a connection request to the method */
		    via_post_connect(vc_ptr, value);
		    
		    MPIU_Free(value);
		    MPIU_Free(methods);
		    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_CONNECT_ALLOC);
		    return vc_ptr;
		}
	    }
	    break;
#endif
    
#ifdef WITH_METHOD_TCP
	case MM_TCP_METHOD:
	    if (strstr(methods, "tcp"))
	    {
		/* get the tcp method business card */
		snprintf(key, 100, "business_card_tcp:%d", rank);
		PMI_KVS_Get(kvs_name, key, value);
		
		/* check to see if we can connect with this business card */
		if (tcp_can_connect(value))
		{
		    /* allocate a vc for this method */
		    vc_ptr = mm_vc_alloc(MM_TCP_METHOD);
		    /* copy the kvs name and rank into the vc. this may not be necessary */
		    vc_ptr->pmi_kvsname = kvs_name;
		    vc_ptr->rank = rank;
		    /* post a connection request to the method */
		    tcp_post_connect(vc_ptr, value);
		    
		    MPIU_Free(value);
		    MPIU_Free(methods);
		    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_CONNECT_ALLOC);
		    return vc_ptr;
		}
	    }
	    break;
#endif

#ifdef WITH_METHOD_SOCKET
	case MM_SOCKET_METHOD:
	    if (strstr(methods, "socket"))
	    {
		/* get the tcp method business card */
		/*dbg_printf("B: remote rank: %d\n", rank);*/
		snprintf(key, 100, "business_card_socket:%d", rank);
		/*dbg_printf("+PMI_KVS_Get(%s):", key);*/
		PMI_KVS_Get(kvs_name, key, value);
		/*dbg_printf("%s-\n", value);*/
		
		/* check to see if we can connect with this business card */
		if (socket_can_connect(value))
		{
		    /* allocate a vc for this method */
		    vc_ptr = mm_vc_alloc(MM_SOCKET_METHOD);
		    /* copy the kvs name and rank into the vc. this may not be necessary */
		    vc_ptr->pmi_kvsname = kvs_name;
		    vc_ptr->rank = rank;
		    /* post a connection request to the method */
		    socket_post_connect(vc_ptr, value);
		    
		    MPIU_Free(value);
		    MPIU_Free(methods);
		    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_CONNECT_ALLOC);
		    return vc_ptr;
		}
	    }
	    break;
#endif
	default:
	    break;
	}
    }
	    
#ifdef WITH_METHOD_TCP
    /* If no method is selected from the ordered list, attempt to make a tcp connection */
    if (strstr(methods, "tcp"))
    {
	/* get the tcp method business card */
	snprintf(key, 100, "business_card_tcp:%d", rank);
	PMI_KVS_Get(kvs_name, key, value);
	
	/* check to see if we can connect with this business card */
	if (tcp_can_connect(value))
	{
	    /* allocate a vc for this method */
	    vc_ptr = mm_vc_alloc(MM_TCP_METHOD);
	    /* copy the kvs name and rank into the vc. this may not be necessary */
	    vc_ptr->pmi_kvsname = kvs_name;
	    vc_ptr->rank = rank;
	    /* post a connection request to the method */
	    tcp_post_connect(vc_ptr, value);
	    
	    MPIU_Free(value);
	    MPIU_Free(methods);
	    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_CONNECT_ALLOC);
	    return vc_ptr;
	}
    }
#endif

    MPIU_Free(value);
    MPIU_Free(value);

    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_CONNECT_ALLOC);
    return NULL;
}

/*@
   mm_vc_free - free a virtual connection

   Parameters:
+  MPIDI_VC *ptr - vc pointer

   Notes:
@*/
int mm_vc_free(MPIDI_VC *ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MM_VC_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_MM_VC_FREE);

    if (ptr->method == MM_PACKER_METHOD || ptr->method == MM_UNPACKER_METHOD)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_FREE);
	return MPI_SUCCESS;
    }

    BlockFree(MPID_Process.VC_allocator, ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_MM_VC_FREE);
    return MPI_SUCCESS;
}

