/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDPOST_H_INCLUDED)
#define MPICH_MPIDPOST_H_INCLUDED

/*
 * Channel API prototypes
 */

/*E
  MPIDI_CH3_Init - Initialize the channel implementation.

  Output Parameters:
+ has_args - boolean value that is true if the command line arguments are available on every node
. has_env - boolean value that is true if the environment variable settings are available on every node
- has_parent - boolean value that is true if this MPI job was spawned by another set of MPI processes

  Return value:
  A MPI error class.
E*/
int MPIDI_CH3_Init(int * argc, char *** argv, int * has_args, int * has_env,
		   int * has_parent);

/*E
  MPIDI_CH3_Finalize - Shutdown the channel implementation.

  Return value:
  A MPI error class.
E*/
int MPIDI_CH3_Finalize(void);


/*E
  MPIDI_CH3_InitParent - Create the parent intercommunicator to be returned by MPI_Comm_get_parent().

  Output Parameters:
. comm_parent - new inter-communicator spanning the spawning processes and the spawned processes

  Return value:
  A MPI error code.
  
  NOTE:
  MPIDI_CH3_InitParent() should only be called if MPIDI_CH3_Init() returns with has_parent set to TRUE.
E*/
int MPIDI_CH3_InitParent(MPID_Comm * comm_parent);


/*E
  MPIDI_CH3_iStartMsg - A non-blocking request to send a CH3 packet.  A request object is allocated only if the send could not be
  completed immediately.

  Input Parameters:
+ vc - virtual connection to send the message over
. pkt - pointer to a MPIDI_CH3_Pkt_t structure containing the substructure to be sent
- pkt_sz - size of the packet substucture

  Output Parameters:
. sreq_ptr - send request or NULL if the send completed immediately

  Return value:
  An mpi error code.
  
  NOTE:
  The packet structure may be allocated on the stack.

  IMPLEMETORS:
  If the send can not be completed immediately, the CH3 packet structure must be stored internally until the request is complete.
  
  If the send completes immediately, the channel implementation shold return NULL and must not call MPIDI_CH3U_Handle_send_req().
E*/
int MPIDI_CH3_iStartMsg(MPIDI_VC * vc, void * pkt, MPIDI_msg_sz_t pkt_sz, MPID_Request **sreq_ptr);


/*E
  MPIDI_CH3_iStartMsgv - A non-blocking request to send a CH3 packet and associated data.  A request object is allocated only if
  the send could not be completed immediately.

  Input Parameters:
+ vc - virtual connection to send the message over
. iov - a vector of a structure contains a buffer pointer and length
- iov_n - number of elements in the vector

  Output Parameters:
. sreq_ptr - send request or NULL if the send completed immediately

  Return value:
  An mpi error code.
  
  NOTE:
  The first element in the vector must point to the packet structure.   The packet structure and the vector may be allocated on
  the stack.

  IMPLEMENTORS:
  If the send can not be completed immediately, the CH3 packet structure and the vector must be stored internally until the
  request is complete.
  
  If the send completes immediately, the channel implementation shold return NULL and must not call MPIDI_CH3U_Handle_send_req().
E*/
int MPIDI_CH3_iStartMsgv(MPIDI_VC * vc, MPID_IOV * iov, int iov_n, MPID_Request **sreq_ptr);


/*E
  MPIDI_CH3_iSend - A non-blocking request to send a CH3 packet using an existing request object.  When the send is complete
  the channel implementation will call MPIDI_CH3U_Handle_send_req().

  Input Parameters:
+ vc - virtual connection over which to send the CH3 packet
. sreq - pointer to the send request object
. pkt - pointer to a MPIDI_CH3_Pkt_t structure containing the substructure to be sent
- pkt_sz - size of the packet substucture

  Return value:
  An mpi error code.
  
  NOTE:
  The packet structure may be allocated on the stack.

  IMPLEMETORS:
  If the send can not be completed immediately, the packet structure must be stored internally until the request is complete.

  If the send completes immediately, the channel implementation still must call MPIDI_CH3U_Handle_send_req().
E*/
int MPIDI_CH3_iSend(MPIDI_VC * vc, MPID_Request * sreq, void * pkt, MPIDI_msg_sz_t pkt_sz);


