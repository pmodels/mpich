/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CTXID_EAGER_SIZE
      category    : THREADS
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The MPIR_CVAR_CTXID_EAGER_SIZE environment variable allows you to
        specify how many words in the context ID mask will be set aside
        for the eager allocation protocol.  If the application is running
        out of context IDs, reducing this value may help.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/*
 * Here are the routines to find a new context id.  The algorithm is discussed
 * in detail in the mpich coding document.  There are versions for
 * single threaded and multithreaded MPI.
 *
 * Both the threaded and non-threaded routines use the same mask of
 * available context id values.
 */
static uint32_t context_mask[MPIR_MAX_CONTEXT_MASK];
const int ALL_OWN_MASK_FLAG = MPIR_MAX_CONTEXT_MASK;

/* utility function to pretty print a context ID for debugging purposes, see
 * mpiimpl.h for more info on the various fields */
#ifdef MPL_USE_DBG_LOGGING
static void dump_context_id(int context_id, char *out_str, int len)
{
    int subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, context_id);
    const char *subcomm_type_name = NULL;

    switch (subcomm_type) {
        case 0:
            subcomm_type_name = "parent";
            break;
        case 1:
            subcomm_type_name = "intranode";
            break;
        case 2:
            subcomm_type_name = "internode";
            break;
        default:
            MPIR_Assert(FALSE);
            break;
    }
    snprintf(out_str, len,
             "context_id=%d (%#x): DYNAMIC_PROC=%d PREFIX=%#x IS_LOCALCOMM=%d SUBCOMM=%s SUFFIX=%s",
             context_id,
             context_id,
             MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, context_id),
             MPIR_CONTEXT_READ_FIELD(PREFIX, context_id),
             MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id),
             subcomm_type_name, (MPIR_CONTEXT_READ_FIELD(SUFFIX, context_id) ? "coll" : "pt2pt"));
}

/* Create a string that contains the context mask.  This is
   used only with the logging interface, and must be used by one thread at
   a time (should this be enforced by the logging interface?).
   Converts the mask to hex and returns a pointer to that string.

   Callers should own the context ID critical section, or should be prepared to
   suffer data races in any fine-grained locking configuration.

   This routine is no longer static in order to allow advanced users and
   developers to debug context ID problems "in the field".  We provide a
   prototype here to keep the compiler happy, but users will need to put a
   (possibly "extern") copy of the prototype in their own code in order to call
   this routine.
 */
static char *context_mask_to_str(void)
{
    static char bufstr[MPIR_MAX_CONTEXT_MASK * 8 + 1];
    int i;
    int maxset = 0;

    for (maxset = MPIR_MAX_CONTEXT_MASK - 1; maxset >= 0; maxset--) {
        if (context_mask[maxset] != 0)
            break;
    }

    for (i = 0; i < maxset; i++) {
        snprintf(&bufstr[i * 8], 9, "%.8x", context_mask[i]);
    }
    return bufstr;
}
#endif

/* Returns useful debugging information about the context ID mask bit-vector.
 * This includes the total number of possibly valid IDs (the size of the ID
 * space) and the number of free IDs remaining in the mask.  NULL arguments are
 * fine, they will be ignored.
 *
 * This routine is for debugging in very particular situations and does not
 * attempt to control concurrent access to the mask vector.
 *
 * Callers should own the context ID critical section, or should be prepared to
 * suffer data races in any fine-grained locking configuration.
 *
 * The routine is non-static in order to permit "in the field debugging".  We
 * provide a prototype here to keep the compiler happy. */
static void context_mask_stats(int *free_ids, int *total_ids)
{
    if (free_ids) {
        unsigned int i, j;
        *free_ids = 0;

        /* if this ever needs to be fast, use a lookup table to do a per-nibble
         * or per-byte lookup of the popcount instead of checking each bit at a
         * time (or just track the count when manipulating the mask and keep
         * that count stored in a variable) */
        for (i = 0; i < MPIR_MAX_CONTEXT_MASK; ++i) {
            for (j = 0; j < sizeof(context_mask[0]) * 8; ++j) {
                *free_ids += (context_mask[i] & (0x1U << j)) >> j;
            }
        }
    }
    if (total_ids) {
        *total_ids = MPIR_MAX_CONTEXT_MASK * sizeof(context_mask[0]) * 8;
    }
}

