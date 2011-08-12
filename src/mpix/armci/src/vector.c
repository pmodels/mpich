/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <armci.h>
#include <armci_internals.h>
#include <debug.h>
#include <gmr.h>

#ifndef NO_USE_CTREE
#include <conflict_tree.h>
#endif


/** Check an I/O vector operation's buffers for overlap.
  *
  * @param[in] iov      Vector of transfer information.
  * @return             Logical true when regions overlap, 0 otherwise.
  */
int ARMCII_Iov_check_overlap(void **ptrs, int count, int size) {
#ifndef NO_CHECK_OVERLAP
#ifdef NO_USE_CTREE
  int i, j;

  if (!ARMCII_GLOBAL_STATE.iov_checks) return 0;

  for (i = 0; i < count; i++) {
    for (j = i+1; j < count; j++) {
      const uint8_t *ptr_1_lo = ptrs[i];
      const uint8_t *ptr_1_hi = ((uint8_t*)ptrs[i]) + size - 1;
      const uint8_t *ptr_2_lo = ptrs[j];
      const uint8_t *ptr_2_hi = ((uint8_t*)ptrs[j]) + size - 1;

      if (   (ptr_1_lo >= ptr_2_lo && ptr_1_lo <= ptr_2_hi)
          || (ptr_1_hi >= ptr_2_lo && ptr_1_hi <= ptr_2_hi)
          || (ptr_1_lo <  ptr_2_lo && ptr_1_hi >  ptr_2_hi)) {
        ARMCII_Dbg_print(DEBUG_CAT_IOV, "IOV regions overlap: [%p, %p] - [%p, %p]\n",
            ptr_1_lo, ptr_1_hi, ptr_2_lo, ptr_2_hi);
        return 1;
      }
    }
  }
#else
  int i;
  ctree_t ctree = CTREE_EMPTY;

  if (!ARMCII_GLOBAL_STATE.iov_checks) return 0;

  for (i = 0; i < count; i++) {
    int conflict = ctree_insert(&ctree, ptrs[i], ((uint8_t*)ptrs[i]) + size - 1);

    if (conflict) {
      ctree_t cnode = ctree_locate(ctree, ptrs[i], ((uint8_t*)ptrs[i]) + size - 1);

      ARMCII_Dbg_print(DEBUG_CAT_IOV, "IOV regions overlap: [%p, %p] - [%p, %p]\n",
          ptrs[i], ((uint8_t*)ptrs[i]) + size - 1, cnode->lo, cnode->hi);

      ctree_destroy(&ctree);
      return 1;
    }
  }

  ctree_destroy(&ctree);
#endif /* NO_USE_CTREE */
#endif /* NO_CHECK_OVERLAP */

  return 0;
}


/** Check if a set of pointers all corresponds to the same allocation.
  *
  * @param[in] ptrs  An array of count shared pointers valid on proc.
  * @param[in] count Size of the ptrs array.
  * @param[in] proc  Process on which the pointers are valid.
  * @return          Non-zero (true) on success, zero (false) otherwise.
  */
int ARMCII_Iov_check_same_allocation(void **ptrs, int count, int proc) {
  int i;
  gmr_t *mreg;
  void *base, *extent;

  if (!ARMCII_GLOBAL_STATE.iov_checks) return 1;

  mreg = gmr_lookup(ptrs[0], proc);

  /* If local, all must be local */
  if (mreg == NULL) {
    for (i = 1; i < count; i++) {
      mreg = gmr_lookup(ptrs[i], proc);
      if (mreg != NULL)
        return 0;
    }
  }
  /* If shared, all must fall in this region */
  else {
    base   = mreg->slices[proc].base;
    extent = ((uint8_t*) base) + mreg->slices[proc].size;

    for (i = 1; i < count; i++)
      if ( !(ptrs[i] >= base && ptrs[i] < extent) )
        return 0;
  }

  return 1;
}


/** Perform an I/O vector operation.  Local buffers must be private.
  *
  * @param[in] op          Operation to be performed (ARMCII_OP_PUT, ...)
  * @param[in] src         Array of source pointers
  * @param[in] dst         Array of destination pointers
  * @param[in] count       Length of pointer arrays
  * @param[in] size        Size of each transfer
  * @param[in] datatype    Data type for accumulate op (ignored for all others)
  * @param[in] overlapping Do remote regions overlap?
  * @param[in] same_alloc  Do remote regions correspond to the same allocation?
  * @param[in] proc        Target process
  * @return                Zero on success, error code otherwise
  */