/*E
  MPIDI_CH3_iSendv - A non-blocking request to send a CH3 packet and associated data using an existing request object.  When
  the send is complete the channel implementation will call MPIDI_CH3U_Handle_send_req().

  Input Parameters:
+ vc - virtual connection over which to send the CH3 packet and data
. sreq - pointer to the send request object
. iov - a vector of a structure contains a buffer pointer and length
- iov_n - number of elements in the vector

  Return value:
  An mpi error code.
  
  NOTE:
  The first element in the vector must point to the packet structure.   The packet structure and the vector may be allocated on
  the stack.

  IMPLEMENTORS:
  If the send can not be completed immediately, the packet structure and the vector must be stored internally until the request is
  complete.

  If the send completes immediately, the channel implementation still must call MPIDI_CH3U_Handle_send_req().
E*/
int MPIDI_CH3_iSendv(MPIDI_VC * vc, MPID_Request * sreq, MPID_IOV * iov, int iov_n);


/*E
  MPIDI_CH3_iWrite - A non-blocking request to send more data.  The buffer containing the data to be sent is described by the I/O
  vector in the send request.  When the send is complete the channel implementation will call MPIDI_CH3U_Handle_send_req().

  Input Parameters:
+ vc - virtual connection over which to send the data 
- sreq - pointer to the send request object

  Return value:
  An mpi error code.
  
  IMPLEMENTORS:
  If the send completes immediately, the channel implementation still must call MPIDI_CH3U_Handle_send_req().
E*/
int MPIDI_CH3_iWrite(MPIDI_VC * vc, MPID_Request * sreq);


/*E
  MPIDI_CH3_iRead - A non-blocking request to receive more data.  The buffer in which to recieve the data is described by the
  I/O vector in the receive request.  When the receive is complete the channel implementation will call
  MPIDI_CH3U_Handle_recv_req().

  Input Parameters:
+ vc - virtual connection over which to send the data 
- sreq - pointer to the send request object

  Return value:
  An mpi error code.
  
  IMPLEMENTORS:
  If the receive completes immediately, the channel implementation still must call MPIDI_CH3U_Handle_recv_req().
E*/
int MPIDI_CH3_iRead(MPIDI_VC * vc, MPID_Request * rreq);


/*E
  MPIDI_CH3_Cancel_send - Attempt to cancel a send request by removing the request from the local send queue.

  Input Parameters:
+ vc - virtual connection over which to send the data 
- sreq - pointer to the send request object

  Output Parameters:
. cancelled - TRUE if the send request was successful.  FALSE otherwise.

  Return value:
  An mpi error code.
  
  IMPLEMENTORS:
  The send request may not be removed from the send queue if one or more bytes of the message have already been sent.
E*/
int MPIDI_CH3_Cancel_send(MPIDI_VC * vc, MPID_Request * sreq, int *cancelled);


/*E
  MPIDI_CH3_Request_create - Allocate and initialize a new request object.

  Return value:
  A pointer to an initialized request object, or NULL if an error occurred.
  
  IMPLEMENTORS:
  MPIDI_CH3_Request_create() must call MPIDI_CH3U_Request_create() after the request object has been allocated.
E*/
MPID_Request * MPIDI_CH3_Request_create(void);


/*E
  MPIDI_CH3_Request_add_ref - Increment the reference count associated with a request object

  Input Parameters:
. req - pointer to the request object
E*/
void MPIDI_CH3_Request_add_ref(MPID_Request * req);

/*E
  MPIDI_CH3_Request_release_ref - Decrement the reference count associated with a request object.

  Input Parameters:
. req - pointer to the request object

  Output Parameters:
. inuse - TRUE if the object is still inuse; FALSE otherwise.
E*/
void MPIDI_CH3_Request_release_ref(MPID_Request * req, int * inuse);


/*E
  MPIDI_CH3_Request_destroy - Release resources in use by an existing request object.

  Input Parameters:
. req - pointer to the request object

  IMPLEMENTORS:
  MPIDI_CH3_Request_destroy() must call MPIDI_CH3U_Request_destroy() before request object is freed.
E*/
void MPIDI_CH3_Request_destroy(MPID_Request * req);


