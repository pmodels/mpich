/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <armci.h>
#include <armci_internals.h>
#include <gmr.h>
#include <debug.h>


/** Convert an ARMCI strided access description into an MPI subarray datatype.
  *
  * @param[in]  stride_array    Array of strides
  * @param[in]  count           Array of transfer counts
  * @param[in]  stride_levels   Number of levels of striding
  * @param[in]  old_type        Type of the data element described by count and stride_array
  * @param[out] new_type        New MPI type for the given strided access
  */
void ARMCII_Strided_to_dtype(int stride_array[/*stride_levels*/], int count[/*stride_levels+1*/],
                             int stride_levels, MPI_Datatype old_type, MPI_Datatype *new_type)
{
  int sizes   [stride_levels+1];
  int subsizes[stride_levels+1];
  int starts  [stride_levels+1];
  int i, old_type_size;

  MPI_Type_size(old_type, &old_type_size);

  /* Eliminate counts that don't count (all 1 counts at the end) */
  for (i = stride_levels+1; i > 0 && stride_levels > 0 && count[i-1] == 1; i--)
    stride_levels--;

  /* A correct strided spec should me monotonic increasing and stride_array[i+1] should
     be a multiple of stride_array[i]. */
  if (stride_levels > 0) {
    for (i = 1; i < stride_levels; i++)
      ARMCII_Assert(stride_array[i] >= stride_array[i-1] && stride_array[i] % stride_array[i-1] == 0);
  }

  /* Test for a contiguous transfer */
  if (stride_levels == 0) {
    int elem_count = count[0]/old_type_size;

    ARMCII_Assert(count[0] % old_type_size == 0);
    MPI_Type_contiguous(elem_count, old_type, new_type);
  }

  /* Transfer is non-contiguous */
  else {

    for (i = 0; i < stride_levels+1; i++)
      starts[i] = 0;

    sizes   [stride_levels] = stride_array[0]/old_type_size;
    subsizes[stride_levels] = count[0]/old_type_size;

    ARMCII_Assert(stride_array[0] % old_type_size == 0 && count[0] % old_type_size == 0);

    for (i = 1; i < stride_levels; i++) {
      /* Convert strides into dimensions by dividing out contributions from lower dims */
      sizes   [stride_levels-i] = stride_array[i]/stride_array[i-1];
      subsizes[stride_levels-i] = count[i];

      ARMCII_Assert_msg(stride_array[i] % stride_array[i-1] == 0, "Invalid striding");
    }

    sizes   [0] = count[stride_levels];
    subsizes[0] = count[stride_levels];

    MPI_Type_create_subarray(stride_levels+1, sizes, subsizes, starts, MPI_ORDER_C, old_type, new_type);
  }
}


/** Blocking operation that transfers data from the calling process to the
  * memory of the remote process.  The data transfer is strided and blocking.
  *
  * @param[in] src_ptr         Source starting address of the data block to put.
  * @param[in] src_stride_arr  Source array of stride distances in bytes.
  * @param[in] dst_ptr         Destination starting address to put data.
  * @param[in] dst_stride_ar   Destination array of stride distances in bytes.
  * @param[in] count           Block size in each dimension. count[0] should be the
  *                            number of bytes of contiguous data in leading dimension.
  * @param[in] stride_levels   The level of strides.
  * @param[in] proc            Remote process ID (destination).
  *
  * @return                    Zero on success, error code otherwise.
  */
