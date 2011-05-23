/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <armci.h>
#include <armci_internals.h>
#include <gmr.h>
#include <debug.h>


/** Prepare a set of buffers for use with a put operation.  The returned set of
  * buffers is guaranteed to be in private space.  Copies will be made if needed,
  * the result should be completed by finish.
  *
  * @param[in]  orig_bufs Original set of buffers.
  * @param[out] new_bufs  Pointer to the set of private buffers.
  * @param[in]  count     Number of entries in the buffer list.
  * @param[in]  size      The size of the buffers (all are of the same size).
  * @return               Number of buffers that were moved.
  */
int ARMCII_Buf_prepare_putv(void **orig_bufs, void ***new_bufs_ptr, int count, int size) {
  int num_moved = 0;

  if (ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD) {
    void **new_bufs = malloc(count*sizeof(void*));
    int i;

    for (i = 0; i < count; i++)
      new_bufs[i] = NULL;

    for (i = 0; i < count; i++) {
      // Check if the source buffer is within a shared region.  If so, copy it
      // into a private buffer.
      gmr_t *mreg = gmr_lookup(orig_bufs[i], ARMCI_GROUP_WORLD.rank);

      if (mreg != NULL) {
        MPI_Alloc_mem(size, MPI_INFO_NULL, &new_bufs[i]);
        ARMCII_Assert(new_bufs[i] != NULL);

        gmr_dla_lock(mreg);
        ARMCI_Copy(orig_bufs[i], new_bufs[i], size);
        // gmr_get(mreg, orig_bufs[i], new_bufs[i], size, ARMCI_GROUP_WORLD.rank);
        gmr_dla_unlock(mreg);

        num_moved++;
      } else {
        new_bufs[i] = orig_bufs[i];
      }
    }

    *new_bufs_ptr = new_bufs;
  }
  else {
    *new_bufs_ptr = orig_bufs;
  }
  
  return num_moved;
}


/** Finish a set of prepared buffers.  Will perform communication and copies as
  * needed to ensure results are in the original buffers.  Temporary space will be
  * freed.
  *
  * @param[in]  orig_bufs Original set of buffers.
  * @param[out] new_bufs  Set of private buffers.
  * @param[in]  count     Number of entries in the buffer list.
  * @param[in]  size      The size of the buffers (all are of the same size).
  */
void ARMCII_Buf_finish_putv(void **orig_bufs, void **new_bufs, int count, int size) {
  if (ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD) {
    int i;

    for (i = 0; i < count; i++) {
      if (orig_bufs[i] != new_bufs[i]) {
        MPI_Free_mem(new_bufs[i]);
      }
    }

    free(new_bufs);
  }
}


/** Prepare a set of buffers for use with an accumulate operation.  The
  * returned set of buffers is guaranteed to be in private space and scaled.
  * Copies will be made if needed, the result should be completed by finish.
  *
  * @param[in]  orig_bufs Original set of buffers.
  * @param[out] new_bufs  Pointer to the set of private buffers.
  * @param[in]  count     Number of entries in the buffer list.
  * @param[in]  size      The size of the buffers (all are of the same size).
  * @param[in]  datatype  The type of the buffer.
  * @param[in]  scale     Scaling constant to apply to each buffer.
  * @return               Number of buffers that were moved.
  */
int ARMCII_Buf_prepare_accv(void **orig_bufs, void ***new_bufs_ptr, int count, int size,
                            int datatype, void *scale) {

  void **new_bufs;
  int i, scaled, num_moved = 0;
  
  new_bufs = malloc(count*sizeof(void*));
  ARMCII_Assert(new_bufs != NULL);

  scaled = ARMCII_Buf_acc_is_scaled(datatype, scale);

  for (i = 0; i < count; i++) {
    gmr_t *mreg;

    // Check if the source buffer is within a shared region.
    mreg = gmr_lookup(orig_bufs[i], ARMCI_GROUP_WORLD.rank);

    if (scaled) {
      MPI_Alloc_mem(size, MPI_INFO_NULL, &new_bufs[i]);
      ARMCII_Assert(new_bufs[i] != NULL);

      // Lock if needed so we can directly access the buffer
      if (mreg != NULL && ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD)
        gmr_dla_lock(mreg);

      ARMCII_Buf_acc_scale(orig_bufs[i], new_bufs[i], size, datatype, scale);

      if (mreg != NULL && ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD)
        gmr_dla_unlock(mreg);
    } else {
      new_bufs[i] = orig_bufs[i];
    }

    if (mreg != NULL && ARMCII_GLOBAL_STATE.shr_buf_method == ARMCII_SHR_BUF_COPY) {
      // If the buffer wasn't copied, we should copy it into a private buffer
      if (new_bufs[i] == orig_bufs[i]) {
        MPI_Alloc_mem(size, MPI_INFO_NULL, &new_bufs[i]);
        ARMCII_Assert(new_bufs[i] != NULL);

        gmr_dla_lock(mreg);
        ARMCI_Copy(orig_bufs[i], new_bufs[i], size);
        gmr_dla_unlock(mreg);
      }
    }

    if (new_bufs[i] == orig_bufs[i])
      num_moved++;
  }

  *new_bufs_ptr = new_bufs;
  
  return num_moved;
}


