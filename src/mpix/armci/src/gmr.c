/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include <armci.h>
#include <armcix.h>
#include <armci_internals.h>
#include <debug.h>
#include <gmr.h>


/** Linked list of shared memory regions.
  */
gmr_t *gmr_list = NULL;


/** Create a distributed shared memory region. Collective on ARMCI group.
  *
  * @param[in]  local_size Size of the local slice of the memory region.
  * @param[out] base_ptrs  Array of base pointers for each process in group.
  * @param[in]  group      Group on which to perform allocation.
  * @return                Pointer to the memory region object.
  */
gmr_t *gmr_create(int local_size, void **base_ptrs, ARMCI_Group *group) {
  int           i, aggregate_size;
  int           alloc_me, alloc_nproc;
  int           world_me, world_nproc;
  MPI_Group     world_group, alloc_group;
  gmr_t        *mreg;
  gmr_slice_t  *alloc_slices, gmr_slice;

  ARMCII_Assert(local_size >= 0);
  ARMCII_Assert(group != NULL);

  MPI_Comm_rank(group->comm, &alloc_me);
  MPI_Comm_size(group->comm, &alloc_nproc);
  MPI_Comm_rank(ARMCI_GROUP_WORLD.comm, &world_me);
  MPI_Comm_size(ARMCI_GROUP_WORLD.comm, &world_nproc);

  mreg = malloc(sizeof(gmr_t));
  ARMCII_Assert(mreg != NULL);

  mreg->slices = malloc(sizeof(gmr_slice_t)*world_nproc);
  ARMCII_Assert(mreg->slices != NULL);
  alloc_slices = malloc(sizeof(gmr_slice_t)*alloc_nproc);
  ARMCII_Assert(alloc_slices != NULL);

  mreg->group          = *group; /* NOTE: I think it is invalid in GA/ARMCI to
                                    free a group before its allocations.  If
                                    this is not the case, then assignment here
                                    is incorrect and this should really
                                    duplicated the group (communicator). */

  mreg->nslices        = world_nproc;
  mreg->access_mode    = ARMCIX_MODE_ALL;
  mreg->lock_state     = GMR_LOCK_UNLOCKED;
  mreg->dla_lock_count = 0;
  mreg->prev           = NULL;
  mreg->next           = NULL;

  /* Allocate my slice of the GMR */
  alloc_slices[alloc_me].size = local_size;

  if (local_size == 0) {
    alloc_slices[alloc_me].base = NULL;
  } else {
    MPI_Alloc_mem(local_size, MPI_INFO_NULL, &(alloc_slices[alloc_me].base));
    ARMCII_Assert(alloc_slices[alloc_me].base != NULL);
  }

  /* Debugging: Zero out shared memory if enabled */
  if (ARMCII_GLOBAL_STATE.debug_alloc && local_size > 0) {
    ARMCII_Assert(alloc_slices[alloc_me].base != NULL);
    ARMCII_Bzero(alloc_slices[alloc_me].base, local_size);
  }

  /* All-to-all on <base, size> to build up slices vector */
  gmr_slice = alloc_slices[alloc_me];
  MPI_Allgather(  &gmr_slice, sizeof(gmr_slice_t), MPI_BYTE,
                 alloc_slices, sizeof(gmr_slice_t), MPI_BYTE, group->comm);

  /* Check for a global size 0 allocation */
  for (i = aggregate_size = 0; i < alloc_nproc; i++) {
    aggregate_size += alloc_slices[i].size;
  }

  /* Everyone asked for 0 bytes, return a NULL vector */
  if (aggregate_size == 0) {
    free(alloc_slices);
    free(mreg->slices);
    free(mreg);

    for (i = 0; i < alloc_nproc; i++)
      base_ptrs[i] = NULL;

    return NULL;
  }

  MPI_Win_create(alloc_slices[alloc_me].base, local_size, 1, MPI_INFO_NULL, group->comm, &mreg->window);

  /* Populate the base pointers array */
  for (i = 0; i < alloc_nproc; i++)
    base_ptrs[i] = alloc_slices[i].base;

  /* We have to do lookup on global ranks, so shovel the contents of
     alloc_slices into the mreg->slices array which is indexed by global rank. */
  memset(mreg->slices, 0, sizeof(gmr_slice_t)*world_nproc);

  MPI_Comm_group(ARMCI_GROUP_WORLD.comm, &world_group);
  MPI_Comm_group(group->comm, &alloc_group);

  for (i = 0; i < alloc_nproc; i++) {
    int world_rank;
    MPI_Group_translate_ranks(alloc_group, 1, &i, world_group, &world_rank);
    mreg->slices[world_rank] = alloc_slices[i];
  }

  free(alloc_slices);
  MPI_Group_free(&world_group);
  MPI_Group_free(&alloc_group);

  /* Create the RMW mutex: Keeps RMW operations atomic wrt each other */
  mreg->rmw_mutex = ARMCIX_Create_mutexes_hdl(1, group);

  /* Append the new region onto the region list */
  if (gmr_list == NULL) {
    gmr_list = mreg;

  } else {
    gmr_t *parent = gmr_list;

    while (parent->next != NULL)
      parent = parent->next;

    parent->next = mreg;
    mreg->prev   = parent;
  }

  return mreg;
}


