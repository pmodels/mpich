/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MPID_UCC_DTYPES_H_
#define _MPID_UCC_DTYPES_H_

#define MPIDI_COMMON_UCC_DTYPE_NULL ((ucc_datatype_t)-1)
#define MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED ((ucc_datatype_t)-2)

#define MPIDI_COMMON_UCC_DTYPE_MAP_SIGNED(_ctype)   \
    do {                                            \
        switch (sizeof(signed _ctype)) {            \
        case 1:                                     \
            return UCC_DT_INT8;                     \
        case 2:                                     \
            return UCC_DT_INT16;                    \
        case 4:                                     \
            return UCC_DT_INT32;                    \
        case 8:                                     \
            return UCC_DT_INT64;                    \
        case 16:                                    \
            return UCC_DT_INT128;                   \
        }                                           \
    } while (0)
#define MPIDI_COMMON_UCC_DTYPE_MAP_UNSIGNED(_ctype) \
    do {                                            \
        switch (sizeof(unsigned _ctype)) {          \
        case 1:                                     \
            return UCC_DT_UINT8;                    \
        case 2:                                     \
            return UCC_DT_UINT16;                   \
        case 4:                                     \
            return UCC_DT_UINT32;                   \
        case 8:                                     \
            return UCC_DT_UINT64;                   \
        case 16:                                    \
            return UCC_DT_UINT128;                  \
        }                                           \
    } while (0)
#define MPIDI_COMMON_UCC_DTYPE_MAP_FLOAT(_ctype)    \
    do {                                            \
        switch (sizeof(_ctype)) {                   \
        case 4:                                     \
            return UCC_DT_FLOAT32;                  \
        case 8:                                     \
            return UCC_DT_FLOAT64;                  \
        case 16:                                    \
            return UCC_DT_FLOAT128;                 \
        }                                           \
    } while (0)

static inline ucc_datatype_t mpidi_mpi_dtype_to_ucc_dtype(MPI_Datatype datatype)
{
    if (HANDLE_IS_BUILTIN(datatype) && ((datatype) & 0xff)) {
        switch (MPIR_Internal_types[(datatype) & 0xff].dtype) {
            case MPI_CHAR:
            case MPI_INT8_T:
            case MPI_SIGNED_CHAR:
                return UCC_DT_INT8;
            case MPI_BYTE:
            case MPI_PACKED:
            case MPI_UINT8_T:
            case MPI_UNSIGNED_CHAR:
                return UCC_DT_UINT8;
            case MPI_INT16_T:
                return UCC_DT_INT16;
            case MPI_UINT16_T:
                return UCC_DT_UINT16;
            case MPI_INT32_T:
                return UCC_DT_INT32;
            case MPI_UINT32_T:
                return UCC_DT_UINT32;
            case MPI_INT64_T:
                return UCC_DT_INT64;
            case MPI_UINT64_T:
                return UCC_DT_UINT64;
            case MPI_SHORT:
                MPIDI_COMMON_UCC_DTYPE_MAP_SIGNED(short int);
                break;
            case MPI_INT:
                MPIDI_COMMON_UCC_DTYPE_MAP_SIGNED(int);
                break;
            case MPI_LONG:
                MPIDI_COMMON_UCC_DTYPE_MAP_SIGNED(long int);
                break;
            case MPI_LONG_LONG:
                MPIDI_COMMON_UCC_DTYPE_MAP_SIGNED(long long int);
                break;
            case MPI_UNSIGNED_SHORT:
                MPIDI_COMMON_UCC_DTYPE_MAP_UNSIGNED(short int);
                break;
            case MPI_UNSIGNED:
                MPIDI_COMMON_UCC_DTYPE_MAP_UNSIGNED(int);
                break;
            case MPI_UNSIGNED_LONG:
                MPIDI_COMMON_UCC_DTYPE_MAP_UNSIGNED(long);
                break;
            case MPI_UNSIGNED_LONG_LONG:
                MPIDI_COMMON_UCC_DTYPE_MAP_UNSIGNED(long long);
                break;
            case MPI_FLOAT:
                MPIDI_COMMON_UCC_DTYPE_MAP_FLOAT(float);
                break;
            case MPI_DOUBLE:
                MPIDI_COMMON_UCC_DTYPE_MAP_FLOAT(double);
                break;
            case MPI_LONG_DOUBLE:
                MPIDI_COMMON_UCC_DTYPE_MAP_FLOAT(long double);
                break;
        }
    }
    if ((datatype == MPIR_BYTE_INTERNAL) || (datatype == MPIR_PACKED_INTERNAL)) {
        /* This is most probably a call from an internal collective where
         * the data has already been packed. Treat it like MPI_BYTE. */
        return UCC_DT_UINT8;
    }
    return MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED;
}

