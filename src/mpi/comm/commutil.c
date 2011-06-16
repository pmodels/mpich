/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* This is the utility file for comm that contains the basic comm items
   and storage management */
#ifndef MPID_COMM_PREALLOC
#define MPID_COMM_PREALLOC 8
#endif

/* Preallocated comm objects */
/* initialized in initthread.c */
MPID_Comm MPID_Comm_builtin[MPID_COMM_N_BUILTIN] = { {0} };
MPID_Comm MPID_Comm_direct[MPID_COMM_PREALLOC]   = { {0} };
MPIU_Object_alloc_t MPID_Comm_mem = { 0, 0, 0, 0, MPID_COMM, 
				      sizeof(MPID_Comm), MPID_Comm_direct,
                                      MPID_COMM_PREALLOC};

/* utility function to pretty print a context ID for debugging purposes, see
 * mpiimpl.h for more info on the various fields */
#ifdef USE_DBG_LOGGING
static void MPIR_Comm_dump_context_id(MPIR_Context_id_t context_id, char *out_str, int len)
{
    int subcomm_type = MPID_CONTEXT_READ_FIELD(SUBCOMM,context_id);
    const char *subcomm_type_name = NULL;

    switch (subcomm_type) {
        case 0: subcomm_type_name = "parent"; break;
        case 1: subcomm_type_name = "intranode"; break;
        case 2: subcomm_type_name = "internode"; break;
        default: MPIU_Assert(FALSE); break;
    }
    MPIU_Snprintf(out_str, len,
                  "context_id=%d (%#x): DYNAMIC_PROC=%d PREFIX=%#x IS_LOCALCOMM=%d SUBCOMM=%s SUFFIX=%s",
                  context_id,
                  context_id,
                  MPID_CONTEXT_READ_FIELD(DYNAMIC_PROC,context_id),
                  MPID_CONTEXT_READ_FIELD(PREFIX,context_id),
                  MPID_CONTEXT_READ_FIELD(IS_LOCALCOMM,context_id),
                  subcomm_type_name,
                  (MPID_CONTEXT_READ_FIELD(SUFFIX,context_id) ? "coll" : "pt2pt"));
}
#endif

/* FIXME :
   Reusing context ids can lead to a race condition if (as is desirable)
   MPI_Comm_free does not include a barrier.  Consider the following:
   Process A frees the communicator.
   Process A creates a new communicator, reusing the just released id
   Process B sends a message to A on the old communicator.
   Process A receives the message, and believes that it belongs to the
   new communicator.
   Process B then cancels the message, and frees the communicator.

   The likelyhood of this happening can be reduced by introducing a gap
   between when a context id is released and when it is reused.  An alternative
   is to use an explicit message (in the implementation of MPI_Comm_free)
   to indicate that a communicator is being freed; this will often require
   less communication than a barrier in MPI_Comm_free, and will ensure that 
   no messages are later sent to the same communicator (we may also want to
   have a similar check when building fault-tolerant versions of MPI).
 */

/* Zeroes most non-handle fields in a communicator, as well as initializing any
 * other special fields, such as a per-object mutex.  Also defaults the
 * reference count to 1, under the assumption that the caller holds a reference
 * to it.
 *
 * !!! The resulting struct is _not_ ready for communication !!! */
int MPIR_Comm_init(MPID_Comm *comm_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Object_set_ref(comm_p, 1);

    MPIU_THREAD_MPI_OBJ_INIT(comm_p);

    /* Clear many items (empty means to use the default; some of these
       may be overridden within the upper-level communicator initialization) */
    comm_p->errhandler   = NULL;
    comm_p->attributes   = NULL;
    comm_p->remote_group = NULL;
    comm_p->local_group  = NULL;
    comm_p->coll_fns     = NULL;
    comm_p->topo_fns     = NULL;
    comm_p->name[0]      = '\0';

    comm_p->hierarchy_kind  = MPID_HIERARCHY_FLAT;
    comm_p->node_comm       = NULL;
    comm_p->node_roots_comm = NULL;
    comm_p->intranode_table = NULL;
    comm_p->internode_table = NULL;

    /* Fields not set include context_id, remote and local size, and
       kind, since different communicator construction routines need
       different values */
fn_fail:
    return mpi_errno;
}


/*
    Create a communicator structure and perform basic initialization 
    (mostly clearing fields and updating the reference count).  
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create
#undef FCNAME
#define FCNAME "MPIR_Comm_create"
int MPIR_Comm_create( MPID_Comm **newcomm_ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *newptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_CREATE);

    newptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    MPIU_ERR_CHKANDJUMP(!newptr, mpi_errno, MPI_ERR_OTHER, "**nomem")

    *newcomm_ptr = newptr;

    mpi_errno = MPIR_Comm_init(newptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Insert this new communicator into the list of known communicators.
       Make this conditional on debugger support to match the test in 
       MPIR_Comm_release . */
    MPIR_COMML_REMEMBER( newptr );

 fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_CREATE);

    return mpi_errno;
}

/* Create a local intra communicator from the local group of the
   specified intercomm. */
