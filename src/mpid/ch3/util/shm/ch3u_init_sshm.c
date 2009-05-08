/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#include "mpidi_ch3_impl.h"
#include "pmi.h"

static int getNumProcessors( void );
static int getNodeRootRank( int, int * );

/* FIXME: This file contains too many ifdefs.  A cleaner design is needed; 
   examples may be found in MPICH1.  For example, separate files for each 
   type of shared memory and common routines implementing a shared memory
   abstraction */

/*  MPIDI_CH3U_Init_sshm - does scalable shared memory specific channel 
 *  initialization
 *     bc_val     - business card value buffer pointer, updated to the next 
 *                  available location or freed if published.
 *     val_max_sz - maximum value buffer size reduced by the number of 
 *                  characters written
 *                               
 */

/* This routine is used only by channels/{sshm,ssm}/src/ch3_init.c */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Init_sshm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Init_sshm(int has_parent, MPIDI_PG_t *pg_p, int pg_rank,
			 char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int pg_size;
    int p;
    MPIDI_CH3I_PG *pgch;
#ifdef USE_PERSISTENT_SHARED_MEMORY
    char * parent_bizcard = NULL;
#endif
#ifdef USE_MQSHM
    char queue_name[MPIDI_MAX_SHM_NAME_LENGTH];
    int initialize_queue = 0;
    int root_rank = 0;
#endif
    int key_max_sz;
    int val_max_sz;
    char * key = NULL;
    char * val = NULL;
    char *kvsname = NULL;
    MPIU_CHKLMEM_DECL(2);

    srand(getpid()); /* brad : needed by generate_shm_string */

    pgch = (MPIDI_CH3I_PG *)pg_p->channel_private;

    MPIDI_CH3I_Process.shm_reading_list = NULL;
    MPIDI_CH3I_Process.shm_writing_list = NULL;
    MPIDI_CH3I_Process.num_cpus = -1;

    /* Get the kvsname associated with MPI_COMM_WORLD */
    MPIDI_PG_GetConnKVSname( &kvsname );

    /* brad : need to set these locally */
    pmi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
    MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get_key_length_max",
                         "**pmi_kvs_get_key_length_max %d", pmi_errno);
    MPIU_CHKLMEM_MALLOC(key, char *, key_max_sz, mpi_errno, "key");

    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER,
                         "**pmi_kvs_get_value_length_max",
                         "**pmi_kvs_get_value_length_max %d", pmi_errno);
    MPIU_CHKLMEM_MALLOC(val,char *,val_max_sz,mpi_errno,"val");

#ifdef MPIDI_CH3_USES_SHM_NAME
    pgch->shm_name = NULL;
    pgch->shm_name = MPIU_Malloc(sizeof(char) * MPIDI_MAX_SHM_NAME_LENGTH);
    if (pgch->shm_name == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**nomem");
    }
#endif

    /* set the global variable defaults */
    pgch->nShmEagerLimit = MPIDI_SHM_EAGER_LIMIT;
#ifdef HAVE_SHARED_PROCESS_READ
    pgch->nShmRndvLimit = MPIDI_SHM_RNDV_LIMIT;
#endif
    pgch->nShmWaitSpinCount = MPIDI_CH3I_SPIN_COUNT_DEFAULT;
    pgch->nShmWaitYieldCount = MPIDI_CH3I_YIELD_COUNT_DEFAULT;

    /* Figure out how many processors are available and set the spin count 
       accordingly */
    /* If there were topology information available we could calculate a 
       multi-cpu number */
    {
	int ncpus = getNumProcessors();
	/* if you know the number of processors, calculate the spin count 
	   relative to that number */
        if (ncpus == 1)
            pgch->nShmWaitSpinCount = 1;
	/* FIXME: Why is this commented out? */
	/*
        else if (ncpus  < num_procs_per_node)
            pg->ch.nShmWaitSpinCount = ( MPIDI_CH3I_SPIN_COUNT_DEFAULT * ncpus) / num_procs_per_node;
	*/
	if (ncpus > 0)
	    MPIDI_CH3I_Process.num_cpus = ncpus;
    }

/* FIXME: This code probably reflects a bug caused by commenting out the above
   code */
#ifndef HAVE_WINDOWS_H    /* brad - nShmWaitSpinCount is uninitialized in sshm but probably shouldn't be */
    pgch->nShmWaitSpinCount = 1;
/* FIXME: MPIU_g_nLockSpinCount is in a different module
   (mpid/common/locks) where it is used (and defined) only in some cases. 
   Commenting out this bogus access for now; uncommenting it may cause
   link failures. Update - see FIXME in mpidu_process_locks - there are
   serious problem with how configuration information is passed to 
   mpidu_process_locks.h , leading to *different* choices in the files
   in mpid/ch3 that include it (like this one) and in mpidu_process_locks.c 
   (!).  For that reason, I've left this statement in the code, but 
   once mpidu_process_locks is fixed, this will need to be as well. */
    MPIU_g_nLockSpinCount = 1;
