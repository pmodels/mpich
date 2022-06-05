/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

/* FIXME: This does not belong here */
#ifdef USE_MPIDI_DBG_PRINT_VC
char *MPIDI_DBG_parent_str = "?";
#endif

#include "datatype.h"

static int init_pg(int *has_parent, int *pg_rank_p, MPIDI_PG_t **pg_p);
static int pg_compare_ids(void * id1, void * id2);
static int pg_destroy(MPIDI_PG_t * pg );

MPIDI_Process_t MPIDI_Process;
MPIDI_CH3U_SRBuf_element_t * MPIDI_CH3U_SRBuf_pool;
MPIDI_CH3U_Win_fns_t MPIDI_CH3U_Win_fns;
MPIDI_CH3U_Win_hooks_t MPIDI_CH3U_Win_hooks;
MPIDI_CH3U_Win_pkt_ordering_t MPIDI_CH3U_Win_pkt_orderings;

#if defined(MPL_USE_DBG_LOGGING)
MPL_dbg_class MPIDI_CH3_DBG_CONNECT;
MPL_dbg_class MPIDI_CH3_DBG_DISCONNECT;
MPL_dbg_class MPIDI_CH3_DBG_PROGRESS;
MPL_dbg_class MPIDI_CH3_DBG_CHANNEL;
MPL_dbg_class MPIDI_CH3_DBG_OTHER;
MPL_dbg_class MPIDI_CH3_DBG_MSG;
MPL_dbg_class MPIDI_CH3_DBG_VC;
MPL_dbg_class MPIDI_CH3_DBG_REFCOUNT;
#endif /* MPL_USE_DBG_LOGGING */

static int finalize_failed_procs_group(void *param)
{
    int mpi_errno = MPI_SUCCESS;
    if (MPIDI_Failed_procs_group != MPIR_Group_empty) {
        mpi_errno = MPIR_Group_free_impl(MPIDI_Failed_procs_group);
        MPIR_ERR_CHECK(mpi_errno);
    }
    
 fn_fail:
    return mpi_errno;
}

static int init_local(int requested, int *provided);
static int init_world(void);

int MPID_Init(int requested, int *provided)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = init_local(requested, provided);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = init_world();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int init_local(int requested, int *provided)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPICH_THREAD_LEVEL >= requested)
        *provided = requested;
    else
        *provided = MPICH_THREAD_LEVEL;

    /* initialization routine for ch3u_comm.c */
    mpi_errno = MPIDI_CH3I_Comm_init();
    MPIR_ERR_CHECK(mpi_errno);
    
    /* init group of failed processes, and set finalize callback */
    MPIDI_Failed_procs_group = MPIR_Group_empty;
    MPIR_Add_finalize(finalize_failed_procs_group, NULL, MPIR_FINALIZE_CALLBACK_PRIO-1);

    /* Create the string that will cache the last group of failed processes
     * we received from PMI */
    int max_vallen;
    max_vallen = MPIR_pmi_max_val_size() + 1;
    MPIDI_failed_procs_string = MPL_malloc(sizeof(char) * max_vallen, MPL_MEM_STRINGS);

    /*
     * Set global process attributes.  These can be overridden by the channel 
     * if necessary.
     */
    MPIR_Process.attrs.io = MPI_ANY_SOURCE;

    /*
     * Perform channel-independent PMI initialization
     */
    MPIDI_PG_t * pg=NULL;
    int has_parent, pg_rank;
    mpi_errno = init_pg(&has_parent, &pg_rank, &pg);
    if (mpi_errno) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**ch3|ch3_init");
    }
    
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
    MPIR_ERR_CHECK(mpi_errno);

    /* Initialize Window functions table with defaults, then call the channel's
       init function. */
    MPIDI_Win_fns_init(&MPIDI_CH3U_Win_fns);
    MPIDI_CH3_Win_fns_init(&MPIDI_CH3U_Win_fns);
    MPIDI_CH3_Win_hooks_init(&MPIDI_CH3U_Win_hooks);

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH3_DBG_CONNECT = MPL_dbg_class_alloc("CH3_CONNECT", "ch3_connect");;
    MPIDI_CH3_DBG_DISCONNECT = MPL_dbg_class_alloc("CH3_DISCONNECT", "ch3_disconnect");
    MPIDI_CH3_DBG_PROGRESS = MPL_dbg_class_alloc("CH3_PROGRESS", "ch3_progress");
    MPIDI_CH3_DBG_CHANNEL = MPL_dbg_class_alloc("CH3_CHANNEL", "ch3_channel");
    MPIDI_CH3_DBG_OTHER = MPL_dbg_class_alloc("CH3_OTHER", "ch3_other");
    MPIDI_CH3_DBG_MSG = MPL_dbg_class_alloc("CH3_MSG", "ch3_msg");
    MPIDI_CH3_DBG_VC = MPL_dbg_class_alloc("VC", "vc");
    MPIDI_CH3_DBG_REFCOUNT = MPL_dbg_class_alloc("REFCOUNT", "refcount");