/* FIXME this is an alternative constructor that doesn't use MPIR_Comm_create! */
#undef FUNCNAME
#define FUNCNAME MPIR_Setup_intercomm_localcomm
#undef FCNAME
#define FCNAME "MPIR_Setup_intercomm_localcomm"
int MPIR_Setup_intercomm_localcomm( MPID_Comm *intercomm_ptr )
{
    MPID_Comm *localcomm_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_SETUP_INTERCOMM_LOCALCOMM);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_SETUP_INTERCOMM_LOCALCOMM);

    localcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    MPIU_ERR_CHKANDJUMP(!localcomm_ptr,mpi_errno,MPI_ERR_OTHER,"**nomem");

    /* get sensible default values for most fields (usually zeros) */
    mpi_errno = MPIR_Comm_init(localcomm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* use the parent intercomm's recv ctx as the basis for our ctx */
    localcomm_ptr->recvcontext_id = MPID_CONTEXT_SET_FIELD(IS_LOCALCOMM, intercomm_ptr->recvcontext_id, 1);
    localcomm_ptr->context_id = localcomm_ptr->recvcontext_id;

    MPIU_DBG_MSG_FMT(COMM,TYPICAL,(MPIU_DBG_FDEST, "setup_intercomm_localcomm ic=%p ic->context_id=%d ic->recvcontext_id=%d lc->recvcontext_id=%d", intercomm_ptr, intercomm_ptr->context_id, intercomm_ptr->recvcontext_id, localcomm_ptr->recvcontext_id));

    /* Duplicate the VCRT references */
    MPID_VCRT_Add_ref( intercomm_ptr->local_vcrt );
    localcomm_ptr->vcrt = intercomm_ptr->local_vcrt;
    localcomm_ptr->vcr  = intercomm_ptr->local_vcr;

    /* Save the kind of the communicator */
    localcomm_ptr->comm_kind   = MPID_INTRACOMM;

    /* Set the sizes and ranks */
    localcomm_ptr->remote_size = intercomm_ptr->local_size;
    localcomm_ptr->local_size  = intercomm_ptr->local_size;
    localcomm_ptr->rank        = intercomm_ptr->rank;

    /* TODO More advanced version: if the group is available, dup it by 
       increasing the reference count instead of recreating it later */
    /* FIXME  : No coll_fns functions for the collectives */
    /* FIXME  : No local functions for the topology routines */

    intercomm_ptr->local_comm = localcomm_ptr;

    /* sets up the SMP-aware sub-communicators and tables */
    mpi_errno = MPIR_Comm_commit(localcomm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_SETUP_INTERCOMM_LOCALCOMM);

    return mpi_errno;
}

/* Provides a hook for the top level functions to perform some manipulation on a
   communicator just before it is given to the application level.
  
   For example, we create sub-communicators for SMP-aware collectives at this
   step. */
int MPIR_Comm_commit(MPID_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int num_local = -1, num_external = -1;
    int local_rank = -1, external_rank = -1;
    int *local_procs = NULL, *external_procs = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_COMMIT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_COMMIT);

    /* It's OK to relax these assertions, but we should do so very
       intentionally.  For now this function is the only place that we create
       our hierarchy of communicators */
    MPIU_Assert(comm->node_comm == NULL);
    MPIU_Assert(comm->node_roots_comm == NULL);

    if (comm->comm_kind == MPID_INTRACOMM) {

        mpi_errno = MPIU_Find_local_and_external(comm,
                                                 &num_local,    &local_rank,    &local_procs,
                                                 &num_external, &external_rank, &external_procs,
                                                 &comm->intranode_table, &comm->internode_table);
        if (mpi_errno) {
            if (MPIR_Err_is_fatal(mpi_errno)) MPIU_ERR_POP(mpi_errno);

            /* Non-fatal errors simply mean that this communicator will not have
               any node awareness.  Node-aware collectives are an optimization. */
            MPIU_DBG_MSG_P(COMM,VERBOSE,"MPIU_Find_local_and_external failed for comm_ptr=%p", comm);
            if (comm->intranode_table)
                MPIU_Free(comm->intranode_table);
            if (comm->internode_table)
                MPIU_Free(comm->internode_table);

            mpi_errno = MPI_SUCCESS;
            goto fn_exit;
        }

        /* defensive checks */
        MPIU_Assert(num_local > 0);
        MPIU_Assert(num_local > 1 || external_rank >= 0);
        MPIU_Assert(external_rank < 0 || external_procs != NULL);

        /* if the node_roots_comm and comm would be the same size, then creating
           the second communicator is useless and wasteful. */
        if (num_external == comm->remote_size) {
            MPIU_Assert(num_local == 1);
            goto fn_exit;
        }

        /* we don't need a local comm if this process is the only one on this node */
        if (num_local > 1) {
            mpi_errno = MPIR_Comm_create(&comm->node_comm);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            comm->node_comm->context_id = comm->context_id + MPID_CONTEXT_INTRANODE_OFFSET;
            comm->node_comm->recvcontext_id = comm->node_comm->context_id;
            comm->node_comm->rank = local_rank;
            comm->node_comm->comm_kind = MPID_INTRACOMM;
            comm->node_comm->hierarchy_kind = MPID_HIERARCHY_NODE;
            comm->node_comm->local_comm = NULL;

            comm->node_comm->local_size  = num_local;
            comm->node_comm->remote_size = num_local;

            MPID_VCRT_Create( num_local, &comm->node_comm->vcrt );
            MPID_VCRT_Get_ptr( comm->node_comm->vcrt, &comm->node_comm->vcr );
            for (i = 0; i < num_local; ++i) {
                /* For rank i in the new communicator, find the corresponding
                   rank in the input communicator */
                MPID_VCR_Dup( comm->vcr[local_procs[i]], 
                              &comm->node_comm->vcr[i] );
            }

            MPID_Dev_comm_create_hook( comm->node_comm );
            /* don't call MPIR_Comm_commit here */
        }


        /* this process may not be a member of the node_roots_comm */
        if (local_rank == 0) {
            mpi_errno = MPIR_Comm_create(&comm->node_roots_comm);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            comm->node_roots_comm->context_id = comm->context_id + MPID_CONTEXT_INTERNODE_OFFSET;
            comm->node_roots_comm->recvcontext_id = comm->node_roots_comm->context_id;
            comm->node_roots_comm->rank = external_rank;
            comm->node_roots_comm->comm_kind = MPID_INTRACOMM;
            comm->node_roots_comm->hierarchy_kind = MPID_HIERARCHY_NODE_ROOTS;
            comm->node_roots_comm->local_comm = NULL;

            comm->node_roots_comm->local_size  = num_external;
            comm->node_roots_comm->remote_size = num_external;

            MPID_VCRT_Create( num_external, &comm->node_roots_comm->vcrt );
            MPID_VCRT_Get_ptr( comm->node_roots_comm->vcrt, &comm->node_roots_comm->vcr );
            for (i = 0; i < num_external; ++i) {
                /* For rank i in the new communicator, find the corresponding
                   rank in the input communicator */
                MPID_VCR_Dup( comm->vcr[external_procs[i]], 
                              &comm->node_roots_comm->vcr[i] );
            }

            MPID_Dev_comm_create_hook( comm->node_roots_comm );
            /* don't call MPIR_Comm_commit here */
        }

        comm->hierarchy_kind = MPID_HIERARCHY_PARENT;
    }

