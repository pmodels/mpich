/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include <armci.h>
#include <armcix.h>
#include <armci_internals.h>
#include <debug.h>
#include <gmr.h>


/** Declare the start of a local access epoch.  This allows direct access to
  * data in local memory.
  *
  * @param[in] ptr Pointer to the allocation that will be accessed directly 
  */
void ARMCI_Access_begin(void *ptr) {
  gmr_t *mreg;

  mreg = gmr_lookup(ptr, ARMCI_GROUP_WORLD.rank);
  ARMCII_Assert_msg(mreg != NULL, "Invalid remote pointer");

  ARMCII_Assert_msg((mreg->access_mode & ARMCIX_MODE_NO_LOAD_STORE) == 0,
      "Direct local access is not permitted in the current access mode");

  gmr_dla_lock(mreg);
}


/** Declare the end of a local access epoch.
  *
  * \note MPI-2 does not allow multiple locks at once, so you can have only one
  * access epoch open at a time and cannot do put/get/acc while in an access
  * region.
  *
  * @param[in] ptr Pointer to the allocation that was accessed directly 
  */
void ARMCI_Access_end(void *ptr) {
  gmr_t *mreg;

  mreg = gmr_lookup(ptr, ARMCI_GROUP_WORLD.rank);
  ARMCII_Assert_msg(mreg != NULL, "Invalid remote pointer");

  gmr_dla_unlock(mreg);
}


/** Set the acess mode for the given allocation.  Collective across the
  * allocation's group.  Waits for all processes, finishes all communication,
  * and then sets the new access mode.
  *
  * @param[in] new_mode The new access mode.
  * @param[in] ptr      Pointer within the allocation.
  * @return             Zero upon success, error code otherwise.
  */
int ARMCIX_Mode_set(int new_mode, void *ptr, ARMCI_Group *group) {
  gmr_t *mreg;

  mreg = gmr_lookup(ptr, ARMCI_GROUP_WORLD.rank);
  ARMCII_Assert_msg(mreg != NULL, "Invalid remote pointer");

  ARMCII_Assert(group->comm == mreg->group.comm);

  ARMCII_Assert_msg(mreg->lock_state != GMR_LOCK_DLA,
      "Cannot change the access mode; window is locked for local access.");
  ARMCII_Assert_msg(mreg->lock_state == GMR_LOCK_UNLOCKED,
      "Cannot change the access mode on a window that is locked.");

  // Wait for all processes to complete any outstanding communication before we
  // do the mode switch
  MPI_Barrier(mreg->group.comm);

  mreg->access_mode = new_mode;

  return 0;
}


/** Query the access mode for the given allocation.  Non-collective.
  *
  * @param[in] ptr      Pointer within the allocation.
  * @return             Current access mode.
  */
int ARMCIX_Mode_get(void *ptr) {
  gmr_t *mreg;

  mreg = gmr_lookup(ptr, ARMCI_GROUP_WORLD.rank);
  ARMCII_Assert_msg(mreg != NULL, "Invalid remote pointer");

  return mreg->access_mode;
}


/** One-sided get operation.
  *
  * @param[in] src    Source address (remote)
  * @param[in] dst    Destination address (local)
  * @param[in] size   Number of bytes to transfer
  * @param[in] target Process id to target
  * @return           0 on success, non-zero on failure
  */
int ARMCI_Get(void *src, void *dst, int size, int target) {
  gmr_t *src_mreg, *dst_mreg;

  src_mreg = gmr_lookup(src, target);
  dst_mreg = gmr_lookup(dst, ARMCI_GROUP_WORLD.rank);

  ARMCII_Assert_msg(src_mreg != NULL, "Invalid remote pointer");

  /* Local operation */
  if (target == ARMCI_GROUP_WORLD.rank) {
    if (ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD) {
      if (dst_mreg) gmr_dla_lock(dst_mreg);  /* FIXME: Is this a hold-while wait?  Probably need an extra copy to be safe.. */
      gmr_dla_lock(src_mreg);
    }

    ARMCI_Copy(src, dst, size);
    
    if (ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD) {
      if (dst_mreg) gmr_dla_unlock(dst_mreg);
      gmr_dla_unlock(src_mreg);
    }
  }

  /* Origin buffer is private */
  else if (dst_mreg == NULL || ARMCII_GLOBAL_STATE.shr_buf_method == ARMCII_SHR_BUF_NOGUARD) {
    gmr_lock(src_mreg, target);
    gmr_get(src_mreg, src, dst, size, target);
    gmr_unlock(src_mreg, target);
  }

  /* COPY: Either origin and target buffers are in the same window and we can't
   * lock the same window twice (MPI semantics) or the user has requested
   * always-copy mode. */
  else {
    void *dst_buf;

    MPI_Alloc_mem(size, MPI_INFO_NULL, &dst_buf);
    ARMCII_Assert(dst_buf != NULL);

    gmr_lock(src_mreg, target);
    gmr_get(src_mreg, src, dst_buf, size, target);
    gmr_unlock(src_mreg, target);

    gmr_dla_lock(dst_mreg);
    ARMCI_Copy(dst_buf, dst, size);
    MPI_Free_mem(dst_buf);
    gmr_dla_unlock(dst_mreg);
  }

  return 0;
}


/** One-sided put operation.
  *
  * @param[in] src    Source address (remote)
  * @param[in] dst    Destination address (local)
  * @param[in] size   Number of bytes to transfer
  * @param[in] target Process id to target
  * @return           0 on success, non-zero on failure
  */
