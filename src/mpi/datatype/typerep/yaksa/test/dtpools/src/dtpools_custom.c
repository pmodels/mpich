#include <string.h>
/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include "dtpools_internal.h"

#define SKIP_SPACE while (isspace(*s)) s++
#define EXPECT_DIGIT \
    do { \
        while (isspace(*s)) s++; \
        if (!isdigit(*s)) { \
            DTPI_ERR_ASSERT(0, rc); \
        } \
    } while (0)
#define EXPECT_ALPHA \
    do { \
        while (isspace(*s)) s++; \
        if (!isalpha(*s)) { \
            DTPI_ERR_ASSERT(0, rc); \
        } \
    } while (0)
#define EXPECT_CHAR(c) \
    do { \
        while (isspace(*s)) s++; \
        if (*s != (c)) { \
            DTPI_ERR_ASSERT(0, rc); \
        } \
    } while (0)
#define SKIP_DIGIT \
    do { \
        while (*s && isdigit(*s)) s++; \
        DTPI_ERR_ASSERT(*s, rc); \
    } while (0)
#define SKIP_ALPHA \
    do { \
        while (*s && isalpha(*s)) s++; \
        DTPI_ERR_ASSERT(*s, rc); \
    } while (0)
#define SKIP_UNTIL(c) \
    do { \
        while (*s && *s != (c)) s++; \
        DTPI_ERR_ASSERT(*s, rc); \
    } while (0)