fn_exit:
    if (external_procs != NULL)
        MPIU_Free(external_procs);
    if (local_procs != NULL)
        MPIU_Free(local_procs);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_COMMIT);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Returns true if the given communicator is aware of node topology information,
   false otherwise.  Such information could be used to implement more efficient
   collective communication, for example. */
int MPIR_Comm_is_node_aware(MPID_Comm * comm)
{
    return (comm->hierarchy_kind == MPID_HIERARCHY_PARENT);
}

/* Returns true if the communicator is node-aware and processes in all the nodes
   are consecutive. For example, if node 0 contains "0, 1, 2, 3", node 1
   contains "4, 5, 6", and node 2 contains "7", we shall return true. */
int MPIR_Comm_is_node_consecutive(MPID_Comm * comm)
{
    int i = 0, curr_nodeidx = 0;
    int *internode_table = comm->internode_table;

    if (!MPIR_Comm_is_node_aware(comm))
        return 0;

    for (; i < comm->local_size; i++)
    {
        if (internode_table[i] == curr_nodeidx + 1)
            curr_nodeidx++;
        else if (internode_table[i] != curr_nodeidx)
            return 0;
    }

    return 1;
}

/*
 * Here are the routines to find a new context id.  The algorithm is discussed 
 * in detail in the mpich2 coding document.  There are versions for
 * single threaded and multithreaded MPI.
 *
 * Both the threaded and non-threaded routines use the same mask of
 * available context id values.
 */
static uint32_t context_mask[MPIR_MAX_CONTEXT_MASK];
static int initialize_context_mask = 1;

#ifdef USE_DBG_LOGGING
/* Create a string that contains the context mask.  This is
   used only with the logging interface, and must be used by one thread at 
   a time (should this be enforced by the logging interface?).
   Converts the mask to hex and returns a pointer to that string */
static char *MPIR_ContextMaskToStr( void )
{
    static char bufstr[MPIR_MAX_CONTEXT_MASK*8+1];
    int i;
    int maxset=0;

    for (maxset=MPIR_MAX_CONTEXT_MASK-1; maxset>=0; maxset--) {
	if (context_mask[maxset] != 0) break;
    }

    for (i=0; i<maxset; i++) {
	MPIU_Snprintf( &bufstr[i*8], 9, "%.8x", context_mask[i] );
    }
    return bufstr;
}
#endif

#ifdef MPICH_DEBUG_HANDLEALLOC
static int MPIU_CheckContextIDsOnFinalize(void *context_mask_ptr)
{
    int i;
    uint32_t *mask = context_mask_ptr;
    /* the predefined communicators should be freed by this point, so we don't
     * need to special case bits 0,1, and 2 */
    for (i = 0; i < MPIR_MAX_CONTEXT_MASK; ++i) {
        if (~mask[i]) {
            /* some bits were still cleared */
            printf("leaked context IDs detected: mask=%p mask[%d]=%#x\n", mask, i, (int)mask[i]);
        }
    }
    return MPI_SUCCESS;
}
#endif

static void MPIR_Init_contextid(void)
{
    int i;

    for (i=1; i<MPIR_MAX_CONTEXT_MASK; i++) {
	context_mask[i] = 0xFFFFFFFF;
    }
    /* the first three values are already used (comm_world, comm_self,
       and the internal-only copy of comm_world) */
    context_mask[0] = 0xFFFFFFF8; 
    initialize_context_mask = 0;

#ifdef MPICH_DEBUG_HANDLEALLOC
    /* check for context ID leaks in MPI_Finalize.  Use (_PRIO-1) to make sure
     * that we run after MPID_Finalize. */
    MPIR_Add_finalize(MPIU_CheckContextIDsOnFinalize, context_mask,
                      MPIR_FINALIZE_CALLBACK_PRIO - 1);
#endif
}

/* Return the context id corresponding to the first set bit in the mask.
   Return 0 if no bit found.  This function does _not_ alter local_mask. */
static int MPIR_Locate_context_bit(uint32_t local_mask[])
{
    int i, j, context_id = 0;
    for (i=0; i<MPIR_MAX_CONTEXT_MASK; i++) {
	if (local_mask[i]) {
	    /* There is a bit set in this word. */
	    register uint32_t     val, nval;
	    /* The following code finds the highest set bit by recursively
	       checking the top half of a subword for a bit, and incrementing
	       the bit location by the number of bit of the lower sub word if 
	       the high subword contains a set bit.  The assumption is that
	       full-word bitwise operations and compares against zero are 
	       fast */
	    val = local_mask[i];
	    j   = 0;
	    nval = val & 0xFFFF0000;
	    if (nval) {
		j += 16;
		val = nval;
	    }
	    nval = val & 0xFF00FF00;
	    if (nval) {
		j += 8;
		val = nval;
	    }
	    nval = val & 0xF0F0F0F0;
	    if (nval) {
		j += 4;
		val = nval;
	    }
	    nval = val & 0xCCCCCCCC;
	    if (nval) {
		j += 2;
		val = nval;
	    }
	    if (val & 0xAAAAAAAA) {
		j += 1;
	    }
	    context_id = (MPIR_CONTEXT_INT_BITS * i + j) << MPID_CONTEXT_PREFIX_SHIFT;
	    return context_id;
	}
    }
    return 0;
}

