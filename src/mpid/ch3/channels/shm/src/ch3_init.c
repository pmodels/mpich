/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

MPIDI_CH3I_Process_t MPIDI_CH3I_Process = {NULL};

/* Define the ABI version of this channel.  Change this if the channel
   interface (not just the implementation of that interface) changes */
char MPIDI_CH3_ABIVersion[] = "1.1";

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t * pg, int pg_rank )
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    MPIDI_VC_t * vc;
    int pg_size;
    int p;
    char * key;
    char * val;
    char *kvsname = NULL;
    int key_max_sz;
    int val_max_sz;
    char shmemkey[MPIDI_MAX_SHM_NAME_LENGTH];
    int i, j, k;
    int shm_block;
    char local_host[100];
    MPIDI_CH3I_PG *pgch = (MPIDI_CH3I_PG*)pg->channel_private;
    MPIU_CHKLMEM_DECL(2);

#if 1
    if (sizeof(MPIDI_CH3I_PG) > MPIDI_CH3_PG_SIZE * sizeof(int32_t) ) {
	fprintf( stderr, "channel pg data larger than size in MPIDI_PG_t\n" );
	return MPI_ERR_INTERN;
    }
#endif
    /*
     * Extract process group related information from PMI and initialize
     * structures that track the process group connections, MPI_COMM_WORLD, and
     * MPI_COMM_SELF
     */
    /* MPID_Init in mpid_init.c handles the process group initialization. */

    /* Get the kvsname associated with MPI_COMM_WORLD */
    MPIDI_PG_GetConnKVSname( &kvsname );

    /* set the global variable defaults */
    pgch->nShmEagerLimit = MPIDI_SHM_EAGER_LIMIT;
#ifdef HAVE_SHARED_PROCESS_READ
    pgch->nShmRndvLimit = MPIDI_SHM_RNDV_LIMIT;
#ifdef HAVE_WINDOWS_H
    pgch->pSharedProcessHandles = NULL;
#else
    pgch->pSharedProcessIDs = NULL;
    pgch->pSharedProcessFileDescriptors = NULL;
#endif
#endif
    pgch->addr = NULL;
#ifdef USE_POSIX_SHM
    pgch->key[0] = '\0';
    pgch->id = -1;
#elif defined (USE_SYSV_SHM)
    pgch->key = -1;
    pgch->id = -1;
#elif defined (USE_WINDOWS_SHM)
    pgch->key[0] = '\0';
    pgch->id = NULL;
#else
#error No shared memory subsystem defined
#endif
    pgch->nShmWaitSpinCount = MPIDI_CH3I_SPIN_COUNT_DEFAULT;
    pgch->nShmWaitYieldCount = MPIDI_CH3I_YIELD_COUNT_DEFAULT;

    /* Initialize the VC table associated with this process
       group (and thus COMM_WORLD) */
    pg_size = pg->size;

    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)pg->vct[p].channel_private;
	/* FIXME: the vc's must be set to active for the close protocol to 
	   work in the shm channel */
        MPIDI_CHANGE_VC_STATE(&(pg->vct[p]), ACTIVE);
	/* FIXME: Should the malloc be within the init? */
	MPIDI_CH3_VC_Init( &pg->vct[p] );
	/* FIXME: Need to free this request when the vc is removed */
	vcch->req = (MPID_Request*)MPIU_Malloc(sizeof(MPID_Request));
	/* FIXME: Should these also be set in the VC_Init, or 
	   is VC_Init moot (never called because this channel does not
	   support dynamic processes?) */
	vcch->shm_reading_pkt = TRUE;
#ifdef USE_SHM_UNEX
	vcch->unex_finished_next = NULL;
	vcch->unex_list = NULL;
