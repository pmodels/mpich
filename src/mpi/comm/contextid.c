/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"
#include "mpiinfo.h"    /* MPIU_Info_free */

#include "mpl_utlist.h"
#include "mpiu_uthash.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CTXID_EAGER_SIZE
      category    : THREADS
      type        : int
      default     : 2
      class       : device
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
static int initialize_context_mask = 1;
const int ALL_OWN_MASK_FLAG = MPIR_MAX_CONTEXT_MASK;

/* utility function to pretty print a context ID for debugging purposes, see
 * mpiimpl.h for more info on the various fields */
#ifdef USE_DBG_LOGGING
static void dump_context_id(MPIU_Context_id_t context_id, char *out_str, int len)
{
    int subcomm_type = MPID_CONTEXT_READ_FIELD(SUBCOMM, context_id);
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
        MPIU_Assert(FALSE);
        break;
    }
    MPL_snprintf(out_str, len,
                  "context_id=%d (%#x): DYNAMIC_PROC=%d PREFIX=%#x IS_LOCALCOMM=%d SUBCOMM=%s SUFFIX=%s",
                  context_id,
                  context_id,
                  MPID_CONTEXT_READ_FIELD(DYNAMIC_PROC, context_id),
                  MPID_CONTEXT_READ_FIELD(PREFIX, context_id),
                  MPID_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id),
                  subcomm_type_name,
                  (MPID_CONTEXT_READ_FIELD(SUFFIX, context_id) ? "coll" : "pt2pt"));
}
#endif

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
        MPL_snprintf(&bufstr[i * 8], 9, "%.8x", context_mask[i]);
    }
    return bufstr;
}

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
        int i, j;
        *free_ids = 0;

        /* if this ever needs to be fast, use a lookup table to do a per-nibble
         * or per-byte lookup of the popcount instead of checking each bit at a
         * time (or just track the count when manipulating the mask and keep
         * that count stored in a variable) */
        for (i = 0; i < MPIR_MAX_CONTEXT_MASK; ++i) {
            for (j = 0; j < sizeof(context_mask[0]) * 8; ++j) {
                *free_ids += (context_mask[i] & (0x1 << j)) >> j;
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

static void context_id_init(void)
{
    int i;

    for (i = 1; i < MPIR_MAX_CONTEXT_MASK; i++) {
        context_mask[i] = 0xFFFFFFFF;
    }
    /* The first two values are already used (comm_world, comm_self).
     * The third value is also used for the internal-only copy of
     * comm_world, if needed by mpid. */
#ifdef MPID_NEEDS_ICOMM_WORLD
    context_mask[0] = 0xFFFFFFF8;
#else
    context_mask[0] = 0xFFFFFFFC;
#endif
    initialize_context_mask = 0;

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
            context_id = (MPIR_CONTEXT_INT_BITS * i + j) << MPID_CONTEXT_PREFIX_SHIFT;
            return context_id;
        }
    }
    return 0;
}

/* Allocates a context ID from the given mask by clearing the bit
 * corresponding to the the given id.  Returns 0 on failure, id on
 * success. */
static int allocate_context_bit(uint32_t mask[], MPIU_Context_id_t id)
{
    int raw_prefix, idx, bitpos;
    raw_prefix = MPID_CONTEXT_READ_FIELD(PREFIX, id);
    idx = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;

    /* the bit should not already be cleared (allocated) */
    MPIU_Assert(mask[idx] & (1 << bitpos));

    /* clear the bit */
    mask[idx] &= ~(1 << bitpos);

    MPIU_DBG_MSG_FMT(COMM, VERBOSE, (MPIU_DBG_FDEST,
                                     "allocating contextid = %d, (mask=%p, mask[%d], bit %d)",
                                     id, mask, idx, bitpos));
    return id;
}

/* Allocates the first available context ID from context_mask based on the available
 * bits given in local_mask.  This function will clear the corresponding bit in
 * context_mask if allocation was successful.
 *
 * Returns 0 on failure.  Returns the allocated context ID on success. */
static int find_and_allocate_context_id(uint32_t local_mask[])
{
    MPIU_Context_id_t context_id;
    context_id = locate_context_bit(local_mask);
    if (context_id != 0) {
        context_id = allocate_context_bit(context_mask, context_id);
    }
    return context_id;
}

