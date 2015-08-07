/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_NBC_H_INCLUDED
#define MPIR_NBC_H_INCLUDED

/* This specifies the interface that must be exposed by the ADI in order to
 * support MPI-3 non-blocking collectives.  MPID_Sched_ routines are all
 * permitted to be inlines.  They are not permitted to be macros.
 *
 * Most (currently all) devices will just use the default implementation that
 * lives in "src/mpid/common/sched" */

/* The device must supply a typedef for MPID_Sched_t.  MPID_Sched_t is a handle
 * to the schedule (often a pointer under the hood), not the actual schedule.
 * This makes it easy to cheaply pass the schedule between functions.  Many
 *
 * The device must also define a constant (possibly a macro) for an invalid
 * schedule: MPID_SCHED_NULL */

/* Context/tag strategy for send/recv ops:
 * -------------------------------
 *
 * Blocking collectives were able to more or less safely separate all
 * communication between different collectives by using a fixed tag per
 * operation.  This prevents some potentially very surprising message matching
 * patterns when two different collectives are posted on the same communicator
 * in rapid succession.  But this strategy probably won't work for NBC because
 * multiple operations of any combination of types can be outstanding at the
 * same time.
 *
 * The MPI-3 draft standard says that all collective ops must be collectively
 * posted in a consistent order w.r.t. other collective operations, including
 * nonblocking collectives.  This means that we can just use a counter to assign
 * tag values that is incremented at each collective start.  We can jump through
 * some hoops to make sure that the blocking collective code is left
 * undisturbed, but it's cleaner to just update them to use the new counter
 * mechanism as well.
 */

/* Open question: should tag allocation be rolled into Sched_start?  Keeping it
 * separate potentially allows more parallelism in the future, but it also
 * pushes more work onto the clients of this interface. */
int MPID_Sched_next_tag(MPID_Comm *comm_ptr, int *tag);

/* the device must provide a typedef for MPID_Sched_t in mpidpre.h */

/* creates a new opaque schedule object and returns a handle to it in (*sp) */
int MPID_Sched_create(MPID_Sched_t *sp);
/* clones orig and returns a handle to the new schedule in (*cloned) */
int MPID_Sched_clone(MPID_Sched_t orig, MPID_Sched_t *cloned);
/* sets (*sp) to MPID_SCHED_NULL and gives you back a request pointer in (*req).
 * The caller is giving up ownership of the opaque schedule object.
 *
 * comm should be the primary (user) communicator with which this collective is
 * associated, even if other hidden communicators are used for a subset of the
 * operations.  It will be used for error handling and similar operations. */
int MPID_Sched_start(MPID_Sched_t *sp, MPID_Comm *comm, int tag, MPID_Request **req);

/* send and recv take a comm ptr to enable hierarchical collectives */
int MPID_Sched_send(const void *buf, int count, MPI_Datatype datatype, int dest, MPID_Comm *comm, MPID_Sched_t s);
int MPID_Sched_recv(void *buf, int count, MPI_Datatype datatype, int src, MPID_Comm *comm, MPID_Sched_t s);

/* just like MPI_Issend, can't complete until the matching recv is posted */
int MPID_Sched_ssend(const void *buf, int count, MPI_Datatype datatype, int dest, MPID_Comm *comm, MPID_Sched_t s);

int MPID_Sched_reduce(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Sched_t s);
/* packing/unpacking can be accomplished by passing MPI_PACKED as either intype
 * or outtype */
int MPID_Sched_copy(const void *inbuf,  int incount,  MPI_Datatype intype,
                    void *outbuf, int outcount, MPI_Datatype outtype, MPID_Sched_t s);
/* require that all previously added ops are complete before subsequent ops
 * may begin to execute */
int MPID_Sched_barrier(MPID_Sched_t s);

/* A convenience macro for the extremely common case that "mpi_errno" is the
 * variable used for tracking error state and MPIR_ERR_POP is needed.  This
 * declutters the NBC code substantially. */
#define MPID_SCHED_BARRIER(sched_)              \
    do {                                        \
        mpi_errno = MPID_Sched_barrier(sched_); \
        if (mpi_errno) MPIR_ERR_POP(mpi_errno); \
    } while (0)