#ifdef MPICH_DEBUG_HANDLEALLOC
static int check_context_ids_on_finalize(void *context_mask_ptr)
{
    int i;
    uint32_t *mask = context_mask_ptr;
    /* the predefined communicators should be freed by this point, so we don't
     * need to special case bits 0,1, and 2 */
    for (i = 0; i < MPIR_MAX_CONTEXT_MASK; ++i) {
        if (~mask[i]) {
            /* some bits were still cleared */
            printf("leaked context IDs detected: mask=%p mask[%d]=%#x\n", mask, i, (int) mask[i]);
        }
    }
    return MPI_SUCCESS;
}
#endif

void MPIR_context_id_init(void)
{
    int i;

    for (i = 1; i < MPIR_MAX_CONTEXT_MASK; i++) {
        context_mask[i] = 0xFFFFFFFF;
    }
    /* reserve the first two values for comm_world and comm_self */
    context_mask[0] = 0xFFFFFFFC;

#ifdef MPICH_DEBUG_HANDLEALLOC
    /* check for context ID leaks in MPI_Finalize.  Use (_PRIO-1) to make sure
     * that we run after MPID_Finalize. */
    MPIR_Add_finalize(check_context_ids_on_finalize, context_mask, MPIR_FINALIZE_CALLBACK_PRIO - 1);
#endif
}

/* Return the context id corresponding to the first set bit in the mask.
   Return 0 if no bit found.  This function does _not_ alter local_mask. */
static int locate_context_bit(uint32_t local_mask[])
{
    int i, j, context_id = 0;
    for (i = 0; i < MPIR_MAX_CONTEXT_MASK; i++) {
        if (local_mask[i]) {
            /* There is a bit set in this word. */
            register uint32_t val, nval;
            /* The following code finds the highest set bit by recursively
             * checking the top half of a subword for a bit, and incrementing
             * the bit location by the number of bit of the lower sub word if
             * the high subword contains a set bit.  The assumption is that
             * full-word bitwise operations and compares against zero are
             * fast */
            val = local_mask[i];
            j = 0;
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
            context_id = (MPIR_CONTEXT_INT_BITS * i + j) << MPIR_CONTEXT_PREFIX_SHIFT;
            return context_id;
        }
    }
    return 0;
}

/* Allocates a context ID from the given mask by clearing the bit
 * corresponding to the the given id.  Returns 0 on failure, id on
 * success. */
static int allocate_context_bit(uint32_t mask[], int id)
{
    int raw_prefix, idx, bitpos;
    raw_prefix = MPIR_CONTEXT_READ_FIELD(PREFIX, id);
    idx = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;

    /* the bit should not already be cleared (allocated) */
    MPIR_Assert(mask[idx] & (1U << bitpos));

    /* clear the bit */
    mask[idx] &= ~(1U << bitpos);

    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST,
                                             "allocating contextid = %d, (mask=%p, mask[%d], bit %d)",
                                             id, mask, idx, bitpos));
    return id;
}

/* EAGER CONTEXT ID ALLOCATION: Attempt to allocate the context ID during the
 * initial synchronization step.  If eager protocol fails, threads fall back to
 * the base algorithm.
 *
 * They are used to avoid deadlock in multi-threaded case. In single-threaded
 * case, they are not used.
 */
static int eager_nelem = -1;
static int eager_in_use = 0;

/* In multi-threaded case, mask_in_use is used to maintain thread safety. In
 * single-threaded case, it is always 0. */
static volatile int mask_in_use = 0;

/* In multi-threaded case, lowest_context_id is used to prioritize access when
 * multiple threads are contending for the mask, lowest_tag is used to break
 * ties when MPI_Comm_create_group is invoked my multiple threads on the same
 * parent communicator.  In single-threaded case, lowest_context_id is always
 * set to parent context id in sched_cb_gcn_copy_mask and lowest_tag is not
 * used.
 */

int MPIR_Get_contextid_sparse(MPIR_Comm * comm_ptr, int *context_id, int ignore_id)
{
    return MPIR_Get_contextid_sparse_group(comm_ptr, NULL /*group_ptr */ ,
                                           MPIR_Process.attrs.tag_ub /*tag */ ,
                                           context_id, ignore_id);
}

struct gcn_state {
    int *ctx0;
    int *ctx1;
    int own_mask;
    int own_eager_mask;
    int iter;
    uint64_t tag;
    MPIR_Comm *comm_ptr;
    MPIR_Comm *comm_ptr_inter;
    MPIR_Request *req_ptr;
    MPIR_Comm *new_comm;
    MPIR_Group *group_ptr;
    bool ignore_id;
    bool is_inter_comm;
    bool gcn_in_list;
    int error;
    uint32_t local_mask[MPIR_MAX_CONTEXT_MASK + 1];
    struct gcn_state *next;
    enum {
        MPIR_GCN__COPYMASK,
        MPIR_GCN__ALLREDUCE,
        MPIR_GCN__INTERCOMM_SENDRECV,
        MPIR_GCN__INTERCOMM_BCAST,
    } stage;
    union {
        MPIR_Request *allreduce_request;
        MPIR_Request *sendrecv_reqs[2];
        MPIR_Request *bcast_request;
    } u;
};
struct gcn_state *next_gcn = NULL;

