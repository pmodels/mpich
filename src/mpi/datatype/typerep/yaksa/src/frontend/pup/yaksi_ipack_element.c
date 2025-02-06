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
 *  contribute some data to our packing.
 *
 *  2. Partial pack the next block.  Once packed, if either the pack
 *  buffer is full or there's data left over in this block, return.
 *
 *  3. Perform a full pack of the next few blocks.
 *
 *  4. Partial pack the next block.  Once packed, if either the pack
 *  buffer is full or there's data left over in this block, return.
 */

static int pack_sub_hvector(const void *inbuf, yaksi_type_s * type, uintptr_t inoffset,
                            void *outbuf, uintptr_t max_pack_bytes, uintptr_t * actual_pack_bytes,
                            yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    /* make sure we have something left to pack after the offset */
    assert(inoffset < type->size);

    const char *sbuf = (const char *) inbuf;
    char *dbuf = (char *) outbuf;
    uintptr_t remoffset = inoffset;
    uintptr_t bytes_in_block = type->size / type->u.hvector.count;
    uintptr_t rem_pack_bytes = YAKSU_MIN(max_pack_bytes, type->size - inoffset);
    uintptr_t tmp_pack_bytes;

    *actual_pack_bytes = 0;

    /* step 1: skip the first few blocks */
    if (remoffset) {
        uintptr_t skipblocks = remoffset / bytes_in_block;

        remoffset %= bytes_in_block;
        sbuf += skipblocks * type->u.hvector.stride;
    }


    /* step 2: partial pack the next block */
    if (remoffset) {
        assert(bytes_in_block > remoffset);

        rc = yaksi_ipack(sbuf, type->u.hvector.blocklength, type->u.hvector.child, remoffset,
                         dbuf, rem_pack_bytes, &tmp_pack_bytes, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        *actual_pack_bytes += tmp_pack_bytes;
        rem_pack_bytes -= tmp_pack_bytes;

        if (rem_pack_bytes == 0) {
            /* if we are out of pack buffer space, return */
            goto fn_exit;
        } else if (tmp_pack_bytes < bytes_in_block - remoffset) {
            /* if we could not pack all of the data in the input type
             * for some reason, return */
            goto fn_exit;
        }

        remoffset = 0;
        sbuf += type->u.hvector.stride;
        dbuf += tmp_pack_bytes;
    }


    /* step 3: perform a full pack of the next few blocks */
    uintptr_t numblocks;
    numblocks = rem_pack_bytes / bytes_in_block;
    for (int i = 0; i < numblocks; i++) {
        rc = yaksi_ipack_backend(sbuf, dbuf, type->u.hvector.blocklength, type->u.hvector.child,
                                 info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        *actual_pack_bytes += bytes_in_block;
        rem_pack_bytes -= bytes_in_block;

        sbuf += type->u.hvector.stride;
        dbuf += bytes_in_block;
    }


    /* step 4: partial pack the next block */
    if (rem_pack_bytes) {
        rc = yaksi_ipack(sbuf, type->u.hvector.blocklength, type->u.hvector.child, remoffset,
                         dbuf, rem_pack_bytes, &tmp_pack_bytes, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        *actual_pack_bytes += tmp_pack_bytes;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_sub_blkhindx(const void *inbuf, yaksi_type_s * type, uintptr_t inoffset,
                             void *outbuf, uintptr_t max_pack_bytes, uintptr_t * actual_pack_bytes,
                             yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    /* make sure we have something left to pack after the offset */
    assert(inoffset < type->size);

    uintptr_t remoffset = inoffset;
    uintptr_t bytes_in_block = type->size / type->u.blkhindx.count;
    uintptr_t rem_pack_bytes = YAKSU_MIN(max_pack_bytes, type->size - inoffset);
    uintptr_t tmp_pack_bytes;
    uintptr_t blockid = 0;
    const char *sbuf;
    char *dbuf = (char *) outbuf;

    *actual_pack_bytes = 0;

    /* step 1: skip the first few blocks */
    if (remoffset) {
        uintptr_t skipblocks = remoffset / bytes_in_block;

        remoffset %= bytes_in_block;
        blockid = skipblocks;
    }


    /* step 2: partial pack the next block */
    if (remoffset) {
        assert(type->u.blkhindx.blocklength * type->u.blkhindx.child->size > remoffset);

        sbuf = (const char *) inbuf + type->u.blkhindx.array_of_displs[blockid++];
        rc = yaksi_ipack(sbuf, type->u.blkhindx.blocklength, type->u.blkhindx.child, remoffset,
                         dbuf, rem_pack_bytes, &tmp_pack_bytes, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        dbuf += tmp_pack_bytes;

        *actual_pack_bytes += tmp_pack_bytes;
        rem_pack_bytes -= tmp_pack_bytes;

        if (rem_pack_bytes == 0) {
            /* if we are out of pack buffer space, return */
            goto fn_exit;
        } else if (tmp_pack_bytes < bytes_in_block - remoffset) {
            /* if we could not pack all of the data in the input type
             * for some reason, return */
            goto fn_exit;
        }

        remoffset = 0;
    }


    /* step 3: perform a full pack of the next few blocks */
    uintptr_t numblocks;
    numblocks = rem_pack_bytes / bytes_in_block;
    for (int i = 0; i < numblocks; i++) {
        sbuf = (const char *) inbuf + type->u.blkhindx.array_of_displs[blockid++];
        rc = yaksi_ipack_backend(sbuf, dbuf, type->u.blkhindx.blocklength, type->u.blkhindx.child,
                                 info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        dbuf += type->u.blkhindx.blocklength * type->u.blkhindx.child->size;

        *actual_pack_bytes += bytes_in_block;
        rem_pack_bytes -= bytes_in_block;
    }


    /* step 4: partial pack the next block */
    if (rem_pack_bytes) {
        sbuf = (const char *) inbuf + type->u.blkhindx.array_of_displs[blockid++];
        rc = yaksi_ipack(sbuf, type->u.blkhindx.blocklength, type->u.blkhindx.child, remoffset,
                         dbuf, rem_pack_bytes, &tmp_pack_bytes, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        dbuf += tmp_pack_bytes;

        *actual_pack_bytes += tmp_pack_bytes;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_sub_hindexed(const void *inbuf, yaksi_type_s * type, uintptr_t inoffset,
                             void *outbuf, uintptr_t max_pack_bytes, uintptr_t * actual_pack_bytes,
                             yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    /* make sure we have something left to pack after the offset */
    assert(inoffset < type->size);

    uintptr_t remoffset = inoffset;
    uintptr_t rem_pack_bytes = YAKSU_MIN(max_pack_bytes, type->size - inoffset);
    uintptr_t tmp_pack_bytes;
    uintptr_t blockid = 0;
    const char *sbuf;
    char *dbuf = (char *) outbuf;

    *actual_pack_bytes = 0;

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

        sbuf = (const char *) inbuf + type->u.hindexed.array_of_displs[blockid];
        rc = yaksi_ipack(sbuf, type->u.hindexed.array_of_blocklengths[blockid],
                         type->u.hindexed.child, remoffset, dbuf, rem_pack_bytes, &tmp_pack_bytes,
                         info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        dbuf += tmp_pack_bytes;

        *actual_pack_bytes += tmp_pack_bytes;
        rem_pack_bytes -= tmp_pack_bytes;

        if (rem_pack_bytes == 0) {
            /* if we are out of pack buffer space, return */
            goto fn_exit;
        } else if (tmp_pack_bytes < bytes_in_block - remoffset) {
            /* if we could not pack all of the data in the input type
             * for some reason, return */
            goto fn_exit;
        }

        remoffset = 0;
        blockid++;
    }


    /* step 3: perform a full pack of the next few blocks */
    while (1) {
        uintptr_t bytes_in_block;
        do {
            bytes_in_block = type->u.hindexed.array_of_blocklengths[blockid] *
                type->u.hindexed.child->size;
        } while (bytes_in_block == 0 && ++blockid);

        if (rem_pack_bytes < bytes_in_block)
            break;

        sbuf = (const char *) inbuf + type->u.hindexed.array_of_displs[blockid];
        rc = yaksi_ipack_backend(sbuf, dbuf, type->u.hindexed.array_of_blocklengths[blockid],
                                 type->u.hindexed.child, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        dbuf += bytes_in_block;

        *actual_pack_bytes += bytes_in_block;
        rem_pack_bytes -= bytes_in_block;
        blockid++;

        if (rem_pack_bytes == 0) {
            /* if we are out of pack buffer space, return */
            goto fn_exit;
        }
    }


    /* step 4: partial pack the next block */
    if (rem_pack_bytes) {
        uintptr_t bytes_in_block;
        do {
            bytes_in_block = type->u.hindexed.array_of_blocklengths[blockid] *
                type->u.hindexed.child->size;
        } while (bytes_in_block == 0 && ++blockid);

        sbuf = (const char *) inbuf + type->u.hindexed.array_of_displs[blockid];
        rc = yaksi_ipack(sbuf, type->u.hindexed.array_of_blocklengths[blockid],
                         type->u.hindexed.child, remoffset, dbuf, rem_pack_bytes, &tmp_pack_bytes,
                         info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        dbuf += tmp_pack_bytes;

        *actual_pack_bytes += tmp_pack_bytes;
        blockid++;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_sub_struct(const void *inbuf, yaksi_type_s * type, uintptr_t inoffset,
                           void *outbuf, uintptr_t max_pack_bytes, uintptr_t * actual_pack_bytes,
                           yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    /* make sure we have something left to pack after the offset */
    assert(inoffset < type->size);

    uintptr_t remoffset = inoffset;
    uintptr_t rem_pack_bytes = YAKSU_MIN(max_pack_bytes, type->size - inoffset);
    uintptr_t tmp_pack_bytes;
    uintptr_t blockid = 0;
    const char *sbuf;
    char *dbuf = (char *) outbuf;

    *actual_pack_bytes = 0;

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

        sbuf = (const char *) inbuf + type->u.str.array_of_displs[blockid];
        rc = yaksi_ipack(sbuf, type->u.str.array_of_blocklengths[blockid],
                         type->u.str.array_of_types[blockid], remoffset, dbuf, rem_pack_bytes,
                         &tmp_pack_bytes, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        dbuf += tmp_pack_bytes;

        *actual_pack_bytes += tmp_pack_bytes;
        rem_pack_bytes -= tmp_pack_bytes;

        if (rem_pack_bytes == 0) {
            /* if we are out of pack buffer space, return */
            goto fn_exit;
        } else if (tmp_pack_bytes < bytes_in_block - remoffset) {
            /* if we could not pack all of the data in the input type
             * for some reason, return */
            goto fn_exit;
        }

        remoffset = 0;
        blockid++;
    }


    /* step 3: perform a full pack of the next few blocks */
    while (1) {
        uintptr_t bytes_in_block;
        do {
            bytes_in_block = type->u.str.array_of_blocklengths[blockid] *
                type->u.str.array_of_types[blockid]->size;
        } while (bytes_in_block == 0 && ++blockid);

        if (rem_pack_bytes < bytes_in_block)
            break;

        sbuf = (const char *) inbuf + type->u.str.array_of_displs[blockid];
        rc = yaksi_ipack_backend(sbuf, dbuf, type->u.str.array_of_blocklengths[blockid],
                                 type->u.str.array_of_types[blockid], info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        dbuf += bytes_in_block;

        *actual_pack_bytes += bytes_in_block;
        rem_pack_bytes -= bytes_in_block;
        blockid++;

        if (rem_pack_bytes == 0) {
            /* if we are out of pack buffer space, return */
            goto fn_exit;
        }
    }


    /* step 4: partial pack the next block */
    if (rem_pack_bytes) {
        uintptr_t bytes_in_block;
        do {
            bytes_in_block = type->u.str.array_of_blocklengths[blockid] *
                type->u.str.array_of_types[blockid]->size;
        } while (bytes_in_block == 0 && ++blockid);

        sbuf = (const char *) inbuf + type->u.str.array_of_displs[blockid];
        rc = yaksi_ipack(sbuf, type->u.str.array_of_blocklengths[blockid],
                         type->u.str.array_of_types[blockid], remoffset, dbuf, rem_pack_bytes,
                         &tmp_pack_bytes, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
        dbuf += tmp_pack_bytes;

        *actual_pack_bytes += tmp_pack_bytes;
        blockid++;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksi_ipack_element(const void *inbuf, yaksi_type_s * type, uintptr_t inoffset,
                        void *outbuf, uintptr_t max_pack_bytes, uintptr_t * actual_pack_bytes,
                        yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    *actual_pack_bytes = 0;

    /* if we cannot fit at least one basic element, return */
    if (type->kind == YAKSI_TYPE_KIND__BUILTIN && max_pack_bytes < type->size)
        goto fn_exit;

    /* builtin types do not have any child elements */
    assert(type->kind != YAKSI_TYPE_KIND__BUILTIN);

    switch (type->kind) {
        case YAKSI_TYPE_KIND__HVECTOR:
            rc = pack_sub_hvector(inbuf, type, inoffset, outbuf, max_pack_bytes, actual_pack_bytes,
                                  info, op, request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            rc = pack_sub_blkhindx(inbuf, type, inoffset, outbuf, max_pack_bytes,
                                   actual_pack_bytes, info, op, request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            rc = pack_sub_hindexed(inbuf, type, inoffset, outbuf, max_pack_bytes,
                                   actual_pack_bytes, info, op, request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__STRUCT:
            rc = pack_sub_struct(inbuf, type, inoffset, outbuf, max_pack_bytes, actual_pack_bytes,
                                 info, op, request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = yaksi_ipack_element(inbuf, type->u.resized.child, inoffset, outbuf,
                                     max_pack_bytes, actual_pack_bytes, info, op, request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__CONTIG:
            rc = yaksi_ipack(inbuf, type->u.contig.count, type->u.contig.child, inoffset,
                             outbuf, max_pack_bytes, actual_pack_bytes, info, op, request);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__SUBARRAY:
            {
                yaksi_type_s *primary = type->u.subarray.primary;
                const char *sbuf = (const char *) inbuf + type->true_lb - primary->true_lb;
                rc = yaksi_ipack_element(sbuf, primary, inoffset, outbuf, max_pack_bytes,
                                         actual_pack_bytes, info, op, request);
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
