/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "utlist.h"

/* A random guess at an appropriate value, we can tune it later.  It could also
 * be a real tunable parameter. */
#define MPIDU_SCHED_INITIAL_ENTRIES (16)

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_COLL_SCHED_DUMP
      category    : COLLECTIVE
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Print schedule data for nonblocking collective operations.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static const char *entry_to_str(enum MPIDU_Sched_entry_type type)
{
    switch (type) {
        case MPIDU_SCHED_ENTRY_SEND:
            return "SEND";
        case MPIDU_SCHED_ENTRY_RECV:
            return "RECV";
        case MPIDU_SCHED_ENTRY_REDUCE:
            return "REDUCE";
        case MPIDU_SCHED_ENTRY_COPY:
            return "COPY";
        case MPIDU_SCHED_ENTRY_NOP:
            return "NOP";
        case MPIDU_SCHED_ENTRY_CB:
            return "CB";
        default:
            return "(out of range)";
    }
}

static void entry_dump(FILE * fh, struct MPIDU_Sched_entry *e)
{
    switch (e->type) {
        case MPIDU_SCHED_ENTRY_SEND:
            {
                struct MPIDU_Sched_send *s = &(e->u.send);
                fprintf(fh, "\t\tSend: " MPI_AINT_FMT_DEC_SPEC " of type %x from %d\n", s->count,
                        s->datatype, s->dest);
                fprintf(fh, "\t\t from buff: %p\n", s->buf);
            }
            break;
        case MPIDU_SCHED_ENTRY_RECV:
            {
                struct MPIDU_Sched_recv *r = &(e->u.recv);
                fprintf(fh, "\t\tRecv: " MPI_AINT_FMT_DEC_SPEC " of type %x from %d\n", r->count,
                        r->datatype, r->src);
                fprintf(fh, "\t\t Into buff: %p\n", r->buf);
            }
            break;
        case MPIDU_SCHED_ENTRY_REDUCE:
            {
                struct MPIDU_Sched_reduce *rd = &(e->u.reduce);
                fprintf(fh, "\t\tReduce: %p -> %p\n", rd->inbuf, rd->inoutbuf);
                fprintf(fh, "\t\t  " MPI_AINT_FMT_DEC_SPEC " elements of type %x\n", rd->count,
                        rd->datatype);
                fprintf(fh, "\t\t Op: %x\n", rd->op);
            }
            break;
        case MPIDU_SCHED_ENTRY_COPY:
            {
                struct MPIDU_Sched_copy *cp = &(e->u.copy);
                fprintf(fh, "\t\tFrom: %p " MPI_AINT_FMT_DEC_SPEC " of type %x\n", cp->inbuf,
                        cp->incount, cp->intype);
                fprintf(fh, "\t\tTo:   %p " MPI_AINT_FMT_DEC_SPEC " of type %x\n", cp->outbuf,
                        cp->outcount, cp->outtype);
            }
            break;
        case MPIDU_SCHED_ENTRY_NOP:
            break;
        case MPIDU_SCHED_ENTRY_CB:
            {
                struct MPIDU_Sched_cb *cb = &(e->u.cb);
                fprintf(fh, "\t\tcb_type=%d\n", cb->cb_type);
                fprintf(fh, "\t\tcb_addr: %p\n", cb->u.cb_p);
            }
            break;
        default:
            break;
    }
}

/* utility function for debugging, dumps the given schedule object to fh */
static void sched_dump(struct MPIDU_Sched *s, FILE * fh)
{
    int i;

    fprintf(fh, "================================\n");
    fprintf(fh, "s=%p\n", s);
    if (s) {
        fprintf(fh, "s->size=%zd\n", s->size);
        fprintf(fh, "s->idx=%zd\n", s->idx);
        fprintf(fh, "s->num_entries=%d\n", s->num_entries);
        fprintf(fh, "s->tag=%d\n", s->tag);
        fprintf(fh, "s->req=%p\n", s->req);
        fprintf(fh, "s->entries=%p\n", s->entries);
        for (i = 0; i < s->num_entries; ++i) {
            fprintf(fh, "--------------------------------\n");
            fprintf(fh, "&s->entries[%d]=%p\n", i, &s->entries[i]);
            fprintf(fh, "\ts->entries[%d].type=%s\n", i, entry_to_str(s->entries[i].type));
            fprintf(fh, "\ts->entries[%d].status=%d\n", i, s->entries[i].status);
            fprintf(fh, "\ts->entries[%d].is_barrier=%s\n", i,
                    (s->entries[i].is_barrier ? "TRUE" : "FALSE"));
            entry_dump(fh, &(s->entries[i]));
        }
    }
    fprintf(fh, "================================\n");
    /*
     * fprintf(fh, "s->next=%p\n", s->next);
     * fprintf(fh, "s->prev=%p\n", s->prev);
     */
}

struct MPIDU_Sched_state {
    struct MPIDU_Sched *head;
    /* no need for a tail with utlist */
};

/* holds on to all incomplete schedules on which progress should be made */
static struct MPIDU_Sched_state all_schedules = { NULL };