#endif /* MPL_USE_DBG_LOGGING */

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int init_world(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    /*
     * Let the channel perform any necessary initialization
     * The channel init should assume that PMI_Init has been called and that
     * the basic information about the job has been extracted from PMI (e.g.,
     * the size and rank of this process, and the process group id)
     */
    int has_parent = MPIR_Process.has_parent;
    mpi_errno = MPIDI_CH3_Init(has_parent, MPIDI_Process.my_pg, MPIR_Process.rank);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**ch3|ch3_init");
    }

    /* setup receive queue statistics */
    mpi_errno = MPIDI_CH3U_Recvq_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* Ask channel to expose Window packet ordering. */
    MPIDI_CH3_Win_pkt_orderings_init(&MPIDI_CH3U_Win_pkt_orderings);

    MPIR_Comm_register_hint(MPIR_COMM_HINT_EAGER_THRESH, "eager_rendezvous_threshold",
                            NULL, MPIR_COMM_HINT_TYPE_INT, 0, 0);

    mpi_errno = MPIDI_RMA_init();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

static int init_spawn(void)
{
    int mpi_errno = MPI_SUCCESS;
    char * parent_port;
    MPIR_FUNC_ENTER;
#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS

    /* FIXME: To allow just the "root" process to
       request the port and then use MPIR_Bcast_allcomm_auto to
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
        MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
                            "**ch3|get_parent_port");
    }
    MPL_DBG_MSG_S(MPIDI_CH3_DBG_CONNECT,VERBOSE,"Parent port is %s", parent_port);

    mpi_errno = MPID_Comm_connect(parent_port, NULL, 0, MPIR_Process.comm_world,
                                  &MPIR_Process.comm_parent);
    MPIR_ERR_CHKANDJUMP1(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                         "**ch3|conn_parent",
                         "**ch3|conn_parent %s", parent_port);

    MPIR_Assert(MPIR_Process.comm_parent != NULL);
    MPL_strncpy(MPIR_Process.comm_parent->name, "MPI_COMM_PARENT", MPI_MAX_OBJECT_NAME);

    /* FIXME: Check that this intercommunicator gets freed in MPI_Finalize
       if not already freed.  */
#endif
    MPIR_FUNC_EXIT;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This allows each channel to perform final initialization after the
 rest of MPI_Init completes.  */
int MPID_InitCompleted( void )
{
    int mpi_errno;

    if (MPIR_Process.has_parent) {
        mpi_errno = init_spawn();
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIDI_CH3_InitCompleted();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int MPID_Allocate_vci(int *vci)
{
    int mpi_errno = MPI_SUCCESS;
    *vci = 0;
    MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**ch3nostream");
    return mpi_errno;
}

int MPID_Deallocate_vci(int vci)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}
/*
 * Initialize the process group structure by using PMI calls.
 * This routine initializes PMI and uses PMI calls to setup the 
 * process group structures.
 * 
 */
static int init_pg(int *has_parent, int *pg_rank_p, MPIDI_PG_t **pg_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pg_rank, pg_size, appnum;
    int usePMI=1;
    char *pg_id;
    MPIDI_PG_t *pg = 0;

    *pg_rank_p = -1;

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
	 * rank and size information about our process group
	 */
        *has_parent = MPIR_Process.has_parent;
        pg_rank = MPIR_Process.rank;
        pg_size = MPIR_Process.size;
        appnum = MPIR_Process.appnum;

	/* Note that if pmi is not available, the value of MPI_APPNUM is 
	   not set */
	if (appnum != -1) {
	    MPIR_Process.attrs.appnum = appnum;
	}
        
        pg_id = MPL_strdup(MPIR_pmi_job_id());
    }
    else {
	pg_id = MPL_strdup("0");
    }

    /*
     * Initialize the process group tracking subsystem
     */
    mpi_errno = MPIDI_PG_Init(pg_compare_ids, pg_destroy);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**dev|pg_init");
    }

    /*
     * Create a new structure to track the process group for our MPI_COMM_WORLD
     */
    mpi_errno = MPIDI_PG_Create(pg_size, pg_id, &pg);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**dev|pg_create");
    }

    /* FIXME: We can allow the channels to tell the PG how to get
       connection information by passing the pg to the channel init routine */
    if (usePMI) {
	/* Tell the process group how to get connection information */
        mpi_errno = MPIDI_PG_InitConnKVS( pg );
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* FIXME: Who is this for and where does it belong? */
#ifdef USE_MPIDI_DBG_PRINT_VC
    MPIDI_DBG_parent_str = (*has_parent) ? "+" : "";
#endif

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
    int mpi_errno = MPI_SUCCESS;

    *val_max_sz_p = MPIR_pmi_max_val_size();

    /* This memory is returned by this routine */
    *bc_val_p = MPL_malloc(*val_max_sz_p, MPL_MEM_ADDRESS);
    if (*bc_val_p == NULL) {
	MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**nomem","**nomem %d",
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
    MPL_free( bc_val );
    
    return 0;
}

/* FIXME: The PG code should supply these, since it knows how the 
   pg_ids and other data are represented */
static int pg_compare_ids(void * id1, void * id2)
{
    return (strcmp((char *) id1, (char *) id2) == 0) ? TRUE : FALSE;
}


static int pg_destroy(MPIDI_PG_t * pg)
{
    MPL_free(pg->id);

    return MPI_SUCCESS;
}

