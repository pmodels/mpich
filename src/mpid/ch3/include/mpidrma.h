/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#if !defined(MPICH_MPIDRMA_H_INCLUDED)
#define MPICH_MPIDRMA_H_INCLUDED

#include "mpl_utlist.h"
#include "mpidi_ch3_impl.h"

MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(RMA, rma_wincreate_allgather);
MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(RMA, rma_winfree_rs);
MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(RMA, rma_winfree_complete);

typedef enum MPIDI_RMA_Op_type {
    MPIDI_RMA_PUT               = 23,
    MPIDI_RMA_GET               = 24,
    MPIDI_RMA_ACCUMULATE        = 25,
 /* REMOVED: MPIDI_RMA_LOCK     = 26, */
    MPIDI_RMA_ACC_CONTIG        = 27,
    MPIDI_RMA_GET_ACCUMULATE    = 28,
    MPIDI_RMA_COMPARE_AND_SWAP  = 29,
    MPIDI_RMA_FETCH_AND_OP      = 30
} MPIDI_RMA_Op_type_t;

/* Special case RMA operations */

enum MPIDI_RMA_Datatype {
    MPIDI_RMA_DATATYPE_BASIC    = 50,
    MPIDI_RMA_DATATYPE_DERIVED  = 51
};

/* We start with an arbitrarily chosen number (42), to help with
 * debugging when a packet type is not initialized or wrongly
 * initialized. */
enum MPID_Lock_state {
    MPID_LOCK_NONE              = 42,
    MPID_LOCK_SHARED_ALL
};

/*
 * RMA Declarations.  We should move these into something separate from
 * a Request.
 */

/* to send derived datatype across in RMA ops */
typedef struct MPIDI_RMA_dtype_info { /* for derived datatypes */
    int           is_contig; 
    int           max_contig_blocks;
    MPI_Aint      size;
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
typedef struct MPIDI_RMA_Op {
    struct MPIDI_RMA_Op *prev;  /* pointer to next element in list */
    struct MPIDI_RMA_Op *next;  /* pointer to next element in list */
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
} MPIDI_RMA_Op_t;

typedef struct MPIDI_PT_single_op {
    int type;  /* put, get, or accum. */
    void *addr;
    int count;
    MPI_Datatype datatype;
    MPI_Op op;
    void *data;  /* for queued puts and accumulates, data is copied here */
    MPI_Request request_handle;  /* for gets */
    int data_recd;  /* to indicate if the data has been received */
    MPIDI_CH3_Pkt_flags_t flags;
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

typedef MPIDI_RMA_Op_t * MPIDI_RMA_Ops_list_t;

/* Return nonzero if the RMA operations list is empty.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_isempty
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_RMA_Ops_isempty(MPIDI_RMA_Ops_list_t *list)
{
    return *list == NULL;
}


/* Return a pointer to the first element in the list.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_head
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline MPIDI_RMA_Op_t *MPIDI_CH3I_RMA_Ops_head(MPIDI_RMA_Ops_list_t *list)
{
    return *list;
}


/* Return a pointer to the last element in the list.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_tail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline MPIDI_RMA_Op_t *MPIDI_CH3I_RMA_Ops_tail(MPIDI_RMA_Ops_list_t *list)
{
    return (*list) ? (*list)->prev : NULL;
}


/* Append an element to the tail of the RMA ops list
 *
 * @param IN    list      Pointer to the RMA ops list
 * @param IN    elem      Pointer to the element to be appended
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_append
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_RMA_Ops_append(MPIDI_RMA_Ops_list_t *list,
                                             MPIDI_RMA_Op_t *elem)
{
    MPL_DL_APPEND(*list, elem);
}


/* Allocate a new element on the tail of the RMA operations list.
 *
 * @param IN    list      Pointer to the RMA ops list
 * @param OUT   new_ptr   Pointer to the element that was allocated
 * @return                MPI error class
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_alloc_tail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_RMA_Ops_alloc_tail(MPIDI_RMA_Ops_list_t *list,
                                                MPIDI_RMA_Op_t **new_elem)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_Op_t *tmp_ptr;
    MPIU_CHKPMEM_DECL(1);

    /* FIXME: We should use a pool allocator here */
    MPIU_CHKPMEM_MALLOC(tmp_ptr, MPIDI_RMA_Op_t *, sizeof(MPIDI_RMA_Op_t),
                        mpi_errno, "RMA operation entry");

    tmp_ptr->next = NULL;
    tmp_ptr->dataloop = NULL;
    tmp_ptr->request = NULL;

    MPL_DL_APPEND(*list, tmp_ptr);

    *new_elem = tmp_ptr;

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    *new_elem = NULL;
    goto fn_exit;
}


/* Unlink an element from the RMA ops list
 *
 * @param IN    list      Pointer to the RMA ops list
 * @param IN    elem      Pointer to the element to be unlinked
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_unlink
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_RMA_Ops_unlink(MPIDI_RMA_Ops_list_t *list,
                                             MPIDI_RMA_Op_t *elem)
{
    MPL_DL_DELETE(*list, elem);
}


/* Free an element in the RMA operations list.
 *
 * @param IN    list      Pointer to the RMA ops list
 * @param IN    curr_ptr  Pointer to the element to be freed.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_free_elem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_RMA_Ops_free_elem(MPIDI_RMA_Ops_list_t *list,
                                                MPIDI_RMA_Op_t *curr_ptr)
{
    MPIDI_RMA_Op_t *tmp_ptr = curr_ptr;

    MPIU_Assert(curr_ptr != NULL);

    MPL_DL_DELETE(*list, curr_ptr);

    /* Check if we allocated a dataloop for this op (see send/recv_rma_msg) */
    if (tmp_ptr->dataloop != NULL)
        MPIU_Free(tmp_ptr->dataloop);
    MPIU_Free( tmp_ptr );
}


