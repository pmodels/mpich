/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

/* For details on this algorithm, please see:
 *
 * Robert Latham, Robert Ross, and Rajeev Thakur. "Implementing MPI-IO Atomic
 * Mode and Shared File Pointers Using MPI One-Sided Communication."
 * International Journal of High Performance Computing Applications,
 * 21(2):132–143, 2007.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>

#include <mpi.h>
#include <mpix.h>

#define MPI_MUTEX_TAG 100

#ifdef ENABLE_DEBUG
#define debug_print(...) do { printf(__VA_ARGS__); } while(0)
#else
#define debug_print(...)
#endif

/* TODO: Make these all no-ops for sequential runs */

/** Create a group of MPI mutexes.  Collective onthe MPI group.
  *
  * @param[in] count  Number of mutexes on the local process.
  * @param[in] comm   MPI communicator on which to create mutexes
  * @return           Handle to the mutex group.
  */
MPIX_Mutex MPIX_Mutex_create(int my_count, MPI_Comm comm) {
  int rank, nproc, max_count, i;
  MPIX_Mutex hdl;

  hdl = malloc(sizeof(struct mpix_mutex_s));
  assert(hdl != NULL);

  MPI_Comm_dup(comm, &hdl->comm);

  MPI_Comm_rank(hdl->comm, &rank);
  MPI_Comm_size(hdl->comm, &nproc);

  hdl->my_count = my_count;

  /* Find the max. count to determine how many windows we need. */
  MPI_Allreduce(&my_count, &max_count, 1, MPI_INT, MPI_MAX, hdl->comm);
  assert(max_count > 0);

  hdl->max_count = max_count;
  hdl->windows = malloc(sizeof(MPI_Win)*max_count);

  if (my_count > 0) {
    hdl->bases = malloc(sizeof(uint8_t*)*my_count);
  } else {
    hdl->bases = NULL;
  }

  /* We need multiple windows here: one for each mutex.  Otherwise
     performance will suffer due to exclusive access epochs. */
  for (i = 0; i < max_count; i++) {
    int   size = 0;
    void *base = NULL;

    if (i < my_count) {
      MPI_Alloc_mem(nproc, MPI_INFO_NULL, &hdl->bases[i]);
      assert(hdl->bases[i] != NULL);
      bzero(hdl->bases[i], nproc);

      base = hdl->bases[i];
      size = nproc;
    }

    MPI_Win_create(base, size, sizeof(uint8_t), MPI_INFO_NULL, hdl->comm, &hdl->windows[i]);
  }

  return hdl;
}


/** Destroy a group of MPI mutexes.  Collective.
  *
  * @param[in] hdl Handle to the group that should be destroyed.
  * @return        Zero on success, non-zero otherwise.
  */
int MPIX_Mutex_destroy(MPIX_Mutex hdl) {
  int i;

  for (i = 0; i < hdl->max_count; i++) {
    MPI_Win_free(&hdl->windows[i]);
  }
    
  if (hdl->bases != NULL) {
    for (i = 0; i < hdl->my_count; i++)
      MPI_Free_mem(hdl->bases[i]);

    free(hdl->bases);
  }

  MPI_Comm_free(&hdl->comm);
  free(hdl);

  return 0;
}


/** Lock a mutex.
  * 
  * @param[in] hdl        Mutex group that the mutex belongs to.
  * @param[in] mutex      Desired mutex number [0..count-1]
  * @param[in] proc       Rank of process where the mutex lives
  */
void MPIX_Mutex_lock(MPIX_Mutex hdl, int mutex, int proc) {
  int       rank, nproc, already_locked, i;
  uint8_t *buf;

  assert(mutex >= 0 && mutex < hdl->max_count);

  MPI_Comm_rank(hdl->comm, &rank);
  MPI_Comm_size(hdl->comm, &nproc);

  assert(proc >= 0 && proc < nproc);

  buf = malloc(nproc*sizeof(uint8_t));
  assert(buf != NULL);

  buf[rank] = 1;

  /* Get all data from the lock_buf, except the byte belonging to
   * me. Set the byte belonging to me to 1. */
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, hdl->windows[mutex]);
  
  MPI_Put(&buf[rank], 1, MPI_BYTE, proc, rank, 1, MPI_BYTE, hdl->windows[mutex]);

  /* Get data to the left of rank */
  if (rank > 0) {
    MPI_Get(buf, rank, MPI_BYTE, proc, 0, rank, MPI_BYTE, hdl->windows[mutex]);
  }

  /* Get data to the right of rank */
  if (rank < nproc - 1) {
    MPI_Get(&buf[rank+1], nproc-1-rank, MPI_BYTE, proc, rank + 1, nproc-1-rank, MPI_BYTE, hdl->windows[mutex]);
  }
  
  MPI_Win_unlock(proc, hdl->windows[mutex]);

  assert(buf[rank] == 1);

  for (i = already_locked = 0; i < nproc; i++)
    if (buf[i] && i != rank)
      already_locked = 1;

  /* Wait for notification */
  if (already_locked) {
    MPI_Status status;
    debug_print("waiting for notification [proc = %d, mutex = %d]\n", proc, mutex);
    MPI_Recv(NULL, 0, MPI_BYTE, MPI_ANY_SOURCE, MPI_MUTEX_TAG+mutex, hdl->comm, &status);
  }

  debug_print("lock acquired [proc = %d, mutex = %d]\n", proc, mutex);
  free(buf);
}


/** Unlock a mutex.
  * 
  * @param[in] hdl   Mutex group that the mutex belongs to.
  * @param[in] mutex Desired mutex number [0..count-1]
  * @param[in] proc  Rank of process where the mutex lives
  */
void MPIX_Mutex_unlock(MPIX_Mutex hdl, int mutex, int proc) {
  int      rank, nproc, i;
  uint8_t *buf;

  assert(mutex >= 0 && mutex < hdl->max_count);

  MPI_Comm_rank(hdl->comm, &rank);
  MPI_Comm_size(hdl->comm, &nproc);

  assert(proc >= 0 && proc < nproc);

  buf = malloc(nproc*sizeof(uint8_t));

  buf[rank] = 0;

  /* Get all data from the lock_buf, except the byte belonging to
   * me. Set the byte belonging to me to 0. */
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, hdl->windows[mutex]);
  
  MPI_Put(&buf[rank], 1, MPI_BYTE, proc, rank, 1, MPI_BYTE, hdl->windows[mutex]);

  /* Get data to the left of rank */
  if (rank > 0) {
    MPI_Get(buf, rank, MPI_BYTE, proc, 0, rank, MPI_BYTE, hdl->windows[mutex]);
  }

  /* Get data to the right of rank */
  if (rank < nproc - 1) {
    MPI_Get(&buf[rank+1], nproc-1-rank, MPI_BYTE, proc, rank + 1, nproc-1-rank, MPI_BYTE, hdl->windows[mutex]);
  }
  
  MPI_Win_unlock(proc, hdl->windows[mutex]);

  assert(buf[rank] == 0);

  /* Notify the next waiting process, starting to my right for fairness */
  for (i = 1; i < nproc; i++) {
    int p = (rank + i) % nproc;
    if (buf[p] == 1) {
      debug_print("notifying %d [proc = %d, mutex = %d]\n", p, proc, mutex);
      MPI_Send(NULL, 0, MPI_BYTE, p, MPI_MUTEX_TAG+mutex, hdl->comm);
      break;
    }
  }

  debug_print("lock released [proc = %d, mutex = %d]\n", proc, mutex);
  free(buf);
}