int ARMCII_Iov_op_dispatch(enum ARMCII_Op_e op, void **src, void **dst, int count, int size,
    int datatype, int overlapping, int same_alloc, int proc) {

  MPI_Datatype type;
  int type_count, type_size;

  if (op == ARMCII_OP_ACC) {
    ARMCII_Acc_type_translate(datatype, &type, &type_size);
    type_count = size/type_size;
    ARMCII_Assert_msg(size % type_size == 0, "Transfer size is not a multiple of type size");
  } else {
    type = MPI_BYTE;
    MPI_Type_size(type, &type_size);
    type_count = size/type_size;
    ARMCII_Assert_msg(size % type_size == 0, "Transfer size is not a multiple of type size");
  }

  // CONSERVATIVE CASE: If remote pointers overlap or remote pointers correspond to
  // multiple allocations, use the safe implementation to avoid invalid MPI
  // use.

  if (overlapping || !same_alloc || ARMCII_GLOBAL_STATE.iov_method == ARMCII_IOV_CONSRV) {
    if (overlapping) ARMCII_Warning("IOV remote buffers overlap\n");
    if (!same_alloc) ARMCII_Warning("IOV remote buffers are not within the same allocation\n");
    return ARMCII_Iov_op_safe(op, src, dst, count, type_count, type, proc);
  }

  // OPTIMIZED CASE: It's safe for us to issue all the operations under a
  // single lock.

  else if (   ARMCII_GLOBAL_STATE.iov_method == ARMCII_IOV_DIRECT
           || ARMCII_GLOBAL_STATE.iov_method == ARMCII_IOV_AUTO  ) {
    if (ARMCII_GLOBAL_STATE.no_mpi_bottom == 1) {
      return ARMCII_Iov_op_datatype_no_bottom(op, src, dst, count, type_count, type, proc);
    } else {
      return ARMCII_Iov_op_datatype(op, src, dst, count, type_count, type, proc);
    }

  } else if (ARMCII_GLOBAL_STATE.iov_method == ARMCII_IOV_BATCHED) {
    return ARMCII_Iov_op_batched(op, src, dst, count, type_count, type, proc);

  } else {
    ARMCII_Error("unknown iov method (%d)\n", ARMCII_GLOBAL_STATE.iov_method);
    return 1;
  }
}


/** Safe implementation of the ARMCI IOV operation
  */
int ARMCII_Iov_op_safe(enum ARMCII_Op_e op, void **src, void **dst, int count, int elem_count,
    MPI_Datatype type, int proc) {
  
  int i;

  for (i = 0; i < count; i++) {
    gmr_t *mreg;
    void *shr_ptr;

    switch(op) {
      case ARMCII_OP_PUT:
        shr_ptr = dst[i];
        break;
      case ARMCII_OP_GET:
        shr_ptr = src[i];
        break;
      case ARMCII_OP_ACC:
        shr_ptr = dst[i];
        break;
      default:
        ARMCII_Error("unknown operation (%d)", op);
        return 1;
    }

    mreg = gmr_lookup(shr_ptr, proc);
    ARMCII_Assert_msg(mreg != NULL, "Invalid remote pointer");

    gmr_lock(mreg, proc);

    switch(op) {
      case ARMCII_OP_PUT:
        gmr_put(mreg, src[i], dst[i], elem_count, proc);
        break;
      case ARMCII_OP_GET:
        gmr_get(mreg, src[i], dst[i], elem_count, proc);
        break;
      case ARMCII_OP_ACC:
        gmr_accumulate(mreg, src[i], dst[i], elem_count, type, proc);
        break;
      default:
        ARMCII_Error("unknown operation (%d)", op);
        return 1;
    }

    gmr_unlock(mreg, proc);
  }

  return 0;
}


/** Optimized implementation of the ARMCI IOV operation that uses a single
  * lock/unlock pair.
  */