/** Destroy/free a shared memory region.
  *
  * @param[in] ptr   Pointer within range of the segment (e.g. base pointer).
  * @param[in] group Group on which to perform the free.
  */
void gmr_destroy(gmr_t *mreg, ARMCI_Group *group) {
  int   search_proc_in, search_proc_out, search_proc_out_grp;
  void *search_base;
  int   alloc_me, alloc_nproc;
  int   world_me, world_nproc;

  MPI_Comm_rank(group->comm, &alloc_me);
  MPI_Comm_size(group->comm, &alloc_nproc);
  MPI_Comm_rank(ARMCI_GROUP_WORLD.comm, &world_me);
  MPI_Comm_size(ARMCI_GROUP_WORLD.comm, &world_nproc);

  /* All-to-all exchange of a <base address, proc> pair.  This is so that we
   * can support passing NULL into ARMCI_Free() which is permitted when a
   * process allocates 0 bytes.  Unfortunately, in this case we still need to
   * identify the mem region and free it.
   */

  if (mreg == NULL)
    search_proc_in = -1;
  else {
    search_proc_in = world_me;
    search_base    = mreg->slices[world_me].base;
  }

  /* Collectively decide on who will provide the base address */
  MPI_Allreduce(&search_proc_in, &search_proc_out, 1, MPI_INT, MPI_MAX, group->comm);

  /* Everyone passed NULL.  Nothing to free. */
  if (search_proc_out < 0)
    return;

  /* Translate world rank to group rank */
  search_proc_out_grp = ARMCII_Translate_absolute_to_group(group->comm, search_proc_out);

  /* Broadcast the base address */
  MPI_Bcast(&search_base, sizeof(void*), MPI_BYTE, search_proc_out_grp, group->comm);

  /* If we were passed NULL, look up the mem region using the <base, proc> pair */
  if (mreg == NULL)
    mreg = gmr_lookup(search_base, search_proc_out);

  /* If it's still not found, the user may have passed the wrong group */
  ARMCII_Assert_msg(mreg != NULL, "Could not locate the desired allocation");

  switch (mreg->lock_state) {
    case GMR_LOCK_UNLOCKED:
      break;
    case GMR_LOCK_DLA:
      ARMCII_Warning("Releasing direct local access before freeing shared allocation\n");
      gmr_dla_unlock(mreg);
      break;
    default:
      ARMCII_Error("Unable to free locked memory region (%d)\n", mreg->lock_state);
  }

  /* Remove from the list of mem regions */
  if (mreg->prev == NULL) {
    ARMCII_Assert(gmr_list == mreg);
    gmr_list = mreg->next;

    if (mreg->next != NULL)
      mreg->next->prev = NULL;

  } else {
    mreg->prev->next = mreg->next;
    if (mreg->next != NULL)
      mreg->next->prev = mreg->prev;
  }

  /* Destroy the window and free all buffers */
  MPI_Win_free(&mreg->window);

  if (mreg->slices[world_me].base != NULL)
    MPI_Free_mem(mreg->slices[world_me].base);

  free(mreg->slices);
  ARMCIX_Destroy_mutexes_hdl(mreg->rmw_mutex);

  free(mreg);
}


/** Destroy all memory regions (called by finalize).
  *
  * @return Number of mem regions destroyed.
  */
int gmr_destroy_all(void) {
  int count = 0;

  while (gmr_list != NULL) {
    gmr_destroy(gmr_list, &gmr_list->group);
    count++;
  }

  return count;
}

/** Lookup a shared memory region using an address and process id.
  *
  * @param[in] ptr  Pointer within range of the segment (e.g. base pointer).
  * @param[in] proc Process on which the data lives.
  * @return         Pointer to the mem region object.
  */
