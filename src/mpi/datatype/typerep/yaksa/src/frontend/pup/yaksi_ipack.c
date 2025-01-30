/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int yaksi_ipack(const void *inbuf, uintptr_t incount, yaksi_type_s * type, uintptr_t inoffset,
                void *outbuf, uintptr_t max_pack_bytes, uintptr_t * actual_pack_bytes,
                yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    *actual_pack_bytes = 0;

    /* We follow these steps:
     *
     *   1. Skip the first few elements till we reach an element that
     *   can contribute some data to our packing.
     *
     *   2. Partial pack the next element.  Once packed, if either the
     *   pack buffer is full or there's data left over in this
     *   element, return.
     *
     *   3. Perform a full pack of the next few elements.
     *
     *   4. Partial pack the next element.  Once packed, if either the
     *   pack buffer is full or there's data left over in this
     *   element, return.
     *
     * In the common case, we expect to execute only step 3.
     */

    const char *sbuf = (const char *) inbuf;
    char *dbuf = (char *) outbuf;
    uintptr_t remoffset = inoffset;
    uintptr_t rem_pack_bytes = YAKSU_MIN(max_pack_bytes, incount * type->size - inoffset);
    uintptr_t tmp_pack_bytes;

    /* step 1: skip the first few elements */
    if (remoffset) {
        uintptr_t skipelems = remoffset / type->size;

        remoffset %= type->size;
        sbuf += skipelems * type->extent;
    }


    /* step 2: partial pack the next element */
    if (remoffset) {
        assert(type->size > remoffset);

        rc = yaksi_ipack_element(sbuf, type, remoffset, dbuf, rem_pack_bytes, &tmp_pack_bytes,
                                 info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        *actual_pack_bytes += tmp_pack_bytes;
        rem_pack_bytes -= tmp_pack_bytes;

        if (rem_pack_bytes == 0) {
            /* if we are out of pack buffer space, return */
            goto fn_exit;
        } else if (tmp_pack_bytes < type->size - remoffset) {
            /* if we could not pack all of the data in the input type
             * for some reason, return */
            goto fn_exit;
        }

        remoffset = 0;
        sbuf += type->extent;
        dbuf += tmp_pack_bytes;
    }


    /* step 3: perform a full pack of the next few elements */
    uintptr_t numelems;
    numelems = rem_pack_bytes / type->size;
    if (numelems) {
        rc = yaksi_ipack_backend(sbuf, dbuf, numelems, type, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        *actual_pack_bytes += numelems * type->size;
        rem_pack_bytes -= numelems * type->size;

        sbuf += numelems * type->extent;
        dbuf += numelems * type->size;
    }


    /* step 4: partial pack the next element */
    if (rem_pack_bytes) {
        rc = yaksi_ipack_element(sbuf, type, remoffset, dbuf, rem_pack_bytes, &tmp_pack_bytes,
                                 info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);

        *actual_pack_bytes += tmp_pack_bytes;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
