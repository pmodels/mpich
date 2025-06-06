/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "ofi_impl.h"
#include "ofi_events.h"

int MPIDI_OFI_retry_progress(int vci, int retry)
{
    /* We do not call progress on hooks form netmod level
     * because it is not reentrant safe.
     */
    int mpi_errno;
    /* call global progress sparingly. I assume the netmod progress on its own
     * will resolve most of the resource busy issue. Call global progress when
     * that is not resolving. */
    if ((retry & 0xff) == 0) {
        MPIDI_OFI_THREAD_CS_EXIT_VCI_OPTIONAL(vci);
        mpi_errno = MPID_Progress_test(NULL);
        MPIDI_OFI_THREAD_CS_ENTER_VCI_OPTIONAL(vci);
    } else {
        mpi_errno = MPIDI_OFI_progress_uninlined(vci);
    }
    return mpi_errno;
}

typedef struct MPIDI_OFI_mr_key_allocator_t {
    uint64_t chunk_size;
    uint64_t num_ints;
    uint64_t last_free_mr_key;
    uint64_t *bitmask;
} MPIDI_OFI_mr_key_allocator_t;

static MPIDI_OFI_mr_key_allocator_t mr_key_allocator;
static MPID_Thread_mutex_t mr_key_allocator_lock;

int MPIDI_OFI_mr_key_allocator_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    int err;
    MPID_Thread_mutex_create(&mr_key_allocator_lock, &err);
    MPIR_Assert(err == 0);

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
 * (auto-generated) (max_mr_key_size * 8) bit key; since the highest bit is reserved
 * for key type, a valid key has maximal (max_mr_key_size * 8 - 1) bits. For example,
 * when the max_mr_key_size is 8 bytes, a valid key has 63 bits. */
