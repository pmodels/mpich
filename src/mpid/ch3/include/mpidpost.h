/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDPOST_H_INCLUDED
#define MPIDPOST_H_INCLUDED

#include "mpid_coll.h"

/* FIXME: mpidpost.h is included by mpiimpl.h .  However, mpiimpl.h should 
   refer only to the ADI3 prototypes and should never include prototypes 
   specific to any particular device.  Factor the include files to maintain
   better modularity by providing mpiimpl.h with only the definitions that it
   needs */
/*
 * Channel API prototypes
 */

/*@
  MPIDI_CH3_Progress_start - Mark the beginning of a progress epoch.

  Input Parameters:
. state - pointer to a MPID_Progress_state object

  Return value:
  An MPI error code.
  
  NOTE:
  This routine need only be called if the code might call 
  MPIDI_CH3_Progress_wait().  It is normally used as follows example:
.vb
      if (*req->cc_ptr != 0)
      {
          MPID_Progress_state state;
          
          MPIDI_CH3_Progress_start(&state);
          {
              while(*req->cc_ptr != 0)
              {
                  MPIDI_CH3_Progress_wait(&state);
              }
          }
          MPIDI_CH3_Progress_end(&state);
      }
.ve

  IMPLEMENTORS:
  A multi-threaded implementation might save the current value of a request 
  completion counter in the state.
@*/
void MPIDI_CH3_Progress_start(MPID_Progress_state * state);


/*@
  MPIDI_CH3_Progress_wait - Give the channel implementation an opportunity to 
  make progress on outstanding communication requests.

  Input Parameters:
. state - pointer to the same MPID_Progress_state object passed to
  MPIDI_CH3_Progress_start

  Return value:
  An MPI error code.
  
  NOTE:
  MPIDI_CH3_Progress_start/end() need to be called.
  
  IMPLEMENTORS:
  A multi-threaded implementation would return immediately if the a request 
  had been completed between the call to
  MPIDI_CH3_Progress_start() and MPIDI_CH3_Progress_wait().  This could be 
  implemented by checking a request completion counter
  in the progress state against a global counter, and returning if they did 
  not match.
@*/
int MPIDI_CH3_Progress_wait(MPID_Progress_state * state);


/*@
  MPIDI_CH3_Progress_end - Mark the end of a progress epoch.
  
  Input Parameters:
. state - pointer to the same MPID_Progress_state object passed to
  MPIDI_CH3_Progress_start

  Return value:
  An MPI error code.
@*/
void MPIDI_CH3_Progress_end(MPID_Progress_state * state);


/*@
  MPIDI_CH3_Progress_test - Give the channel implementation an opportunity to 
  make progress on outstanding communication requests.

  Return value:
  An MPI error code.
  
  NOTE:
  This function implicitly marks the beginning and end of a progress epoch.
@*/
int MPIDI_CH3_Progress_test(void);


/*@
  MPIDI_CH3_Progress_poke - Give the channel implementation a moment of 
  opportunity to make progress on outstanding communication.

  Return value:
  An mpi error code.
  
  IMPLEMENTORS:
  This routine is similar to MPIDI_CH3_Progress_test but may not be as 
  thorough in its attempt to satisfy all outstanding
  communication.
@*/
int MPIDI_CH3_Progress_poke(void);



int MPIDI_CH3_Open_port(char *port_name);

int MPIDI_CH3_Comm_spawn_multiple(int count, char ** commands, char *** argvs,
				  int * maxprocs, MPIR_Info ** info_ptrs,
				  int root,
                                  MPIR_Comm * comm_ptr, MPIR_Comm ** intercomm,
				  int * errcodes);

int MPIDI_CH3_Comm_accept(char * port_name, int root, MPIR_Comm * comm_ptr,
			  MPIR_Comm ** newcomm);

int MPIDI_CH3_Comm_connect(char * port_name, int root, MPIR_Comm * comm_ptr,
			   MPIR_Comm ** newcomm);


/* Include definitions from the channel which require items defined by this 
   file (mpidimpl.h) or the file it includes
   (mpiimpl.h). */
#include "mpidi_ch3_post.h"

/*
 * Request utility macros (public - can be used in MPID macros)
 */

/* SHMEM: In the case of a single-threaded shmem channel sharing requests 
   between processes, a write barrier must be performed
   before decrementing the completion counter.  This ensures that other fields
   in the req structure are updated before the
   completion is signalled.  How should that be incorporated into this code 
   from the channel level? */
/* The above comment is accurate, although we do not currently have any channels
 * that do this.  Memory barriers are included in fine-grained multithreaded
 * versions of the MPIR_cc_incr/decr macros. */
#define MPIDI_CH3U_Request_decrement_cc(req_, incomplete_)   \
    MPIR_cc_decr((req_)->cc_ptr, incomplete_)
#define MPIDI_CH3U_Request_increment_cc(req_, was_incomplete_)   \
    MPIR_cc_incr((req_)->cc_ptr, was_incomplete_)

/* versions that do not require return */
#define MPIDI_CH3U_Request_dec_cc(req_)   \
    MPIR_cc_dec((req_)->cc_ptr)
#define MPIDI_CH3U_Request_inc_cc(req_)   \
    MPIR_cc_inc((req_)->cc_ptr)
/*
 * Device level request management macros
 */