#endif
    }
    
    /* save my vc_ptr for easy access */
    /* MPIDI_PG_Get_vc_set_activer(pg, pg_rank, &MPIDI_CH3I_Process.vc); */
    /* FIXME: Figure out whether this is a common feature of process 
       groups (and thus make it part of the general PG_Init) or 
       something else.  Avoid a "get" routine because of the danger in
       using "get" where "dup" is required. */
    MPIDI_CH3I_Process.vc = &pg->vct[pg_rank];

    /* Initialize Progress Engine */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_progress", 0);
	return mpi_errno;
    }

    /* Allocate space for pmi keys and values */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    key_max_sz++;
    MPIU_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    val_max_sz++;
    MPIU_CHKLMEM_MALLOC(val, char *, val_max_sz, mpi_errno, "val");

    /* initialize the shared memory */
    shm_block = sizeof(MPIDI_CH3I_SHM_Queue_t) * pg_size; 

    if (pg_size > 1)
    {
	if (pg_rank == 0)
	{
	    /* Put the shared memory key */
	    MPIDI_Generate_shm_string(shmemkey,sizeof(shmemkey));
	    if (MPIU_Strncpy(key, "SHMEMKEY", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    if (MPIU_Strncpy(val, shmemkey, val_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    pmi_errno = PMI_KVS_Put(kvsname, key, val);
	    if (pmi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);
		return mpi_errno;
	    }

	    /* Put the hostname to make sure everyone is on the same host */
	    if (MPIU_Strncpy(key, "SHMHOST", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    MPID_Get_processor_name( val, val_max_sz, 0 );
	    pmi_errno = PMI_KVS_Put(kvsname, key, val);
	    if (pmi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);
		return mpi_errno;
	    }

	    pmi_errno = PMI_KVS_Commit(kvsname);
	    if (pmi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
		return mpi_errno;
	    }
	    pmi_errno = PMI_Barrier();
	    if (pmi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
		return mpi_errno;
	    }
	}
	else
	{
	    /* Get the shared memory key */
	    if (MPIU_Strncpy(key, "SHMEMKEY", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    pmi_errno = PMI_Barrier();
	    if (pmi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
		return mpi_errno;
	    }
	    pmi_errno = PMI_KVS_Get(kvsname, key, val, val_max_sz);
	    if (pmi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
		return mpi_errno;
	    }
	    if (MPIU_Strncpy(shmemkey, val, val_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    /* Get the root host and make sure local process is on the same node */
	    if (MPIU_Strncpy(key, "SHMHOST", key_max_sz))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
		return mpi_errno;
	    }
	    pmi_errno = PMI_KVS_Get(kvsname, key, val, val_max_sz);
	    if (pmi_errno != 0)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
		return mpi_errno;
	    }
	    MPID_Get_processor_name( local_host, sizeof(local_host), NULL );
	    if (strcmp(val, local_host))
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmhost", "**shmhost %s %s", local_host, val);
		return mpi_errno;
	    }
	}

	MPIU_DBG_PRINTF(("KEY = %s\n", shmemkey));
#if defined(USE_POSIX_SHM) || defined(USE_WINDOWS_SHM)
	if (MPIU_Strncpy(pgch->key, shmemkey, MPIDI_MAX_SHM_NAME_LENGTH))
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**strncpy", 0);
	    return mpi_errno;
	}
#elif defined (USE_SYSV_SHM)
	pgch->key = atoi(shmemkey);
#else
#error No shared memory subsystem defined
#endif

	mpi_errno = MPIDI_CH3I_SHM_Get_mem( pg, pg_size * shm_block, pg_rank, pg_size, TRUE );
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmgetmem", 0);
	    return mpi_errno;
	}
    }
    else
    {
	mpi_errno = MPIDI_CH3I_SHM_Get_mem( pg, shm_block, 0, 1, FALSE );
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmgetmem", 0);
	    return mpi_errno;
	}
    }

    /* initialize each shared memory queue */
    for (i=0; i<pg_size; i++)
    {
	MPIDI_CH3I_VC *vcch;
	/* MPIDI_PG_Get_vc_set_activer(pg, i, &vc); */
	/* FIXME: Move this code to the general init pg for shared
	   memory */
	vc = &pg->vct[i];
	vcch = (MPIDI_CH3I_VC *)vc->channel_private;
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
	if (pgch->pSharedProcessHandles)
	    vcch->hSharedProcessHandle = pgch->pSharedProcessHandles[i];
#else
	if (pgch->pSharedProcessIDs)
	{
	    vcch->nSharedProcessID = pgch->pSharedProcessIDs[i];
	    vcch->nSharedProcessFileDescriptor = pgch->pSharedProcessFileDescriptors[i];
	}
#endif
#endif
	if (i == pg_rank)
	{
	    vcch->shm = (MPIDI_CH3I_SHM_Queue_t*)((char*)pgch->addr + (shm_block * i));
	    for (j=0; j<pg_size; j++)
	    {
		vcch->shm[j].head_index = 0;
		vcch->shm[j].tail_index = 0;
		for (k=0; k<MPIDI_CH3I_NUM_PACKETS; k++)
		{
		    vcch->shm[j].packet[k].offset = 0;
		    vcch->shm[j].packet[k].avail = MPIDI_CH3I_PKT_AVAILABLE;
		}
	    }
	}
	else
	{
	    /*vcch->shm += pg_rank;*/
	    vcch->shm = NULL;
	    vcch->write_shmq = (MPIDI_CH3I_SHM_Queue_t*)((char*)pgch->addr + (shm_block * i)) + pg_rank;
	    vcch->read_shmq = (MPIDI_CH3I_SHM_Queue_t*)((char*)pgch->addr + (shm_block * pg_rank)) + i;
	    /* post a read of the first packet header */
	    vcch->shm_reading_pkt = TRUE;
	}
    }

#ifdef HAVE_WINDOWS_H
    {
	/* if you know the number of processors, calculate the spin count relative to that number */
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        if (info.dwNumberOfProcessors == 1)
            pgch->nShmWaitSpinCount = 1;
        else if (info.dwNumberOfProcessors < (DWORD) pg_size)
            pgch->nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * info.dwNumberOfProcessors ) / pg_size;
    }
#else
    /* figure out how many processors are available and set the spin count accordingly */
#if defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
    {
	int num_cpus;
	num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (num_cpus == 1)
	    pgch->nShmWaitSpinCount = 1;
	else if (num_cpus > 0 && num_cpus < pg_size)
            pgch->nShmWaitSpinCount = 1;
	    /* pgch->nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * num_cpus ) / pg_size; */
    }
#else
    /* if the number of cpus cannot be determined, set the spin count to 1 */
    pgch->nShmWaitSpinCount = 1;
#endif
#endif

    pmi_errno = PMI_KVS_Commit(kvsname);
    if (pmi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
	return mpi_errno;
    }
    pmi_errno = PMI_Barrier();
    if (pmi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
	return mpi_errno;
    }
#ifdef USE_POSIX_SHM
    if (shm_unlink(pgch->key))
    {
	/* Everyone is unlinking the same object so failure is ok? */
	/*
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_unlink", "**shm_unlink %s %d", pgch->key, errno);
	return mpi_errno;
	*/
    }
#elif defined (USE_SYSV_SHM)
    if (shmctl(pgch->id, IPC_RMID, NULL))
    {
	/* Everyone is removing the same object so failure is ok? */
	if (errno != EINVAL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmctl", "**shmctl %d %d", pgch->id, errno);
	    return mpi_errno;
	}
    }
#endif

 fn_fail:
    MPIU_CHKLMEM_FREEALL();
    
    return mpi_errno;
}