int ARMCI_Put(void *src, void *dst, int size, int target) {
  gmr_t *src_mreg, *dst_mreg;

  src_mreg = gmr_lookup(src, ARMCI_GROUP_WORLD.rank);
  dst_mreg = gmr_lookup(dst, target);

  ARMCII_Assert_msg(dst_mreg != NULL, "Invalid remote pointer");

  /* Local operation */
  if (target == ARMCI_GROUP_WORLD.rank) {
    if (ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD) {
      gmr_dla_lock(dst_mreg);
      if (src_mreg) gmr_dla_lock(src_mreg);
    }

    ARMCI_Copy(src, dst, size);
    
    if (ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD) {
      gmr_dla_unlock(dst_mreg);
      if (src_mreg) gmr_dla_unlock(src_mreg);
    }
  }

  /* Origin buffer is private */
  else if (src_mreg == NULL || ARMCII_GLOBAL_STATE.shr_buf_method == ARMCII_SHR_BUF_NOGUARD) {
    gmr_lock(dst_mreg, target);
    gmr_put(dst_mreg, src, dst, size, target);
    gmr_unlock(dst_mreg, target);
  }

  /* COPY: Either origin and target buffers are in the same window and we can't
   * lock the same window twice (MPI semantics) or the user has requested
   * always-copy mode. */
  else {
    void *src_buf;

    MPI_Alloc_mem(size, MPI_INFO_NULL, &src_buf);
    ARMCII_Assert(src_buf != NULL);

    gmr_dla_lock(src_mreg);
    ARMCI_Copy(src, src_buf, size);
    gmr_dla_unlock(src_mreg);

    gmr_lock(dst_mreg, target);
    gmr_put(dst_mreg, src_buf, dst, size, target);
    gmr_unlock(dst_mreg, target);

    MPI_Free_mem(src_buf);
  }

  return 0;
}


/** One-sided accumulate operation.
  *
  * @param[in] datatype ARMCI data type for the accumulate operation (see armci.h)
  * @param[in] scale    Pointer for a scalar of type datatype that will be used to
  *                     scale values in the source buffer
  * @param[in] src      Source address (remote)
  * @param[in] dst      Destination address (local)
  * @param[in] bytes    Number of bytes to transfer
  * @param[in] proc     Process id to target
  * @return             0 on success, non-zero on failure
  */
int ARMCI_Acc(int datatype, void *scale, void *src, void *dst, int bytes, int proc) {
  void  *src_buf;
  int    count, type_size, scaled, src_is_locked = 0;
  MPI_Datatype type;
  gmr_t *src_mreg, *dst_mreg;

  src_mreg = gmr_lookup(src, ARMCI_GROUP_WORLD.rank);
  dst_mreg = gmr_lookup(dst, proc);

  ARMCII_Assert_msg(dst_mreg != NULL, "Invalid remote pointer");

  /* Prepare the input data: Apply scaling if needed and acquire the DLA lock if
   * needed.  We hold the DLA lock if (src_buf == src && src_mreg != NULL). */

  scaled = ARMCII_Buf_acc_is_scaled(datatype, scale);

  if (src_mreg && ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD) {
    gmr_dla_lock(src_mreg);
    src_is_locked = 1;
  }

  if (scaled) {
      MPI_Alloc_mem(bytes, MPI_INFO_NULL, &src_buf);
      ARMCII_Assert(src_buf != NULL);
      ARMCII_Buf_acc_scale(src, src_buf, bytes, datatype, scale);
  } else {
    src_buf = src;
  }

  /* Check if we need to copy: user requested it or same mem region */
  if (   (src_buf == src) /* buf_prepare didn't make a copy */
      && (ARMCII_GLOBAL_STATE.shr_buf_method == ARMCII_SHR_BUF_COPY || src_mreg == dst_mreg) )
  {
    MPI_Alloc_mem(bytes, MPI_INFO_NULL, &src_buf);
    ARMCII_Assert(src_buf != NULL);
    ARMCI_Copy(src, src_buf, bytes);
  }

  /* Unlock early if src_buf is a copy */
  if (src_buf != src && src_is_locked) {
    gmr_dla_unlock(src_mreg);
    src_is_locked = 0;
  }

  ARMCII_Acc_type_translate(datatype, &type, &type_size);
  count = bytes/type_size;

  ARMCII_Assert_msg(bytes % type_size == 0, 
      "Transfer size is not a multiple of the datatype size");

  /* TODO: Support a local accumulate operation more efficiently */

  gmr_lock(dst_mreg, proc);
  gmr_accumulate(dst_mreg, src_buf, dst, count, type, proc);
  gmr_unlock(dst_mreg, proc);

  if (src_is_locked) {
    gmr_dla_unlock(src_mreg);
    src_is_locked = 0;
  }

  if (src_buf != src)
    MPI_Free_mem(src_buf);

  return 0;
}


/** One-sided copy of data from the source to the destination.  Set a flag on
  * the remote process when the transfer is complete.
  *
  * @param[in] src   Source buffer
  * @param[in] dst   Destination buffer on proc
  * @param[in] size  Number of bytes to transfer
  * @param[in] flag  Address of the flag buffer on proc
  * @param[in] value Value to set the flag to
  * @param[in] proc  Process id of the target
  * @return          0 on success, non-zero on failure
  */
int ARMCI_Put_flag(void *src, void* dst, int size, int *flag, int value, int proc) {
  ARMCI_Put(src, dst, size, proc);
  ARMCI_Fence(proc);
  ARMCI_Put(&value, flag, sizeof(int), proc);

  return 0;
}