gmr_t *gmr_lookup(void *ptr, int proc) {
  gmr_t *mreg;

  mreg = gmr_list;

  while (mreg != NULL) {
    ARMCII_Assert(proc < mreg->nslices);

    if (proc < mreg->nslices) {
      const uint8_t *base = mreg->slices[proc].base;
      const int      size = mreg->slices[proc].size;

      if ((uint8_t*) ptr >= base && (uint8_t*) ptr < base + size)
        break;
    }

    mreg = mreg->next;
  }

  return mreg;
}


/** One-sided put operation.  Source buffer must be private.
  *
  * @param[in] mreg   Memory region
  * @param[in] src    Source address (local)
  * @param[in] dst    Destination address (remote)
  * @param[in] size   Number of bytes to transfer
  * @param[in] proc   Absolute process id of target process
  * @return           0 on success, non-zero on failure
  */
int gmr_put(gmr_t *mreg, void *src, void *dst, int size, int proc) {
  ARMCII_Assert_msg(src != NULL, "Invalid local address");
  return gmr_put_typed(mreg, src, size, MPI_BYTE, dst, size, MPI_BYTE, proc);
}


/** One-sided put operation with type arguments.  Source buffer must be private.
  *
  * @param[in] mreg      Memory region
  * @param[in] src       Address of source data
  * @param[in] src_count Number of elements of the given type at the source
  * @param[in] src_type  MPI datatype of the source elements
  * @param[in] dst       Address of destination buffer
  * @param[in] dst_count Number of elements of the given type at the destination
  * @param[in] src_type  MPI datatype of the destination elements
  * @param[in] size      Number of bytes to transfer
  * @param[in] proc      Absolute process id of target process
  * @return              0 on success, non-zero on failure
  */
int gmr_put_typed(gmr_t *mreg, void *src, int src_count, MPI_Datatype src_type,
    void *dst, int dst_count, MPI_Datatype dst_type, int proc) {

  int disp, grp_proc;
  MPI_Aint lb, extent;

  grp_proc = ARMCII_Translate_absolute_to_group(mreg->group.comm, proc);
  ARMCII_Assert(grp_proc >= 0);

  // Calculate displacement from beginning of the window
  if (dst == MPI_BOTTOM) 
    disp = 0;
  else
    disp = (int) ((uint8_t*)dst - (uint8_t*)mreg->slices[proc].base);

  // Perform checks
  MPI_Type_get_true_extent(dst_type, &lb, &extent);
  ARMCII_Assert(mreg->lock_state != GMR_LOCK_UNLOCKED);
  ARMCII_Assert_msg(disp >= 0 && disp < mreg->slices[proc].size, "Invalid remote address");
  ARMCII_Assert_msg(disp + dst_count*extent <= mreg->slices[proc].size, "Transfer is out of range");

  MPI_Put(src, src_count, src_type, grp_proc, disp, dst_count, dst_type, mreg->window);

  return 0;
}


/** One-sided get operation.  Destination buffer must be private.
  *
  * @param[in] mreg   Memory region
  * @param[in] src    Source address (remote)
  * @param[in] dst    Destination address (local)
  * @param[in] size   Number of bytes to transfer
  * @param[in] proc   Absolute process id of target process
  * @return           0 on success, non-zero on failure
  */
int gmr_get(gmr_t *mreg, void *src, void *dst, int size, int proc) {
  ARMCII_Assert_msg(dst != NULL, "Invalid local address");
  return gmr_get_typed(mreg, src, size, MPI_BYTE, dst, size, MPI_BYTE, proc);
}


/** One-sided get operation with type arguments.  Destination buffer must be private.
  *
  * @param[in] mreg      Memory region
  * @param[in] src       Address of source data
  * @param[in] src_count Number of elements of the given type at the source
  * @param[in] src_type  MPI datatype of the source elements
  * @param[in] dst       Address of destination buffer
  * @param[in] dst_count Number of elements of the given type at the destination
  * @param[in] src_type  MPI datatype of the destination elements
  * @param[in] size      Number of bytes to transfer
  * @param[in] proc      Absolute process id of target process
  * @return              0 on success, non-zero on failure
  */