int ARMCI_PutS(void *src_ptr, int src_stride_ar[/*stride_levels*/],
               void *dst_ptr, int dst_stride_ar[/*stride_levels*/], 
               int count[/*stride_levels+1*/], int stride_levels, int proc) {

  int err;

  if (ARMCII_GLOBAL_STATE.strided_method == ARMCII_STRIDED_DIRECT) {
    void         *src_buf = NULL;
    gmr_t *mreg, *gmr_loc = NULL;
    MPI_Datatype src_type, dst_type;

    /* COPY: Guard shared buffers */
    if (ARMCII_GLOBAL_STATE.shr_buf_method == ARMCII_SHR_BUF_COPY) {
      gmr_loc = gmr_lookup(src_ptr, ARMCI_GROUP_WORLD.rank);

      if (gmr_loc != NULL) {
        int i, size;

        for (i = 1, size = count[0]; i < stride_levels+1; i++)
          size *= count[i];

        MPI_Alloc_mem(size, MPI_INFO_NULL, &src_buf);
        ARMCII_Assert(src_buf != NULL);

        gmr_dla_lock(gmr_loc);
        armci_write_strided(src_ptr, stride_levels, src_stride_ar, count, src_buf);
        gmr_dla_unlock(gmr_loc);

        MPI_Type_contiguous(size, MPI_BYTE, &src_type);
      }
    }

    /* NOGUARD: If src_buf hasn't been assigned to a copy, the strided source
     * buffer is going to be used directly. */
    if (src_buf == NULL) { 
        src_buf = src_ptr;
        ARMCII_Strided_to_dtype(src_stride_ar, count, stride_levels, MPI_BYTE, &src_type);
    }

    ARMCII_Strided_to_dtype(dst_stride_ar, count, stride_levels, MPI_BYTE, &dst_type);

    MPI_Type_commit(&src_type);
    MPI_Type_commit(&dst_type);

    mreg = gmr_lookup(dst_ptr, proc);
    ARMCII_Assert_msg(mreg != NULL, "Invalid shared pointer");

    gmr_lock(mreg, proc);
    gmr_put_typed(mreg, src_buf, 1, src_type, dst_ptr, 1, dst_type, proc);
    gmr_unlock(mreg, proc);

    MPI_Type_free(&src_type);
    MPI_Type_free(&dst_type);

    /* COPY: Free temporary buffer */
    if (src_buf != src_ptr)
      MPI_Free_mem(src_buf);

    err = 0;

  } else {
    armci_giov_t iov;

    ARMCII_Strided_to_iov(&iov, src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels);
    err = ARMCI_PutV(&iov, 1, proc);

    free(iov.src_ptr_array);
    free(iov.dst_ptr_array);
  }

  return err;
}


/** Blocking operation that transfers data from the remote process to the
  * memory of the calling process.  The data transfer is strided and blocking.
  *
  * @param[in] src_ptr         Source starting address of the data block to put.
  * @param[in] src_stride_arr  Source array of stride distances in bytes.
  * @param[in] dst_ptr         Destination starting address to put data.
  * @param[in] dst_stride_ar   Destination array of stride distances in bytes.
  * @param[in] count           Block size in each dimension. count[0] should be the
  *                            number of bytes of contiguous data in leading dimension.
  * @param[in] stride_levels   The level of strides.
  * @param[in] proc            Remote process ID (destination).
  *
  * @return                    Zero on success, error code otherwise.
  */