int ARMCII_Iov_op_batched(enum ARMCII_Op_e op, void **src, void **dst, int count, int elem_count,
    MPI_Datatype type, int proc) {

  int i;
  gmr_t *mreg;
  void *shr_ptr;

  switch(op) {
    case ARMCII_OP_PUT:
      shr_ptr = dst[0];
      break;
    case ARMCII_OP_GET:
      shr_ptr = src[0];
      break;
    case ARMCII_OP_ACC:
      shr_ptr = dst[0];
      break;
    default:
      ARMCII_Error("unknown operation (%d)", op);
      return 1;
  }

  mreg = gmr_lookup(shr_ptr, proc);
  ARMCII_Assert_msg(mreg != NULL, "Invalid remote pointer");

  gmr_lock(mreg, proc);

  for (i = 0; i < count; i++) {

    if (   ARMCII_GLOBAL_STATE.iov_batched_limit > 0 
        && i % ARMCII_GLOBAL_STATE.iov_batched_limit == 0
        && i > 0 )
    {
      gmr_unlock(mreg, proc);
      gmr_lock(mreg, proc);
    }

    switch(op) {
      case ARMCII_OP_PUT:
        gmr_put(mreg, src[i], dst[i], elem_count, proc);
        break;
      case ARMCII_OP_GET:
        gmr_get(mreg, src[i], dst[i], elem_count, proc);
        break;
      case ARMCII_OP_ACC:
        gmr_accumulate(mreg, src[i], dst[i], elem_count, type, proc);
        break;
      default:
        ARMCII_Error("unknown operation (%d)", op);
        return 1;
    }
  }

  gmr_unlock(mreg, proc);

  return 0;
}


/** Optimized implementation of the ARMCI IOV operation that uses an MPI
  * datatype to achieve a one-sided gather/scatter.
  */
int ARMCII_Iov_op_datatype(enum ARMCII_Op_e op, void **src, void **dst, int count, int elem_count,
    MPI_Datatype type, int proc) {

    gmr_t *mreg;
    MPI_Datatype  type_loc, type_rem;
    MPI_Aint      disp_loc[count];
    int           disp_rem[count];
    int           block_len[count];
    void         *dst_win_base;
    int           dst_win_size, i, type_size;
    void        **buf_rem, **buf_loc;
    MPI_Aint      base_rem;

    switch(op) {
      case ARMCII_OP_ACC:
      case ARMCII_OP_PUT:
        buf_rem = dst;
        buf_loc = src;
        break;
      case ARMCII_OP_GET:
        buf_rem = src;
        buf_loc = dst;
        break;
      default:
        ARMCII_Error("unknown operation (%d)", op);
        return 1;
    }

    MPI_Type_size(type, &type_size);

    mreg = gmr_lookup(buf_rem[0], proc);
    ARMCII_Assert_msg(mreg != NULL, "Invalid remote pointer");

    dst_win_base = mreg->slices[proc].base;
    dst_win_size = mreg->slices[proc].size;

    MPI_Get_address(dst_win_base, &base_rem);

    for (i = 0; i < count; i++) {
      MPI_Aint target_rem;
      MPI_Get_address(buf_loc[i], &disp_loc[i]);
      MPI_Get_address(buf_rem[i], &target_rem);
      disp_rem[i]  = (target_rem - base_rem)/type_size;
      block_len[i] = elem_count;

      ARMCII_Assert_msg((target_rem - base_rem) % type_size == 0, "Transfer size is not a multiple of type size");
      ARMCII_Assert_msg(disp_rem[i] >= 0 && disp_rem[i] < dst_win_size, "Invalid remote pointer");
      ARMCII_Assert_msg(((uint8_t*)buf_rem[i]) + block_len[i] <= ((uint8_t*)dst_win_base) + dst_win_size, "Transfer exceeds buffer length");
    }

    MPI_Type_create_hindexed(count, block_len, disp_loc, type, &type_loc);
    MPI_Type_create_indexed_block(count, elem_count, disp_rem, type, &type_rem);
    //MPI_Type_indexed(count, block_len, disp_rem, type, &type_rem);

    MPI_Type_commit(&type_loc);
    MPI_Type_commit(&type_rem);

    gmr_lock(mreg, proc);

    switch(op) {
      case ARMCII_OP_ACC:
        gmr_accumulate_typed(mreg, MPI_BOTTOM, 1, type_loc, MPI_BOTTOM, 1, type_rem, proc);
        break;
      case ARMCII_OP_PUT:
        gmr_put_typed(mreg, MPI_BOTTOM, 1, type_loc, MPI_BOTTOM, 1, type_rem, proc);
        break;
      case ARMCII_OP_GET:
        gmr_get_typed(mreg, MPI_BOTTOM, 1, type_rem, MPI_BOTTOM, 1, type_loc, proc);
        break;
      default:
        ARMCII_Error("unknown operation (%d)", op);
        return 1;
    }

    gmr_unlock(mreg, proc);

    MPI_Type_free(&type_loc);
    MPI_Type_free(&type_rem);

    return 0;
}    


