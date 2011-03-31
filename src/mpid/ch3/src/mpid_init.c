/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#define MAX_JOBID_LEN 1024

#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

/* FIXME: This does not belong here */
#ifdef USE_MPIU_DBG_PRINT_VC
char *MPIU_DBG_parent_str = "?";
#endif

/* FIXME: the PMI init function should ONLY do the PMI operations, not the 
   process group or bc operations.  These should be in a separate routine */
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

int MPIDI_Use_pmi2_api = 0;

static int InitPG( int *argc_p, char ***argv_p,
		   int *has_args, int *has_env, int *has_parent, 
		   int *pg_rank_p, MPIDI_PG_t **pg_p );
static int MPIDI_CH3I_PG_Compare_ids(void * id1, void * id2);
static int MPIDI_CH3I_PG_Destroy(MPIDI_PG_t * pg );

int MPICH_ATTR_FAILED_PROCESSES = MPI_KEYVAL_INVALID;
static int failed_procs_delete_fn(MPI_Comm comm, int keyval, void *attr_val, void *extra_data);

MPIDI_Process_t MPIDI_Process = { NULL };
MPIDI_CH3U_SRBuf_element_t * MPIDI_CH3U_SRBuf_pool = NULL;

#undef FUNCNAME
#define FUNCNAME MPID_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Init(int *argc, char ***argv, int requested, int *provided, 
	      int *has_args, int *has_env)
{
    int mpi_errno = MPI_SUCCESS;
    int has_parent;
    MPIDI_PG_t * pg=NULL;
    int pg_rank=-1;
    int pg_size;
    MPID_Comm * comm;
    int p;
    int *attr_val = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_INIT);

    /* FIXME: This is a good place to check for environment variables
       and command line options that may control the device */
    MPIDI_Use_pmi2_api = FALSE;
#ifdef USE_PMI2_API
    MPIDI_Use_pmi2_api = TRUE;
#else
    {
        int ret, val;
        ret = MPL_env2bool("MPICH_USE_PMI2_API", &val);
        if (ret == 1 && val)
            MPIDI_Use_pmi2_api = TRUE;
    }
#endif
    
#if 1
    /* This is a sanity check because we define a generic packet size
     */
    if (sizeof(MPIDI_CH3_PktGeneric_t) < sizeof(MPIDI_CH3_Pkt_t)) {
	fprintf( stderr, "Internal error - packet definition is too small.  Generic is %ld bytes, MPIDI_CH3_Pkt_t is %ld\n", (long int)sizeof(MPIDI_CH3_PktGeneric_t),
		 (long int)sizeof(MPIDI_CH3_Pkt_t) );
	exit(1);
    }
#endif

    /*
     * Set global process attributes.  These can be overridden by the channel 
     * if necessary.
     */
    MPIR_Process.attrs.tag_ub          = MPIDI_TAG_UB;

    /* If the channel requires any setup before making any other 
       channel calls (including CH3_PG_Init), the channel will define
       this routine (the dynamically loaded channel uses this) */
