/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_NBC_H_INCLUDED
#define MPIR_NBC_H_INCLUDED

/* This specifies the interface that must be exposed by the ADI in order to
 * support MPI-3 non-blocking collectives.  MPIR_Sched_ routines are all
 * permitted to be inlines.  They are not permitted to be macros.
 *
 * Most (currently all) devices will just use the default implementation that
 * lives in "src/mpid/common/sched" */

/* The device must supply a typedef for MPIR_Sched_t.  MPIR_Sched_t is a handle
 * to the schedule (often a pointer under the hood), not the actual schedule.
 * This makes it easy to cheaply pass the schedule between functions.  Many
 *
 * The device must also define a constant (possibly a macro) for an invalid
 * schedule: MPIR_SCHED_NULL */

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

#define MPIR_SCHED_NULL (NULL)

/* Open question: should tag allocation be rolled into Sched_start?  Keeping it
 * separate potentially allows more parallelism in the future, but it also
 * pushes more work onto the clients of this interface. */
int MPIR_Sched_next_tag(MPIR_Comm * comm_ptr, int *tag);

/* the device must provide a typedef for MPIR_Sched_t in mpidpre.h */

/* creates a new opaque schedule object and returns a handle to it in (*sp) */
int MPIR_Sched_create(MPIR_Sched_t * sp);
/* clones orig and returns a handle to the new schedule in (*cloned) */
int MPIR_Sched_clone(MPIR_Sched_t orig, MPIR_Sched_t * cloned);
/* sets (*sp) to MPIR_SCHED_NULL and gives you back a request pointer in (*req).
 * The caller is giving up ownership of the opaque schedule object.
 *
 * comm should be the primary (user) communicator with which this collective is
 * associated, even if other hidden communicators are used for a subset of the
 * operations.  It will be used for error handling and similar operations. */
int MPIR_Sched_start(MPIR_Sched_t * sp, MPIR_Comm * comm, int tag, MPIR_Request ** req);

/* send and recv take a comm ptr to enable hierarchical collectives */
int MPIR_Sched_send(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest,
                    MPIR_Comm * comm, MPIR_Sched_t s);
int MPIR_Sched_recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int src, MPIR_Comm * comm,
                    MPIR_Sched_t s);

/* just like MPI_Issend, can't complete until the matching recv is posted */
int MPIR_Sched_ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest,
                     MPIR_Comm * comm, MPIR_Sched_t s);

int MPIR_Sched_reduce(const void *inbuf, void *inoutbuf, MPI_Aint count, MPI_Datatype datatype,
                      MPI_Op op, MPIR_Sched_t s);
/* packing/unpacking can be accomplished by passing MPI_PACKED as either intype
 * or outtype */
int MPIR_Sched_copy(const void *inbuf, MPI_Aint incount, MPI_Datatype intype,
                    void *outbuf, MPI_Aint outcount, MPI_Datatype outtype, MPIR_Sched_t s);
/* require that all previously added ops are complete before subsequent ops
 * may begin to execute */
int MPIR_Sched_barrier(MPIR_Sched_t s);

/* A convenience macro for the extremely common case that "mpi_errno" is the
 * variable used for tracking error state and MPIR_ERR_POP is needed.  This
 * declutters the NBC code substantially. */
#define MPIR_SCHED_BARRIER(sched_)              \
    do {                                        \
        mpi_errno = MPIR_Sched_barrier(sched_); \
        if (mpi_errno) MPIR_ERR_POP(mpi_errno); \
    } while (0)

/* Defers evaluating (*count) until the entry actually begins to execute.  This
 * permits algorithms that accumulate/dissipate bytes as rounds progress without
 * excessive (re)calculation of counts for/from other processes.
 *
 * A corresponding _recv_defer function is not currently provided because there
 * is no known use case.  The recv count is just an upper bound, not an exact
 * amount to be received, so an oversized recv is used instead of deferral. */
int MPIR_Sched_send_defer(const void *buf, const MPI_Aint * count, MPI_Datatype datatype, int dest,
                          MPIR_Comm * comm, MPIR_Sched_t s);
/* Just like MPIR_Sched_recv except it populates the given status object with
 * the received count and error information, much like a normal recv.  Often
 * useful in conjunction with MPIR_Sched_send_defer. */
int MPIR_Sched_recv_status(void *buf, MPI_Aint count, MPI_Datatype datatype, int src,
                           MPIR_Comm * comm, MPI_Status * status, MPIR_Sched_t s);

/* buffer management, fancy reductions, etc */
int MPIR_Sched_cb(MPIR_Sched_cb_t * cb_p, void *cb_state, MPIR_Sched_t s);
int MPIR_Sched_cb2(MPIR_Sched_cb2_t * cb_p, void *cb_state, void *cb_state2, MPIR_Sched_t s);

/* TODO: develop a caching infrastructure for use by the upper level as well,
 * hopefully s.t. uthash can be used somehow */

/* common callback utility functions */
int MPIR_Sched_cb_free_buf(MPIR_Comm * comm, int tag, void *state);

/* an upgraded version of MPIR_CHKPMEM_MALLOC/_DECL/_REAP/_COMMIT that adds
 * corresponding cleanup callbacks to the given schedule at _COMMIT time */
#define MPIR_SCHED_CHKPMEM_DECL(n_)                               \
    void *(mpir_sched_chkpmem_stk_[n_]) = { NULL };               \
    int mpir_sched_chkpmem_stk_sp_=0;                             \
    MPIR_AssertDeclValue(const int mpir_sched_chkpmem_stk_sz_,n_)

#define MPIR_SCHED_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,class_,stmt_)  \
    do {                                                                          \
        (pointer_) = (type_)MPL_malloc(nbytes_,class_);                           \
        if (pointer_) {                                                           \
            MPIR_Assert(mpir_sched_chkpmem_stk_sp_ < mpir_sched_chkpmem_stk_sz_); \
            mpir_sched_chkpmem_stk_[mpir_sched_chkpmem_stk_sp_++] = (pointer_);   \
        } else if ((nbytes_) > 0) {                                               \
            MPIR_CHKMEM_SETERR((rc_),(nbytes_),(name_));                          \
            stmt_;                                                                \
        }                                                                         \
    } while (0)

#define MPIR_SCHED_CHKPMEM_MALLOC(pointer_,type_,nbytes_,rc_,name_,class_) \
    MPIR_SCHED_CHKPMEM_MALLOC_ORSTMT(pointer_,type_,nbytes_,rc_,name_,class_,goto fn_fail)

/* just cleanup, don't add anything to the schedule */
#define MPIR_SCHED_CHKPMEM_REAP(sched_)                                       \
    do {                                                                      \
        while (mpir_sched_chkpmem_stk_sp_ > 0) {                              \
            MPL_free(mpir_sched_chkpmem_stk_[--mpir_sched_chkpmem_stk_sp_]); \
        }                                                                     \
    } while (0)

#define MPIR_SCHED_CHKPMEM_COMMIT(sched_)                                                      \
    do {                                                                                       \
        MPIR_SCHED_BARRIER(s);                                                                 \
        while (mpir_sched_chkpmem_stk_sp_ > 0) {                                               \
            mpi_errno = MPIR_Sched_cb(&MPIR_Sched_cb_free_buf,                                 \
                                      (mpir_sched_chkpmem_stk_[--mpir_sched_chkpmem_stk_sp_]), \
                                      (sched_));                                               \
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);                                            \
        }                                                                                      \
    } while (0)

#endif /* MPIR_NBC_H_INCLUDED */