#define MPID_Prequest_free_hook(req_) do {} while(0)
#define MPID_Part_request_free_hook(req_) do {} while(0)

/*
 * Device level progress engine macros
 */
#define MPID_Progress_start(progress_state_) MPIDI_CH3_Progress_start(progress_state_)
#define MPID_Progress_wait(progress_state_)  MPIDI_CH3_Progress_wait(progress_state_)
#define MPID_Progress_end(progress_state_)   MPIDI_CH3_Progress_end(progress_state_)
/* This is static inline instead of macro because otherwise MPID_Progress_test will
 * be a chain of macros and therefore can not be used as a callback function */
static inline int MPID_Progress_test(MPID_Progress_state * state) /* state is unused */
{
    return MPIDI_CH3_Progress_test();
}
#define MPID_Progress_poke()		     MPIDI_CH3_Progress_poke()

/* Dynamic process support */
int MPIDI_GPID_GetAllInComm( MPIR_Comm *comm_ptr, int local_size,
                             MPIDI_Gpid local_gpids[], int *singlePG );
int MPIDI_GPID_Get( MPIR_Comm *comm_ptr, int rank, MPIDI_Gpid *gpid );
int MPIDI_GPID_ToLpidArray( int size, MPIDI_Gpid gpid[], int lpid[] );
int MPIDI_PG_ForwardPGInfo( MPIR_Comm *peer_ptr, MPIR_Comm *comm_ptr,
                            int nPGids, const MPIDI_Gpid gpids[],
                            int root );
int MPID_Intercomm_exchange_map( MPIR_Comm *local_comm_ptr, int local_leader,
                                 MPIR_Comm *peer_comm_ptr, int remote_leader,
                                 int *remote_size, int **remote_lpids,
                                 int *is_low_group);
int MPID_Create_intercomm_from_lpids( MPIR_Comm *newcomm_ptr,
                                      int size, const int lpids[] );

#define MPID_INTERCOMM_NO_DYNPROC(comm) (0)

/* ULFM support */
MPL_STATIC_INLINE_PREFIX int MPID_Comm_AS_enabled(MPIR_Comm * comm_ptr)
{
    return comm_ptr->dev.anysource_enabled;
}

MPL_STATIC_INLINE_PREFIX int MPID_Request_is_anysource(MPIR_Request * request_ptr)
{
    int ret = 0;

    if (request_ptr->kind == MPIR_REQUEST_KIND__RECV)
        ret = request_ptr->dev.match.parts.rank == MPI_ANY_SOURCE;

    return ret;
}

/* communicator hooks */
int MPIDI_CH3I_Comm_commit_pre_hook(struct MPIR_Comm *);
int MPIDI_CH3I_Comm_destroy_hook(struct MPIR_Comm *);
int MPIDI_CH3I_Comm_commit_post_hook(struct MPIR_Comm *);

/*
  Device override hooks for asynchronous progress threads
*/
MPL_STATIC_INLINE_PREFIX int MPID_Init_async_thread(void)
{
    return MPIR_Init_async_thread();
}

MPL_STATIC_INLINE_PREFIX int MPID_Finalize_async_thread(void)
{
    return MPIR_Finalize_async_thread();
}

MPL_STATIC_INLINE_PREFIX int MPID_Test(MPIR_Request * request_ptr, int *flag, MPI_Status * status)
{
    return MPIR_Test_impl(request_ptr, flag, status);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testall(int count, MPIR_Request * request_ptrs[],
                                          int *flag, MPI_Status array_of_statuses[],
                                          int requests_property)
{
    return MPIR_Testall_impl(count, request_ptrs, flag, array_of_statuses, requests_property);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testany(int count, MPIR_Request * request_ptrs[],
                                          int *indx, int *flag, MPI_Status * status)
{
    return MPIR_Testany_impl(count, request_ptrs, indx, flag, status);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testsome(int incount, MPIR_Request * request_ptrs[],
                                           int *outcount, int array_of_indices[],
                                           MPI_Status array_of_statuses[])
{
    return MPIR_Testsome_impl(incount, request_ptrs, outcount, array_of_indices, array_of_statuses);
}

MPL_STATIC_INLINE_PREFIX int MPID_Waitall(int count, MPIR_Request * request_ptrs[],
                                          MPI_Status array_of_statuses[], int request_properties)
{
    return MPIR_Waitall_impl(count, request_ptrs, array_of_statuses, request_properties);
}

MPL_STATIC_INLINE_PREFIX int MPID_Wait(MPIR_Request * request_ptr, MPI_Status * status)
{
    return MPIR_Wait_impl(request_ptr, status);
}

MPL_STATIC_INLINE_PREFIX int MPID_Waitany(int count, MPIR_Request * request_ptrs[],
                                          int *indx, MPI_Status * status)
{
    return MPIR_Waitany_impl(count, request_ptrs, indx, status);
}

MPL_STATIC_INLINE_PREFIX int MPID_Waitsome(int incount, MPIR_Request * request_ptrs[],
                                           int *outcount, int array_of_indices[],
                                           MPI_Status array_of_statuses[])
{
    return MPIR_Waitsome_impl(incount, request_ptrs, outcount, array_of_indices, array_of_statuses);
}

#endif /* MPIDPOST_H_INCLUDED */