/* Allocates a context ID from the given mask by clearing the bit
 * corresponding to the the given id.  Returns 0 on failure, id on
 * success. */
static int MPIR_Allocate_context_bit(uint32_t mask[], MPIR_Context_id_t id)
{
    int raw_prefix, idx, bitpos;
    raw_prefix = MPID_CONTEXT_READ_FIELD(PREFIX,id);
    idx    = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;

    /* the bit should not already be cleared (allocated) */
    MPIU_Assert(mask[idx] & (1<<bitpos));

    /* clear the bit */
    mask[idx] &= ~(1<<bitpos);

    MPIU_DBG_MSG_FMT(COMM,VERBOSE,(MPIU_DBG_FDEST,
            "allocating contextid = %d, (mask=%p, mask[%d], bit %d)",
            id, mask, idx, bitpos));
    return id;
}

/* Allocates the first available context ID from context_mask based on the available
 * bits given in local_mask.  This function will clear the corresponding bit in
 * context_mask if allocation was successful.
 *
 * Returns 0 on failure.  Returns the allocated context ID on success. */
static int MPIR_Find_and_allocate_context_id(uint32_t local_mask[])
{
    MPIR_Context_id_t context_id;
    context_id = MPIR_Locate_context_bit(local_mask);
    if (context_id != 0) {
        context_id = MPIR_Allocate_context_bit(context_mask, context_id);
    }
    return context_id;
}

/* Older, simpler interface.  Allocates a context ID collectively over the given
 * communicator. */
#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Get_contextid(MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Get_contextid_sparse(comm_ptr, context_id, FALSE);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(*context_id != MPIR_INVALID_CONTEXT_ID);
fn_fail:
    return mpi_errno;
}


#ifndef MPICH_IS_THREADED
/* Unthreaded (only one MPI call active at any time) */

#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid_sparse
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Get_contextid_sparse(MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id, int ignore_id)
{
    int mpi_errno = MPI_SUCCESS;
    uint32_t     local_mask[MPIR_MAX_CONTEXT_MASK];
    int errflag = FALSE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_CONTEXTID);

    *context_id = 0;

    if (initialize_context_mask) {
	MPIR_Init_contextid();
    }

    if (ignore_id) {
        /* We are not participating in the resulting communicator, so our
         * context ID space doesn't matter.  Set the mask to "all available". */
        memset(local_mask, 0xff, MPIR_MAX_CONTEXT_MASK * sizeof(int));
    }
    else {
        MPIU_Memcpy( local_mask, context_mask, MPIR_MAX_CONTEXT_MASK * sizeof(int) );
    }

    /* Note that this is the unthreaded version */
    /* Comm must be an intracommunicator */
    mpi_errno = MPIR_Allreduce_impl( MPI_IN_PLACE, local_mask, MPIR_MAX_CONTEXT_MASK, 
                                     MPI_INT, MPI_BAND, comm_ptr, &errflag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    if (ignore_id) {
        *context_id = MPIR_Locate_context_bit(local_mask);
        MPIU_ERR_CHKANDJUMP(!(*context_id), mpi_errno, MPIR_ERR_RECOVERABLE, "**toomanycomm");
    }
    else {
        *context_id = MPIR_Find_and_allocate_context_id(local_mask);
        MPIU_ERR_CHKANDJUMP(!(*context_id), mpi_errno, MPIR_ERR_RECOVERABLE, "**toomanycomm");
    }

fn_exit:
    if (ignore_id)
        *context_id = MPIR_INVALID_CONTEXT_ID;
    MPIU_DBG_MSG_S(COMM,VERBOSE,"Context mask = %s",MPIR_ContextMaskToStr());

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_CONTEXTID);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#else /* MPICH_IS_THREADED is set and true */

/* Additional values needed to maintain thread safety */
static volatile int mask_in_use = 0;
/* lowestContextId is used to break ties when multiple threads
   are contending for the mask */
#define MPIR_MAXID (1 << 30)
static volatile int lowestContextId = MPIR_MAXID;

