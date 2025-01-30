/*
* Copyright (C) by Argonne National Laboratory
*     See COPYRIGHT in top-level directory
*/

#include <assert.h>
#include <hip/hip_runtime.h>
#include <hip/hip_runtime_api.h>
#include "yaksi.h"
#include "yaksuri_hipi.h"
#include <stdlib.h>

#define THREAD_BLOCK_SIZE  (256)
#define MAX_GRIDSZ_X       ((1U << 31) - 1)
#define MAX_GRIDSZ_Y       (65535U)
#define MAX_GRIDSZ_Z       (65535U)

#define MAX_IOV_LENGTH (8192)

static int get_thread_block_dims(uintptr_t count, yaksi_type_s * type, unsigned int *n_threads,
                                 unsigned int *n_blocks_x, unsigned int *n_blocks_y,
                                 unsigned int *n_blocks_z)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_hipi_type_s *hip_type = (yaksuri_hipi_type_s *) type->backend.hip.priv;

    *n_threads = THREAD_BLOCK_SIZE;
    uintptr_t n_blocks = count * hip_type->num_elements / THREAD_BLOCK_SIZE;
    n_blocks += ! !(count * hip_type->num_elements % THREAD_BLOCK_SIZE);

    if (n_blocks <= MAX_GRIDSZ_X) {
        *n_blocks_x = (unsigned int) n_blocks;
        *n_blocks_y = 1;
        *n_blocks_z = 1;
    } else if (n_blocks <= MAX_GRIDSZ_X * MAX_GRIDSZ_Y) {
        *n_blocks_x = (unsigned int) (YAKSU_CEIL(n_blocks, MAX_GRIDSZ_Y));
        *n_blocks_y = (unsigned int) (YAKSU_CEIL(n_blocks, (*n_blocks_x)));
        *n_blocks_z = 1;
    } else {
        uintptr_t n_blocks_xy = YAKSU_CEIL(n_blocks, MAX_GRIDSZ_Z);
        *n_blocks_x = (unsigned int) (YAKSU_CEIL(n_blocks_xy, MAX_GRIDSZ_Y));
        *n_blocks_y = (unsigned int) (YAKSU_CEIL(n_blocks_xy, (*n_blocks_x)));
        *n_blocks_z =
            (unsigned int) (YAKSU_CEIL(n_blocks, (uintptr_t) (*n_blocks_x) * (*n_blocks_y)));
    }

    return rc;
}

int yaksuri_hipi_pup_is_supported(yaksi_type_s * type, yaksa_op_t op, bool * is_supported)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_hipi_type_s *hip_type = (yaksuri_hipi_type_s *) type->backend.hip.priv;

    if ((type->is_contig && op == YAKSA_OP__REPLACE) || hip_type->pack)
        *is_supported = true;
    else
        *is_supported = false;

    return rc;
}

uintptr_t yaksuri_hipi_get_iov_pack_threshold(yaksi_info_s * info)
{
    uintptr_t iov_pack_threshold = YAKSURI_HIPI_INFO__DEFAULT_IOV_PUP_THRESHOLD;
    if (info) {
        yaksuri_hipi_info_s *hip_info = (yaksuri_hipi_info_s *) info->backend.hip.priv;
        iov_pack_threshold = hip_info->iov_pack_threshold;
    }

    return iov_pack_threshold;
}

uintptr_t yaksuri_hipi_get_iov_unpack_threshold(yaksi_info_s * info)
{
    uintptr_t iov_unpack_threshold = YAKSURI_HIPI_INFO__DEFAULT_IOV_PUP_THRESHOLD;
    if (info) {
        yaksuri_hipi_info_s *hip_info = (yaksuri_hipi_info_s *) info->backend.hip.priv;
        iov_unpack_threshold = hip_info->iov_unpack_threshold;
    }

    return iov_unpack_threshold;
}

