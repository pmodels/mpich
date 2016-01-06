/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIU_THREAD_POBJ_H_INCLUDED)
#define MPIU_THREAD_POBJ_H_INCLUDED

/* There are multiple locks, one for each (major) object */

/* MT FIXME the following description is almost right, but it needs minor
 * updates and revision to account for the COMPLETION CS and other issues in the
 * request */
/* The fine-grained locking discipline for requests is unfortunately complicated:
 *
 * (1) Raw allocation and deallocation of requests is protected internally by
 * the HANDLEALLOC critical section.  This is currently the same as the HANDLE
 * CS, not sure why we have both...
 *
 * (2) Once allocated, a directly allocated request is intially held exclusively
 * by a single thread.  Direct allocation is common for send requests, but recv
 * requests are usually created differently.
 *
 * (3) Most receive requests are created as the result of a call to FDP_or_AEU
 * or FDU_or_AEP.  Calls to these functions (along with the other receive queue
 * functions) must be inside a MSGQUEUE CS.  This CS protects the queue data
 * structures as well as any fields inside the requests while they are in the
 * queue.  For example, assume a call to FDU_or_AEP, as in MPID_Recv.  If the
 * FDU case hits, the MSGQUEUE CS may be released immediately after the call.
 * If the AEP case hits, however, the MSGQUEUE CS must remain held until any
 * request field manipulation (such as dev.recv_pending_count) is complete.
 *
 * (4) In both the send and receive request cases, there is usually a particular
 * thread in some upper-level code (e.g. MPI_Send) with interest in the
 * completion of the request.  This may or may not be a thread that is also
 * making progress on this request (often not).  The upper level code must not
 * attempt to access any request fields (such as the status) until completion is
 * signalled by the lower layer.
 *
 * (5) Once removed from the receive queue, the request is once again
 * exclusively owned by the dequeuing thread.  From here, the dequeuing thread
 * may do whatever it wants with the request without holding any CS, until it
 * signals the request's completion.  Signalling completion indicates that the
 * thread in the upper layer polling on it may access the rest of the fields in
 * the request.  This completion signalling is lock-free and must be implemented
 * carefully to work correctly in the face of optimizing compilers and CPUs.
 * The upper-level thread now wholly owns the request until it is deallocated.
 *
 * (6) In ch3:nemesis at least, multithreaded access to send requests is managed
 * by the MPIDCOMM (progress engine) CS.  The completion signalling pattern
 * applies here (think MPI_Isend/MPI_Wait).
 *
 * (7) Request cancellation is tricky-ish.  For send cancellation, it is
 * possible that the completion counter is actually *incremented* because a
 * pkt is sent to the recipient asking for remote cancellation.  By asking for
 * cancellation (of any kind of req), the upper layer gives up its exclusive
 * access to the request and must wait for the completion counter to drop to 0
 * before exclusively accessing the request fields.
 *
 * The completion counter is a reference count, much like the object liveness
 * reference count.  However it differs from a normal refcount because of
 * guarantees in the MPI Standard.  Applications must not attempt to complete
 * (wait/test/free) a given request concurrently in two separate threads.  So
 * checking for cc==0 is safe because only one thread is ever allowed to make
 * that check.
 *
 * A non-zero completion count must always be accompanied by a normal reference
 * that is logically held by the progress engine.  Similarly, once the
 * completion counter drops to zero, the progress engine is expected to release
 * its reference.
 */
/* lock ordering: if MPIDCOMM+MSGQUEUE must be aquired at the same time, then
 * the order should be to acquire MPIDCOMM first, then MSGQUEUE.  Release in
 * reverse order. */

/* POBJ locks are all real recursive ops */
#define MPIUI_THREAD_CS_ENTER_POBJ(mutex) MPIUI_THREAD_CS_ENTER_NONRECURSIVE("POBJ", mutex)
#define MPIUI_THREAD_CS_EXIT_POBJ(mutex) MPIUI_THREAD_CS_EXIT_NONRECURSIVE("POBJ", mutex)
#define MPIUI_THREAD_CS_YIELD_POBJ(mutex) MPIUI_THREAD_CS_YIELD_NONRECURSIVE("POBJ", mutex)

/* ALLGRAN locks are all real nonrecursive ops */
#define MPIUI_THREAD_CS_ENTER_ALLGRAN(mutex) MPIUI_THREAD_CS_ENTER_NONRECURSIVE("ALLGRAN", mutex)
#define MPIUI_THREAD_CS_EXIT_ALLGRAN(mutex) MPIUI_THREAD_CS_EXIT_NONRECURSIVE("ALLGRAN", mutex)
#define MPIUI_THREAD_CS_YIELD_ALLGRAN(mutex) MPIUI_THREAD_CS_YIELD_NONRECURSIVE("ALLGRAN", mutex)

/* GLOBAL locks are all NO-OPs */
#define MPIUI_THREAD_CS_ENTER_GLOBAL(mutex) do {} while (0)
#define MPIUI_THREAD_CS_EXIT_GLOBAL(mutex) do {} while (0)
#define MPIUI_THREAD_CS_YIELD_GLOBAL(mutex) do {} while (0)

#endif /* !defined(MPIU_THREAD_POBJ_H_INCLUDED) */