/* returns TRUE if any schedules are currently pending completion by the
 * progress engine, FALSE otherwise */
int MPIDU_Sched_are_pending(void)
{
    /* this function is only called within a critical section to decide whether
     * yield is necessary. (ref: .../ch3/.../mpid_nem_inline.h)
     * therefore, there is no need for additional lock protection.
     */
    return (all_schedules.head != NULL);
}

int MPIDU_Sched_next_tag(MPIR_Comm * comm_ptr, int *tag)
{
    int mpi_errno = MPI_SUCCESS;
    /* TODO there should be an internal accessor/utility macro for getting the
     * TAG_UB value that doesn't require using the attribute interface */
    int tag_ub = MPIR_Process.attrs.tag_ub;
#if defined(HAVE_ERROR_CHECKING)
    int start = MPI_UNDEFINED;
    int end = MPI_UNDEFINED;
    struct MPIDU_Sched *elt = NULL;
#endif
    MPIR_FUNC_ENTER;

    *tag = comm_ptr->next_sched_tag;
    ++comm_ptr->next_sched_tag;

#if defined(HAVE_ERROR_CHECKING)
    /* Upon entry into the second half of the tag space, ensure there are no
     * outstanding schedules still using the second half of the space.  Check
     * the first half similarly on wraparound. */
    if (comm_ptr->next_sched_tag == (tag_ub / 2)) {
        start = tag_ub / 2;
        end = tag_ub;
    } else if (comm_ptr->next_sched_tag == (tag_ub)) {
        start = MPIR_FIRST_NBC_TAG;
        end = tag_ub / 2;
    }
    if (start != MPI_UNDEFINED) {
        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_SCHED_LIST_MUTEX);
        DL_FOREACH(all_schedules.head, elt) {
            if (elt->tag >= start && elt->tag < end) {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**toomanynbc");
            }
        }
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_SCHED_LIST_MUTEX);
    }
#endif

    /* wrap the tag values around to the start, but don't allow it to conflict
     * with the tags used by the blocking collectives */
    if (comm_ptr->next_sched_tag == tag_ub) {
        comm_ptr->next_sched_tag = MPIR_FIRST_NBC_TAG;
    }
#if defined(HAVE_ERROR_CHECKING)
  fn_fail:
#endif
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

void MPIDU_Sched_set_tag(struct MPIDU_Sched *s, int tag)
{
    s->tag = tag;
}

/* initiates the schedule entry "e" in the NBC described by "s", where
 * "e" is at "idx" in "s".  This means posting nonblocking sends/recvs,
 * performing reductions, calling callbacks, etc. */