static inline const char *mpidi_ucc_dtype_to_str(ucc_datatype_t datatype)
{
#ifndef NDEBUG
    switch (datatype) {
        case UCC_DT_INT8:
            return "UCC_DT_INT8";
        case UCC_DT_INT16:
            return "UCC_DT_INT16";
        case UCC_DT_INT32:
            return "UCC_DT_INT32";
        case UCC_DT_INT64:
            return "UCC_DT_INT64";
        case UCC_DT_INT128:
            return "UCC_DT_INT128";
        case UCC_DT_UINT8:
            return "UCC_DT_UINT8";
        case UCC_DT_UINT16:
            return "UCC_DT_UINT16";
        case UCC_DT_UINT32:
            return "UCC_DT_UINT32";
        case UCC_DT_UINT64:
            return "UCC_DT_UINT64";
        case UCC_DT_UINT128:
            return "UCC_DT_UINT128";
        case UCC_DT_FLOAT32:
            return "UCC_DT_FLOAT32";
        case UCC_DT_FLOAT64:
            return "UCC_DT_FLOAT64";
        case UCC_DT_FLOAT128:
            return "UCC_DT_FLOAT128";
        default:
            return "unknown";
    }
#else
    static char buffer[32];
    snprintf(buffer, sizeof(buffer), "%" PRId64, datatype);
    return buffer;
#endif
}


#define MPIDI_COMMON_UCC_REDUCTION_OP_NULL ((ucc_reduction_op_t)-1)
#define MPIDI_COMMON_UCC_REDUCTION_OP_UNSUPPORTED ((ucc_reduction_op_t)-2)

static inline ucc_reduction_op_t mpidi_mpi_op_to_ucc_reduction_op(MPI_Op operation)
{
    switch (operation) {
        case MPI_MAX:
            return UCC_OP_MAX;
        case MPI_MIN:
            return UCC_OP_MIN;
        case MPI_SUM:
            return UCC_OP_SUM;
        case MPI_PROD:
            return UCC_OP_PROD;
        case MPI_LAND:
            return UCC_OP_LAND;
        case MPI_BAND:
            return UCC_OP_BAND;
        case MPI_LOR:
            return UCC_OP_LOR;
        case MPI_BOR:
            return UCC_OP_BOR;
        case MPI_LXOR:
            return UCC_OP_LXOR;
        case MPI_BXOR:
            return UCC_OP_BXOR;
        default:
            return MPIDI_COMMON_UCC_REDUCTION_OP_UNSUPPORTED;
    }
}

static inline const char *mpidi_ucc_reduction_op_to_str(ucc_reduction_op_t operation)
{
#ifndef NDEBUG
    switch (operation) {
        case UCC_OP_MAX:
            return "UCC_OP_MAX";
        case UCC_OP_MIN:
            return "UCC_OP_MIN";
        case UCC_OP_SUM:
            return "UCC_OP_SUM";
        case UCC_OP_PROD:
            return "UCC_OP_PROD";
        case UCC_OP_LAND:
            return "UCC_OP_LAND";
        case UCC_OP_BAND:
            return "UCC_OP_BAND";
        case UCC_OP_LOR:
            return "UCC_OP_LOR";
        case UCC_OP_BOR:
            return "UCC_OP_BOR";
        case UCC_OP_LXOR:
            return "UCC_OP_LXOR";
        case UCC_OP_BXOR:
            return "UCC_OP_BXOR";
        default:
            return "unknown";
    }
#else
    static char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", operation);
    return buffer;
#endif
}

