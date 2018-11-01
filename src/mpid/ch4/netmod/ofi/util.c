/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include <mpidimpl.h>
#include "ofi_impl.h"
#include "ofi_events.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_handle_cq_error_util
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_OFI_handle_cq_error_util(int vni_idx, ssize_t ret)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_HANDLE_CQ_ERROR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_HANDLE_CQ_ERROR);

    mpi_errno = MPIDI_OFI_handle_cq_error(vni_idx, ret);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_HANDLE_CQ_ERROR);
    return mpi_errno;
}

int MPIDI_OFI_retry_progress()
{
    /* We do not call progress on hooks form netmod level
     * because it is not reentrant safe.
     */
    return MPIDI_Progress_test(MPIDI_PROGRESS_NM | MPIDI_PROGRESS_SHM);
}

typedef struct MPIDI_OFI_index_allocator_t {
    int chunk_size;
    int num_ints;
    int start;
    int last_free_index;
    uint64_t *bitmask;
} MPIDI_OFI_index_allocator_t;

void MPIDI_OFI_index_allocator_create(void **indexmap, int start, MPL_memory_class class)
{
    MPIDI_OFI_index_allocator_t *allocator;
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_UTIL_MUTEX);
    allocator = MPL_malloc(sizeof(MPIDI_OFI_index_allocator_t), class);
    allocator->chunk_size = 128;
    allocator->num_ints = allocator->chunk_size;
    allocator->start = start;
    allocator->last_free_index = 0;
    allocator->bitmask = MPL_malloc(sizeof(uint64_t) * allocator->num_ints, class);
    memset(allocator->bitmask, 0xFF, sizeof(uint64_t) * allocator->num_ints);
    assert(allocator != NULL);
    *indexmap = allocator;
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_UTIL_MUTEX);
}

#define MPIDI_OFI_INDEX_CALC(val,nval,shift,mask) \
    if ((val & mask) == 0) {                              \
        val >>= shift##ULL;                               \
        nval += shift;                                    \
    }
int MPIDI_OFI_index_allocator_alloc(void *indexmap, MPL_memory_class class)
{
    int i;
    MPIDI_OFI_index_allocator_t *allocator = indexmap;
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_UTIL_MUTEX);
    for (i = allocator->last_free_index; i < allocator->num_ints; i++) {
        if (allocator->bitmask[i]) {
            register uint64_t val, nval;
            val = allocator->bitmask[i];
            nval = 2;
            MPIDI_OFI_INDEX_CALC(val, nval, 32, 0xFFFFFFFFULL);
            MPIDI_OFI_INDEX_CALC(val, nval, 16, 0xFFFFULL);
            MPIDI_OFI_INDEX_CALC(val, nval, 8, 0xFFULL);
            MPIDI_OFI_INDEX_CALC(val, nval, 4, 0xFULL);
            MPIDI_OFI_INDEX_CALC(val, nval, 2, 0x3ULL);
            nval -= val & 0x1ULL;
            allocator->bitmask[i] &= ~(0x1ULL << (nval - 1));
            allocator->last_free_index = i;
            MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_UTIL_MUTEX);
            return i * sizeof(uint64_t) * 8 + (nval - 1) + allocator->start;
        }
        if (i == allocator->num_ints - 1) {
            allocator->num_ints += allocator->chunk_size;
            allocator->bitmask = MPL_realloc(allocator->bitmask,
                                             sizeof(uint64_t) * allocator->num_ints, class);
            MPIR_Assert(allocator->bitmask);
            memset(&allocator->bitmask[i + 1], 0xFF, sizeof(uint64_t) * allocator->chunk_size);
        }
    }
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_UTIL_MUTEX);
    return -1;
}

void MPIDI_OFI_index_allocator_free(void *indexmap, int index)
{
    int int_index, bitpos, numbits;
    MPIDI_OFI_index_allocator_t *allocator = indexmap;
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_UTIL_MUTEX);
    numbits = sizeof(uint64_t) * 8;
    int_index = (index + 1 - allocator->start) / numbits;
    bitpos = (index - allocator->start) % numbits;

    allocator->last_free_index = MPL_MIN(int_index, allocator->last_free_index);
    allocator->bitmask[int_index] |= (0x1ULL << bitpos);
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_UTIL_MUTEX);
}