static int MPIDU_Sched_start_entry(struct MPIDU_Sched *s, size_t idx, struct MPIDU_Sched_entry *e)
{
    int mpi_errno = MPI_SUCCESS, ret_errno = MPI_SUCCESS;
    MPIR_Request *r = s->req;
    MPIR_Comm *comm;

    MPIR_FUNC_ENTER;

    MPIR_Assert(e->status == MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED);

    switch (e->type) {
        case MPIDU_SCHED_ENTRY_SEND:
            comm = e->u.send.comm;
            MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "starting SEND entry %d\n", (int) idx);
            if (e->u.send.count_p) {
                /* deferred send */
                /* originally there was no branch and send.count_p was set to
                 * &send.count, but this requires patching up the pointers
                 * during realloc of entries, so this is easier */
                ret_errno = MPIC_Isend(e->u.send.buf, *e->u.send.count_p, e->u.send.datatype,
                                       e->u.send.dest, s->tag, comm, &e->u.send.sreq,
                                       &r->u.nbc.errflag);
            } else {
                if (e->u.send.is_sync) {
                    ret_errno = MPIC_Issend(e->u.send.buf, e->u.send.count, e->u.send.datatype,
                                            e->u.send.dest, s->tag, comm, &e->u.send.sreq,
                                            &r->u.nbc.errflag);
                } else {
                    ret_errno = MPIC_Isend(e->u.send.buf, e->u.send.count, e->u.send.datatype,
                                           e->u.send.dest, s->tag, comm, &e->u.send.sreq,
                                           &r->u.nbc.errflag);
                }
            }
            /* Check if the error is actually fatal to the NBC or we can continue. */
            if (unlikely(ret_errno)) {
                if (MPIR_ERR_NONE == r->u.nbc.errflag) {
                    if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(ret_errno)) {
                        r->u.nbc.errflag = MPIR_ERR_PROC_FAILED;
                    } else {
                        r->u.nbc.errflag = MPIR_ERR_OTHER;
                    }
                }
                e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "Sched SEND failed. Errflag: %d\n",
                              (int) r->u.nbc.errflag);
                if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                    return ret_errno;
                }
            } else {
                e->status = MPIDU_SCHED_ENTRY_STATUS_STARTED;
            }
            break;
        case MPIDU_SCHED_ENTRY_RECV:
            MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "starting RECV entry %d\n", (int) idx);
            comm = e->u.recv.comm;
            ret_errno = MPIC_Irecv(e->u.recv.buf, e->u.recv.count, e->u.recv.datatype,
                                   e->u.recv.src, s->tag, comm, &e->u.recv.rreq);
            /* Check if the error is actually fatal to the NBC or we can continue. */
            if (unlikely(ret_errno)) {
                if (MPIR_ERR_NONE == r->u.nbc.errflag) {
                    if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(ret_errno)) {
                        r->u.nbc.errflag = MPIR_ERR_PROC_FAILED;
                    } else {
                        r->u.nbc.errflag = MPIR_ERR_OTHER;
                    }
                }
                /* We should set the status to failed here - since the request is not freed. this
                 * will be handled later in MPIDU_Sched_progress_state, so set to started here */
                e->status = MPIDU_SCHED_ENTRY_STATUS_STARTED;
                MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "Sched RECV failed. Errflag: %d\n",
                              (int) r->u.nbc.errflag);
                if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                    return ret_errno;
                }
            } else {
                e->status = MPIDU_SCHED_ENTRY_STATUS_STARTED;
            }
            break;
        case MPIDU_SCHED_ENTRY_PT2PT_SEND:
            ret_errno = MPID_Isend(e->u.send.buf, e->u.send.count, e->u.send.datatype,
                                   e->u.send.dest, e->u.send.tag, e->u.send.comm,
                                   MPIR_CONTEXT_INTRA_PT2PT, &e->u.send.sreq);
            if (unlikely(ret_errno)) {
                e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                    return ret_errno;
                }
            } else {
                e->status = MPIDU_SCHED_ENTRY_STATUS_STARTED;
            }
            break;
        case MPIDU_SCHED_ENTRY_PT2PT_RECV:
            ret_errno = MPID_Irecv(e->u.recv.buf, e->u.recv.count, e->u.recv.datatype,
                                   e->u.recv.src, e->u.recv.tag, e->u.recv.comm,
                                   MPIR_CONTEXT_INTRA_PT2PT, &e->u.recv.rreq);
            if (unlikely(ret_errno)) {
                e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                    return ret_errno;
                }
            } else {
                e->status = MPIDU_SCHED_ENTRY_STATUS_STARTED;
            }
            break;
        case MPIDU_SCHED_ENTRY_REDUCE:
            MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "starting REDUCE entry %d\n", (int) idx);
            mpi_errno =
                MPIR_Reduce_local(e->u.reduce.inbuf, e->u.reduce.inoutbuf, e->u.reduce.count,
                                  e->u.reduce.datatype, e->u.reduce.op);
            MPIR_ERR_CHECK(mpi_errno);
            e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
            break;
        case MPIDU_SCHED_ENTRY_COPY:
            MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "starting COPY entry %d\n", (int) idx);
            mpi_errno = MPIR_Localcopy(e->u.copy.inbuf, e->u.copy.incount, e->u.copy.intype,
                                       e->u.copy.outbuf, e->u.copy.outcount, e->u.copy.outtype);
            MPIR_ERR_CHECK(mpi_errno);
            e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
            break;
        case MPIDU_SCHED_ENTRY_NOP:
            MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "starting NOOP entry %d\n", (int) idx);
            /* nothing to be done */
            break;
        case MPIDU_SCHED_ENTRY_CB:
            MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "starting CB entry %d\n", (int) idx);
            if (e->u.cb.cb_type == MPIDU_SCHED_CB_TYPE_1) {
                ret_errno = e->u.cb.u.cb_p(r->comm, s->tag, e->u.cb.cb_state);
                if (s->kind == MPIR_SCHED_KIND_GENERALIZED) {
                    /* Sched entries list can be reallocated inside callback */
                    e = &s->entries[idx];
                } else {
                    MPIR_Assert(e == &s->entries[idx]);
                }
                if (unlikely(ret_errno)) {
                    if (MPIR_ERR_NONE == r->u.nbc.errflag) {
                        if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(ret_errno)) {
                            r->u.nbc.errflag = MPIR_ERR_PROC_FAILED;
                        } else {
                            r->u.nbc.errflag = MPIR_ERR_OTHER;
                        }
                    }
                    e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                    if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                        return ret_errno;
                    }
                } else {
                    e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
                }
            } else if (e->u.cb.cb_type == MPIDU_SCHED_CB_TYPE_2) {
                ret_errno = e->u.cb.u.cb2_p(r->comm, s->tag, e->u.cb.cb_state, e->u.cb.cb_state2);
                if (s->kind == MPIR_SCHED_KIND_GENERALIZED) {
                    /* Sched entries list can be reallocated inside callback */
                    e = &s->entries[idx];
                } else {
                    MPIR_Assert(e == &s->entries[idx]);
                }
                if (unlikely(ret_errno)) {
                    if (MPIR_ERR_NONE == r->u.nbc.errflag) {
                        if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(ret_errno)) {
                            r->u.nbc.errflag = MPIR_ERR_PROC_FAILED;
                        } else {
                            r->u.nbc.errflag = MPIR_ERR_OTHER;
                        }
                    }
                    e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                    if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                        return ret_errno;
                    }
                } else {
                    e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
                }
            } else {
                MPL_DBG_MSG_D(MPIR_DBG_COMM, TYPICAL, "unknown callback type, e->u.cb.cb_type=%d",
                              e->u.cb.cb_type);
                e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
            }

            break;
        default:
            MPL_DBG_MSG_D(MPIR_DBG_COMM, TYPICAL, "unknown entry type, e->type=%d", e->type);
            break;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
    if (r)
        r->status.MPI_ERROR = mpi_errno;
    goto fn_exit;
}