/** Finish a set of prepared buffers.  Will perform communication and copies as
  * needed to ensure results are in the original buffers.  Temporary space will be
  * freed.
  *
  * @param[in]  orig_bufs Original set of buffers.
  * @param[out] new_bufs  Set of private buffers.
  * @param[in]  count     Number of entries in the buffer list.
  * @param[in]  size      The size of the buffers (all are of the same size).
  */
void ARMCII_Buf_finish_accv(void **orig_bufs, void **new_bufs, int count, int size) {
  int i;

  for (i = 0; i < count; i++) {
    if (orig_bufs[i] != new_bufs[i]) {
      MPI_Free_mem(new_bufs[i]);
    }
  }

  free(new_bufs);
}


/** Prepare a set of buffers for use with a get operation.  The returned set of
  * buffers is guaranteed to be in private space.  Copies will be made if needed,
  * the result should be completed by finish.
  *
  * @param[in]  orig_bufs Original set of buffers.
  * @param[out] new_bufs  Pointer to the set of private buffers.
  * @param[in]  count     Number of entries in the buffer list.
  * @param[in]  size      The size of the buffers (all are of the same size).
  * @return               Number of buffers that were moved.
  */
int ARMCII_Buf_prepare_getv(void **orig_bufs, void ***new_bufs_ptr, int count, int size) {
  int num_moved = 0;

  if (ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD) {
    void **new_bufs = malloc(count*sizeof(void*));
    int i;

    for (i = 0; i < count; i++)
      new_bufs[i] = NULL;

    for (i = 0; i < count; i++) {
      // Check if the destination buffer is within a shared region.  If not, create
      // a temporary private buffer to hold the result.
      gmr_t *mreg = gmr_lookup(orig_bufs[i], ARMCI_GROUP_WORLD.rank);

      if (mreg != NULL) {
        MPI_Alloc_mem(size, MPI_INFO_NULL, &new_bufs[i]);
        ARMCII_Assert(new_bufs[i] != NULL);
        num_moved++;
      } else {
        new_bufs[i] = orig_bufs[i];
      }
    }

    *new_bufs_ptr = new_bufs;
  } else {
    *new_bufs_ptr = orig_bufs;
  }
  
  return num_moved;
}


/** Finish a set of prepared buffers.  Will perform communication and copies as
  * needed to ensure results are in the original buffers.  Temporary space will be
  * freed.
  *
  * @param[in]  orig_bufs Original set of buffers.
  * @param[out] new_bufs  Set of private buffers.
  * @param[in]  count     Number of entries in the buffer list.
  * @param[in]  size      The size of the buffers (all are of the same size).
  */
void ARMCII_Buf_finish_getv(void **orig_bufs, void **new_bufs, int count, int size) {
  if (ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD) {
    int i;

    for (i = 0; i < count; i++) {
      if (orig_bufs[i] != new_bufs[i]) {
        gmr_t *mreg = gmr_lookup(orig_bufs[i], ARMCI_GROUP_WORLD.rank);
        ARMCII_Assert(mreg != NULL);

        gmr_dla_lock(mreg);
        ARMCI_Copy(new_bufs[i], orig_bufs[i], size);
        // gmr_put(mreg, new_bufs[i], orig_bufs[i], size, ARMCI_GROUP_WORLD.rank);
        gmr_dla_unlock(mreg);

        MPI_Free_mem(new_bufs[i]);
      }
    }
  }
}


/** Check if an operation with the given parameters requires scaling.
  *
  * @param[in] datatype Type of the data involved in the operation
  * @param[in] scale    Value of type datatype to scale
  * @return             Nonzero if scale is not the identity scale
  */
int ARMCII_Buf_acc_is_scaled(int datatype, void *scale) {
  switch (datatype) {
    case ARMCI_ACC_INT:
      if (*((int*)scale) == 1)
        return 0;
      break;

    case ARMCI_ACC_LNG:
      if (*((long*)scale) == 1)
        return 0;
      break;

    case ARMCI_ACC_FLT:
      if (*((float*)scale) == 1.0)
        return 0;
      break;

    case ARMCI_ACC_DBL:
      if (*((double*)scale) == 1.0)
        return 0;
      break;

    case ARMCI_ACC_CPL:
      if (((float*)scale)[0] == 1.0 && ((float*)scale)[1] == 0.0)
        return 0;
      break;

    case ARMCI_ACC_DCP:
      if (((double*)scale)[0] == 1.0 && ((double*)scale)[1] == 0.0)
        return 0;
      break;

    default:
      ARMCII_Error("unknown data type (%d)", datatype);
  }

  return 1;
}