void MPIDI_OFI_index_allocator_destroy(void *indexmap)
{
    MPIDI_OFI_index_allocator_t *allocator;
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_OFI_THREAD_UTIL_MUTEX);
    allocator = (MPIDI_OFI_index_allocator_t *) indexmap;
    MPL_free(allocator->bitmask);
    MPL_free(allocator);
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_OFI_THREAD_UTIL_MUTEX);
}

/* Translate the control message to get a huge message into a request to
 * actually perform the data transfer. */
static inline int MPIDI_OFI_get_huge(MPIDI_OFI_send_control_t * info)
{
    MPIDI_OFI_huge_recv_t *recv = NULL;
    MPIR_Comm *comm_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_GET_HUGE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_GET_HUGE);

    /* Look up the communicator */
    comm_ptr = MPIDI_CH4U_context_id_to_comm(info->comm_id);

    /* If there has been a posted receive, search through the list of unmatched
     * receives to find the one that goes with the incoming message. */
    {
        MPIDI_OFI_huge_recv_list_t *list_ptr;

        MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                        (MPL_DBG_FDEST, "SEARCHING POSTED LIST: (%d, %d, %d)", info->comm_id,
                         info->origin_rank, info->tag));

        LL_FOREACH(MPIDI_posted_huge_recv_head, list_ptr) {
            if (list_ptr->comm_id == info->comm_id &&
                list_ptr->rank == info->origin_rank && list_ptr->tag == info->tag) {
                MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                                (MPL_DBG_FDEST, "MATCHED POSTED LIST: (%d, %d, %d, %d)",
                                 info->comm_id, info->origin_rank, info->tag,
                                 list_ptr->rreq->handle));

                LL_DELETE(MPIDI_posted_huge_recv_head, MPIDI_posted_huge_recv_tail, list_ptr);

                recv = (MPIDI_OFI_huge_recv_t *)
                    MPIDI_CH4U_map_lookup(MPIDI_OFI_COMM(comm_ptr).huge_recv_counters,
                                          list_ptr->rreq->handle);

                MPL_free(list_ptr);
                break;
            }
        }
    }

    if (recv == NULL) { /* Put the struct describing the transfer on an
                         * unexpected list to be retrieved later */
        MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                        (MPL_DBG_FDEST, "CREATING UNEXPECTED HUGE RECV: (%d, %d, %d)",
                         info->comm_id, info->origin_rank, info->tag));

        /* If this is unexpected, create a new tracker and put it in the unexpected list. */
        recv = (MPIDI_OFI_huge_recv_t *) MPL_calloc(sizeof(*recv), 1, MPL_MEM_COMM);
        if (!recv)
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");

        LL_APPEND(MPIDI_unexp_huge_recv_head, MPIDI_unexp_huge_recv_tail, recv);
    }

    recv->event_id = MPIDI_OFI_EVENT_GET_HUGE;
    recv->cur_offset = MPIDI_Global.max_send;
    recv->remote_info = *info;
    recv->comm_ptr = comm_ptr;
    recv->next = NULL;
    MPIDI_OFI_get_huge_event(NULL, (MPIR_Request *) recv);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_GET_HUGE);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_control_handler(int handler_id, void *am_hdr,
                              void **data,
                              size_t * data_sz,
                              int *is_contig,
                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_send_control_t *ctrlsend = (MPIDI_OFI_send_control_t *) am_hdr;
    *req = NULL;
    *target_cmpl_cb = NULL;

    switch (ctrlsend->type) {
        case MPIDI_OFI_CTRL_HUGEACK:{
                mpi_errno = MPIDI_OFI_dispatch_function(NULL, ctrlsend->ackreq, 0);
                goto fn_exit;
            }
            break;

        case MPIDI_OFI_CTRL_HUGE:{
                mpi_errno = MPIDI_OFI_get_huge(ctrlsend);
                goto fn_exit;
            }
            break;

        default:
            fprintf(stderr, "Bad control type: 0x%08x  %d\n", ctrlsend->type, ctrlsend->type);
            MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
}


/* MPI Datatype Processing for RMA */
#define isS_INT(x) ((x)==MPI_INTEGER ||                                \
    (x) == MPI_INT32_T || (x) == MPI_INTEGER4 ||       \
                     (x) == MPI_INT)
#define isUS_INT(x) ((x) == MPI_UINT32_T || (x) == MPI_UNSIGNED)
#define isS_SHORT(x) ((x) == MPI_SHORT || (x) == MPI_INT16_T ||        \
                       (x) == MPI_INTEGER2)