/*E
  MPIDI_CH3_Progress_start - Mark the beginning of a progress epoch.

  NOTE:
  This routine need only be called if the code might call MPIDI_CH3_Progress(TRUE).  For example:
.vb
      while(*req->cc_ptr != 0)
      {
          MPIDI_CH3_Progress_start();
          if (*req->cc_ptr != 0)
          {
              MPIDI_CH3_Progress(TRUE);
          }
          else
          {
              MPIDI_CH3_Progress_end();
          }
      }
.ve

  IMPLEMENTORS:
  A multi-threaded implementation might simply save the current value of a request completion counter in thread specific storage.
E*/
void MPIDI_CH3_Progress_start(void);


/*E
  MPIDI_CH3_Progress_end - Mark the end of a progress epoch.

  NOTE:
  This function should only be called if MPIDI_CH3_Progress_end() has been called and MPIDI_CH3_Progress() has not been called.
E*/
void MPIDI_CH3_Progress_end(void);


/*E
  MPIDI_CH3_Progress_test - Give the channel implementation an opportunity to make progress on outstanding communication requests.

  Return value:
  An mpi error code.
  
  NOTE:
  This function implicitly marks the end of a progress epoch.
  
  DESIGNERS:
  Thread specific storage could be avoided if a progress context were passed into the progress routines.
E*/
int MPIDI_CH3_Progress_test(void);


/*E
  MPIDI_CH3_Progress_wait - Give the channel implementation an opportunity to make progress on outstanding communication requests.

  Return value:
  An mpi error code.
  
  NOTE:
  MPIDI_CH3_Progress_start/end() needs to be called.  This function implicitly marks the end of a progress epoch.
  
  IMPLEMENTORS:
  A multi-threaded implementation would return immediately if the request completion counter did not match the
  value in thread specific storage.

  DESIGNERS:
  Thread specific storage could be avoided if a progress context were passed into the progress routines.
E*/
int MPIDI_CH3_Progress_wait(void);

/*E
  MPIDI_CH3_Progress_poke - Give the channel implementation a moment of opportunity to make progress on outstanding communication.

  Return value:
  An mpi error code.
  
  IMPLEMENTORS:
  This routine is similar to MPIDI_CH3_Progress_test but may not be as thorough in its attempt to satisfy all outstanding
  communication.
E*/
int MPIDI_CH3_Progress_poke(void);


/*E
  MPIDI_CH3_Progress_signal_completion - Inform the progress engine that a pending request has completed.

  IMPLEMENTORS:
  In a single-threaded environment, this routine can be implemented by incrementing a request completion counter.  In a
  multi-threaded environment, the request completion counter must be atomically incremented, and any threaded blocking in the
  progress engine must be woken up.
E*/
void MPIDI_CH3_Progress_signal_completion(void);


/* int MPIDI_CH3_Comm_spawn(const char *, const char *[], const int , MPI_Info, const int, MPID_Comm *, MPID_Comm *, int []); */

int MPIDI_CH3_Open_port(char *port_name);

int MPIDI_CH3_Comm_spawn_multiple(int count, char **commands, 
                                  char ***argvs, int *maxprocs, 
                                  MPID_Info **info_ptrs, int root,
                                  MPID_Comm *comm_ptr, MPID_Comm
                                  **intercomm, int *errcodes);

int MPIDI_CH3_Comm_accept(char *port_name, int root, MPID_Comm
                          *comm_ptr, MPID_Comm **newcomm); 

int MPIDI_CH3_Comm_connect(char *port_name, int root, MPID_Comm
                           *comm_ptr, MPID_Comm **newcomm); 


/*
 * Channel upcall prototypes
 */

/*E
  MPIDI_CH3U_Handle_recv_pkt- Handle a freshly received CH3 packet.

  Input Parameters:
+ vc - virtual connection over which the packet was received
- pkt - pointer to the CH3 packet

  NOTE:
  Multiple threads may note simultaneously call this routine with the same virtual connection.  This constraint eliminates the
  need to lock the VC and thus improves performance.  If simultaneous upcalls for a single VC are a possible, then the calling
  routine must serialize the calls (perhaps by locking the VC).  Special consideration may need to be given to packet ordering
  if the channel has made guarantees about ordering.
E*/
int MPIDI_CH3U_Handle_recv_pkt(MPIDI_VC * vc, MPIDI_CH3_Pkt_t * pkt);