/* EAGER CONTEXT ID ALLOCATION: Attempt to allocate the context ID during the
 * initial synchronization step.  If eager protocol fails, threads fall back to
 * the base algorithm.
 *
 * They are used to avoid deadlock in multi-threaded case. In single-threaded
 * case, they are not used.
 */
static volatile int eager_nelem = -1;
static volatile int eager_in_use = 0;

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

#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid_sparse
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Get_contextid_sparse(MPID_Comm * comm_ptr, MPIU_Context_id_t * context_id, int ignore_id)
{
    return MPIR_Get_contextid_sparse_group(comm_ptr, NULL /*group_ptr */ ,
                                           MPIR_Process.attrs.tag_ub /*tag */ ,
                                           context_id, ignore_id);
}

struct gcn_state {
    MPIU_Context_id_t *ctx0;
    MPIU_Context_id_t *ctx1;
    int own_mask;
    int own_eager_mask;
    int first_iter;
    uint64_t tag;
    MPID_Comm *comm_ptr;
    MPID_Comm *comm_ptr_inter;
    MPID_Sched_t s;
    MPID_Comm *new_comm;
    MPID_Comm_kind_t gcn_cid_kind;
    uint32_t local_mask[MPIR_MAX_CONTEXT_MASK + 1];
    struct gcn_state *next;
};
struct gcn_state *next_gcn = NULL;

/* All pending context_id allocations are added to a list. The context_id allocations are ordered
 * according to the context_id of of parrent communicator and the tag, wherby blocking context_id
 * allocations  can have the same tag, while nonblocking operations cannot. In the non-blocking
 * case, the user is reponsible for the right tags if "comm_create_group" is used */
