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


/** Non-blocking put operation.  Note: the implementation is not non-blocking
  */
int ARMCI_NbPut(void *src, void *dst, int bytes, int proc, armci_hdl_t *handle) {
  return ARMCI_Put(src, dst, bytes, proc);
}


/** Non-blocking get operation.  Note: the implementation is not non-blocking
  */
int ARMCI_NbGet(void *src, void *dst, int bytes, int proc, armci_hdl_t *handle) {
  return ARMCI_Get(src, dst, bytes, proc);
}


/** Non-blocking accumulate operation.  Note: the implementation is not non-blocking
  */
int ARMCI_NbAcc(int datatype, void *scale, void *src, void *dst, int bytes, int proc, armci_hdl_t *hdl) {
  return ARMCI_Acc(datatype, scale, src, dst, bytes, proc);
}


/** Wait for a non-blocking operation to finish.
  */
int ARMCI_Wait(armci_hdl_t* handle) {
  return 0;
}


/** Check if a non-blocking operation has finished.
  */
int ARMCI_Test(armci_hdl_t* handle) {
  return 0;
}


/** Wait for all non-blocking operations with implicit (NULL) handles to finish.
  */
int ARMCI_WaitAll(void) {
  return 0;
}