/* Allocates a new context ID collectively over the given communicator.  This
 * routine is "sparse" in the sense that while it is collective, some processes
 * may not care about the value selected context ID.
 *
 * One example of this case is processes who pass MPI_UNDEFINED as the color
 * value to MPI_Comm_split.  Such processes should pass ignore_id==TRUE to
 * obtain the best performance and utilization of the context ID space.
 *
 * Processes that pass ignore_id==TRUE will receive
 * (*context_id==MPIR_INVALID_CONTEXT_ID) and should not attempt to use it.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid_sparse
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Get_contextid_sparse(MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id, int ignore_id)
{
    int          mpi_errno = MPI_SUCCESS;
    uint32_t     local_mask[MPIR_MAX_CONTEXT_MASK];
    int          own_mask = 0;
    int          testCount = 10; /* if you change this value, you need to also change 
				    it below where it is reinitialized */
    int errflag = FALSE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_CONTEXTID);

    *context_id = 0;

    /* We lock only around access to the mask.  If another thread is
       using the mask, we take a mask of zero */
    MPIU_DBG_MSG_FMT(COMM, VERBOSE, (MPIU_DBG_FDEST,
         "Entering; shared state is %d:%d, my ctx id is %d",
         mask_in_use, lowestContextId, comm_ptr->context_id));
    /* We need a special test in this loop for the case where some process
     has exhausted its supply of context ids.  In the single threaded case, 
     this is simple, because the algorithm is deterministic (see above).  In 
     the multithreaded case, it is more complicated, because we may get a
     zero for the context mask because some other thread holds the mask.  
     In addition, we can't check for the case where this process did not
     select MPI_THREAD_MULTIPLE, because one of the other processes
     may have selected MPI_THREAD_MULTIPLE.  To handle this case, after a 
     fixed number of failures, we test to see if some process has exhausted 
     its supply of context ids.  If so, all processes can invoke the 
     out-of-context-id error.  That fixed number of tests is in testCount */
    while (*context_id == 0) {
	/* In all but the global-critical-section case, we must ensure that
	   only one thread access the context id mask at a time */
	MPIU_THREAD_CS_ENTER(CONTEXTID,);
	if (initialize_context_mask) {
	    MPIR_Init_contextid();
	}
        if (ignore_id) {
            /* We are not participating in the resulting communicator, so our
             * context ID space doesn't matter.  Set the mask to "all available". */
            memset(local_mask, 0xff, MPIR_MAX_CONTEXT_MASK * sizeof(int));
            own_mask = 0;
            /* don't need to touch mask_in_use/lowestContextId b/c our thread
             * doesn't ever need to "win" the mask */
        }
        else {
            if (mask_in_use || comm_ptr->context_id > lowestContextId) {
                memset( local_mask, 0, MPIR_MAX_CONTEXT_MASK * sizeof(int) );
                own_mask        = 0;
                if (comm_ptr->context_id < lowestContextId) {
                    lowestContextId = comm_ptr->context_id;
                }
                MPIU_DBG_MSG_D( COMM, VERBOSE, 
                   "In in-use, set lowestContextId to %d", lowestContextId );
            }
            else {
                MPIU_Memcpy( local_mask, context_mask, MPIR_MAX_CONTEXT_MASK * sizeof(int) );
                mask_in_use     = 1;
                own_mask        = 1;
                lowestContextId = comm_ptr->context_id;
                MPIU_DBG_MSG( COMM, VERBOSE, "Copied local_mask" );
            }
        }
	MPIU_THREAD_CS_EXIT(CONTEXTID,);
	
	/* Now, try to get a context id */
        MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);
	/* In the global and brief-global cases, note that this routine will
	   release that global lock when it needs to wait.  That will allow 
	   other processes to enter the global or brief global critical section.
	 */ 
	mpi_errno = MPIR_Allreduce_impl( MPI_IN_PLACE, local_mask, MPIR_MAX_CONTEXT_MASK,
                                         MPI_INT, MPI_BAND, comm_ptr, &errflag );
	if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        /* MT FIXME 2/3 cases don't seem to need the CONTEXTID CS, check and
         * narrow this region */
        MPIU_THREAD_CS_ENTER(CONTEXTID,);
        if (ignore_id) {
            /* we don't care what the value was, but make sure that everyone
             * who did care agreed on a value */
            *context_id = MPIR_Locate_context_bit(local_mask);
            /* used later in out-of-context ids check and outer while loop condition */
        }
        else if (own_mask) {
	    /* There is a chance that we've found a context id */
	    /* Find_and_allocate_context_id updates the context_mask if it finds a match */
	    *context_id = MPIR_Find_and_allocate_context_id(local_mask);
	    MPIU_DBG_MSG_D( COMM, VERBOSE, 
			    "Context id is now %hd", *context_id );
	    if (*context_id > 0) {
		/* If we were the lowest context id, reset the value to
		   allow the other threads to compete for the mask */
		if (lowestContextId == comm_ptr->context_id) {
		    lowestContextId = MPIR_MAXID;
		    /* Else leave it alone; there is another thread waiting */
		}
	    }
	    else {
		/* else we did not find a context id. Give up the mask in case
                   there is another thread (with a lower input context id)
                   waiting for it.

		   We need to ensure that any other threads have the 
		   opportunity to run.  We do this by releasing the single
		   mutex, yielding, and then reaquiring the mutex.
		   We might want to do something more sophisticated, such
		   as using a condition variable (if we know for sure that
		   there is another thread on this process that is waiting).
		*/
		MPIU_THREAD_CS_YIELD(CONTEXTID,);
	    }
	    mask_in_use = 0;
	}
	else {
	    /* As above, force this thread to yield */
	    MPIU_THREAD_CS_YIELD(CONTEXTID,);
	}
        MPIU_THREAD_CS_EXIT(CONTEXTID,);

	/* Here is the test for out-of-context ids */
        /* FIXME we may be able to "rotate" this a half iteration up to the top
         * where we already have to grab the lock */
	if ((testCount-- == 0) && (*context_id == 0)) {
	    int hasNoId, totalHasNoId;
            MPIU_THREAD_CS_ENTER(CONTEXTID,);
	    hasNoId = MPIR_Locate_context_bit(context_mask) == 0;
            MPIU_THREAD_CS_EXIT(CONTEXTID,);

            /* we _must_ release the lock above in order to avoid deadlocking on
             * this blocking allreduce operation */
	    mpi_errno = MPIR_Allreduce_impl( &hasNoId, &totalHasNoId, 1, MPI_INT,
                                             MPI_MAX, comm_ptr, &errflag );
	    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
	    if (totalHasNoId == 1) {
		/* Release the mask for use by other threads */
		if (own_mask) {
                    MPIU_THREAD_CS_ENTER(CONTEXTID,);
		    mask_in_use = 0;
                    MPIU_THREAD_CS_EXIT(CONTEXTID,);
		}
		MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**toomanycomm");
	    }
	    else { /* reinitialize testCount */
		testCount = 10;
                MPIU_DBG_MSG_D(COMM, VERBOSE, "reinitialized testCount to %d", testCount);
	    }
	}
    }