static void gcn_init(struct gcn_state *st, MPIR_Comm * comm_ptr, int tag,
                     bool ignore_id, MPIR_Group * group_ptr);
static void add_gcn_to_list(struct gcn_state *st);
static void remove_gcn_from_list(struct gcn_state *st);
static int gcn_copy_mask(struct gcn_state *st);
static int gcn_allocate_cid(struct gcn_state *st);
static int gcn_allreduce(struct gcn_state *st);
static int gcn_check_id_exhaustion(struct gcn_state *st);
static int gcn_intercomm_sendrecv(struct gcn_state *st);
static int gcn_intercomm_bcast(struct gcn_state *st);

static void gcn_init(struct gcn_state *st, MPIR_Comm * comm_ptr, int tag, bool ignore_id,
                     MPIR_Group * group_ptr)
{
    if (eager_nelem < 0) {
        /* Ensure that at least one word of deadlock-free context IDs is
         * always set aside for the base protocol */
        MPIR_Assert(MPIR_CVAR_CTXID_EAGER_SIZE >= 0 &&
                    MPIR_CVAR_CTXID_EAGER_SIZE < MPIR_MAX_CONTEXT_MASK - 1);
        eager_nelem = MPIR_CVAR_CTXID_EAGER_SIZE;
    }

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    st->iter = 0;
    st->comm_ptr = comm_ptr;
    st->tag = tag;
    st->ignore_id = ignore_id;
    st->group_ptr = group_ptr;
    st->own_mask = 0;
    st->own_eager_mask = 0;
    st->req_ptr = NULL;
    st->error = 0;
    st->gcn_in_list = false;
}

/* All pending context_id allocations are added to a list. The context_id allocations are ordered
 * according to the context_id of of parent communicator and the tag, wherby blocking context_id
 * allocations  can have the same tag, while nonblocking operations cannot. In the non-blocking
 * case, the user is responsible for the right tags if "comm_create_group" is used */
static void add_gcn_to_list(struct gcn_state *st)
{
    /* we'll only call this from gcn_allocate_cid within CS */
    struct gcn_state *tmp = NULL;
    if (next_gcn == NULL) {
        next_gcn = st;
        st->next = NULL;
    } else if (next_gcn->comm_ptr->context_id > st->comm_ptr->context_id ||
               (next_gcn->comm_ptr->context_id == st->comm_ptr->context_id &&
                next_gcn->tag > st->tag)) {
        st->next = next_gcn;
        next_gcn = st;
    } else {
        for (tmp = next_gcn;
             tmp->next != NULL &&
             ((st->comm_ptr->context_id > tmp->next->comm_ptr->context_id) ||
              ((st->comm_ptr->context_id == tmp->next->comm_ptr->context_id) &&
               (st->tag >= tmp->next->tag))); tmp = tmp->next);

        st->next = tmp->next;
        tmp->next = st;
    }
    st->gcn_in_list = true;
}

static void remove_gcn_from_list(struct gcn_state *st)
{
    /* we'll only call this from gcn_allocate_cid within CS */
    if (next_gcn == st) {
        next_gcn = st->next;
    } else {
        struct gcn_state *tmp;
        for (tmp = next_gcn; tmp->next != st; tmp = tmp->next);
        tmp->next = st->next;
    }
    st->gcn_in_list = false;
}

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
 *
 * If a group pointer is given, the call is _not_ sparse, and only processes
 * in the group should call this routine.  That is, it is collective only over
 * the given group.
 */