int ARMCI_GetS(void *src_ptr, int src_stride_ar[/*stride_levels*/],
               void *dst_ptr, int dst_stride_ar[/*stride_levels*/], 
               int count[/*stride_levels+1*/], int stride_levels, int proc) {

  int err;

  if (ARMCII_GLOBAL_STATE.strided_method == ARMCII_STRIDED_DIRECT) {
    void         *dst_buf = NULL;
    gmr_t *mreg, *gmr_loc = NULL;
    MPI_Datatype src_type, dst_type;

    /* COPY: Guard shared buffers */
    if (ARMCII_GLOBAL_STATE.shr_buf_method == ARMCII_SHR_BUF_COPY) {
      gmr_loc = gmr_lookup(dst_ptr, ARMCI_GROUP_WORLD.rank);

      if (gmr_loc != NULL) {
        int i, size;

        for (i = 1, size = count[0]; i < stride_levels+1; i++)
          size *= count[i];

        MPI_Alloc_mem(size, MPI_INFO_NULL, &dst_buf);
        ARMCII_Assert(dst_buf != NULL);

        MPI_Type_contiguous(size, MPI_BYTE, &dst_type);
      }
    }

    /* NOGUARD: If dst_buf hasn't been assigned to a copy, the strided source
     * buffer is going to be used directly. */
    if (dst_buf == NULL) { 
        dst_buf = dst_ptr;
        ARMCII_Strided_to_dtype(dst_stride_ar, count, stride_levels, MPI_BYTE, &dst_type);
    }

    ARMCII_Strided_to_dtype(src_stride_ar, count, stride_levels, MPI_BYTE, &src_type);

    MPI_Type_commit(&src_type);
    MPI_Type_commit(&dst_type);

    mreg = gmr_lookup(src_ptr, proc);
    ARMCII_Assert_msg(mreg != NULL, "Invalid shared pointer");

    gmr_lock(mreg, proc);
    gmr_get_typed(mreg, src_ptr, 1, src_type, dst_buf, 1, dst_type, proc);
    gmr_unlock(mreg, proc);

    /* COPY: Finish the transfer */
    if (dst_buf != dst_ptr) {
      gmr_dla_lock(gmr_loc);
      armci_read_strided(dst_ptr, stride_levels, dst_stride_ar, count, dst_buf);
      gmr_dla_unlock(gmr_loc);
      MPI_Free_mem(dst_buf);
    }

    MPI_Type_free(&src_type);
    MPI_Type_free(&dst_type);

    err = 0;

  } else {
    armci_giov_t iov;

    ARMCII_Strided_to_iov(&iov, src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels);
    err = ARMCI_GetV(&iov, 1, proc);

    free(iov.src_ptr_array);
    free(iov.dst_ptr_array);
  }

  return err;
}


/** Blocking operation that accumulates data from the local process into the
  * memory of the remote process.  The data transfer is strided and blocking.
  *
  * @param[in] datatype        Type of data to be transferred.
  * @param[in] scale           Pointer to the value that input data should be scaled by.
  * @param[in] src_ptr         Source starting address of the data block to put.
  * @param[in] src_stride_arr  Source array of stride distances in bytes.
  * @param[in] dst_ptr         Destination starting address to put data.
  * @param[in] dst_stride_ar   Destination array of stride distances in bytes.
  * @param[in] count           Block size in each dimension. count[0] should be the
  *                            number of bytes of contiguous data in leading dimension.
  * @param[in] stride_levels   The level of strides.
  * @param[in] proc            Remote process ID (destination).
  *
  * @return                    Zero on success, error code otherwise.
  */