/* Posts or performs any NOT_STARTED operations in the given schedule that are
 * permitted to be started.  That is, this routine will respect schedule
 * barriers appropriately. */
static int MPIDU_Sched_continue(struct MPIDU_Sched *s)
{
    int mpi_errno = MPI_SUCCESS;
    size_t i;

    MPIR_FUNC_ENTER;

    for (i = s->idx; i < s->num_entries; ++i) {
        struct MPIDU_Sched_entry *e = &s->entries[i];

        if (e->status == MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED) {
            mpi_errno = MPIDU_Sched_start_entry(s, i, e);
            /* Sched entries list can be reallocated inside callback */
            e = &s->entries[i];
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* _start_entry may have completed the operation, but won't update s->idx */
        if (i == s->idx && e->status >= MPIDU_SCHED_ENTRY_STATUS_COMPLETE) {
            ++s->idx;   /* this is valid even for barrier entries */
        }

        /* watch the indexing, s->idx might have been incremented above, so
         * ||-short-circuit matters here */
        if (e->is_barrier && (e->status < MPIDU_SCHED_ENTRY_STATUS_COMPLETE || (s->idx != i + 1))) {
            /* we've hit a barrier but outstanding operations before this
             * barrier remain, so we cannot proceed past the barrier */
            break;
        }
    }
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* creates a new opaque schedule object and returns a handle to it in (*sp) */
int MPIDU_Sched_create(MPIR_Sched_t * sp, enum MPIR_Sched_kind kind)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched *s;
    MPIR_CHKPMEM_DECL(2);

    MPIR_FUNC_ENTER;

    *sp = NULL;

    /* this mem will be freed by the progress engine when the request is completed */
    MPIR_CHKPMEM_MALLOC(s, struct MPIDU_Sched *, sizeof(struct MPIDU_Sched), mpi_errno,
                        "schedule object", MPL_MEM_COMM);

    s->size = MPIDU_SCHED_INITIAL_ENTRIES;
    s->idx = 0;
    s->num_entries = 0;
    s->tag = -1;
    s->req = NULL;
    s->entries = NULL;
    s->kind = kind;
    s->buffers = NULL;
    s->handles = NULL;
    s->next = NULL;     /* only needed for sanity checks */
    s->prev = NULL;     /* only needed for sanity checks */

    /* this mem will be freed by the progress engine when the request is completed */
    MPIR_CHKPMEM_MALLOC(s->entries, struct MPIDU_Sched_entry *,
                        MPIDU_SCHED_INITIAL_ENTRIES * sizeof(struct MPIDU_Sched_entry), mpi_errno,
                        "schedule entries vector", MPL_MEM_COMM);

    /* TODO in a debug build, defensively mark all entries as status=INVALID */

    MPIR_CHKPMEM_COMMIT();
    *sp = s;
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

/* clones orig and returns a handle to the new schedule in (*cloned) */
int MPIDU_Sched_clone(MPIR_Sched_t orig, MPIR_Sched_t * cloned)
{
    int mpi_errno = MPI_SUCCESS;
    /* TODO implement this function for real */
    MPIR_Assert_fmt_msg(FALSE, ("clone not yet implemented"));
    MPIR_Assertp(FALSE);
    return mpi_errno;
}

int MPIDU_Sched_free(struct MPIDU_Sched *s)
{
    MPL_free(s->entries);
    if (s->buffers) {
        for (void **p = (void **)utarray_front(s->buffers); p;
             p = (void **) utarray_next(s->buffers, p)) {
            MPL_free(*p);
        }
        utarray_free(s->buffers);
    }
    if (s->handles) {
        for (int *p = (int *)utarray_front(s->handles); p; p = (int *) utarray_next(s->handles, p)) {
            if (HANDLE_GET_MPI_KIND(*p) == MPIR_COMM) {
                MPIR_Comm *comm;
                MPIR_Comm_get_ptr(*p, comm);
                MPIR_Comm_release(comm);
            } else if (HANDLE_GET_MPI_KIND(*p) == MPIR_DATATYPE) {
                MPIR_Datatype_release_if_not_builtin(*p);
            } else if (HANDLE_GET_MPI_KIND(*p) == MPIR_OP) {
                MPIR_Op_release_if_not_builtin(*p);
            } else {
                MPIR_Assert(0);
            }
        }
        utarray_free(s->handles);
    }
    MPL_free(s);
    return MPI_SUCCESS;
}

int MPIDU_Sched_reset(struct MPIDU_Sched *s)
{
    MPIR_Assert(s->kind == MPIR_SCHED_KIND_PERSISTENT);

    for (int i = 0; i < s->num_entries; ++i) {
        s->entries[i].status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    }
    s->idx = 0;
    /* do not reset tag */
    s->req = NULL;
    s->next = NULL;     /* only needed for sanity checks */
    s->prev = NULL;     /* only needed for sanity checks */
    return MPI_SUCCESS;
}

void *MPIDU_Sched_alloc_state(struct MPIDU_Sched *s, MPI_Aint size)
{
    void *p = MPL_malloc(size, MPL_MEM_OTHER);
    if (p == NULL) {
        /* Caller should process error */
        return p;
    }

    if (s->buffers == NULL) {
        utarray_new(s->buffers, &ut_ptr_icd, MPL_MEM_OTHER);
    }
    utarray_push_back(s->buffers, &p, MPL_MEM_OTHER);
    return p;
}

static void sched_add_ref(struct MPIDU_Sched *s, int handle)
{
    if (s->handles == NULL) {
        utarray_new(s->handles, &ut_int_icd, MPL_MEM_OTHER);
    }
    utarray_push_back(s->handles, &handle, MPL_MEM_OTHER);
}

int MPIDU_Sched_start(struct MPIDU_Sched *s, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *r;

    MPIR_FUNC_ENTER;

    *req = NULL;

    /* sanity check the schedule */
    MPIR_Assert(s->num_entries <= s->size);
    MPIR_Assert(s->num_entries == 0 || s->idx < s->num_entries);
    MPIR_Assert(s->req == NULL);
    MPIR_Assert(s->next == NULL);
    MPIR_Assert(s->prev == NULL);
    MPIR_Assert(s->entries != NULL);

    /* now create and populate the request */
    r = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!r)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    /* FIXME is this right when comm/datatype GC is used? */
    MPIR_Comm_add_ref(comm);
    r->comm = comm;
    /* req refcount is currently 1, for the user's request.  Increment for the
     * schedule's reference */
    MPIR_Request_add_ref(r);
    s->req = r;
    *req = r;
    /* cc is 1, which is fine b/c we only use it as a signal, rather than
     * incr/decr on every constituent operation */

    /* Now kick off any initial operations.  Do this before we tell the progress
     * engine about this req+sched, otherwise we have more MT issues to worry
     * about.  Skipping this step will increase latency. */
    mpi_errno = MPIDU_Sched_continue(s);
    MPIR_ERR_CHECK(mpi_errno);

    /* finally, enqueue in the list of all pending schedules so that the
     * progress engine can make progress on it */

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_SCHED_LIST_MUTEX);
    DL_APPEND(all_schedules.head, s);
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_SCHED_LIST_MUTEX);

    MPIR_Progress_hook_activate(MPIR_Nbc_progress_hook_id);

    MPL_DBG_MSG_P(MPIR_DBG_COMM, TYPICAL, "started schedule s=%p\n", s);
    if (MPIR_CVAR_COLL_SCHED_DUMP)
        sched_dump(s, stderr);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (*req)
        *req = NULL;
    if (r) {
        MPIR_Request_free(r);   /* the schedule's ref */
        MPIR_Request_free(r);   /* the user's ref */
    }

    goto fn_exit;
}


