/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <armci.h>


/** Initialize Non-blocking handle.
  */
void ARMCI_INIT_HANDLE(armci_hdl_t *hdl) {
  return;
}


/** Mark a handle as aggregate.
  */
void ARMCI_SET_AGGREGATE_HANDLE(armci_hdl_t *hdl) {
  return;
}


/** Clear an aggregate handle.
  */
void ARMCI_UNSET_AGGREGATE_HANDLE(armci_hdl_t *hdl) {
  return;
}


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_NbPut = PARMCI_NbPut
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_NbPut ARMCI_NbPut
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_NbPut as PARMCI_NbPut
#endif
/* -- end weak symbols block -- */

/** Non-blocking put operation.  Note: the implementation is not non-blocking
  */
int PARMCI_NbPut(void *src, void *dst, int bytes, int proc, armci_hdl_t *handle) {
  return PARMCI_Put(src, dst, bytes, proc);
}


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_NbGet = PARMCI_NbGet
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_NbGet ARMCI_NbGet
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_NbGet as PARMCI_NbGet
#endif
/* -- end weak symbols block -- */

/** Non-blocking get operation.  Note: the implementation is not non-blocking
  */
int PARMCI_NbGet(void *src, void *dst, int bytes, int proc, armci_hdl_t *handle) {
  return PARMCI_Get(src, dst, bytes, proc);
}


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_NbAcc = PARMCI_NbAcc
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_NbAcc ARMCI_NbAcc
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_NbAcc as PARMCI_NbAcc
#endif
/* -- end weak symbols block -- */

/** Non-blocking accumulate operation.  Note: the implementation is not non-blocking
  */
int PARMCI_NbAcc(int datatype, void *scale, void *src, void *dst, int bytes, int proc, armci_hdl_t *hdl) {
  return PARMCI_Acc(datatype, scale, src, dst, bytes, proc);
}


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Wait = PARMCI_Wait
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Wait ARMCI_Wait
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Wait as PARMCI_Wait
#endif
/* -- end weak symbols block -- */

/** Wait for a non-blocking operation to finish.
  */
int PARMCI_Wait(armci_hdl_t* handle) {
  return 0;
}


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Test = PARMCI_Test
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Test ARMCI_Test
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Test as PARMCI_Test
#endif
/* -- end weak symbols block -- */

/** Check if a non-blocking operation has finished.
  */
int PARMCI_Test(armci_hdl_t* handle) {
  return 0;
}


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_WaitAll = PARMCI_WaitAll
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_WaitAll ARMCI_WaitAll
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_WaitAll as PARMCI_WaitAll
#endif
/* -- end weak symbols block -- */

/** Wait for all non-blocking operations with implicit (NULL) handles to finish.
  */
int PARMCI_WaitAll(void) {
  return 0;
}