/* Defers evaluating (*count) until the entry actually begins to execute.  This
 * permits algorithms that accumulate/dissipate bytes as rounds progress without
 * excessive (re)calculation of counts for/from other processes.
 *
 * A corresponding _recv_defer function is not currently provided because there
 * is no known use case.  The recv count is just an upper bound, not an exact
 * amount to be received, so an oversized recv is used instead of deferral. */
int MPID_Sched_send_defer(const void *buf, const int *count, MPI_Datatype datatype, int dest, MPID_Comm *comm, MPID_Sched_t s);
/* Just like MPID_Sched_recv except it populates the given status object with
 * the received count and error information, much like a normal recv.  Often
 * useful in conjunction with MPID_Sched_send_defer. */
int MPID_Sched_recv_status(void *buf, int count, MPI_Datatype datatype, int src, MPID_Comm *comm, MPI_Status *status, MPID_Sched_t s);

/* Sched_cb_t funcitons take a comm parameter, the value of which will be the
 * comm passed to Sched_start */
/* callback entries must be used judiciously, otherwise they will prevent
 * caching opportunities */
typedef int (MPID_Sched_cb_t)(MPID_Comm *comm, int tag, void *state);
typedef int (MPID_Sched_cb2_t)(MPID_Comm *comm, int tag, void *state, void *state2);
/* buffer management, fancy reductions, etc */
int MPID_Sched_cb(MPID_Sched_cb_t *cb_p, void *cb_state, MPID_Sched_t s);
int MPID_Sched_cb2(MPID_Sched_cb2_t *cb_p, void *cb_state, void *cb_state2, MPID_Sched_t s);

/* TODO: develop a caching infrastructure for use by the upper level as well,
 * hopefully s.t. uthash can be used somehow */

/* common callback utility functions */
int MPIR_Sched_cb_free_buf(MPID_Comm *comm, int tag, void *state);

/* an upgraded version of MPIU_CHKPMEM_MALLOC/_DECL/_REAP/_COMMIT that adds
 * corresponding cleanup callbacks to the given schedule at _COMMIT time */
#define MPIR_SCHED_CHKPMEM_DECL(n_)                               \
    void *(mpir_sched_chkpmem_stk_[n_]) = { NULL };               \
    int mpir_sched_chkpmem_stk_sp_=0;                             \
    MPIU_AssertDeclValue(const int mpir_sched_chkpmem_stk_sz_,n_)

#define MPIR_SCHED_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,stmt_)  \
    do {                                                                          \
        (pointer_) = (type_)MPIU_Malloc(nbytes_);                                 \
        if (pointer_) {                                                           \
            MPIU_Assert(mpir_sched_chkpmem_stk_sp_ < mpir_sched_chkpmem_stk_sz_); \
            mpir_sched_chkpmem_stk_[mpir_sched_chkpmem_stk_sp_++] = (pointer_);   \
        } else if ((nbytes_) > 0) {                                               \
            MPIU_CHKMEM_SETERR((rc_),(nbytes_),(name_));                          \
            stmt_;                                                                \
        }                                                                         \
    } while (0)

#define MPIR_SCHED_CHKPMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_) \
    MPIR_SCHED_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,goto fn_fail)

/* just cleanup, don't add anything to the schedule */
#define MPIR_SCHED_CHKPMEM_REAP(sched_)                                       \
    do {                                                                      \
        while (mpir_sched_chkpmem_stk_sp_ > 0) {                              \
            MPIU_Free(mpir_sched_chkpmem_stk_[--mpir_sched_chkpmem_stk_sp_]); \
        }                                                                     \
    } while (0)

#define MPIR_SCHED_CHKPMEM_COMMIT(sched_)                                                      \
    do {                                                                                       \
        MPID_SCHED_BARRIER(s);                                                                 \
        while (mpir_sched_chkpmem_stk_sp_ > 0) {                                               \
            mpi_errno = MPID_Sched_cb(&MPIR_Sched_cb_free_buf,                                 \
                                      (mpir_sched_chkpmem_stk_[--mpir_sched_chkpmem_stk_sp_]), \
                                      (sched_));                                               \
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);                                            \
        }                                                                                      \
    } while (0)

#endif /* !defined(MPIR_NBC_H_INCLUDED) */