#ifdef HAVE_CH3_PRELOAD
    mpi_errno = MPIDI_CH3_PreLoad();
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
#endif
    /*
     * Perform channel-independent PMI initialization
     */
    mpi_errno = InitPG( argc, argv, 
			has_args, has_env, &has_parent, &pg_rank, &pg );
    if (mpi_errno) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**ch3|ch3_init");
    }
    
    /* FIXME: Why are pg_size and pg_rank handled differently? */
    pg_size = MPIDI_PG_Get_size(pg);
    MPIDI_Process.my_pg = pg;  /* brad : this is rework for shared memories 
				* because they need this set earlier
                                * for getting the business card
                                */
    MPIDI_Process.my_pg_rank = pg_rank;
    /* FIXME: Why do we add a ref to pg here? */
    MPIDI_PG_add_ref(pg);

    /* We intentionally call this before the channel init so that the channel
       can use the node_id info. */
    /* Ideally this wouldn't be needed.  Once we have PMIv2 support for node
       information we should probably eliminate this function. */
    mpi_errno = MPIDI_Populate_vc_node_ids(pg, pg_rank);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Initialize FTB after PMI init */
    mpi_errno = MPIDU_Ftb_init();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    /*
     * Let the channel perform any necessary initialization
     * The channel init should assume that PMI_Init has been called and that
     * the basic information about the job has been extracted from PMI (e.g.,
     * the size and rank of this process, and the process group id)
     */
    mpi_errno = MPIU_CALL(MPIDI_CH3,Init(has_parent, pg, pg_rank));
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**ch3|ch3_init");
    }

    /*
     * Initialize the MPI_COMM_WORLD object
     */
    comm = MPIR_Process.comm_world;

    comm->rank        = pg_rank;
    comm->remote_size = pg_size;
    comm->local_size  = pg_size;
    
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**dev|vcrt_create", 
			     "**dev|vcrt_create %s", "MPI_COMM_WORLD");
    }
    
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**dev|vcrt_get_ptr", 
			     "dev|vcrt_get_ptr %s", "MPI_COMM_WORLD");
    }
    
    /* Initialize the connection table on COMM_WORLD from the process group's
       connection table */
    for (p = 0; p < pg_size; p++)
    {
	MPID_VCR_Dup(&pg->vct[p], &comm->vcr[p]);
    }

    MPID_Dev_comm_create_hook (comm);
    mpi_errno = MPIR_Comm_commit(comm);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /*
     * Initialize the MPI_COMM_SELF object
     */
    comm = MPIR_Process.comm_self;
    comm->rank        = 0;
    comm->remote_size = 1;
    comm->local_size  = 1;
    
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**dev|vcrt_create", 
			     "**dev|vcrt_create %s", "MPI_COMM_SELF");
    }
    
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**dev|vcrt_get_ptr", 
			     "dev|vcrt_get_ptr %s", "MPI_COMM_WORLD");
    }
    
    MPID_VCR_Dup(&pg->vct[pg_rank], &comm->vcr[0]);

    MPID_Dev_comm_create_hook (comm);
    mpi_errno = MPIR_Comm_commit(comm);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Currently, mpidpre.h always defines MPID_NEEDS_ICOMM_WORLD. */
#ifdef MPID_NEEDS_ICOMM_WORLD
    /*
     * Initialize the MPIR_ICOMM_WORLD object (an internal, private version
     * of MPI_COMM_WORLD) 
     */
    comm = MPIR_Process.icomm_world;

    comm->rank        = pg_rank;
    comm->remote_size = pg_size;
    comm->local_size  = pg_size;
    MPID_VCRT_Add_ref( MPIR_Process.comm_world->vcrt );
    comm->vcrt = MPIR_Process.comm_world->vcrt;
    comm->vcr  = MPIR_Process.comm_world->vcr;
    
    MPID_Dev_comm_create_hook (comm);
    mpi_errno = MPIR_Comm_commit(comm);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
#endif
    
    /*
     * If this process group was spawned by a MPI application, then
     * form the MPI_COMM_PARENT inter-communicator.
     */

    /*
     * FIXME: The code to handle the parent case should be in a separate 
     * routine and should not rely on #ifdefs
     */
#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
    if (has_parent) {
	char * parent_port;

	/* FIXME: To allow just the "root" process to 
	   request the port and then use MPIR_Bcast_intra to 
	   distribute it to the rest of the processes,
	   we need to perform the Bcast after MPI is
	   otherwise initialized.  We could do this
	   by adding another MPID call that the MPI_Init(_thread)
	   routine would make after the rest of MPI is 
	   initialized, but before MPI_Init returns.
	   In fact, such a routine could be used to 
	   perform various checks, including parameter
	   consistency value (e.g., all processes have the
	   same environment variable values). Alternately,
	   we could allow a few routines to operate with 
	   predefined parameter choices (e.g., bcast, allreduce)
	   for the purposes of initialization. */
	mpi_errno = MPIDI_CH3_GetParentPort(&parent_port);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, 
				"**ch3|get_parent_port");
	}
	MPIU_DBG_MSG_S(CH3_CONNECT,VERBOSE,"Parent port is %s\n", parent_port);
	    
	mpi_errno = MPID_Comm_connect(parent_port, NULL, 0, 
				      MPIR_Process.comm_world, &comm);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
				 "**ch3|conn_parent", 
				 "**ch3|conn_parent %s", parent_port);
	}

	MPIR_Process.comm_parent = comm;
	MPIU_Assert(MPIR_Process.comm_parent != NULL);
	MPIU_Strncpy(comm->name, "MPI_COMM_PARENT", MPI_MAX_OBJECT_NAME);
	
	/* FIXME: Check that this intercommunicator gets freed in MPI_Finalize
	   if not already freed.  */
    }
