/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#if !defined(MPICH_MPIDRMA_H_INCLUDED)
#define MPICH_MPIDRMA_H_INCLUDED

typedef enum MPIDI_RMA_Op_type_e {
    MPIDI_RMA_PUT               = 23,
    MPIDI_RMA_GET               = 24,
    MPIDI_RMA_ACCUMULATE        = 25,
    MPIDI_RMA_LOCK              = 26,
    MPIDI_RMA_ACC_CONTIG        = 27,
    MPIDI_RMA_GET_ACCUMULATE    = 28,
    MPIDI_RMA_COMPARE_AND_SWAP  = 29,
    MPIDI_RMA_FETCH_AND_OP      = 30
} MPIDI_RMA_Op_type_t;

/* Special case RMA operations */

enum MPIDI_RMA_Datatype_e {
    MPIDI_RMA_DATATYPE_BASIC    = 50,
    MPIDI_RMA_DATATYPE_DERIVED  = 51
};

enum MPID_Lock_state_e {
    MPID_LOCK_NONE              = 0,
    MPID_LOCK_SHARED_ALL        = 1
};

/*
 * RMA Declarations.  We should move these into something separate from
 * a Request.
 */

/* to send derived datatype across in RMA ops */
typedef struct MPIDI_RMA_dtype_info { /* for derived datatypes */
    int           is_contig; 
    int           max_contig_blocks;
    int           size;     
    MPI_Aint      extent;   
    int           dataloop_size; /* not needed because this info is sent in 
				    packet header. remove it after lock/unlock 
				    is implemented in the device */
    void          *dataloop;  /* pointer needed to update pointers
                                 within dataloop on remote side */
    int           dataloop_depth; 
    int           eltype;
    MPI_Aint ub, lb, true_ub, true_lb;
    int has_sticky_ub, has_sticky_lb;
} MPIDI_RMA_dtype_info;

/* for keeping track of RMA ops, which will be executed at the next sync call */
typedef struct MPIDI_RMA_ops {
    struct MPIDI_RMA_ops *next;  /* pointer to next element in list */
    /* FIXME: It would be better to setup the packet that will be sent, at 
       least in most cases (if, as a result of the sync/ops/sync sequence,
       a different packet type is needed, it can be extracted from the 
       information otherwise stored). */
    MPIDI_RMA_Op_type_t type;
    void *origin_addr;
    int origin_count;
    MPI_Datatype origin_datatype;
    int target_rank;
    MPI_Aint target_disp;
    int target_count;
    MPI_Datatype target_datatype;
    MPI_Op op;  /* for accumulate */
    int lock_type;  /* for win_lock */
    /* Used to complete operations */
    struct MPID_Request *request;
    MPIDI_RMA_dtype_info dtype_info;
    void *dataloop;
    void *result_addr;
    int result_count;
    MPI_Datatype result_datatype;
    void *compare_addr;
    int compare_count;
    MPI_Datatype compare_datatype;
} MPIDI_RMA_ops;

typedef struct MPIDI_PT_single_op {
    int type;  /* put, get, or accum. */
    void *addr;
    int count;
    MPI_Datatype datatype;
    MPI_Op op;
    void *data;  /* for queued puts and accumulates, data is copied here */
    MPI_Request request_handle;  /* for gets */
    int data_recd;  /* to indicate if the data has been received */
} MPIDI_PT_single_op;

typedef struct MPIDI_Win_lock_queue {
    struct MPIDI_Win_lock_queue *next;
    int lock_type;
    MPI_Win source_win_handle;
    MPIDI_VC_t * vc;
    struct MPIDI_PT_single_op *pt_single_op;  /* to store info for 
						 lock-put-unlock optimization */
} MPIDI_Win_lock_queue;

/* Routine use to tune RMA optimizations */
void MPIDI_CH3_RMA_SetAccImmed( int flag );

/*** RMA OPS LIST HELPER ROUTINES ***/