/** Optimized implementation of the ARMCI IOV operation that uses an MPI
  * datatype to achieve a one-sided gather/scatter.  Does not use MPI_BOTTOM.
  */
int ARMCII_Iov_op_datatype_no_bottom(enum ARMCII_Op_e op, void **src, void **dst, int count, int elem_count,
    MPI_Datatype type, int proc) {

    gmr_t *mreg;
    MPI_Datatype  type_loc, type_rem;
    MPI_Aint      disp_loc[count];
    int           disp_rem[count];
    int           block_len[count];
    void         *dst_win_base;
    int           dst_win_size, i, type_size;
    void        **buf_rem, **buf_loc;
    MPI_Aint      base_rem;
    MPI_Aint      base_loc;
    void         *base_loc_ptr;

    switch(op) {
      case ARMCII_OP_ACC:
      case ARMCII_OP_PUT:
        buf_rem = dst;
        buf_loc = src;
        break;
      case ARMCII_OP_GET:
        buf_rem = src;
        buf_loc = dst;
        break;
      default:
        ARMCII_Error("unknown operation (%d)", op);
        return 1;
    }

    MPI_Type_size(type, &type_size);

    mreg = gmr_lookup(buf_rem[0], proc);
    ARMCII_Assert_msg(mreg != NULL, "Invalid remote pointer");

    dst_win_base = mreg->slices[proc].base;
    dst_win_size = mreg->slices[proc].size;

    MPI_Get_address(dst_win_base, &base_rem);

    /* Pick a base address for the start of the origin's datatype */
    base_loc_ptr = buf_loc[0];
    MPI_Get_address(base_loc_ptr, &base_loc);

    for (i = 0; i < count; i++) {
      MPI_Aint target_rem, target_loc;
      MPI_Get_address(buf_loc[i], &target_loc);
      MPI_Get_address(buf_rem[i], &target_rem);
      disp_loc[i]  =  target_loc - base_loc;
      disp_rem[i]  = (target_rem - base_rem)/type_size;
      block_len[i] = elem_count;

      ARMCII_Assert_msg((target_rem - base_rem) % type_size == 0, "Transfer size is not a multiple of type size");
      ARMCII_Assert_msg(disp_rem[i] >= 0 && disp_rem[i] < dst_win_size, "Invalid remote pointer");
      ARMCII_Assert_msg(((uint8_t*)buf_rem[i]) + block_len[i] <= ((uint8_t*)dst_win_base) + dst_win_size, "Transfer exceeds buffer length");
    }

    MPI_Type_create_hindexed(count, block_len, disp_loc, type, &type_loc);
    MPI_Type_create_indexed_block(count, elem_count, disp_rem, type, &type_rem);
    //MPI_Type_indexed(count, block_len, disp_rem, type, &type_rem);

    MPI_Type_commit(&type_loc);
    MPI_Type_commit(&type_rem);

    gmr_lock(mreg, proc);

    switch(op) {
      case ARMCII_OP_ACC:
        gmr_accumulate_typed(mreg, base_loc_ptr, 1, type_loc, MPI_BOTTOM, 1, type_rem, proc);
        break;
      case ARMCII_OP_PUT:
        gmr_put_typed(mreg, base_loc_ptr, 1, type_loc, MPI_BOTTOM, 1, type_rem, proc);
        break;
      case ARMCII_OP_GET:
        gmr_get_typed(mreg, MPI_BOTTOM, 1, type_rem, base_loc_ptr, 1, type_loc, proc);
        break;
      default:
        ARMCII_Error("unknown operation (%d)", op);
        return 1;
    }

    gmr_unlock(mreg, proc);

    MPI_Type_free(&type_loc);
    MPI_Type_free(&type_rem);

    return 0;
}    


/** Generalized I/O vector one-sided put.
  *
  * @param[in] iov      Vector of transfer information.
  * @param[in] iov_len  Length of iov.
  * @param[in] proc     Target process.
  * @return             Success 0, otherwise non-zero.
  */
