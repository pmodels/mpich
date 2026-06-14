/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPIR_TYPEREP_H_INCLUDED
#define MPIR_TYPEREP_H_INCLUDED

#if (MPICH_DATATYPE_ENGINE == MPICH_DATATYPE_ENGINE_YAKSA)
#include "yaksa.h"
typedef struct {
    yaksa_request_t req;
    yaksa_info_t info;
} MPIR_Typerep_req;
#define MPIR_TYPEREP_REQ_NULL YAKSA_REQUEST__NULL
#define MPIR_TYPEREP_HANDLE_TYPE yaksa_type_t
#define MPIR_TYPEREP_HANDLE_NULL YAKSA_TYPE__NULL
#else
typedef struct {
    int req;
    int info;
} MPIR_Typerep_req;             /* unused in dataloop */
#define MPIR_TYPEREP_REQ_NULL 0
#define MPIR_TYPEREP_HANDLE_TYPE struct MPII_Dataloop *
#define MPIR_TYPEREP_HANDLE_NULL NULL
#endif

/* FIXME: bad names. Not gpu-specific, confusing with MPIR_Request.
 *        It's a general async handle.
 */
typedef enum {
    MPIR_NULL_REQUEST = 0,
    MPIR_TYPEREP_REQUEST,
    MPIR_GPU_REQUEST,
} MPIR_request_type_t;

typedef struct {
    union {
        MPIR_Typerep_req y_req;
        MPL_gpu_request gpu_req;
    } u;
    MPIR_request_type_t type;
} MPIR_gpu_req;


void MPIR_Typerep_init(void);
void MPIR_Typerep_finalize(void);

int MPIR_Typerep_create_vector(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride,
                               MPI_Datatype oldtype, MPIR_Datatype * newtype);
int MPIR_Typerep_create_hvector(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride,
                                MPI_Datatype oldtype, MPIR_Datatype * newtype);
int MPIR_Typerep_create_contig(MPI_Aint count, MPI_Datatype oldtype, MPIR_Datatype * newtype);
int MPIR_Typerep_create_dup(MPI_Datatype oldtype, MPIR_Datatype * newtype);
int MPIR_Typerep_create_indexed_block(MPI_Aint count, MPI_Aint blocklength,
                                      const MPI_Aint * array_of_displacements, MPI_Datatype oldtype,
                                      MPIR_Datatype * newtype);
int MPIR_Typerep_create_hindexed_block(MPI_Aint count, MPI_Aint blocklength,
                                       const MPI_Aint * array_of_displacements,
                                       MPI_Datatype oldtype, MPIR_Datatype * newtype);
int MPIR_Typerep_create_indexed(MPI_Aint count, const MPI_Aint * array_of_blocklengths,
                                const MPI_Aint * array_of_displacements, MPI_Datatype oldtype,
                                MPIR_Datatype * newtype);
int MPIR_Typerep_create_hindexed(MPI_Aint count, const MPI_Aint * array_of_blocklengths,
                                 const MPI_Aint * array_of_displacements, MPI_Datatype oldtype,
                                 MPIR_Datatype * newtype);
int MPIR_Typerep_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                                MPIR_Datatype * newtype);
int MPIR_Typerep_create_struct(MPI_Aint count, const MPI_Aint * array_of_blocklengths,
                               const MPI_Aint * array_of_displacements,
                               const MPI_Datatype * array_of_types, MPIR_Datatype * newtype);

void MPIR_Typerep_commit(MPI_Datatype type);
void MPIR_Typerep_free(MPIR_Datatype * typeptr);

int MPIR_Typerep_flatten_size(MPIR_Datatype * datatype_ptr, int *flattened_type_size);
int MPIR_Typerep_flatten(MPIR_Datatype * datatype_ptr, void *flattened_type);
int MPIR_Typerep_unflatten(MPIR_Datatype ** datatype_ptr, void *flattened_type);

/* byte-based offset routine: do not use; only maintained for backward
 * compatibility with ch3 */
int MPIR_Typerep_to_iov(const void *buf, MPI_Aint count, MPI_Datatype type, MPI_Aint byte_offset,
                        struct iovec *iov, MPI_Aint max_iov_len, MPI_Aint max_iov_bytes,
                        MPI_Aint * actual_iov_len, MPI_Aint * actual_iov_bytes);
/* iov element count based routine */
int MPIR_Typerep_to_iov_offset(const void *buf, MPI_Aint count, MPI_Datatype type,
                               MPI_Aint iov_offset, struct iovec *iov, MPI_Aint max_iov_len,
                               MPI_Aint * actual_iov_len);
int MPIR_Typerep_iov_len(MPI_Aint count, MPI_Datatype type, MPI_Aint max_iov_bytes,
                         MPI_Aint * iov_len, MPI_Aint * actual_iov_bytes);

/* We nearly always just want the total iov number */
#define MPIR_Typerep_get_iov_len(count, type, iov_len) \
    MPIR_Typerep_iov_len(count, type, -1, iov_len, NULL)

