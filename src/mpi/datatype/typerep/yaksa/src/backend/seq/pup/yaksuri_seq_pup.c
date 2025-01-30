/*
* Copyright (C) by Argonne National Laboratory
*     See COPYRIGHT in top-level directory
*/

#include <string.h>
#include "yaksi.h"
#include "yaksuri_seqi.h"
#include <assert.h>
#include <stdlib.h>

#define MAX_IOV_LENGTH (8192)

int yaksuri_seq_pup_is_supported(yaksi_type_s * type, yaksa_op_t op, bool * is_supported)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_seqi_type_s *seq_type = (yaksuri_seqi_type_s *) type->backend.seq.priv;

    if (seq_type->pack || (type->is_contig && op == YAKSA_OP__REPLACE))
        *is_supported = true;
    else
        *is_supported = false;

    return rc;
}

int yaksuri_seq_ipack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                      yaksi_info_s * info, yaksa_op_t op)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_seqi_type_s *seq_type = (yaksuri_seqi_type_s *) type->backend.seq.priv;

    uintptr_t iov_pack_threshold = YAKSURI_SEQI_INFO__DEFAULT_IOV_PUP_THRESHOLD;
    if (info) {
        yaksuri_seqi_info_s *seq_info = (yaksuri_seqi_info_s *) info->backend.seq.priv;
        iov_pack_threshold = seq_info->iov_pack_threshold;
    }

    if (op == YAKSA_OP__REPLACE && type->is_contig) {
        memcpy(outbuf, (const char *) inbuf + type->true_lb, type->size * count);
    } else if (op == YAKSA_OP__REPLACE && type->size / type->num_contig >= iov_pack_threshold) {
        struct iovec iov[MAX_IOV_LENGTH];
        char *dbuf = (char *) outbuf;
        uintptr_t offset = 0;
        while (offset < type->num_contig * count) {
            uintptr_t actual_iov_len;
            rc = yaksi_iov(inbuf, count, type, offset, iov, MAX_IOV_LENGTH, &actual_iov_len);
            YAKSU_ERR_CHECK(rc, fn_fail);

            for (uintptr_t i = 0; i < actual_iov_len; i++) {
                memcpy(dbuf, iov[i].iov_base, iov[i].iov_len);
                dbuf += iov[i].iov_len;
            }

            offset += actual_iov_len;
        }
    } else {
        assert(seq_type->pack);
        rc = seq_type->pack(inbuf, outbuf, count, type, op);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_seq_iunpack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                        yaksi_info_s * info, yaksa_op_t op)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_seqi_type_s *seq_type = (yaksuri_seqi_type_s *) type->backend.seq.priv;

    uintptr_t iov_unpack_threshold = YAKSURI_SEQI_INFO__DEFAULT_IOV_PUP_THRESHOLD;
    if (info) {
        yaksuri_seqi_info_s *seq_info = (yaksuri_seqi_info_s *) info->backend.seq.priv;
        iov_unpack_threshold = seq_info->iov_unpack_threshold;
    }

    if (op == YAKSA_OP__REPLACE && type->is_contig) {
        memcpy((char *) outbuf + type->true_lb, inbuf, type->size * count);
    } else if (op == YAKSA_OP__REPLACE && type->size / type->num_contig >= iov_unpack_threshold) {
        struct iovec iov[MAX_IOV_LENGTH];
        const char *sbuf = (const char *) inbuf;
        uintptr_t offset = 0;

        while (offset < type->num_contig * count) {
            uintptr_t actual_iov_len;
            rc = yaksi_iov(outbuf, count, type, offset, iov, MAX_IOV_LENGTH, &actual_iov_len);
            YAKSU_ERR_CHECK(rc, fn_fail);

            for (uintptr_t i = 0; i < actual_iov_len; i++) {
                memcpy(iov[i].iov_base, sbuf, iov[i].iov_len);
                sbuf += iov[i].iov_len;
            }

            offset += actual_iov_len;
        }
    } else {
        assert(seq_type->unpack);
        rc = seq_type->unpack(inbuf, outbuf, count, type, op);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