uint64_t MPIDI_OFI_mr_key_alloc(int key_type, uint64_t requested_key)
{
    uint64_t ret_key = MPIDI_OFI_INVALID_MR_KEY;

    // Shift key by number of bits - 1
    uint64_t key_mask = 1ULL << (MPIDI_OFI_global.max_mr_key_size * 8 - 1);

    switch (key_type) {
        case MPIDI_OFI_LOCAL_MR_KEY:
            {
                MPID_THREAD_CS_ENTER(VCI, mr_key_allocator_lock);
                for (int i = mr_key_allocator.last_free_mr_key; i < mr_key_allocator.num_ints; i++) {
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
                        if ((ret_key & key_mask) != 0)
                            return MPIDI_OFI_INVALID_MR_KEY;
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
                MPID_THREAD_CS_EXIT(VCI, mr_key_allocator_lock);
                break;
            }

        case MPIDI_OFI_COLL_MR_KEY:
            {
                MPIR_Assert(requested_key != MPIDI_OFI_INVALID_MR_KEY);
                ret_key = requested_key | key_mask;
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

                MPID_THREAD_CS_ENTER(VCI, mr_key_allocator_lock);
                numbits = sizeof(uint64_t) * 8;
                int_index = alloc_key / numbits;
                bitpos = alloc_key % numbits;
                mr_key_allocator.last_free_mr_key =
                    MPL_MIN(int_index, mr_key_allocator.last_free_mr_key);
                mr_key_allocator.bitmask[int_index] |= (0x1ULL << bitpos);
                MPID_THREAD_CS_EXIT(VCI, mr_key_allocator_lock);
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
    MPID_THREAD_CS_ENTER(VCI, mr_key_allocator_lock);
    MPL_free(mr_key_allocator.bitmask);
    MPID_THREAD_CS_EXIT(VCI, mr_key_allocator_lock);
}

int MPIDI_OFI_control_handler(void *am_hdr, void *data, MPI_Aint data_sz,
                              uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_send_control_t *ctrlsend = (MPIDI_OFI_send_control_t *) am_hdr;

    if (attr & MPIDIG_AM_ATTR__IS_ASYNC) {
        *req = NULL;
    }

    int local_vci = MPIDIG_AM_ATTR_DST_VCI(attr);
    MPIR_AssertDeclValue(int remote_vci, MPIDIG_AM_ATTR_SRC_VCI(attr));
    switch (ctrlsend->type) {
        case MPIDI_OFI_CTRL_HUGEACK:
            mpi_errno = MPIDI_OFI_dispatch_function(local_vci, NULL, ctrlsend->u.huge_ack.ackreq);
            break;

        case MPIDI_OFI_CTRL_HUGE:
            MPIR_Assert(local_vci == ctrlsend->u.huge.info.vci_dst);
            MPIR_Assert(remote_vci == ctrlsend->u.huge.info.vci_src);
            mpi_errno = MPIDI_OFI_recv_huge_control(local_vci,
                                                    ctrlsend->u.huge.info.comm_id,
                                                    ctrlsend->u.huge.info.origin_rank,
                                                    ctrlsend->u.huge.info.tag,
                                                    &(ctrlsend->u.huge.info));
            break;

        default:
            fprintf(stderr, "Bad control type: 0x%08x  %d\n", ctrlsend->type, ctrlsend->type);
            MPIR_Assert(0);
    }

    return mpi_errno;
}

static bool check_mpi_acc_valid(MPI_Datatype dtype, MPI_Op op)
{
    bool valid_flag = false;

    /* Check if the <datatype, op> is supported by MPICH. Note that MPICH
     * supports more combinations than that specified in standard (see definition
     * of these checking routines for extended support). */

    /* Predefined reduce operation, NO_OP, REPLACE */
    if (op != MPI_OP_NULL) {
        valid_flag = MPIR_Internal_op_dt_check(op, dtype);
    } else {
        /* Compare and swap */
        if (MPIR_Type_is_rma_atomic(dtype))
            valid_flag = true;
    }

    return valid_flag;
}

int MPIDI_OFI_datatype_to_ofi(MPI_Datatype dt, enum fi_datatype *fi_dt)
{
    *fi_dt = FI_DATATYPE_LAST;

    if (!HANDLE_IS_BUILTIN(dt)) {
        goto fn_fail;
    }

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(dt)) {
        case MPIR_INT8:
            *fi_dt = FI_INT8;
            break;
        case MPIR_INT16:
            *fi_dt = FI_INT16;
            break;
        case MPIR_INT32:
            *fi_dt = FI_INT32;
            break;
        case MPIR_INT64:
            *fi_dt = FI_INT64;
            break;
        case MPIR_UINT8:
            *fi_dt = FI_UINT8;
            break;
        case MPIR_UINT16:
            *fi_dt = FI_UINT16;
            break;
        case MPIR_UINT32:
            *fi_dt = FI_UINT32;
            break;
        case MPIR_UINT64:
            *fi_dt = FI_UINT64;
            break;
        case MPIR_FLOAT32:
            *fi_dt = FI_FLOAT;
            break;
        case MPIR_FLOAT64:
            *fi_dt = FI_DOUBLE;
            break;
        case MPIR_FLOAT128:
            *fi_dt = FI_LONG_DOUBLE;
            break;
        case MPIR_COMPLEX32:
            *fi_dt = FI_FLOAT_COMPLEX;
            break;
        case MPIR_COMPLEX64:
            *fi_dt = FI_DOUBLE_COMPLEX;
            break;
        default:
            /* no matching type */
            goto fn_fail;
    }

    return MPI_SUCCESS;
  fn_fail:
    return -1;
}

int MPIDI_OFI_op_to_ofi(MPI_Op op, enum fi_op *fi_op)
{
    *fi_op = FI_ATOMIC_OP_LAST;

    switch (op) {
        case MPI_SUM:
            *fi_op = FI_SUM;
            break;
        case MPI_PROD:
            *fi_op = FI_PROD;
            break;
        case MPI_MAX:
            *fi_op = FI_MAX;
            break;
        case MPI_MIN:
            *fi_op = FI_MIN;
            break;
        case MPI_BAND:
            *fi_op = FI_BAND;
            break;
        case MPI_BOR:
            *fi_op = FI_BOR;
            break;
        case MPI_BXOR:
            *fi_op = FI_BXOR;
            break;
        case MPI_LAND:
            *fi_op = FI_LAND;
            break;
        case MPI_LOR:
            *fi_op = FI_LOR;
            break;
        case MPI_LXOR:
            *fi_op = FI_LXOR;
            break;
        case MPI_REPLACE:
            *fi_op = FI_ATOMIC_WRITE;
            break;
        case MPI_NO_OP:
            *fi_op = FI_ATOMIC_READ;
            break;
        case MPI_OP_NULL:
            *fi_op = FI_CSWAP;
            break;
        default:
            /* no matching op */
            goto fn_fail;
    }

    return MPI_SUCCESS;
  fn_fail:
    return -1;
}

/* we also need inverse conversion for check_mpi_acc_valid(dt, op) */

static MPI_Datatype datatype_from_ofi(enum fi_datatype fi_dt)
{
    /* order from libfabric man page */
    switch (fi_dt) {
        case FI_INT8:
            return MPIR_INT8;
        case FI_UINT8:
            return MPIR_UINT8;
        case FI_INT16:
            return MPIR_INT16;
        case FI_UINT16:
            return MPIR_UINT16;
        case FI_INT32:
            return MPIR_INT32;
        case FI_UINT32:
            return MPIR_UINT32;
        case FI_INT64:
            return MPIR_INT64;
        case FI_UINT64:
            return MPIR_UINT64;
#ifdef FI_INT128
        case FI_INT128:
            return MPIR_INT128;
        case FI_UINT128:
            return MPIR_UINT128;
#endif
        case FI_FLOAT:
            return MPIR_FLOAT32;
        case FI_DOUBLE:
            return MPIR_FLOAT64;
        case FI_FLOAT_COMPLEX:
            return MPIR_COMPLEX32;
        case FI_DOUBLE_COMPLEX:
            return MPIR_COMPLEX64;
        case FI_LONG_DOUBLE:
            return MPIR_LONG_DOUBLE_INTERNAL;
        case FI_LONG_DOUBLE_COMPLEX:
            return MPIR_C_LONG_DOUBLE_COMPLEX_INTERNAL;
        default:
            return MPI_DATATYPE_NULL;
    }
}

static MPI_Op op_from_ofi(enum fi_op fi_op)
{
    /* order from libfabric man page */
    switch (fi_op) {
        case FI_MIN:
            return MPI_MIN;
        case FI_MAX:
            return MPI_MAX;
        case FI_SUM:
            return MPI_SUM;
        case FI_PROD:
            return MPI_PROD;
        case FI_LOR:
            return MPI_LOR;
        case FI_LAND:
            return MPI_LAND;
        case FI_BOR:
            return MPI_BOR;
        case FI_BAND:
            return MPI_BAND;
        case FI_LXOR:
            return MPI_LXOR;
        case FI_BXOR:
            return MPI_BXOR;
        case FI_ATOMIC_WRITE:
            return MPI_REPLACE;
        case FI_ATOMIC_READ:
            return MPI_NO_OP;
        case FI_CSWAP:
            return MPI_OP_NULL;
        default:
            return MPI_OP_NULL;
    }
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
    MPIR_Assert(MPIDI_OFI_DT_MAX > FI_DATATYPE_LAST);
    MPIR_Assert(MPIDI_OFI_OP_MAX > FI_ATOMIC_OP_LAST);

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

    for (i = 0; i < FI_DATATYPE_LAST; i++) {
        enum fi_datatype fi_dt = (enum fi_datatype) i;
        MPI_Datatype dt = datatype_from_ofi(fi_dt);

        for (j = 0; j < FI_ATOMIC_OP_LAST; j++) {
            enum fi_op fi_op = (enum fi_op) j;
            MPI_Op op = op_from_ofi(fi_op);

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