/* Perform the channel-specific vc initialization.  This routine is used
   in MPIDI_CH3_Init and in routines that create and initialize connections */
int MPIDI_CH3_VC_Init( MPIDI_VC_t *vc )
{
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    vcch->sendq_head         = NULL;
    vcch->sendq_tail         = NULL;

    /* Which of these do we need? */
    vcch->recv_active        = NULL;
    vcch->send_active        = NULL;
    vcch->req                = NULL;
    vcch->read_shmq          = NULL;
    vcch->write_shmq         = NULL;
    vcch->shm                = NULL;
    vcch->shm_state          = 0;
    return 0;
}

/* Free the request structure that may have been added to this vc */
int MPIDI_CH3_VC_Destroy( struct MPIDI_VC *vc )
{
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    if (vcch->req) {
	MPIU_Free( vcch->req );
    }
    return MPI_SUCCESS;
}

/* This function simply tells the CH3 device to use the defaults for the 
   MPI-2 RMA functions */
int MPIDI_CH3_RMAFnsInit( MPIDI_RMAFns *a ) 
{ 
    return 0;
}

/* This routine is a hook for initializing information for a process
   group before the MPIDI_CH3_VC_Init routine is called */
int MPIDI_CH3_PG_Init( MPIDI_PG_t *pg )
{
    /* FIXME: This should call a routine from the ch3/util/shm directory
       to initialize the use of shared memory for processes WITHIN this 
       process group */
    return MPI_SUCCESS;
}

/* This routine is a hook for any operations that need to be performed before
   freeing a process group */
int MPIDI_CH3_PG_Destroy( struct MPIDI_PG *pg )
{
    return MPI_SUCCESS;
}

/* A dummy function so that all channels provide the same set of functions, 
   enabling dll channels */
int MPIDI_CH3_InitCompleted( void )
{
    return MPI_SUCCESS;
}