/* Free an element in the RMA operations list.
 *
 * @param IN    list      Pointer to the RMA ops list
 * @param INOUT curr_ptr  Pointer to the element to be freed.  Will be updated
 *                        to point to the element following the element that
 *                        was freed.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_free_and_next
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_RMA_Ops_free_and_next(MPIDI_RMA_Ops_list_t *list,
                                                    MPIDI_RMA_Op_t **curr_ptr)
{
    MPIDI_RMA_Op_t *next_ptr = (*curr_ptr)->next;

    MPIDI_CH3I_RMA_Ops_free_elem(list, *curr_ptr);
    *curr_ptr = next_ptr;
}


/* Free the entire RMA operations list.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_RMA_Ops_free(MPIDI_RMA_Ops_list_t *list)
{
    MPIDI_RMA_Op_t *curr_ptr, *tmp_ptr;

    MPL_DL_FOREACH_SAFE(*list, curr_ptr, tmp_ptr) {
        MPIDI_CH3I_RMA_Ops_free_elem(list, curr_ptr);
    }
}


/* Retrieve the RMA ops list pointer from the window.  This routine detects
 * whether we are in an active or passive target epoch and returns the correct
 * ops list; we use a shared list for active target and separate per-target
 * lists for passive target.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Get_ops_list
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline MPIDI_RMA_Ops_list_t *MPIDI_CH3I_RMA_Get_ops_list(MPID_Win *win_ptr,
                                                                int target)
{
    if (win_ptr->epoch_state == MPIDI_EPOCH_FENCE ||
        win_ptr->epoch_state == MPIDI_EPOCH_START ||
        win_ptr->epoch_state == MPIDI_EPOCH_PSCW)
    {
        return &win_ptr->at_rma_ops_list;
    }
    else {
        return &win_ptr->targets[target].rma_ops_list;
    }
}


/* ------------------------------------------------------------------------ */
/*
 * Followings are new routines for origin completion for RMA operations.
 */
/* ------------------------------------------------------------------------ */

#define ASSIGN_COPY(src, dest, count, type)     \
    {                                           \
        type *src_ = (type *) src;              \
        type *dest_ = (type *) dest;            \
        int i;                                  \
        for (i = 0; i < count; i++)             \
            dest_[i] = src_[i];                 \
        goto fn_exit;                           \
    }