#endif	
    
    /*
     * Set provided thread level
     */
    if (provided != NULL)
    {
	/* This must be min(requested,MPICH_THREAD_LEVEL) if runtime
	   control of thread level is available */
	*provided = (MPICH_THREAD_LEVEL < requested) ? 
	    MPICH_THREAD_LEVEL : requested;
    }

    /* create attribute to list failed processes */
    mpi_errno = MPIR_Comm_create_keyval_impl(MPI_COMM_NULL_COPY_FN,
                                             failed_procs_delete_fn,
                                             &MPICH_ATTR_FAILED_PROCESSES, 0);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    attr_val = MPIU_Malloc(sizeof(int));
    if (!attr_val) { MPIU_CHKMEM_SETERR(mpi_errno, sizeof(int), "attr_val"); goto fn_fail; }
    *attr_val = MPI_PROC_NULL;
    mpi_errno = MPIR_Comm_set_attr_impl(MPIR_Process.comm_world, MPICH_ATTR_FAILED_PROCESSES, attr_val, MPIR_ATTR_PTR);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_INIT);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* This allows each channel to perform final initialization after the
 rest of MPI_Init completes.  */
int MPID_InitCompleted( void )
{
    int mpi_errno;
    mpi_errno = MPIU_CALL(MPIDI_CH3,InitCompleted());
    return mpi_errno;
}

/*
 * Initialize the process group structure by using PMI calls.
 * This routine initializes PMI and uses PMI calls to setup the 
 * process group structures.
 * 
 */
static int InitPG( int *argc, char ***argv, 
		   int *has_args, int *has_env, int *has_parent, 
		   int *pg_rank_p, MPIDI_PG_t **pg_p )
{
    int pmi_errno;
    int mpi_errno = MPI_SUCCESS;
    int pg_rank, pg_size, appnum, pg_id_sz;
    int usePMI=1;
    char *pg_id;
    MPIDI_PG_t *pg = 0;

    /* See if the channel will provide the PMI values.  The channel
     is responsible for defining HAVE_CH3_PRE_INIT and providing 
    the MPIDI_CH3_Pre_init function.  */
    /* FIXME: Document this */
#ifdef HAVE_CH3_PRE_INIT
    {
	int setvals;
	mpi_errno = MPIDI_CH3_Pre_init( &setvals, has_parent, &pg_rank, 
					&pg_size );
	if (mpi_errno) {
	    goto fn_fail;
	}
	if (setvals) usePMI = 0;
    }
#endif 

    /* If we use PMI here, make the PMI calls to get the
       basic values.  Note that systems that return setvals == true
       do not make use of PMI for the KVS routines either (it is
       assumed that the discover connection information through some
       other mechanism */
    /* FIXME: We may want to allow the channel to ifdef out the use
       of PMI calls, or ask the channel to provide stubs that 
       return errors if the routines are in fact used */
    if (usePMI) {
	/*
	 * Initialize the process manangement interface (PMI), 
	 * and get rank and size information about our process group
	 */

#ifdef USE_PMI2_API
        mpi_errno = PMI2_Init(has_parent, &pg_size, &pg_rank, &appnum);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
#else
	pmi_errno = PMI_Init(has_parent);
	if (pmi_errno != PMI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_init",
			     "**pmi_init %d", pmi_errno);
	}

	pmi_errno = PMI_Get_rank(&pg_rank);
	if (pmi_errno != PMI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_rank",
			     "**pmi_get_rank %d", pmi_errno);
	}

	pmi_errno = PMI_Get_size(&pg_size);
	if (pmi_errno != 0) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_size",
			     "**pmi_get_size %d", pmi_errno);
	}
	
	pmi_errno = PMI_Get_appnum(&appnum);
	if (pmi_errno != PMI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_appnum",
				 "**pmi_get_appnum %d", pmi_errno);
	}
#endif
	/* Note that if pmi is not availble, the value of MPI_APPNUM is 
	   not set */
	if (appnum != -1) {
	    MPIR_Process.attrs.appnum = appnum;
	}

#ifdef USE_PMI2_API
        
        /* This memory will be freed by the PG_Destroy if there is an error */
	pg_id = MPIU_Malloc(MAX_JOBID_LEN);
	if (pg_id == NULL) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
				 MAX_JOBID_LEN);
	}

        mpi_errno = PMI2_Job_GetId(pg_id, MAX_JOBID_LEN);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        

