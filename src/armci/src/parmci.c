/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

/* If no weak symbols support */
#if !defined(HAVE_PRAGMA_WEAK) && !defined(HAVE_PRAGMA_HP_SEC_DEF) && !defined(HAVE_PRAGMA_CRI_DUP)

#include "armci.h"

#pragma weak ARMCI_Init
int ARMCI_Init(void) {
  return PARMCI_Init();
}

#pragma weak ARMCI_Init_args
int ARMCI_Init_args(int *argc, char ***argv) {
  return PARMCI_Init_args(argc, argv);
}

#pragma weak ARMCI_Initialized
int ARMCI_Initialized(void) {
  return PARMCI_Initialized();
}

#pragma weak ARMCI_Finalize
int ARMCI_Finalize(void) {
  return PARMCI_Finalize();
}

#pragma weak ARMCI_Malloc
int ARMCI_Malloc(void **base_ptrs, armci_size_t size) {
  return PARMCI_Malloc(base_ptrs, size);
}

#pragma weak ARMCI_Free
int ARMCI_Free(void *ptr) {
  return PARMCI_Free(ptr);
}

#pragma weak ARMCI_Malloc_local
void *ARMCI_Malloc_local(armci_size_t size) {
  return PARMCI_Malloc_local(size);
}

#pragma weak ARMCI_Free_local
int ARMCI_Free_local(void *ptr) {
  return PARMCI_Free_local(ptr);
}

#pragma weak ARMCI_Barrier
void ARMCI_Barrier(void) {
  PARMCI_Barrier();
  return;
}

#pragma weak ARMCI_Fence
void ARMCI_Fence(int proc) {
  PARMCI_Fence(proc);
  return;
}

#pragma weak ARMCI_AllFence
void ARMCI_AllFence(void) {
  PARMCI_AllFence();
  return;
}

#pragma weak ARMCI_Access_begin
void ARMCI_Access_begin(void *ptr) {
  PARMCI_Access_begin(ptr);
  return;
}

#pragma weak ARMCI_Access_end
void ARMCI_Access_end(void *ptr) {
  PARMCI_Access_end(ptr);
  return;
}

#pragma weak ARMCI_Get
int ARMCI_Get(void *src, void *dst, int size, int target) {
  return PARMCI_Get(src, dst, size, target);
}

#pragma weak ARMCI_Put
int ARMCI_Put(void *src, void *dst, int size, int target) {
  return PARMCI_Put(src, dst, size, target);
}

#pragma weak ARMCI_Acc
int ARMCI_Acc(int datatype, void *scale, void *src, void *dst, int bytes, int proc) {
  return PARMCI_Acc(datatype, scale, src, dst, bytes, proc);
}

#pragma weak ARMCI_PutS
int ARMCI_PutS(void *src_ptr, int src_stride_ar[], void *dst_ptr, int dst_stride_ar[], int count[], int stride_levels, int proc) {
  return PARMCI_PutS(src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels, proc);
}

#pragma weak ARMCI_GetS
int ARMCI_GetS(void *src_ptr, int src_stride_ar[], void *dst_ptr, int dst_stride_ar[], int count[], int stride_levels, int proc) {
  return PARMCI_GetS(src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels, proc);
}

#pragma weak ARMCI_AccS
int ARMCI_AccS(int datatype, void *scale, void *src_ptr, int src_stride_ar[], void *dst_ptr, int dst_stride_ar[], int count[], int stride_levels, int proc) {
  return PARMCI_AccS(datatype, scale, src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels, proc);
}

#pragma weak ARMCI_Put_flag
int ARMCI_Put_flag(void *src, void* dst, int size, int *flag, int value, int proc) {
  return PARMCI_Put_flag(src, dst, size, flag, value, proc);
}

#pragma weak ARMCI_PutS_flag
int ARMCI_PutS_flag(void *src_ptr, int src_stride_ar[], void *dst_ptr, int dst_stride_ar[], int count[], int stride_levels, int *flag, int value, int proc) {
  return PARMCI_PutS_flag(src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels, flag, value, proc);
}

#pragma weak ARMCI_PutV
int ARMCI_PutV(armci_giov_t *iov, int iov_len, int proc) {
  return PARMCI_PutV(iov, iov_len, proc);
}

#pragma weak ARMCI_GetV
int ARMCI_GetV(armci_giov_t *iov, int iov_len, int proc) {
  return PARMCI_GetV(iov, iov_len, proc);
}

#pragma weak ARMCI_AccV
int ARMCI_AccV(int datatype, void *scale, armci_giov_t *iov, int iov_len, int proc) {
  return PARMCI_AccV(datatype, scale, iov, iov_len, proc);
}

#pragma weak ARMCI_Wait
int ARMCI_Wait(armci_hdl_t* hdl) {
  return PARMCI_Wait(hdl);
}

#pragma weak ARMCI_Test
int ARMCI_Test(armci_hdl_t* hdl) {
  return PARMCI_Test(hdl);
}

#pragma weak ARMCI_WaitAll
int ARMCI_WaitAll(void) {
  return PARMCI_WaitAll();
}

#pragma weak ARMCI_NbPut
int ARMCI_NbPut(void *src, void *dst, int bytes, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbPut(src, dst, bytes, proc, hdl);
}

