/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifdef ENABLE_NCCL

#define CUDA_ERR_CHECK(ret)             \
    if (unlikely((ret) != cudaSuccess)) \
    goto fn_fail

/*
 * Static helper functions 
 */

static int MPIR_NCCLcomm_init(MPIR_Comm *comm_ptr, int rank)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size = comm_ptr->local_size;
    cudaError_t ret;

    MPIR_NCCLcomm *ncclcomm;
    ncclcomm = MPL_malloc(sizeof(MPIR_NCCLcomm), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!ncclcomm, mpi_errno, MPI_ERR_OTHER, "**nomem");

    if (rank == 0) {
        ncclGetUniqueId(&(ncclcomm->id));
    }

    mpi_errno = MPIR_Bcast_impl(&(ncclcomm->id), sizeof(ncclcomm->id), MPIR_UINT8, 0, comm_ptr,
                                MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

    ret = cudaStreamCreate(&(ncclcomm->stream));
    CUDA_ERR_CHECK(ret);
    ret = ncclCommInitRank(&(ncclcomm->ncclcomm), comm_size, ncclcomm->id, rank);
    CUDA_ERR_CHECK(ret);

    comm_ptr->cclcomm->ncclcomm = ncclcomm;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

static int MPIR_NCCL_check_init_and_init(MPIR_Comm *comm_ptr, int rank)
{
    int mpi_errno = MPI_SUCCESS;

    if(!comm_ptr->cclcomm) {
        mpi_errno = MPIR_CCLcomm_init(comm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (!comm_ptr->cclcomm->ncclcomm) {
        mpi_errno = MPIR_NCCLcomm_init(comm_ptr, comm_ptr->rank);
        MPIR_ERR_CHECK(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

static int MPIR_NCCL_red_op_is_supported(MPI_Op op)
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

static int MPIR_NCCL_get_red_op(MPI_Op op, ncclRedOp_t *redOp)
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

static int MPIR_NCCL_datatype_is_supported(MPI_Datatype dtype)
{
    dtype = dtype & ~MPIR_TYPE_INDEX_MASK;
    switch (dtype) {
    case MPIR_INT8:
    case MPIR_UINT8:
    case MPIR_INT32:
    case MPIR_UINT32:
    case MPIR_INT64:
    case MPIR_UINT64:
    case MPIR_FLOAT16:
    case MPIR_FLOAT32:
    case MPIR_FLOAT64:
        return 1;
    default:
        return 0;
    }
}

static int MPIR_NCCL_get_datatype(MPI_Datatype dtype, ncclDataType_t *nccl_dtype)
{
    int mpi_errno = MPI_SUCCESS;

    dtype = dtype & ~MPIR_TYPE_INDEX_MASK;
    switch (dtype) {
    // Ignoring ncclChar b/c MPICH treats MPI_CHAR as MPIR_INT8 internally
    case MPIR_INT8:
        *nccl_dtype = ncclInt8;
        break;
    case MPIR_UINT8:
        *nccl_dtype = ncclUint8;
        break;
    case MPIR_INT32:
        *nccl_dtype = ncclInt32;
        break;
    case MPIR_UINT32:
        *nccl_dtype = ncclUint32;
        break;
    case MPIR_INT64:
        *nccl_dtype = ncclInt64;
        break;
    case MPIR_UINT64:
        *nccl_dtype = ncclUint64;
        break;
    case MPIR_FLOAT16:
        *nccl_dtype = ncclFloat16;
    case MPIR_FLOAT32:
        *nccl_dtype = ncclFloat32;
        break;
    case MPIR_FLOAT64:
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

/*
 * External functions 
 */

int MPIR_NCCL_check_requirements_red_op(const void *sendbuf, void *recvbuf, MPI_Datatype datatype, MPI_Op op)
{
    /* NCCL requires a supported red op and datatype, and both bufs must be on GPU */
    if (!MPIR_NCCL_red_op_is_supported(op) || !MPIR_NCCL_datatype_is_supported(datatype) || !MPIR_CCL_check_both_gpu_bufs(sendbuf, recvbuf)) {
        return 0;
    }
    
    return 1;
}

int MPIR_NCCL_Allreduce(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    cudaError_t ret;

    ncclRedOp_t ncclOp;
    mpi_errno = MPIR_NCCL_get_red_op(op, &ncclOp);
    MPIR_ERR_CHECK(mpi_errno);

    ncclDataType_t ncclDatatype;
    mpi_errno = MPIR_NCCL_get_datatype(datatype, &ncclDatatype);
    MPIR_ERR_CHECK(mpi_errno);

    /* Check the CCLcomm and NCCLcomm are initialized and init them if they are not */
    mpi_errno = MPIR_NCCL_check_init_and_init(comm_ptr, comm_ptr->rank);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_NCCLcomm *ncclcomm = comm_ptr->cclcomm->ncclcomm;

    /* Set the device to where the buffer resides */
    MPL_pointer_attr_t recv_attr;
    mpi_errno = MPL_gpu_query_pointer_attr(recvbuf, &recv_attr);
    MPIR_ERR_CHECK(mpi_errno);
    ret = cudaSetDevice(recv_attr.device_attr.device);
    CUDA_ERR_CHECK(ret);

    /* Finally make the NCCL call */
    ret = ncclAllReduce(sendbuf, recvbuf, count, ncclDatatype, ncclOp, ncclcomm->ncclcomm,
                        ncclcomm->stream);
    CUDA_ERR_CHECK(ret);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

int MPIR_NCCLcomm_free(MPIR_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    cudaError_t ret;

    MPIR_Assert(comm->cclcomm->ncclcomm);
    MPIR_CCLcomm *cclcomm = comm->cclcomm;

    ret = cudaStreamSynchronize(cclcomm->ncclcomm->stream);
    CUDA_ERR_CHECK(ret);
    ret = ncclCommDestroy(cclcomm->ncclcomm->ncclcomm);
    CUDA_ERR_CHECK(ret);
    ret = cudaStreamDestroy(cclcomm->ncclcomm->stream);
    CUDA_ERR_CHECK(ret);

    MPL_free(cclcomm->ncclcomm);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* ENABLE_NCCL */