/*E
  MPIDI_CH3U_Handle_recv_req - Process a receive request for which all of the data has been received (and copied) into the
  buffers described by the request's IOV.

  Input Parameters:
+ vc - virtual connection over which the data was received
- rreq - pointer to the receive request object
E*/
int MPIDI_CH3U_Handle_recv_req(MPIDI_VC * vc, MPID_Request * rreq);


/*E
  MPIDI_CH3U_Handle_send_req- Process a send request for which all of the data described the request's IOV has been completely
  buffered and/or sent.

  Input Parameters:
+ vc - virtual connection over which the data was sent
- sreq - pointer to the send request object
E*/

int MPIDI_CH3U_Handle_send_req(MPIDI_VC * vc, MPID_Request * sreq);


/*E
  MPIDI_CH3U_Request_create - Initialize the channel device (ch3) component of a request.

  Input Parameters:
. req - pointer to the request object

  IMPLEMENTORS:
  This routine must be called by MPIDI_CH3_Request_create().
E*/
void MPIDI_CH3U_Request_create(MPID_Request * req);


/*E
  MPIDI_CH3U_Request_destroy - Free resources associated with the channel device (ch3) component of a request.

  Input Parameters:
. req - pointer to the request object

  IMPLEMENTORS:
  This routine must be called by MPIDI_CH3_Request_destroy().
E*/
void MPIDI_CH3U_Request_destroy(MPID_Request * req);



/*
 * Channel utility prototypes
 */
MPID_Request * MPIDI_CH3U_Recvq_FU(int, int, int);
MPID_Request * MPIDI_CH3U_Recvq_FDU(MPI_Request, MPIDI_Message_match *);
MPID_Request * MPIDI_CH3U_Recvq_FDU_or_AEP(int, int, int, int * found);
int MPIDI_CH3U_Recvq_DP(MPID_Request * rreq);
MPID_Request * MPIDI_CH3U_Recvq_FDP(MPIDI_Message_match * match);
MPID_Request * MPIDI_CH3U_Recvq_FDP_or_AEU(MPIDI_Message_match * match, int * found);

void MPIDI_CH3U_Request_complete(MPID_Request * req);
void MPIDI_CH3U_Request_increment_cc(MPID_Request * req, int * was_incomplete);
void MPIDI_CH3U_Request_decrement_cc(MPID_Request * req, int * incomplete);

int MPIDI_CH3U_Request_load_send_iov(MPID_Request * const sreq, MPID_IOV * const iov, int * const iov_n);
int MPIDI_CH3U_Request_load_recv_iov(MPID_Request * const rreq);
int MPIDI_CH3U_Request_unpack_uebuf(MPID_Request * rreq);
int MPIDI_CH3U_Request_unpack_srbuf(MPID_Request * rreq);
void MPIDI_CH3U_Buffer_copy(const void * const sbuf, int scount, MPI_Datatype sdt, int * smpi_errno,
			    void * const rbuf, int rcount, MPI_Datatype rdt, MPIDI_msg_sz_t * rdata_sz, int * rmpi_errno);

/* Include definitions from the channel which require items defined by this file (mpidimpl.h) or the file it includes
   (mpiimpl.h). */
#include "mpidi_ch3_post.h"

#include "mpid_datatype.h"

/*
 * Request utility macros (public - can be used in MPID macros)
 */
#if defined(MPICH_SINGLE_THREADED)
/* SHMEM: In the case of a single-threaded shmem channel sharing requests between processes, a write barrier must be performed
   before decrementing the completion counter.  This insures that other fields in the req structure are updated before the
   completion is signalled.  How should that be incorporated into this code from the channel level? */
#define MPIDI_CH3U_Request_decrement_cc(req_, incomplete_)	\
{								\
    *(incomplete_) = --(*(req_)->cc_ptr);			\
}
#else /* !defined(MPICH_SINGLE_THREADED) */
/* The inc/dec of the completion counter must be atomic since the progress engine could be completing the request in one thread
   and the application could be cancelling the request in another thread. */
