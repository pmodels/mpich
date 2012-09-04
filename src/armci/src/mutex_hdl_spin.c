/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

/* These mutexes are built using only MPI-2 atomic accumulate.  The only
 * drawback is that they are vulnerable to livelock.  Here's how the lock
 * algorithm works:
 *
 * Let mutex be an integer that is initially 0.  I hold the mutex when after
 * adding my rank to it, it is equal to my rank.
 *
 * function lock(mutex, p):
 *
 *   acc(mutex, p, me)      // mutex = mutex + me
 *
 *   while (get(mutex, p) != me) {
 *     acc(mutex, p, -1*me) // -1*me is the value to be accumulated
 *     sleep(random)        // Try to avoid livelock/do some backoff
 *     acc(mutex, p, me)
 *   }
 *
 * function unlock(mutex, p)
 *   acc(mutex, p, -1*me)
 */

// TODO: Should each mutex be in a different window?

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>

#include <debug.h>
#include <armci.h>
#include <armcix.h>
#include <armci_internals.h>

#define MAX_TIMEOUT 1000
#define TIMEOUT_MUL 2
#define MIN(A,B) (((A) < (B)) ? (A) : (B))


/** Create a mutex group.  Collective.
  *
  * @param[in] count Number of mutexes to create on the calling process
  * @return          Handle to the mutex group
  */
armcix_mutex_hdl_t ARMCIX_Create_mutexes_hdl(int count, ARMCI_Group *pgroup) {
  int         ierr, i;
  armcix_mutex_hdl_t hdl;

  hdl = malloc(sizeof(struct armcix_mutex_hdl_s));
  ARMCII_Assert(hdl != NULL);

  MPI_Comm_dup(pgroup->comm, &hdl->comm);

  if (count > 0) {
    MPI_Alloc_mem(count*sizeof(long), MPI_INFO_NULL, &hdl->base);
    ARMCII_Assert(hdl->base != NULL);
  } else {
    hdl->base = NULL;
  }

  hdl->count = count;

  // Initialize mutexes to 0
  for (i = 0; i < count; i++)
    hdl->base[i] = 0;

  ierr = MPI_Win_create(hdl->base, count*sizeof(long), sizeof(long) /* displacement size */,
                        MPI_INFO_NULL, hdl->comm, &hdl->window);
  ARMCII_Assert(ierr == MPI_SUCCESS);

  return hdl;
}


/** Destroy/free a mutex group.  Collective.
  * 
  * @param[in] hdl Group to destroy
  */
int ARMCIX_Destroy_mutexes_hdl(armcix_mutex_hdl_t hdl) {
  MPI_Win_free(&hdl->window);
  
  if (hdl->base) 
    MPI_Free_mem(hdl->base);

  MPI_Comm_free(&hdl->comm);

  free(hdl);
  
  return 0;
}


/** Lock a mutex.
  * 
  * @param[in] hdl         Mutex group that the mutex belongs to.
  * @param[in] mutex       Desired mutex number [0..count-1]
  * @param[in] world_proc  Absolute ID of process where the mutex lives
  */
void ARMCIX_Lock_hdl(armcix_mutex_hdl_t hdl, int mutex, int world_proc) {
  int       rank, nproc, proc;
  long      lock_val, unlock_val, lock_out;
  int       timeout = 1;

  MPI_Comm_rank(hdl->comm, &rank);
  MPI_Comm_size(hdl->comm, &nproc);

  /* User gives us the absolute ID.  Translate to the rank in the mutex's group. */
  proc = ARMCII_Translate_absolute_to_group(hdl->comm, world_proc);
  ARMCII_Assert(proc >= 0);

  lock_val   = rank+1;    // Map into range 1..nproc
  unlock_val = -1 * (rank+1);

  /* mutex <- mutex + rank */
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, hdl->window);
  MPI_Accumulate(&lock_val, 1, MPI_LONG, proc, mutex, 1, MPI_LONG, MPI_SUM, hdl->window);
  MPI_Win_unlock(proc, hdl->window);

  for (;;) {
    /* read mutex value */
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, hdl->window);
    MPI_Get(&lock_out, 1, MPI_LONG, proc, mutex, 1, MPI_LONG, hdl->window);
    MPI_Win_unlock(proc, hdl->window);

    ARMCII_Assert(lock_out > 0);
    ARMCII_Assert(lock_out <= nproc*(nproc+1)/2); // Must be < sum of all ranks

    /* We are holding the mutex */
    if (lock_out == rank+1)
      break;

    /* mutex <- mutex - rank */
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, hdl->window);
    MPI_Accumulate(&unlock_val, 1, MPI_LONG, proc, mutex, 1, MPI_LONG, MPI_SUM, hdl->window);
    MPI_Win_unlock(proc, hdl->window);

    /* Exponential backoff */
    usleep(timeout + rand()%timeout);
    timeout = MIN(timeout*TIMEOUT_MUL, MAX_TIMEOUT);
    if (rand() % nproc == 0) // Chance to reset timeout
      timeout = 1;

    /* mutex <- mutex + rank */
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, hdl->window);
    MPI_Accumulate(&lock_val, 1, MPI_LONG, proc, mutex, 1, MPI_LONG, MPI_SUM, hdl->window);
    MPI_Win_unlock(proc, hdl->window);
  }
}