#else
	/* Now, initialize the process group information with PMI calls */
	/*
	 * Get the process group id
	 */
	pmi_errno = PMI_KVS_Get_name_length_max(&pg_id_sz);
	if (pmi_errno != PMI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
				 "**pmi_get_id_length_max", 
				 "**pmi_get_id_length_max %d", pmi_errno);
	}

	/* This memory will be freed by the PG_Destroy if there is an error */
	pg_id = MPIU_Malloc(pg_id_sz + 1);
	if (pg_id == NULL) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
				 pg_id_sz+1);
	}

	/* Note in the singleton init case, the pg_id is a dummy.
	   We'll want to replace this value if we join an 
	   Process manager */
	pmi_errno = PMI_KVS_Get_my_name(pg_id, pg_id_sz);
	if (pmi_errno != PMI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_id",
				 "**pmi_get_id %d", pmi_errno);
	}
#endif
    }
    else {
	/* Create a default pg id */
	pg_id = MPIU_Malloc(2);
	if (pg_id == NULL) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**nomem");
	}
	MPIU_Strncpy( pg_id, "0", 2 );
    }

    /*
     * Initialize the process group tracking subsystem
     */
    mpi_errno = MPIDI_PG_Init(argc, argv, 
			     MPIDI_CH3I_PG_Compare_ids, MPIDI_CH3I_PG_Destroy);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**dev|pg_init");
    }

    /*
     * Create a new structure to track the process group for our MPI_COMM_WORLD
     */
    mpi_errno = MPIDI_PG_Create(pg_size, pg_id, &pg);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**dev|pg_create");
    }

    /* FIXME: We can allow the channels to tell the PG how to get
       connection information by passing the pg to the channel init routine */
    if (usePMI) {
	/* Tell the process group how to get connection information */
        mpi_errno = MPIDI_PG_InitConnKVS( pg );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* FIXME: Who is this for and where does it belong? */
#ifdef USE_MPIU_DBG_PRINT_VC
    MPIU_DBG_parent_str = (*has_parent) ? "+" : "";
#endif

    /* FIXME: has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env  = TRUE;

    *pg_p      = pg;
    *pg_rank_p = pg_rank;
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (pg) {
	MPIDI_PG_Destroy( pg );
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/*
 * Create the storage for the business card. 
 *
 * The routine MPIDI_CH3I_BCFree should be called with the original 
 * value *bc_val_p .  Note that the routines that set the value 
 * of the businesscard return a pointer to the first free location,
 * so you need to remember the original location in order to free 
 * it later.
 *
 */
int MPIDI_CH3I_BCInit( char **bc_val_p, int *val_max_sz_p )
{
    int pmi_errno;
    int mpi_errno = MPI_SUCCESS;
#ifdef USE_PMI2_API
    *val_max_sz_p = PMI2_MAX_VALLEN;
#else
    pmi_errno = PMI_KVS_Get_value_length_max(val_max_sz_p);
    if (pmi_errno != PMI_SUCCESS)
    {
        MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
                             "**pmi_kvs_get_value_length_max",
                             "**pmi_kvs_get_value_length_max %d", pmi_errno);
    }
#endif
    /* This memroy is returned by this routine */
    *bc_val_p = MPIU_Malloc(*val_max_sz_p);
    if (*bc_val_p == NULL) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**nomem","**nomem %d",
			     *val_max_sz_p);
    }
    
    /* Add a null to simplify looking at the bc */
    **bc_val_p = 0;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* Free the business card.  This routine should be called once the business
   card is published. */
int MPIDI_CH3I_BCFree( char *bc_val )
{
    /* */
    if (bc_val) {
	MPIU_Free( bc_val );
    }
    
    return 0;
}

/* FIXME: The PG code should supply these, since it knows how the 
   pg_ids and other data are represented */
static int MPIDI_CH3I_PG_Compare_ids(void * id1, void * id2)
{
    return (strcmp((char *) id1, (char *) id2) == 0) ? TRUE : FALSE;
}


static int MPIDI_CH3I_PG_Destroy(MPIDI_PG_t * pg)
{
    if (pg->id != NULL)
    { 
	MPIU_Free(pg->id);
    }
    
    return MPI_SUCCESS;
}

static int failed_procs_delete_fn(MPI_Comm comm ATTRIBUTE((unused)),
                                  int keyval ATTRIBUTE((unused)),
                                  void *attr_val,
                                  void *extra_data ATTRIBUTE((unused)))
{
    MPIU_UNREFERENCED_ARG(comm);
    MPIU_UNREFERENCED_ARG(keyval);
    MPIU_UNREFERENCED_ARG(extra_data);

    MPIU_Free(attr_val);
    return MPI_SUCCESS;
}

