/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_RMA_SHM_H_INCLUDED)
#define MPID_RMA_SHM_H_INCLUDED

#include "mpl_utlist.h"
#include "mpid_rma_types.h"

static inline int do_accumulate_op(void *source_buf, int source_count, MPI_Datatype source_dtp,
                                   void *target_buf, int target_count, MPI_Datatype target_dtp,
                                   MPI_Aint stream_offset, MPI_Op acc_op);

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
        case MPI_CHAR:
            ASSIGN_COPY(src, dest, scount, char);

        case MPI_SHORT:
            ASSIGN_COPY(src, dest, scount, signed short int);

        case MPI_INT:
            ASSIGN_COPY(src, dest, scount, signed int);

        case MPI_LONG:
            ASSIGN_COPY(src, dest, scount, signed long int);

        case MPI_LONG_LONG_INT:        /* covers MPI_LONG_LONG too */
            ASSIGN_COPY(src, dest, scount, signed long long int);

        case MPI_SIGNED_CHAR:
            ASSIGN_COPY(src, dest, scount, signed char);

        case MPI_UNSIGNED_CHAR:
            ASSIGN_COPY(src, dest, scount, unsigned char);

        case MPI_UNSIGNED_SHORT:
            ASSIGN_COPY(src, dest, scount, unsigned short int);

        case MPI_UNSIGNED:
            ASSIGN_COPY(src, dest, scount, unsigned int);

        case MPI_UNSIGNED_LONG:
            ASSIGN_COPY(src, dest, scount, unsigned long int);

        case MPI_UNSIGNED_LONG_LONG:
            ASSIGN_COPY(src, dest, scount, unsigned long long int);

        case MPI_FLOAT:
            ASSIGN_COPY(src, dest, scount, float);

        case MPI_DOUBLE:
            ASSIGN_COPY(src, dest, scount, double);

        case MPI_LONG_DOUBLE:
            ASSIGN_COPY(src, dest, scount, long double);

#if 0
            /* FIXME: we need a configure check to define HAVE_WCHAR_T before
             * this can be enabled */
        case MPI_WCHAR:
            ASSIGN_COPY(src, dest, scount, wchar_t);
#endif

#if 0
            /* FIXME: we need a configure check to define HAVE_C_BOOL before
             * this can be enabled */
        case MPI_C_BOOL:
            ASSIGN_COPY(src, dest, scount, _Bool);
#endif

#if HAVE_INT8_T
        case MPI_INT8_T:
            ASSIGN_COPY(src, dest, scount, int8_t);
#endif /* HAVE_INT8_T */

#if HAVE_INT16_T
        case MPI_INT16_T:
            ASSIGN_COPY(src, dest, scount, int16_t);
#endif /* HAVE_INT16_T */

#if HAVE_INT32_T
        case MPI_INT32_T:
            ASSIGN_COPY(src, dest, scount, int32_t);
#endif /* HAVE_INT32_T */

#if HAVE_INT64_T
        case MPI_INT64_T:
            ASSIGN_COPY(src, dest, scount, int64_t);
#endif /* HAVE_INT64_T */

#if HAVE_UINT8_T
        case MPI_UINT8_T:
            ASSIGN_COPY(src, dest, scount, uint8_t);
#endif /* HAVE_UINT8_T */

#if HAVE_UINT16_T
        case MPI_UINT16_T:
            ASSIGN_COPY(src, dest, scount, uint16_t);
#endif /* HAVE_UINT16_T */

#if HAVE_UINT32_T
        case MPI_UINT32_T:
            ASSIGN_COPY(src, dest, scount, uint32_t);
#endif /* HAVE_UINT32_T */

#if HAVE_UINT64_T
        case MPI_UINT64_T:
            ASSIGN_COPY(src, dest, scount, uint64_t);
#endif /* HAVE_UINT64_T */

        case MPI_AINT:
            ASSIGN_COPY(src, dest, scount, MPI_Aint);

        case MPI_COUNT:
            ASSIGN_COPY(src, dest, scount, MPI_Count);

        case MPI_OFFSET:
            ASSIGN_COPY(src, dest, scount, MPI_Offset);

#if 0
            /* FIXME: we need a configure check to define HAVE_C_COMPLEX before
             * this can be enabled */
        case MPI_C_COMPLEX:    /* covers MPI_C_FLOAT_COMPLEX as well */
            ASSIGN_COPY(src, dest, scount, float _Complex);
#endif

#if 0
            /* FIXME: we need a configure check to define HAVE_C_DOUPLE_COMPLEX
             * before this can be enabled */
        case MPI_C_DOUBLE_COMPLEX:
            ASSIGN_COPY(src, dest, scount, double _Complex);
