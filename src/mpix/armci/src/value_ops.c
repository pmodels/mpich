/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <armci.h>
#include <debug.h>

/* Put value operations */

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_PutValueInt = PARMCI_PutValueInt
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_PutValueInt ARMCI_PutValueInt
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_PutValueInt as PARMCI_PutValueInt
#endif
/* -- end weak symbols block -- */

int PARMCI_PutValueInt(int src, void *dst, int proc) {
  return PARMCI_Put(&src, dst, sizeof(int), proc);
}

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_PutValueLong = PARMCI_PutValueLong
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_PutValueLong ARMCI_PutValueLong
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_PutValueLong as PARMCI_PutValueLong
#endif
/* -- end weak symbols block -- */

int PARMCI_PutValueLong(long src, void *dst, int proc) {
  return PARMCI_Put(&src, dst, sizeof(long), proc);
}

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_PutValueFloat = PARMCI_PutValueFloat
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_PutValueFloat ARMCI_PutValueFloat
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_PutValueFloat as PARMCI_PutValueFloat
#endif
/* -- end weak symbols block -- */

int PARMCI_PutValueFloat(float src, void *dst, int proc) {
  return PARMCI_Put(&src, dst, sizeof(float), proc);
}

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_PutValueDouble = PARMCI_PutValueDouble
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_PutValueDouble ARMCI_PutValueDouble
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_PutValueDouble as PARMCI_PutValueDouble
#endif
/* -- end weak symbols block -- */

int PARMCI_PutValueDouble(double src, void *dst, int proc) {
  return PARMCI_Put(&src, dst, sizeof(double), proc);
}

/* Non-blocking put operations */

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_NbPutValueInt = PARMCI_NbPutValueInt
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_NbPutValueInt ARMCI_NbPutValueInt
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_NbPutValueInt as PARMCI_NbPutValueInt
#endif
/* -- end weak symbols block -- */

int PARMCI_NbPutValueInt(int src, void *dst, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbPut(&src, dst, sizeof(int), proc, hdl);
}

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_NbPutValueLong = PARMCI_NbPutValueLong
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_NbPutValueLong ARMCI_NbPutValueLong
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_NbPutValueLong as PARMCI_NbPutValueLong
#endif
/* -- end weak symbols block -- */

int PARMCI_NbPutValueLong(long src, void *dst, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbPut(&src, dst, sizeof(long), proc, hdl);
}

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_NbPutValueFloat = PARMCI_NbPutValueFloat
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_NbPutValueFloat ARMCI_NbPutValueFloat
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_NbPutValueFloat as PARMCI_NbPutValueFloat
#endif
/* -- end weak symbols block -- */

int PARMCI_NbPutValueFloat(float src, void *dst, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbPut(&src, dst, sizeof(float), proc, hdl);
}

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_NbPutValueDouble = PARMCI_NbPutValueDouble
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_NbPutValueDouble ARMCI_NbPutValueDouble
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_NbPutValueDouble as PARMCI_NbPutValueDouble
#endif
/* -- end weak symbols block -- */

int PARMCI_NbPutValueDouble(double src, void *dst, int proc, armci_hdl_t *hdl) {
  return PARMCI_NbPut(&src, dst, sizeof(double), proc, hdl);
}

/* Get value operations */

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_GetValueInt = PARMCI_GetValueInt
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_GetValueInt ARMCI_GetValueInt
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_GetValueInt as PARMCI_GetValueInt
#endif
/* -- end weak symbols block -- */

int    PARMCI_GetValueInt(void *src, int proc) {
  int val;
  PARMCI_Get(src, &val, sizeof(int), proc);
  return val;
}

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_GetValueLong = PARMCI_GetValueLong
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_GetValueLong ARMCI_GetValueLong
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_GetValueLong as PARMCI_GetValueLong
#endif
/* -- end weak symbols block -- */

long   PARMCI_GetValueLong(void *src, int proc) {
  long val;
  PARMCI_Get(src, &val, sizeof(long), proc);
  return val;
}

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_GetValueFloat = PARMCI_GetValueFloat
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_GetValueFloat ARMCI_GetValueFloat
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_GetValueFloat as PARMCI_GetValueFloat
#endif
/* -- end weak symbols block -- */

float  PARMCI_GetValueFloat(void *src, int proc) {     
  float val;
  PARMCI_Get(src, &val, sizeof(float), proc);
  return val;
}

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_GetValueDouble = PARMCI_GetValueDouble
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_GetValueDouble ARMCI_GetValueDouble
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_GetValueDouble as PARMCI_GetValueDouble
#endif
/* -- end weak symbols block -- */

double PARMCI_GetValueDouble(void *src, int proc) {     
  double val;
  PARMCI_Get(src, &val, sizeof(double), proc);
  return val;
}
