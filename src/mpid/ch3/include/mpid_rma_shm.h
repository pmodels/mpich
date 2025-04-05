/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPID_RMA_SHM_H_INCLUDED
#define MPID_RMA_SHM_H_INCLUDED

#include "utlist.h"
#include "mpid_rma_types.h"

static inline int do_accumulate_op(void *source_buf, MPI_Aint source_count, MPI_Datatype source_dtp,
                                   void *target_buf, MPI_Aint target_count, MPI_Datatype target_dtp,
                                   MPI_Aint stream_offset, MPI_Op acc_op,
                                   MPIDI_RMA_Acc_srcbuf_kind_t srckind);

#define ASSIGN_COPY(src, dest, count, type)     \
    {                                           \
        type *src_ = (type *) src;              \
        type *dest_ = (type *) dest;            \
        int i;                                  \
        for (i = 0; i < count; i++)             \
            dest_[i] = src_[i];                 \
        goto fn_exit;                           \
    }

static inline int shm_copy(const void *src, int scount, MPI_Datatype stype,
                           void *dest, int dcount, MPI_Datatype dtype)
{
    int mpi_errno = MPI_SUCCESS;

    /* We use a threshold of operations under which a for loop of assignments is
     * used.  Even though this happens at smaller block lengths, making it
     * potentially inefficient, it can take advantage of some vectorization
     * available on most modern processors. */
#define SHM_OPS_THRESHOLD  (16)

    if (MPIR_DATATYPE_IS_PREDEFINED(stype) && MPIR_DATATYPE_IS_PREDEFINED(dtype) &&
        scount <= SHM_OPS_THRESHOLD) {

        /* FIXME: We currently only optimize a few predefined datatypes, which
         * have a direct C datatype mapping. */

        /* The below list of datatypes is based on those specified in the MPI-3
         * standard on page 665. */
        switch (stype) {
        case MPIR_INT8:
        case MPIR_UINT8:
            ASSIGN_COPY(src, dest, scount, MPIR_INT8_CTYPE);
        case MPIR_INT16:
        case MPIR_UINT16:
            ASSIGN_COPY(src, dest, scount, MPIR_INT16_CTYPE);
        case MPIR_INT32:
        case MPIR_UINT32:
            ASSIGN_COPY(src, dest, scount, MPIR_INT32_CTYPE);
        case MPIR_INT64:
        case MPIR_UINT64:
            ASSIGN_COPY(src, dest, scount, MPIR_INT64_CTYPE);
#ifdef MPIR_INT128_CTYPE
        case MPIR_INT128:
        case MPIR_UINT128:
            ASSIGN_COPY(src, dest, scount, MPIR_INT128_CTYPE);
#endif
#ifdef MPIR_FLOAT16_CTYPE
        case MPIR_FLOAT16:
            ASSIGN_COPY(src, dest, scount, MPIR_FLOAT16_CTYPE);
#endif
        case MPIR_FLOAT32:
            ASSIGN_COPY(src, dest, scount, MPIR_FLOAT32_CTYPE);
        case MPIR_FLOAT64:
            ASSIGN_COPY(src, dest, scount, MPIR_FLOAT64_CTYPE);
#ifdef MPIR_FLOAT128_CTYPE
        case MPIR_FLOAT128:
            ASSIGN_COPY(src, dest, scount, MPIR_FLOAT128_CTYPE);
#endif
        case MPIR_ALT_FLOAT96:
        case MPIR_ALT_FLOAT128:
            ASSIGN_COPY(src, dest, scount, long double);

        /* FIXME: native complex types */

        default:
            /* Just to make sure the switch statement is not empty */
            ;
        }
    }

    mpi_errno = MPIR_Localcopy(src, scount, stype, dest, dcount, dtype);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

static inline int MPIDI_CH3I_Shm_put_op(const void *origin_addr, MPI_Aint origin_count, MPI_Datatype
                                        origin_datatype, int target_rank, MPI_Aint target_disp,
                                        MPI_Aint target_count, MPI_Datatype target_datatype,
                                        MPIR_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    void *base = NULL;
    int disp_unit;

    MPIR_FUNC_ENTER;

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = MPIR_Get_intranode_rank(win_ptr->comm_ptr, target_rank);
        MPIR_Assert(local_target_rank >= 0);
        base = win_ptr->shm_base_addrs[local_target_rank];
        disp_unit = win_ptr->basic_info_table[target_rank].disp_unit;
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    mpi_errno = shm_copy(origin_addr, origin_count, origin_datatype,
                         (char *) base + disp_unit * target_disp, target_count, target_datatype);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


static inline int MPIDI_CH3I_Shm_acc_op(const void *origin_addr, MPI_Aint origin_count, MPI_Datatype
                                        origin_datatype, int target_rank, MPI_Aint target_disp,
                                        MPI_Aint target_count, MPI_Datatype target_datatype, MPI_Op op,
                                        MPIR_Win * win_ptr)
{
    void *base = NULL;
    int disp_unit, shm_op = 0;
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPI_Datatype basic_type;
    MPI_Aint stream_elem_count, stream_unit_count;
    MPI_Aint predefined_dtp_size, predefined_dtp_extent, predefined_dtp_count;
    MPI_Aint total_len, rest_len;
    MPI_Aint origin_dtp_size;
    MPIR_Datatype*origin_dtp_ptr = NULL;

    MPIR_FUNC_ENTER;

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = MPIR_Get_intranode_rank(win_ptr->comm_ptr, target_rank);
        MPIR_Assert(local_target_rank >= 0);
        shm_op = 1;
        base = win_ptr->shm_base_addrs[local_target_rank];
        disp_unit = win_ptr->basic_info_table[target_rank].disp_unit;
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    if (MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
        if (shm_op) {
            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        }
        mpi_errno = do_accumulate_op((void *) origin_addr, origin_count, origin_datatype,
                                     MPIR_get_contig_ptr(base, disp_unit * target_disp),
                                     target_count, target_datatype, 0, op,
                                     MPIDI_RMA_ACC_SRCBUF_DEFAULT);
        if (shm_op) {
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        }

        MPIR_ERR_CHECK(mpi_errno);

        goto fn_exit;
    }

    /* Get total length of origin data */
    MPIR_Datatype_get_size_macro(origin_datatype, origin_dtp_size);
    total_len = origin_dtp_size * origin_count;

    MPIR_Datatype_get_ptr(origin_datatype, origin_dtp_ptr);
    MPIR_Assert(origin_dtp_ptr != NULL && origin_dtp_ptr->basic_type != MPI_DATATYPE_NULL);
    basic_type = origin_dtp_ptr->basic_type;
    MPIR_Datatype_get_size_macro(basic_type, predefined_dtp_size);
    predefined_dtp_count = total_len / predefined_dtp_size;
    MPIR_Datatype_get_extent_macro(basic_type, predefined_dtp_extent);
    MPIR_Assert(predefined_dtp_count > 0 && predefined_dtp_size > 0 && predefined_dtp_extent > 0);

    stream_elem_count = MPIDI_CH3U_Acc_stream_size / predefined_dtp_extent;
    stream_unit_count = (predefined_dtp_count - 1) / stream_elem_count + 1;
    MPIR_Assert(stream_elem_count > 0 && stream_unit_count > 0);

    rest_len = total_len;
    for (i = 0; i < stream_unit_count; i++) {
        void *packed_buf = NULL;
        MPI_Aint stream_offset, stream_size, stream_count;

        stream_offset = i * stream_elem_count * predefined_dtp_size;
        stream_size = MPL_MIN(stream_elem_count * predefined_dtp_size, rest_len);
        stream_count = stream_size / predefined_dtp_size;
        rest_len -= stream_size;

        packed_buf = MPL_malloc(stream_size, MPL_MEM_BUFFER);

        MPI_Aint actual_pack_bytes;
        MPIR_Typerep_pack(origin_addr, origin_count, origin_datatype,
                       stream_offset, packed_buf, stream_size, &actual_pack_bytes,
                       MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(actual_pack_bytes == stream_size);

        if (shm_op) {
            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        }

        MPIR_Assert(stream_count == (int) stream_count);
        mpi_errno = do_accumulate_op((void *) packed_buf, (int) stream_count, basic_type,
                                     (void *) ((char *) base + disp_unit * target_disp),
                                     target_count, target_datatype, stream_offset, op,
                                     MPIDI_RMA_ACC_SRCBUF_PACKED);

        if (shm_op) {
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        }

        MPIR_ERR_CHECK(mpi_errno);

        MPL_free(packed_buf);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


static inline int MPIDI_CH3I_Shm_get_acc_op(const void *origin_addr, MPI_Aint origin_count, MPI_Datatype
                                            origin_datatype, void *result_addr, MPI_Aint result_count,
                                            MPI_Datatype result_datatype, int target_rank, MPI_Aint
                                            target_disp, MPI_Aint target_count,
                                            MPI_Datatype target_datatype, MPI_Op op,
                                            MPIR_Win * win_ptr)
{
    int disp_unit, shm_locked = 0;
    void *base = NULL;
    int i;
    MPI_Datatype basic_type;
    MPI_Aint stream_elem_count, stream_unit_count;
    MPI_Aint predefined_dtp_size, predefined_dtp_extent, predefined_dtp_count;
    MPI_Aint total_len, rest_len;
    MPI_Aint origin_dtp_size;
    MPIR_Datatype*origin_dtp_ptr = NULL;
    int is_empty_origin = FALSE;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* Judge if origin buffer is empty */
    if (op == MPI_NO_OP)
        is_empty_origin = TRUE;

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = MPIR_Get_intranode_rank(win_ptr->comm_ptr, target_rank);
        MPIR_Assert(local_target_rank >= 0);
        base = win_ptr->shm_base_addrs[local_target_rank];
        disp_unit = win_ptr->basic_info_table[target_rank].disp_unit;
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        shm_locked = 1;
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    /* Perform the local get first, then the accumulate */
    mpi_errno = shm_copy((char *) base + disp_unit * target_disp, target_count, target_datatype,
                         result_addr, result_count, result_datatype);
    MPIR_ERR_CHECK(mpi_errno);

    if (is_empty_origin == TRUE || MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {

        mpi_errno = do_accumulate_op((void *) origin_addr, origin_count, origin_datatype,
                                     (void *) ((char *) base + disp_unit * target_disp),
                                     target_count, target_datatype, 0, op,
                                     MPIDI_RMA_ACC_SRCBUF_DEFAULT);
        if (shm_locked) {
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        }

        MPIR_ERR_CHECK(mpi_errno);

        goto fn_exit;
    }

    /* Get total length of origin data */
    MPIR_Datatype_get_size_macro(origin_datatype, origin_dtp_size);
    total_len = origin_dtp_size * origin_count;

    MPIR_Datatype_get_ptr(origin_datatype, origin_dtp_ptr);
    MPIR_Assert(origin_dtp_ptr != NULL && origin_dtp_ptr->basic_type != MPI_DATATYPE_NULL);
    basic_type = origin_dtp_ptr->basic_type;
    MPIR_Datatype_get_size_macro(basic_type, predefined_dtp_size);
    predefined_dtp_count = total_len / predefined_dtp_size;
    MPIR_Datatype_get_extent_macro(basic_type, predefined_dtp_extent);
    MPIR_Assert(predefined_dtp_count > 0 && predefined_dtp_size > 0 && predefined_dtp_extent > 0);

    stream_elem_count = MPIDI_CH3U_Acc_stream_size / predefined_dtp_extent;
    stream_unit_count = (predefined_dtp_count - 1) / stream_elem_count + 1;
    MPIR_Assert(stream_elem_count > 0 && stream_unit_count > 0);

    rest_len = total_len;
    for (i = 0; i < stream_unit_count; i++) {
        void *packed_buf = NULL;
        MPI_Aint stream_offset, stream_size, stream_count;

        stream_offset = i * stream_elem_count * predefined_dtp_size;
        stream_size = MPL_MIN(stream_elem_count * predefined_dtp_size, rest_len);
        stream_count = stream_size / predefined_dtp_size;
        rest_len -= stream_size;

        packed_buf = MPL_malloc(stream_size, MPL_MEM_BUFFER);

        MPI_Aint actual_pack_bytes;
        MPIR_Typerep_pack(origin_addr, origin_count, origin_datatype,
                       stream_offset, packed_buf, stream_size, &actual_pack_bytes,
                       MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(actual_pack_bytes == stream_size);

        MPIR_Assert(stream_count == (int) stream_count);
        mpi_errno = do_accumulate_op((void *) packed_buf, (int) stream_count, basic_type,
                                     (void *) ((char *) base + disp_unit * target_disp),
                                     target_count, target_datatype, stream_offset, op,
                                     MPIDI_RMA_ACC_SRCBUF_PACKED);

        MPIR_ERR_CHECK(mpi_errno);

        MPL_free(packed_buf);
    }

    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        shm_locked = 0;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


static inline int MPIDI_CH3I_Shm_get_op(void *origin_addr, MPI_Aint origin_count,
                                        MPI_Datatype origin_datatype, int target_rank,
                                        MPI_Aint target_disp, MPI_Aint target_count,
                                        MPI_Datatype target_datatype, MPIR_Win * win_ptr)
{
    void *base = NULL;
    int disp_unit;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = MPIR_Get_intranode_rank(win_ptr->comm_ptr, target_rank);
        MPIR_Assert(local_target_rank >= 0);
        base = win_ptr->shm_base_addrs[local_target_rank];
        disp_unit = win_ptr->basic_info_table[target_rank].disp_unit;
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    mpi_errno = shm_copy((char *) base + disp_unit * target_disp, target_count, target_datatype,
                         origin_addr, origin_count, origin_datatype);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


static inline int MPIDI_CH3I_Shm_cas_op(const void *origin_addr, const void *compare_addr,
                                        void *result_addr, MPI_Datatype datatype, int target_rank,
                                        MPI_Aint target_disp, MPIR_Win * win_ptr)
{
    void *base = NULL, *dest_addr = NULL;
    int disp_unit;
    MPI_Aint len;
    int shm_locked = 0;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = MPIR_Get_intranode_rank(win_ptr->comm_ptr, target_rank);
        MPIR_Assert(local_target_rank >= 0);
        base = win_ptr->shm_base_addrs[local_target_rank];
        disp_unit = win_ptr->basic_info_table[target_rank].disp_unit;

        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        shm_locked = 1;
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    dest_addr = (char *) base + disp_unit * target_disp;

    MPIR_Datatype_get_size_macro(datatype, len);
    MPIR_Memcpy(result_addr, dest_addr, len);

    if (MPIR_Compare_equal(compare_addr, dest_addr, datatype)) {
        MPIR_Memcpy(dest_addr, origin_addr, len);
    }

    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        shm_locked = 0;
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}


static inline int MPIDI_CH3I_Shm_fop_op(const void *origin_addr, void *result_addr,
                                        MPI_Datatype datatype, int target_rank,
                                        MPI_Aint target_disp, MPI_Op op, MPIR_Win * win_ptr)
{
    void *base = NULL, *dest_addr = NULL;
    MPIR_op_function *uop = NULL;
    int disp_unit;
    MPI_Aint len;
    int shm_locked = 0;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* FIXME: shouldn't this be an assert? */
    if (!MPIR_Internal_op_dt_check(op, datatype)) {
        goto fn_exit;
    }

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = MPIR_Get_intranode_rank(win_ptr->comm_ptr, target_rank);
        MPIR_Assert(local_target_rank >= 0);
        base = win_ptr->shm_base_addrs[local_target_rank];
        disp_unit = win_ptr->basic_info_table[target_rank].disp_unit;

        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        shm_locked = 1;
    }
    else {
        base = win_ptr->base;
        disp_unit = win_ptr->disp_unit;
    }

    dest_addr = (char *) base + disp_unit * target_disp;

    MPIR_Datatype_get_size_macro(datatype, len);
    MPIR_Memcpy(result_addr, dest_addr, len);

    uop = MPIR_OP_HDL_TO_FN(op);
    MPI_Aint one = 1;

    (*uop) ((void *) origin_addr, dest_addr, &one, &datatype);

    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        shm_locked = 0;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}


#endif /* MPID_RMA_SHM_H_INCLUDED */