int ARMCI_AccS(int datatype, void *scale,
               void *src_ptr, int src_stride_ar[/*stride_levels*/],
               void *dst_ptr, int dst_stride_ar[/*stride_levels*/],
               int count[/*stride_levels+1*/], int stride_levels, int proc) {

  int err;

  if (ARMCII_GLOBAL_STATE.strided_method == ARMCII_STRIDED_DIRECT) {
    void         *src_buf = NULL;
    gmr_t *mreg, *gmr_loc = NULL;
    MPI_Datatype src_type, dst_type, mpi_datatype;
    int          scaled, mpi_datatype_size;

    ARMCII_Acc_type_translate(datatype, &mpi_datatype, &mpi_datatype_size);
    scaled = ARMCII_Buf_acc_is_scaled(datatype, scale);

    /* SCALE: copy and scale if requested */
    if (scaled) {
      armci_giov_t iov;
      int i, nelem;

      if (ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD)
        gmr_loc = gmr_lookup(src_ptr, ARMCI_GROUP_WORLD.rank);

      for (i = 1, nelem = count[0]/mpi_datatype_size; i < stride_levels+1; i++)
        nelem *= count[i];

      MPI_Alloc_mem(nelem*mpi_datatype_size, MPI_INFO_NULL, &src_buf);
      ARMCII_Assert(src_buf != NULL);

      if (gmr_loc != NULL) gmr_dla_lock(gmr_loc);

      /* Shoehorn the strided information into an IOV */
      ARMCII_Strided_to_iov(&iov, src_ptr, src_stride_ar, src_ptr, src_stride_ar, count, stride_levels);

      for (i = 0; i < iov.ptr_array_len; i++)
        ARMCII_Buf_acc_scale(iov.src_ptr_array[i], ((uint8_t*)src_buf) + i*iov.bytes, iov.bytes, datatype, scale);

      free(iov.src_ptr_array);
      free(iov.dst_ptr_array);

      if (gmr_loc != NULL) gmr_dla_unlock(gmr_loc);

      MPI_Type_contiguous(nelem, mpi_datatype, &src_type);
    }

    /* COPY: Guard shared buffers */
    else if (ARMCII_GLOBAL_STATE.shr_buf_method == ARMCII_SHR_BUF_COPY) {
      gmr_loc = gmr_lookup(src_ptr, ARMCI_GROUP_WORLD.rank);

      if (gmr_loc != NULL) {
        int i, nelem;

        for (i = 1, nelem = count[0]/mpi_datatype_size; i < stride_levels+1; i++)
          nelem *= count[i];

        MPI_Alloc_mem(nelem*mpi_datatype_size, MPI_INFO_NULL, &src_buf);
        ARMCII_Assert(src_buf != NULL);

        gmr_dla_lock(gmr_loc);
        armci_write_strided(src_ptr, stride_levels, src_stride_ar, count, src_buf);
        gmr_dla_unlock(gmr_loc);

        MPI_Type_contiguous(nelem, mpi_datatype, &src_type);
      }
    }

    /* NOGUARD: If src_buf hasn't been assigned to a copy, the strided source
     * buffer is going to be used directly. */
    if (src_buf == NULL) { 
        src_buf = src_ptr;
        ARMCII_Strided_to_dtype(src_stride_ar, count, stride_levels, mpi_datatype, &src_type);
    }

    ARMCII_Strided_to_dtype(dst_stride_ar, count, stride_levels, mpi_datatype, &dst_type);

    MPI_Type_commit(&src_type);
    MPI_Type_commit(&dst_type);

    int src_size, dst_size;

    MPI_Type_size(src_type, &src_size);
    MPI_Type_size(dst_type, &dst_size);

    ARMCII_Assert(src_size == dst_size);

    mreg = gmr_lookup(dst_ptr, proc);
    ARMCII_Assert_msg(mreg != NULL, "Invalid shared pointer");

    gmr_lock(mreg, proc);
    gmr_accumulate_typed(mreg, src_buf, 1, src_type, dst_ptr, 1, dst_type, proc);
    gmr_unlock(mreg, proc);

    MPI_Type_free(&src_type);
    MPI_Type_free(&dst_type);

    /* COPY/SCALE: Free temp buffer */
    if (src_buf != src_ptr)
      MPI_Free_mem(src_buf);

    err = 0;

  } else {
    armci_giov_t iov;

    ARMCII_Strided_to_iov(&iov, src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels);
    err = ARMCI_AccV(datatype, scale, &iov, 1, proc);

    free(iov.src_ptr_array);
    free(iov.dst_ptr_array);
  }

  return err;
}


/** Non-blocking operation that transfers data from the calling process to the
  * memory of the remote process.  The data transfer is strided and blocking.
  *
  * @param[in] src_ptr         Source starting address of the data block to put.
  * @param[in] src_stride_arr  Source array of stride distances in bytes.
  * @param[in] dst_ptr         Destination starting address to put data.
  * @param[in] dst_stride_ar   Destination array of stride distances in bytes.
  * @param[in] count           Block size in each dimension. count[0] should be the
  *                            number of bytes of contiguous data in leading dimension.
  * @param[in] stride_levels   The level of strides.
  * @param[in] proc            Remote process ID (destination).
  *
  * @return                    Zero on success, error code otherwise.
  */
int ARMCI_NbPutS(void *src_ptr, int src_stride_ar[/*stride_levels*/],
               void *dst_ptr, int dst_stride_ar[/*stride_levels*/], 
               int count[/*stride_levels+1*/], int stride_levels, int proc, armci_hdl_t *hdl) {

  return ARMCI_PutS(src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels, proc);
}