fn_exit:
    if (ignore_id)
        *context_id = MPIR_INVALID_CONTEXT_ID;
    MPIU_DBG_MSG_S(COMM,VERBOSE,"Context mask = %s",MPIR_ContextMaskToStr());
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_CONTEXTID);
    return mpi_errno;
fn_fail:
    /* Release the masks */
    if (own_mask) {
        mask_in_use = 0;
    }
    goto fn_exit;
}
#endif

/* Get a context for a new intercomm.  There are two approaches 
   here (for MPI-1 codes only)
   (a) Each local group gets a context; the groups exchange, and
       the low value is accepted and the high one returned.  This
       works because the context ids are taken from the same pool.
   (b) Form a temporary intracomm over all processes and use that
       with the regular algorithm.
   
   In some ways, (a) is the better approach because it is the one that
   extends to MPI-2 (where the last step, returning the context, is 
   not used and instead separate send and receive context id value 
   are kept).  For this reason, we'll use (a).

   Even better is to separate the local and remote context ids.  Then
   each group of processes can manage their context ids separately.
*/
/* 
 * This uses the thread-safe (if necessary) routine to get a context id
 * and does not need its own thread-safe version.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Get_intercomm_contextid
#undef FCNAME
#define FCNAME "MPIR_Get_intercomm_contextid"
int MPIR_Get_intercomm_contextid( MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id, 
				  MPIR_Context_id_t *recvcontext_id )
{
    MPIR_Context_id_t mycontext_id, remote_context_id;
    int mpi_errno = MPI_SUCCESS;
    int tag = 31567; /* FIXME  - we need an internal tag or 
		        communication channel.  Can we use a different
		        context instead?.  Or can we use the tag 
		        provided in the intercomm routine? (not on a dup, 
			but in that case it can use the collective context) */
    int errflag = FALSE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);

    if (!comm_ptr->local_comm) {
        /* Manufacture the local communicator */
        mpi_errno = MPIR_Setup_intercomm_localcomm( comm_ptr );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIR_Get_contextid( comm_ptr->local_comm, &mycontext_id );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(mycontext_id != 0);

    /* MPIC routine uses an internal context id.  The local leads (process 0)
       exchange data */
    remote_context_id = -1;
    if (comm_ptr->rank == 0) {
        mpi_errno = MPIC_Sendrecv( &mycontext_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, tag,
                                   &remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, tag, 
                                   comm_ptr->handle, MPI_STATUS_IGNORE );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* Make sure that all of the local processes now have this
       id */
    mpi_errno = MPIR_Bcast_impl( &remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 
                                 0, comm_ptr->local_comm, &errflag );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    /* The recvcontext_id must be the one that was allocated out of the local
     * group, not the remote group.  Otherwise we could end up posting two
     * MPI_ANY_SOURCE,MPI_ANY_TAG recvs on the same context IDs even though we
     * are attempting to post them for two separate communicators. */
    *context_id     = remote_context_id;
    *recvcontext_id = mycontext_id;
 fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Free_contextid
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void MPIR_Free_contextid( MPIR_Context_id_t context_id )
{
    int idx, bitpos, raw_prefix;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_FREE_CONTEXTID);
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_FREE_CONTEXTID);

    /* Convert the context id to the bit position */
    raw_prefix = MPID_CONTEXT_READ_FIELD(PREFIX,context_id);
    idx    = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;

    /* --BEGIN ERROR HANDLING-- */
    if (idx < 0 || idx >= MPIR_MAX_CONTEXT_MASK) {
	MPID_Abort( 0, MPI_ERR_INTERN, 1, 
		    "In MPIR_Free_contextid, idx is out of range" );
    }

    /* The low order bits for dynamic context IDs don't have meaning the
     * same way that low bits of non-dynamic ctx IDs do.  So we have to
     * check the dynamic case first. */
    if (MPID_CONTEXT_READ_FIELD(DYNAMIC_PROC, context_id)) {
        MPIU_DBG_MSG_D(COMM,VERBOSE,"skipping dynamic process ctx id, context_id=%d", context_id);
        goto fn_exit;
    }
    else { /* non-dynamic context ID */
        /* In terms of the context ID bit vector, intercomms and their constituent
         * localcomms have the same value.  To avoid a double-free situation we just
         * don't free the context ID for localcomms and assume it will be cleaned up
         * when the parent intercomm is itself completely freed. */
        if (MPID_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id)) {
#ifdef USE_DBG_LOGGING
            char dump_str[1024];
            MPIR_Comm_dump_context_id(context_id, dump_str, sizeof(dump_str));
            MPIU_DBG_MSG_S(COMM,VERBOSE,"skipping localcomm id: %s", dump_str);
#endif
            goto fn_exit;
        }
        else if (MPID_CONTEXT_READ_FIELD(SUBCOMM, context_id)) {
            MPIU_DBG_MSG_D(COMM,VERBOSE,"skipping non-parent communicator ctx id, context_id=%d", context_id);
            goto fn_exit;
        }
    }

    /* Check that this context id has been allocated */
    if ( (context_mask[idx] & (0x1 << bitpos)) != 0 ) {
#ifdef USE_DBG_LOGGING
        char dump_str[1024];
        MPIR_Comm_dump_context_id(context_id, dump_str, sizeof(dump_str));
        MPIU_DBG_MSG_S(COMM,VERBOSE,"context dump: %s", dump_str);
        MPIU_DBG_MSG_S(COMM,VERBOSE,"context mask = %s",MPIR_ContextMaskToStr());
#endif
	MPID_Abort( 0, MPI_ERR_INTERN, 1, 
		    "In MPIR_Free_contextid, the context id is not in use" );
    }
    /* --END ERROR HANDLING-- */

    MPIU_THREAD_CS_ENTER(CONTEXTID,);
    /* MT: Note that this update must be done atomically in the multithreaedd
       case.  In the "one, single lock" implementation, that lock is indeed
       held when this operation is called. */
    context_mask[idx] |= (0x1 << bitpos);
    MPIU_THREAD_CS_EXIT(CONTEXTID,);

    MPIU_DBG_MSG_FMT(COMM,VERBOSE,
                     (MPIU_DBG_FDEST,
                      "Freed context %d, mask[%d] bit %d (prefix=%#x)",
                      context_id, idx, bitpos, raw_prefix));
fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_FREE_CONTEXTID);
}

/*
 * Copy a communicator, including creating a new context and copying the
 * virtual connection tables and clearing the various fields.
 * Does *not* copy attributes.  If size is < the size of the local group
 * in the input communicator, copy only the first size elements.
 * If this process is not a member, return a null pointer in outcomm_ptr.
 * This is only supported in the case where the communicator is in 
 * Intracomm (not an Intercomm).  Note that this is all that is required
 * for cart_create and graph_create.
 *
 * Used by cart_create, graph_create, and dup_create 
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_copy
#undef FCNAME
#define FCNAME "MPIR_Comm_copy"
int MPIR_Comm_copy( MPID_Comm *comm_ptr, int size, MPID_Comm **outcomm_ptr )
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Context_id_t new_context_id, new_recvcontext_id;
    MPID_Comm *newcomm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_COPY);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_COPY);

    /* Get a new context first.  We need this to be collective over the
       input communicator */
    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
       use the appropriate algorithm to get a new context id.  Be careful
       of intercomms here */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	mpi_errno = 
	    MPIR_Get_intercomm_contextid( 
		 comm_ptr, &new_context_id, &new_recvcontext_id );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
	mpi_errno = MPIR_Get_contextid( comm_ptr, &new_context_id );
	new_recvcontext_id = new_context_id;
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_Assert(new_context_id != 0);
    }
    /* --BEGIN ERROR HANDLING-- */
    if (new_context_id == 0) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**toomanycomm" );
    }
    /* --END ERROR HANDLING-- */

    /* This is the local size, not the remote size, in the case of
       an intercomm */
    if (comm_ptr->rank >= size) {
        *outcomm_ptr = 0;
        /* always free the recvcontext ID, never the "send" ID */
        MPIR_Free_contextid(new_recvcontext_id);
	goto fn_exit;
    }

    /* We're left with the processes that will have a non-null communicator.
       Create the object, initialize the data, and return the result */

    mpi_errno = MPIR_Comm_create( &newcomm_ptr );
    if (mpi_errno) goto fn_fail;

    newcomm_ptr->context_id     = new_context_id;
    newcomm_ptr->recvcontext_id = new_recvcontext_id;

    /* Save the kind of the communicator */
    newcomm_ptr->comm_kind   = comm_ptr->comm_kind;
    newcomm_ptr->local_comm  = 0;

    /* There are two cases here - size is the same as the old communicator,
       or it is smaller.  If the size is the same, we can just add a reference.
       Otherwise, we need to create a new VCRT.  Note that this is the
       test that matches the test on rank above. */
    if (size == comm_ptr->local_size) {
	/* Duplicate the VCRT references */
	MPID_VCRT_Add_ref( comm_ptr->vcrt );
	newcomm_ptr->vcrt = comm_ptr->vcrt;
	newcomm_ptr->vcr  = comm_ptr->vcr;
    }
    else {
	int i;
	/* The "remote" vcr gets the shortened vcrt */
	MPID_VCRT_Create( size, &newcomm_ptr->vcrt );
	MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, 
			   &newcomm_ptr->vcr );
	for (i=0; i<size; i++) {
	    /* For rank i in the new communicator, find the corresponding
	       rank in the input communicator */
	    MPID_VCR_Dup( comm_ptr->vcr[i], &newcomm_ptr->vcr[i] );
	}
    }

    /* If it is an intercomm, duplicate the local vcrt references */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	MPID_VCRT_Add_ref( comm_ptr->local_vcrt );
	newcomm_ptr->local_vcrt = comm_ptr->local_vcrt;
	newcomm_ptr->local_vcr  = comm_ptr->local_vcr;
    }

    /* Set the sizes and ranks */
    newcomm_ptr->rank        = comm_ptr->rank;
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	newcomm_ptr->local_size   = comm_ptr->local_size;
	newcomm_ptr->remote_size  = comm_ptr->remote_size;
	newcomm_ptr->is_low_group = comm_ptr->is_low_group;
    }
    else {
	newcomm_ptr->local_size  = size;
	newcomm_ptr->remote_size = size;
    }

    /* Inherit the error handler (if any) */
    MPIU_THREAD_CS_ENTER(MPI_OBJ, comm_ptr);
    newcomm_ptr->errhandler = comm_ptr->errhandler;
    if (comm_ptr->errhandler) {
	MPIR_Errhandler_add_ref( comm_ptr->errhandler );
    }
    MPIU_THREAD_CS_EXIT(MPI_OBJ, comm_ptr);

    /* Notify the device of the new communicator */
    MPID_Dev_comm_create_hook(newcomm_ptr);
    mpi_errno = MPIR_Comm_commit(newcomm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Start with no attributes on this communicator */
    newcomm_ptr->attributes = 0;
    *outcomm_ptr = newcomm_ptr;

 fn_fail:
 fn_exit:

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_COPY);

    return mpi_errno;
}

/* Common body between MPIR_Comm_release and MPIR_comm_release_always.  This
 * helper function frees the actual MPID_Comm structure and any associated
 * storage.  It also releases any refernces to other objects, such as the VCRT.
 * This function should only be called when the communicator's reference count
 * has dropped to 0. */