int MPIR_Get_contextid_sparse_group(MPIR_Comm * comm_ptr, MPIR_Group * group_ptr, int tag,
                                    int *context_id, int ignore_id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* Group-collective and ignore_id should never be combined */
    MPIR_Assert(!(group_ptr != NULL && ignore_id));

    struct gcn_state st;
    gcn_init(&st, comm_ptr, tag, ignore_id, group_ptr);

    *context_id = 0;

    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST,
                                             "Entering; shared state is %d:%d, my ctx id is %d, tag=%d",
                                             mask_in_use, eager_in_use, comm_ptr->context_id, tag));

    while (*context_id == 0) {
        mpi_errno = gcn_copy_mask(&st);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = gcn_allreduce(&st);
        MPIR_ERR_CHECK(mpi_errno);

        *context_id = gcn_allocate_cid(&st);

        if (*context_id == 0) {
            mpi_errno = st.error;
            MPIR_ERR_CHECK(mpi_errno);

            /* we did not find a context id. Give up the mask in case
             * there is another thread in the gcn_next_list
             * waiting for it.  We need to ensure that any other threads
             * have the opportunity to run, hence yielding */
            MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        }
    }

  fn_exit:
    if (ignore_id)
        *context_id = MPIR_INVALID_CONTEXT_ID;
    MPL_DBG_MSG_S(MPIR_DBG_COMM, VERBOSE, "Context mask = %s", context_mask_to_str());
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}



/* Try to find a valid context id.
 *
 * If the context id is found, then broadcast it; if not, then retry the
 * nonblocking context id allocation algorithm again.
 *
 * Note the subtle difference on thread handling between the nonblocking
 * algorithm (sched_cb_gcn_allocate_cid) and the blocking algorithm
 * (MPIR_Get_contextid_sparse_group). In nonblocking algorithm, there is no
 * need to yield to another thread because this thread will not block the
 * progress. On the contrary, unnecessary yield will allow other threads to
 * execute and insert wrong order of entries to the nonblocking schedule and
 * cause errors.
 */
static int gcn_allocate_cid(struct gcn_state *st)
{
    int context_id = 0;

    MPID_THREAD_CS_ENTER(VCI, MPIR_THREAD_VCI_CTX_MUTEX);

    context_id = locate_context_bit(st->local_mask);
    if (st->own_eager_mask) {
        st->own_eager_mask = 0;
        eager_in_use = 0;
    } else if (st->own_mask) {
        st->own_mask = 0;
        mask_in_use = 0;
    }

    if (context_id > 0) {
        if (!st->ignore_id) {
            int id = allocate_context_bit(context_mask, context_id);
            MPIR_Assertp(id == context_id);
        }
        if (st->gcn_in_list) {
            remove_gcn_from_list(st);
        }
    } else {
        st->error = gcn_check_id_exhaustion(st);
        if (!st->error && !st->gcn_in_list && !st->ignore_id) {
            add_gcn_to_list(st);
        } else if (st->error && st->gcn_in_list) {
            remove_gcn_from_list(st);
        }
    }
    MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_VCI_CTX_MUTEX);

    st->iter++;

    return context_id;
}

static int gcn_copy_mask(struct gcn_state *st)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_THREAD_CS_ENTER(VCI, MPIR_THREAD_VCI_CTX_MUTEX);

    if (st->ignore_id) {
        /* We are not participating in the resulting communicator, so our
         * context ID space doesn't matter.  Set the mask to "all available". */
        memset(st->local_mask, 0xff, MPIR_MAX_CONTEXT_MASK * sizeof(int));
        st->own_mask = 0;
        /* don't need to touch mask_in_use/lowest_context_id b/c our thread
         * doesn't ever need to "win" the mask */
    } else if (st->iter == 0) {
        /* Deadlock avoidance: Only participate in context id loop when all
         * processes have called this routine.  On the first iteration, use the
         * "eager" allocation protocol.
         */
        memset(st->local_mask, 0, (MPIR_MAX_CONTEXT_MASK + 1) * sizeof(int));
        st->own_eager_mask = 0;

        /* Attempt to reserve the eager mask segment */
        if (!eager_in_use && eager_nelem > 0) {
            int i;
            for (i = 0; i < eager_nelem; i++)
                st->local_mask[i] = context_mask[i];

            eager_in_use = 1;
            st->own_eager_mask = 1;
        }
    } else {
        MPIR_Assert(next_gcn != NULL);
        /*If we are here, at least one element must be in the list, at least myself */

        /* only the first element in the list can own the mask. However, maybe the mask is used
         * by another thread, which added another allocation to the list before. So we have to check,
         * if the mask is used and mark, if we own it */
        if (mask_in_use || st != next_gcn) {
            memset(st->local_mask, 0, MPIR_MAX_CONTEXT_MASK * sizeof(int));
            st->own_mask = 0;
        } else {
            /* Copy safe mask segment to local_mask */
            int i;
            for (i = 0; i < eager_nelem; i++)
                st->local_mask[i] = 0;
            for (i = eager_nelem; i < MPIR_MAX_CONTEXT_MASK; i++)
                st->local_mask[i] = context_mask[i];
            mask_in_use = 1;
            st->own_mask = 1;
        }
    }

    /* Note: MPIR_MAX_CONTEXT_MASK elements of local_mask are used by the
     * context ID allocation algorithm.  The additional element is ignored
     * by the context ID mask access routines and is used as a flag for
     * detecting context ID exhaustion (explained below). */
    if (st->own_mask || st->ignore_id)
        st->local_mask[ALL_OWN_MASK_FLAG] = 1;
    else
        st->local_mask[ALL_OWN_MASK_FLAG] = 0;

    MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_VCI_CTX_MUTEX);
    return mpi_errno;
}