int gmr_get_typed(gmr_t *mreg, void *src, int src_count, MPI_Datatype src_type,
    void *dst, int dst_count, MPI_Datatype dst_type, int proc) {

  int disp, grp_proc;
  MPI_Aint lb, extent;

  grp_proc = ARMCII_Translate_absolute_to_group(mreg->group.comm, proc);
  ARMCII_Assert(grp_proc >= 0);

  // Calculate displacement from beginning of the window
  if (src == MPI_BOTTOM) 
    disp = 0;
  else
    disp = (int) ((uint8_t*)src - (uint8_t*)mreg->slices[proc].base);

  // Perform checks
  MPI_Type_get_true_extent(src_type, &lb, &extent);
  ARMCII_Assert(mreg->lock_state != GMR_LOCK_UNLOCKED);
  ARMCII_Assert_msg(disp >= 0 && disp < mreg->slices[proc].size, "Invalid remote address");
  ARMCII_Assert_msg(disp + src_count*extent <= mreg->slices[proc].size, "Transfer is out of range");

  MPI_Get(dst, dst_count, dst_type, grp_proc, disp, src_count, src_type, mreg->window);

  return 0;
}


/** One-sided accumulate operation.  Source buffer must be private.
  *
  * @param[in] mreg     Memory region
  * @param[in] src      Source address (local)
  * @param[in] dst      Destination address (remote)
  * @param[in] type     MPI type of the given buffers
  * @param[in] count    Number of elements of the given type to transfer
  * @param[in] proc     Absolute process id of the target
  * @return             0 on success, non-zero on failure
  */
int gmr_accumulate(gmr_t *mreg, void *src, void *dst, int count, MPI_Datatype type, int proc) {
  ARMCII_Assert_msg(src != NULL, "Invalid local address");
  return gmr_accumulate_typed(mreg, src, count, type, dst, count, type, proc);
}


/** One-sided accumulate operation with typed arguments.  Source buffer must be private.
  *
  * @param[in] mreg      Memory region
  * @param[in] src       Address of source data
  * @param[in] src_count Number of elements of the given type at the source
  * @param[in] src_type  MPI datatype of the source elements
  * @param[in] dst       Address of destination buffer
  * @param[in] dst_count Number of elements of the given type at the destination
  * @param[in] src_type  MPI datatype of the destination elements
  * @param[in] size      Number of bytes to transfer
  * @param[in] proc      Absolute process id of target process
  * @return              0 on success, non-zero on failure
  */
int gmr_accumulate_typed(gmr_t *mreg, void *src, int src_count, MPI_Datatype src_type,
    void *dst, int dst_count, MPI_Datatype dst_type, int proc) {

  int disp, grp_proc;
  MPI_Aint lb, extent;

  grp_proc = ARMCII_Translate_absolute_to_group(mreg->group.comm, proc);
  ARMCII_Assert(grp_proc >= 0);

  // Calculate displacement from beginning of the window
  if (dst == MPI_BOTTOM) 
    disp = 0;
  else
    disp = (int) ((uint8_t*)dst - (uint8_t*)mreg->slices[proc].base);

  // Perform checks
  MPI_Type_get_true_extent(dst_type, &lb, &extent);
  ARMCII_Assert(mreg->lock_state != GMR_LOCK_UNLOCKED);
  ARMCII_Assert_msg(disp >= 0 && disp < mreg->slices[proc].size, "Invalid remote address");
  ARMCII_Assert_msg(disp + dst_count*extent <= mreg->slices[proc].size, "Transfer is out of range");

  MPI_Accumulate(src, src_count, src_type, grp_proc, disp, dst_count, dst_type, MPI_SUM, mreg->window);

  return 0;
}

/** Lock a memory region so that one-sided operations can be performed.
  *
  * @param[in] mreg     Memory region
  * @param[in] mode     Lock mode (exclusive, shared, etc...)
  * @param[in] proc     Absolute process id of the target
  * @return             0 on success, non-zero on failure
  */
void gmr_lock(gmr_t *mreg, int proc) {
  int grp_proc = ARMCII_Translate_absolute_to_group(mreg->group.comm, proc);
  int grp_me   = ARMCII_Translate_absolute_to_group(mreg->group.comm, ARMCI_GROUP_WORLD.rank);
  int lock_assert, lock_mode;

  ARMCII_Assert(grp_proc >= 0 && grp_me >= 0);
  ARMCII_Assert(mreg->lock_state == GMR_LOCK_UNLOCKED || mreg->lock_state == GMR_LOCK_DLA);

  /* Check for active DLA and suspend if needed */
  if (mreg->lock_state == GMR_LOCK_DLA) {
    ARMCII_Assert(grp_me == mreg->lock_target);
    MPI_Win_unlock(mreg->lock_target, mreg->window);
    mreg->lock_state = GMR_LOCK_DLA_SUSP;
  }

  if (   mreg->access_mode & ARMCIX_MODE_CONFLICT_FREE 
      && mreg->access_mode & ARMCIX_MODE_NO_LOAD_STORE )
  {
    /* Only non-conflicting RMA accesses allowed.
       Shared and exclusive locks. */
    lock_assert = MPI_MODE_NOCHECK;
    lock_mode   = MPI_LOCK_SHARED;
  } else if (mreg->access_mode & ARMCIX_MODE_CONFLICT_FREE) {
    /* Non-conflicting RMA and local accesses allowed.
       Shared and exclusive locks. */
    lock_assert = 0;
    lock_mode   = MPI_LOCK_SHARED;
  } else {
    /* Conflicting RMA and local accesses allowed.
       Exclusive locks. */
    lock_assert = 0;
    lock_mode   = MPI_LOCK_EXCLUSIVE;
  }

  MPI_Win_lock(lock_mode, grp_proc, lock_assert, mreg->window);

  if (lock_mode == MPI_LOCK_EXCLUSIVE)
    mreg->lock_state = GMR_LOCK_EXCLUSIVE;
  else
    mreg->lock_state = GMR_LOCK_SHARED;

  mreg->lock_target = grp_proc;
}