int yaksuri_hipi_ipack_with_stream(const void *inbuf, void *outbuf, uintptr_t count,
                                   yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                   int target, void *stream_p)
{
    int rc = YAKSA_SUCCESS;
    hipStream_t stream = *(hipStream_t *) stream_p;
    yaksuri_hipi_type_s *hip_type = (yaksuri_hipi_type_s *) type->backend.hip.priv;
    hipError_t cerr;

    uintptr_t iov_pack_threshold = yaksuri_hipi_get_iov_pack_threshold(info);

    /* shortcut for contiguous types */
    if (op == YAKSA_OP__REPLACE && type->is_contig) {
        /* hip performance is optimized when we synchronize on the
         * source buffer's GPU */
        cerr = hipMemcpyAsync(outbuf, (const char *) inbuf + type->true_lb, count * type->size,
                              hipMemcpyDefault, stream);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    } else if (op == YAKSA_OP__REPLACE && type->size / type->num_contig >= iov_pack_threshold) {
        struct iovec iov[MAX_IOV_LENGTH];
        char *dbuf = (char *) outbuf;
        uintptr_t offset = 0;

        while (offset < type->num_contig * count) {
            uintptr_t actual_iov_len;
            rc = yaksi_iov(inbuf, count, type, offset, iov, MAX_IOV_LENGTH, &actual_iov_len);
            YAKSU_ERR_CHECK(rc, fn_fail);

            for (uintptr_t i = 0; i < actual_iov_len; i++) {
                hipMemcpyAsync(dbuf, iov[i].iov_base, iov[i].iov_len, hipMemcpyDefault, stream);
                dbuf += iov[i].iov_len;
            }

            offset += actual_iov_len;
        }
    } else {
        rc = yaksuri_hipi_md_alloc(type);
        YAKSU_ERR_CHECK(rc, fn_fail);

        unsigned int n_threads;
        unsigned int n_blocks_x, n_blocks_y, n_blocks_z;
        rc = get_thread_block_dims(count, type, &n_threads, &n_blocks_x, &n_blocks_y, &n_blocks_z);
        YAKSU_ERR_CHECK(rc, fn_fail);

        int cur_device;
        cerr = hipGetDevice(&cur_device);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        cerr = hipSetDevice(target);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        hip_type->pack(inbuf, outbuf, count, op, hip_type->md, n_threads, n_blocks_x, n_blocks_y,
                       n_blocks_z, stream);

        cerr = hipSetDevice(cur_device);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_hipi_iunpack_with_stream(const void *inbuf, void *outbuf, uintptr_t count,
                                     yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                     int target, void *stream_p)
{
    int rc = YAKSA_SUCCESS;
    hipStream_t stream = *(hipStream_t *) stream_p;
    yaksuri_hipi_type_s *hip_type = (yaksuri_hipi_type_s *) type->backend.hip.priv;
    hipError_t cerr;

    uintptr_t iov_unpack_threshold = yaksuri_hipi_get_iov_unpack_threshold(info);

    /* shortcut for contiguous types */
    if (op == YAKSA_OP__REPLACE && type->is_contig) {
        /* hip performance is optimized when we synchronize on the
         * source buffer's GPU */
        cerr = hipMemcpyAsync((char *) outbuf + type->true_lb, inbuf, count * type->size,
                              hipMemcpyDefault, stream);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    } else if (op == YAKSA_OP__REPLACE && type->size / type->num_contig >= iov_unpack_threshold) {
        struct iovec iov[MAX_IOV_LENGTH];
        const char *sbuf = (const char *) inbuf;
        uintptr_t offset = 0;

        while (offset < type->num_contig * count) {
            uintptr_t actual_iov_len;
            rc = yaksi_iov(outbuf, count, type, offset, iov, MAX_IOV_LENGTH, &actual_iov_len);
            YAKSU_ERR_CHECK(rc, fn_fail);

            for (uintptr_t i = 0; i < actual_iov_len; i++) {
                hipMemcpyAsync(iov[i].iov_base, sbuf, iov[i].iov_len, hipMemcpyDefault, stream);
                sbuf += iov[i].iov_len;
            }

            offset += actual_iov_len;
        }
    } else {
        rc = yaksuri_hipi_md_alloc(type);
        YAKSU_ERR_CHECK(rc, fn_fail);

        unsigned int n_threads;
        unsigned int n_blocks_x, n_blocks_y, n_blocks_z;
        rc = get_thread_block_dims(count, type, &n_threads, &n_blocks_x, &n_blocks_y, &n_blocks_z);
        YAKSU_ERR_CHECK(rc, fn_fail);

        int cur_device;
        cerr = hipGetDevice(&cur_device);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        cerr = hipSetDevice(target);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

        hip_type->unpack(inbuf, outbuf, count, op, hip_type->md, n_threads, n_blocks_x,
                         n_blocks_y, n_blocks_z, stream);

        cerr = hipSetDevice(cur_device);
        YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_hipi_ipack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                       yaksi_info_s * info, yaksa_op_t op, int target)
{
    return yaksuri_hipi_ipack_with_stream(inbuf, outbuf, count, type, info, op, target,
                                          &yaksuri_hipi_global.stream[target]);
}

int yaksuri_hipi_iunpack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                         yaksi_info_s * info, yaksa_op_t op, int target)
{
    return yaksuri_hipi_iunpack_with_stream(inbuf, outbuf, count, type, info, op, target,
                                            &yaksuri_hipi_global.stream[target]);
}

int yaksuri_hipi_synchronize(int target)
{
    int rc = YAKSA_SUCCESS;
    hipError_t cerr;

    cerr = hipStreamSynchronize(yaksuri_hipi_global.stream[target]);
    YAKSURI_HIPI_HIP_ERR_CHKANDJUMP(cerr, rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_hipi_flush_all(void)
{
    return YAKSA_SUCCESS;
}