#define MPIR_TYPEREP_FLAG_NONE          0x0UL
#define MPIR_TYPEREP_FLAG_STREAM        0x1UL

int MPIR_Typerep_copy(void *outbuf, const void *inbuf, MPI_Aint num_bytes, uint32_t flags);
int MPIR_Typerep_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                      MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                      MPI_Aint * actual_pack_bytes, uint32_t flags);
int MPIR_Typerep_unpack(const void *inbuf, MPI_Aint insize,
                        void *outbuf, MPI_Aint outcount, MPI_Datatype datatype, MPI_Aint outoffset,
                        MPI_Aint * actual_unpack_bytes, uint32_t flags);
int MPIR_Typerep_icopy(void *outbuf, const void *inbuf, MPI_Aint num_bytes,
                       MPIR_Typerep_req * typerep_req, uint32_t flags);
int MPIR_Typerep_ipack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                       MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                       MPI_Aint * actual_pack_bytes, MPIR_Typerep_req * typerep_req,
                       uint32_t flags);
int MPIR_Typerep_iunpack(const void *inbuf, MPI_Aint insize, void *outbuf, MPI_Aint outcount,
                         MPI_Datatype datatype, MPI_Aint outoffset, MPI_Aint * actual_unpack_bytes,
                         MPIR_Typerep_req * typerep_req, uint32_t flags);

MPI_Aint MPIR_Typerep_size_external32(MPI_Datatype type);
int MPIR_Typerep_pack_external(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                               void *outbuf, MPI_Aint * actual_pack_bytes);
int MPIR_Typerep_unpack_external(const void *inbuf, void *outbuf, MPI_Aint outcount,
                                 MPI_Datatype datatype, MPI_Aint * actual_unpack_bytes);

void MPIR_Typerep_debug(MPI_Datatype type);

/* HACK: if count > 0, it only return true if data_sz <= MPIR_CVAR_YAKSA_REDUCTION_THRESHOLD */
int MPIR_Typerep_reduce_is_supported(MPI_Op op, MPI_Aint count, MPI_Datatype datatype);

int MPIR_Typerep_op(void *source_buf, MPI_Aint source_count, MPI_Datatype source_dtp,
                    void *target_buf, MPI_Aint target_count, MPI_Datatype target_dtp, MPI_Op op,
                    bool source_is_packed, int mapped_device);
int MPIR_Typerep_reduce(const void *in_buf, void *out_buf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op);

int MPIR_Typerep_pack_stream(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                             MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                             MPI_Aint * actual_pack_bytes, void *stream);
int MPIR_Typerep_unpack_stream(const void *inbuf, MPI_Aint insize,
                               void *outbuf, MPI_Aint outcount, MPI_Datatype datatype,
                               MPI_Aint outoffset, MPI_Aint * actual_unpack_bytes, void *stream);

int MPIR_Typerep_wait(MPIR_Typerep_req typerep_req);
int MPIR_Typerep_test(MPIR_Typerep_req typerep_req, int *completed);

MPL_STATIC_INLINE_PREFIX void MPIR_async_test(MPIR_gpu_req * areq, int *is_done)
{
    int err;
    switch (areq->type) {
        case MPIR_NULL_REQUEST:
            /* a dummy, immediately complete */
            *is_done = 1;
            break;
        case MPIR_TYPEREP_REQUEST:
            MPIR_Typerep_test(areq->u.y_req, is_done);
            if (*is_done) {
                areq->type = MPIR_NULL_REQUEST;
            }
            break;
        case MPIR_GPU_REQUEST:
            err = MPL_gpu_test(&areq->u.gpu_req, is_done);
            MPIR_Assertp(err == MPL_SUCCESS);
            if (*is_done) {
                areq->type = MPIR_NULL_REQUEST;
            }
            break;
        default:
            MPIR_Assert(0);
    }
}

/* declared here due to dependency on MPIR_Typerep_req */
int MPIR_Localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype);
int MPIR_Ilocalcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                    MPIR_Typerep_req * typerep_req);
int MPIR_Localcopy_stream(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                          void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, void *stream);
int MPIR_Localcopy_gpu(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                       MPI_Aint sendoffset, MPL_pointer_attr_t * sendattr, void *recvbuf,
                       MPI_Aint recvcount, MPI_Datatype recvtype, MPI_Aint recvoffset,
                       MPL_pointer_attr_t * recvattr,
                       MPL_gpu_engine_type_t enginetype, bool commit);
int MPIR_Ilocalcopy_gpu(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                        MPI_Aint sendoffset, MPL_pointer_attr_t * sendattr, void *recvbuf,
                        MPI_Aint recvcount, MPI_Datatype recvtype, MPI_Aint recvoffset,
                        MPL_pointer_attr_t * recvattr,
                        MPL_gpu_engine_type_t enginetype, bool commit, MPIR_gpu_req * req);

#endif /* MPIR_TYPEREP_H_INCLUDED */