#define isUS_SHORT(x) ((x) == MPI_UNSIGNED_SHORT || (x) == MPI_UINT16_T)
#define isS_CHAR(x) ((x) == MPI_SIGNED_CHAR || (x) == MPI_INT8_T ||    \
                      (x) == MPI_INTEGER1 || (x) == MPI_CHAR)
#define isUS_CHAR(x) ((x) == MPI_BYTE ||                               \
                       (x) == MPI_UNSIGNED_CHAR || (x) == MPI_UINT8_T)
#define isS_LONG(x) ((x) == MPI_LONG || (x) == MPI_AINT)
#define isUS_LONG(x) ((x) == MPI_UNSIGNED_LONG)
#define isS_LONG_LONG(x) ((x) == MPI_INT64_T || (x) == MPI_OFFSET ||   \
    (x) == MPI_INTEGER8 || (x) == MPI_LONG_LONG || \
                           (x) == MPI_LONG_LONG_INT || (x) == MPI_COUNT)
#define isUS_LONG_LONG(x) ((x) == MPI_UINT64_T || (x) == MPI_UNSIGNED_LONG_LONG)
#define isFLOAT(x) ((x) == MPI_FLOAT || (x) == MPI_REAL)
#define isDOUBLE(x) ((x) == MPI_DOUBLE || (x) == MPI_DOUBLE_PRECISION)
#define isLONG_DOUBLE(x) ((x) == MPI_LONG_DOUBLE)
#define isLOC_TYPE(x) ((x) == MPI_2REAL || (x) == MPI_2DOUBLE_PRECISION || \
    (x) == MPI_2INTEGER || (x) == MPI_FLOAT_INT ||  \
    (x) == MPI_DOUBLE_INT || (x) == MPI_LONG_INT || \
    (x) == MPI_2INT || (x) == MPI_SHORT_INT ||      \
                        (x) == MPI_LONG_DOUBLE_INT)
#define isBOOL(x) ((x) == MPI_C_BOOL)
#define isLOGICAL(x) ((x) == MPI_LOGICAL)
#define isSINGLE_COMPLEX(x) ((x) == MPI_COMPLEX || (x) == MPI_C_FLOAT_COMPLEX)
#define isDOUBLE_COMPLEX(x) ((x) == MPI_DOUBLE_COMPLEX || (x) == MPI_COMPLEX8 || \
                              (x) == MPI_C_DOUBLE_COMPLEX)

#undef FUNCNAME
#define FUNCNAME check_mpi_acc_valid
#undef FCNAME
#define FCNAME MPL_QUOTE(check_mpi_acc_valid)
MPL_STATIC_INLINE_PREFIX bool check_mpi_acc_valid(MPI_Datatype dtype, MPI_Op op)
{
    bool valid_flag = false;

    /* Check if the <datatype, op> is supported by MPICH. Note that MPICH
     * supports more combinations than that specified in standard (see definition
     * of these checking routines for extended support). */

    /* Predefined reduce operation, NO_OP, REPLACE */
    if (op != MPI_OP_NULL) {
        int mpi_errno;
        mpi_errno = (*MPIR_OP_HDL_TO_DTYPE_FN(op)) (dtype);
        if (mpi_errno == MPI_SUCCESS)
            valid_flag = TRUE;
    } else {
        /* Compare and swap */
        if (MPIR_Type_is_rma_atomic(dtype))
            valid_flag = true;
    }

    return valid_flag;
}