#endif

#if 0
            /* FIXME: we need a configure check to define
             * HAVE_C_LONG_DOUPLE_COMPLEX before this can be enabled */
        case MPI_C_LONG_DOUBLE_COMPLEX:
            ASSIGN_COPY(src, dest, scount, long double _Complex);
#endif

#if 0
            /* Types that don't have a direct equivalent */
        case MPI_BYTE:
        case MPI_PACKED:
#endif

#if 0   /* Fortran types */
        case MPI_INTEGER:
        case MPI_REAL:
        case MPI_DOUBLE_PRECISION:
        case MPI_COMPLEX:
        case MPI_LOGICAL:
        case MPI_CHARACTER:
#endif

#if 0   /* C++ types */
        case MPI_CXX_BOOL:
        case MPI_CXX_FLOAT_COMPLEX:
        case MPI_CXX_DOUBLE_COMPLEX:
        case MPI_CXX_LONG_DOUBLE_COMPLEX:
#endif

#if 0   /* Optional Fortran types */
        case MPI_DOUBLE_COMPLEX:
        case MPI_INTEGER1:
        case MPI_INTEGER2:
        case MPI_INTEGER4:
        case MPI_INTEGER8:
        case MPI_INTEGER16:
        case MPI_REAL2:
        case MPI_REAL4:
        case MPI_REAL8:
        case MPI_REAL16:
        case MPI_COMPLEX4:
        case MPI_COMPLEX8:
        case MPI_COMPLEX16:
        case MPI_COMPLEX32:
#endif

#if 0   /* C datatypes for reduction functions */
        case MPI_FLOAT_INT:
        case MPI_DOUBLE_INT:
        case MPI_LONG_INT:
        case MPI_2INT:
        case MPI_LONG_DOUBLE_INT:
#endif

#if 0   /* Fortran datatypes for reduction functions */
        case MPI_2REAL:
        case MPI_2DOUBLE_PRECISION:
        case MPI_2INTEGER:
#endif

#if 0   /* Random types not present in the standard */
        case MPI_2COMPLEX:
        case MPI_2DOUBLE_COMPLEX:
