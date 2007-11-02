/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "mpidi_ch3_impl.h"
#include "pmi.h"

int MPIDI_CH3_packet_len;
MPIDI_VC *MPIDI_CH3_vc_table;

int MPIDI_CH3I_my_rank;

MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int * argc, char *** argv, int * has_args, int * has_env,
		   int * has_parent)
{
    int mpi_errno;
    MPIDI_CH3I_Process_group_t * pg;
    
    int pg_rank;
    int pg_size;
    MPIDI_VC * vc_table;
    MPID_Comm * comm, *commworld, *intercomm;
    int p;
    int port;
    char * key;
    char * val;
    int key_max_sz;
    int val_max_sz;
    int name_sz;
    int id_sz;

    MPIDI_CH3I_inside_handler = 0;
	
    MPIDI_CH3_packet_len = 16384;

    mpi_errno = PMI_Init(has_parent);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_init", "**pmi_init %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Get_rank(&pg_rank);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_rank", "**pmi_get_rank %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Get_size(&pg_size);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_size", "**pmi_get_size %d", mpi_errno);
	return mpi_errno;
    }

    MPIDI_CH3I_my_rank = pg_rank;

    /*MPIU_Timer_init(pg_rank, pg_size);*/
    MPIU_dbg_init(pg_rank);

    /* Allocate process group data structure and populate */
    pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    if (pg == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->size = pg_size;
    mpi_errno = PMI_KVS_Get_name_length_max(&name_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	MPID_Abort (NULL, MPI_SUCCESS, -1);
    }
    pg->kvs_name = MPIU_Malloc(name_sz + 1);
    if (pg->kvs_name == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_my_name(pg->kvs_name, name_sz);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_my_name", "**pmi_kvs_get_my_name %d", mpi_errno);
	return mpi_errno;
    }

    mpi_errno = PMI_Get_id_length_max(&id_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", mpi_errno);
	return mpi_errno;
    }
    pg->pg_id = MPIU_Malloc(id_sz + 1);
    if (pg->pg_id == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_Get_id(pg->pg_id, id_sz);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id", "**pmi_get_id %d", mpi_errno);
	return mpi_errno;
    }

    pg->ref_count = 1;
    pg->next = NULL;
    MPIDI_CH3I_Process.pg = pg;
    
    /* Allocate and initialize the VC table associated with this
     * process group (and thus COMM_WORLD) */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * pg_size);
    if (vc_table == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->ref_count += pg_size;
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3U_VC_init(&vc_table[p], p);
	vc_table[p].ch.pg = pg;
	vc_table[p].ch.pg_rank = p;
	vc_table[p].ch.sendq_head = NULL;
	vc_table[p].ch.sendq_tail = NULL;
	vc_table[p].ch.req = (MPID_Request*)MPIU_Malloc(sizeof(MPID_Request));
	vc_table[p].ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	vc_table[p].ch.recv_active = NULL;
	vc_table[p].ch.send_active = NULL;
	vc_table[p].ch.reading_pkt = TRUE;
    }
    pg->vc_table = vc_table;
    MPIDI_CH3_vc_table = vc_table;
    
    /* Initialize MPI_COMM_WORLD object */
    comm = MPIR_Process.comm_world;
    comm->rank = pg_rank;
    comm->remote_size = comm->local_size = pg_size;
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**init_vcrt",
					 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**init_getptr",
					 0);
	return mpi_errno;
    }
    for (p = 0; p < pg_size; p++)
    {
	mpi_errno = MPID_VCR_Dup(&vc_table[p], &comm->vcr[p]);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrdup", 0);
	    return mpi_errno;
	}
    }
    
    /* Initialize MPI_COMM_SELF object */
    comm = MPIR_Process.comm_self;
    comm->rank = 0;
    comm->remote_size = comm->local_size = 1;
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**init_vcrt",
					 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**init_getptr",
					 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCR_Dup(&vc_table[pg_rank], &comm->vcr[0]);    
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_vcrdup", 0);
	return mpi_errno;
    }

    /* Initialize Progress Engine */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**init_progress", 0);
	return mpi_errno;
    }
    
    mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
	return mpi_errno;
    }
    key_max_sz++;
    key = MPIU_Malloc(key_max_sz);
    if (key == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
	return mpi_errno;
    }
    val_max_sz++;
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    
    /* initialize the infinband functions */
    mpi_errno = ibu_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_ibu", 0);
	return mpi_errno;
    }
    /* create a completion set for this process */
    mpi_errno = ibu_create_set(&MPIDI_CH3I_Process.set);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_ibu_set", 0);
	return mpi_errno;
    }
    /* get and put the local id for this process in the PMI database */
    port = ibu_get_lid();

    mpi_errno = MPIU_Snprintf(key, key_max_sz, "P%d-lid", pg_rank);
    if (mpi_errno < 0 || mpi_errno > key_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = snprintf(val, val_max_sz, "%d", port);
    if (mpi_errno < 0 || mpi_errno > val_max_sz)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**snprintf", "**snprintf %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_KVS_Put(pg->kvs_name, key, val);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", mpi_errno);
	return mpi_errno;
    }

    MPIU_DBG_PRINTF(("Published lid=%d\n", port));
    
    mpi_errno = PMI_KVS_Commit(pg->kvs_name);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", mpi_errno);
	return mpi_errno;
    }
    mpi_errno = PMI_Barrier();
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", mpi_errno);
	return mpi_errno;
    }

    /* XXX - has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env = TRUE;

    *has_parent = 0;
    
    if (*has_parent) 
    {
        /* This process was spawned. Create intercommunicator with parents. */

        if (pg_rank == 0)
	{
            /* get the port name of the root of the parents */
            mpi_errno = PMI_KVS_Get(pg->kvs_name, "PARENT_ROOT_PORT_NAME", val, val_max_sz);
            if (mpi_errno != 0)
            {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", mpi_errno);
                return mpi_errno;
            }
        }

        /* do a connect with the root */
        MPID_Comm_get_ptr(MPI_COMM_WORLD, commworld);
        mpi_errno = MPIDI_CH3_Comm_connect(val, 0, commworld, &intercomm);
        if (mpi_errno != MPI_SUCCESS) return mpi_errno;

        MPIR_Process.comm_parent = intercomm;
    }

    /* for now, connect all the processes at init time */
    MPIDI_DBG_PRINTF((65, "ch3_init", "calling setup_connections.\n"));fflush(stdout);
    MPIDI_CH3I_Setup_connections();
    MPIDI_DBG_PRINTF((65, "ch3_init", "connections formed, exiting\n"));fflush(stdout);

    MPIU_Free(val);
    MPIU_Free(key);

    return MPI_SUCCESS;
}