#endif

    pmi_errno = PMI_Get_size(&pg_size);
    if (pmi_errno != 0) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_size",
			     "**pmi_get_size %d", pmi_errno);
    }

    /* FIXME: Is this initialization correct (it should have been done
       when the pg was created. If not, then the process by which channels
       and PG's are initialized needs to be documented and explained! 
       An alternate approach to having this done after the PG is created
       is to define a method initialization routine that is called
       after the pg is created but before the final, channel-specific 
       virtual connection initialization is done. */
    /* Initialize the VC table associated with this process group (and thus 
       COMM_WORLD) */
    /* Note that this is *not* called with MPIU_CALL.  This routine is 
       not included within the general dllchan channel, and hence this
       routine may directly call the specific channel's VC_Init routine */
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3_VC_Init( &pg_p->vct[p] );
    }

    /* brad : do the shared memory specific setup items so we can later do the
     *         shared memory aspects of the business card
     */

    MPID_Get_processor_name( pgch->shm_hostname, 
			     sizeof(pgch->shm_hostname), 0);

#ifdef MPIDI_CH3_USES_SHM_NAME
    MPIDI_Process.my_pg = pg_p;  /* was later prior but internally 
				    GetParentPort needs this */    
#ifdef USE_PERSISTENT_SHARED_MEMORY
    if (has_parent) /* set in PMI_Init */
    {
        mpi_errno = MPIDI_CH3_GetParentPort(&parent_bizcard);
        if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				"**ch3|get_parent_port");
        }

	/* Parse the shared memory queue name from the bizcard */
	{
	    char *orig_str, *tmp_str = MPIU_Malloc(sizeof(char) * MPIDI_MAX_SHM_NAME_LENGTH);
	    if (tmp_str == NULL) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    mpi_errno = MPIU_Str_get_string_arg(parent_bizcard, 
		 MPIDI_CH3I_SHM_QUEUE_KEY, tmp_str, MPIDI_MAX_SHM_NAME_LENGTH);
	    if (mpi_errno != MPIU_STR_SUCCESS) {
		MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
				     "**fail", "**fail %d", mpi_errno);
	    }
	    orig_str = tmp_str;
	    while (*tmp_str != ':' && *tmp_str != '\0')
		tmp_str++;
	    if (*tmp_str == ':')
	    {
		tmp_str++;
		mpi_errno = MPIU_Strncpy(pgch->shm_name, tmp_str, 
					 MPIDI_MAX_SHM_NAME_LENGTH);
		MPIU_Free(orig_str);
		if (mpi_errno != 0) {
		    MPIU_ERR_POP(mpi_errno);
		}
	    }
	    else
	    {
		MPIU_Free(orig_str);
		MPIU_ERR_POP(mpi_errno);
	    }
	}
    } /* has_parent */
#else
    /* NOTE: Do not use shared memory to communicate to parent */
    pgch->shm_name[0] = 0;
#endif
#endif            

#ifdef USE_MQSHM

    MPIU_Strncpy(key, MPIDI_CH3I_SHM_QUEUE_NAME_KEY, key_max_sz );
    if (pg_rank == 0)
    {
#ifdef USE_PERSISTENT_SHARED_MEMORY
	/* With persistent shared memory, only the first process of the first
	   group needs to create the bootstrap queue. */
	/* Everyone else including spawned processes will attach to this queue.
	 */
	if (has_parent)
	{
	    /* If you have a parent then you must not initialize the queue 
	       since the parent already did. */
	    initialize_queue = 0;
	    MPIU_Strncpy(queue_name, pgch->shm_name, MPIDI_MAX_SHM_NAME_LENGTH);
	    MPIU_Strncpy(val, queue_name, val_max_sz);
	}
	else
#else
	/* Without persistent shared memory the root process of each process 
	 * group creates a unique
	 * bootstrap queue to be used only by processes within the same process
	 * group */
#endif
	{
	    mpi_errno = MPIDI_CH3I_BootstrapQ_create_unique_name(queue_name, MPIDI_MAX_SHM_NAME_LENGTH);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**boot_create");
	    }
	    /* If you don't have a parent then you must initialize the queue */
	    initialize_queue = 1;
	    MPIU_Strncpy(val, queue_name, val_max_sz);
	    MPIU_Strncpy(pgch->shm_name, val, val_max_sz);
	}

	mpi_errno = MPIDI_CH3I_BootstrapQ_create_named(&pgch->bootstrapQ, 
					       queue_name, initialize_queue);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**boot_create");
	}
	/*printf("root process created bootQ: '%s'\n", queue_name);fflush(stdout);*/

	pmi_errno = PMI_KVS_Put(kvsname, key, val);          
        MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);

	pmi_errno = PMI_KVS_Commit(kvsname);
        MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
	
	pmi_errno = PMI_Barrier();
        MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
    }
    else
    {
	pmi_errno = PMI_Barrier();
        MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
	
	pmi_errno = PMI_KVS_Get(kvsname, key, val, val_max_sz);
        MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);

	MPIU_Strncpy(queue_name, val, MPIDI_MAX_SHM_NAME_LENGTH);