#define MPIDI_COMMON_UCC_VERBOSE_INPLACE_UNSUPPORTED(_collop)           \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_DTYPE,      \
                             "MPI_INPLACE is not supported (" #_collop  \
                             ")");

#define MPIDI_COMMON_UCC_VERBOSE_REDUCTION_OP_UNSUPPORTED(_collop)      \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_DTYPE,      \
                             "MPI reduction operation is not supported" \
                             "(" #_collop ")");

#define MPIDI_COMMON_UCC_VERBOSE_DTYPE_UNSUPPORTED(_collop)             \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_DTYPE,      \
                             "MPI datatype is not supported (" #_collop \
                             ")");

#define MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_TRY(_collop, ...)        \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_DTYPE,      \
                             "Try to pack MPI datatype (" #_collop      \
                             ") " __VA_ARGS__);

#define MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_TRY_S(_collop)           \
    MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_TRY(_collop, "for sending")

#define MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_TRY_R(_collop)           \
    MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_TRY(_collop, "for receivig")

#define MPIDI_COMMON_UCC_VERBOSE_DTYPE_PACKING_RES(_collop, _ucc_dt)    \
    MPIDI_COMMON_UCC_VERBOSE(MPIDI_COMMON_UCC_VERBOSE_LEVEL_DTYPE,      \
                             "Packing MPI datatype (" #_collop          \
                             ") %s", _ucc_dt !=                         \
                             MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED ?       \
                             "success" : "failed");