/* idx and e are permitted to be NULL */
static int MPIDU_Sched_add_entry(struct MPIDU_Sched *s, int *idx, struct MPIDU_Sched_entry **e)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    struct MPIDU_Sched_entry *ei;

    MPIR_Assert(s->entries != NULL);
    MPIR_Assert(s->size > 0);

    if (s->num_entries == s->size) {
        /* need to grow the entries array */
        s->entries =
            MPL_realloc(s->entries, 2 * s->size * sizeof(struct MPIDU_Sched_entry), MPL_MEM_COMM);
        if (s->entries == NULL)
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
        s->size *= 2;
    }

    i = s->num_entries++;
    ei = &s->entries[i];

    if (idx != NULL)
        *idx = i;
    if (e != NULL)
        *e = ei;
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* do these ops need an entry handle returned? */
int MPIDU_Sched_send(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest,
                     MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    MPIR_ERR_CHECK(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_SEND;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.send.buf = buf;
    e->u.send.count = count;
    e->u.send.count_p = NULL;
    e->u.send.datatype = datatype;
    e->u.send.dest = dest;
    e->u.send.sreq = NULL;      /* will be populated by _start_entry */
    e->u.send.comm = comm;
    e->u.send.is_sync = FALSE;

    /* the user may free the comm & type after initiating but before the
     * underlying send is actually posted, so we must add a reference here and
     * release it at entry completion time */
    MPIR_Comm_add_ref(comm);
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    if (s->kind != MPIR_SCHED_KIND_GENERALIZED) {
        sched_add_ref(s, comm->handle);
        sched_add_ref(s, datatype);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Sched_pt2pt_send(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                           int tag, int dest, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    MPIR_ERR_CHECK(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_PT2PT_SEND;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.send.buf = buf;
    e->u.send.count = count;
    e->u.send.count_p = NULL;
    e->u.send.datatype = datatype;
    e->u.send.dest = dest;
    e->u.send.sreq = NULL;      /* will be populated by _start_entry */
    e->u.send.comm = comm;
    e->u.send.is_sync = FALSE;
    e->u.send.tag = tag;

    /* the user may free the comm & type after initiating but before the
     * underlying send is actually posted, so we must add a reference here and
     * release it at entry completion time */
    MPIR_Comm_add_ref(comm);
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    if (s->kind != MPIR_SCHED_KIND_GENERALIZED) {
        sched_add_ref(s, comm->handle);
        sched_add_ref(s, datatype);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Sched_ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest,
                      MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    MPIR_ERR_CHECK(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_SEND;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.send.buf = buf;
    e->u.send.count = count;
    e->u.send.count_p = NULL;
    e->u.send.datatype = datatype;
    e->u.send.dest = dest;
    e->u.send.sreq = NULL;      /* will be populated by _start_entry */
    e->u.send.comm = comm;
    e->u.send.is_sync = TRUE;

    /* the user may free the comm & type after initiating but before the
     * underlying send is actually posted, so we must add a reference here and
     * release it at entry completion time */
    MPIR_Comm_add_ref(comm);
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    if (s->kind != MPIR_SCHED_KIND_GENERALIZED) {
        sched_add_ref(s, comm->handle);
        sched_add_ref(s, datatype);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPIDU_Sched_send_defer(const void *buf, const MPI_Aint * count, MPI_Datatype datatype, int dest,
                           MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    MPIR_ERR_CHECK(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_SEND;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.send.buf = buf;
    e->u.send.count = MPI_UNDEFINED;
    e->u.send.count_p = count;
    e->u.send.datatype = datatype;
    e->u.send.dest = dest;
    e->u.send.sreq = NULL;      /* will be populated by _start_entry */
    e->u.send.comm = comm;
    e->u.send.is_sync = FALSE;

    /* the user may free the comm & type after initiating but before the
     * underlying send is actually posted, so we must add a reference here and
     * release it at entry completion time */
    MPIR_Comm_add_ref(comm);
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    if (s->kind != MPIR_SCHED_KIND_GENERALIZED) {
        sched_add_ref(s, comm->handle);
        sched_add_ref(s, datatype);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Sched_recv_status(void *buf, MPI_Aint count, MPI_Datatype datatype, int src,
                            MPIR_Comm * comm, MPI_Status * status, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    MPIR_ERR_CHECK(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_RECV;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.recv.buf = buf;
    e->u.recv.count = count;
    e->u.recv.datatype = datatype;
    e->u.recv.src = src;
    e->u.recv.rreq = NULL;      /* will be populated by _start_entry */
    e->u.recv.comm = comm;
    e->u.recv.status = status;
    status->MPI_ERROR = MPI_SUCCESS;
    MPIR_Comm_add_ref(comm);
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    if (s->kind != MPIR_SCHED_KIND_GENERALIZED) {
        sched_add_ref(s, comm->handle);
        sched_add_ref(s, datatype);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Sched_recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int src, MPIR_Comm * comm,
                     MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    MPIR_ERR_CHECK(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_RECV;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.recv.buf = buf;
    e->u.recv.count = count;
    e->u.recv.datatype = datatype;
    e->u.recv.src = src;
    e->u.recv.rreq = NULL;      /* will be populated by _start_entry */
    e->u.recv.comm = comm;
    e->u.recv.status = MPI_STATUS_IGNORE;

    MPIR_Comm_add_ref(comm);
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    if (s->kind != MPIR_SCHED_KIND_GENERALIZED) {
        sched_add_ref(s, comm->handle);
        sched_add_ref(s, datatype);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Sched_pt2pt_recv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                           int tag, int src, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    MPIR_ERR_CHECK(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_PT2PT_RECV;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.recv.buf = buf;
    e->u.recv.count = count;
    e->u.recv.datatype = datatype;
    e->u.recv.src = src;
    e->u.recv.rreq = NULL;      /* will be populated by _start_entry */
    e->u.recv.comm = comm;
    e->u.recv.status = MPI_STATUS_IGNORE;
    e->u.recv.tag = tag;

    MPIR_Comm_add_ref(comm);
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    if (s->kind != MPIR_SCHED_KIND_GENERALIZED) {
        sched_add_ref(s, comm->handle);
        sched_add_ref(s, datatype);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Sched_reduce(const void *inbuf, void *inoutbuf, MPI_Aint count, MPI_Datatype datatype,
                       MPI_Op op, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;
    struct MPIDU_Sched_reduce *reduce = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    MPIR_ERR_CHECK(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_REDUCE;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;
    reduce = &e->u.reduce;

    reduce->inbuf = inbuf;
    reduce->inoutbuf = inoutbuf;
    reduce->count = count;
    reduce->datatype = datatype;
    reduce->op = op;

    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIR_Op_add_ref_if_not_builtin(op);
    if (s->kind != MPIR_SCHED_KIND_GENERALIZED) {
        sched_add_ref(s, datatype);
        sched_add_ref(s, op);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Schedules a copy of "incount" copies of "intype" from "inbuf" to "outbuf" as
 * specified by "outcount" and "outtype".  It is erroneous to attempt to copy
 * more data than will fit into the (outbuf,outcount,outtype)-triple.  This
 * corresponds naturally with the buffer sizing rules for send-recv.
 *
 * Packing/unpacking can be accomplished by passing MPI_PACKED as either intype
 * or outtype. */
int MPIDU_Sched_copy(const void *inbuf, MPI_Aint incount, MPI_Datatype intype,
                     void *outbuf, MPI_Aint outcount, MPI_Datatype outtype, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;
    struct MPIDU_Sched_copy *copy = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    MPIR_ERR_CHECK(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_COPY;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;
    copy = &e->u.copy;

    copy->inbuf = inbuf;
    copy->incount = incount;
    copy->intype = intype;
    copy->outbuf = outbuf;
    copy->outcount = outcount;
    copy->outtype = outtype;

    MPIR_Datatype_add_ref_if_not_builtin(intype);
    MPIR_Datatype_add_ref_if_not_builtin(outtype);
    if (s->kind != MPIR_SCHED_KIND_GENERALIZED) {
        sched_add_ref(s, intype);
        sched_add_ref(s, outtype);
    }

    /* some sanity checking up front */
#if defined(HAVE_ERROR_CHECKING) && !defined(NDEBUG)
    {
        MPI_Aint intype_size, outtype_size;
        MPIR_Datatype_get_size_macro(intype, intype_size);
        MPIR_Datatype_get_size_macro(outtype, outtype_size);
        if (incount * intype_size > outcount * outtype_size) {
            MPL_error_printf("truncation: intype=%#x, intype_size=" MPI_AINT_FMT_DEC_SPEC
                             ", incount=" MPI_AINT_FMT_DEC_SPEC ", outtype=%#x, outtype_size="
                             MPI_AINT_FMT_DEC_SPEC " outcount=" MPI_AINT_FMT_DEC_SPEC "\n", intype,
                             intype_size, incount, outtype, outtype_size, outcount);
        }
    }
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* require that all previously added ops are complete before subsequent ops
 * may begin to execute */
int MPIDU_Sched_barrier(MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    /* mark the previous entry as a barrier unless we're at the beginning, which
     * would be a pointless barrier */
    if (s->num_entries > 0) {
        s->entries[s->num_entries - 1].is_barrier = TRUE;
    }

    return mpi_errno;
}

/* buffer management, fancy reductions, etc */
int MPIDU_Sched_cb(MPIR_Sched_cb_t * cb_p, void *cb_state, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;
    struct MPIDU_Sched_cb *cb = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    MPIR_ERR_CHECK(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_CB;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;
    cb = &e->u.cb;

    cb->cb_type = MPIDU_SCHED_CB_TYPE_1;
    cb->u.cb_p = cb_p;
    cb->cb_state = cb_state;
    cb->cb_state2 = NULL;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* buffer management, fancy reductions, etc */
int MPIDU_Sched_cb2(MPIR_Sched_cb2_t * cb_p, void *cb_state, void *cb_state2, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;
    struct MPIDU_Sched_cb *cb = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    MPIR_ERR_CHECK(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_CB;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;
    cb = &e->u.cb;

    cb->cb_type = MPIDU_SCHED_CB_TYPE_2;
    cb->u.cb2_p = cb_p;
    cb->cb_state = cb_state;
    cb->cb_state2 = cb_state2;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* returns TRUE in (*made_progress) if any of the outstanding schedules in state completed */
static int MPIDU_Sched_progress_state(struct MPIDU_Sched_state *state, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    size_t i;
    struct MPIDU_Sched *s;
    struct MPIDU_Sched *tmp;
    if (made_progress)
        *made_progress = FALSE;

    DL_FOREACH_SAFE(state->head, s, tmp) {
        if (MPIR_CVAR_COLL_SCHED_DUMP)
            sched_dump(s, stderr);

        for (i = s->idx; i < s->num_entries; ++i) {
            struct MPIDU_Sched_entry *e = &s->entries[i];

            switch (e->type) {
                case MPIDU_SCHED_ENTRY_SEND:
                    if (e->u.send.sreq != NULL && MPIR_Request_is_complete(e->u.send.sreq)) {
                        MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                                        (MPL_DBG_FDEST, "completed SEND entry %d, sreq=%p\n",
                                         (int) i, e->u.send.sreq));
                        if (s->req->u.nbc.errflag != MPIR_ERR_NONE)
                            e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                        else
                            e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
                        MPIR_Request_free(e->u.send.sreq);
                        e->u.send.sreq = NULL;
                        if (s->kind == MPIR_SCHED_KIND_GENERALIZED) {
                            MPIR_Comm_release(e->u.send.comm);
                            MPIR_Datatype_release_if_not_builtin(e->u.send.datatype);
                        }
                    }
                    break;
                case MPIDU_SCHED_ENTRY_RECV:
                    if (e->u.recv.rreq != NULL && MPIR_Request_is_complete(e->u.recv.rreq)) {
                        MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                                        (MPL_DBG_FDEST, "completed RECV entry %d, rreq=%p\n",
                                         (int) i, e->u.recv.rreq));
                        MPIR_Process_status(&e->u.recv.rreq->status, &s->req->u.nbc.errflag);
                        if (e->u.recv.status != MPI_STATUS_IGNORE) {
                            MPI_Aint recvd;
                            e->u.recv.status->MPI_ERROR = e->u.recv.rreq->status.MPI_ERROR;
                            MPIR_Get_count_impl(&e->u.recv.rreq->status, MPI_BYTE, &recvd);
                            MPIR_STATUS_SET_COUNT(*(e->u.recv.status), recvd);
                        }
                        if (s->req->u.nbc.errflag != MPIR_ERR_NONE)
                            e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                        else
                            e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
                        MPIR_Request_free(e->u.recv.rreq);
                        e->u.recv.rreq = NULL;
                        if (s->kind == MPIR_SCHED_KIND_GENERALIZED) {
                            MPIR_Comm_release(e->u.recv.comm);
                            MPIR_Datatype_release_if_not_builtin(e->u.recv.datatype);
                        }
                    }
                    break;
                case MPIDU_SCHED_ENTRY_PT2PT_SEND:
                    if (e->u.send.sreq != NULL && MPIR_Request_is_complete(e->u.send.sreq)) {
                        if (s->req->status.MPI_ERROR) {
                            e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                        } else {
                            e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
                        }
                        MPIR_Request_free(e->u.send.sreq);
                        e->u.send.sreq = NULL;
                        if (s->kind == MPIR_SCHED_KIND_GENERALIZED) {
                            MPIR_Comm_release(e->u.send.comm);
                            MPIR_Comm_release(e->u.send.comm);
                        }
                        MPIR_Datatype_release_if_not_builtin(e->u.send.datatype);
                    }
                    break;
                case MPIDU_SCHED_ENTRY_PT2PT_RECV:
                    if (e->u.recv.rreq != NULL && MPIR_Request_is_complete(e->u.recv.rreq)) {
                        if (s->req->status.MPI_ERROR) {
                            e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                        } else {
                            e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
                        }
                        MPIR_Request_free(e->u.recv.rreq);
                        e->u.recv.rreq = NULL;
                        if (s->kind == MPIR_SCHED_KIND_GENERALIZED) {
                            MPIR_Comm_release(e->u.recv.comm);
                            MPIR_Datatype_release_if_not_builtin(e->u.recv.datatype);
                        }
                    }
                    break;
                default:
                    /* all other entry types don't have any sub-requests that
                     * need to be checked */
                    break;
            }

            if (i == s->idx && e->status >= MPIDU_SCHED_ENTRY_STATUS_COMPLETE) {
                ++s->idx;
                MPL_DBG_MSG_D(MPIR_DBG_COMM, VERBOSE, "completed OTHER entry %d\n", (int) i);
                if (e->is_barrier) {
                    /* post/perform the next round of operations */
                    mpi_errno = MPIDU_Sched_continue(s);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            } else if (e->is_barrier && e->status < MPIDU_SCHED_ENTRY_STATUS_COMPLETE) {
                /* don't process anything after this barrier entry */
                break;
            }
        }

        if (s->idx == s->num_entries) {
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST, "completing and dequeuing s=%p r=%p\n", s, s->req));

            /* dequeue this schedule from the state, it's complete */
            DL_DELETE(state->head, s);

            /* TODO refactor into a sched_complete routine? */
            switch (s->req->u.nbc.errflag) {
                case MPIR_ERR_PROC_FAILED:
                    MPIR_ERR_SET(s->req->status.MPI_ERROR, MPIX_ERR_PROC_FAILED, "**proc_failed");
                    break;
                case MPIR_ERR_OTHER:
                    MPIR_ERR_SET(s->req->status.MPI_ERROR, MPI_ERR_OTHER, "**other");
                    break;
                case MPIR_ERR_NONE:
                default:
                    break;
            }

            /* NOTE: persistent sched s may get freed by MPI_Request_free as soon as we
             *       complete the request. Access s->kind before we complete the request.
             */
            bool not_persistent = s->kind != MPIR_SCHED_KIND_PERSISTENT;

            MPIR_Request_complete(s->req);

            if (not_persistent) {
                MPIDU_Sched_free(s);
            }

            if (made_progress)
                *made_progress = TRUE;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* returns TRUE in (*made_progress) if any of the outstanding schedules completed */
int MPIDU_Sched_progress(int *made_progress)
{
    /* Sched progress may call callback functions that will call into progress again.
     * For example, with MPI_Comm_idup, sched_cb_gcn_allocate_cid will call MPIR_Allreduce.
     * This inner progress should skip Sched progress to avoid recursive situation.
     */
    static int in_sched_progress = 0;

    if (in_sched_progress) {
        return MPI_SUCCESS;
    } else {
        int mpi_errno;

        MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_SCHED_LIST_MUTEX);
        in_sched_progress = 1;
        mpi_errno = MPIDU_Sched_progress_state(&all_schedules, made_progress);
        if (!mpi_errno && all_schedules.head == NULL)
            MPIR_Progress_hook_deactivate(MPIR_Nbc_progress_hook_id);
        in_sched_progress = 0;
        MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_SCHED_LIST_MUTEX);

        return mpi_errno;
    }
}