#ifdef MPIDI_CH3_USES_SHM_NAME
	MPIU_Strncpy(pgch->shm_name, val, MPIDI_MAX_SHM_NAME_LENGTH);
#endif
	/*printf("process %d got bootQ name: '%s'\n", pg_rank, queue_name);fflush(stdout);*/
	mpi_errno = getNodeRootRank(pg_rank, &root_rank);
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**node_root_rank");
	}
	if (pg_rank == root_rank)
	{
#ifdef USE_PERSISTENT_SHARED_MEMORY
	    /* With persistent shared memory, only the first process of the 
	       first group needs to create the bootstrap queue. */
	    /* Everyone else including spawned processes will attach to this 
	       queue. */
	    initialize_queue = !has_parent;
#else
	    /* Without persistent shared memory the root of each node 
	       initializes the queue */
	    initialize_queue = 1;
#endif
	}
	else
	{
	    /* The root process initialized the queue */
	    initialize_queue = 0;
	}
	mpi_errno = MPIDI_CH3I_BootstrapQ_create_named(&pgch->bootstrapQ, 
					       queue_name, initialize_queue);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**boot_create");
	}
    }
    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

#ifdef USE_PERSISTENT_SHARED_MEMORY
    /* The bootstrap queue cannot be unlinked because it can be used outside 
       of this process group. */
    /* Spawned groups will use it and other MPI jobs may use it by calling 
       MPI_Comm_connect/accept */
    /* By not unlinking here, if the program aborts, the 
     * shared memory segments can be left dangling.
     */
#else
    /* Unlinking here prevents leaking shared memory segments but also 
     *  prevents any other processes
     * from attaching to the segment later thus preventing the implementation 
     * of MPI_Comm_connect/accept/spawn/spawn_multiple
     */
    mpi_errno = MPIDI_CH3I_BootstrapQ_unlink(pgch->bootstrapQ);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**boot_unlink");
    }
#endif

#else

    mpi_errno = MPIDI_CH3I_BootstrapQ_create(&pgch->bootstrapQ);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**boot_create");
    }

#endif

    /* brad : the pg needs to be set for sshm channels.  for all channels this
       is done in mpid_init.c */
    MPIDI_Process.my_pg = pg_p;

    /* brad : get the sshm part of the business card  */
    mpi_errno = MPIDI_CH3U_Get_business_card_sshm(bc_val_p, val_max_sz_p);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_buscard");
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (pg_p != NULL)
    {
	MPIDI_PG_Destroy( pg_p );
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* This routine initializes shm-specific elements of the VC */
int MPIDI_VC_InitShm( MPIDI_VC_t *vc ) 
{
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    vcch->recv_active        = NULL;
    vcch->send_active        = NULL;
    vcch->req                = NULL;
    vcch->read_shmq          = NULL;
    vcch->write_shmq         = NULL;
    vcch->shm                = NULL;
    vcch->shm_state          = 0;
    vcch->shm_next_reader    = NULL;
    vcch->shm_next_writer    = NULL;
    vcch->shm_read_connected = 0;
    return 0;
}

/* Return the number of processors, or one if the number cannot be 
   determined */
#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTL) && \
    !defined(_POSIX_C_SOURCE)
#include <sys/types.h>
#include <sys/sysctl.h>
#define USE_SYSCTL
#endif

static int getNumProcessors( void )
{
#ifdef HAVE_WINDOWS_H
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
#elif defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
    int num_cpus;
    num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    return num_cpus;
#elif defined(USE_SYSCTL) && defined(CTL_HW) && defined(HW_NCPU)
    int num_cpus, rc, len=sizeof(int);
    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    rc = sysctl( mib, 2, &num_cpus, &len, (void *)0, 0 );
    if (rc != 0) num_cpus = 1;
    return num_cpus;
#else
    return 1;
#endif
}

/* Return the lowest rank on the same node as this process */
#undef FUNCNAME
#define FUNCNAME getNodeRootRank
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int getNodeRootRank(int pg_rank, int *root_rank)
{
    int *ranks = NULL, num_ranks, min_rank;
    int pmi_errno, i;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);

    pmi_errno = PMI_Get_clique_size(&num_ranks);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pmi_get_clique_size", 
			     "**pmi_get_clique_size %d", pmi_errno);
    }

    if (num_ranks < 1)
    {
	/* PMI_Get_clique_size returned an invalid size */
	MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
	    "**pmi_invalid_clique_size",
	    "**pmi_invalid_clique_size %d", num_ranks);
    }

    MPIU_CHKLMEM_MALLOC(ranks, int *, sizeof(int) * num_ranks, mpi_errno, "an array of ranks");

    pmi_errno = PMI_Get_clique_ranks(ranks, num_ranks);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pmi_get_clique_ranks", 
			     "**pmi_get_clique_ranks %d", pmi_errno);
    }

    min_rank = ranks[0];
    /* If the list is not sorted then find the lowest rank */
    for (i=1; i<num_ranks; i++)
    {
	min_rank = MPIU_MIN(min_rank, ranks[i]);
    }

    *root_rank = min_rank;
fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