int ARMCI_PutV(armci_giov_t *iov, int iov_len, int proc) {
  int v;

  for (v = 0; v < iov_len; v++) {
    void **src_buf;
    int    overlapping, same_alloc;

    if (iov[v].ptr_array_len == 0) continue; // NOP //
    if (iov[v].bytes == 0) continue; // NOP //

    overlapping = ARMCII_Iov_check_overlap(iov[v].dst_ptr_array, iov[v].ptr_array_len, iov[v].bytes);
    same_alloc  = ARMCII_Iov_check_same_allocation(iov[v].dst_ptr_array, iov[v].ptr_array_len, proc);

    ARMCII_Buf_prepare_putv(iov[v].src_ptr_array, &src_buf, iov[v].ptr_array_len, iov[v].bytes);
    ARMCII_Iov_op_dispatch(ARMCII_OP_PUT, src_buf, iov[v].dst_ptr_array, iov[v].ptr_array_len, iov[v].bytes, 0, overlapping, same_alloc, proc);
    ARMCII_Buf_finish_putv(iov[v].src_ptr_array, src_buf, iov[v].ptr_array_len, iov[v].bytes);
  }

  return 0;
}


/** Generalized I/O vector one-sided get.
  *
  * @param[in] iov      Vector of transfer information.
  * @param[in] iov_len  Length of iov.
  * @param[in] proc     Target process.
  * @return             Success 0, otherwise non-zero.
  */
int ARMCI_GetV(armci_giov_t *iov, int iov_len, int proc) {
  int v;

  for (v = 0; v < iov_len; v++) {
    void **dst_buf;
    int    overlapping, same_alloc;

    if (iov[v].ptr_array_len == 0) continue; // NOP //
    if (iov[v].bytes == 0) continue; // NOP //

    // overlapping = ARMCII_Iov_check_overlap(iov[v].src_ptr_array, iov[v].ptr_array_len, iov[v].bytes);
    overlapping = ARMCII_Iov_check_overlap(iov[v].dst_ptr_array, iov[v].ptr_array_len, iov[v].bytes);
    same_alloc  = ARMCII_Iov_check_same_allocation(iov[v].src_ptr_array, iov[v].ptr_array_len, proc);

    ARMCII_Buf_prepare_getv(iov[v].dst_ptr_array, &dst_buf, iov[v].ptr_array_len, iov[v].bytes);
    ARMCII_Iov_op_dispatch(ARMCII_OP_GET, iov[v].src_ptr_array, dst_buf, iov[v].ptr_array_len, iov[v].bytes, 0, overlapping, same_alloc, proc);
    ARMCII_Buf_finish_getv(iov[v].dst_ptr_array, dst_buf, iov[v].ptr_array_len, iov[v].bytes);
  }

  return 0;
}


/** Generalized I/O vector one-sided accumulate.
  *
  * @param[in] iov      Vector of transfer information.
  * @param[in] iov_len  Length of iov.
  * @param[in] proc     Target process.
  * @return             Success 0, otherwise non-zero.
  */
int ARMCI_AccV(int datatype, void *scale, armci_giov_t *iov, int iov_len, int proc) {
  int v;

  for (v = 0; v < iov_len; v++) {
    void **src_buf;
    int    overlapping, same_alloc;

    if (iov[v].ptr_array_len == 0) continue; // NOP //
    if (iov[v].bytes == 0) continue; // NOP //

    overlapping = ARMCII_Iov_check_overlap(iov[v].dst_ptr_array, iov[v].ptr_array_len, iov[v].bytes);
    same_alloc  = ARMCII_Iov_check_same_allocation(iov[v].dst_ptr_array, iov[v].ptr_array_len, proc);

    ARMCII_Buf_prepare_accv(iov[v].src_ptr_array, &src_buf, iov[v].ptr_array_len, iov[v].bytes, datatype, scale);
    ARMCII_Iov_op_dispatch(ARMCII_OP_ACC, src_buf, iov[v].dst_ptr_array, iov[v].ptr_array_len, iov[v].bytes, datatype, overlapping, same_alloc, proc);
    ARMCII_Buf_finish_accv(iov[v].src_ptr_array, src_buf, iov[v].ptr_array_len, iov[v].bytes);
  }

  return 0;
}


int ARMCI_NbPutV(armci_giov_t *iov, int iov_len, int proc, armci_hdl_t* handle) {
  return ARMCI_PutV(iov, iov_len, proc);
}

int ARMCI_NbGetV(armci_giov_t *iov, int iov_len, int proc, armci_hdl_t* handle) {
  return ARMCI_GetV(iov, iov_len, proc);
}

int ARMCI_NbAccV(int datatype, void *scale, armci_giov_t *iov, int iov_len, int proc, armci_hdl_t* handle) {
  return ARMCI_AccV(datatype, scale, iov, iov_len, proc);
}