#endif

        default:
            /* Just to make sure the switch statement is not empty */
            ;
        }
    }

    mpi_errno = MPIR_Localcopy(src, scount, stype, dest, dcount, dtype);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_put_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_put_op(const void *origin_addr, int origin_count, MPI_Datatype
                                        origin_datatype, int target_rank, MPI_Aint target_disp,
                                        int target_count, MPI_Datatype target_datatype,
                                        MPIR_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    void *base = NULL;
    int disp_unit;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_PUT_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3I_SHM_PUT_OP);

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = win_ptr->comm_ptr->intranode_table[target_rank];
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
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3I_SHM_PUT_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_acc_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_acc_op(const void *origin_addr, int origin_count, MPI_Datatype
                                        origin_datatype, int target_rank, MPI_Aint target_disp,
                                        int target_count, MPI_Datatype target_datatype, MPI_Op op,
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_ACC_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3I_SHM_ACC_OP);

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = win_ptr->comm_ptr->intranode_table[target_rank];
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
                                     (void *) ((char *) base + disp_unit * target_disp),
                                     target_count, target_datatype, 0, op);
        if (shm_op) {
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        }

        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

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
        MPIR_Segment *seg = NULL;
        void *packed_buf = NULL;
        MPI_Aint first, last;
        int is_predef_contig;
        MPI_Aint stream_offset, stream_size, stream_count;

        stream_offset = i * stream_elem_count * predefined_dtp_size;
        stream_size = MPL_MIN(stream_elem_count * predefined_dtp_size, rest_len);
        stream_count = stream_size / predefined_dtp_size;
        rest_len -= stream_size;

        first = stream_offset;
        last = stream_offset + stream_size;

        packed_buf = MPL_malloc(stream_size);

        seg = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(seg == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPIR_Segment");
        MPIR_Segment_init(origin_addr, origin_count, origin_datatype, seg, 0);
        MPIR_Segment_pack(seg, first, &last, packed_buf);
        MPIR_Segment_free(seg);

        MPIR_Datatype_is_contig(basic_type, &is_predef_contig);

        if (!is_predef_contig) {
            void *tmpbuf = MPL_malloc(stream_count * predefined_dtp_extent);
            mpi_errno = MPIR_Localcopy(tmpbuf, stream_count, basic_type,
                                       packed_buf, stream_size, MPI_BYTE);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
            MPL_free(packed_buf);
            packed_buf = tmpbuf;
        }

        if (shm_op) {
            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        }

        MPIR_Assert(stream_count == (int) stream_count);
        mpi_errno = do_accumulate_op((void *) packed_buf, (int) stream_count, basic_type,
                                     (void *) ((char *) base + disp_unit * target_disp),
                                     target_count, target_datatype, stream_offset, op);

        if (shm_op) {
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        }

        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        MPL_free(packed_buf);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ACC_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_get_acc_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_get_acc_op(const void *origin_addr, int origin_count, MPI_Datatype
                                            origin_datatype, void *result_addr, int result_count,
                                            MPI_Datatype result_datatype, int target_rank, MPI_Aint
                                            target_disp, int target_count,
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_ACC_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_ACC_OP);

    /* Judge if origin buffer is empty */
    if (op == MPI_NO_OP)
        is_empty_origin = TRUE;

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = win_ptr->comm_ptr->intranode_table[target_rank];
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
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

    if (is_empty_origin == TRUE || MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {

        mpi_errno = do_accumulate_op((void *) origin_addr, origin_count, origin_datatype,
                                     (void *) ((char *) base + disp_unit * target_disp),
                                     target_count, target_datatype, 0, op);
        if (shm_locked) {
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        }

        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

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
        MPIR_Segment *seg = NULL;
        void *packed_buf = NULL;
        MPI_Aint first, last;
        int is_predef_contig;
        MPI_Aint stream_offset, stream_size, stream_count;

        stream_offset = i * stream_elem_count * predefined_dtp_size;
        stream_size = MPL_MIN(stream_elem_count * predefined_dtp_size, rest_len);
        stream_count = stream_size / predefined_dtp_size;
        rest_len -= stream_size;

        first = stream_offset;
        last = stream_offset + stream_size;

        packed_buf = MPL_malloc(stream_size);

        seg = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(seg == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPIR_Segment");
        MPIR_Segment_init(origin_addr, origin_count, origin_datatype, seg, 0);
        MPIR_Segment_pack(seg, first, &last, packed_buf);
        MPIR_Segment_free(seg);

        MPIR_Datatype_is_contig(basic_type, &is_predef_contig);

        if (!is_predef_contig) {
            void *tmpbuf = MPL_malloc(stream_count * predefined_dtp_extent);
            mpi_errno = MPIR_Localcopy(tmpbuf, stream_count, basic_type,
                                       packed_buf, stream_size, MPI_BYTE);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
            MPL_free(packed_buf);
            packed_buf = tmpbuf;
        }

        MPIR_Assert(stream_count == (int) stream_count);
        mpi_errno = do_accumulate_op((void *) packed_buf, (int) stream_count, basic_type,
                                     (void *) ((char *) base + disp_unit * target_disp),
                                     target_count, target_datatype, stream_offset, op);

        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        MPL_free(packed_buf);
    }

    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        shm_locked = 0;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_ACC_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_get_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_get_op(void *origin_addr, int origin_count,
                                        MPI_Datatype origin_datatype, int target_rank,
                                        MPI_Aint target_disp, int target_count,
                                        MPI_Datatype target_datatype, MPIR_Win * win_ptr)
{
    void *base = NULL;
    int disp_unit;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_OP);

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = win_ptr->comm_ptr->intranode_table[target_rank];
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
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_cas_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_cas_op(const void *origin_addr, const void *compare_addr,
                                        void *result_addr, MPI_Datatype datatype, int target_rank,
                                        MPI_Aint target_disp, MPIR_Win * win_ptr)
{
    void *base = NULL, *dest_addr = NULL;
    int disp_unit;
    MPI_Aint len;
    int shm_locked = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_CAS_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3I_SHM_CAS_OP);

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = win_ptr->comm_ptr->intranode_table[target_rank];
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

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3I_SHM_CAS_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Shm_fop_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Shm_fop_op(const void *origin_addr, void *result_addr,
                                        MPI_Datatype datatype, int target_rank,
                                        MPI_Aint target_disp, MPI_Op op, MPIR_Win * win_ptr)
{
    void *base = NULL, *dest_addr = NULL;
    MPI_User_function *uop = NULL;
    int disp_unit;
    MPI_Aint len;
    int one, shm_locked = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_FOP_OP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_MPIDI_CH3I_SHM_FOP_OP);

    if (win_ptr->shm_allocated == TRUE) {
        int local_target_rank = win_ptr->comm_ptr->intranode_table[target_rank];
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
    one = 1;

    (*uop) ((void *) origin_addr, dest_addr, &one, &datatype);

    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        shm_locked = 0;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_MPIDI_CH3I_SHM_FOP_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#endif /* MPID_RMA_SHM_H_INCLUDED */