/** Non-blocking operation that transfers data from the remote process to the
  * memory of the calling process.  The data transfer is strided and blocking.
  *
  * @param[in] src_ptr         Source starting address of the data block to put.
  * @param[in] src_stride_arr  Source array of stride distances in bytes.
  * @param[in] dst_ptr         Destination starting address to put data.
  * @param[in] dst_stride_ar   Destination array of stride distances in bytes.
  * @param[in] count           Block size in each dimension. count[0] should be the
  *                            number of bytes of contiguous data in leading dimension.
  * @param[in] stride_levels   The level of strides.
  * @param[in] proc            Remote process ID (destination).
  *
  * @return                    Zero on success, error code otherwise.
  */
int ARMCI_NbGetS(void *src_ptr, int src_stride_ar[/*stride_levels*/],
               void *dst_ptr, int dst_stride_ar[/*stride_levels*/], 
               int count[/*stride_levels+1*/], int stride_levels, int proc, armci_hdl_t *hdl) {

  return ARMCI_GetS(src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels, proc);
}


/** Non-blocking operation that accumulates data from the local process into the
  * memory of the remote process.  The data transfer is strided and blocking.
  *
  * @param[in] datatype        Type of data to be transferred.
  * @param[in] scale           Pointer to the value that input data should be scaled by.
  * @param[in] src_ptr         Source starting address of the data block to put.
  * @param[in] src_stride_arr  Source array of stride distances in bytes.
  * @param[in] dst_ptr         Destination starting address to put data.
  * @param[in] dst_stride_ar   Destination array of stride distances in bytes.
  * @param[in] count           Block size in each dimension. count[0] should be the
  *                            number of bytes of contiguous data in leading dimension.
  * @param[in] stride_levels   The level of strides.
  * @param[in] proc            Remote process ID (destination).
  *
  * @return                    Zero on success, error code otherwise.
  */
int ARMCI_NbAccS(int datatype, void *scale,
               void *src_ptr, int src_stride_ar[/*stride_levels*/],
               void *dst_ptr, int dst_stride_ar[/*stride_levels*/],
               int count[/*stride_levels+1*/], int stride_levels, int proc, armci_hdl_t *hdl) {

  return ARMCI_AccS(datatype, scale, src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels, proc);
}


/** Translate a strided operation into a more general IO Vector.
  *
  * @param[in] src_ptr         Source starting address of the data block to put.
  * @param[in] src_stride_arr  Source array of stride distances in bytes.
  * @param[in] dst_ptr         Destination starting address to put data.
  * @param[in] dst_stride_ar   Destination array of stride distances in bytes.
  * @param[in] count           Block size in each dimension. count[0] should be the
  *                            number of bytes of contiguous data in leading dimension.
  * @param[in] stride_levels   The level of strides.
  *
  * @return                    Zero on success, error code otherwise.
  */
void ARMCII_Strided_to_iov(armci_giov_t *iov,
               void *src_ptr, int src_stride_ar[/*stride_levels*/],
               void *dst_ptr, int dst_stride_ar[/*stride_levels*/], 
               int count[/*stride_levels+1*/], int stride_levels) {

  int i;

  iov->bytes = count[0];
  iov->ptr_array_len = 1;

  for (i = 0; i < stride_levels; i++)
    iov->ptr_array_len *= count[i+1];

  iov->src_ptr_array = malloc(iov->ptr_array_len*sizeof(void*));
  iov->dst_ptr_array = malloc(iov->ptr_array_len*sizeof(void*));

  ARMCII_Assert(iov->src_ptr_array != NULL && iov->dst_ptr_array != NULL);

  // Case 1: Non-strided transfer
  if (stride_levels == 0) {
    iov->src_ptr_array[0] = src_ptr;
    iov->dst_ptr_array[0] = dst_ptr;

  // Case 2: Strided transfer
  } else {
    int idx[stride_levels];
    int xfer;

    for (i = 0; i < stride_levels; i++)
      idx[i] = 0;

    for (xfer = 0; idx[stride_levels-1] < count[stride_levels]; xfer++) {
      int disp_src = 0;
      int disp_dst = 0;

      ARMCII_Assert(xfer < iov->ptr_array_len);

      // Calculate displacements from base pointers
      for (i = 0; i < stride_levels; i++) {
        disp_src += src_stride_ar[i]*idx[i];
        disp_dst += dst_stride_ar[i]*idx[i];
      }

      // Add to the IO Vector
      iov->src_ptr_array[xfer] = ((uint8_t*)src_ptr) + disp_src;
      iov->dst_ptr_array[xfer] = ((uint8_t*)dst_ptr) + disp_dst;

      // Increment innermost index
      idx[0] += 1;

      // Propagate "carry" overflows outward.  We're done when the outermost
      // index is greater than the requested count.
      for (i = 0; i < stride_levels-1; i++) {
        if (idx[i] >= count[i+1]) {
          idx[i]    = 0;
          idx[i+1] += 1;
        }
      }
    }

    ARMCII_Assert(xfer == iov->ptr_array_len);
  }
}