#undef FUNCNAME
#define FUNCNAME mpi_to_ofi
#undef FCNAME
#define FCNAME MPL_QUOTE(mpi_to_ofi)
static inline int mpi_to_ofi(MPI_Datatype dt, enum fi_datatype *fi_dt, MPI_Op op, enum fi_op *fi_op)
{
    *fi_dt = FI_DATATYPE_LAST;
    *fi_op = FI_ATOMIC_OP_LAST;

    if (isS_INT(dt))
        *fi_dt = FI_INT32;
    else if (isUS_INT(dt))
        *fi_dt = FI_UINT32;
    else if (isFLOAT(dt))
        *fi_dt = FI_FLOAT;
    else if (isDOUBLE(dt))
        *fi_dt = FI_DOUBLE;
    else if (isLONG_DOUBLE(dt))
        *fi_dt = FI_LONG_DOUBLE;
    else if (isS_CHAR(dt))
        *fi_dt = FI_INT8;
    else if (isUS_CHAR(dt))
        *fi_dt = FI_UINT8;
    else if (isS_SHORT(dt))
        *fi_dt = FI_INT16;
    else if (isUS_SHORT(dt))
        *fi_dt = FI_UINT16;
    else if (isS_LONG(dt))
        *fi_dt = FI_INT64;
    else if (isUS_LONG(dt))
        *fi_dt = FI_UINT64;
    else if (isS_LONG_LONG(dt))
        *fi_dt = FI_INT64;
    else if (isUS_LONG_LONG(dt))
        *fi_dt = FI_UINT64;
    else if (isSINGLE_COMPLEX(dt))
        *fi_dt = FI_FLOAT_COMPLEX;
    else if (isDOUBLE_COMPLEX(dt))
        *fi_dt = FI_DOUBLE_COMPLEX;
    else if (isLOC_TYPE(dt))
        *fi_dt = FI_DATATYPE_LAST;
    else if (isLOGICAL(dt))
        *fi_dt = FI_UINT32;
    else if (isBOOL(dt))
        *fi_dt = FI_UINT8;

    if (*fi_dt == FI_DATATYPE_LAST)
        goto fn_fail;

    *fi_op = FI_ATOMIC_OP_LAST;

    switch (op) {
        case MPI_SUM:
            *fi_op = FI_SUM;
            goto fn_exit;

        case MPI_PROD:
            *fi_op = FI_PROD;
            goto fn_exit;

        case MPI_MAX:
            *fi_op = FI_MAX;
            goto fn_exit;

        case MPI_MIN:
            *fi_op = FI_MIN;
            goto fn_exit;

        case MPI_BAND:
            *fi_op = FI_BAND;
            goto fn_exit;

        case MPI_BOR:
            *fi_op = FI_BOR;
            goto fn_exit;

        case MPI_BXOR:
            *fi_op = FI_BXOR;
            goto fn_exit;
            break;

        case MPI_LAND:
            if (isLONG_DOUBLE(dt))
                goto fn_fail;

            *fi_op = FI_LAND;
            goto fn_exit;

        case MPI_LOR:
            if (isLONG_DOUBLE(dt))
                goto fn_fail;

            *fi_op = FI_LOR;
            goto fn_exit;

        case MPI_LXOR:
            if (isLONG_DOUBLE(dt))
                goto fn_fail;

            *fi_op = FI_LXOR;
            goto fn_exit;

        case MPI_REPLACE:{
                *fi_op = FI_ATOMIC_WRITE;
                goto fn_exit;
            }

        case MPI_NO_OP:{
                *fi_op = FI_ATOMIC_READ;
                goto fn_exit;
            }

        case MPI_OP_NULL:{
                *fi_op = FI_CSWAP;
                goto fn_exit;
            }

        default:
            goto fn_fail;
    }

  fn_exit:
    return MPI_SUCCESS;
  fn_fail:
    return -1;
}

static MPI_Datatype mpi_dtypes[] = {
    MPI_CHAR, MPI_UNSIGNED_CHAR, MPI_SIGNED_CHAR, MPI_BYTE,
    MPI_WCHAR, MPI_SHORT, MPI_UNSIGNED_SHORT, MPI_INT,
    MPI_UNSIGNED, MPI_LONG, MPI_UNSIGNED_LONG, MPI_FLOAT,
    MPI_DOUBLE, MPI_LONG_DOUBLE, MPI_LONG_LONG, MPI_UNSIGNED_LONG_LONG,
    MPI_PACKED, MPI_LB, MPI_UB, MPI_2INT,

    MPI_INT8_T, MPI_INT16_T, MPI_INT32_T,
    MPI_INT64_T, MPI_UINT8_T, MPI_UINT16_T,
    MPI_UINT32_T, MPI_UINT64_T, MPI_C_BOOL,
    MPI_C_FLOAT_COMPLEX, MPI_C_DOUBLE_COMPLEX, MPI_C_LONG_DOUBLE_COMPLEX,
    /* address/offset/count types */
    MPI_AINT, MPI_OFFSET, MPI_COUNT,
    /* Fortran types */
#ifdef HAVE_FORTRAN_BINDING
    MPI_COMPLEX, MPI_DOUBLE_COMPLEX, MPI_LOGICAL, MPI_REAL,
    MPI_DOUBLE_PRECISION, MPI_INTEGER, MPI_2INTEGER,

#ifdef MPICH_DEFINE_2COMPLEX
    MPI_2COMPLEX, MPI_2DOUBLE_COMPLEX,
#endif
    MPI_2REAL, MPI_2DOUBLE_PRECISION, MPI_CHARACTER,
    MPI_REAL4, MPI_REAL8, MPI_REAL16, MPI_COMPLEX8, MPI_COMPLEX16,
    MPI_COMPLEX32, MPI_INTEGER1, MPI_INTEGER2, MPI_INTEGER4, MPI_INTEGER8,
    MPI_INTEGER16,
#endif
    MPI_FLOAT_INT, MPI_DOUBLE_INT,
    MPI_LONG_INT, MPI_SHORT_INT,
    MPI_LONG_DOUBLE_INT,
    (MPI_Datatype) - 1,
};