static inline int shm_copy(const void *src, int scount, MPI_Datatype stype,
                           void *dest, int dcount, MPI_Datatype dtype)
{
    int mpi_errno = MPI_SUCCESS;

    /* We use a threshold of operations under which a for loop of assignments is
     * used.  Even though this happens at smaller block lengths, making it
     * potentially inefficient, it can take advantage of some vectorization
     * available on most modern processors. */
#define SHM_OPS_THRESHOLD  (16)

    if (MPIR_DATATYPE_IS_PREDEFINED(stype) && MPIR_DATATYPE_IS_PREDEFINED(dtype) &&
        scount <= SHM_OPS_THRESHOLD) {

        /* FIXME: We currently only optimize a few predefined datatypes, which
         * have a direct C datatype mapping. */

        /* The below list of datatypes is based on those specified in the MPI-3
         * standard on page 665. */
        switch (stype) {
            case MPI_CHAR:
                ASSIGN_COPY(src, dest, scount, char);

            case MPI_SHORT:
                ASSIGN_COPY(src, dest, scount, signed short int);

            case MPI_INT:
                ASSIGN_COPY(src, dest, scount, signed int);

            case MPI_LONG:
                ASSIGN_COPY(src, dest, scount, signed long int);

            case MPI_LONG_LONG_INT:  /* covers MPI_LONG_LONG too */
                ASSIGN_COPY(src, dest, scount, signed long long int);

            case MPI_SIGNED_CHAR:
                ASSIGN_COPY(src, dest, scount, signed char);

            case MPI_UNSIGNED_CHAR:
                ASSIGN_COPY(src, dest, scount, unsigned char);

            case MPI_UNSIGNED_SHORT:
                ASSIGN_COPY(src, dest, scount, unsigned short int);

            case MPI_UNSIGNED:
                ASSIGN_COPY(src, dest, scount, unsigned int);

            case MPI_UNSIGNED_LONG:
                ASSIGN_COPY(src, dest, scount, unsigned long int);

            case MPI_UNSIGNED_LONG_LONG:
                ASSIGN_COPY(src, dest, scount, unsigned long long int);

            case MPI_FLOAT:
                ASSIGN_COPY(src, dest, scount, float);

            case MPI_DOUBLE:
                ASSIGN_COPY(src, dest, scount, double);

            case MPI_LONG_DOUBLE:
                ASSIGN_COPY(src, dest, scount, long double);

#if 0
            /* FIXME: we need a configure check to define HAVE_WCHAR_T before
             * this can be enabled */
            case MPI_WCHAR:
                ASSIGN_COPY(src, dest, scount, wchar_t);
#endif

#if 0
            /* FIXME: we need a configure check to define HAVE_C_BOOL before
             * this can be enabled */
            case MPI_C_BOOL:
                ASSIGN_COPY(src, dest, scount, _Bool);
#endif

#if HAVE_INT8_T
            case MPI_INT8_T:
                ASSIGN_COPY(src, dest, scount, int8_t);
#endif /* HAVE_INT8_T */

#if HAVE_INT16_T
            case MPI_INT16_T:
                ASSIGN_COPY(src, dest, scount, int16_t);
#endif /* HAVE_INT16_T */

#if HAVE_INT32_T
            case MPI_INT32_T:
                ASSIGN_COPY(src, dest, scount, int32_t);
#endif /* HAVE_INT32_T */

#if HAVE_INT64_T
            case MPI_INT64_T:
                ASSIGN_COPY(src, dest, scount, int64_t);
#endif /* HAVE_INT64_T */

#if HAVE_UINT8_T
            case MPI_UINT8_T:
                ASSIGN_COPY(src, dest, scount, uint8_t);
#endif /* HAVE_UINT8_T */

#if HAVE_UINT16_T
            case MPI_UINT16_T:
                ASSIGN_COPY(src, dest, scount, uint16_t);
#endif /* HAVE_UINT16_T */

#if HAVE_UINT32_T
            case MPI_UINT32_T:
                ASSIGN_COPY(src, dest, scount, uint32_t);
#endif /* HAVE_UINT32_T */

#if HAVE_UINT64_T
            case MPI_UINT64_T:
                ASSIGN_COPY(src, dest, scount, uint64_t);
#endif /* HAVE_UINT64_T */

            case MPI_AINT:
                ASSIGN_COPY(src, dest, scount, MPI_Aint);

            case MPI_COUNT:
                ASSIGN_COPY(src, dest, scount, MPI_Count);

            case MPI_OFFSET:
                ASSIGN_COPY(src, dest, scount, MPI_Offset);

#if 0
            /* FIXME: we need a configure check to define HAVE_C_COMPLEX before
             * this can be enabled */
            case MPI_C_COMPLEX:  /* covers MPI_C_FLOAT_COMPLEX as well */
                ASSIGN_COPY(src, dest, scount, float _Complex);
#endif

#if 0
            /* FIXME: we need a configure check to define HAVE_C_DOUPLE_COMPLEX
             * before this can be enabled */
            case MPI_C_DOUBLE_COMPLEX:
                ASSIGN_COPY(src, dest, scount, double _Complex);
#endif

#if 0
            /* FIXME: we need a configure check to define
             * HAVE_C_LONG_DOUPLE_COMPLEX before this can be enabled */
            case MPI_C_LONG_DOUBLE_COMPLEX:
                ASSIGN_COPY(src, dest, scount, long double _Complex);
#endif

#if 0
            /* Types that don't have a direct equivalent */
            case MPI_BYTE:
            case MPI_PACKED:
#endif

#if 0 /* Fortran types */
            case MPI_INTEGER:
            case MPI_REAL:
            case MPI_DOUBLE_PRECISION:
            case MPI_COMPLEX:
            case MPI_LOGICAL:
            case MPI_CHARACTER:
#endif

#if 0 /* C++ types */
            case MPI_CXX_BOOL:
            case MPI_CXX_FLOAT_COMPLEX:
            case MPI_CXX_DOUBLE_COMPLEX:
            case MPI_CXX_LONG_DOUBLE_COMPLEX:
#endif

#if 0 /* Optional Fortran types */
            case MPI_DOUBLE_COMPLEX:
            case MPI_INTEGER1:
            case MPI_INTEGER2:
            case MPI_INTEGER4:
            case MPI_INTEGER8:
            case MPI_INTEGER16:
            case MPI_REAL2:
            case MPI_REAL4:
            case MPI_REAL8:
            case MPI_REAL16:
            case MPI_COMPLEX4:
            case MPI_COMPLEX8:
            case MPI_COMPLEX16:
            case MPI_COMPLEX32:
#endif

#if 0 /* C datatypes for reduction functions */
            case MPI_FLOAT_INT:
            case MPI_DOUBLE_INT:
            case MPI_LONG_INT:
            case MPI_2INT:
            case MPI_LONG_DOUBLE_INT:
#endif

#if 0 /* Fortran datatypes for reduction functions */
            case MPI_2REAL:
            case MPI_2DOUBLE_PRECISION:
            case MPI_2INTEGER:
#endif

#if 0 /* Random types not present in the standard */
            case MPI_2COMPLEX:
            case MPI_2DOUBLE_COMPLEX:
#endif

            default:
                /* Just to make sure the switch statement is not empty */
                ;
        }
    }

    mpi_errno = MPIR_Localcopy(src, scount, stype, dest, dcount, dtype);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

 fn_exit:
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_put_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_put_op(const void *origin_addr, int origin_count, MPI_Datatype
                                        origin_datatype, int target_rank, MPI_Aint target_disp,
                                        int target_count, MPI_Datatype target_datatype,
                                        MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    void *base = NULL;
    int disp_unit;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_PUT_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_PUT_OP);

    if (win_ptr->shm_allocated == TRUE) {
        base = win_ptr->shm_base_addrs[target_rank];
        disp_unit = win_ptr->disp_units[target_rank];
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    mpi_errno = shm_copy(origin_addr, origin_count, origin_datatype,
                         (char *) base + disp_unit * target_disp, target_count, target_datatype);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_PUT_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_acc_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_acc_op(const void *origin_addr, int origin_count, MPI_Datatype
                                        origin_datatype, int target_rank, MPI_Aint target_disp,
                                        int target_count, MPI_Datatype target_datatype, MPI_Op op,
                                        MPID_Win *win_ptr)
{
    void *base = NULL;
    int disp_unit, shm_op = 0;
    MPI_User_function *uop = NULL;
    MPID_Datatype *dtp;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_ACC_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_ACC_OP);

    if (win_ptr->shm_allocated == TRUE) {
        shm_op = 1;
        base = win_ptr->shm_base_addrs[target_rank];
        disp_unit = win_ptr->disp_units[target_rank];
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    if (op == MPI_REPLACE)
    {
        if (shm_op) MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        mpi_errno = shm_copy(origin_addr, origin_count, origin_datatype,
                             (char *) base + disp_unit * target_disp, target_count, target_datatype);
        if (shm_op) MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        goto fn_exit;
    }

    MPIU_ERR_CHKANDJUMP1((HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN),
                         mpi_errno, MPI_ERR_OP, "**opnotpredefined",
                         "**opnotpredefined %d", op );

    /* get the function by indexing into the op table */
    uop = MPIR_OP_HDL_TO_FN(op);

    if (MPIR_DATATYPE_IS_PREDEFINED(origin_datatype) && MPIR_DATATYPE_IS_PREDEFINED(target_datatype))
    {
        /* Cast away const'ness for origin_address in order to
         * avoid changing the prototype for MPI_User_function */
        if (shm_op) MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        (*uop)((void *) origin_addr, (char *) base + disp_unit*target_disp,
               &target_count, &target_datatype);
        if (shm_op) MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }
    else
    {
        /* derived datatype */

        MPID_Segment *segp;
        DLOOP_VECTOR *dloop_vec;
        MPI_Aint first, last;
        int vec_len, i, type_size, count;
        MPI_Datatype type;
        MPI_Aint true_lb, true_extent, extent;
        void *tmp_buf=NULL, *target_buf;
        const void *source_buf;

        if (origin_datatype != target_datatype)
        {
            /* first copy the data into a temporary buffer with
               the same datatype as the target. Then do the
               accumulate operation. */

            MPIR_Type_get_true_extent_impl(target_datatype, &true_lb, &true_extent);
            MPID_Datatype_get_extent_macro(target_datatype, extent);

            MPIU_CHKLMEM_MALLOC(tmp_buf, void *,
                                target_count * (MPIR_MAX(extent,true_extent)),
                                mpi_errno, "temporary buffer");
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *)((char*)tmp_buf - true_lb);

            mpi_errno = MPIR_Localcopy(origin_addr, origin_count,
                                       origin_datatype, tmp_buf,
                                       target_count, target_datatype);
            if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        }

        if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            /* target predefined type, origin derived datatype */

            if (shm_op) MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
            (*uop)(tmp_buf, (char *) base + disp_unit * target_disp,
                   &target_count, &target_datatype);
            if (shm_op) MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        }
        else {

            segp = MPID_Segment_alloc();
            MPIU_ERR_CHKANDJUMP1((!segp), mpi_errno, MPI_ERR_OTHER,
                                 "**nomem","**nomem %s","MPID_Segment_alloc");
            MPID_Segment_init(NULL, target_count, target_datatype, segp, 0);
            first = 0;
            last  = SEGMENT_IGNORE_LAST;

            MPID_Datatype_get_ptr(target_datatype, dtp);
            vec_len = dtp->max_contig_blocks * target_count + 1;
            /* +1 needed because Rob says so */
            MPIU_CHKLMEM_MALLOC(dloop_vec, DLOOP_VECTOR *,
                                vec_len * sizeof(DLOOP_VECTOR),
                                mpi_errno, "dloop vector");

            MPID_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);

            source_buf = (tmp_buf != NULL) ? (const void *)tmp_buf : origin_addr;
            target_buf = (char *) base + disp_unit * target_disp;
            type = dtp->eltype;
            type_size = MPID_Datatype_get_basic_size(type);
            if (shm_op) MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
            for (i=0; i<vec_len; i++)
            {
		MPIU_Assign_trunc(count, (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size, int);

                (*uop)((char *)source_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
                       (char *)target_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
                       &count, &type);
            }
            if (shm_op) MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

            MPID_Segment_free(segp);
        }
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ACC_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    if (shm_op) MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_get_acc_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_get_acc_op(const void *origin_addr, int origin_count, MPI_Datatype
                                            origin_datatype, void *result_addr, int result_count,
                                            MPI_Datatype result_datatype, int target_rank, MPI_Aint
                                            target_disp, int target_count, MPI_Datatype target_datatype,
                                            MPI_Op op, MPID_Win *win_ptr)
{
    int disp_unit, shm_locked = 0;
    void *base = NULL;
    MPI_User_function *uop = NULL;
    MPID_Datatype *dtp;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_ACC_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_ACC_OP);

    if (win_ptr->shm_allocated == TRUE) {
        base = win_ptr->shm_base_addrs[target_rank];
        disp_unit = win_ptr->disp_units[target_rank];
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        shm_locked = 1;
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    /* Perform the local get first, then the accumulate */
    mpi_errno = shm_copy((char *) base + disp_unit * target_disp, target_count, target_datatype,
                         result_addr, result_count, result_datatype);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    /* NO_OP: Don't perform the accumulate */
    if (op == MPI_NO_OP) {
        if (shm_locked) {
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
            shm_locked = 0;
        }

        goto fn_exit;
    }

    if (op == MPI_REPLACE) {
        mpi_errno = shm_copy(origin_addr, origin_count, origin_datatype,
                             (char *) base + disp_unit * target_disp, target_count, target_datatype);

        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        if (shm_locked) {
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
            shm_locked = 0;
        }

        goto fn_exit;
    }

    MPIU_ERR_CHKANDJUMP1((HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN),
                          mpi_errno, MPI_ERR_OP, "**opnotpredefined",
                          "**opnotpredefined %d", op );

    /* get the function by indexing into the op table */
    uop = MPIR_OP_HDL_TO_FN(op);

    if ((op == MPI_NO_OP || MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) &&
        MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
        /* Cast away const'ness for origin_address in order to
         * avoid changing the prototype for MPI_User_function */
        (*uop)((void *) origin_addr, (char *) base + disp_unit*target_disp,
               &target_count, &target_datatype);
    }
    else {
        /* derived datatype */

        MPID_Segment *segp;
        DLOOP_VECTOR *dloop_vec;
        MPI_Aint first, last;
        int vec_len, i, type_size, count;
        MPI_Datatype type;
        MPI_Aint true_lb, true_extent, extent;
        void *tmp_buf=NULL, *target_buf;
        const void *source_buf;

        if (origin_datatype != target_datatype) {
            /* first copy the data into a temporary buffer with
               the same datatype as the target. Then do the
               accumulate operation. */

            MPIR_Type_get_true_extent_impl(target_datatype, &true_lb, &true_extent);
            MPID_Datatype_get_extent_macro(target_datatype, extent);

            MPIU_CHKLMEM_MALLOC(tmp_buf, void *,
                                target_count * (MPIR_MAX(extent,true_extent)),
                                mpi_errno, "temporary buffer");
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *)((char*)tmp_buf - true_lb);

            mpi_errno = MPIR_Localcopy(origin_addr, origin_count,
                                       origin_datatype, tmp_buf,
                                       target_count, target_datatype);
            if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        }

        if (MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            /* target predefined type, origin derived datatype */

            (*uop)(tmp_buf, (char *) base + disp_unit * target_disp,
                   &target_count, &target_datatype);
        }
        else {

            segp = MPID_Segment_alloc();
            MPIU_ERR_CHKANDJUMP1((!segp), mpi_errno, MPI_ERR_OTHER,
                                 "**nomem","**nomem %s","MPID_Segment_alloc");
            MPID_Segment_init(NULL, target_count, target_datatype, segp, 0);
            first = 0;
            last  = SEGMENT_IGNORE_LAST;

            MPID_Datatype_get_ptr(target_datatype, dtp);
            vec_len = dtp->max_contig_blocks * target_count + 1;
            /* +1 needed because Rob says so */
            MPIU_CHKLMEM_MALLOC(dloop_vec, DLOOP_VECTOR *,
                                vec_len * sizeof(DLOOP_VECTOR),
                                mpi_errno, "dloop vector");

            MPID_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);

            source_buf = (tmp_buf != NULL) ? (const void *)tmp_buf : origin_addr;
            target_buf = (char *) base + disp_unit * target_disp;
            type = dtp->eltype;
            type_size = MPID_Datatype_get_basic_size(type);

            for (i=0; i<vec_len; i++) {
		MPIU_Assign_trunc(count, (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size, int);
                (*uop)((char *)source_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
                       (char *)target_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
                       &count, &type);
            }

            MPID_Segment_free(segp);
        }
    }

    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        shm_locked = 0;
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_ACC_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_get_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_get_op(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                                        int target_rank, MPI_Aint target_disp, int target_count,
                                        MPI_Datatype target_datatype, MPID_Win *win_ptr)
{
    void *base = NULL;
    int disp_unit;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_OP);

    if (win_ptr->shm_allocated == TRUE) {
        base = win_ptr->shm_base_addrs[target_rank];
        disp_unit = win_ptr->disp_units[target_rank];
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    mpi_errno = shm_copy((char *) base + disp_unit * target_disp, target_count, target_datatype,
                         origin_addr, origin_count, origin_datatype);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_cas_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_cas_op(const void *origin_addr, const void *compare_addr,
                                        void *result_addr, MPI_Datatype datatype, int target_rank,
                                        MPI_Aint target_disp, MPID_Win *win_ptr)
{
    void *base = NULL, *dest_addr = NULL;
    int disp_unit;
    MPI_Aint len;
    int shm_locked = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_CAS_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_CAS_OP);

    if (win_ptr->shm_allocated == TRUE) {
        base = win_ptr->shm_base_addrs[target_rank];
        disp_unit = win_ptr->disp_units[target_rank];

        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        shm_locked = 1;
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    dest_addr = (char *) base + disp_unit * target_disp;

    MPID_Datatype_get_size_macro(datatype, len);
    MPIU_Memcpy(result_addr, dest_addr, len);

    if (MPIR_Compare_equal(compare_addr, dest_addr, datatype)) {
        MPIU_Memcpy(dest_addr, origin_addr, len);
    }

    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        shm_locked = 0;
    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_CAS_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_fop_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_fop_op(const void *origin_addr, void *result_addr,
                                        MPI_Datatype datatype, int target_rank,
                                        MPI_Aint target_disp, MPI_Op op, MPID_Win *win_ptr)
{
    void *base = NULL, *dest_addr = NULL;
    MPI_User_function *uop = NULL;
    int disp_unit;
    MPI_Aint len;
    int one, shm_locked = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_FOP_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_FOP_OP);

    if (win_ptr->shm_allocated == TRUE) {
        base = win_ptr->shm_base_addrs[target_rank];
        disp_unit = win_ptr->disp_units[target_rank];

        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        shm_locked = 1;
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    dest_addr = (char *) base + disp_unit * target_disp;

    MPID_Datatype_get_size_macro(datatype, len);
    MPIU_Memcpy(result_addr, dest_addr, len);

    uop = MPIR_OP_HDL_TO_FN(op);
    one = 1;

    (*uop)((void *) origin_addr, dest_addr, &one, &datatype);

    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        shm_locked = 0;
    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_FOP_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Wait_for_pt_ops_finish
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Wait_for_pt_ops_finish(MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, total_pt_rma_puts_accs;
    MPID_Comm *comm_ptr;
    int errflag = FALSE;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_WAIT_FOR_PT_OPS_FINISH);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_WAIT_FOR_PT_OPS_FINISH);

    comm_ptr = win_ptr->comm_ptr;
    MPIR_T_PVAR_TIMER_START(RMA, rma_winfree_rs);
    mpi_errno = MPIR_Reduce_scatter_block_impl(win_ptr->pt_rma_puts_accs,
                                               &total_pt_rma_puts_accs, 1,
                                               MPI_INT, MPI_SUM, comm_ptr, &errflag);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    MPIR_T_PVAR_TIMER_END(RMA, rma_winfree_rs);

    if (total_pt_rma_puts_accs != win_ptr->my_pt_rma_puts_accs)
    {
	MPID_Progress_state progress_state;

	/* poke the progress engine until the two are equal */
	MPIR_T_PVAR_TIMER_START(RMA, rma_winfree_complete);
	MPID_Progress_start(&progress_state);
	while (total_pt_rma_puts_accs != win_ptr->my_pt_rma_puts_accs)
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPID_Progress_end(&progress_state);
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**winnoprogress");
	    }
	    /* --END ERROR HANDLING-- */
	}
	MPID_Progress_end(&progress_state);
	MPIR_T_PVAR_TIMER_END(RMA, rma_winfree_complete);
    }

 fn_exit:
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_WAIT_FOR_PT_OPS_FINISH);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#undef FCNAME

#endif
