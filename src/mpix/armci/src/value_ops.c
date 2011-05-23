/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <armci.h>
#include <debug.h>

/* Put value operations */

int ARMCI_PutValueInt(int src, void *dst, int proc) {
  return ARMCI_Put(&src, dst, sizeof(int), proc);
}

int ARMCI_PutValueLong(long src, void *dst, int proc) {
  return ARMCI_Put(&src, dst, sizeof(long), proc);
}

int ARMCI_PutValueFloat(float src, void *dst, int proc) {
  return ARMCI_Put(&src, dst, sizeof(float), proc);
}

int ARMCI_PutValueDouble(double src, void *dst, int proc) {
  return ARMCI_Put(&src, dst, sizeof(double), proc);
}

/* Non-blocking put operations */

int ARMCI_NbPutValueInt(int src, void *dst, int proc, armci_hdl_t *hdl) {
  return ARMCI_NbPut(&src, dst, sizeof(int), proc, hdl);
}

int ARMCI_NbPutValueLong(long src, void *dst, int proc, armci_hdl_t *hdl) {
  return ARMCI_NbPut(&src, dst, sizeof(long), proc, hdl);
}

int ARMCI_NbPutValueFloat(float src, void *dst, int proc, armci_hdl_t *hdl) {
  return ARMCI_NbPut(&src, dst, sizeof(float), proc, hdl);
}

int ARMCI_NbPutValueDouble(double src, void *dst, int proc, armci_hdl_t *hdl) {
  return ARMCI_NbPut(&src, dst, sizeof(double), proc, hdl);
}

/* Get value operations */

int    ARMCI_GetValueInt(void *src, int proc) {
  int val;
  ARMCI_Get(src, &val, sizeof(int), proc);
  return val;
}

long   ARMCI_GetValueLong(void *src, int proc) {
  long val;
  ARMCI_Get(src, &val, sizeof(long), proc);
  return val;
}

float  ARMCI_GetValueFloat(void *src, int proc) {     
  float val;
  ARMCI_Get(src, &val, sizeof(float), proc);
  return val;
}

double ARMCI_GetValueDouble(void *src, int proc) {     
  double val;
  ARMCI_Get(src, &val, sizeof(double), proc);
  return val;
}