/** Attempt to lock a mutex (non-blocking).
  * 
  * @param[in] hdl         Mutex group that the mutex belongs to.
  * @param[in] mutex       Desired mutex number [0..count-1]
  * @param[in] world_proc  Absolute ID of process where the mutex lives
  * @return                0 on success, non-zero on failure
  */
int ARMCIX_Trylock_hdl(armcix_mutex_hdl_t hdl, int mutex, int world_proc) {
  int       rank, nproc, proc;
  long      lock_val, unlock_val, lock_out;

  ARMCII_Assert(mutex >= 0);

  MPI_Comm_rank(hdl->comm, &rank);
  MPI_Comm_size(hdl->comm, &nproc);

  /* User gives us the absolute ID.  Translate to the rank in the mutex's group. */
  proc = ARMCII_Translate_absolute_to_group(hdl->comm, world_proc);
  ARMCII_Assert(proc >= 0);

  lock_val   = rank+1;
  unlock_val = -1 * (rank+1);

  /* mutex <- mutex + rank */
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, hdl->window);
  MPI_Accumulate(&lock_val, 1, MPI_LONG, proc, mutex, 1, MPI_LONG, MPI_SUM, hdl->window);
  MPI_Win_unlock(proc, hdl->window);

  /* read mutex value */
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, hdl->window);
  MPI_Get(&lock_out, 1, MPI_LONG, proc, mutex, 1, MPI_LONG, hdl->window);
  MPI_Win_unlock(proc, hdl->window);

  ARMCII_Assert(lock_out > 0);
  ARMCII_Assert(lock_out <= nproc*(nproc+1)/2); // Must be < sum of all ranks

  /* We are holding the mutex */
  if (lock_out == rank+1)
    return 0;

  /* mutex <- mutex - rank */
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, hdl->window);
  MPI_Accumulate(&unlock_val, 1, MPI_LONG, proc, mutex, 1, MPI_LONG, MPI_SUM, hdl->window);
  MPI_Win_unlock(proc, hdl->window);

  return 1;
}


/** Unlock a mutex.
  * 
  * @param[in] hdl         Mutex group that the mutex belongs to.
  * @param[in] mutex       Desired mutex number [0..count-1]
  * @param[in] world_proc  Absolute ID of process where the mutex lives
  */
void ARMCIX_Unlock_hdl(armcix_mutex_hdl_t hdl, int mutex, int world_proc) {
  int       rank, nproc, proc;
  long      unlock_val;

  ARMCII_Assert(mutex >= 0);

  MPI_Comm_rank(hdl->comm, &rank);
  MPI_Comm_size(hdl->comm, &nproc);

  /* User gives us the absolute ID.  Translate to the rank in the mutex's group. */
  proc = ARMCII_Translate_absolute_to_group(hdl->comm, world_proc);
  ARMCII_Assert(proc >= 0);

  unlock_val = -1 * (rank+1);

  /* mutex <- mutex - rank */
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, proc, 0, hdl->window);
  MPI_Accumulate(&unlock_val, 1, MPI_LONG, proc, mutex, 1, MPI_LONG, MPI_SUM, hdl->window);
  MPI_Win_unlock(proc, hdl->window);
}

