/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#define CUDA_ERR_CHECK(ret) if (unlikely((ret) != cudaSuccess)) goto fn_fail

/*
 * CCLcomm functions, currently tied to NCCL
 */

int MPIR_CCLcomm_init(MPIR_Comm * comm, int rank)
{
    int mpi_errno = MPI_SUCCESS;
    cudaError_t ret;
    int comm_size = comm->local_size;

    MPIR_CCLcomm *cclcomm;
    cclcomm = MPL_malloc(sizeof(MPIR_CCLcomm), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!cclcomm, mpi_errno, MPI_ERR_OTHER, "**nomem");

    if(rank == 0) {
        ncclGetUniqueId(&(cclcomm->id));
    }

    mpi_errno = MPIR_Bcast_impl(&(cclcomm->id), sizeof(cclcomm->id), MPI_INT, 0, comm,
                                    MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);
    
    ret = cudaSetDevice(0); //TODO: Fix this for multi-GPU nodes
    CUDA_ERR_CHECK(ret);
    ret = cudaStreamCreate(&(cclcomm->stream));
    CUDA_ERR_CHECK(ret);
    ret = ncclCommInitRank(&(cclcomm->ncclcomm), comm_size, cclcomm->id, rank);
    CUDA_ERR_CHECK(ret);

    cclcomm->comm = comm;
    comm->cclcomm = cclcomm;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_CCLcomm_free(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    cudaError_t ret;

    MPIR_Assert(comm->cclcomm);
    MPIR_CCLcomm *cclcomm = comm->cclcomm;

    ret = cudaStreamSynchronize(cclcomm->stream);
    CUDA_ERR_CHECK(ret);
    ret = ncclCommDestroy(cclcomm->ncclcomm);
    CUDA_ERR_CHECK(ret);
    ret = cudaStreamDestroy(cclcomm->stream);
    CUDA_ERR_CHECK(ret);

    MPL_free(cclcomm);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * NCCL-specific functions
 */

int MPIR_NCCL_red_op_is_supported(MPI_Op op)
{
    switch (op) {
      case MPI_SUM:
      case MPI_PROD:
      case MPI_MIN:
      case MPI_MAX:
        return 1;
      default:
        return 0;
    }
}

int MPIR_NCCL_get_red_op(MPI_Op op, ncclRedOp_t *redOp)
{
    int mpi_errno = MPI_SUCCESS;

    switch (op) {
      case MPI_SUM:
        *redOp = ncclSum;
        break;
      case MPI_PROD:
        *redOp = ncclProd;
        break;
      case MPI_MIN:
        *redOp = ncclMin;
        break;
      case MPI_MAX:
        *redOp = ncclMax;
        break;
      default:
        goto fn_fail;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    mpi_errno = MPI_ERR_ARG;
    goto fn_exit;
}

int MPIR_NCCL_datatype_is_supported(MPI_Datatype dtype)
{
    switch (dtype) {
      case MPI_CHAR:
      case MPI_SIGNED_CHAR:
      case MPI_CHARACTER:
      case MPI_INTEGER1:
      case MPI_INTEGER:
      case MPI_INT8_T:
      case MPI_BYTE:
      case MPI_UINT8_T:
      case MPI_INT:
      case MPI_INT32_T:
      case MPI_INTEGER4:
      case MPI_UNSIGNED:
      case MPI_UINT32_T:
      case MPI_LONG:
      case MPI_INT64_T:
      case MPI_INTEGER8:
      case MPI_UNSIGNED_LONG:
      case MPI_UINT64_T:
      case MPI_DOUBLE_COMPLEX:
      case MPI_FLOAT:
      case MPI_C_COMPLEX:
      case MPI_CXX_FLOAT_COMPLEX:
      case MPI_DOUBLE:
      case MPI_C_DOUBLE_COMPLEX:
      case MPI_COMPLEX8:
      case MPI_CXX_DOUBLE_COMPLEX:
        return 1;
      default:
        return 0;
    }
}

int MPIR_NCCL_get_datatype(MPI_Datatype dtype, ncclDataType_t *nccl_dtype)
{
    int mpi_errno = MPI_SUCCESS;

    switch (dtype) {
      case MPI_CHAR:
      case MPI_SIGNED_CHAR:
      case MPI_CHARACTER:
        *nccl_dtype = ncclChar;
        break;
      case MPI_INTEGER1:
      case MPI_INTEGER:
      case MPI_INT8_T:
        *nccl_dtype = ncclInt8;
        break;
      case MPI_BYTE:
      case MPI_UINT8_T:
        *nccl_dtype = ncclUint8;
        break;
      case MPI_INT:
      case MPI_INT32_T:
      case MPI_INTEGER4:
        *nccl_dtype = ncclInt32;
        break;
      case MPI_UNSIGNED:
      case MPI_UINT32_T:
        *nccl_dtype = ncclUint32;
        break;
      case MPI_LONG:
      case MPI_INT64_T:
      case MPI_INTEGER8:
        *nccl_dtype = ncclInt64;
        break;
      case MPI_UNSIGNED_LONG:
      case MPI_UINT64_T:
        *nccl_dtype = ncclUint64;
        break;
      case MPI_DOUBLE_COMPLEX:
        *nccl_dtype = ncclFloat16;
      case MPI_FLOAT:
      case MPI_C_COMPLEX:
      case MPI_CXX_FLOAT_COMPLEX:
        *nccl_dtype = ncclFloat32;
        break;
      case MPI_DOUBLE:
      case MPI_C_DOUBLE_COMPLEX:
      case MPI_COMPLEX8:
      case MPI_CXX_DOUBLE_COMPLEX:
        *nccl_dtype = ncclFloat64;
        break;
      default:
        goto fn_fail;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    mpi_errno = MPI_ERR_ARG;
    goto fn_exit;
}

int MPIR_NCCL_Allreduce(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    cudaError_t ret;

    ncclRedOp_t ncclOp;
    mpi_errno = MPIR_NCCL_get_red_op(op, &ncclOp);
    MPIR_ERR_CHECK(mpi_errno);

    ncclDataType_t ncclDatatype;
    mpi_errno = MPIR_NCCL_get_datatype(datatype, &ncclDatatype);
    MPIR_ERR_CHECK(mpi_errno);

    if(!comm_ptr->cclcomm) {
      mpi_errno = MPIR_CCLcomm_init(comm_ptr, comm_ptr->rank);
      MPIR_ERR_CHECK(mpi_errno);
    }

    ret = ncclAllReduce(sendbuf, recvbuf, count, ncclDatatype, ncclOp, comm_ptr->cclcomm->ncclcomm, 
                        comm_ptr->cclcomm->stream);
    CUDA_ERR_CHECK(ret);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * CCL wrapper functions
 */

int MPIR_CCL_red_op_is_supported(MPI_Op op)
{
  MPIR_NCCL_red_op_is_supported(op);
}

int MPIR_CCL_datatype_is_supported(MPI_Datatype datatype)
{
  MPIR_NCCL_datatype_is_supported(datatype);
}

int MPIR_CCL_Allreduce(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag)
{
  return MPIR_NCCL_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
}