static int gcn_check_id_exhaustion(struct gcn_state *st)
{
    int mpi_errno = MPI_SUCCESS;

    /* Test for context ID exhaustion: All threads that will participate in
     * the new communicator owned the mask and could not allocate a context
     * ID.  This indicates that either some process has no context IDs
     * available, or that some are available, but the allocation cannot
     * succeed because there is no common context ID. */
    if (st->local_mask[ALL_OWN_MASK_FLAG] == 1) {
        int nfree = 0;
        int ntotal = 0;
        int minfree;

        /* NOTE: we only call this with CS from gcn_allocate_cid, so it's safe */
        context_mask_stats(&nfree, &ntotal);

        if (st->ignore_id)
            minfree = INT_MAX;
        else
            minfree = nfree;

        if (!st->req_ptr) {
            /* only run Allreduce if we are in a blocking call */
            if (st->group_ptr != NULL) {
                int coll_tag = st->tag | MPIR_TAG_COLL_BIT;     /* Shift tag into the tagged coll space */
                mpi_errno = MPII_Allreduce_group(MPI_IN_PLACE, &minfree, 1, MPIR_INT_INTERNAL,
                                                 MPI_MIN, st->comm_ptr, st->group_ptr, coll_tag,
                                                 MPIR_COLL_ATTR_SYNC);
            } else {
                mpi_errno = MPIR_Allreduce_fallback(MPI_IN_PLACE, &minfree, 1, MPIR_INT_INTERNAL,
                                                    MPI_MIN, st->comm_ptr, MPIR_COLL_ATTR_SYNC);
            }
        }

        if (minfree > 0) {
            MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER,
                                 "**toomanycommfrag", "**toomanycommfrag %d %d %d",
                                 nfree, ntotal, st->ignore_id);
        } else {
            MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER,
                                 "**toomanycomm", "**toomanycomm %d %d %d",
                                 nfree, ntotal, st->ignore_id);
        }
    }

  fn_fail:
    return mpi_errno;
}

static int gcn_allreduce(struct gcn_state *st)
{
    int mpi_errno;

    int count = MPIR_MAX_CONTEXT_MASK + 1;
    MPI_Datatype datatype = MPIR_UINT32_T_INTERNAL;
    MPI_Op op = MPI_BAND;

    if (st->req_ptr) {
        mpi_errno = MPIR_Iallreduce_tag(MPI_IN_PLACE, st->local_mask, count, datatype, op,
                                        st->comm_ptr, st->tag, &st->u.allreduce_request);
    } else {
        if (st->group_ptr != NULL) {
            int coll_tag = st->tag | MPIR_TAG_COLL_BIT; /* Shift tag into the tagged coll space */
            mpi_errno = MPII_Allreduce_group(MPI_IN_PLACE, st->local_mask, count, datatype, op,
                                             st->comm_ptr, st->group_ptr,
                                             coll_tag, MPIR_COLL_ATTR_SYNC);
        } else {
            mpi_errno = MPIR_Allreduce_fallback(MPI_IN_PLACE, st->local_mask, count, datatype, op,
                                                st->comm_ptr, MPIR_COLL_ATTR_SYNC);
        }
    }

    return mpi_errno;
}

static int gcn_intercomm_sendrecv(struct gcn_state *st)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIC_Irecv(st->ctx1, 1, MPIR_CONTEXT_ID_T_DATATYPE,
                           0, st->tag, st->comm_ptr_inter, &st->u.sendrecv_reqs[0]);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIC_Isend(st->ctx0, 1, MPIR_CONTEXT_ID_T_DATATYPE,
                           0, st->tag, st->comm_ptr_inter, &st->u.sendrecv_reqs[1], 0);
    MPIR_ERR_CHECK(mpi_errno);

  fn_fail:
    return mpi_errno;
}