#undef FUNCNAME
#define FUNCNAME add_gcn_to_list
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int add_gcn_to_list(struct gcn_state *new_state)
{
    int mpi_errno = 0;
    struct gcn_state *tmp = NULL;
    if (next_gcn == NULL) {
        next_gcn = new_state;
        new_state->next = NULL;
    }
    else if (next_gcn->comm_ptr->context_id > new_state->comm_ptr->context_id ||
             (next_gcn->comm_ptr->context_id == new_state->comm_ptr->context_id &&
              next_gcn->tag > new_state->tag)) {
        new_state->next = next_gcn;
        next_gcn = new_state;
    }
    else {
        for (tmp = next_gcn;
             tmp->next != NULL &&
             ((new_state->comm_ptr->context_id > tmp->next->comm_ptr->context_id) ||
              ((new_state->comm_ptr->context_id == tmp->next->comm_ptr->context_id) &&
               (new_state->tag >= tmp->next->tag))); tmp = tmp->next);

        new_state->next = tmp->next;
        tmp->next = new_state;

    }
    return mpi_errno;
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
 * (*context_id==MPIU_INVALID_CONTEXT_ID) and should not attempt to use it.
 *
 * If a group pointer is given, the call is _not_ sparse, and only processes
 * in the group should call this routine.  That is, it is collective only over
 * the given group.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid_sparse_group
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Get_contextid_sparse_group(MPID_Comm * comm_ptr, MPID_Group * group_ptr, int tag,
                                    MPIU_Context_id_t * context_id, int ignore_id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    struct gcn_state st;
    struct gcn_state *tmp;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_CONTEXTID);

    st.first_iter = 1;
    st.comm_ptr = comm_ptr;
    st.tag = tag;
    st.own_mask = 0;
    st.own_eager_mask = 0;
    /* Group-collective and ignore_id should never be combined */
    MPIU_Assert(!(group_ptr != NULL && ignore_id));

    *context_id = 0;

    MPIU_DBG_MSG_FMT(COMM, VERBOSE, (MPIU_DBG_FDEST,
                                     "Entering; shared state is %d:%d, my ctx id is %d, tag=%d",
                                     mask_in_use, eager_in_use, comm_ptr->context_id, tag));

    while (*context_id == 0) {
        /* We lock only around access to the mask (except in the global locking
         * case).  If another thread is using the mask, we take a mask of zero. */
        MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_CTX_MUTEX);

        if (initialize_context_mask) {
            context_id_init();
        }

        if (eager_nelem < 0) {
            /* Ensure that at least one word of deadlock-free context IDs is
             * always set aside for the base protocol */
            MPIU_Assert(MPIR_CVAR_CTXID_EAGER_SIZE >= 0 &&
                        MPIR_CVAR_CTXID_EAGER_SIZE < MPIR_MAX_CONTEXT_MASK - 1);
            eager_nelem = MPIR_CVAR_CTXID_EAGER_SIZE;
        }

        if (ignore_id) {
            /* We are not participating in the resulting communicator, so our
             * context ID space doesn't matter.  Set the mask to "all available". */
            memset(st.local_mask, 0xff, MPIR_MAX_CONTEXT_MASK * sizeof(int));
            st.own_mask = 0;
            /* don't need to touch mask_in_use/lowest_context_id b/c our thread
             * doesn't ever need to "win" the mask */
        }

        /* Deadlock avoidance: Only participate in context id loop when all
         * processes have called this routine.  On the first iteration, use the
         * "eager" allocation protocol.
         */
        else if (st.first_iter) {
            memset(st.local_mask, 0, MPIR_MAX_CONTEXT_MASK * sizeof(int));
            st.own_eager_mask = 0;
            /* Attempt to reserve the eager mask segment */
            if (!eager_in_use && eager_nelem > 0) {
                int i;
                for (i = 0; i < eager_nelem; i++)
                    st.local_mask[i] = context_mask[i];

                eager_in_use = 1;
                st.own_eager_mask = 1;
            }
        }

        else {
            MPIU_Assert(next_gcn != NULL);
            /*If we are here, at least one element must be in the list, at least myself */

            /* only the first element in the list can own the mask. However, maybe the mask is used
             * by another thread, which added another allcoation to the list bevore. So we have to check,
             * if the mask is used and mark, if we own it */
            if (mask_in_use || &st != next_gcn) {
                memset(st.local_mask, 0, MPIR_MAX_CONTEXT_MASK * sizeof(int));
                st.own_mask = 0;
                MPIU_DBG_MSG_FMT(COMM, VERBOSE, (MPIU_DBG_FDEST,
                                                 "Mask is in use, my context_id is %d, owner context id is %d",
                                                 st.comm_ptr->context_id,
                                                 next_gcn->comm_ptr->context_id));
            }
            else {
                int i;
                /* Copy safe mask segment to local_mask */
                for (i = 0; i < eager_nelem; i++)
                    st.local_mask[i] = 0;
                for (i = eager_nelem; i < MPIR_MAX_CONTEXT_MASK; i++)
                    st.local_mask[i] = context_mask[i];

                mask_in_use = 1;
                st.own_mask = 1;
                MPIU_DBG_MSG(COMM, VERBOSE, "Copied local_mask");
            }
        }
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_CTX_MUTEX);

        /* Note: MPIR_MAX_CONTEXT_MASK elements of local_mask are used by the
         * context ID allocation algorithm.  The additional element is ignored
         * by the context ID mask access routines and is used as a flag for
         * detecting context ID exhaustion (explained below). */
        if (st.own_mask || ignore_id)
            st.local_mask[ALL_OWN_MASK_FLAG] = 1;
        else
            st.local_mask[ALL_OWN_MASK_FLAG] = 0;

        /* Now, try to get a context id */
        MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);
        /* In the global and brief-global cases, note that this routine will
         * release that global lock when it needs to wait.  That will allow
         * other processes to enter the global or brief global critical section.
         */
        if (group_ptr != NULL) {
            int coll_tag = tag | MPIR_Process.tagged_coll_mask; /* Shift tag into the tagged coll space */
            mpi_errno = MPIR_Allreduce_group(MPI_IN_PLACE, st.local_mask, MPIR_MAX_CONTEXT_MASK + 1,
                                             MPI_INT, MPI_BAND, comm_ptr, group_ptr, coll_tag,
                                             &errflag);
        }
        else {
            mpi_errno = MPIR_Allreduce_impl(MPI_IN_PLACE, st.local_mask, MPIR_MAX_CONTEXT_MASK + 1,
                                            MPI_INT, MPI_BAND, comm_ptr, &errflag);
        }
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* MT FIXME 2/3 cases don't seem to need the CONTEXTID CS, check and
         * narrow this region */
        MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_CTX_MUTEX);
        if (ignore_id) {
            /* we don't care what the value was, but make sure that everyone
             * who did care agreed on a value */
            *context_id = locate_context_bit(st.local_mask);
            /* used later in out-of-context ids check and outer while loop condition */
        }
        else if (st.own_eager_mask) {
            /* There is a chance that we've found a context id */
            /* Find_and_allocate_context_id updates the context_mask if it finds a match */
            *context_id = find_and_allocate_context_id(st.local_mask);
            MPIU_DBG_MSG_D(COMM, VERBOSE, "Context id is now %hd", *context_id);

            st.own_eager_mask = 0;
            eager_in_use = 0;
            if (*context_id <= 0) {
                /* else we did not find a context id. Give up the mask in case
                 * there is another thread (with a lower input context id)
                 * waiting for it.  We need to ensure that any other threads
                 * have the opportunity to run, hence yielding */
                /* FIXME: Do we need to do an GLOBAL yield here?
                 * When we do a collective operation, we anyway yield
                 * for other others */
                MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
                MPID_THREAD_CS_YIELD(POBJ, MPIR_THREAD_POBJ_CTX_MUTEX);
            }
        }
        else if (st.own_mask) {
            /* There is a chance that we've found a context id */
            /* Find_and_allocate_context_id updates the context_mask if it finds a match */
            *context_id = find_and_allocate_context_id(st.local_mask);
            MPIU_DBG_MSG_D(COMM, VERBOSE, "Context id is now %hd", *context_id);

            mask_in_use = 0;

            if (*context_id > 0) {
                /* If we found a new context id, we have to remove the element from the list, so the
                 * next allocation can own the mask */
                if (next_gcn == &st) {
                    next_gcn = st.next;
                }
                else {
                    for (tmp = next_gcn; tmp->next != &st; tmp = tmp->next);    /* avoid compiler warnings */
                    tmp->next = st.next;
                }
            }
            else {
                /* else we did not find a context id. Give up the mask in case
                 * there is another thread in the gcn_next_list
                 * waiting for it.  We need to ensure that any other threads
                 * have the opportunity to run, hence yielding */
                /* FIXME: Do we need to do an GLOBAL yield here?
                 * When we do a collective operation, we anyway yield
                 * for other others */
                MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
                MPID_THREAD_CS_YIELD(POBJ, MPIR_THREAD_POBJ_CTX_MUTEX);
            }
        }
        else {
            /* As above, force this thread to yield */
            /* FIXME: Do we need to do an GLOBAL yield here?  When we
             * do a collective operation, we anyway yield for other
             * others */
            MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
            MPID_THREAD_CS_YIELD(POBJ, MPIR_THREAD_POBJ_CTX_MUTEX);
        }
        MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_CTX_MUTEX);

        /* Test for context ID exhaustion: All threads that will participate in
         * the new communicator owned the mask and could not allocate a context
         * ID.  This indicates that either some process has no context IDs
         * available, or that some are available, but the allocation cannot
         * succeed because there is no common context ID. */
        if (*context_id == 0 && st.local_mask[ALL_OWN_MASK_FLAG] == 1) {
            /* --BEGIN ERROR HANDLING-- */
            int nfree = 0;
            int ntotal = 0;
            int minfree;

            if (st.own_mask) {
                MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_CTX_MUTEX);
                mask_in_use = 0;
                MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_CTX_MUTEX);
            }

            context_mask_stats(&nfree, &ntotal);
            if (ignore_id)
                minfree = INT_MAX;
            else
                minfree = nfree;

            if (group_ptr != NULL) {
                int coll_tag = tag | MPIR_Process.tagged_coll_mask;     /* Shift tag into the tagged coll space */
                mpi_errno = MPIR_Allreduce_group(MPI_IN_PLACE, &minfree, 1, MPI_INT, MPI_MIN,
                                                 comm_ptr, group_ptr, coll_tag, &errflag);
            }
            else {
                mpi_errno = MPIR_Allreduce_impl(MPI_IN_PLACE, &minfree, 1, MPI_INT,
                                                MPI_MIN, comm_ptr, &errflag);
            }

            if (minfree > 0) {
                MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER,
                                     "**toomanycommfrag", "**toomanycommfrag %d %d %d",
                                     nfree, ntotal, ignore_id);
            }
            else {
                MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER,
                                     "**toomanycomm", "**toomanycomm %d %d %d",
                                     nfree, ntotal, ignore_id);
            }
            /* --END ERROR HANDLING-- */
        }
        if (st.first_iter == 1) {
            st.first_iter = 0;
            /* to avoid deadlocks, the element is not added to the list bevore the first iteration */
            if (!ignore_id && *context_id == 0)
                add_gcn_to_list(&st);
        }
    }

  fn_exit:
    if (ignore_id)
        *context_id = MPIU_INVALID_CONTEXT_ID;
    MPIU_DBG_MSG_S(COMM, VERBOSE, "Context mask = %s", context_mask_to_str());
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_CONTEXTID);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    /* Release the masks */
    if (st.own_mask) {
        /* is it safe to access this without holding the CS? */
        mask_in_use = 0;
    }
    /*If in list, remove it */
    if (!st.first_iter && !ignore_id) {
        if (next_gcn == &st) {
            next_gcn = st.next;
        }
        else {
            for (tmp = next_gcn; tmp->next != &st; tmp = tmp->next);
            tmp->next = st.next;
        }
    }


    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



