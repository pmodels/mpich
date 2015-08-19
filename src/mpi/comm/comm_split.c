/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : COMMUNICATOR
      description : cvars that control communicator construction and operation

cvars:
    - name        : MPIR_CVAR_COMM_SPLIT_USE_QSORT
      category    : COMMUNICATOR
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use qsort(3) in the implementation of MPI_Comm_split instead of bubble sort.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Comm_split */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_split = PMPI_Comm_split
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_split  MPI_Comm_split
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_split as PMPI_Comm_split
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm) __attribute__((weak,alias("PMPI_Comm_split")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_split
#define MPI_Comm_split PMPI_Comm_split

typedef struct splittype {
    int color, key;
} splittype;

/* Same as splittype but with an additional field to stabilize the qsort.  We
 * could just use one combined type, but using separate types simplifies the
 * allgather step. */
typedef struct sorttype {
    int color, key;
    int orig_idx;
} sorttype;

#if defined(HAVE_QSORT)
static int sorttype_compare(const void *v1, const void *v2) {
    const sorttype *s1 = v1;
    const sorttype *s2 = v2;

    if (s1->key > s2->key)
        return 1;
    if (s1->key < s2->key)
        return -1;

    /* (s1->key == s2->key), maintain original order */
    if (s1->orig_idx > s2->orig_idx)
        return 1;
    else if (s1->orig_idx < s2->orig_idx)
        return -1;

    /* --BEGIN ERROR HANDLING-- */
    return 0; /* should never happen */
    /* --END ERROR HANDLING-- */
}
#endif

/* Sort the entries in keytable into increasing order by key.  A stable
   sort should be used incase the key values are not unique. */
static void MPIU_Sort_inttable( sorttype *keytable, int size )
{
    sorttype tmp;
    int i, j;

#if defined(HAVE_QSORT)
    /* temporary switch for profiling performance differences */
    if (MPIR_CVAR_COMM_SPLIT_USE_QSORT)
    {
        /* qsort isn't a stable sort, so we have to enforce stability by keeping
         * track of the original indices */
        for (i = 0; i < size; ++i)
            keytable[i].orig_idx = i;
        qsort(keytable, size, sizeof(sorttype), &sorttype_compare);
    }
    else
#endif
    {
        /* --BEGIN USEREXTENSION-- */
        /* fall through to insertion sort if qsort is unavailable/disabled */
        for (i = 1; i < size; ++i) {
            tmp = keytable[i];
            j = i - 1;
            while (1) {
                if (keytable[j].key > tmp.key) {
                    keytable[j+1] = keytable[j];
                    j = j - 1;
                    if (j < 0)
                        break;
                }
                else {
                    break;
                }
            }
            keytable[j+1] = tmp;
        }
        /* --END USEREXTENSION-- */
    }
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_split_impl(MPID_Comm *comm_ptr, int color, int key, MPID_Comm **newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *local_comm_ptr;
    splittype *table, *remotetable=0;
    sorttype *keytable, *remotekeytable=0;
    int rank, size, remote_size, i, new_size, new_remote_size,
	first_entry = 0, first_remote_entry = 0, *last_ptr;
    int in_newcomm; /* TRUE iff *newcomm should be populated */
    MPIU_Context_id_t   new_context_id, remote_context_id;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Comm_map_t *mapper;
    MPIU_CHKLMEM_DECL(4);

    rank        = comm_ptr->rank;
    size        = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
	
    /* Step 1: Find out what color and keys all of the processes have */
    MPIU_CHKLMEM_MALLOC(table,splittype*,size*sizeof(splittype),mpi_errno,
			"table");
    table[rank].color = color;
    table[rank].key   = key;

    /* Get the communicator to use in collectives on the local group of 
       processes */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	if (!comm_ptr->local_comm) {
	    MPIR_Setup_intercomm_localcomm( comm_ptr );
	}
	local_comm_ptr = comm_ptr->local_comm;
    }
    else {
	local_comm_ptr = comm_ptr;
    }
    /* Gather information on the local group of processes */
    mpi_errno = MPIR_Allgather_impl( MPI_IN_PLACE, 2, MPI_INT, table, 2, MPI_INT, local_comm_ptr, &errflag );
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    /* Step 2: How many processes have our same color? */
    new_size = 0;
    if (color != MPI_UNDEFINED) {
	/* Also replace the color value with the index of the *next* value
	   in this set.  The integer first_entry is the index of the 
	   first element */
	last_ptr = &first_entry;
	for (i=0; i<size; i++) {
	    /* Replace color with the index in table of the next item
	       of the same color.  We use this to efficiently populate 
	       the keyval table */
	    if (table[i].color == color) {
		new_size++;
		*last_ptr = i;
		last_ptr  = &table[i].color;
	    }
	}
    }
    /* We don't need to set the last value to -1 because we loop through
       the list for only the known size of the group */

    /* If we're an intercomm, we need to do the same thing for the remote
       table, as we need to know the size of the remote group of the
       same color before deciding to create the communicator */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	splittype mypair;
	/* For the remote group, the situation is more complicated.
	   We need to find the size of our "partner" group in the
	   remote comm.  The easiest way (in terms of code) is for
	   every process to essentially repeat the operation for the
	   local group - perform an (intercommunicator) all gather
	   of the color and rank information for the remote group.
	*/
	MPIU_CHKLMEM_MALLOC(remotetable,splittype*,
			    remote_size*sizeof(splittype),mpi_errno,
			    "remotetable");
	/* This is an intercommunicator allgather */
	
	/* We must use a local splittype because we've already modified the
	   entries in table to indicate the location of the next rank of the
	   same color */
	mypair.color = color;
	mypair.key   = key;
	mpi_errno = MPIR_Allgather_impl( &mypair, 2, MPI_INT, remotetable, 2, MPI_INT,
                                         comm_ptr, &errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
        
	/* Each process can now match its color with the entries in the table */
	new_remote_size = 0;
	last_ptr = &first_remote_entry;
	for (i=0; i<remote_size; i++) {
	    /* Replace color with the index in table of the next item
	       of the same color.  We use this to efficiently populate 
	       the keyval table */
	    if (remotetable[i].color == color) {
		new_remote_size++;
		*last_ptr = i;
		last_ptr  = &remotetable[i].color;
	    }
	}
	/* Note that it might find that there a now processes in the remote
	   group with the same color.  In that case, COMM_SPLIT will
	   return a null communicator */
    }
    else {
	/* Set the size of the remote group to the size of our group.
	   This simplifies the test below for intercomms with an empty remote
	   group (must create comm_null) */
	new_remote_size = new_size;
    }

    in_newcomm = (color != MPI_UNDEFINED && new_remote_size > 0);

    /* Step 3: Create the communicator */
    /* Collectively create a new context id.  The same context id will
       be used by each (disjoint) collections of processes.  The
       processes whose color is MPI_UNDEFINED will not influence the
       resulting context id (by passing ignore_id==TRUE). */
    /* In the multi-threaded case, MPIR_Get_contextid_sparse assumes that the
       calling routine already holds the single criticial section */
    mpi_errno = MPIR_Get_contextid_sparse(local_comm_ptr, &new_context_id, !in_newcomm);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIU_Assert(new_context_id != 0);

    /* In the intercomm case, we need to exchange the context ids */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	if (comm_ptr->rank == 0) {
	    mpi_errno = MPIC_Sendrecv( &new_context_id, 1, MPIU_CONTEXT_ID_T_DATATYPE, 0, 0,
				       &remote_context_id, 1, MPIU_CONTEXT_ID_T_DATATYPE, 
				       0, 0, comm_ptr, MPI_STATUS_IGNORE, &errflag );
	    if (mpi_errno) { MPIR_ERR_POP( mpi_errno ); }
	    mpi_errno = MPIR_Bcast_impl( &remote_context_id, 1, MPIU_CONTEXT_ID_T_DATATYPE, 0, local_comm_ptr, &errflag );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
	}
	else {
	    /* Broadcast to the other members of the local group */
	    mpi_errno = MPIR_Bcast_impl( &remote_context_id, 1, MPIU_CONTEXT_ID_T_DATATYPE, 0, local_comm_ptr, &errflag );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
	}
    }

    *newcomm_ptr = NULL;

    /* Now, create the new communicator structure if necessary */
    if (in_newcomm) {
    
	mpi_errno = MPIR_Comm_create( newcomm_ptr );
	if (mpi_errno) goto fn_fail;

	(*newcomm_ptr)->recvcontext_id = new_context_id;
	(*newcomm_ptr)->local_size	    = new_size;
	(*newcomm_ptr)->comm_kind	    = comm_ptr->comm_kind;
	/* Other fields depend on whether this is an intercomm or intracomm */

	/* Step 4: Order the processes by their key values.  Sort the
	   list that is stored in table.  To simplify the sort, we 
	   extract the table into a smaller array and sort that.
	   Also, store in the "color" entry the rank in the input communicator
	   of the entry. */
	MPIU_CHKLMEM_MALLOC(keytable,sorttype*,new_size*sizeof(sorttype),
			    mpi_errno,"keytable");
	for (i=0; i<new_size; i++) {
	    keytable[i].key   = table[first_entry].key;
	    keytable[i].color = first_entry;
	    first_entry	      = table[first_entry].color;
	}

	/* sort key table.  The "color" entry is the rank of the corresponding
	   process in the input communicator */
	MPIU_Sort_inttable( keytable, new_size );

	if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	    MPIU_CHKLMEM_MALLOC(remotekeytable,sorttype*,
				new_remote_size*sizeof(sorttype),
				mpi_errno,"remote keytable");
	    for (i=0; i<new_remote_size; i++) {
		remotekeytable[i].key   = remotetable[first_remote_entry].key;
		remotekeytable[i].color = first_remote_entry;
		first_remote_entry	= remotetable[first_remote_entry].color;
	    }

	    /* sort key table.  The "color" entry is the rank of the
	       corresponding process in the input communicator */
	    MPIU_Sort_inttable( remotekeytable, new_remote_size );

            MPIR_Comm_map_irregular(*newcomm_ptr, comm_ptr, NULL,
                                    new_size, MPIR_COMM_MAP_DIR_L2L,
                                    &mapper);

            for (i = 0; i < new_size; i++) {
                mapper->src_mapping[i] = keytable[i].color;
		if (keytable[i].color == comm_ptr->rank)
		    (*newcomm_ptr)->rank = i;
            }

	    /* For the remote group, the situation is more complicated.
	       We need to find the size of our "partner" group in the
	       remote comm.  The easiest way (in terms of code) is for
	       every process to essentially repeat the operation for the
	       local group - perform an (intercommunicator) all gather
	       of the color and rank information for the remote group.
	     */
	    /* We apply the same sorting algorithm to the entries that we've
	       found to get the correct order of the entries.

	       Note that if new_remote_size is 0 (no matching processes with
	       the same color in the remote group), then MPI_COMM_SPLIT
	       is required to return MPI_COMM_NULL instead of an intercomm 
	       with an empty remote group. */

            MPIR_Comm_map_irregular(*newcomm_ptr, comm_ptr, NULL,
                                    new_remote_size,
                                    MPIR_COMM_MAP_DIR_R2R, &mapper);

            for (i = 0; i < new_remote_size; i++)
                mapper->src_mapping[i] = remotekeytable[i].color;

	    (*newcomm_ptr)->context_id     = remote_context_id;
	    (*newcomm_ptr)->remote_size    = new_remote_size;
	    (*newcomm_ptr)->local_comm     = 0;
	    (*newcomm_ptr)->is_low_group   = comm_ptr->is_low_group;

	}
	else {
	    /* INTRA Communicator */
	    (*newcomm_ptr)->context_id     = (*newcomm_ptr)->recvcontext_id;
	    (*newcomm_ptr)->remote_size    = new_size;

            MPIR_Comm_map_irregular(*newcomm_ptr, comm_ptr, NULL,
                                    new_size, MPIR_COMM_MAP_DIR_L2L,
                                    &mapper);

            for (i = 0; i < new_size; i++) {
                mapper->src_mapping[i] = keytable[i].color;
		if (keytable[i].color == comm_ptr->rank)
		    (*newcomm_ptr)->rank = i;
            }
	}

	/* Inherit the error handler (if any) */
        MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));
	(*newcomm_ptr)->errhandler = comm_ptr->errhandler;
	if (comm_ptr->errhandler) {
	    MPIR_Errhandler_add_ref( comm_ptr->errhandler );
	}
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_COMM_MUTEX(comm_ptr));

        mpi_errno = MPIR_Comm_commit(*newcomm_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif




#undef FUNCNAME
#define FUNCNAME MPI_Comm_split
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Comm_split - Creates new communicators based on colors and keys

Input Parameters:
+ comm - communicator (handle) 
. color - control of subset assignment (nonnegative integer).  Processes 
  with the same color are in the same new communicator 
- key - control of rank assignment (integer)

Output Parameters:
. newcomm - new communicator (handle) 

Notes:
  The 'color' must be non-negative or 'MPI_UNDEFINED'.

.N ThreadSafe

.N Fortran

Algorithm:
.vb
  1. Use MPI_Allgather to get the color and key from each process
  2. Count the number of processes with the same color; create a 
     communicator with that many processes.  If this process has
     'MPI_UNDEFINED' as the color, create a process with a single member.
  3. Use key to order the ranks
.ve
 
.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_EXHAUSTED

.seealso: MPI_Comm_free
@*/
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SPLIT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SPLIT);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
    
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, &newcomm_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    if (newcomm_ptr)
        MPID_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    else
        *newcomm = MPI_COMM_NULL;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPLIT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_comm_split",
	    "**mpi_comm_split %C %d %d %p", comm, color, key, newcomm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