/** Unlock a memory region.
  *
  * @param[in] mreg     Memory region
  * @param[in] proc     Absolute process id of the target
  * @return             0 on success, non-zero on failure
  */
void gmr_unlock(gmr_t *mreg, int proc) {
  int grp_proc = ARMCII_Translate_absolute_to_group(mreg->group.comm, proc);
  int grp_me   = ARMCII_Translate_absolute_to_group(mreg->group.comm, ARMCI_GROUP_WORLD.rank);

  ARMCII_Assert(grp_proc >= 0 && grp_me >= 0);
  ARMCII_Assert(mreg->lock_state == GMR_LOCK_EXCLUSIVE || mreg->lock_state == GMR_LOCK_SHARED);
  ARMCII_Assert(mreg->lock_target == grp_proc);

  /* Check if DLA is suspended and needs to be resumed */
  if (mreg->dla_lock_count > 0) {

    if (mreg->lock_state != GMR_LOCK_EXCLUSIVE || mreg->lock_target != grp_me) {
      MPI_Win_unlock(grp_proc, mreg->window);
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, grp_me, 0, mreg->window); // FIXME: NOCHECK here?
    }

    mreg->lock_state = GMR_LOCK_DLA;
    mreg->lock_target= grp_me;
  }
  else {
    MPI_Win_unlock(grp_proc, mreg->window);
    mreg->lock_state = GMR_LOCK_UNLOCKED;
  }
}


/** Lock a memory region so that load/store operations can be performed.
  *
  * @param[in] mreg     Memory region
  * @param[in] mode     Lock mode (exclusive, shared, etc...)
  * @return             0 on success, non-zero on failure
  */
void gmr_dla_lock(gmr_t *mreg) {
  int grp_proc = ARMCII_Translate_absolute_to_group(mreg->group.comm, ARMCI_GROUP_WORLD.rank);

  ARMCII_Assert(grp_proc >= 0);
  ARMCII_Assert(mreg->lock_state == GMR_LOCK_UNLOCKED || mreg->lock_state == GMR_LOCK_DLA);
  ARMCII_Assert_msg((mreg->access_mode & ARMCIX_MODE_NO_LOAD_STORE) == 0,
      "Direct local access is not allowed in the current access mode");

  if (mreg->dla_lock_count == 0) {
    ARMCII_Assert(mreg->lock_state == GMR_LOCK_UNLOCKED);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, grp_proc, 0, mreg->window);

    mreg->lock_state = GMR_LOCK_DLA;
    mreg->lock_target= grp_proc;
  }

  ARMCII_Assert(mreg->lock_state == GMR_LOCK_DLA);
  mreg->dla_lock_count++;
}


/** Unlock a memory region that was locked for direct local access.
  *
  * @param[in] mreg     Memory region
  */
void gmr_dla_unlock(gmr_t *mreg) {
  int grp_proc = ARMCII_Translate_absolute_to_group(mreg->group.comm, ARMCI_GROUP_WORLD.rank);

  ARMCII_Assert(grp_proc >= 0);
  ARMCII_Assert(mreg->lock_state == GMR_LOCK_DLA);
  ARMCII_Assert_msg((mreg->access_mode & ARMCIX_MODE_NO_LOAD_STORE) == 0,
      "Direct local access is not allowed in the current access mode");

  mreg->dla_lock_count--;

  if (mreg->dla_lock_count == 0) {
    MPI_Win_unlock(grp_proc, mreg->window);
    mreg->lock_state = GMR_LOCK_UNLOCKED;
  }
}
