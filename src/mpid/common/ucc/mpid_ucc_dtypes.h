/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

#endif /*_MPID_UCC_DTYPES_H_*/