/** Blocking operation that transfers data from the calling process to the
  * memory of the remote process.  The data transfer is strided and blocking.
  * After the transfer completes, the given flag is set on the remote process.
  *
  * @param[in] src_ptr         Source starting address of the data block to put.
  * @param[in] src_stride_arr  Source array of stride distances in bytes.
  * @param[in] dst_ptr         Destination starting address to put data.
  * @param[in] dst_stride_ar   Destination array of stride distances in bytes.
  * @param[in] count           Block size in each dimension. count[0] should be the
  *                            number of bytes of contiguous data in leading dimension.
  * @param[in] stride_levels   The level of strides.
  * @param[in] flag            Location of the flag buffer
  * @param[in] value           Value to set the flag to
  * @param[in] proc            Remote process ID (destination).
  *
  * @return                    Zero on success, error code otherwise.
  */
int ARMCI_PutS_flag(void *src_ptr, int src_stride_ar[/*stride_levels*/],
                 void *dst_ptr, int dst_stride_ar[/*stride_levels*/], 
                 int count[/*stride_levels+1*/], int stride_levels, 
                 int *flag, int value, int proc) {

  ARMCI_PutS(src_ptr, src_stride_ar, dst_ptr, dst_stride_ar, count, stride_levels, proc);
  ARMCI_Fence(proc);
  ARMCI_Put(&value, flag, sizeof(int), proc);

  return 1;
}


/* Pack strided data into a contiguous destination buffer.  This is a local operation.
 *
 * @param[in] src            Pointer to the strided buffer
 * @param[in] stride_levels  Number of levels of striding
 * @param[in] src_stride_arr Array of length stride_levels of stride lengths
 * @param[in] count          Array of length stride_levels+1 of the number of
 *                           units at each stride level (lowest is contiguous)
 * @param[in] dst            Destination contiguous buffer
 */
void armci_write_strided(void *src, int stride_levels, int src_stride_arr[],
                         int count[], char *dst) {
  armci_giov_t iov;
  int i;

  // Shoehorn the strided information into an IOV
  ARMCII_Strided_to_iov(&iov, src, src_stride_arr, src, src_stride_arr, count, stride_levels);

  for (i = 0; i < iov.ptr_array_len; i++)
    ARMCI_Copy(iov.src_ptr_array[i], dst + i*count[0], iov.bytes);

  free(iov.src_ptr_array);
  free(iov.dst_ptr_array);
}


/* Unpack strided data from a contiguous source buffer.  This is a local operation.
 *
 * @param[in] src            Pointer to the contiguous buffer
 * @param[in] stride_levels  Number of levels of striding
 * @param[in] dst_stride_arr Array of length stride_levels of stride lengths
 * @param[in] count          Array of length stride_levels+1 of the number of
 *                           units at each stride level (lowest is contiguous)
 * @param[in] dst            Destination strided buffer
 */
void armci_read_strided(void *dst, int stride_levels, int dst_stride_arr[],
                        int count[], char *src) {
  armci_giov_t iov;
  int i;

  // Shoehorn the strided information into an IOV
  ARMCII_Strided_to_iov(&iov, dst, dst_stride_arr, dst, dst_stride_arr, count, stride_levels);

  for (i = 0; i < iov.ptr_array_len; i++)
    ARMCI_Copy(src + i*count[0], iov.dst_ptr_array[i], iov.bytes);

  free(iov.src_ptr_array);
  free(iov.dst_ptr_array);
}