static inline ucc_datatype_t mpidi_ucc_dtype_packing_send(const void *sbuf, MPI_Aint scount,
                                                          int num_procs, MPI_Datatype mpi_dtype,
                                                          MPIDI_common_ucc_req_t * req)
{
    ucc_datatype_t ucc_dtype;
    int contig;
    MPIR_Datatype *dtype_ptr;
    MPI_Datatype basic_dtype = MPI_DATATYPE_NULL;
    MPI_Aint true_lb;
    size_t data_size;

    if (MPIDI_common_ucc_priv.dtype_packing_disabled)
        goto fn_fail;

    MPIDI_Datatype_get_info(scount, mpi_dtype, contig, data_size, dtype_ptr, true_lb);
    MPIR_Datatype_get_basic_type(mpi_dtype, basic_dtype);
    req->basic_size = MPIR_Datatype_get_basic_size(basic_dtype);

    ucc_dtype = mpidi_mpi_dtype_to_ucc_dtype(basic_dtype);
    if (data_size && (ucc_dtype == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED))
        goto fn_exit;

    req->scounts_tmp = MPL_malloc(sizeof(MPI_Aint), MPL_MEM_COLL);
    MPIR_Assert(req->scounts_tmp);

    if (!data_size) {
        /* This is a zero-sized message. Treat it as if it were zero elements of MPI_BYTE */
        req->scounts_tmp[0] = 0;
        ucc_dtype = mpidi_mpi_dtype_to_ucc_dtype(MPI_BYTE);
        goto fn_exit;
    }

    if (contig) {
        req->sbuf_tmp = (char *) sbuf + true_lb;
        req->scounts_tmp[0] = data_size / req->basic_size;
        MPIR_Assert((data_size % req->basic_size) == 0);
    } else {
        MPI_Aint actual_packed_bytes;
        MPI_Aint size_of_pack_buffer;
        MPIR_Pack_size(scount * num_procs, mpi_dtype, &size_of_pack_buffer);
        req->sbuf_tmp = req->sbuf_free = MPL_malloc(size_of_pack_buffer, MPL_MEM_BUFFER);
        MPIR_Assert(req->sbuf_tmp);
        int rc = MPIR_Typerep_pack(sbuf, scount * num_procs, mpi_dtype, 0, req->sbuf_tmp,
                                   size_of_pack_buffer, &actual_packed_bytes,
                                   MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(rc == MPI_SUCCESS);
        MPIR_Assert(actual_packed_bytes == size_of_pack_buffer);
        req->scounts_tmp[0] = actual_packed_bytes / (num_procs * req->basic_size);
        MPIR_Assert((actual_packed_bytes % (num_procs * req->basic_size)) == 0);
    }

  fn_exit:
    return ucc_dtype;
  fn_fail:
    ucc_dtype = MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED;
    goto fn_exit;
}

static inline ucc_datatype_t mpidi_ucc_dtype_packing_sendv(const void *sbuf,
                                                           const MPI_Aint * scounts,
                                                           const MPI_Aint * sdispls,
                                                           int num_procs,
                                                           MPI_Datatype mpi_dtype,
                                                           MPIDI_common_ucc_req_t * req)
{
    ucc_datatype_t ucc_dtype;
    MPI_Aint total_len = 0;
    const void *buf_displ = NULL;
    MPI_Aint extent = 0;
    MPI_Datatype basic_dtype = MPI_DATATYPE_NULL;
    MPI_Aint actual_packed_bytes;
    MPI_Aint size_of_pack_buffer;
    MPI_Aint dtype_size;

    if (MPIDI_common_ucc_priv.dtype_packing_disabled)
        goto fn_fail;

    MPIR_Datatype_get_size_macro(mpi_dtype, dtype_size);
    MPIR_Datatype_get_basic_type(mpi_dtype, basic_dtype);
    req->basic_size = MPIR_Datatype_get_basic_size(basic_dtype);

    ucc_dtype = mpidi_mpi_dtype_to_ucc_dtype(basic_dtype);
    if (dtype_size && (ucc_dtype == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED))
        goto fn_exit;

    req->scounts_tmp = MPL_malloc(num_procs * sizeof(MPI_Aint), MPL_MEM_COLL);
    MPIR_Assert(req->scounts_tmp);
    req->sdispls_tmp = MPL_malloc(num_procs * sizeof(MPI_Aint), MPL_MEM_COLL);
    MPIR_Assert(req->sdispls_tmp);

    if (!dtype_size) {
        /* This is a zero-sized datatype. Treat it as if it were zero elements of MPI_BYTE */
        for (int i = 0; i < num_procs; i++) {
            req->scounts_tmp[i] = 0;
            req->sdispls_tmp[i] = 0;
        }
        ucc_dtype = mpidi_mpi_dtype_to_ucc_dtype(MPI_BYTE);
        goto fn_exit;
    }

    MPIR_Datatype_get_extent_macro(mpi_dtype, extent);
    for (int i = 0; i < num_procs; i++) {
        buf_displ = (char *) sbuf + sdispls[i] * extent;
        MPIR_Pack_size(scounts[i], mpi_dtype, &size_of_pack_buffer);
        req->scounts_tmp[i] = size_of_pack_buffer / req->basic_size;
        MPIR_Assert((size_of_pack_buffer % req->basic_size) == 0);
        total_len += size_of_pack_buffer;
    }

    req->sbuf_tmp = req->sbuf_free = MPL_malloc(total_len, MPL_MEM_BUFFER);
    MPIR_Assert(req->sbuf_tmp);

    char *ptr = req->sbuf_tmp;
    for (int i = 0; i < num_procs; i++) {
        buf_displ = (char *) sbuf + sdispls[i] * extent;
        MPIR_Pack_size(scounts[i], mpi_dtype, &size_of_pack_buffer);
        int rc = MPIR_Typerep_pack(buf_displ, scounts[i], mpi_dtype, 0, ptr,
                                   size_of_pack_buffer, &actual_packed_bytes,
                                   MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(rc == MPI_SUCCESS);
        MPIR_Assert(actual_packed_bytes == size_of_pack_buffer);
        ptr += actual_packed_bytes;
        req->sdispls_tmp[i] = i ? (req->sdispls_tmp[i - 1] + req->scounts_tmp[i - 1]) : 0;
    }

  fn_exit:
    return ucc_dtype;
  fn_fail:
    ucc_dtype = MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED;
    goto fn_exit;
}

static inline ucc_datatype_t mpidi_ucc_dtype_packing_recv_prep(const void *rbuf, MPI_Aint rcount,
                                                               MPI_Datatype mpi_dtype,
                                                               int num_procs,
                                                               MPIDI_common_ucc_req_t * req)
{
    ucc_datatype_t ucc_dtype;
    int contig;
    MPIR_Datatype *dtype_ptr;
    MPI_Datatype basic_dtype = MPI_DATATYPE_NULL;
    MPI_Aint true_lb;
    size_t data_size;

    if (MPIDI_common_ucc_priv.dtype_packing_disabled)
        goto fn_fail;

    MPIDI_Datatype_get_info(rcount, mpi_dtype, contig, data_size, dtype_ptr, true_lb);
    MPIR_Datatype_get_basic_type(mpi_dtype, basic_dtype);
    req->basic_size = MPIR_Datatype_get_basic_size(basic_dtype);

    ucc_dtype = mpidi_mpi_dtype_to_ucc_dtype(basic_dtype);
    if (data_size && (ucc_dtype == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED))
        goto fn_exit;

    req->rcounts_tmp = MPL_malloc(sizeof(MPI_Aint), MPL_MEM_COLL);
    MPIR_Assert(req->rcounts_tmp);

    if (!data_size) {
        /* This is a zero-sized message. Treat it as if it were zero elements of MPI_BYTE */
        req->rcounts_tmp[0] = 0;
        ucc_dtype = mpidi_mpi_dtype_to_ucc_dtype(MPI_BYTE);
        goto fn_exit;
    }

    if (contig) {
        req->rbuf_tmp = (char *) rbuf + true_lb;
        req->rcounts_tmp[0] = data_size / req->basic_size;
        MPIR_Assert((data_size % req->basic_size) == 0);
    } else {
        MPI_Aint size_of_pack_buffer;
        MPIR_Pack_size(rcount, mpi_dtype, &size_of_pack_buffer);
        req->rbuf_tmp = req->rbuf_free =
            MPL_malloc(size_of_pack_buffer * num_procs, MPL_MEM_BUFFER);
        req->rcounts_tmp[0] = size_of_pack_buffer / req->basic_size;
        MPIR_Assert((size_of_pack_buffer % req->basic_size) == 0);
    }

  fn_exit:
    return ucc_dtype;
  fn_fail:
    ucc_dtype = MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED;
    goto fn_exit;
}

static inline void mpidi_ucc_dtype_packing_recv_done(void *rbuf, MPI_Aint rcount,
                                                     MPI_Datatype mpi_dtype, int num_procs,
                                                     MPIDI_common_ucc_req_t * req)
{
    if (req->rbuf_tmp && (req->rbuf_tmp == req->rbuf_free)) {
        /* Here is where we should end up in response to `mpidi_ucc_dtype_packing_recv_prep()` if
         * the data has actually been packed (i.e., not if only the buffer start has been shifted).
         */
        MPIR_Assert(!req->rdispls_tmp);
        MPI_Aint actual_unpacked_bytes;
        MPI_Aint size_of_pack_buffer = req->rcounts_tmp[0] * req->basic_size * num_procs;
        int rc = MPIR_Typerep_unpack(req->rbuf_tmp, size_of_pack_buffer, rbuf, rcount * num_procs,
                                     mpi_dtype, 0, &actual_unpacked_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(rc == MPI_SUCCESS);
        MPIR_Assert(actual_unpacked_bytes == size_of_pack_buffer);
    }
}

static inline ucc_datatype_t mpidi_ucc_dtype_packing_recv_prepv(const void *rbuf,
                                                                const MPI_Aint * rcounts,
                                                                const MPI_Aint * rdispls,
                                                                MPI_Datatype mpi_dtype,
                                                                int num_procs,
                                                                MPIDI_common_ucc_req_t * req)
{
    ucc_datatype_t ucc_dtype;
    MPI_Aint *len_vec = NULL;
    MPI_Aint total_len = 0;
    MPI_Datatype basic_dtype = MPI_DATATYPE_NULL;
    MPI_Aint size_of_pack_buffer;
    MPI_Aint dtype_size;

    if (MPIDI_common_ucc_priv.dtype_packing_disabled)
        goto fn_fail;

    MPIR_Datatype_get_size_macro(mpi_dtype, dtype_size);
    MPIR_Datatype_get_basic_type(mpi_dtype, basic_dtype);
    req->basic_size = MPIR_Datatype_get_basic_size(basic_dtype);

    ucc_dtype = mpidi_mpi_dtype_to_ucc_dtype(basic_dtype);
    if (dtype_size && (ucc_dtype == MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED))
        goto fn_exit;

    len_vec = MPL_malloc(num_procs * sizeof(MPI_Aint), MPL_MEM_COLL);
    MPIR_Assert(len_vec);
    req->rcounts_tmp = MPL_malloc(num_procs * sizeof(MPI_Aint), MPL_MEM_COLL);
    MPIR_Assert(req->rcounts_tmp);
    req->rdispls_tmp = MPL_malloc(num_procs * sizeof(MPI_Aint), MPL_MEM_COLL);
    MPIR_Assert(req->rdispls_tmp);

    if (!dtype_size) {
        /* This is a zero-sized datatype. Treat it as if it were zero elements of MPI_BYTE */
        for (int i = 0; i < num_procs; i++) {
            req->rcounts_tmp[i] = 0;
            req->rdispls_tmp[i] = 0;
        }
        ucc_dtype = mpidi_mpi_dtype_to_ucc_dtype(MPI_BYTE);
        goto fn_exit;
    }

    for (int i = 0; i < num_procs; i++) {
        MPIR_Pack_size(rcounts[i], mpi_dtype, &size_of_pack_buffer);
        len_vec[i] = size_of_pack_buffer;
        total_len += size_of_pack_buffer;
    }

    req->rbuf_tmp = req->rbuf_free = MPL_malloc(total_len, MPL_MEM_BUFFER);
    MPIR_Assert(req->rbuf_tmp);

    for (int i = 0; i < num_procs; i++) {
        req->rcounts_tmp[i] = len_vec[i] / req->basic_size;
        MPIR_Assert((len_vec[i] % req->basic_size) == 0);
        req->rdispls_tmp[i] = i ? (req->rdispls_tmp[i - 1] + len_vec[i - 1] / req->basic_size) : 0;
    }

  fn_exit:
    if (len_vec)
        MPL_free(len_vec);
    return ucc_dtype;
  fn_fail:
    ucc_dtype = MPIDI_COMMON_UCC_DTYPE_UNSUPPORTED;
    goto fn_exit;
}

static inline void mpidi_ucc_dtype_packing_recv_donev(void *rbuf, const MPI_Aint rcounts[],
                                                      const MPI_Aint rdispls[],
                                                      MPI_Datatype mpi_dtype, int num_procs,
                                                      MPIDI_common_ucc_req_t * req)
{
    if (req->rbuf_tmp) {
        /* Here is where we should end up in response to `mpidi_ucc_dtype_packing_recv_prepv()`
         * after the data has been packed and `rcounts_tmp` and `rdispls_tmp` have been set up.
         */
        MPIR_Assert(req->rcounts_tmp && req->rdispls_tmp);
        MPI_Aint extent = 0;
        MPI_Aint actual_unpacked_bytes;
        char *buf_ptr = NULL;
        char *tmp_ptr = NULL;
        MPIR_Datatype_get_extent_macro(mpi_dtype, extent);
        for (int i = 0; i < num_procs; i++) {
            tmp_ptr = (char *) req->rbuf_tmp + req->rdispls_tmp[i] * req->basic_size;
            buf_ptr = (char *) rbuf + rdispls[i] * extent;
            int rc = MPIR_Typerep_unpack(tmp_ptr, req->rcounts_tmp[i] * req->basic_size, buf_ptr,
                                         rcounts[i], mpi_dtype, 0, &actual_unpacked_bytes,
                                         MPIR_TYPEREP_FLAG_NONE);
            MPIR_Assert(rc == MPI_SUCCESS);
            MPIR_Assert(actual_unpacked_bytes == (req->rcounts_tmp[i] * req->basic_size));
        }
    }
}

#endif /*_MPID_UCC_DTYPES_H_*/