/* Return nonzero if the RMA operations list is empty.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_ops_isempty
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_ops_isempty(MPID_Win *win_ptr)
{
    return win_ptr->rma_ops_list_head == NULL;
}


/* Allocate a new element on the tail of the RMA operations list.
 *
 * @param IN    win_ptr   Window containing the RMA ops list
 * @param OUT   curr_ptr  Pointer to the element to the element that was
 *                        updated.
 * @return                MPI error class
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_ops_alloc_tail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_ops_alloc_tail(MPID_Win *win_ptr,
                                                MPIDI_RMA_ops **curr_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_ops *tmp_ptr;
    MPIU_CHKPMEM_DECL(1);

    /* This assertion can fail because the tail pointer is not maintainted (see
       free_and_next).  If this happens, consider refactoring to a previous
       element pointer (instead of a pointer to the prevous next pointer) or a
       doubly-linked list. */
    MPIU_Assert(MPIDI_CH3I_Win_ops_isempty(win_ptr) || win_ptr->rma_ops_list_tail != NULL);

    /* FIXME: We should use a pool allocator here */
    MPIU_CHKPMEM_MALLOC(tmp_ptr, MPIDI_RMA_ops *, sizeof(MPIDI_RMA_ops),
                        mpi_errno, "RMA operation entry");

    tmp_ptr->next = NULL;
    tmp_ptr->dataloop = NULL;

    if (MPIDI_CH3I_Win_ops_isempty(win_ptr))
        win_ptr->rma_ops_list_head = tmp_ptr;
    else
        win_ptr->rma_ops_list_tail->next = tmp_ptr;

    win_ptr->rma_ops_list_tail = tmp_ptr;
    *curr_ptr = tmp_ptr;

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


/* Free an element in the RMA operations list.
 *
 * NOTE: It is not currently possible (singly-linked list) for this operation
 * to maintain the rma_ops_list_tail pointer.  If the freed element was at the
 * tail, the tail pointer will become stale.  One should not rely a correct
 * value of rma_ops_list_tail after calling this function -- specifically, one
 * should be wary of calling alloc_tail unless the free operation emptied the
 * list.
 *
 * @param IN    win_ptr   Window containing the RMA ops list
 * @param INOUT curr_ptr  Pointer to the element to be freed.  Will be updated
 *                        to point to the element following the element that
 *                        was freed.
 * @param IN    prev_next_ptr Pointer to the previous operation's next pointer.
 *                        Used to unlink the element reference by curr_ptr.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_ops_free_and_next
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_Win_ops_free_and_next(MPID_Win *win_ptr,
                                                    MPIDI_RMA_ops **curr_ptr,
                                                    MPIDI_RMA_ops **prev_next_ptr)
{
    MPIDI_RMA_ops *tmp_ptr = *curr_ptr;

    MPIU_Assert(tmp_ptr != NULL && *prev_next_ptr == tmp_ptr);

    /* Check if we are down to one element in the ops list */
    if (win_ptr->rma_ops_list_head->next == NULL) {
        MPIU_Assert(tmp_ptr == win_ptr->rma_ops_list_head);
        win_ptr->rma_ops_list_tail = NULL;
        win_ptr->rma_ops_list_head = NULL;
    }

    /* Check if this free invalidates the tail pointer.  If so, set it to NULL
       for safety. */
    if (win_ptr->rma_ops_list_tail == *curr_ptr)
        win_ptr->rma_ops_list_tail = NULL;

    /* Unlink the element */
    *curr_ptr = tmp_ptr->next;
    *prev_next_ptr = tmp_ptr->next;

    /* Check if we allocated a dataloop for this op (see send/recv_rma_msg) */
    if (tmp_ptr->dataloop != NULL)
        MPIU_Free(tmp_ptr->dataloop);
    MPIU_Free( tmp_ptr );
}


/* Free the entire RMA operations list.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_ops_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_Win_ops_free(MPID_Win *win_ptr)
{
    MPIDI_RMA_ops *curr_ptr = win_ptr->rma_ops_list_head;

    while (curr_ptr != NULL) {
        MPIDI_RMA_ops *tmp_ptr = curr_ptr->next;

        /* Free this ops list entry */
        if (curr_ptr->dataloop != NULL)
            MPIU_Free(tmp_ptr->dataloop);
        MPIU_Free( curr_ptr );

        curr_ptr = tmp_ptr;
    }

    win_ptr->rma_ops_list_tail = NULL;
    win_ptr->rma_ops_list_head = NULL;
}


#undef FUNCNAME
#undef FCNAME

#endif
