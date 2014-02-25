/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpl_utlist.h"

/* A random guess at an appropriate value, we can tune it later.  It could also
 * be a real tunable parameter. */
#define MPIDU_SCHED_INITIAL_ENTRIES (16)

#if 0
#define dprintf fprintf
#else
/* FIXME this requires VA_ARGS macros */
#define dprintf(...) do{}while(0)
#endif

/* helper macros to improve code readability */
/* we pessimistically assume that MPI_TYPE_NULL may be passed as a "valid" type
 * for send/recv when MPI_PROC_NULL is the destination/src */
#define dtype_add_ref_if_not_builtin(datatype_)                    \
    do {                                                           \
        if ((datatype_) != MPI_DATATYPE_NULL &&                    \
            HANDLE_GET_KIND((datatype_)) != HANDLE_KIND_BUILTIN)   \
        {                                                          \
            MPID_Datatype *dtp_ = NULL;                            \
            MPID_Datatype_get_ptr((datatype_), dtp_);              \
            MPID_Datatype_add_ref(dtp_);                           \
        }                                                          \
    } while (0)
#define dtype_release_if_not_builtin(datatype_)                    \
    do {                                                           \
        if ((datatype_) != MPI_DATATYPE_NULL &&                    \
            HANDLE_GET_KIND((datatype_)) != HANDLE_KIND_BUILTIN)   \
        {                                                          \
            MPID_Datatype *dtp_ = NULL;                            \
            MPID_Datatype_get_ptr((datatype_), dtp_);              \
            MPID_Datatype_release(dtp_);                           \
        }                                                          \
    } while (0)

/* TODO move to a header somewhere? */
void MPIDU_Sched_dump(struct MPIDU_Sched *s);
void MPIDU_Sched_dump_fh(struct MPIDU_Sched *s, FILE *fh);

struct MPIDU_Sched_state {
    struct MPIDU_Sched *head;
    /* no need for a tail with utlist */
};

/* holds on to all incomplete schedules on which progress should be made */
struct MPIDU_Sched_state all_schedules = {NULL};

/* returns TRUE if any schedules are currently pending completion by the
 * progress engine, FALSE otherwise */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sched_are_pending
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sched_are_pending(void)
{
    return (all_schedules.head != NULL);
}

#undef FUNCNAME
#define FUNCNAME MPID_Sched_next_tag
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Sched_next_tag(MPID_Comm *comm_ptr, int *tag)
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

    *tag = comm_ptr->next_sched_tag;
    ++comm_ptr->next_sched_tag;

#if defined(HAVE_ERROR_CHECKING)
    /* Upon entry into the second half of the tag space, ensure there are no
     * outstanding schedules still using the second half of the space.  Check
     * the first half similarly on wraparound. */
    if (comm_ptr->next_sched_tag == (tag_ub / 2)) {
        start = tag_ub / 2;
        end = tag_ub;
    }
    else if (comm_ptr->next_sched_tag == (tag_ub)) {
        start = MPIR_FIRST_NBC_TAG;
        end = tag_ub / 2;
    }
    if (start != MPI_UNDEFINED) {
        MPL_DL_FOREACH(all_schedules.head, elt) {
            if (elt->tag >= start && elt->tag < end) {
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**toomanynbc");
            }
        }
    }
#endif

    /* wrap the tag values around to the start, but don't allow it to conflict
     * with the tags used by the blocking collectives */
    if (comm_ptr->next_sched_tag == tag_ub) {
        comm_ptr->next_sched_tag = MPIR_FIRST_NBC_TAG;
    }

fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sched_start_entry
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* initiates the schedule entry "e" in the NBC described by "s", where
 * "e" is at "idx" in "s".  This means posting nonblocking sends/recvs,
 * performing reductions, calling callbacks, etc. */
