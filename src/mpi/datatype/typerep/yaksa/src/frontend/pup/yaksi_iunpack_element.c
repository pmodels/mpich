/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* In each of the below functions, we follow these steps:
 *
 *  1. Skip the first few blocks till we reach a block that can
 *  contribute some data to our unpacking.
 *
 *  2. Partial unpack the next block.  Once unpacked, if either the
 *  unpack buffer is full or there's data left over in this block,
 *  return.
 *
 *  3. Perform a full unpack of the next few blocks.
 *
 *  4. Partial unpack the next block.  Once unpacked, if either the
 *  unpack buffer is full or there's data left over in this block,
 *  return.
 */

static int unpack_sub_hvector(const void *inbuf, uintptr_t insize, void *outbuf,
                              yaksi_type_s * type, uintptr_t outoffset,
                              uintptr_t * actual_unpack_bytes, yaksi_info_s * info,
                              yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    /* make sure we have something left to unpack after the offset */
    assert(outoffset < type->size);

    const char *sbuf = (const char *) inbuf;
    char *dbuf = (char *) outbuf;
    uintptr_t remoffset = outoffset;
    uintptr_t bytes_in_block = type->size / type->u.hvector.count;
    uintptr_t rem_unpack_bytes = YAKSU_MIN(insize, type->size - outoffset);

    /* step 1: skip the first few blocks */
    if (remoffset) {
        uintptr_t skipblocks = remoffset / bytes_in_block;

        remoffset %= bytes_in_block;
        dbuf += skipblocks * type->u.hvector.stride;
    }


    /* step 2: partial pack the next block */
    if (remoffset) {
        assert(bytes_in_block > remoffset);

        uintptr_t tmp_unpack_bytes = YAKSU_MIN(rem_unpack_bytes,
                                               bytes_in_block - remoffset);

        uintptr_t tmp_actual_unpack_bytes;
        rc = yaksi_iunpack(sbuf, tmp_unpack_bytes, dbuf, type->u.hvector.blocklength,
                           type->u.hvector.child, remoffset, &tmp_actual_unpack_bytes, info,
                           op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rem_unpack_bytes -= tmp_actual_unpack_bytes;
        *actual_unpack_bytes += tmp_actual_unpack_bytes;

        if (rem_unpack_bytes == 0 ||
            tmp_actual_unpack_bytes <
            type->u.hvector.child->size * type->u.hvector.blocklength - remoffset) {
            /* if we are out of unpack buffer space or if we could not
             * unpack fully, return */
            goto fn_exit;
        }

        remoffset = 0;
        sbuf += tmp_unpack_bytes;
        dbuf += type->u.hvector.stride;
    }


    /* step 3: perform a full pack of the next few elements */
    uintptr_t numblocks;
    numblocks = rem_unpack_bytes / bytes_in_block;
    for (int i = 0; i < numblocks; i++) {
        rc = yaksi_iunpack_backend(sbuf, dbuf, type->u.hvector.blocklength, type->u.hvector.child,
                                   info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rem_unpack_bytes -= bytes_in_block;
        *actual_unpack_bytes += bytes_in_block;

        sbuf += bytes_in_block;
        dbuf += type->u.hvector.stride;
    }


    /* step 4: partial pack the next element */
    if (rem_unpack_bytes) {
        uintptr_t tmp_actual_unpack_bytes;

        rc = yaksi_iunpack(sbuf, rem_unpack_bytes, dbuf, type->u.hvector.blocklength,
                           type->u.hvector.child, remoffset, &tmp_actual_unpack_bytes, info,
                           op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        *actual_unpack_bytes += tmp_actual_unpack_bytes;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_sub_blkhindx(const void *inbuf, uintptr_t insize, void *outbuf,
                               yaksi_type_s * type, uintptr_t outoffset,
                               uintptr_t * actual_unpack_bytes, yaksi_info_s * info,
                               yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    /* make sure we have something left to unpack after the offset */
    assert(outoffset < type->size);

    const char *sbuf = (const char *) inbuf;
    char *dbuf;
    uintptr_t remoffset = outoffset;
    uintptr_t bytes_in_block = type->size / type->u.blkhindx.count;
    uintptr_t rem_unpack_bytes = YAKSU_MIN(insize, type->size - outoffset);
    uintptr_t blockid = 0;

    /* step 1: skip the first few blocks */
    if (remoffset) {
        uintptr_t skipblocks = remoffset / bytes_in_block;

        remoffset %= bytes_in_block;
        blockid = skipblocks;
    }


    /* step 2: partial pack the next block */
    if (remoffset) {
        assert(bytes_in_block > remoffset);

        uintptr_t tmp_unpack_bytes = YAKSU_MIN(rem_unpack_bytes,
                                               bytes_in_block - remoffset);

        uintptr_t tmp_actual_unpack_bytes;
        dbuf = (char *) outbuf + type->u.blkhindx.array_of_displs[blockid++];
        rc = yaksi_iunpack(sbuf, tmp_unpack_bytes, dbuf, type->u.blkhindx.blocklength,
                           type->u.blkhindx.child, remoffset, &tmp_actual_unpack_bytes, info,
                           op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        sbuf += tmp_actual_unpack_bytes;

        rem_unpack_bytes -= tmp_actual_unpack_bytes;
        *actual_unpack_bytes += tmp_actual_unpack_bytes;

        if (rem_unpack_bytes == 0 ||
            tmp_actual_unpack_bytes <
            type->u.blkhindx.child->size * type->u.blkhindx.blocklength - remoffset) {
            /* if we are out of unpack buffer space or if we could not
             * unpack fully, return */
            goto fn_exit;
        }

        remoffset = 0;
    }


    /* step 3: perform a full pack of the next few elements */
    uintptr_t numblocks;
    numblocks = rem_unpack_bytes / bytes_in_block;
    for (int i = 0; i < numblocks; i++) {
        dbuf = (char *) outbuf + type->u.blkhindx.array_of_displs[blockid++];
        rc = yaksi_iunpack_backend(sbuf, dbuf, type->u.blkhindx.blocklength, type->u.blkhindx.child,
                                   info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        sbuf += bytes_in_block;

        rem_unpack_bytes -= bytes_in_block;
        *actual_unpack_bytes += bytes_in_block;
    }


    /* step 4: partial pack the next element */
    if (rem_unpack_bytes) {
        uintptr_t tmp_actual_unpack_bytes;

        dbuf = (char *) outbuf + type->u.blkhindx.array_of_displs[blockid++];
        rc = yaksi_iunpack(sbuf, rem_unpack_bytes, dbuf, type->u.blkhindx.blocklength,
                           type->u.blkhindx.child, remoffset, &tmp_actual_unpack_bytes, info,
                           op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        sbuf += tmp_actual_unpack_bytes;
        *actual_unpack_bytes += tmp_actual_unpack_bytes;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_sub_hindexed(const void *inbuf, uintptr_t insize, void *outbuf,
                               yaksi_type_s * type, uintptr_t outoffset,
                               uintptr_t * actual_unpack_bytes, yaksi_info_s * info,
                               yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    /* make sure we have something left to unpack after the offset */
    assert(outoffset < type->size);

    const char *sbuf = (const char *) inbuf;
    char *dbuf;
    uintptr_t remoffset = outoffset;
    uintptr_t rem_unpack_bytes = YAKSU_MIN(insize, type->size - outoffset);
    uintptr_t blockid = 0;

    /* step 1: skip the first few blocks */
    if (remoffset) {
        for (int i = 0; i < type->u.hindexed.count; i++) {
            uintptr_t bytes_in_block = type->u.hindexed.array_of_blocklengths[i] *
                type->u.hindexed.child->size;

            if (remoffset < bytes_in_block) {
                break;
            } else {
                remoffset -= bytes_in_block;
                blockid++;
            }
        }
    }


    /* step 2: partial pack the next block */
    if (remoffset) {
        uintptr_t bytes_in_block;
        do {
            bytes_in_block = type->u.hindexed.array_of_blocklengths[blockid] *
                type->u.hindexed.child->size;
        } while (bytes_in_block == 0 && ++blockid);
        assert(bytes_in_block > remoffset);

        uintptr_t tmp_unpack_bytes = YAKSU_MIN(rem_unpack_bytes, bytes_in_block - remoffset);
        uintptr_t tmp_actual_unpack_bytes;

        dbuf = (char *) outbuf + type->u.hindexed.array_of_displs[blockid];
        rc = yaksi_iunpack(sbuf, tmp_unpack_bytes, dbuf,
                           type->u.hindexed.array_of_blocklengths[blockid], type->u.hindexed.child,
                           remoffset, &tmp_actual_unpack_bytes, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        sbuf += tmp_actual_unpack_bytes;

        rem_unpack_bytes -= tmp_actual_unpack_bytes;
        *actual_unpack_bytes += tmp_actual_unpack_bytes;

        if (rem_unpack_bytes == 0 ||
            tmp_actual_unpack_bytes < type->u.hindexed.child->size *
            type->u.hindexed.array_of_blocklengths[blockid] - remoffset) {
            /* if we are out of unpack buffer space or if we could not
             * unpack fully, return */
            goto fn_exit;
        }

        remoffset = 0;
        blockid++;
    }


    /* step 3: perform a full pack of the next few elements */
    while (1) {
        uintptr_t bytes_in_block;
        do {
            bytes_in_block = type->u.hindexed.array_of_blocklengths[blockid] *
                type->u.hindexed.child->size;
        } while (bytes_in_block == 0 && ++blockid);

        if (rem_unpack_bytes < bytes_in_block)
            break;

        dbuf = (char *) outbuf + type->u.hindexed.array_of_displs[blockid];
        rc = yaksi_iunpack_backend(sbuf, dbuf, type->u.hindexed.array_of_blocklengths[blockid],
                                   type->u.hindexed.child, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        sbuf += bytes_in_block;

        rem_unpack_bytes -= bytes_in_block;
        *actual_unpack_bytes += bytes_in_block;
        blockid++;

        if (rem_unpack_bytes == 0) {
            /* if we are out of unpack buffer space, return */
            goto fn_exit;
        }
    }


    /* step 4: partial pack the next element */
    if (rem_unpack_bytes) {
        uintptr_t bytes_in_block;
        do {
            bytes_in_block = type->u.hindexed.array_of_blocklengths[blockid] *
                type->u.hindexed.child->size;
        } while (bytes_in_block == 0 && ++blockid);

        uintptr_t tmp_actual_unpack_bytes;

        dbuf = (char *) outbuf + type->u.hindexed.array_of_displs[blockid];
        rc = yaksi_iunpack(sbuf, rem_unpack_bytes, dbuf,
                           type->u.hindexed.array_of_blocklengths[blockid], type->u.hindexed.child,
                           remoffset, &tmp_actual_unpack_bytes, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        sbuf += tmp_actual_unpack_bytes;
        *actual_unpack_bytes += tmp_actual_unpack_bytes;

        blockid++;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_sub_struct(const void *inbuf, uintptr_t insize, void *outbuf, yaksi_type_s * type,
                             uintptr_t outoffset, uintptr_t * actual_unpack_bytes,
                             yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    /* make sure we have something left to unpack after the offset */
    assert(outoffset < type->size);

    const char *sbuf = (const char *) inbuf;
    char *dbuf;
    uintptr_t remoffset = outoffset;
    uintptr_t rem_unpack_bytes = YAKSU_MIN(insize, type->size - outoffset);
    uintptr_t blockid = 0;

    /* step 1: skip the first few blocks */
    if (remoffset) {
        for (int i = 0; i < type->u.str.count; i++) {
            uintptr_t bytes_in_block = type->u.str.array_of_blocklengths[i] *
                type->u.str.array_of_types[i]->size;

            if (remoffset < bytes_in_block) {
                break;
            } else {
                remoffset -= bytes_in_block;
                blockid++;
            }
        }
    }


    /* step 2: partial pack the next block */
    if (remoffset) {
        uintptr_t bytes_in_block;
        do {
            bytes_in_block = type->u.str.array_of_blocklengths[blockid] *
                type->u.str.array_of_types[blockid]->size;
        } while (bytes_in_block == 0 && ++blockid);
        assert(bytes_in_block > remoffset);

        uintptr_t tmp_unpack_bytes = YAKSU_MIN(rem_unpack_bytes, bytes_in_block - remoffset);
        uintptr_t tmp_actual_unpack_bytes;

        dbuf = (char *) outbuf + type->u.str.array_of_displs[blockid];
        rc = yaksi_iunpack(sbuf, tmp_unpack_bytes, dbuf,
                           type->u.str.array_of_blocklengths[blockid],
                           type->u.str.array_of_types[blockid], remoffset, &tmp_actual_unpack_bytes,
                           info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        sbuf += tmp_actual_unpack_bytes;
        rem_unpack_bytes -= tmp_actual_unpack_bytes;
        *actual_unpack_bytes += tmp_actual_unpack_bytes;

        if (rem_unpack_bytes == 0 ||
            tmp_actual_unpack_bytes < type->u.str.array_of_types[blockid]->size *
            type->u.str.array_of_blocklengths[blockid] - remoffset) {
            /* if we are out of unpack buffer space or if we could not
             * unpack fully, return */
            goto fn_exit;
        }

        remoffset = 0;
        blockid++;
    }


    /* step 3: perform a full pack of the next few elements */
    while (1) {
        uintptr_t bytes_in_block;
        do {
            bytes_in_block = type->u.str.array_of_blocklengths[blockid] *
                type->u.str.array_of_types[blockid]->size;
        } while (bytes_in_block == 0 && ++blockid);

        if (rem_unpack_bytes < bytes_in_block)
            break;

        dbuf = (char *) outbuf + type->u.str.array_of_displs[blockid];
        rc = yaksi_iunpack_backend(sbuf, dbuf, type->u.str.array_of_blocklengths[blockid],
                                   type->u.str.array_of_types[blockid], info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        sbuf += bytes_in_block;

        rem_unpack_bytes -= bytes_in_block;
        *actual_unpack_bytes += bytes_in_block;

        blockid++;

        if (rem_unpack_bytes == 0) {
            /* if we are out of unpack buffer space, return */
            goto fn_exit;
        }
    }


    /* step 4: partial pack the next element */
    if (rem_unpack_bytes) {
        uintptr_t bytes_in_block;
        do {
            bytes_in_block = type->u.str.array_of_blocklengths[blockid] *
                type->u.str.array_of_types[blockid]->size;
        } while (bytes_in_block == 0 && ++blockid);

        uintptr_t tmp_actual_unpack_bytes;

        dbuf = (char *) outbuf + type->u.str.array_of_displs[blockid];
        rc = yaksi_iunpack(sbuf, rem_unpack_bytes, dbuf,
                           type->u.str.array_of_blocklengths[blockid],
                           type->u.str.array_of_types[blockid], remoffset, &tmp_actual_unpack_bytes,
                           info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        sbuf += tmp_actual_unpack_bytes;
        *actual_unpack_bytes += tmp_actual_unpack_bytes;

        blockid++;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksi_iunpack_element(const void *inbuf, uintptr_t insize, void *outbuf, yaksi_type_s * type,
                          uintptr_t outoffset, uintptr_t * actual_unpack_bytes,
                          yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    *actual_unpack_bytes = 0;
    if (type->kind == YAKSI_TYPE_KIND__BUILTIN && insize < type->size) {
        goto fn_exit;
    }

    /* builtin types do not have any child elements */
    assert(type->kind != YAKSI_TYPE_KIND__BUILTIN);

    switch (type->kind) {
        case YAKSI_TYPE_KIND__HVECTOR:
            rc = unpack_sub_hvector(inbuf, insize, outbuf, type, outoffset, actual_unpack_bytes,
                                    info, op, request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            rc = unpack_sub_blkhindx(inbuf, insize, outbuf, type, outoffset, actual_unpack_bytes,
                                     info, op, request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            rc = unpack_sub_hindexed(inbuf, insize, outbuf, type, outoffset, actual_unpack_bytes,
                                     info, op, request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__STRUCT:
            rc = unpack_sub_struct(inbuf, insize, outbuf, type, outoffset, actual_unpack_bytes,
                                   info, op, request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = yaksi_iunpack_element(inbuf, insize, outbuf, type->u.resized.child, outoffset,
                                       actual_unpack_bytes, info, op, request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__CONTIG:
            rc = yaksi_iunpack(inbuf, insize, outbuf, type->u.contig.count,
                               type->u.contig.child, outoffset, actual_unpack_bytes, info, op,
                               request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__SUBARRAY:
            {
                yaksi_type_s *primary = type->u.subarray.primary;
                char *dbuf = (char *) outbuf + type->true_lb - primary->true_lb;
                rc = yaksi_iunpack_element(inbuf, insize, dbuf, primary, outoffset,
                                           actual_unpack_bytes, info, op, request);
                YAKSU_ERR_CHECK(rc, fn_fail);
                break;
            }

        default:
            assert(0);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