/** Prepare a set of buffers for use with an accumulate operation.  The
  * returned set of buffers is guaranteed to be in private space and scaled.
  * Copies will be made if needed, the result should be completed by finish.
  *
  * @param[in]  buf       Original set of buffers.
  * @param[in]  count     Number of entries in the buffer list.
  * @param[in]  size      The size of the buffers (all are of the same size).
  * @param[in]  datatype  The type of the buffer.
  * @param[in]  scale     Scaling constant to apply to each buffer.
  * @return               Pointer to the new buffer or buf
  */
void ARMCII_Buf_acc_scale(void *buf_in, void *buf_out, int size, int datatype, void *scale) {
  int   j, nelem;
  int   type_size;
  MPI_Datatype type;

  switch (datatype) {
    case ARMCI_ACC_INT:
      MPI_Type_size(MPI_INT, &type_size);
      type = MPI_INT;
      nelem= size/type_size;

      {
        int *src_i = (int*) buf_in;
        int *scl_i = (int*) buf_out;
        const int s = *((int*) scale);

        for (j = 0; j < nelem; j++)
          scl_i[j] = src_i[j]*s;
      }
      break;

    case ARMCI_ACC_LNG:
      MPI_Type_size(MPI_LONG, &type_size);
      type = MPI_LONG;
      nelem= size/type_size;

      {
        long *src_l = (long*) buf_in;
        long *scl_l = (long*) buf_out;
        const long s = *((long*) scale);

        for (j = 0; j < nelem; j++)
          scl_l[j] = src_l[j]*s;
      }
      break;

    case ARMCI_ACC_FLT:
      MPI_Type_size(MPI_FLOAT, &type_size);
      type = MPI_FLOAT;
      nelem= size/type_size;

      {
        float *src_f = (float*) buf_in;
        float *scl_f = (float*) buf_out;
        const float s = *((float*) scale);

        for (j = 0; j < nelem; j++)
          scl_f[j] = src_f[j]*s;
      }
      break;

    case ARMCI_ACC_DBL:
      MPI_Type_size(MPI_DOUBLE, &type_size);
      type = MPI_DOUBLE;
      nelem= size/type_size;

      {
        double *src_d = (double*) buf_in;
        double *scl_d = (double*) buf_out;
        const double s = *((double*) scale);

        for (j = 0; j < nelem; j++)
          scl_d[j] = src_d[j]*s;
      }
      break;

    case ARMCI_ACC_CPL:
      MPI_Type_size(MPI_FLOAT, &type_size);
      type = MPI_FLOAT;
      nelem= size/type_size;

      {
        float *src_fc = (float*) buf_in;
        float *scl_fc = (float*) buf_out;
        const float s_r = ((float*)scale)[0];
        const float s_c = ((float*)scale)[1];

        for (j = 0; j < nelem; j += 2) {
          // Complex multiplication: (a + bi)*(c + di)
          const float src_fc_j   = src_fc[j];
          const float src_fc_j_1 = src_fc[j+1];
          /*
          scl_fc[j]   = src_fc[j]*s_r   - src_fc[j+1]*s_c;
          scl_fc[j+1] = src_fc[j+1]*s_r + src_fc[j]*s_c;
          */
          scl_fc[j]   = src_fc_j*s_r   - src_fc_j_1*s_c;
          scl_fc[j+1] = src_fc_j_1*s_r + src_fc_j*s_c;
        }
      }
      break;

    case ARMCI_ACC_DCP:
      MPI_Type_size(MPI_DOUBLE, &type_size);
      type = MPI_DOUBLE;
      nelem= size/type_size;

      {
        double *src_dc = (double*) buf_in;
        double *scl_dc = (double*) buf_out;
        const double s_r = ((double*)scale)[0];
        const double s_c = ((double*)scale)[1];

        for (j = 0; j < nelem; j += 2) {
          // Complex multiplication: (a + bi)*(c + di)
          const double src_dc_j   = src_dc[j];
          const double src_dc_j_1 = src_dc[j+1];
          /*
          scl_dc[j]   = src_dc[j]*s_r   - src_dc[j+1]*s_c;
          scl_dc[j+1] = src_dc[j+1]*s_r + src_dc[j]*s_c;
          */
          scl_dc[j]   = src_dc_j*s_r   - src_dc_j_1*s_c;
          scl_dc[j+1] = src_dc_j_1*s_r + src_dc_j*s_c;
        }
      }
      break;

    default:
      ARMCII_Error("unknown data type (%d)", datatype);
  }

  ARMCII_Assert_msg(size % type_size == 0, 
      "Transfer size is not a multiple of the datatype size");
}
