/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <assert.h>

/* The backends have no kernels for struct types, so a struct is packed through
 * the frontend with one call per field per element, which is very slow. We replace
 * the struct with an hindexed type over its contiguous pieces, resized to the
 * struct's lb and extent, so the backend kernels apply.
 *
 * A side effect: a struct of mixed types becomes hindexed bytes, so a partial pack
 * or unpack (e.g. a pipelined send) can stop at any chunk boundary rather than only
 * at an element boundary. */

#define FLATTEN_MAX_PIECES (1024)

/* Builds an hindexed type over the struct's contiguous pieces and returns it in
 * *flat. Returns *flat = NULL when it is not suitable:
 *   - the leaf is a pair type
 *   - there are no pieces or more than FLATTEN_MAX_PIECES pieces
 *   - it breaks alignment
 */
static int flatten_struct(yaksi_type_s * type, yaksi_type_s ** flat);

int yaksi_type_create_struct(intptr_t count, const intptr_t * array_of_blocklengths,
                             const intptr_t * array_of_displs, yaksi_type_s ** array_of_intypes,
                             yaksi_type_s ** newtype)
{
    int rc = YAKSA_SUCCESS;

    /* shortcut for hindexed types */
    bool is_hindexed = true;
    for (intptr_t i = 1; i < count; i++) {
        if (array_of_intypes[i] != array_of_intypes[i - 1])
            is_hindexed = false;
    }
    if (is_hindexed) {
        rc = yaksi_type_create_hindexed(count, array_of_blocklengths, array_of_displs,
                                        array_of_intypes[0], newtype);
        YAKSU_ERR_CHECK(rc, fn_fail);
        goto fn_exit;
    }

    /* regular struct type */
    yaksi_type_s *outtype;
    outtype = (yaksi_type_s *) malloc(sizeof(yaksi_type_s));
    YAKSU_ERR_CHKANDJUMP(!outtype, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);
    yaksu_atomic_store(&outtype->refcount, 1);

    outtype->kind = YAKSI_TYPE_KIND__STRUCT;

    outtype->size = 0;
    for (intptr_t i = 0; i < count; i++) {
        outtype->size += array_of_intypes[i]->size * array_of_blocklengths[i];
        yaksu_atomic_incr(&array_of_intypes[i]->refcount);
    }

    int is_set;
    intptr_t last_ub;
    int is_noncontig;
    is_set = 0;
    last_ub = 0;
    is_noncontig = 0;
    outtype->alignment = 0;
    for (intptr_t idx = 0; idx < count; idx++) {
        if (array_of_blocklengths[idx] == 0)
            continue;

        intptr_t lb, ub;
        if (array_of_intypes[idx]->extent > 0) {
            lb = array_of_displs[idx] + array_of_intypes[idx]->lb;
            ub = array_of_displs[idx] + array_of_intypes[idx]->ub +
                array_of_intypes[idx]->extent * (array_of_blocklengths[idx] - 1);
        } else {
            lb = array_of_displs[idx] + array_of_intypes[idx]->lb +
                array_of_intypes[idx]->extent * (array_of_blocklengths[idx] - 1);
            ub = array_of_displs[idx] + array_of_intypes[idx]->ub;
        }

        intptr_t true_lb = lb - array_of_intypes[idx]->lb + array_of_intypes[idx]->true_lb;
        intptr_t true_ub = ub - array_of_intypes[idx]->ub + array_of_intypes[idx]->true_ub;

        if (idx > 0 && true_lb != last_ub) {
            is_noncontig = 1;
        }
        last_ub = true_ub;

        int tree_depth = array_of_intypes[idx]->tree_depth;
        if (outtype->alignment < array_of_intypes[idx]->alignment)
            outtype->alignment = array_of_intypes[idx]->alignment;

        if (is_set) {
            outtype->lb = YAKSU_MIN(lb, outtype->lb);
            outtype->true_lb = YAKSU_MIN(true_lb, outtype->true_lb);
            outtype->ub = YAKSU_MAX(ub, outtype->ub);
            outtype->true_ub = YAKSU_MAX(true_ub, outtype->true_ub);
            outtype->tree_depth = YAKSU_MAX(tree_depth, outtype->tree_depth);
        } else {
            outtype->lb = lb;
            outtype->true_lb = true_lb;
            outtype->ub = ub;
            outtype->true_ub = true_ub;
            outtype->tree_depth = tree_depth;
        }

        is_set = 1;
    }

    /* adjust ub based on alignment */
    assert(outtype->alignment);
    uintptr_t diff;
    diff = (outtype->ub - outtype->lb) % outtype->alignment;
    if (diff) {
        outtype->ub += outtype->alignment - diff;
    }

    outtype->tree_depth++;
    outtype->extent = outtype->ub - outtype->lb;

    /* detect if the outtype is contiguous */
    if (!is_noncontig && (outtype->ub - outtype->lb) == outtype->size) {
        outtype->is_contig = true;

        for (intptr_t i = 0; i < count; i++) {
            if (array_of_intypes[i]->is_contig == false) {
                outtype->is_contig = false;
                break;
            }
        }
    } else {
        outtype->is_contig = false;
    }

    if (outtype->is_contig && outtype->ub == outtype->extent) {
        outtype->num_contig = 1;
    } else {
        outtype->num_contig = 0;
        for (intptr_t i = 0; i < count; i++) {
            if (array_of_intypes[i]->is_contig && array_of_blocklengths[i]) {
                outtype->num_contig++;
            } else {
                outtype->num_contig += array_of_intypes[i]->num_contig * array_of_blocklengths[i];
            }
        }
    }

    outtype->u.str.count = count;
    outtype->u.str.array_of_blocklengths = (intptr_t *) malloc(count * sizeof(intptr_t));
    outtype->u.str.array_of_displs = (intptr_t *) malloc(count * sizeof(intptr_t));
    outtype->u.str.array_of_types = (yaksi_type_s **) malloc(count * sizeof(yaksi_type_s *));
    for (intptr_t i = 0; i < count; i++) {
        outtype->u.str.array_of_blocklengths[i] = array_of_blocklengths[i];
        outtype->u.str.array_of_displs[i] = array_of_displs[i];
        outtype->u.str.array_of_types[i] = array_of_intypes[i];
    }

    rc = yaksur_type_create_hook(outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    yaksi_type_s *flat;
    rc = flatten_struct(outtype, &flat);
    YAKSU_ERR_CHECK(rc, fn_fail);
    if (flat) {
        rc = yaksi_type_free(outtype);
        YAKSU_ERR_CHECK(rc, fn_fail);
        outtype = flat;
    }
    *newtype = outtype;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static void find_leaf_type(yaksi_type_s * type, yaksi_type_s ** leaf, bool * is_mixed);

static int flatten_struct(yaksi_type_s * type, yaksi_type_s ** flat)
{
    int rc = YAKSA_SUCCESS;
    struct iovec *iov = NULL;
    intptr_t *blocklens = NULL;
    intptr_t *displs = NULL;

    *flat = NULL;

    yaksi_type_s *leaf = NULL;
    bool is_mixed = false;
    find_leaf_type(type, &leaf, &is_mixed);
    assert(leaf);

    yaksi_type_s *child;
    if (is_mixed) {
        rc = yaksi_type_get(YAKSA_TYPE__BYTE, &child);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else if (leaf->is_contig) {
        child = leaf;
    } else {
        goto fn_exit;
    }

    uintptr_t iov_len;
    rc = yaksi_iov_len(1, type, &iov_len);
    YAKSU_ERR_CHECK(rc, fn_fail);
    if (iov_len == 0 || iov_len > FLATTEN_MAX_PIECES) {
        goto fn_exit;
    }

    iov = (struct iovec *) malloc(iov_len * sizeof(struct iovec));
    blocklens = (intptr_t *) malloc(iov_len * sizeof(intptr_t));
    displs = (intptr_t *) malloc(iov_len * sizeof(intptr_t));
    YAKSU_ERR_CHKANDJUMP(!iov || !blocklens || !displs, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);

    uintptr_t actual_iov_len;
    rc = yaksi_iov(NULL, 1, type, 0, iov, iov_len, &actual_iov_len);
    YAKSU_ERR_CHECK(rc, fn_fail);

    /* pieces in type-map order, adjacent ones merged */
    intptr_t num_blocks = 0;
    for (uintptr_t i = 0; i < actual_iov_len; i++) {
        intptr_t disp = (intptr_t) iov[i].iov_base;
        intptr_t len = (intptr_t) iov[i].iov_len;
        if (num_blocks > 0 && displs[num_blocks - 1] + blocklens[num_blocks - 1] == disp) {
            blocklens[num_blocks - 1] += len;
        } else {
            displs[num_blocks] = disp;
            blocklens[num_blocks] = len;
            num_blocks++;
        }
    }
    for (intptr_t i = 0; i < num_blocks; i++) {
        assert(blocklens[i] % child->size == 0);
        blocklens[i] /= child->size;
    }

    yaksi_type_s *hindexed;
    rc = yaksi_type_create_hindexed(num_blocks, blocklens, displs, child, &hindexed);
    YAKSU_ERR_CHECK(rc, fn_fail);

    yaksi_type_s *outtype;
    rc = yaksi_type_create_resized(hindexed, type->lb, type->extent, &outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);
    rc = yaksi_type_free(hindexed);
    YAKSU_ERR_CHECK(rc, fn_fail);

    assert(outtype->size == type->size);

    /* The struct's true bounds include zero-size members, which the iov omits.
     * Keep the struct if the flattened type's true bounds differ. */
    if (outtype->true_lb != type->true_lb || outtype->true_ub != type->true_ub) {
        rc = yaksi_type_free(outtype);
        YAKSU_ERR_CHECK(rc, fn_fail);
        goto fn_exit;
    }

    /* The alignment pads the extent of an enclosing struct, so keep the struct's.
     * Only set it on an object we own; a dup of an existing type is shared. */
    if (outtype->alignment != type->alignment) {
        if (yaksu_atomic_load(&outtype->refcount) > 1) {
            rc = yaksi_type_free(outtype);
            YAKSU_ERR_CHECK(rc, fn_fail);
            goto fn_exit;
        }
        outtype->alignment = type->alignment;
    }

    *flat = outtype;

  fn_exit:
    free(iov);
    free(blocklens);
    free(displs);
    return rc;
  fn_fail:
    goto fn_exit;
}

/* Recursively descends the type tree to find its leaf (builtin) type in *leaf;
 * sets *is_mixed if more than one distinct builtin type is found */
static void find_leaf_type(yaksi_type_s * type, yaksi_type_s ** leaf, bool * is_mixed)
{
    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            if (*leaf == NULL) {
                *leaf = type;
            } else if (*leaf != type) {
                *is_mixed = true;
            }
            break;
        case YAKSI_TYPE_KIND__CONTIG:
            find_leaf_type(type->u.contig.child, leaf, is_mixed);
            break;
        case YAKSI_TYPE_KIND__DUP:
            find_leaf_type(type->u.dup.child, leaf, is_mixed);
            break;
        case YAKSI_TYPE_KIND__RESIZED:
            find_leaf_type(type->u.resized.child, leaf, is_mixed);
            break;
        case YAKSI_TYPE_KIND__HVECTOR:
            find_leaf_type(type->u.hvector.child, leaf, is_mixed);
            break;
        case YAKSI_TYPE_KIND__BLKHINDX:
            find_leaf_type(type->u.blkhindx.child, leaf, is_mixed);
            break;
        case YAKSI_TYPE_KIND__HINDEXED:
            find_leaf_type(type->u.hindexed.child, leaf, is_mixed);
            break;
        case YAKSI_TYPE_KIND__STRUCT:
            for (intptr_t i = 0; i < type->u.str.count; i++) {
                if (type->u.str.array_of_blocklengths[i] > 0) {
                    find_leaf_type(type->u.str.array_of_types[i], leaf, is_mixed);
                }
            }
            break;
        case YAKSI_TYPE_KIND__SUBARRAY:
            find_leaf_type(type->u.subarray.primary, leaf, is_mixed);
            break;
        default:
            assert(0);
    }
}

YAKSA_API_PUBLIC int yaksa_type_create_struct(intptr_t count,
                                              const intptr_t * array_of_blocklengths,
                                              const intptr_t * array_of_displs,
                                              const yaksa_type_t * array_of_types,
                                              yaksa_info_t info, yaksa_type_t * newtype)
{
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    uintptr_t total_size;
    total_size = 0;
    for (intptr_t i = 0; i < count; i++) {
        yaksi_type_s *type;
        rc = yaksi_type_get(array_of_types[i], &type);
        YAKSU_ERR_CHECK(rc, fn_fail);

        total_size += type->size * array_of_blocklengths[i];
    }
    if (total_size == 0) {
        *newtype = YAKSA_TYPE__NULL;
        goto fn_exit;
    }

    assert(count > 0);
    yaksi_type_s **array_of_intypes;
    array_of_intypes = (yaksi_type_s **) malloc(count * sizeof(yaksi_type_s *));

    for (intptr_t i = 0; i < count; i++) {
        rc = yaksi_type_get(array_of_types[i], &array_of_intypes[i]);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    yaksi_type_s *outtype;
    rc = yaksi_type_create_struct(count, array_of_blocklengths, array_of_displs, array_of_intypes,
                                  &outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksi_type_handle_alloc(outtype, newtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    free(array_of_intypes);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
