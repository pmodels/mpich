/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include <debug.h>
#include <armci.h>
#include <armci_internals.h>
#include <armcix.h>

#define MAX_TIMEOUT 1000
#define TIMEOUT_MUL 2
#define MIN(A,B) (((A) < (B)) ? (A) : (B))


/** This is the handle for the "default" group of mutexes used by the
  * standard ARMCI mutex API
  */
static armcix_mutex_hdl_t armci_mutex_hdl = NULL;


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Create_mutexes = PARMCI_Create_mutexes
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Create_mutexes ARMCI_Create_mutexes
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Create_mutexes as PARMCI_Create_mutexes
#endif
/* -- end weak symbols block -- */

/** Create ARMCI mutexes.  Collective.
  *
  * @param[in] count Number of mutexes to create on the calling process
  */
int PARMCI_Create_mutexes(int count) {
  if (armci_mutex_hdl != NULL)
    ARMCII_Error("attempted to create ARMCI mutexes multiple times");

  armci_mutex_hdl = ARMCIX_Create_mutexes_hdl(count, &ARMCI_GROUP_WORLD);

  if (armci_mutex_hdl != NULL)
    return 0;
  else
    return 1;
}


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Destroy_mutexes = PARMCI_Destroy_mutexes
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Destroy_mutexes ARMCI_Destroy_mutexes
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Destroy_mutexes as PARMCI_Destroy_mutexes
#endif
/* -- end weak symbols block -- */

/** Destroy/free ARMCI mutexes.  Collective.
  */
int PARMCI_Destroy_mutexes(void) {
  int err;

  if (armci_mutex_hdl == NULL)
    ARMCII_Error("attempted to free unallocated ARMCI mutexes");
  
  err = ARMCIX_Destroy_mutexes_hdl(armci_mutex_hdl);
  armci_mutex_hdl = NULL;

  return err;
}


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Lock = PARMCI_Lock
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Lock ARMCI_Lock
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Lock as PARMCI_Lock
#endif
/* -- end weak symbols block -- */

/** Lock a mutex.
  *
  * @param[in] mutex Number of the mutex to lock
  * @param[in] proc  Target process for the lock operation
  */
void PARMCI_Lock(int mutex, int proc) {
  if (armci_mutex_hdl == NULL)
    ARMCII_Error("attempted to lock on unallocated ARMCI mutexes");
  
  ARMCIX_Lock_hdl(armci_mutex_hdl, mutex, proc);
}


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Unlock = PARMCI_Unlock
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Unlock ARMCI_Unlock
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Unlock as PARMCI_Unlock
#endif
/* -- end weak symbols block -- */

/** Unlock a mutex.
  *
  * @param[in] mutex Number of the mutex to unlock
  * @param[in] proc  Target process for the unlock operation
  */
void PARMCI_Unlock(int mutex, int proc) {
  if (armci_mutex_hdl == NULL)
    ARMCII_Error("attempted to unlock on unallocated ARMCI mutexes");
  
  ARMCIX_Unlock_hdl(armci_mutex_hdl, mutex, proc);
}
