/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* yaksa_iov_len_max is a slow version of yaksa_iov_len.
 * It has the same semantics of MPIR_Typerep_iov_len in MPICH, with a max_iov_bytes
 */

/* Recursively find number of segments within a type. Assume -
 *     1. max_iov_bytes < type->size
 *     2. iov_len contains previous count, do not reset
 */
static int yaksi_iov_len_sub(yaksi_type_s * type, uintptr_t * rem_iov_bytes, uintptr_t * iov_len)
{
    int rc = YAKSA_SUCCESS;

    if (type->num_contig == 1 || type->kind == YAKSI_TYPE_KIND__BUILTIN) {
        /* we always "pack" full elements and full segments into the IOV, never partial */
        goto fn_exit;
    }

    switch (type->kind) {
        case YAKSI_TYPE_KIND__CONTIG:
        case YAKSI_TYPE_KIND__HVECTOR:
        case YAKSI_TYPE_KIND__BLKHINDX:
        case YAKSI_TYPE_KIND__HINDEXED:
            /* all types with multiple count of single child type */
            {
                yaksi_type_s *child_type;
                if (type->kind == YAKSI_TYPE_KIND__CONTIG) {
                    child_type = type->u.contig.child;
                } else if (type->kind == YAKSI_TYPE_KIND__HVECTOR) {
                    child_type = type->u.hvector.child;
                } else if (type->kind == YAKSI_TYPE_KIND__BLKHINDX) {
                    child_type = type->u.blkhindx.child;
                } else {        /* YAKSI_TYPE_KIND__HINDEXED */
                    child_type = type->u.hindexed.child;
                }

                if (child_type->is_contig) {
                    switch (type->kind) {
                        case YAKSI_TYPE_KIND__HVECTOR:
                        case YAKSI_TYPE_KIND__BLKHINDX:
                            {
                                uintptr_t num_blks;
                                if (type->kind == YAKSI_TYPE_KIND__HVECTOR) {
                                    num_blks = child_type->u.hvector.blocklength;
                                } else {
                                    num_blks = child_type->u.blkhindx.blocklength;
                                }
                                uintptr_t sub_size = child_type->size * num_blks;
                                uintptr_t n = (*rem_iov_bytes) / sub_size;
                                *rem_iov_bytes -= n * sub_size;
                                *iov_len += n;
                            }
                            break;
                        case YAKSI_TYPE_KIND__HINDEXED:
                            for (int i = 0; i < type->u.hindexed.count; i++) {
                                uintptr_t sub_size = child_type->size *
                                    child_type->u.hindexed.array_of_blocklengths[i];
                                if (*rem_iov_bytes < sub_size) {
                                    break;
                                }
                                *rem_iov_bytes -= sub_size;
                                *iov_len += 1;
                            }
                            break;
                        default:
                            assert(0);
                    }
                } else {
                    /* take out the integral subtypes, then recurse */
                    uintptr_t child_size = child_type->size;
                    uintptr_t child_num_contig = child_type->num_contig;;

                    uintptr_t n;
                    n = (*rem_iov_bytes) / child_size;
                    *rem_iov_bytes -= n * child_size;
                    *iov_len += n * child_num_contig;

                    rc = yaksi_iov_len_sub(child_type, rem_iov_bytes, iov_len);
                }
            }
            break;

        case YAKSI_TYPE_KIND__STRUCT:
            for (int i = 0; i < type->u.str.count; i++) {
                yaksi_type_s *child_type = type->u.str.array_of_types[i];
                uintptr_t n = type->u.str.array_of_blocklengths[i];
                uintptr_t child_size = child_type->size;

                if (child_type->is_contig) {
                    if (*rem_iov_bytes >= n * child_size) {
                        *rem_iov_bytes -= n * child_size;
                        *iov_len += 1;
                    } else {
                        break;
                    }
                } else {
                    uintptr_t child_num_contig = child_type->num_contig;;
                    if (*rem_iov_bytes >= n * child_size) {
                        *rem_iov_bytes -= n * child_size;
                        *iov_len += n * child_num_contig;
                    } else {
                        n = (*rem_iov_bytes) / child_size;
                        *rem_iov_bytes -= n * child_size;
                        *iov_len += n * child_num_contig;
                        rc = yaksi_iov_len_sub(child_type, rem_iov_bytes, iov_len);
                        goto fn_exit;
                    }
                }
            }
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = yaksi_iov_len_sub(type->u.resized.child, rem_iov_bytes, iov_len);
            goto fn_exit;

        case YAKSI_TYPE_KIND__DUP:
            rc = yaksi_iov_len_sub(type->u.dup.child, rem_iov_bytes, iov_len);
            goto fn_exit;

        case YAKSI_TYPE_KIND__SUBARRAY:
            rc = yaksi_iov_len_sub(type->u.subarray.primary, rem_iov_bytes, iov_len);
            goto fn_exit;

        default:
            printf("    kind = %d\n", type->kind);
            assert(0);
    }

  fn_exit:
    return rc;
}

/* internal version of yaksa_iov_len_max */
static int yaksi_iov_len_max(uintptr_t count, yaksi_type_s * type, uintptr_t max_iov_bytes,
                             uintptr_t * iov_len, uintptr_t * actual_iov_bytes)
{
    int rc = YAKSA_SUCCESS;

    /* contig case is trivial */
    if (type->is_contig) {
        if (max_iov_bytes >= count * type->size) {
            *iov_len = 1;
            *actual_iov_bytes = count * type->size;
        } else {
            *iov_len = 0;
            *actual_iov_bytes = 0;
        }
        goto fn_exit;
    }

    uintptr_t num_contig;
    rc = yaksi_iov_len(1, type, &num_contig);
    YAKSU_ERR_CHECK(rc, fn_fail);

    /* fast path */
    if (max_iov_bytes >= count * type->size) {
        *iov_len = count * num_contig;
        *actual_iov_bytes = count * type->size;
        goto fn_exit;
    }

    uintptr_t rem_bytes;
    rem_bytes = max_iov_bytes;
    *iov_len = 0;

    uintptr_t n;
    n = rem_bytes / type->size;
    rem_bytes -= n * type->size;
    *iov_len += n * num_contig;

    rc = yaksi_iov_len_sub(type, &rem_bytes, iov_len);
    YAKSU_ERR_CHECK(rc, fn_fail);

    *actual_iov_bytes = max_iov_bytes - rem_bytes;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_iov_len_max(uintptr_t count, yaksa_type_t type, uintptr_t max_iov_bytes,
                                       uintptr_t * iov_len, uintptr_t * actual_iov_bytes)
{
    yaksi_type_s *yaksi_type;
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    rc = yaksi_type_get(type, &yaksi_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksi_iov_len_max(count, yaksi_type, max_iov_bytes, iov_len, actual_iov_bytes);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