#undef FUNCNAME
#define FUNCNAME comm_delete
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int comm_delete(MPID_Comm * comm_ptr, int isDisconnect)
{
    int in_use;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_COMM_DELETE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_COMM_DELETE);

    MPIU_Assert(MPIU_Object_get_ref(comm_ptr) == 0); /* sanity check */

    /* Remove the attributes, executing the attribute delete routine.
       Do this only if the attribute functions are defined.
       This must be done first, because if freeing the attributes
       returns an error, the communicator is not freed */
    if (MPIR_Process.attr_free && comm_ptr->attributes) {
        /* Temporarily add a reference to this communicator because
           the attr_free code requires a valid communicator */
        MPIU_Object_add_ref( comm_ptr );
        mpi_errno = MPIR_Process.attr_free( comm_ptr->handle,
                                            &comm_ptr->attributes );
        /* Release the temporary reference added before the call to
           attr_free */
        MPIU_Object_release_ref( comm_ptr, &in_use);
    }

    /* If the attribute delete functions return failure, the
       communicator must not be freed.  That is the reason for the
       test on mpi_errno here. */
    if (mpi_errno == MPI_SUCCESS) {
        /* If this communicator is our parent, and we're disconnecting
           from the parent, mark that fact */
        if (MPIR_Process.comm_parent == comm_ptr)
            MPIR_Process.comm_parent = NULL;

        /* Notify the device that the communicator is about to be
           destroyed */
        MPID_Dev_comm_destroy_hook(comm_ptr);

        /* Free the VCRT */
        mpi_errno = MPID_VCRT_Release(comm_ptr->vcrt, isDisconnect);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_POP(mpi_errno);
        }
        if (comm_ptr->comm_kind == MPID_INTERCOMM) {
            mpi_errno = MPID_VCRT_Release(
                                          comm_ptr->local_vcrt, isDisconnect);
            if (mpi_errno != MPI_SUCCESS) {
                MPIU_ERR_POP(mpi_errno);
            }
            if (comm_ptr->local_comm)
                MPIR_Comm_release(comm_ptr->local_comm, isDisconnect );
        }

        /* Free the local and remote groups, if they exist */
        if (comm_ptr->local_group)
            MPIR_Group_release(comm_ptr->local_group);
        if (comm_ptr->remote_group)
            MPIR_Group_release(comm_ptr->remote_group);

        /* free the intra/inter-node communicators, if they exist */
        if (comm_ptr->node_comm)
            MPIR_Comm_release(comm_ptr->node_comm, isDisconnect);
        if (comm_ptr->node_roots_comm)
            MPIR_Comm_release(comm_ptr->node_roots_comm, isDisconnect);
        if (comm_ptr->intranode_table != NULL)
            MPIU_Free(comm_ptr->intranode_table);
        if (comm_ptr->internode_table != NULL)
            MPIU_Free(comm_ptr->internode_table);

        /* Free the context value.  This should come after freeing the
         * intra/inter-node communicators since those free calls won't
         * release this context ID and releasing this before then could lead
         * to races once we make threading finer grained. */
        /* This must be the recvcontext_id (i.e. not the (send)context_id)
         * because in the case of intercommunicators the send context ID is
         * allocated out of the remote group's bit vector, not ours. */
        MPIR_Free_contextid( comm_ptr->recvcontext_id );

        /* We need to release the error handler */
        /* no MPI_OBJ CS needed */
        if (comm_ptr->errhandler &&
            ! (HANDLE_GET_KIND(comm_ptr->errhandler->handle) ==
               HANDLE_KIND_BUILTIN) ) {
            int errhInuse;
            MPIR_Errhandler_release_ref( comm_ptr->errhandler,&errhInuse);
            if (!errhInuse) {
                MPIU_Handle_obj_free( &MPID_Errhandler_mem,
                                      comm_ptr->errhandler );
            }
        }

        /* Remove from the list of active communicators if
           we are supporting message-queue debugging.  We make this
           conditional on having debugger support since the
           operation is not constant-time */
        MPIR_COMML_FORGET( comm_ptr );

        /* Check for predefined communicators - these should not
           be freed */
        if (! (HANDLE_GET_KIND(comm_ptr->handle) == HANDLE_KIND_BUILTIN) )
            MPIU_Handle_obj_free( &MPID_Comm_mem, comm_ptr );
    }
    else {
        /* If the user attribute free function returns an error,
           then do not free the communicator */
        MPIR_Comm_add_ref( comm_ptr );
    }

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_COMM_DELETE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Release a reference to a communicator.  If there are no pending
   references, delete the communicator and recover all storage and 
   context ids */
#undef FUNCNAME 
#define FUNCNAME MPIR_Comm_release
#undef FCNAME
#define FCNAME "MPIR_Comm_release"
int MPIR_Comm_release(MPID_Comm * comm_ptr, int isDisconnect)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_RELEASE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_RELEASE);

    MPIR_Comm_release_ref(comm_ptr, &in_use);
    if (!in_use) {
        mpi_errno = comm_delete(comm_ptr, isDisconnect);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_RELEASE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Release a reference to a communicator.  If there are no pending
   references, delete the communicator and recover all storage and
   context ids.  This version of the function always manipulates the reference
   counts, even for predefined objects. */
#undef FUNCNAME
#define FUNCNAME MPIR_Comm_release_always
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Comm_release_always(MPID_Comm *comm_ptr, int isDisconnect)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_RELEASE_ALWAYS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_RELEASE_ALWAYS);

    /* we want to short-circuit any optimization that avoids reference counting
     * predefined communicators, such as MPI_COMM_WORLD or MPI_COMM_SELF. */
    MPIU_Object_release_ref_always(comm_ptr, &in_use);
    if (!in_use) {
        mpi_errno = comm_delete(comm_ptr, isDisconnect);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_RELEASE_ALWAYS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