static int MPIDU_Sched_start_entry(struct MPIDU_Sched *s, size_t idx, struct MPIDU_Sched_entry *e)
{
    int mpi_errno = MPI_SUCCESS;
    int context_offset;
    MPID_Request *r = s->req;
    MPID_Comm *comm;

    MPIU_Assert(e->status == MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED);

    switch (e->type) {
        case MPIDU_SCHED_ENTRY_SEND:
            comm = e->u.send.comm;
            context_offset = (comm->comm_kind == MPID_INTRACOMM) ?
                MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;
            if (e->u.send.count_p) {
                /* deferred send */
                /* originally there was no branch and send.count_p was set to
                 * &send.count, but this requires patching up the pointers
                 * during realloc of entries, so this is easier */
                mpi_errno = MPID_Isend(e->u.send.buf, *e->u.send.count_p, e->u.send.datatype,
                                       e->u.send.dest, s->tag, comm, context_offset,
                                       &e->u.send.sreq);
            }
            else {
                if (e->u.send.is_sync) {
                    mpi_errno = MPID_Issend(e->u.send.buf, e->u.send.count, e->u.send.datatype,
                                            e->u.send.dest, s->tag, comm, context_offset,
                                            &e->u.send.sreq);
                }
                else {
                    mpi_errno = MPID_Isend(e->u.send.buf, e->u.send.count, e->u.send.datatype,
                                           e->u.send.dest, s->tag, comm, context_offset,
                                           &e->u.send.sreq);
                }
            }
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            e->status = MPIDU_SCHED_ENTRY_STATUS_STARTED;
            break;
        case MPIDU_SCHED_ENTRY_RECV:
            comm = e->u.recv.comm;
            context_offset = (comm->comm_kind == MPID_INTRACOMM) ?
                MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;
            mpi_errno = MPID_Irecv(e->u.recv.buf, e->u.recv.count, e->u.recv.datatype,
                                   e->u.recv.src, s->tag, comm, context_offset,
                                   &e->u.recv.rreq);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            e->status = MPIDU_SCHED_ENTRY_STATUS_STARTED;
            break;
        case MPIDU_SCHED_ENTRY_REDUCE:
            mpi_errno = MPIR_Reduce_local_impl(e->u.reduce.inbuf, e->u.reduce.inoutbuf, e->u.reduce.count,
                                               e->u.reduce.datatype, e->u.reduce.op);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            if (HANDLE_GET_KIND(e->u.reduce.op) != HANDLE_KIND_BUILTIN) {
                MPID_Op *op_ptr = NULL;
                MPID_Op_get_ptr(e->u.reduce.op, op_ptr);
                MPIR_Op_release(op_ptr);
            }
            dtype_release_if_not_builtin(e->u.reduce.datatype);
            e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
            break;
        case MPIDU_SCHED_ENTRY_COPY:
            mpi_errno = MPIR_Localcopy(e->u.copy.inbuf, e->u.copy.incount, e->u.copy.intype,
                                       e->u.copy.outbuf, e->u.copy.outcount, e->u.copy.outtype);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            dtype_release_if_not_builtin(e->u.copy.intype);
            dtype_release_if_not_builtin(e->u.copy.outtype);
            e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
            break;
        case MPIDU_SCHED_ENTRY_NOP:
            /* nothing to be done */
            break;
        case MPIDU_SCHED_ENTRY_CB:
            if (e->u.cb.cb_type == MPIDU_SCHED_CB_TYPE_1) {
                mpi_errno = e->u.cb.u.cb_p(r->comm, s->tag, e->u.cb.cb_state);
                if (mpi_errno) {
                    e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                    MPIU_ERR_POP(mpi_errno);
                }
            }
            else if (e->u.cb.cb_type == MPIDU_SCHED_CB_TYPE_2) {
                mpi_errno = e->u.cb.u.cb2_p(r->comm, s->tag, e->u.cb.cb_state, e->u.cb.cb_state2);
                if (mpi_errno) {
                    e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                    MPIU_ERR_POP(mpi_errno);
                }
            }
            else {
                MPIU_Assert_fmt_msg(FALSE, ("unknown callback type, e->u.cb.cb_type=%d", e->u.cb.cb_type));
            }
            e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
            break;
        default:
            MPIU_Assert_fmt_msg(FALSE, ("unknown entry type, e->type=%d", e->type));
            break;
    }

fn_exit:
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
#undef FUNCNAME
#define FUNCNAME MPIDU_Sched_continue
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Sched_continue(struct MPIDU_Sched *s)
{
    int mpi_errno = MPI_SUCCESS;
    size_t i;

    for (i = s->idx; i < s->num_entries; ++i) {
        struct MPIDU_Sched_entry *e = &s->entries[i];

        if (e->status == MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED) {
            mpi_errno = MPIDU_Sched_start_entry(s, i, e);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }

        /* _start_entry may have completed the operation, but won't update s->idx */
        if (i == s->idx && e->status == MPIDU_SCHED_ENTRY_STATUS_COMPLETE) {
            ++s->idx; /* this is valid even for barrier entries */
        }

        /* watch the indexing, s->idx might have been incremented above, so
         * ||-short-circuit matters here */
        if (e->is_barrier && (e->status != MPIDU_SCHED_ENTRY_STATUS_COMPLETE || (s->idx != i+1))) {
            /* we've hit a barrier but outstanding operations before this
             * barrier remain, so we cannot proceed past the barrier */
            break;
        }
    }
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Sched_create
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* creates a new opaque schedule object and returns a handle to it in (*sp) */
int MPID_Sched_create(MPID_Sched_t *sp)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched *s;
    MPIU_CHKPMEM_DECL(2);

    *sp = NULL;

    /* this mem will be freed by the progress engine when the request is completed */
    MPIU_CHKPMEM_MALLOC(s, struct MPIDU_Sched *, sizeof(struct MPIDU_Sched), mpi_errno, "schedule object");

    s->size = MPIDU_SCHED_INITIAL_ENTRIES;
    s->idx = 0;
    s->num_entries = 0;
    s->tag = -1;
    s->req = NULL;
    s->entries = NULL;
    s->next = NULL; /* only needed for sanity checks */
    s->prev = NULL; /* only needed for sanity checks */

    /* this mem will be freed by the progress engine when the request is completed */
    MPIU_CHKPMEM_MALLOC(s->entries, struct MPIDU_Sched_entry *, MPIDU_SCHED_INITIAL_ENTRIES*sizeof(struct MPIDU_Sched_entry), mpi_errno, "schedule entries vector");

    /* TODO in a debug build, defensively mark all entries as status=INVALID */

    MPIU_CHKPMEM_COMMIT();
    *sp = s;
fn_exit:
    return mpi_errno;
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Sched_clone
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* clones orig and returns a handle to the new schedule in (*cloned) */
int MPID_Sched_clone(MPID_Sched_t orig, MPID_Sched_t *cloned)
{
    int mpi_errno = MPI_SUCCESS;
    /* TODO implement this function for real */
    MPIU_Assert_fmt_msg(FALSE, ("clone not yet implemented"));
    MPIU_Assertp(FALSE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Sched_start
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* sets (*sp) to MPID_SCHED_NULL and gives you back a request pointer in (*req).
 * The caller is giving up ownership of the opaque schedule object. */
int MPID_Sched_start(MPID_Sched_t *sp, MPID_Comm *comm, int tag, MPID_Request **req)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *r;
    struct MPIDU_Sched *s = *sp;

    *req = NULL;
    *sp = MPID_SCHED_NULL;

    /* sanity check the schedule */
    MPIU_Assert(s->num_entries <= s->size);
    MPIU_Assert(s->num_entries == 0 || s->idx < s->num_entries);
    MPIU_Assert(s->req == NULL);
    MPIU_Assert(s->next == NULL);
    MPIU_Assert(s->prev == NULL);
    MPIU_Assert(s->entries != NULL);

    /* now create and populate the request */
    r = MPID_Request_create();
    if (!r) MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
    r->kind = MPID_COLL_REQUEST;
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
    s->tag = tag;

    /* Now kick off any initial operations.  Do this before we tell the progress
     * engine about this req+sched, otherwise we have more MT issues to worry
     * about.  Skipping this step will increase latency. */
    mpi_errno = MPIDU_Sched_continue(s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* finally, enqueue in the list of all pending schedules so that the
     * progress engine can make progress on it */
    MPL_DL_APPEND(all_schedules.head, s);

    dprintf(stderr, "started schedule s=%p\n", s);
    MPIDU_Sched_dump(s);

fn_exit:
    return mpi_errno;
fn_fail:
    if (*req)
        *req = NULL;
    if (r) {
        /* overly complicated, but we can't just destroy the req directly
         * because we aren't truly inside the device and don't have access to
         * MPIDI_CH3_Request_destroy (we may not even be CH3) */
        int inuse = TRUE;
        MPIR_Request_release_ref(r, &inuse); /* the schedule's ref */
        MPIU_Assert(inuse);
        MPID_Request_release(r); /* the user's ref */
    }

    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sched_add_entry
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* idx and e are permitted to be NULL */
static int MPIDU_Sched_add_entry(struct MPIDU_Sched *s, int *idx, struct MPIDU_Sched_entry **e)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    struct MPIDU_Sched_entry *ei;

    MPIU_Assert(s->entries != NULL);
    MPIU_Assert(s->size > 0);

    if (s->num_entries == s->size) {
        /* need to grow the entries array */
        s->entries = MPIU_Realloc(s->entries, 2*s->size*sizeof(struct MPIDU_Sched_entry));
        if (s->entries == NULL) MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
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

#undef FUNCNAME
#define FUNCNAME MPID_Sched_send
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* do these ops need an entry handle returned? */
int MPID_Sched_send(const void *buf, int count, MPI_Datatype datatype, int dest, MPID_Comm *comm, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_SEND;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.send.buf = buf;
    e->u.send.count = count;
    e->u.send.count_p = NULL;
    e->u.send.datatype = datatype;
    e->u.send.dest = dest;
    e->u.send.sreq = NULL; /* will be populated by _start_entry */
    e->u.send.comm = comm;
    e->u.send.is_sync = FALSE;

    /* the user may free the comm & type after initiating but before the
     * underlying send is actually posted, so we must add a reference here and
     * release it at entry completion time */
    MPIR_Comm_add_ref(comm);
    dtype_add_ref_if_not_builtin(datatype);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Sched_ssend
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Sched_ssend(const void *buf, int count, MPI_Datatype datatype, int dest, MPID_Comm *comm, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_SEND;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.send.buf = buf;
    e->u.send.count = count;
    e->u.send.count_p = NULL;
    e->u.send.datatype = datatype;
    e->u.send.dest = dest;
    e->u.send.sreq = NULL; /* will be populated by _start_entry */
    e->u.send.comm = comm;
    e->u.send.is_sync = TRUE;

    /* the user may free the comm & type after initiating but before the
     * underlying send is actually posted, so we must add a reference here and
     * release it at entry completion time */
    MPIR_Comm_add_ref(comm);
    dtype_add_ref_if_not_builtin(datatype);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Sched_send_defer
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Sched_send_defer(const void *buf, const int *count, MPI_Datatype datatype, int dest, MPID_Comm *comm, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_SEND;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.send.buf = buf;
    e->u.send.count = MPI_UNDEFINED;
    e->u.send.count_p = count;
    e->u.send.datatype = datatype;
    e->u.send.dest = dest;
    e->u.send.sreq = NULL; /* will be populated by _start_entry */
    e->u.send.comm = comm;
    e->u.send.is_sync = FALSE;

    /* the user may free the comm & type after initiating but before the
     * underlying send is actually posted, so we must add a reference here and
     * release it at entry completion time */
    MPIR_Comm_add_ref(comm);
    dtype_add_ref_if_not_builtin(datatype);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Sched_recv_status
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Sched_recv_status(void *buf, int count, MPI_Datatype datatype, int src, MPID_Comm *comm, MPI_Status *status, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_RECV;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.recv.buf = buf;
    e->u.recv.count = count;
    e->u.recv.datatype = datatype;
    e->u.recv.src = src;
    e->u.recv.rreq = NULL; /* will be populated by _start_entry */
    e->u.recv.comm = comm;
    e->u.recv.status = status;

    MPIR_Comm_add_ref(comm);
    dtype_add_ref_if_not_builtin(datatype);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Sched_recv
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Sched_recv(void *buf, int count, MPI_Datatype datatype, int src, MPID_Comm *comm, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_RECV;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;

    e->u.recv.buf = buf;
    e->u.recv.count = count;
    e->u.recv.datatype = datatype;
    e->u.recv.src = src;
    e->u.recv.rreq = NULL; /* will be populated by _start_entry */
    e->u.recv.comm = comm;
    e->u.recv.status = MPI_STATUS_IGNORE;

    MPIR_Comm_add_ref(comm);
    dtype_add_ref_if_not_builtin(datatype);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Sched_reduce
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Sched_reduce(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;
    struct MPIDU_Sched_reduce *reduce = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    e->type = MPIDU_SCHED_ENTRY_REDUCE;
    e->status = MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED;
    e->is_barrier = FALSE;
    reduce = &e->u.reduce;

    reduce->inbuf = inbuf;
    reduce->inoutbuf = inoutbuf;
    reduce->count = count;
    reduce->datatype = datatype;
    reduce->op = op;

    dtype_add_ref_if_not_builtin(datatype);
    if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
        MPID_Op *op_ptr = NULL;
        MPID_Op_get_ptr(op, op_ptr);
        MPIR_Op_add_ref(op_ptr);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Sched_copy
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* Schedules a copy of "incount" copies of "intype" from "inbuf" to "outbuf" as
 * specified by "outcount" and "outtype".  It is erroneous to attempt to copy
 * more data than will fit into the (outbuf,outcount,outtype)-triple.  This
 * corresponds naturally with the buffer sizing rules for send-recv.
 *
 * Packing/unpacking can be accomplished by passing MPI_PACKED as either intype
 * or outtype. */
int MPID_Sched_copy(const void *inbuf,  int incount,  MPI_Datatype intype,
                    void *outbuf, int outcount, MPI_Datatype outtype, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;
    struct MPIDU_Sched_copy *copy = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

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

    dtype_add_ref_if_not_builtin(intype);
    dtype_add_ref_if_not_builtin(outtype);

    /* some sanity checking up front */
#if defined(HAVE_ERROR_CHECKING) && !defined(NDEBUG)
    {
        MPI_Aint intype_size, outtype_size;
        MPID_Datatype_get_size_macro(intype, intype_size);
        MPID_Datatype_get_size_macro(outtype, outtype_size);
        if (incount * intype_size > outcount * outtype_size) {
            MPIU_Error_printf("truncation: intype=%#x, intype_size=%lld, incount=%d, outtype=%#x, outtype_size=%lld outcount=%d\n",
                              intype, (long long)intype_size, incount, outtype, (long long)outtype_size, outcount);
        }
    }
#endif

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Sched_barrier
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* require that all previously added ops are complete before subsequent ops
 * may begin to execute */
int MPID_Sched_barrier(MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    /* mark the previous entry as a barrier unless we're at the beginning, which
     * would be a pointless barrier */
    if (s->num_entries > 0) {
        s->entries[s->num_entries-1].is_barrier = TRUE;
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Sched_cb
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* buffer management, fancy reductions, etc */
int MPID_Sched_cb(MPID_Sched_cb_t *cb_p, void *cb_state, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;
    struct MPIDU_Sched_cb *cb = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

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

#undef FUNCNAME
#define FUNCNAME MPID_Sched_cb2
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* buffer management, fancy reductions, etc */
int MPID_Sched_cb2(MPID_Sched_cb2_t *cb_p, void *cb_state, void *cb_state2, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIDU_Sched_entry *e = NULL;
    struct MPIDU_Sched_cb *cb = NULL;

    mpi_errno = MPIDU_Sched_add_entry(s, NULL, &e);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

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
#undef FUNCNAME
#define FUNCNAME MPIDU_Sched_progress_state
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Sched_progress_state(struct MPIDU_Sched_state *state, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    size_t i;
    struct MPIDU_Sched *s;
    struct MPIDU_Sched *tmp;

    if (made_progress)
        *made_progress = FALSE;

    MPL_DL_FOREACH_SAFE(state->head, s, tmp) {
        dprintf(stderr, "making progress on s=%p\n", s);
        /*MPIDU_Sched_dump(s);*/

        for (i = s->idx; i < s->num_entries; ++i) {
            struct MPIDU_Sched_entry *e = &s->entries[i];

            switch (e->type) {
                case MPIDU_SCHED_ENTRY_SEND:
                    if (e->u.send.sreq != NULL && MPID_Request_is_complete(e->u.send.sreq)) {
                        dprintf(stderr, "completed SEND entry %d, sreq=%p\n", i, e->u.send.sreq);
                        e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
                        mpi_errno = e->u.send.sreq->status.MPI_ERROR;
                        MPID_Request_release(e->u.send.sreq);
                        e->u.send.sreq = NULL;
                        MPIR_Comm_release(e->u.send.comm, /*isDisconnect=*/FALSE);
                        dtype_release_if_not_builtin(e->u.send.datatype);
                        if (mpi_errno) {
                            e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                            MPIU_ERR_POP(mpi_errno);
                        }
                        else {
                            e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
                        }
                    }
                    break;
                case MPIDU_SCHED_ENTRY_RECV:
                    if (e->u.recv.rreq != NULL && MPID_Request_is_complete(e->u.recv.rreq)) {
                        dprintf(stderr, "completed RECV entry %d, rreq=%p\n", i, e->u.recv.rreq);
                        mpi_errno = e->u.recv.rreq->status.MPI_ERROR;
                        MPIR_Request_extract_status(e->u.recv.rreq, e->u.recv.status);
                        MPID_Request_release(e->u.recv.rreq);
                        e->u.recv.rreq = NULL;
                        MPIR_Comm_release(e->u.recv.comm, /*isDisconnect=*/FALSE);
                        dtype_release_if_not_builtin(e->u.recv.datatype);
                        if (mpi_errno) {
                            e->status = MPIDU_SCHED_ENTRY_STATUS_FAILED;
                            MPIU_ERR_POP(mpi_errno);
                        }
                        else {
                            e->status = MPIDU_SCHED_ENTRY_STATUS_COMPLETE;
                        }
                    }
                    break;
                default:
                    /* all other entry types don't have any sub-requests that
                     * need to be checked */
                    break;
            }

            if (i == s->idx && e->status == MPIDU_SCHED_ENTRY_STATUS_COMPLETE) {
                ++s->idx;
                if (e->is_barrier) {
                    dprintf(stderr, "completed barrier in entry %d\n", i);
                    /* post/perform the next round of operations */
                    mpi_errno = MPIDU_Sched_continue(s);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                }
            }
            else if (e->is_barrier && e->status != MPIDU_SCHED_ENTRY_STATUS_COMPLETE) {
                /* don't process anything after this barrier entry */
                break;
            }
        }

        if (s->idx == s->num_entries) {
            dprintf(stderr, "completing and dequeuing s=%p r=%p\n", s, s->req);

            /* dequeue this schedule from the state, it's complete */
            MPL_DL_DELETE(state->head, s);

            /* TODO refactor into a sched_complete routine? */
            MPID_REQUEST_SET_COMPLETED(s->req);
            MPID_Request_release(s->req);
            s->req = NULL;
            MPIU_Free(s->entries);
            MPIU_Free(s);

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
#undef FUNCNAME
#define FUNCNAME MPIDU_Sched_progress
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sched_progress(int *made_progress)
{
    return MPIDU_Sched_progress_state(&all_schedules, made_progress);
}

static const char *entry_to_str(enum MPIDU_Sched_entry_type type) ATTRIBUTE((unused,used));
static const char *entry_to_str(enum MPIDU_Sched_entry_type type)
{
    switch(type) {
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

/* utility function for debugging, dumps the given schedule object to fh */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sched_dump_fh
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void MPIDU_Sched_dump_fh(struct MPIDU_Sched *s, FILE *fh)
{
    int i;
    dprintf(fh, "--------------------------------\n");
    dprintf(fh, "s=%p\n", s);
    if (s) {
        dprintf(fh, "s->size=%zd\n", s->size);
        dprintf(fh, "s->idx=%zd\n", s->idx);
        dprintf(fh, "s->num_entries=%d\n", s->num_entries);
        dprintf(fh, "s->tag=%d\n", s->tag);
        dprintf(fh, "s->req=%p\n", s->req);
        dprintf(fh, "s->entries=%p\n", s->entries);
        for (i = 0; i < s->num_entries; ++i) {
            dprintf(fh, "&s->entries[%d]=%p\n", i, &s->entries[i]);
            dprintf(fh, "s->entries[%d].type=%s\n", i, entry_to_str(s->entries[i].type));
            dprintf(fh, "s->entries[%d].status=%d\n", i, s->entries[i].status);
            dprintf(fh, "s->entries[%d].is_barrier=%s\n", i, (s->entries[i].is_barrier ? "TRUE" : "FALSE"));
        }
    }
    dprintf(fh, "--------------------------------\n");
    /*
    dprintf(fh, "s->next=%p\n", s->next);
    dprintf(fh, "s->prev=%p\n", s->prev);
    */
}

/* utility function for debugging, dumps the given schedule object to stderr */
#undef FUNCNAME
#define FUNCNAME MPIDU_Sched_dump
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void MPIDU_Sched_dump(struct MPIDU_Sched *s)
{
    MPIDU_Sched_dump_fh(s, stderr);
}

