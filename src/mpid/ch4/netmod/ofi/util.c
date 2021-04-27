/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "ofi_impl.h"
#include "ofi_events.h"

#define MPIDI_OFI_MR_KEY_PREFIX_SHIFT 63

int MPIDI_OFI_handle_cq_error_util(int vni_idx, ssize_t ret)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ERROR_UTIL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ERROR_UTIL);

    mpi_errno = MPIDI_OFI_handle_cq_error(vni_idx, ret);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ERROR_UTIL);
    return mpi_errno;
}

int MPIDI_OFI_retry_progress(void)
{
    /* We do not call progress on hooks form netmod level
     * because it is not reentrant safe.
     */
    return MPID_Progress_test(NULL);
}

typedef struct MPIDI_OFI_mr_key_allocator_t {
    uint64_t chunk_size;
    uint64_t num_ints;
    uint64_t last_free_mr_key;
    uint64_t *bitmask;
} MPIDI_OFI_mr_key_allocator_t;

static MPIDI_OFI_mr_key_allocator_t mr_key_allocator;

int MPIDI_OFI_mr_key_allocator_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    mr_key_allocator.chunk_size = 128;
    mr_key_allocator.num_ints = mr_key_allocator.chunk_size;
    mr_key_allocator.last_free_mr_key = 0;
    mr_key_allocator.bitmask = MPL_malloc(sizeof(uint64_t) * mr_key_allocator.num_ints,
                                          MPL_MEM_RMA);
    MPIR_ERR_CHKANDSTMT(mr_key_allocator.bitmask == NULL, mpi_errno,
                        MPI_ERR_NO_MEM, goto fn_fail, "**nomem");
    memset(mr_key_allocator.bitmask, 0xFF, sizeof(uint64_t) * mr_key_allocator.num_ints);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#define MPIDI_OFI_INDEX_CALC(val,nval,shift,mask) \
    if ((val & mask) == 0) {                              \
        val >>= shift##ULL;                               \
        nval += shift;                                    \
    }

/* when key_type is MPIDI_OFI_LOCAL_MR_KEY, the input requested_key is ignored
 * and can be passed as MPIDI_OFI_INVALID_MR_KEY because mr key allocator will
 * decide which key to use. When key_type is MPIDI_OFI_COLL_MR_KEY, user should
 * pass a collectively unique key as requested_key and mr key allocator will mark
 * coll bit of the key and return to user.
 * we use highest bit of key to distinguish coll (user-specific key) and local
 * (auto-generated) 64-bits key; since the highest bit is reserved for key type,
 * a valid key has maximal 63 bits. */
uint64_t MPIDI_OFI_mr_key_alloc(int key_type, uint64_t requested_key)
{
    uint64_t ret_key = MPIDI_OFI_INVALID_MR_KEY;

    switch (key_type) {
        case MPIDI_OFI_LOCAL_MR_KEY:
            {
                uint64_t i;
                for (i = mr_key_allocator.last_free_mr_key; i < mr_key_allocator.num_ints; i++) {
                    if (mr_key_allocator.bitmask[i]) {
                        register uint64_t val, nval;
                        val = mr_key_allocator.bitmask[i];
                        nval = 2;
                        MPIDI_OFI_INDEX_CALC(val, nval, 32, 0xFFFFFFFFULL);
                        MPIDI_OFI_INDEX_CALC(val, nval, 16, 0xFFFFULL);
                        MPIDI_OFI_INDEX_CALC(val, nval, 8, 0xFFULL);
                        MPIDI_OFI_INDEX_CALC(val, nval, 4, 0xFULL);
                        MPIDI_OFI_INDEX_CALC(val, nval, 2, 0x3ULL);
                        nval -= val & 0x1ULL;
                        mr_key_allocator.bitmask[i] &= ~(0x1ULL << (nval - 1));
                        mr_key_allocator.last_free_mr_key = i;
                        ret_key = i * sizeof(uint64_t) * 8 + (nval - 1);
                        /* assert local key does not exceed its range */
                        MPIR_Assert((ret_key & (1ULL << MPIDI_OFI_MR_KEY_PREFIX_SHIFT)) == 0);
                        break;
                    }
                    if (i == mr_key_allocator.num_ints - 1) {
                        mr_key_allocator.num_ints += mr_key_allocator.chunk_size;
                        mr_key_allocator.bitmask = MPL_realloc(mr_key_allocator.bitmask,
                                                               sizeof(uint64_t) *
                                                               mr_key_allocator.num_ints,
                                                               MPL_MEM_RMA);
                        MPIR_Assert(mr_key_allocator.bitmask);
                        memset(&mr_key_allocator.bitmask[i + 1], 0xFF,
                               sizeof(uint64_t) * mr_key_allocator.chunk_size);
                    }
                }
                break;
            }

        case MPIDI_OFI_COLL_MR_KEY:
            {
                MPIR_Assert(requested_key != MPIDI_OFI_INVALID_MR_KEY);
                ret_key = requested_key | (1ULL << MPIDI_OFI_MR_KEY_PREFIX_SHIFT);
                break;
            }

        default:
            {
                MPIR_Assert(0);
            }
    }

    return ret_key;
}