#if defined(USE_ATOMIC_UPDATES)
/* If locks are not used, a write barrier must be performed if *before* the completion counter reaches zero.  This insures that
   other fields in the req structure are updated before the completion is signalled. */
#define MPIDI_CH3U_Request_decrement_cc(req_, incomplete_)	\
{								\
    int new_cc__;						\
    								\
    MPID_Atomic_write_barrier();				\
    MPID_Atomic_decr_flag((req_)->cc_ptr, new_cc__);		\
    *(incomplete_) = new_cc__;					\
}
#else /* !defined(USE_ATOMIC_UPDATES) */
#define MPIDI_CH3U_Request_decrement_cc(req_, incomplete_)	\
{								\
    MPID_Request_thread_lock(req_);				\
    {								\
	*(incomplete_) = --(*(req_)->cc_ptr);			\
    }								\
    MPID_Request_thread_unlock(req_);				\
}
#endif /* defined(USE_ATOMIC_UPDATES) */
#endif /* defined(MPICH_SINGLE_THREADED) */

#if defined(MPICH_SINGLE_THREADED)
#define MPIDI_CH3U_Request_increment_cc(req_, was_incomplete_)	\
{								\
    *(was_incomplete_) = (*(req_)->cc_ptr)++;			\
}
#else /* !defined(MPICH_SINGLE_THREADED) */
/* The inc/dec of the completion counter must be atomic since the progress engine could be completing the request in one thread
   and the application could be cancelling the request in another thread. */
#if defined(USE_ATOMIC_UPDATES)
#define MPIDI_CH3U_Request_increment_cc(req_, was_incomplete_)	\
{								\
    int old_cc__;						\
								\
    MPID_Atomic_fetch_and_incr((req_)->cc_ptr, old_cc__);	\
    *(was_incomplete_) = old_cc__;				\
}
#else /* !defined(USE_ATOMIC_UPDATES) */
#define MPIDI_CH3U_Request_increment_cc(req_, was_incomplete_)	\
{								\
    MPID_Request_thread_lock(req_);				\
    {								\
	*(was_incomplete_) = (*(req_)->cc_ptr)++;		\
    }								\
    MPID_Request_thread_unlock(req_);				\
}
#endif /* defined(USE_ATOMIC_UPDATES) */
#endif /* defined(MPICH_SINGLE_THREADED */

/*
 * Device level request management macros
 */
#define MPID_Request_create() (MPIDI_CH3_Request_create())

#define MPID_Request_add_ref(req_) MPIDI_CH3_Request_add_ref(req_)

#define MPID_Request_release(_req)			\
{							\
    int ref_count;					\
							\
    MPIDI_CH3_Request_release_ref((_req), &ref_count);	\
    if (ref_count == 0)					\
    {							\
	MPIDI_CH3_Request_destroy(_req);		\
    }							\
}

#if defined(MPICH_SINGLE_THREADED)
#define MPID_Request_set_completed(_req)	\
{						\
    *(_req)->cc_ptr = 0;			\
    MPIDI_CH3_Progress_signal_completion();	\
}
#else
/* MT - If locks are not used, a write barrier must be performed before zeroing the completion counter.  This insures that other
   fields in the req structure are updated before the completion is signaled. */
#define MPID_Request_set_completed(_req)	\
{						\
    MPID_Request_thread_lock(_req);		\
    {						\
	*(_req)->cc_ptr = 0;			\
    }						\
    MPID_Request_thread_unlock(_req);		\
    						\
    MPIDI_CH3_Progress_signal_completion();	\
}
#endif


/*
 * Device level progress engine macros
 */
#define MPID_Progress_start() {MPIDI_CH3_Progress_start();}
#define MPID_Progress_end()   {MPIDI_CH3_Progress_end();}
#define MPID_Progress_test()  MPIDI_CH3_Progress_test()
#define MPID_Progress_wait()  MPIDI_CH3_Progress_wait()
#define MPID_Progress_poke()  MPIDI_CH3_Progress_poke()

#endif /* !defined(MPICH_MPIDPOST_H_INCLUDED) */