static MPI_Op mpi_ops[] = {
    MPI_MAX, MPI_MIN, MPI_SUM, MPI_PROD,
    MPI_LAND, MPI_BAND, MPI_LOR, MPI_BOR,
    MPI_LXOR, MPI_BXOR, MPI_MINLOC, MPI_MAXLOC,
    MPI_REPLACE, MPI_NO_OP, MPI_OP_NULL,
};

#define _TBL MPIDI_Global.win_op_table[i][j]
#define CHECK_ATOMIC(fcn,field1,field2)            \
  atomic_count = 0;                                \
  ret = fcn(MPIDI_Global.ctx[0].tx,                \
    fi_dt,                                 \
    fi_op,                                 \
            &atomic_count);                        \
  if (ret == 0 && atomic_count != 0)                \
    {                                              \
  _TBL.field1 = 1;                             \
  _TBL.field2 = atomic_count;                  \
    }

static inline void create_dt_map()
{
    int i, j;
    size_t dtsize[FI_DATATYPE_LAST];
    dtsize[FI_INT8] = sizeof(int8_t);
    dtsize[FI_UINT8] = sizeof(uint8_t);
    dtsize[FI_INT16] = sizeof(int16_t);
    dtsize[FI_UINT16] = sizeof(uint16_t);
    dtsize[FI_INT32] = sizeof(int32_t);
    dtsize[FI_UINT32] = sizeof(uint32_t);
    dtsize[FI_INT64] = sizeof(int64_t);
    dtsize[FI_UINT64] = sizeof(uint64_t);
    dtsize[FI_FLOAT] = sizeof(float);
    dtsize[FI_DOUBLE] = sizeof(double);
    dtsize[FI_FLOAT_COMPLEX] = sizeof(float complex);
    dtsize[FI_DOUBLE_COMPLEX] = sizeof(double complex);
    dtsize[FI_LONG_DOUBLE] = sizeof(long double);
    dtsize[FI_LONG_DOUBLE_COMPLEX] = sizeof(long double complex);

    /* when atomics are disabled and atomics capability are not
     * enabled call fo fi_atomic*** may crash */
    MPIR_Assert(MPIDI_OFI_ENABLE_ATOMICS);

    for (i = 0; i < MPIDI_OFI_DT_SIZES; i++)
        for (j = 0; j < MPIDI_OFI_OP_SIZES; j++) {
            enum fi_datatype fi_dt = (enum fi_datatype) -1;
            enum fi_op fi_op = (enum fi_op) -1;
            mpi_to_ofi(mpi_dtypes[i], &fi_dt, mpi_ops[j], &fi_op);
            MPIR_Assert(fi_dt != (enum fi_datatype) -1);
            MPIR_Assert(fi_op != (enum fi_op) -1);
            _TBL.dt = fi_dt;
            _TBL.op = fi_op;
            _TBL.atomic_valid = 0;
            _TBL.max_atomic_count = 0;
            _TBL.max_fetch_atomic_count = 0;
            _TBL.max_compare_atomic_count = 0;
            _TBL.mpi_acc_valid = check_mpi_acc_valid(mpi_dtypes[i], mpi_ops[j]);
            ssize_t ret;
            size_t atomic_count;

            if (fi_dt != FI_DATATYPE_LAST && fi_op != FI_ATOMIC_OP_LAST) {
                CHECK_ATOMIC(fi_atomicvalid, atomic_valid, max_atomic_count);
                CHECK_ATOMIC(fi_fetch_atomicvalid, fetch_atomic_valid, max_fetch_atomic_count);
                CHECK_ATOMIC(fi_compare_atomicvalid, compare_atomic_valid,
                             max_compare_atomic_count);
                _TBL.dtsize = dtsize[fi_dt];
            }
        }
}

