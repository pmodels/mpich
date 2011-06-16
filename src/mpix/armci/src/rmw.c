/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <armci.h>
#include <armci_internals.h>
#include <gmr.h>
#include <debug.h>


/** Perform atomic read-modify-write on the given integer or long location and
  * return the location's original value.
  *
  * \note ARMCI RMW operations are atomic with respect to other RMW operations,
  * but not with respect to other one-sided operations (get, put, acc, etc).
  *
  * @param[in]  op    Operation to be performed:
  *                     ARMCI_FETCH_AND_ADD (int)
  *                     ARMCI_FETCH_AND_ADD_LONG
  *                     ARMCI_SWAP (int)
  *                     ARMCI_SWAP_LONG
  * @param[out] ploc  Location to store the original value.
  * @param[in]  prem  Location on which to perform atomic operation.
  * @param[in]  value Value to add to remote location (ignored for swap).
  * @param[in]  proc  Process rank for the target buffer.
  */
int ARMCI_Rmw(int op, void *ploc, void *prem, int value, int proc) {
  int           is_long;
  gmr_t *mreg;

  mreg = gmr_lookup(prem, proc);
  ARMCII_Assert_msg(mreg != NULL, "Invalid remote pointer");

  if (op == ARMCI_SWAP_LONG || op == ARMCI_FETCH_AND_ADD_LONG)
    is_long = 1;
  else
    is_long = 0;

  if (op == ARMCI_SWAP || op == ARMCI_SWAP_LONG) {
    long swap_val_l;
    int  swap_val_i;

    ARMCIX_Lock_hdl(mreg->rmw_mutex, 0, proc);
    ARMCI_Get(prem, is_long ? (void*) &swap_val_l : (void*) &swap_val_i, 
              is_long ? sizeof(long) : sizeof(int), proc);
    ARMCI_Put(ploc, prem, is_long ? sizeof(long) : sizeof(int), proc);
    ARMCIX_Unlock_hdl(mreg->rmw_mutex, 0, proc);

    if (is_long)
      *(long*) ploc = swap_val_l;
    else
      *(int*) ploc = swap_val_i;
  }

  else if (op == ARMCI_FETCH_AND_ADD || op == ARMCI_FETCH_AND_ADD_LONG) {
    long fetch_val_l, new_val_l;
    int  fetch_val_i, new_val_i;
    
    ARMCIX_Lock_hdl(mreg->rmw_mutex, 0, proc);
    ARMCI_Get(prem, is_long ? (void*) &fetch_val_l : (void*) &fetch_val_i,
              is_long ? sizeof(long) : sizeof(int), proc);
    
    if (is_long)
      new_val_l = fetch_val_l + value;
    else
      new_val_i = fetch_val_i + value;

    ARMCI_Put(is_long ? (void*) &new_val_l : (void*) &new_val_i, prem, 
              is_long ? sizeof(long) : sizeof(int), proc);
    ARMCIX_Unlock_hdl(mreg->rmw_mutex, 0, proc);

    if (is_long)
      *(long*) ploc = fetch_val_l;
    else
      *(int*) ploc = fetch_val_i;
  }

  else {
    ARMCII_Error("unsupported operation (%d)", op);
  }

  return 0;
}