#pragma weak ARMCI_NbGet
int ARMCI_NbGet(void *src, void *dst, int bytes, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbGet(src, dst, bytes, proc, hdl);
}

#pragma weak ARMCI_NbAcc
int ARMCI_NbAcc(int datatype, void *scale, void *src, void *dst, int bytes, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbAcc(datatype, scale, src, dst, bytes, proc, hdl);
}

#pragma weak ARMCI_NbPutS
int ARMCI_NbPutS(void *src_ptr, int src_stride_ar[], void *dst_ptr, int dst_stride_ar[], int count[], int stride_levels, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbPutS(src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels, proc, hdl);
}

#pragma weak ARMCI_NbGetS
int ARMCI_NbGetS(void *src_ptr, int src_stride_ar[], void *dst_ptr, int dst_stride_ar[], int count[], int stride_levels, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbGetS(src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels, proc, hdl);
}

#pragma weak ARMCI_NbAccS
int ARMCI_NbAccS(int datatype, void *scale, void *src_ptr, int src_stride_ar[], void *dst_ptr, int dst_stride_ar[], int count[], int stride_levels, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbAccS(datatype, scale, src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels, proc, hdl);
}

#pragma weak ARMCI_NbPutV
int ARMCI_NbPutV(armci_giov_t *iov, int iov_len, int proc, armci_hdl_t* handle) {
  return PARMCI_NbPutV(iov, iov_len, proc, handle);
}

#pragma weak ARMCI_NbGetV
int ARMCI_NbGetV(armci_giov_t *iov, int iov_len, int proc, armci_hdl_t* handle) {
  return PARMCI_NbGetV(iov, iov_len, proc, handle);
}

#pragma weak ARMCI_NbAccV
int ARMCI_NbAccV(int datatype, void *scale, armci_giov_t *iov, int iov_len, int proc, armci_hdl_t* handle) {
  return PARMCI_NbAccV(datatype, scale, iov, iov_len, proc, handle);
}

#pragma weak ARMCI_PutValueInt
int ARMCI_PutValueInt(int src, void *dst, int proc) {
  return PARMCI_PutValueInt(src, dst, proc);
}

#pragma weak ARMCI_PutValueLong
int ARMCI_PutValueLong(long src, void *dst, int proc) {
  return PARMCI_PutValueLong(src, dst, proc);
}

#pragma weak ARMCI_PutValueFloat
int ARMCI_PutValueFloat(float src, void *dst, int proc) {
  return PARMCI_PutValueFloat(src, dst, proc);
}

#pragma weak ARMCI_PutValueDouble
int ARMCI_PutValueDouble(double src, void *dst, int proc) {
  return PARMCI_PutValueDouble(src, dst, proc);
}

#pragma weak ARMCI_NbPutValueInt
int ARMCI_NbPutValueInt(int src, void *dst, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbPutValueInt(src, dst, proc, hdl);
}

#pragma weak ARMCI_NbPutValueLong
int ARMCI_NbPutValueLong(long src, void *dst, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbPutValueLong(src, dst, proc, hdl);
}

#pragma weak ARMCI_NbPutValueFloat
int ARMCI_NbPutValueFloat(float src, void *dst, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbPutValueFloat(src, dst, proc, hdl);
}

#pragma weak ARMCI_NbPutValueDouble
int ARMCI_NbPutValueDouble(double src, void *dst, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbPutValueDouble(src, dst, proc, hdl);
}

#pragma weak ARMCI_GetValueInt
int ARMCI_GetValueInt(void *src, int proc) {
  return PARMCI_GetValueInt(src, proc);
}

#pragma weak ARMCI_GetValueLong
long ARMCI_GetValueLong(void *src, int proc) {
  return PARMCI_GetValueLong(src, proc);
}

#pragma weak ARMCI_GetValueFloat
float ARMCI_GetValueFloat(void *src, int proc) {
  return PARMCI_GetValueFloat(src, proc);
}

#pragma weak ARMCI_GetValueDouble
double ARMCI_GetValueDouble(void *src, int proc) {
  return PARMCI_GetValueDouble(src, proc);
}

#pragma weak ARMCI_Create_mutexes
int ARMCI_Create_mutexes(int count) {
  return PARMCI_Create_mutexes(count);
}

#pragma weak ARMCI_Destroy_mutexes
int ARMCI_Destroy_mutexes(void) {
  return PARMCI_Destroy_mutexes();
}

#pragma weak ARMCI_Lock
void ARMCI_Lock(int mutex, int proc) {
  PARMCI_Lock(mutex, proc);
  return;
}

#pragma weak ARMCI_Unlock
void ARMCI_Unlock(int mutex, int proc) {
  PARMCI_Unlock(mutex, proc);
  return;
}

#pragma weak ARMCI_Rmw
int ARMCI_Rmw(int op, void *ploc, void *prem, int value, int proc) {
  return PARMCI_Rmw(op, ploc, prem, value, proc);
}

#pragma weak armci_msg_barrier
void armci_msg_barrier(void) {
  parmci_msg_barrier();
  return;
}

#pragma weak armci_msg_group_barrier
void armci_msg_group_barrier(ARMCI_Group *group) {
  parmci_msg_group_barrier(group);
  return;
}

#endif