void MPIDI_OFI_mr_key_free(int key_type, uint64_t alloc_key)
{

    switch (key_type) {
        case MPIDI_OFI_LOCAL_MR_KEY:
            {
                uint64_t int_index, bitpos, numbits;

                numbits = sizeof(uint64_t) * 8;
                int_index = alloc_key / numbits;
                bitpos = alloc_key % numbits;
                mr_key_allocator.last_free_mr_key =
                    MPL_MIN(int_index, mr_key_allocator.last_free_mr_key);
                mr_key_allocator.bitmask[int_index] |= (0x1ULL << bitpos);
                break;
            }

        case MPIDI_OFI_COLL_MR_KEY:
            {
                MPIR_Assert(alloc_key != MPIDI_OFI_INVALID_MR_KEY);
                break;
            }

        default:
            {
                MPIR_Assert(0);
            }
    }
}

void MPIDI_OFI_mr_key_allocator_destroy(void)
{
    MPL_free(mr_key_allocator.bitmask);
}

/* Translate the control message to get a huge message into a request to
 * actually perform the data transfer. */
static int MPIDI_OFI_get_huge(MPIDI_OFI_send_control_t * info)
{
    MPIDI_OFI_huge_recv_t *recv_elem = NULL;
    MPIR_Comm *comm_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_GET_HUGE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_GET_HUGE);

    /* Look up the communicator */
    comm_ptr = MPIDIG_context_id_to_comm(info->comm_id);

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

                recv_elem = (MPIDI_OFI_huge_recv_t *)
                    MPIDIU_map_lookup(MPIDI_OFI_COMM(comm_ptr).huge_recv_counters,
                                      list_ptr->rreq->handle);

                /* If this is a "peek" element for an MPI_Probe, it shouldn't be matched. Grab the
                 * important information and remove the element from the list. */
                if (recv_elem->peek) {
                    MPIR_STATUS_SET_COUNT(recv_elem->localreq->status, info->msgsize);
                    MPIDI_OFI_REQUEST(recv_elem->localreq, util_id) = MPIDI_OFI_PEEK_FOUND;
                    MPIDIU_map_erase(MPIDI_OFI_COMM(recv_elem->comm_ptr).huge_recv_counters,
                                     recv_elem->localreq->handle);
                    MPL_free(recv_elem);
                    recv_elem = NULL;
                }

                MPL_free(list_ptr);
                break;
            }
        }
    }

    if (recv_elem == NULL) {    /* Put the struct describing the transfer on an
                                 * unexpected list to be retrieved later */
        MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                        (MPL_DBG_FDEST, "CREATING UNEXPECTED HUGE RECV: (%d, %d, %d)",
                         info->comm_id, info->origin_rank, info->tag));

        /* If this is unexpected, create a new tracker and put it in the unexpected list. */
        recv_elem = (MPIDI_OFI_huge_recv_t *) MPL_calloc(sizeof(*recv_elem), 1, MPL_MEM_COMM);
        if (!recv_elem)
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");

        LL_APPEND(MPIDI_unexp_huge_recv_head, MPIDI_unexp_huge_recv_tail, recv_elem);
    }

    recv_elem->event_id = MPIDI_OFI_EVENT_GET_HUGE;
    recv_elem->cur_offset = MPIDI_OFI_global.max_msg_size;
    recv_elem->remote_info = *info;
    recv_elem->comm_ptr = comm_ptr;
    recv_elem->next = NULL;
    MPIDI_OFI_get_huge_event(NULL, (MPIR_Request *) recv_elem);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_GET_HUGE);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_control_handler(int handler_id, void *am_hdr, void *data, MPI_Aint data_sz,
                              int is_local, int is_async, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_send_control_t *ctrlsend = (MPIDI_OFI_send_control_t *) am_hdr;

    if (is_async)
        *req = NULL;

    switch (ctrlsend->type) {
        case MPIDI_OFI_CTRL_HUGEACK:{
                mpi_errno = MPIDI_OFI_dispatch_function(NULL, ctrlsend->ackreq);
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

static bool check_mpi_acc_valid(MPI_Datatype dtype, MPI_Op op)
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

static int mpi_to_ofi(MPI_Datatype dt, enum fi_datatype *fi_dt, MPI_Op op, enum fi_op *fi_op)
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

#define _TBL MPIDI_OFI_global.win_op_table[i][j]
#define CHECK_ATOMIC(fcn,field1,field2)            \
  atomic_count = 0;                                \
  ret = fcn(ep, fi_dt, fi_op, &atomic_count);      \
  if (ret == 0 && atomic_count != 0) {             \
    _TBL.field1 = 1;                               \
    _TBL.field2 = atomic_count;                    \
  }

static void create_dt_map(struct fid_ep *ep)
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
     * enabled call of fi_atomic*** may crash */
    MPIR_Assert(MPIDI_OFI_ENABLE_ATOMICS);

    memset(MPIDI_OFI_global.win_op_table, 0, sizeof(MPIDI_OFI_global.win_op_table));

    for (i = 0; i < MPIR_DATATYPE_N_PREDEFINED; i++) {
        MPI_Datatype dt = MPIR_Datatype_predefined_get_type(i);

        /* MPICH sets predefined datatype handles to MPI_DATATYPE_NULL if they are not
         * supported on the target platform. Skip it. */
        if (dt == MPI_DATATYPE_NULL)
            continue;

        for (j = 0; j < MPIDIG_ACCU_NUM_OP; j++) {
            MPI_Op op = MPIDIU_win_acc_get_op(j);
            enum fi_datatype fi_dt = (enum fi_datatype) -1;
            enum fi_op fi_op = (enum fi_op) -1;

            mpi_to_ofi(dt, &fi_dt, op, &fi_op);
            MPIR_Assert(fi_dt != (enum fi_datatype) -1);
            MPIR_Assert(fi_op != (enum fi_op) -1);
            _TBL.dt = fi_dt;
            _TBL.op = fi_op;
            _TBL.atomic_valid = 0;
            _TBL.max_atomic_count = 0;
            _TBL.max_fetch_atomic_count = 0;
            _TBL.max_compare_atomic_count = 0;
            _TBL.mpi_acc_valid = check_mpi_acc_valid(dt, op);
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
}

void MPIDI_OFI_index_datatypes(struct fid_ep *ep)
{
    /* do not generate map when atomics are not enabled */
    if (MPIDI_OFI_ENABLE_ATOMICS) {
        create_dt_map(ep);
    }
}