static int gcn_intercomm_bcast(struct gcn_state *st)
{
    return MPIR_Ibcast_tag(st->ctx1, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, st->comm_ptr,
                           st->tag, &st->u.bcast_request);
}

static int gcn_complete(struct gcn_state *st)
{
    int mpi_errno = MPI_SUCCESS;

    if (st->error) {
        /* In the case of failure, the new communicator was half created.
         * So we need to clean the memory allocated for it. */
        MPII_COMML_FORGET(st->new_comm);
        MPIR_Handle_obj_free(&MPIR_Comm_mem, st->new_comm);
        st->req_ptr->status.MPI_ERROR = st->error;
    } else {
        mpi_errno = MPIR_Comm_commit(st->new_comm);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_Grequest_complete_impl(st->req_ptr);

    MPL_free(st);

  fn_fail:
    return mpi_errno;
}

static int async_gcn_poll(MPIX_Async_thing thing)
{
    struct gcn_state *st = MPIR_Async_thing_get_state(thing);

    switch (st->stage) {
        case MPIR_GCN__COPYMASK:
            st->error = gcn_copy_mask(st);
            MPIR_Assert(st->error == MPI_SUCCESS);

            st->error = gcn_allreduce(st);
            MPIR_Assert(st->error == MPI_SUCCESS);

            st->stage = MPIR_GCN__ALLREDUCE;
            return MPIX_ASYNC_NOPROGRESS;

        case MPIR_GCN__ALLREDUCE:
            if (!MPIR_Request_is_complete(st->u.allreduce_request)) {
                return MPIX_ASYNC_NOPROGRESS;
            } else {
                MPIR_Request_free(st->u.allreduce_request);

                int newctxid = gcn_allocate_cid(st);
                if (newctxid) {
                    if (st->ctx0)
                        *st->ctx0 = newctxid;
                    if (st->ctx1)
                        *st->ctx1 = newctxid;
                }

                if (st->error) {
                    gcn_complete(st);
                    return MPIX_ASYNC_DONE;
                } else if (*st->ctx0 == 0) {
                    /* retry */
                    st->stage = MPIR_GCN__COPYMASK;
                    return MPIX_ASYNC_NOPROGRESS;
                } else if (!st->is_inter_comm) {
                    /* done */
                    gcn_complete(st);
                    return MPIX_ASYNC_DONE;
                } else if (st->comm_ptr_inter->rank == 0) {
                    st->error = gcn_intercomm_sendrecv(st);
                    MPIR_Assert(st->error == MPI_SUCCESS);

                    st->stage = MPIR_GCN__INTERCOMM_SENDRECV;
                    return MPIX_ASYNC_NOPROGRESS;
                } else {
                    st->error = gcn_intercomm_bcast(st);
                    MPIR_Assert(st->error == MPI_SUCCESS);

                    st->stage = MPIR_GCN__INTERCOMM_BCAST;
                    return MPIX_ASYNC_NOPROGRESS;
                }
            }

        case MPIR_GCN__INTERCOMM_SENDRECV:
            if (!MPIR_Request_is_complete(st->u.sendrecv_reqs[0]) ||
                !MPIR_Request_is_complete(st->u.sendrecv_reqs[1])) {
                return MPIX_ASYNC_NOPROGRESS;
            } else {
                MPIR_Request_free(st->u.sendrecv_reqs[0]);
                MPIR_Request_free(st->u.sendrecv_reqs[1]);

                st->error = gcn_intercomm_bcast(st);
                MPIR_Assert(st->error == MPI_SUCCESS);

                st->stage = MPIR_GCN__INTERCOMM_BCAST;
                return MPIX_ASYNC_NOPROGRESS;
            }

        case MPIR_GCN__INTERCOMM_BCAST:
            if (!MPIR_Request_is_complete(st->u.bcast_request)) {
                return MPIX_ASYNC_NOPROGRESS;
            } else {
                MPIR_Request_free(st->u.bcast_request);

                gcn_complete(st);
                return MPIX_ASYNC_DONE;
            }
    }

    MPIR_Assert(0);
    return MPIX_ASYNC_NOPROGRESS;
}

static int query_fn(void *extra_state, MPI_Status * status)
{
    /* status points to request->status */
    return status->MPI_ERROR;
}

static int free_fn(void *extra_state)
{
    return MPI_SUCCESS;
}

static int cancel_fn(void *extra_state, int complete)
{
    return MPI_SUCCESS;
}

/** Allocating a new context ID collectively over the given communicator in a
 * nonblocking way.
 *
 * The nonblocking mechanism is implemented by inserting MPIDU_Sched_entry to
 * the nonblocking collective progress, which is a part of the progress engine.
 * It uses a two-level linked list 'all_schedules' to manager all nonblocking
 * collective calls: the first level is a linked list of struct MPIDU_Sched;
 * and each struct MPIDU_Sched is an array of struct MPIDU_Sched_entry. The
 * following four functions are used together to implement the algorithm:
 * sched_cb_gcn_copy_mask, sched_cb_gcn_allocate_cid, sched_cb_gcn_bcast and
 * sched_get_cid_nonblock.
 *
 * The above four functions use the same algorithm as
 * MPIR_Get_contextid_sparse_group (multi-threaded version) to allocate a
 * context id. The algorithm needs to retry the allocation process in the case
 * of conflicts. In MPIR_Get_contextid_sparse_group, it is a while loop.  In
 * the nonblocking algorithm, 1) new entries are inserted to the end of
 * schedule to replace the 'while' loop in MPI_Comm_dup algorithm; 2) all
 * arguments passed to sched_get_cid_nonblock are saved to gcn_state in order
 * to be called in the future; 3) in sched_cb_gcn_allocate_cid, if the first
 * try failed, it will insert sched_cb_gcn_copy_mask to the schedule again.
 *
 * To ensure thread-safety, it shares the same global flag 'mask_in_use' with
 * other communicator functions to protect access to context_mask. And use
 * CONTEXTID lock to protect critical sections.
 *
 * There is a subtle difference between INTRACOMM and INTERCOMM when
 * duplicating a communicator.  They needed to be treated differently in
 * current algorithm. Specifically, 1) when calling sched_get_cid_nonblock, the
 * parameters are different; 2) updating newcommp->recvcontext_id in
 * MPIR_Get_intercomm_contextid_nonblock has been moved to sched_cb_gcn_bcast
 * because this should happen after sched_cb_gcn_allocate_cid has succeed.
 *
 * To avoid deadlock or livelock, it uses the same eager protocol as
 * multi-threaded MPIR_Get_contextid_sparse_group.
 */
static int async_get_cid(MPIR_Comm * comm_ptr, MPIR_Comm * newcommp, MPIR_Comm * comm_inter,
                         MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKPMEM_DECL();

    mpi_errno = MPIR_Grequest_start_impl(query_fn, free_fn, cancel_fn, NULL, req);
    MPIR_ERR_CHECK(mpi_errno);

    struct gcn_state *st = NULL;
    MPIR_CHKPMEM_MALLOC(st, sizeof(struct gcn_state), MPL_MEM_COMM);

    int tag;
    if (comm_inter) {
        /* We need get the tag from the parent intercomm because the localcomm (comm_ptr) shares
         * the same context_id as the parent. */
        mpi_errno = MPIR_Sched_next_tag(comm_inter, &tag);
    } else {
        mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    }
    MPIR_ERR_CHECK(mpi_errno);

    gcn_init(st, comm_ptr, tag, false /* ignore_id */ , NULL /* group_ptr */);

    st->new_comm = newcommp;
    st->comm_ptr_inter = comm_inter;
    st->ctx0 = &newcommp->recvcontext_id;
    st->ctx1 = &newcommp->context_id;
    if (comm_inter) {
        st->is_inter_comm = true;
    } else {
        st->is_inter_comm = false;
    }

    st->req_ptr = *req;

    *(st->ctx0) = 0;
    st->stage = MPIR_GCN__COPYMASK;

    mpi_errno = MPIR_Async_things_add(async_gcn_poll, st, NULL);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIR_Get_contextid_nonblock(MPIR_Comm * comm_ptr, MPIR_Comm * newcommp, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = async_get_cid(comm_ptr, newcommp, NULL /* comm_inter */ , req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Get_intercomm_contextid_nonblock(MPIR_Comm * comm_ptr, MPIR_Comm * newcommp,
                                          MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* do as much local setup as possible */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = async_get_cid(comm_ptr->local_comm, newcommp, comm_ptr, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_fail:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}


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
int MPIR_Get_intercomm_contextid(MPIR_Comm * comm_ptr, int *context_id, int *recvcontext_id)
{
    int mycontext_id, remote_context_id;
    int mpi_errno = MPI_SUCCESS;
    int tag = 31567;            /* FIXME  - we need an internal tag or
                                 * communication channel.  Can we use a different
                                 * context instead?.  Or can we use the tag
                                 * provided in the intercomm routine? (not on a dup,
                                 * but in that case it can use the collective context) */
    MPIR_FUNC_ENTER;

    if (!comm_ptr->local_comm) {
        /* Manufacture the local communicator */
        mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIR_Get_contextid_sparse(comm_ptr->local_comm, &mycontext_id, FALSE);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(mycontext_id != 0);

    /* MPIC routine uses an internal context id.  The local leads (process 0)
     * exchange data */
    remote_context_id = MPIR_INVALID_CONTEXT_ID;
    if (comm_ptr->rank == 0) {
        mpi_errno = MPIC_Sendrecv(&mycontext_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, tag,
                                  &remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE, 0, tag,
                                  comm_ptr, MPI_STATUS_IGNORE, MPIR_COLL_ATTR_SYNC);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Make sure that all of the local processes now have this
     * id */
    mpi_errno = MPIR_Bcast_fallback(&remote_context_id, 1, MPIR_CONTEXT_ID_T_DATATYPE,
                                    0, comm_ptr->local_comm, MPIR_COLL_ATTR_SYNC);
    MPIR_ERR_CHECK(mpi_errno);
    /* The recvcontext_id must be the one that was allocated out of the local
     * group, not the remote group.  Otherwise we could end up posting two
     * MPI_ANY_SOURCE,MPI_ANY_TAG recvs on the same context IDs even though we
     * are attempting to post them for two separate communicators. */
    *context_id = remote_context_id;
    *recvcontext_id = mycontext_id;
  fn_fail:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

void MPIR_Free_contextid(int context_id)
{
    int idx, bitpos, raw_prefix;

    MPIR_FUNC_ENTER;

    /* Convert the context id to the bit position */
    raw_prefix = MPIR_CONTEXT_READ_FIELD(PREFIX, context_id);
    idx = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;

    /* --BEGIN ERROR HANDLING-- */
    if (idx < 0 || idx >= MPIR_MAX_CONTEXT_MASK) {
        MPID_Abort(0, MPI_ERR_INTERN, 1, "In MPIR_Free_contextid, idx is out of range");
    }
    /* --END ERROR HANDLING-- */

    /* The low order bits for dynamic context IDs don't have meaning the
     * same way that low bits of non-dynamic ctx IDs do.  So we have to
     * check the dynamic case first. */
    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, context_id)) {
        MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "skipping dynamic process ctx id, context_id=%d",
                      context_id);
        goto fn_exit;
    } else {    /* non-dynamic context ID */
        /* In terms of the context ID bit vector, intercomms and their constituent
         * localcomms have the same value.  To avoid a double-free situation we just
         * don't free the context ID for localcomms and assume it will be cleaned up
         * when the parent intercomm is itself completely freed. */
        if (MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id)) {
#ifdef MPL_USE_DBG_LOGGING
            char dump_str[1024];
            dump_context_id(context_id, dump_str, sizeof(dump_str));
            MPL_DBG_MSG_S(MPIR_DBG_COMM, VERBOSE, "skipping localcomm id: %s", dump_str);
#endif
            goto fn_exit;
        } else if (MPIR_CONTEXT_READ_FIELD(SUBCOMM, context_id)) {
            MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE,
                          "skipping non-parent communicator ctx id, context_id=%d", context_id);
            goto fn_exit;
        }
    }

    MPID_THREAD_CS_ENTER(VCI, MPIR_THREAD_VCI_CTX_MUTEX);

    /* --BEGIN ERROR HANDLING-- */
    /* Check that this context id has been allocated */
    if ((context_mask[idx] & (0x1U << bitpos)) != 0) {
#ifdef MPL_USE_DBG_LOGGING
        char dump_str[1024];
        dump_context_id(context_id, dump_str, sizeof(dump_str));
        MPL_DBG_MSG_S(MPIR_DBG_COMM, VERBOSE, "context dump: %s", dump_str);
        MPL_DBG_MSG_S(MPIR_DBG_COMM, VERBOSE, "context mask = %s", context_mask_to_str());
#endif
        MPID_Abort(0, MPI_ERR_INTERN, 1, "In MPIR_Free_contextid, the context id is not in use");
    }
    /* --END ERROR HANDLING-- */

    /* MT: Note that this update must be done atomically in the multithreaedd
     * case.  In the "one, single lock" implementation, that lock is indeed
     * held when this operation is called. */
    context_mask[idx] |= (0x1U << bitpos);
    MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_VCI_CTX_MUTEX);

    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                    (MPL_DBG_FDEST,
                     "Freed context %d, mask[%d] bit %d (prefix=%#x)",
                     context_id, idx, bitpos, raw_prefix));
  fn_exit:
    MPIR_FUNC_EXIT;
}
