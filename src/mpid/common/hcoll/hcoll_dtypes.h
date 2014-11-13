#ifndef _HCOLL_DTYPES_H_
#define _HCOLL_DTYPES_H_
#include "hcoll/api/hcoll_dte.h"

static dte_data_representation_t mpi_dtype_2_dte_dtype(MPI_Datatype datatype)
{
    switch (datatype) {
    case MPI_CHAR:
    case MPI_SIGNED_CHAR:
        return DTE_BYTE;
    case MPI_SHORT:
        return DTE_INT16;
    case MPI_INT:
        return DTE_INT32;
    case MPI_LONG:
    case MPI_LONG_LONG:
        return DTE_INT64;
        /* return DTE_INT128; */
    case MPI_BYTE:
    case MPI_UNSIGNED_CHAR:
        return DTE_UBYTE;
    case MPI_UNSIGNED_SHORT:
        return DTE_UINT16;
    case MPI_UNSIGNED:
        return DTE_UINT32;
    case MPI_UNSIGNED_LONG:
    case MPI_UNSIGNED_LONG_LONG:
        return DTE_UINT64;
        /* return DTE_UINT128; */
    case MPI_FLOAT:
        return DTE_FLOAT32;
    case MPI_DOUBLE:
        return DTE_FLOAT64;
    case MPI_LONG_DOUBLE:
        return DTE_FLOAT128;
    default:
        return DTE_ZERO;
    }
}

static hcoll_dte_op_t *mpi_op_2_dte_op(MPI_Op op)
{
    switch (op) {
    case MPI_MAX:
        return &hcoll_dte_op_max;
    case MPI_MIN:
        return &hcoll_dte_op_min;
    case MPI_SUM:
        return &hcoll_dte_op_sum;
    case MPI_PROD:
        return &hcoll_dte_op_prod;
    case MPI_LAND:
        return &hcoll_dte_op_land;
    case MPI_BAND:
        return &hcoll_dte_op_band;
    case MPI_LOR:
        return &hcoll_dte_op_lor;
    case MPI_BOR:
        return &hcoll_dte_op_bor;
    case MPI_LXOR:
        return &hcoll_dte_op_lxor;
    case MPI_BXOR:
        return &hcoll_dte_op_bxor;
    default:
        return &hcoll_dte_op_null;
    }
}
#endif