int DTPI_parse_desc(const char *s, const char **parts, int *num_parts, int max_parts)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    int i = 0;
    while (*s) {
        EXPECT_DIGIT;
        SKIP_DIGIT;
        EXPECT_CHAR(':');
        s++;
        EXPECT_ALPHA;

        parts[i++] = s;
        DTPI_ERR_ASSERT(i < max_parts, rc);

        SKIP_ALPHA;
        EXPECT_CHAR('[');
        SKIP_UNTIL(']');
        s++;
        SKIP_SPACE;
    }
    *num_parts = i;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int custom_contig(DTP_pool_s dtp, DTPI_Attr_s * attr, yaksa_type_t * newtype,
                         intptr_t * new_count, const char *desc, yaksa_type_t type, intptr_t count)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    attr->kind = DTPI_DATATYPE_KIND__CONTIG;
    intptr_t tmp_lb;
    rc = yaksa_type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_RC(rc);
    const char *s = desc;
    EXPECT_CHAR('[');
    s++;

    int n;
    n = sscanf(s, "blklen %d", &attr->u.contig.blklen);
    DTPI_ERR_ASSERT(n == 1, rc);
    rc = yaksa_type_create_contig(attr->u.contig.blklen, type, NULL, newtype);
    DTPI_ERR_CHK_RC(rc);
    DTPI_ERR_ASSERT(count % attr->u.contig.blklen == 0, rc);
    count /= attr->u.contig.blklen;

    *new_count = count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int custom_dup(DTP_pool_s dtp, DTPI_Attr_s * attr, yaksa_type_t * newtype,
                      intptr_t * new_count, const char *desc, yaksa_type_t type, intptr_t count)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    attr->kind = DTPI_DATATYPE_KIND__DUP;
    intptr_t tmp_lb;
    rc = yaksa_type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_RC(rc);
    const char *s = desc;
    EXPECT_CHAR('[');
    s++;

    rc = yaksa_type_create_dup(type, NULL, newtype);
    DTPI_ERR_CHK_RC(rc);

    *new_count = count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int custom_resized(DTP_pool_s dtp, DTPI_Attr_s * attr, yaksa_type_t * newtype,
                          intptr_t * new_count, const char *desc, yaksa_type_t type, intptr_t count)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    attr->kind = DTPI_DATATYPE_KIND__RESIZED;
    intptr_t tmp_lb;
    rc = yaksa_type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_RC(rc);
    const char *s = desc;
    EXPECT_CHAR('[');
    s++;

    int n;
    n = sscanf(s, "lb %zd, extent %zd", &attr->u.resized.lb, &attr->u.resized.extent);
    DTPI_ERR_ASSERT(n == 2, rc);
    rc = yaksa_type_create_resized(type, attr->u.resized.lb, attr->u.resized.extent, NULL, newtype);
    DTPI_ERR_CHK_RC(rc);

    *new_count = count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int custom_vector(DTP_pool_s dtp, DTPI_Attr_s * attr, yaksa_type_t * newtype,
                         intptr_t * new_count, const char *desc, yaksa_type_t type, intptr_t count)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    attr->kind = DTPI_DATATYPE_KIND__VECTOR;
    intptr_t tmp_lb;
    rc = yaksa_type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_RC(rc);
    const char *s = desc;
    EXPECT_CHAR('[');
    s++;

    int n;
    n = sscanf(s, "numblks %d, blklen %d, stride %d", &attr->u.vector.numblks,
               &attr->u.vector.blklen, &attr->u.vector.stride);
    DTPI_ERR_ASSERT(n == 3, rc);
    rc = yaksa_type_create_vector(attr->u.vector.numblks, attr->u.vector.blklen,
                                  attr->u.vector.stride, type, NULL, newtype);
    DTPI_ERR_CHK_RC(rc);
    DTPI_ERR_ASSERT(count % (attr->u.vector.numblks * attr->u.vector.blklen) == 0, rc);
    count /= attr->u.vector.numblks * attr->u.vector.blklen;

    *new_count = count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int custom_hvector(DTP_pool_s dtp, DTPI_Attr_s * attr, yaksa_type_t * newtype,
                          intptr_t * new_count, const char *desc, yaksa_type_t type, intptr_t count)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    attr->kind = DTPI_DATATYPE_KIND__HVECTOR;
    intptr_t tmp_lb;
    rc = yaksa_type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_RC(rc);
    const char *s = desc;
    EXPECT_CHAR('[');
    s++;

    int n;
    n = sscanf(s, "numblks %d, blklen %ld, stride %zd", &attr->u.hvector.numblks,
               &attr->u.hvector.blklen, &attr->u.hvector.stride);
    DTPI_ERR_ASSERT(n == 3, rc);
    rc = yaksa_type_create_hvector(attr->u.hvector.numblks, attr->u.hvector.blklen,
                                   attr->u.hvector.stride, type, NULL, newtype);
    DTPI_ERR_CHK_RC(rc);
    printf("hvector: count=%ld, numblks=%d, blklen=%ld\n", count, attr->u.hvector.numblks,
           attr->u.hvector.blklen);
    DTPI_ERR_ASSERT(count % (attr->u.hvector.numblks * attr->u.hvector.blklen) == 0, rc);
    count /= attr->u.hvector.numblks * attr->u.hvector.blklen;

    *new_count = count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int custom_blkindx(DTP_pool_s dtp, DTPI_Attr_s * attr, yaksa_type_t * newtype,
                          intptr_t * new_count, const char *desc, yaksa_type_t type, intptr_t count)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    attr->kind = DTPI_DATATYPE_KIND__BLKINDX;
    intptr_t tmp_lb;
    rc = yaksa_type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_RC(rc);
    const char *s = desc;
    EXPECT_CHAR('[');
    s++;

    int n;
    n = sscanf(s, "numblks %d, blklen %ld", &attr->u.blkindx.numblks, &attr->u.blkindx.blklen);
    DTPI_ERR_ASSERT(n == 2, rc);
    DTPI_ALLOC_OR_FAIL(attr->u.blkindx.array_of_displs, attr->u.blkindx.numblks * sizeof(intptr_t),
                       rc);
    SKIP_UNTIL('(');
    s++;
    for (int i = 0; i < attr->u.blkindx.numblks; i++) {
        if (i > 0) {
            EXPECT_CHAR(',');
            s++;
        }
        EXPECT_DIGIT;
        attr->u.blkindx.array_of_displs[i] = atoi(s);
        SKIP_DIGIT;
    }

    rc = yaksa_type_create_indexed_block(attr->u.blkindx.numblks, attr->u.blkindx.blklen,
                                         attr->u.blkindx.array_of_displs, type, NULL, newtype);
    DTPI_ERR_CHK_RC(rc);
    DTPI_ERR_ASSERT(count % (attr->u.blkindx.numblks * attr->u.blkindx.blklen) == 0, rc);
    count /= attr->u.blkindx.numblks * attr->u.blkindx.blklen;

    *new_count = count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int custom_blkhindx(DTP_pool_s dtp, DTPI_Attr_s * attr, yaksa_type_t * newtype,
                           intptr_t * new_count, const char *desc, yaksa_type_t type,
                           intptr_t count)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    attr->kind = DTPI_DATATYPE_KIND__BLKHINDX;
    intptr_t tmp_lb;
    rc = yaksa_type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_RC(rc);
    const char *s = desc;
    EXPECT_CHAR('[');
    s++;

    int n;
    n = sscanf(s, "numblks %d, blklen %ld", &attr->u.blkhindx.numblks, &attr->u.blkhindx.blklen);
    DTPI_ERR_ASSERT(n == 2, rc);
    DTPI_ALLOC_OR_FAIL(attr->u.blkhindx.array_of_displs,
                       attr->u.blkhindx.numblks * sizeof(intptr_t), rc);
    SKIP_UNTIL('(');
    s++;
    for (int i = 0; i < attr->u.blkhindx.numblks; i++) {
        if (i > 0) {
            EXPECT_CHAR(',');
            s++;
        }
        EXPECT_DIGIT;
        attr->u.blkhindx.array_of_displs[i] = atoi(s);
        SKIP_DIGIT;
    }

    rc = yaksa_type_create_hindexed_block(attr->u.blkhindx.numblks, attr->u.blkhindx.blklen,
                                          attr->u.blkhindx.array_of_displs, type, NULL, newtype);
    DTPI_ERR_CHK_RC(rc);
    DTPI_ERR_ASSERT(count % (attr->u.blkhindx.numblks * attr->u.blkhindx.blklen) == 0, rc);
    count /= attr->u.blkhindx.numblks * attr->u.blkhindx.blklen;

    *new_count = count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int custom_indexed(DTP_pool_s dtp, DTPI_Attr_s * attr, yaksa_type_t * newtype,
                          intptr_t * new_count, const char *desc, yaksa_type_t type, intptr_t count)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    attr->kind = DTPI_DATATYPE_KIND__INDEXED;
    intptr_t tmp_lb;
    rc = yaksa_type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_RC(rc);
    const char *s = desc;
    EXPECT_CHAR('[');
    s++;

    int n;
    n = sscanf(s, "numblks %d", &attr->u.indexed.numblks);
    DTPI_ERR_ASSERT(n == 1, rc);
    DTPI_ALLOC_OR_FAIL(attr->u.indexed.array_of_blklens, attr->u.indexed.numblks * sizeof(intptr_t),
                       rc);
    SKIP_UNTIL('(');
    s++;
    for (int i = 0; i < attr->u.indexed.numblks; i++) {
        if (i > 0) {
            EXPECT_CHAR(',');
            s++;
        }
        EXPECT_DIGIT;
        attr->u.indexed.array_of_blklens[i] = atoi(s);
        SKIP_DIGIT;
    }
    DTPI_ALLOC_OR_FAIL(attr->u.indexed.array_of_displs, attr->u.indexed.numblks * sizeof(intptr_t),
                       rc);
    SKIP_UNTIL('(');
    s++;
    for (int i = 0; i < attr->u.indexed.numblks; i++) {
        if (i > 0) {
            EXPECT_CHAR(',');
            s++;
        }
        EXPECT_DIGIT;
        attr->u.indexed.array_of_displs[i] = atoi(s);
        SKIP_DIGIT;
    }

    rc = yaksa_type_create_indexed(attr->u.indexed.numblks, attr->u.indexed.array_of_blklens,
                                   attr->u.indexed.array_of_displs, type, NULL, newtype);
    DTPI_ERR_CHK_RC(rc);

    int total_blklen = 0;
    for (int i = 0; i < attr->u.indexed.numblks; i++) {
        total_blklen += attr->u.indexed.array_of_blklens[i];
    }
    DTPI_ERR_ASSERT(count % total_blklen == 0, rc);
    count /= total_blklen;

    *new_count = count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int custom_hindexed(DTP_pool_s dtp, DTPI_Attr_s * attr, yaksa_type_t * newtype,
                           intptr_t * new_count, const char *desc, yaksa_type_t type,
                           intptr_t count)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    attr->kind = DTPI_DATATYPE_KIND__HINDEXED;
    intptr_t tmp_lb;
    rc = yaksa_type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_RC(rc);
    const char *s = desc;
    EXPECT_CHAR('[');
    s++;

    int n;
    n = sscanf(s, "numblks %d", &attr->u.hindexed.numblks);
    DTPI_ERR_ASSERT(n == 1, rc);
    DTPI_ALLOC_OR_FAIL(attr->u.hindexed.array_of_blklens,
                       attr->u.hindexed.numblks * sizeof(intptr_t), rc);
    SKIP_UNTIL('(');
    s++;
    for (int i = 0; i < attr->u.hindexed.numblks; i++) {
        if (i > 0) {
            EXPECT_CHAR(',');
            s++;
        }
        EXPECT_DIGIT;
        attr->u.hindexed.array_of_blklens[i] = atoi(s);
        SKIP_DIGIT;
    }
    DTPI_ALLOC_OR_FAIL(attr->u.hindexed.array_of_displs,
                       attr->u.hindexed.numblks * sizeof(intptr_t), rc);
    SKIP_UNTIL('(');
    s++;
    for (int i = 0; i < attr->u.hindexed.numblks; i++) {
        if (i > 0) {
            EXPECT_CHAR(',');
            s++;
        }
        EXPECT_DIGIT;
        attr->u.hindexed.array_of_displs[i] = atoi(s);
        SKIP_DIGIT;
    }

    rc = yaksa_type_create_hindexed(attr->u.hindexed.numblks, attr->u.hindexed.array_of_blklens,
                                    attr->u.hindexed.array_of_displs, type, NULL, newtype);
    DTPI_ERR_CHK_RC(rc);

    int total_blklen = 0;
    for (int i = 0; i < attr->u.hindexed.numblks; i++) {
        total_blklen += attr->u.hindexed.array_of_blklens[i];
    }
    DTPI_ERR_ASSERT(count % total_blklen == 0, rc);
    count /= total_blklen;

    *new_count = count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int custom_subarray(DTP_pool_s dtp, DTPI_Attr_s * attr, yaksa_type_t * newtype,
                           intptr_t * new_count, const char *desc, yaksa_type_t type,
                           intptr_t count)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    attr->kind = DTPI_DATATYPE_KIND__SUBARRAY;
    intptr_t tmp_lb;
    rc = yaksa_type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_RC(rc);
    const char *s = desc;
    EXPECT_CHAR('[');
    s++;

    int n;
    n = sscanf(s, "ndims %d", &attr->u.subarray.ndims);
    DTPI_ERR_ASSERT(n == 1, rc);
    DTPI_ALLOC_OR_FAIL(attr->u.subarray.array_of_sizes, attr->u.subarray.ndims * sizeof(intptr_t),
                       rc);
    SKIP_UNTIL('(');
    s++;
    for (int i = 0; i < attr->u.subarray.ndims; i++) {
        if (i > 0) {
            EXPECT_CHAR(',');
            s++;
        }
        EXPECT_DIGIT;
        attr->u.subarray.array_of_sizes[i] = atoi(s);
        SKIP_DIGIT;
    }
    DTPI_ALLOC_OR_FAIL(attr->u.subarray.array_of_subsizes,
                       attr->u.subarray.ndims * sizeof(intptr_t), rc);
    SKIP_UNTIL('(');
    s++;
    for (int i = 0; i < attr->u.subarray.ndims; i++) {
        if (i > 0) {
            EXPECT_CHAR(',');
            s++;
        }
        EXPECT_DIGIT;
        attr->u.subarray.array_of_subsizes[i] = atoi(s);
        SKIP_DIGIT;
    }
    DTPI_ALLOC_OR_FAIL(attr->u.subarray.array_of_starts, attr->u.subarray.ndims * sizeof(intptr_t),
                       rc);
    SKIP_UNTIL('(');
    s++;
    for (int i = 0; i < attr->u.subarray.ndims; i++) {
        if (i > 0) {
            EXPECT_CHAR(',');
            s++;
        }
        EXPECT_DIGIT;
        attr->u.subarray.array_of_starts[i] = atoi(s);
        SKIP_DIGIT;
    }
    SKIP_UNTIL('o');
    if (strncmp(s, "order C", 7) == 0) {
        attr->u.subarray.order = YAKSA_SUBARRAY_ORDER__C;
    } else if (strncmp(s, "order FORTRAN", 13) == 0) {
        attr->u.subarray.order = YAKSA_SUBARRAY_ORDER__FORTRAN;
    } else {
        DTPI_ERR_ASSERT(0, rc);
    }

    rc = yaksa_type_create_subarray(attr->u.subarray.ndims, attr->u.subarray.array_of_sizes,
                                    attr->u.subarray.array_of_subsizes,
                                    attr->u.subarray.array_of_starts, attr->u.subarray.order, type,
                                    NULL, newtype);
    DTPI_ERR_CHK_RC(rc);

    int total_blklen = 1;
    for (int i = 0; i < attr->u.subarray.ndims; i++) {
        total_blklen *= attr->u.subarray.array_of_subsizes[i];
    }
    DTPI_ERR_ASSERT(count % total_blklen == 0, rc);
    count /= total_blklen;

    *new_count = count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int custom_struct(DTP_pool_s dtp, DTPI_Attr_s * attr, yaksa_type_t * newtype,
                         intptr_t * new_count, const char *desc, yaksa_type_t type, intptr_t count)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    attr->kind = DTPI_DATATYPE_KIND__STRUCT;
    intptr_t tmp_lb;
    rc = yaksa_type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_RC(rc);
    const char *s = desc;
    EXPECT_CHAR('[');
    s++;

    int n;
    n = sscanf(s, "numblks %d", &attr->u.structure.numblks);
    DTPI_ERR_ASSERT(n == 1, rc);
    DTPI_ALLOC_OR_FAIL(attr->u.structure.array_of_blklens,
                       attr->u.structure.numblks * sizeof(intptr_t), rc);
    SKIP_UNTIL('(');
    s++;
    for (int i = 0; i < attr->u.structure.numblks; i++) {
        if (i > 0) {
            EXPECT_CHAR(',');
            s++;
        }
        EXPECT_DIGIT;
        attr->u.structure.array_of_blklens[i] = atoi(s);
        SKIP_DIGIT;
    }
    DTPI_ALLOC_OR_FAIL(attr->u.structure.array_of_displs,
                       attr->u.structure.numblks * sizeof(intptr_t), rc);
    SKIP_UNTIL('(');
    s++;
    for (int i = 0; i < attr->u.structure.numblks; i++) {
        if (i > 0) {
            EXPECT_CHAR(',');
            s++;
        }
        EXPECT_DIGIT;
        attr->u.structure.array_of_displs[i] = atoi(s);
        SKIP_DIGIT;
    }
    yaksa_type_t *array_of_types;
    DTPI_ALLOC_OR_FAIL(array_of_types, attr->u.structure.numblks * sizeof(yaksa_type_t), rc);
    for (int i = 0; i < attr->u.structure.numblks; i++) {
        array_of_types[i] = type;
    }
    rc = yaksa_type_create_struct(attr->u.structure.numblks, attr->u.structure.array_of_blklens,
                                  attr->u.structure.array_of_displs, array_of_types, NULL, newtype);
    DTPI_ERR_CHK_RC(rc);
    DTPI_FREE(array_of_types);

    int total_blklen = 0;
    for (int i = 0; i < attr->u.structure.numblks; i++) {
        total_blklen += attr->u.structure.array_of_blklens[i];
    }
    DTPI_ERR_ASSERT(count % total_blklen == 0, rc);
    count /= total_blklen;

    *new_count = count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