static int sched_cb_gcn_copy_mask(MPID_Comm * comm, int tag, void *state);
static int sched_cb_gcn_allocate_cid(MPID_Comm * comm, int tag, void *state);
static int sched_cb_gcn_bcast(MPID_Comm * comm, int tag, void *state);
#undef FUNCNAME
#define FUNCNAME sched_cb_commit_comm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int sched_cb_commit_comm(MPID_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    struct gcn_state *st = state;

    mpi_errno = MPIR_Comm_commit(st->new_comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_fail:
    return mpi_errno;

}

#undef FUNCNAME
#define FUNCNAME sched_cb_gcn_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int sched_cb_gcn_bcast(MPID_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    struct gcn_state *st = state;

    if (st->gcn_cid_kind == MPID_INTERCOMM) {
        if (st->comm_ptr_inter->rank == 0) {
            mpi_errno =
                MPID_Sched_recv(st->ctx1, 1, MPIU_CONTEXT_ID_T_DATATYPE, 0, st->comm_ptr_inter,
                                st->s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            mpi_errno =
                MPID_Sched_send(st->ctx0, 1, MPIU_CONTEXT_ID_T_DATATYPE, 0, st->comm_ptr_inter,
                                st->s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(st->s);
        }

        mpi_errno = st->comm_ptr->coll_fns->Ibcast_sched(st->ctx1, 1,
                                                         MPIU_CONTEXT_ID_T_DATATYPE, 0,
                                                         st->comm_ptr, st->s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPID_SCHED_BARRIER(st->s);
    }

    mpi_errno = MPID_Sched_cb(&sched_cb_commit_comm, st, st->s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPID_Sched_cb(&MPIR_Sched_cb_free_buf, st, st->s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_fail:
    return mpi_errno;
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
#undef FUNCNAME
#define FUNCNAME sched_cb_gcn_allocate_cid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int sched_cb_gcn_allocate_cid(MPID_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    struct gcn_state *st = state, *tmp;
    MPIU_Context_id_t newctxid;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    if (st->own_eager_mask) {
        newctxid = find_and_allocate_context_id(st->local_mask);
        if (st->ctx0)
            *st->ctx0 = newctxid;
        if (st->ctx1)
            *st->ctx1 = newctxid;

        st->own_eager_mask = 0;
        eager_in_use = 0;
    }
    else if (st->own_mask) {
        newctxid = find_and_allocate_context_id(st->local_mask);
        if (st->ctx0)
            *st->ctx0 = newctxid;
        if (st->ctx1)
            *st->ctx1 = newctxid;

        /* reset flag for the next try */
        mask_in_use = 0;
        /* If we found a ctx, remove element form list */
        if (newctxid > 0) {
            if (next_gcn == st) {
                next_gcn = st->next;
            }
            else {
                for (tmp = next_gcn; tmp->next != st; tmp = tmp->next);
                tmp->next = st->next;
            }
        }
    }

    if (*st->ctx0 == 0) {
        if (st->local_mask[ALL_OWN_MASK_FLAG] == 1) {
            /* --BEGIN ERROR HANDLING-- */
            int nfree = 0;
            int ntotal = 0;
            int minfree;
            context_mask_stats(&nfree, &ntotal);
            minfree = nfree;
            MPIR_Allreduce_impl(MPI_IN_PLACE, &minfree, 1, MPI_INT,
                                MPI_MIN, st->comm_ptr, &errflag);
            if (minfree > 0) {
                MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER,
                                     "**toomanycommfrag", "**toomanycommfrag %d %d %d",
                                     nfree, ntotal, minfree);
            }
            else {
                MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER,
                                     "**toomanycomm", "**toomanycomm %d %d %d",
                                     nfree, ntotal, minfree);
            }
            /* --END ERROR HANDLING-- */
        }
        else {
            /* do not own mask, try again */
            if (st->first_iter == 1) {
                st->first_iter = 0;
                /* Set the Tag for the idup-operations. We have two problems here:
                 *  1.) The tag should not be used by another (blocking) context_id allocation.
                 *      Therefore, we set tag_up as lower bound for the operation. tag_ub is used by
                 *      most of the other blocking operations, but tag is always >0, so this
                 *      should be fine.
                 *  2.) We need odering between multiple idup operations on the same communicator.
                 *       The problem here is that the iallreduce operations of the first iteration
                 *       are not necessarily completed in the same order as they are issued, also on the
                 *       same communicator. To avoid deadlocks, we cannot add the elements to the
                 *       list bevfore the first iallreduce is completed. The "tag" is created for the
                 *       scheduling - by calling  MPID_Sched_next_tag(comm_ptr, &tag) - and the same
                 *       for a idup operation on all processes. So we use it here. */
                /* FIXME I'm not sure if there can be an overflows for this tag */
                st->tag = (uint64_t) tag + MPIR_Process.attrs.tag_ub;
                add_gcn_to_list(st);
            }
            mpi_errno = MPID_Sched_cb(&sched_cb_gcn_copy_mask, st, st->s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(st->s);
        }
    }
    else {
        /* Successfully allocated a context id */
        mpi_errno = MPID_Sched_cb(&sched_cb_gcn_bcast, st, st->s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPID_SCHED_BARRIER(st->s);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    /* make sure that the pending allocations are scheduled */
    if (!st->first_iter) {
        if (next_gcn == st) {
            next_gcn = st->next;
        }
        else {
            for (tmp = next_gcn; tmp && tmp->next != st; tmp = tmp->next);
            tmp->next = st->next;
        }
    }
    /* In the case of failure, the new communicator was half created.
     * So we need to clean the memory allocated for it. */
    MPIR_Comm_map_free(st->new_comm);
    MPIU_Handle_obj_free(&MPID_Comm_mem, st->new_comm);
    MPIU_Free(st);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME sched_cb_gcn_copy_mask
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int sched_cb_gcn_copy_mask(MPID_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    struct gcn_state *st = state;

    if (st->first_iter) {
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
    }
    else {
        /* Same rules as for the blocking case */
        if (mask_in_use || st != next_gcn) {
            memset(st->local_mask, 0, MPIR_MAX_CONTEXT_MASK * sizeof(int));
            st->own_mask = 0;
            st->local_mask[ALL_OWN_MASK_FLAG] = 0;
        }
        else {
            /* Copy safe mask segment to local_mask */
            int i;
            for (i = 0; i < eager_nelem; i++)
                st->local_mask[i] = 0;
            for (i = eager_nelem; i < MPIR_MAX_CONTEXT_MASK; i++)
                st->local_mask[i] = context_mask[i];
            mask_in_use = 1;
            st->own_mask = 1;
            st->local_mask[ALL_OWN_MASK_FLAG] = 1;
        }
    }

    mpi_errno =
        st->comm_ptr->coll_fns->Iallreduce_sched(MPI_IN_PLACE, st->local_mask,
                                                 MPIR_MAX_CONTEXT_MASK + 1, MPI_UINT32_T, MPI_BAND,
                                                 st->comm_ptr, st->s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPID_SCHED_BARRIER(st->s);

    mpi_errno = MPID_Sched_cb(&sched_cb_gcn_allocate_cid, st, st->s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPID_SCHED_BARRIER(st->s);

  fn_fail:
    return mpi_errno;
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
#undef FUNCNAME
#define FUNCNAME sched_get_cid_nonblock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int sched_get_cid_nonblock(MPID_Comm * comm_ptr, MPID_Comm * newcomm,
                                  MPIU_Context_id_t * ctx0, MPIU_Context_id_t * ctx1,
                                  MPID_Sched_t s, MPID_Comm_kind_t gcn_cid_kind)
{
    int mpi_errno = MPI_SUCCESS;
    struct gcn_state *st = NULL;
    MPIU_CHKPMEM_DECL(1);

    if (initialize_context_mask) {
        context_id_init();
    }

    MPIU_CHKPMEM_MALLOC(st, struct gcn_state *, sizeof(struct gcn_state), mpi_errno, "gcn_state");
    st->ctx0 = ctx0;
    st->ctx1 = ctx1;
    if (gcn_cid_kind == MPID_INTRACOMM) {
        st->comm_ptr = comm_ptr;
        st->comm_ptr_inter = NULL;
    }
    else {
        st->comm_ptr = comm_ptr->local_comm;
        st->comm_ptr_inter = comm_ptr;
    }
    st->s = s;
    st->gcn_cid_kind = gcn_cid_kind;
    *(st->ctx0) = 0;
    st->own_eager_mask = 0;
    st->first_iter = 1;
    st->new_comm = newcomm;
    st->own_mask = 0;
    if (eager_nelem < 0) {
        /* Ensure that at least one word of deadlock-free context IDs is
         * always set aside for the base protocol */
        MPIU_Assert(MPIR_CVAR_CTXID_EAGER_SIZE >= 0 &&
                    MPIR_CVAR_CTXID_EAGER_SIZE < MPIR_MAX_CONTEXT_MASK - 1);
        eager_nelem = MPIR_CVAR_CTXID_EAGER_SIZE;
    }
    mpi_errno = MPID_Sched_cb(&sched_cb_gcn_copy_mask, st, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPID_SCHED_BARRIER(s);

    MPIU_CHKPMEM_COMMIT();
  fn_exit:
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIR_Get_contextid_nonblock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Get_contextid_nonblock(MPID_Comm * comm_ptr, MPID_Comm * newcommp, MPID_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    MPID_Sched_t s;

    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_CONTEXTID_NONBLOCK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_CONTEXTID_NONBLOCK);

    /* now create a schedule */
    mpi_errno = MPID_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPID_Sched_create(&s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* add some entries to it */
    mpi_errno =
        sched_get_cid_nonblock(comm_ptr, newcommp, &newcommp->context_id, &newcommp->recvcontext_id,
                               s, MPID_INTRACOMM);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* finally, kick off the schedule and give the caller a request */
    mpi_errno = MPID_Sched_start(&s, comm_ptr, tag, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_CONTEXTID_NONBLOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIR_Get_intercomm_contextid_nonblock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Get_intercomm_contextid_nonblock(MPID_Comm * comm_ptr, MPID_Comm * newcommp,
                                          MPID_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    MPID_Sched_t s;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID_NONBLOCK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID_NONBLOCK);

    /* do as much local setup as possible */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPIR_Setup_intercomm_localcomm(comm_ptr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    /* now create a schedule */
    mpi_errno = MPID_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPID_Sched_create(&s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* add some entries to it */

    /* first get a context ID over the local comm */
    mpi_errno =
        sched_get_cid_nonblock(comm_ptr, newcommp, &newcommp->recvcontext_id, &newcommp->context_id,
                               s, MPID_INTERCOMM);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* finally, kick off the schedule and give the caller a request */
    mpi_errno = MPID_Sched_start(&s, comm_ptr, tag, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID_NONBLOCK);
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
#undef FUNCNAME
#define FUNCNAME MPIR_Get_intercomm_contextid
#undef FCNAME
#define FCNAME "MPIR_Get_intercomm_contextid"
int MPIR_Get_intercomm_contextid(MPID_Comm * comm_ptr, MPIU_Context_id_t * context_id,
                                 MPIU_Context_id_t * recvcontext_id)
{
    MPIU_Context_id_t mycontext_id, remote_context_id;
    int mpi_errno = MPI_SUCCESS;
    int tag = 31567;            /* FIXME  - we need an internal tag or
                                 * communication channel.  Can we use a different
                                 * context instead?.  Or can we use the tag
                                 * provided in the intercomm routine? (not on a dup,
                                 * but in that case it can use the collective context) */
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);

    if (!comm_ptr->local_comm) {
        /* Manufacture the local communicator */
        mpi_errno = MPIR_Setup_intercomm_localcomm(comm_ptr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIR_Get_contextid_sparse(comm_ptr->local_comm, &mycontext_id, FALSE);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIU_Assert(mycontext_id != 0);

    /* MPIC routine uses an internal context id.  The local leads (process 0)
     * exchange data */
    remote_context_id = -1;
    if (comm_ptr->rank == 0) {
        mpi_errno = MPIC_Sendrecv(&mycontext_id, 1, MPIU_CONTEXT_ID_T_DATATYPE, 0, tag,
                                  &remote_context_id, 1, MPIU_CONTEXT_ID_T_DATATYPE, 0, tag,
                                  comm_ptr, MPI_STATUS_IGNORE, &errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    /* Make sure that all of the local processes now have this
     * id */
    mpi_errno = MPIR_Bcast_impl(&remote_context_id, 1, MPIU_CONTEXT_ID_T_DATATYPE,
                                0, comm_ptr->local_comm, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    /* The recvcontext_id must be the one that was allocated out of the local
     * group, not the remote group.  Otherwise we could end up posting two
     * MPI_ANY_SOURCE,MPI_ANY_TAG recvs on the same context IDs even though we
     * are attempting to post them for two separate communicators. */
    *context_id = remote_context_id;
    *recvcontext_id = mycontext_id;
  fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_GET_INTERCOMM_CONTEXTID);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Free_contextid
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Free_contextid(MPIU_Context_id_t context_id)
{
    int idx, bitpos, raw_prefix;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_FREE_CONTEXTID);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_FREE_CONTEXTID);

    /* Convert the context id to the bit position */
    raw_prefix = MPID_CONTEXT_READ_FIELD(PREFIX, context_id);
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
    if (MPID_CONTEXT_READ_FIELD(DYNAMIC_PROC, context_id)) {
        MPIU_DBG_MSG_D(COMM, VERBOSE, "skipping dynamic process ctx id, context_id=%d", context_id);
        goto fn_exit;
    }
    else {      /* non-dynamic context ID */
        /* In terms of the context ID bit vector, intercomms and their constituent
         * localcomms have the same value.  To avoid a double-free situation we just
         * don't free the context ID for localcomms and assume it will be cleaned up
         * when the parent intercomm is itself completely freed. */
        if (MPID_CONTEXT_READ_FIELD(IS_LOCALCOMM, context_id)) {
#ifdef USE_DBG_LOGGING
            char dump_str[1024];
            dump_context_id(context_id, dump_str, sizeof(dump_str));
            MPIU_DBG_MSG_S(COMM, VERBOSE, "skipping localcomm id: %s", dump_str);
#endif
            goto fn_exit;
        }
        else if (MPID_CONTEXT_READ_FIELD(SUBCOMM, context_id)) {
            MPIU_DBG_MSG_D(COMM, VERBOSE, "skipping non-parent communicator ctx id, context_id=%d",
                           context_id);
            goto fn_exit;
        }
    }

    /* --BEGIN ERROR HANDLING-- */
    /* Check that this context id has been allocated */
    if ((context_mask[idx] & (0x1 << bitpos)) != 0) {
#ifdef USE_DBG_LOGGING
        char dump_str[1024];
        dump_context_id(context_id, dump_str, sizeof(dump_str));
        MPIU_DBG_MSG_S(COMM, VERBOSE, "context dump: %s", dump_str);
        MPIU_DBG_MSG_S(COMM, VERBOSE, "context mask = %s", context_mask_to_str());
#endif
        MPID_Abort(0, MPI_ERR_INTERN, 1, "In MPIR_Free_contextid, the context id is not in use");
    }
    /* --END ERROR HANDLING-- */

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_CTX_MUTEX);
    /* MT: Note that this update must be done atomically in the multithreaedd
     * case.  In the "one, single lock" implementation, that lock is indeed
     * held when this operation is called. */
    context_mask[idx] |= (0x1 << bitpos);
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_CTX_MUTEX);

    MPIU_DBG_MSG_FMT(COMM, VERBOSE,
                     (MPIU_DBG_FDEST,
                      "Freed context %d, mask[%d] bit %d (prefix=%#x)",
                      context_id, idx, bitpos, raw_prefix));
  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_FREE_CONTEXTID);
}