static inline void add_index(MPI_Datatype datatype, int *index)
{
    MPIR_Datatype *dt_ptr;
    MPIR_Datatype_get_ptr(datatype, dt_ptr);
    MPIDI_OFI_DATATYPE(dt_ptr).index = *index;
    (*index)++;
}

void MPIDI_OFI_index_datatypes()
{
    static bool needs_init = true;
    int index = 0;

    if (!needs_init)
        return;

    add_index(MPI_CHAR, &index);
    add_index(MPI_UNSIGNED_CHAR, &index);
    add_index(MPI_SIGNED_CHAR, &index);
    add_index(MPI_BYTE, &index);
    add_index(MPI_WCHAR, &index);
    add_index(MPI_SHORT, &index);
    add_index(MPI_UNSIGNED_SHORT, &index);
    add_index(MPI_INT, &index);
    add_index(MPI_UNSIGNED, &index);
    add_index(MPI_LONG, &index);        /* count=10 */
    add_index(MPI_UNSIGNED_LONG, &index);
    add_index(MPI_FLOAT, &index);
    add_index(MPI_DOUBLE, &index);
    add_index(MPI_LONG_DOUBLE, &index);
    add_index(MPI_LONG_LONG, &index);
    add_index(MPI_UNSIGNED_LONG_LONG, &index);
    add_index(MPI_PACKED, &index);
    add_index(MPI_LB, &index);
    add_index(MPI_UB, &index);
    add_index(MPI_2INT, &index);        /* count=20 */

    /* C99 types */
    add_index(MPI_INT8_T, &index);
    add_index(MPI_INT16_T, &index);
    add_index(MPI_INT32_T, &index);
    add_index(MPI_INT64_T, &index);
    add_index(MPI_UINT8_T, &index);
    add_index(MPI_UINT16_T, &index);
    add_index(MPI_UINT32_T, &index);
    add_index(MPI_UINT64_T, &index);
    add_index(MPI_C_BOOL, &index);
    add_index(MPI_C_FLOAT_COMPLEX, &index);     /* count=30 */
    add_index(MPI_C_DOUBLE_COMPLEX, &index);
    add_index(MPI_C_LONG_DOUBLE_COMPLEX, &index);

    /* address/offset/count types */
    add_index(MPI_AINT, &index);
    add_index(MPI_OFFSET, &index);
    add_index(MPI_COUNT, &index);       /* count=35 */

    /* Fortran types (count=23) */
#ifdef HAVE_FORTRAN_BINDING
    add_index(MPI_COMPLEX, &index);
    add_index(MPI_DOUBLE_COMPLEX, &index);
    add_index(MPI_LOGICAL, &index);
    add_index(MPI_REAL, &index);
    add_index(MPI_DOUBLE_PRECISION, &index);    /* count=40 */
    add_index(MPI_INTEGER, &index);
    add_index(MPI_2INTEGER, &index);
#ifdef MPICH_DEFINE_2COMPLEX
    add_index(MPI_2COMPLEX, &index);
    add_index(MPI_2DOUBLE_COMPLEX, &index);
#endif
    add_index(MPI_2REAL, &index);
    add_index(MPI_2DOUBLE_PRECISION, &index);
    add_index(MPI_CHARACTER, &index);
    add_index(MPI_REAL4, &index);
    add_index(MPI_REAL8, &index);
    add_index(MPI_REAL16, &index);      /* count=50 */
    add_index(MPI_COMPLEX8, &index);
    add_index(MPI_COMPLEX16, &index);
    add_index(MPI_COMPLEX32, &index);
    add_index(MPI_INTEGER1, &index);
    add_index(MPI_INTEGER2, &index);
    add_index(MPI_INTEGER4, &index);
    add_index(MPI_INTEGER8, &index);

    if (MPI_INTEGER16 == MPI_DATATYPE_NULL)
        index++;
    else
        add_index(MPI_INTEGER16, &index);

#endif
    add_index(MPI_FLOAT_INT, &index);
    add_index(MPI_DOUBLE_INT, &index);  /* count=60 */
    add_index(MPI_LONG_INT, &index);
    add_index(MPI_SHORT_INT, &index);
    add_index(MPI_LONG_DOUBLE_INT, &index);

    /* check if static dt_size is correct */
    MPIR_Assert(MPIDI_OFI_DT_SIZES == index);

    /* do not generate map when atomics are not enabled */
    if (MPIDI_OFI_ENABLE_ATOMICS)
        create_dt_map();

    /* only need to do this once */
    needs_init = false;
}