int DTPI_custom_datatype(DTP_pool_s dtp, DTPI_Attr_s ** attr_tree, yaksa_type_t * newtype,
                         uintptr_t * new_count, const char **desc_list, int depth)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    if (depth == 0) {
        DTPI_pool_s *dtpi = dtp.priv;
        *attr_tree = NULL;
        *newtype = dtp.DTP_base_type;
        *new_count = dtpi->base_type_count;
        goto fn_exit;
    }

    DTPI_Attr_s *attr;
    DTPI_ALLOC_OR_FAIL(attr, sizeof(DTPI_Attr_s), rc);
    *attr_tree = attr;

    yaksa_type_t type;
    intptr_t count = 0;
    rc = DTPI_custom_datatype(dtp, &attr->child, &type, &count, desc_list + 1, depth - 1);
    DTPI_ERR_CHK_RC(rc);

    const char *s = desc_list[0];
    if (strncmp(s, "contig", 6) == 0) {
        rc = custom_contig(dtp, attr, newtype, new_count, s + 6, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else if (strncmp(s, "dup", 3) == 0) {
        rc = custom_dup(dtp, attr, newtype, new_count, s + 3, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else if (strncmp(s, "resized", 7) == 0) {
        rc = custom_resized(dtp, attr, newtype, new_count, s + 7, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else if (strncmp(s, "vector", 6) == 0) {
        rc = custom_vector(dtp, attr, newtype, new_count, s + 6, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else if (strncmp(s, "hvector", 7) == 0) {
        rc = custom_hvector(dtp, attr, newtype, new_count, s + 7, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else if (strncmp(s, "blkindx", 7) == 0) {
        rc = custom_blkindx(dtp, attr, newtype, new_count, s + 7, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else if (strncmp(s, "blkhindx", 8) == 0) {
        rc = custom_blkhindx(dtp, attr, newtype, new_count, s + 8, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else if (strncmp(s, "indexed", 7) == 0) {
        rc = custom_indexed(dtp, attr, newtype, new_count, s + 7, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else if (strncmp(s, "hindexed", 8) == 0) {
        rc = custom_hindexed(dtp, attr, newtype, new_count, s + 8, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else if (strncmp(s, "subarray", 8) == 0) {
        rc = custom_subarray(dtp, attr, newtype, new_count, s + 8, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else if (strncmp(s, "structure", 9) == 0) {
        rc = custom_struct(dtp, attr, newtype, new_count, s + 9, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else if (strncmp(s, "struct", 6) == 0) {
        rc = custom_struct(dtp, attr, newtype, new_count, s + 6, type, count);
        DTPI_ERR_CHK_RC(rc);
    } else {
        DTPI_ERR_ASSERT(0, rc);
    }

    if (depth > 1) {
        rc = yaksa_type_free(type);
        DTPI_ERR_CHK_RC(rc);
    }

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}
