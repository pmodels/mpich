/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "dtpools_internal.h"
#include <assert.h>
#include <limits.h>
#include <inttypes.h>

#define VALUE_FITS_IN_INT(val) ((val) <= INT_MAX)
#define VALUE_FITS_IN_AINT(val) ((val) <= (((uint64_t) 1 << (sizeof(MPI_Aint) * 8 - 1)) - 1))

#define confirm_extent(type, extent)                                    \
    do {                                                                \
        MPI_Aint type_extent;                                           \
        MPI_Aint tmp_lb_;                                               \
        MPI_Type_get_extent(type, &tmp_lb_, &type_extent);              \
        if (type_extent != extent) {                                    \
            fprintf(stderr, "expected extent of %" PRIu64 ", but got %zd\n", extent, type_extent); \
            fflush(stderr);                                             \
            assert(0);                                                  \
        }                                                               \
    } while (0)

static int construct_contig(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s * attr,
                            MPI_Datatype * newtype, MPI_Aint * new_count)
{
    MPI_Datatype type;
    MPI_Aint count = 0;
    DTPI_pool_s *dtpi = dtp.priv;
    int rc = DTP_SUCCESS;
    int64_t extent;

    /* setup the child type first */
    rc = DTPI_construct_datatype(dtp, attr_tree_depth - 1, &attr->child, &type, &count);
    DTPI_ERR_CHK_RC(rc);

    MPI_Aint tmp_lb;
    rc = MPI_Type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    while (1) {
        extent = attr->child_type_extent;

        int blklen_attr = DTPI_rand(dtpi) % DTPI_ATTR_CONTIG_BLKLEN__LAST;
        if (blklen_attr == DTPI_ATTR_CONTIG_BLKLEN__ONE)
            attr->u.contig.blklen = 1;
        else if (blklen_attr == DTPI_ATTR_CONTIG_BLKLEN__SMALL)
            attr->u.contig.blklen = DTPI_low_count(count);
        else if (blklen_attr == DTPI_ATTR_CONTIG_BLKLEN__LARGE)
            attr->u.contig.blklen = DTPI_high_count(count);
        else {
            DTPI_ERR_ASSERT(0, rc);
        }

        extent *= attr->u.contig.blklen;

        count /= attr->u.contig.blklen;

        if (VALUE_FITS_IN_AINT(extent * count))
            break;
    }

    rc = MPI_Type_contiguous(attr->u.contig.blklen, type, newtype);
    DTPI_ERR_CHK_MPI_RC(rc);

    confirm_extent(*newtype, extent);

  fn_exit:
    *new_count = count;
    if (attr_tree_depth > 1) {
        rc = MPI_Type_free(&type);
        DTPI_ERR_CHK_MPI_RC(rc);
    }
    return rc;

  fn_fail:
    goto fn_exit;
}

static int construct_dup(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s * attr,
                         MPI_Datatype * newtype, MPI_Aint * new_count)
{
    MPI_Datatype type;
    MPI_Aint count = 0;
    int rc = DTP_SUCCESS;

    /* setup the child type first */
    rc = DTPI_construct_datatype(dtp, attr_tree_depth - 1, &attr->child, &type, &count);
    DTPI_ERR_CHK_RC(rc);

    MPI_Aint tmp_lb;
    rc = MPI_Type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    rc = MPI_Type_dup(type, newtype);
    DTPI_ERR_CHK_MPI_RC(rc);

  fn_exit:
    *new_count = count;
    if (attr_tree_depth > 1) {
        rc = MPI_Type_free(&type);
        DTPI_ERR_CHK_MPI_RC(rc);
    }
    return rc;

  fn_fail:
    goto fn_exit;
}

static int construct_resized(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s * attr,
                             MPI_Datatype * newtype, MPI_Aint * new_count)
{
    MPI_Aint true_lb, true_extent, lb, extent;
    MPI_Datatype type;
    MPI_Aint count = 0;
    DTPI_pool_s *dtpi = dtp.priv;
    int rc = DTP_SUCCESS;

    /* setup the child type first */
    rc = DTPI_construct_datatype(dtp, attr_tree_depth - 1, &attr->child, &type, &count);
    DTPI_ERR_CHK_RC(rc);

    rc = MPI_Type_get_extent(type, &lb, &attr->child_type_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    extent = attr->child_type_extent;

    rc = MPI_Type_get_true_extent(type, &true_lb, &true_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    int64_t e;
    while (1) {
        /* initialize to original values */
        attr->u.resized.lb = lb;
        attr->u.resized.extent = extent;

        int64_t lb_tmp = lb;
        int lb_attr = DTPI_rand(dtpi) % DTPI_ATTR_RESIZED_LB__LAST;
        if (lb_attr == DTPI_ATTR_RESIZED_LB__PACKED) {
            lb_tmp = true_lb;
        } else if (lb_attr == DTPI_ATTR_RESIZED_LB__LOW) {
            lb_tmp -= (int64_t) 2 *extent;
        } else if (lb_attr == DTPI_ATTR_RESIZED_LB__VERY_LOW) {
            lb_tmp -= (int64_t) 10 *extent;
        } else if (lb_attr == DTPI_ATTR_RESIZED_LB__HIGH) {
            lb_tmp += (int64_t) 2 *extent;
        } else if (lb_attr == DTPI_ATTR_RESIZED_LB__VERY_HIGH) {
            lb_tmp += (int64_t) 10 *extent;
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        if (!VALUE_FITS_IN_AINT(lb_tmp))
            continue;
        attr->u.resized.lb = lb_tmp;

        int extent_attr = DTPI_rand(dtpi) % DTPI_ATTR_RESIZED_EXTENT__LAST;
        if (extent_attr == DTPI_ATTR_RESIZED_EXTENT__PACKED) {
            e = (int64_t) true_extent;
        } else if (extent_attr == DTPI_ATTR_RESIZED_EXTENT__HIGH) {
            e = ((int64_t) 2) * extent;
        } else if (extent_attr == DTPI_ATTR_RESIZED_EXTENT__VERY_HIGH) {
            e = ((int64_t) 10) * extent;
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        if (!VALUE_FITS_IN_AINT(e * count))
            continue;

        if (!VALUE_FITS_IN_AINT(e + attr->u.resized.lb))
            continue;

        attr->u.resized.extent = (MPI_Aint) e;
        break;
    }

    rc = MPI_Type_create_resized(type, attr->u.resized.lb, attr->u.resized.extent, newtype);
    DTPI_ERR_CHK_MPI_RC(rc);

  fn_exit:
    *new_count = count;
    if (attr_tree_depth > 1) {
        rc = MPI_Type_free(&type);
        DTPI_ERR_CHK_MPI_RC(rc);
    }
    return rc;

  fn_fail:
    goto fn_exit;
}

static int construct_vector(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s * attr,
                            MPI_Datatype * newtype, MPI_Aint * new_count)
{
    MPI_Datatype type;
    MPI_Aint count = 0;
    DTPI_pool_s *dtpi = dtp.priv;
    int rc = DTP_SUCCESS;
    int64_t extent;

    /* setup the child type first */
    rc = DTPI_construct_datatype(dtp, attr_tree_depth - 1, &attr->child, &type, &count);
    DTPI_ERR_CHK_RC(rc);

    MPI_Aint tmp_lb;
    rc = MPI_Type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    MPI_Aint tmp_count = count;
    while (1) {
      retry:
        extent = attr->child_type_extent;

        /* reset necessary variables in each iteration, in case we
         * need to go through this loop more than once */
        count = tmp_count;

        int numblks_attr = DTPI_rand(dtpi) % DTPI_ATTR_VECTOR_NUMBLKS__LAST;
        if (numblks_attr == DTPI_ATTR_VECTOR_NUMBLKS__ONE)
            attr->u.vector.numblks = 1;
        else if (numblks_attr == DTPI_ATTR_VECTOR_NUMBLKS__SMALL)
            attr->u.vector.numblks = DTPI_low_count(count);
        else if (numblks_attr == DTPI_ATTR_VECTOR_NUMBLKS__LARGE)
            attr->u.vector.numblks = DTPI_high_count(count);
        else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count /= attr->u.vector.numblks;

        int blklen_attr = DTPI_rand(dtpi) % DTPI_ATTR_VECTOR_BLKLEN__LAST;
        if (blklen_attr == DTPI_ATTR_VECTOR_BLKLEN__ONE)
            attr->u.vector.blklen = 1;
        else if (blklen_attr == DTPI_ATTR_VECTOR_BLKLEN__SMALL)
            attr->u.vector.blklen = DTPI_low_count(count);
        else if (blklen_attr == DTPI_ATTR_VECTOR_BLKLEN__LARGE)
            attr->u.vector.blklen = DTPI_high_count(count);
        else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count /= attr->u.vector.blklen;

        int stride_attr = DTPI_rand(dtpi) % DTPI_ATTR_VECTOR_STRIDE__LAST;
        int64_t stride;
        if (stride_attr == DTPI_ATTR_VECTOR_STRIDE__SMALL)
            stride = (int64_t) attr->u.vector.blklen + 1;
        else if (stride_attr == DTPI_ATTR_VECTOR_STRIDE__LARGE)
            stride = (int64_t) attr->u.vector.blklen * 4;
        else if (stride_attr == DTPI_ATTR_VECTOR_STRIDE__NEGATIVE)
            stride = ((int64_t) (-2)) * attr->u.vector.blklen;
        else {
            DTPI_ERR_ASSERT(0, rc);
        }
        if (!VALUE_FITS_IN_INT(stride))
            goto retry;
        attr->u.vector.stride = (int) stride;

        /* calculate the extent for our datatype, to decide whether we
         * want to use this or try again */
        int max_displ_idx = -1;
        int min_displ_idx = -1;
        for (int i = 0; i < attr->u.vector.numblks; i++) {
            if (max_displ_idx == -1)
                max_displ_idx = i;
            else if (attr->u.vector.stride > 0)
                max_displ_idx = i;

            if (min_displ_idx == -1)
                min_displ_idx = i;
            else if (attr->u.vector.stride < 0)
                min_displ_idx = i;
        }

        extent *= (int64_t) attr->u.vector.stride * max_displ_idx -
            (int64_t) attr->u.vector.stride * min_displ_idx + attr->u.vector.blklen;

        if (VALUE_FITS_IN_AINT(extent * count))
            break;
    }

    rc = MPI_Type_vector(attr->u.vector.numblks,
                         attr->u.vector.blklen, attr->u.vector.stride, type, newtype);
    DTPI_ERR_CHK_MPI_RC(rc);

    confirm_extent(*newtype, extent);

  fn_exit:
    *new_count = count;
    if (attr_tree_depth > 1) {
        rc = MPI_Type_free(&type);
        DTPI_ERR_CHK_MPI_RC(rc);
    }
    return rc;

  fn_fail:
    goto fn_exit;
}

static int construct_hvector(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s * attr,
                             MPI_Datatype * newtype, MPI_Aint * new_count)
{
    MPI_Datatype type;
    MPI_Aint count = 0;
    DTPI_pool_s *dtpi = dtp.priv;
    int rc = DTP_SUCCESS;
    int64_t extent;

    /* setup the child type first */
    rc = DTPI_construct_datatype(dtp, attr_tree_depth - 1, &attr->child, &type, &count);
    DTPI_ERR_CHK_RC(rc);

    MPI_Aint tmp_lb;
    rc = MPI_Type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    MPI_Aint tmp_count = count;
    while (1) {
      retry:
        extent = attr->child_type_extent;

        /* reset necessary variables in each iteration, in case we
         * need to go through this loop more than once */
        count = tmp_count;

        int numblks_attr = DTPI_rand(dtpi) % DTPI_ATTR_HVECTOR_NUMBLKS__LAST;
        if (numblks_attr == DTPI_ATTR_HVECTOR_NUMBLKS__ONE)
            attr->u.hvector.numblks = 1;
        else if (numblks_attr == DTPI_ATTR_HVECTOR_NUMBLKS__SMALL)
            attr->u.hvector.numblks = DTPI_low_count(count);
        else if (numblks_attr == DTPI_ATTR_HVECTOR_NUMBLKS__LARGE)
            attr->u.hvector.numblks = DTPI_high_count(count);
        else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count /= attr->u.hvector.numblks;

        int blklen_attr = DTPI_rand(dtpi) % DTPI_ATTR_HVECTOR_BLKLEN__LAST;
        if (blklen_attr == DTPI_ATTR_HVECTOR_BLKLEN__ONE)
            attr->u.hvector.blklen = 1;
        else if (blklen_attr == DTPI_ATTR_HVECTOR_BLKLEN__SMALL)
            attr->u.hvector.blklen = DTPI_low_count(count);
        else if (blklen_attr == DTPI_ATTR_HVECTOR_BLKLEN__LARGE)
            attr->u.hvector.blklen = DTPI_high_count(count);
        else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count /= attr->u.hvector.blklen;

        /* FIXME: we should detect the maximum alignment needed by the
         * compiler instead of arbitrarily incrementing the stride by
         * a constant value. */
        int stride_attr = DTPI_rand(dtpi) % DTPI_ATTR_HVECTOR_STRIDE__LAST;
        int64_t stride;
        if (stride_attr == DTPI_ATTR_HVECTOR_STRIDE__SMALL) {
            stride = (int64_t) attr->u.hvector.blklen * attr->child_type_extent + 8;
        } else if (stride_attr == DTPI_ATTR_HVECTOR_STRIDE__LARGE) {
            stride = (int64_t) attr->u.hvector.blklen * attr->child_type_extent * 4 + 8;
        } else if (stride_attr == DTPI_ATTR_HVECTOR_STRIDE__NEGATIVE) {
            stride = ((int64_t) (-2)) * attr->u.hvector.blklen * attr->child_type_extent * 4 - 8;
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }
        if (!VALUE_FITS_IN_AINT(stride))
            goto retry;
        attr->u.hvector.stride = (MPI_Aint) stride;

        /* calculate the extent for our datatype, to decide whether we
         * want to use this or try again */
        int max_displ_idx = -1;
        int min_displ_idx = -1;
        for (int i = 0; i < attr->u.hvector.numblks; i++) {
            if (max_displ_idx == -1)
                max_displ_idx = i;
            else if (attr->u.hvector.stride > 0)
                max_displ_idx = i;

            if (min_displ_idx == -1)
                min_displ_idx = i;
            else if (attr->u.hvector.stride < 0)
                min_displ_idx = i;
        }

        extent = (int64_t) attr->u.hvector.stride * max_displ_idx -
            (int64_t) attr->u.hvector.stride * min_displ_idx +
            (int64_t) attr->u.hvector.blklen * attr->child_type_extent;

        if (VALUE_FITS_IN_AINT(extent * count))
            break;
    }

    rc = MPI_Type_create_hvector(attr->u.hvector.numblks,
                                 attr->u.hvector.blklen, attr->u.hvector.stride, type, newtype);
    DTPI_ERR_CHK_MPI_RC(rc);

    confirm_extent(*newtype, extent);

  fn_exit:
    *new_count = count;
    if (attr_tree_depth > 1) {
        rc = MPI_Type_free(&type);
        DTPI_ERR_CHK_MPI_RC(rc);
    }
    return rc;

  fn_fail:
    goto fn_exit;
}

static int construct_blkindx(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s * attr,
                             MPI_Datatype * newtype, MPI_Aint * new_count)
{
    MPI_Datatype type;
    MPI_Aint count = 0;
    DTPI_pool_s *dtpi = dtp.priv;
    int rc = DTP_SUCCESS;
    int64_t extent;

    /* setup the child type first */
    rc = DTPI_construct_datatype(dtp, attr_tree_depth - 1, &attr->child, &type, &count);
    DTPI_ERR_CHK_RC(rc);

    MPI_Aint tmp_lb;
    rc = MPI_Type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    MPI_Aint tmp_count = count;
    attr->u.blkindx.array_of_displs = NULL;
    while (1) {
      retry:
        extent = attr->child_type_extent;

        /* reset necessary variables in each iteration, in case we
         * need to go through this loop more than once */
        count = tmp_count;
        if (attr->u.blkindx.array_of_displs)
            DTPI_FREE(attr->u.blkindx.array_of_displs);

        int numblks_attr = DTPI_rand(dtpi) % DTPI_ATTR_BLKINDX_NUMBLKS__LAST;
        if (numblks_attr == DTPI_ATTR_BLKINDX_NUMBLKS__ONE)
            attr->u.blkindx.numblks = 1;
        else if (numblks_attr == DTPI_ATTR_BLKINDX_NUMBLKS__SMALL)
            attr->u.blkindx.numblks = DTPI_low_count(count);
        else if (numblks_attr == DTPI_ATTR_BLKINDX_NUMBLKS__LARGE)
            attr->u.blkindx.numblks = DTPI_high_count(count);
        else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count /= attr->u.blkindx.numblks;

        int blklen_attr = DTPI_rand(dtpi) % DTPI_ATTR_BLKINDX_BLKLEN__LAST;
        if (blklen_attr == DTPI_ATTR_BLKINDX_BLKLEN__ONE)
            attr->u.blkindx.blklen = 1;
        else if (blklen_attr == DTPI_ATTR_BLKINDX_BLKLEN__SMALL)
            attr->u.blkindx.blklen = DTPI_low_count(count);
        else if (blklen_attr == DTPI_ATTR_BLKINDX_BLKLEN__LARGE)
            attr->u.blkindx.blklen = DTPI_high_count(count);
        else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count /= attr->u.blkindx.blklen;

        DTPI_ALLOC_OR_FAIL(attr->u.blkindx.array_of_displs, attr->u.blkindx.numblks * sizeof(int),
                           rc);

        int64_t total_displ = 0;
        int displs_attr = DTPI_rand(dtpi) % DTPI_ATTR_BLKINDX_DISPLS__LAST;
        if (displs_attr == DTPI_ATTR_BLKINDX_DISPLS__SMALL) {
            for (int i = 0; i < attr->u.blkindx.numblks; i++) {
                attr->u.blkindx.array_of_displs[i] = (int) total_displ;
                total_displ += (int64_t) attr->u.blkindx.blklen + 1;
                if (!VALUE_FITS_IN_INT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_BLKINDX_DISPLS__LARGE) {
            for (int i = 0; i < attr->u.blkindx.numblks; i++) {
                attr->u.blkindx.array_of_displs[i] = (int) total_displ;
                total_displ += (int64_t) attr->u.blkindx.blklen * 4;
                if (!VALUE_FITS_IN_INT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_BLKINDX_DISPLS__REDUCING) {
            for (int i = 0; i < attr->u.blkindx.numblks; i++) {
                int idx = attr->u.blkindx.numblks - i - 1;
                attr->u.blkindx.array_of_displs[idx] = (int) total_displ;
                total_displ += (int64_t) attr->u.blkindx.blklen + 1;
                if (!VALUE_FITS_IN_INT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_BLKINDX_DISPLS__UNEVEN) {
            for (int i = 0; i < attr->u.blkindx.numblks; i++) {
                attr->u.blkindx.array_of_displs[i] = (int) total_displ;
                total_displ += (int64_t) attr->u.blkindx.blklen + i;
                if (!VALUE_FITS_IN_INT(total_displ))
                    goto retry;
            }
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        /* calculate the extent for our datatype, to decide whether we
         * want to use this or try again */
        int max_displ_idx = -1;
        int min_displ_idx = -1;
        for (int i = 0; i < attr->u.blkindx.numblks; i++) {
            if (max_displ_idx == -1)
                max_displ_idx = i;
            else if (attr->u.blkindx.array_of_displs[i] >
                     attr->u.blkindx.array_of_displs[max_displ_idx])
                max_displ_idx = i;

            if (min_displ_idx == -1)
                min_displ_idx = i;
            else if (attr->u.blkindx.array_of_displs[i] <
                     attr->u.blkindx.array_of_displs[min_displ_idx])
                min_displ_idx = i;
        }

        extent *= (int64_t) attr->u.blkindx.array_of_displs[max_displ_idx] -
            (int64_t) attr->u.blkindx.array_of_displs[min_displ_idx] + attr->u.blkindx.blklen;

        if (VALUE_FITS_IN_AINT(extent * count))
            break;
    }

    rc = MPI_Type_create_indexed_block(attr->u.blkindx.numblks,
                                       attr->u.blkindx.blklen,
                                       attr->u.blkindx.array_of_displs, type, newtype);
    DTPI_ERR_CHK_MPI_RC(rc);

    confirm_extent(*newtype, extent);

  fn_exit:
    *new_count = count;
    if (attr_tree_depth > 1) {
        rc = MPI_Type_free(&type);
        DTPI_ERR_CHK_MPI_RC(rc);
    }
    return rc;

  fn_fail:
    goto fn_exit;
}

static int construct_blkhindx(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s * attr,
                              MPI_Datatype * newtype, MPI_Aint * new_count)
{
    MPI_Datatype type;
    MPI_Aint count = 0;
    DTPI_pool_s *dtpi = dtp.priv;
    int rc = DTP_SUCCESS;
    int64_t extent;

    /* setup the child type first */
    rc = DTPI_construct_datatype(dtp, attr_tree_depth - 1, &attr->child, &type, &count);
    DTPI_ERR_CHK_RC(rc);

    MPI_Aint tmp_lb;
    rc = MPI_Type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    MPI_Aint tmp_count = count;
    while (1) {
      retry:
        extent = attr->child_type_extent;

        /* reset necessary variables in each iteration, in case we
         * need to go through this loop more than once */
        count = tmp_count;
        if (attr->u.blkhindx.array_of_displs)
            DTPI_FREE(attr->u.blkhindx.array_of_displs);

        int numblks_attr = DTPI_rand(dtpi) % DTPI_ATTR_BLKHINDX_NUMBLKS__LAST;
        if (numblks_attr == DTPI_ATTR_BLKHINDX_NUMBLKS__ONE)
            attr->u.blkhindx.numblks = 1;
        else if (numblks_attr == DTPI_ATTR_BLKHINDX_NUMBLKS__SMALL)
            attr->u.blkhindx.numblks = DTPI_low_count(count);
        else if (numblks_attr == DTPI_ATTR_BLKHINDX_NUMBLKS__LARGE)
            attr->u.blkhindx.numblks = DTPI_high_count(count);
        else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count /= attr->u.blkhindx.numblks;

        int blklen_attr = DTPI_rand(dtpi) % DTPI_ATTR_BLKHINDX_BLKLEN__LAST;
        if (blklen_attr == DTPI_ATTR_BLKHINDX_BLKLEN__ONE)
            attr->u.blkhindx.blklen = 1;
        else if (blklen_attr == DTPI_ATTR_BLKHINDX_BLKLEN__SMALL)
            attr->u.blkhindx.blklen = DTPI_low_count(count);
        else if (blklen_attr == DTPI_ATTR_BLKHINDX_BLKLEN__LARGE)
            attr->u.blkhindx.blklen = DTPI_high_count(count);
        else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count /= attr->u.blkhindx.blklen;

        DTPI_ALLOC_OR_FAIL(attr->u.blkhindx.array_of_displs,
                           attr->u.blkhindx.numblks * sizeof(MPI_Aint), rc);

        /* FIXME: we should detect the maximum alignment needed by the
         * compiler instead of arbitrarily incrementing the stride by
         * a constant value. */
        int64_t total_displ = 8;
        int displs_attr = DTPI_rand(dtpi) % DTPI_ATTR_BLKHINDX_DISPLS__LAST;
        if (displs_attr == DTPI_ATTR_BLKHINDX_DISPLS__SMALL) {
            for (int i = 0; i < attr->u.blkhindx.numblks; i++) {
                attr->u.blkhindx.array_of_displs[i] = (MPI_Aint) total_displ;
                total_displ += ((int64_t) attr->u.blkhindx.blklen + 1) * attr->child_type_extent;
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_BLKHINDX_DISPLS__LARGE) {
            for (int i = 0; i < attr->u.blkhindx.numblks; i++) {
                attr->u.blkhindx.array_of_displs[i] = (MPI_Aint) total_displ;
                total_displ += ((int64_t) attr->u.blkhindx.blklen * 4) * attr->child_type_extent;
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_BLKHINDX_DISPLS__REDUCING) {
            for (int i = 0; i < attr->u.blkhindx.numblks; i++) {
                int idx = attr->u.blkhindx.numblks - i - 1;
                attr->u.blkhindx.array_of_displs[idx] = (MPI_Aint) total_displ;
                total_displ += ((int64_t) attr->u.blkhindx.blklen + 1) * attr->child_type_extent;
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_BLKHINDX_DISPLS__UNEVEN) {
            for (int i = 0; i < attr->u.blkhindx.numblks; i++) {
                attr->u.blkhindx.array_of_displs[i] = (MPI_Aint) total_displ;
                total_displ += ((int64_t) attr->u.blkhindx.blklen + i) * attr->child_type_extent;
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        /* calculate the extent for our datatype, to decide whether we
         * want to use this or try again */
        int max_displ_idx = -1;
        int min_displ_idx = -1;
        for (int i = 0; i < attr->u.blkhindx.numblks; i++) {
            if (max_displ_idx == -1)
                max_displ_idx = i;
            else if (attr->u.blkhindx.array_of_displs[i] >
                     attr->u.blkhindx.array_of_displs[max_displ_idx])
                max_displ_idx = i;

            if (min_displ_idx == -1)
                min_displ_idx = i;
            else if (attr->u.blkhindx.array_of_displs[i] <
                     attr->u.blkhindx.array_of_displs[min_displ_idx])
                min_displ_idx = i;
        }

        extent = (int64_t) attr->u.blkhindx.array_of_displs[max_displ_idx] -
            (int64_t) attr->u.blkhindx.array_of_displs[min_displ_idx] +
            (int64_t) attr->u.blkindx.blklen * attr->child_type_extent;

        if (VALUE_FITS_IN_AINT(extent * count))
            break;
    }

    rc = MPI_Type_create_hindexed_block(attr->u.blkhindx.numblks,
                                        attr->u.blkhindx.blklen,
                                        attr->u.blkhindx.array_of_displs, type, newtype);
    DTPI_ERR_CHK_MPI_RC(rc);

    confirm_extent(*newtype, extent);

  fn_exit:
    *new_count = count;
    if (attr_tree_depth > 1) {
        rc = MPI_Type_free(&type);
        DTPI_ERR_CHK_MPI_RC(rc);
    }
    return rc;

  fn_fail:
    goto fn_exit;
}

static int construct_indexed(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s * attr,
                             MPI_Datatype * newtype, MPI_Aint * new_count)
{
    MPI_Datatype type;
    MPI_Aint count = 0;
    DTPI_pool_s *dtpi = dtp.priv;
    int rc = DTP_SUCCESS;
    int64_t extent;

    /* setup the child type first */
    rc = DTPI_construct_datatype(dtp, attr_tree_depth - 1, &attr->child, &type, &count);
    DTPI_ERR_CHK_RC(rc);

    MPI_Aint tmp_lb;
    rc = MPI_Type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    MPI_Aint tmp_count = count;
    attr->u.indexed.array_of_blklens = NULL;
    attr->u.indexed.array_of_displs = NULL;
    while (1) {
      retry:
        extent = attr->child_type_extent;

        /* reset necessary variables in each iteration, in case we
         * need to go through this loop more than once */
        count = tmp_count;
        if (attr->u.indexed.array_of_blklens)
            DTPI_FREE(attr->u.indexed.array_of_blklens);
        if (attr->u.indexed.array_of_displs)
            DTPI_FREE(attr->u.indexed.array_of_displs);

        int numblks_attr = DTPI_rand(dtpi) % DTPI_ATTR_INDEXED_NUMBLKS__LAST;
        if (numblks_attr == DTPI_ATTR_INDEXED_NUMBLKS__ONE)
            attr->u.indexed.numblks = 1;
        else if (numblks_attr == DTPI_ATTR_INDEXED_NUMBLKS__SMALL)
            attr->u.indexed.numblks = DTPI_low_count(count);
        else if (numblks_attr == DTPI_ATTR_INDEXED_NUMBLKS__LARGE)
            attr->u.indexed.numblks = DTPI_high_count(count);
        else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count /= attr->u.indexed.numblks;

        DTPI_ALLOC_OR_FAIL(attr->u.indexed.array_of_blklens, attr->u.indexed.numblks * sizeof(int),
                           rc);

        int64_t total_blklen = 0;
        int blklen_attr = DTPI_rand(dtpi) % DTPI_ATTR_INDEXED_BLKLEN__LAST;
        if (blklen_attr == DTPI_ATTR_INDEXED_BLKLEN__ONE) {
            for (int i = 0; i < attr->u.indexed.numblks; i++) {
                attr->u.indexed.array_of_blklens[i] = 1;
                total_blklen += attr->u.indexed.array_of_blklens[i];
            }
        } else if (blklen_attr == DTPI_ATTR_INDEXED_BLKLEN__SMALL) {
            for (int i = 0; i < attr->u.indexed.numblks; i++) {
                attr->u.indexed.array_of_blklens[i] = DTPI_low_count(count);
                total_blklen += attr->u.indexed.array_of_blklens[i];
            }
        } else if (blklen_attr == DTPI_ATTR_INDEXED_BLKLEN__LARGE) {
            for (int i = 0; i < attr->u.indexed.numblks; i++) {
                attr->u.indexed.array_of_blklens[i] = DTPI_high_count(count);
                total_blklen += attr->u.indexed.array_of_blklens[i];
            }
        } else if (blklen_attr == DTPI_ATTR_INDEXED_BLKLEN__UNEVEN) {
            for (int i = 0; i < attr->u.indexed.numblks; i++) {
                if (i % 2 == 0)
                    attr->u.indexed.array_of_blklens[i] = DTPI_low_count(count) + 1;
                else
                    attr->u.indexed.array_of_blklens[i] = DTPI_low_count(count) - 1;
                total_blklen += attr->u.indexed.array_of_blklens[i];
            }
            if (attr->u.indexed.numblks % 2) {
                /* if we have an odd number of blocks, adjust the counts */
                int idx = attr->u.indexed.numblks - 1;
                attr->u.indexed.array_of_blklens[idx]--;
                total_blklen--;
            }
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count *= attr->u.indexed.numblks;
        count /= total_blklen;

        DTPI_ALLOC_OR_FAIL(attr->u.indexed.array_of_displs, attr->u.indexed.numblks * sizeof(int),
                           rc);

        int64_t total_displ = 0;
        int displs_attr = DTPI_rand(dtpi) % DTPI_ATTR_INDEXED_DISPLS__LAST;
        if (displs_attr == DTPI_ATTR_INDEXED_DISPLS__SMALL) {
            for (int i = 0; i < attr->u.indexed.numblks; i++) {
                attr->u.indexed.array_of_displs[i] = (int) total_displ;
                total_displ += attr->u.indexed.array_of_blklens[i] + 1;
                if (!VALUE_FITS_IN_INT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_INDEXED_DISPLS__LARGE) {
            for (int i = 0; i < attr->u.indexed.numblks; i++) {
                attr->u.indexed.array_of_displs[i] = (int) total_displ;
                total_displ += attr->u.indexed.array_of_blklens[i] * 4;
                if (!VALUE_FITS_IN_INT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_INDEXED_DISPLS__REDUCING) {
            for (int i = 0; i < attr->u.indexed.numblks; i++) {
                int idx = attr->u.indexed.numblks - i - 1;
                attr->u.indexed.array_of_displs[idx] = (int) total_displ;
                total_displ += attr->u.indexed.array_of_blklens[idx] + 1;
                if (!VALUE_FITS_IN_INT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_INDEXED_DISPLS__UNEVEN) {
            for (int i = 0; i < attr->u.indexed.numblks; i++) {
                attr->u.indexed.array_of_displs[i] = (int) total_displ;
                total_displ += attr->u.indexed.array_of_blklens[i] + i;
                if (!VALUE_FITS_IN_INT(total_displ))
                    goto retry;
            }
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        /* calculate the extent for our datatype, to decide whether we
         * want to use this or try again */
        int max_displ_idx = -1;
        int min_displ_idx = -1;
        for (int i = 0; i < attr->u.indexed.numblks; i++) {
            if (attr->u.indexed.array_of_blklens[i]) {
                if (max_displ_idx == -1)
                    max_displ_idx = i;
                else if (attr->u.indexed.array_of_displs[i] >
                         attr->u.indexed.array_of_displs[max_displ_idx])
                    max_displ_idx = i;

                if (min_displ_idx == -1)
                    min_displ_idx = i;
                else if (attr->u.indexed.array_of_displs[i] <
                         attr->u.indexed.array_of_displs[min_displ_idx])
                    min_displ_idx = i;
            }
        }

        extent *= (int64_t) attr->u.indexed.array_of_displs[max_displ_idx] -
            (int64_t) attr->u.indexed.array_of_displs[min_displ_idx] +
            (int64_t) attr->u.indexed.array_of_blklens[max_displ_idx];

        if (VALUE_FITS_IN_AINT(extent * count))
            break;
    }

    rc = MPI_Type_indexed(attr->u.indexed.numblks,
                          attr->u.indexed.array_of_blklens,
                          attr->u.indexed.array_of_displs, type, newtype);
    DTPI_ERR_CHK_MPI_RC(rc);

    confirm_extent(*newtype, extent);

  fn_exit:
    *new_count = count;
    if (attr_tree_depth > 1) {
        rc = MPI_Type_free(&type);
        DTPI_ERR_CHK_MPI_RC(rc);
    }
    return rc;

  fn_fail:
    goto fn_exit;
}

static int construct_hindexed(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s * attr,
                              MPI_Datatype * newtype, MPI_Aint * new_count)
{
    MPI_Datatype type;
    MPI_Aint count = 0;
    DTPI_pool_s *dtpi = dtp.priv;
    int rc = DTP_SUCCESS;
    int64_t extent;

    /* setup the child type first */
    rc = DTPI_construct_datatype(dtp, attr_tree_depth - 1, &attr->child, &type, &count);
    DTPI_ERR_CHK_RC(rc);

    MPI_Aint tmp_lb;
    rc = MPI_Type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    MPI_Aint tmp_count = count;
    attr->u.hindexed.array_of_blklens = NULL;
    attr->u.hindexed.array_of_displs = NULL;
    while (1) {
      retry:
        extent = attr->child_type_extent;

        /* reset necessary variables in each iteration, in case we
         * need to go through this loop more than once */
        count = tmp_count;
        if (attr->u.hindexed.array_of_blklens)
            DTPI_FREE(attr->u.hindexed.array_of_blklens);
        if (attr->u.hindexed.array_of_displs)
            DTPI_FREE(attr->u.hindexed.array_of_displs);

        int numblks_attr = DTPI_rand(dtpi) % DTPI_ATTR_HINDEXED_NUMBLKS__LAST;
        if (numblks_attr == DTPI_ATTR_HINDEXED_NUMBLKS__ONE)
            attr->u.hindexed.numblks = 1;
        else if (numblks_attr == DTPI_ATTR_HINDEXED_NUMBLKS__SMALL)
            attr->u.hindexed.numblks = DTPI_low_count(count);
        else if (numblks_attr == DTPI_ATTR_HINDEXED_NUMBLKS__LARGE)
            attr->u.hindexed.numblks = DTPI_high_count(count);
        else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count /= attr->u.hindexed.numblks;

        DTPI_ALLOC_OR_FAIL(attr->u.hindexed.array_of_blklens,
                           attr->u.hindexed.numblks * sizeof(int), rc);

        int total_blklen = 0;
        int blklen_attr = DTPI_rand(dtpi) % DTPI_ATTR_HINDEXED_BLKLEN__LAST;
        if (blklen_attr == DTPI_ATTR_HINDEXED_BLKLEN__ONE) {
            for (int i = 0; i < attr->u.hindexed.numblks; i++) {
                attr->u.hindexed.array_of_blklens[i] = 1;
                total_blklen += attr->u.hindexed.array_of_blklens[i];
            }
        } else if (blklen_attr == DTPI_ATTR_HINDEXED_BLKLEN__SMALL) {
            for (int i = 0; i < attr->u.hindexed.numblks; i++) {
                attr->u.hindexed.array_of_blklens[i] = DTPI_low_count(count);
                total_blklen += attr->u.hindexed.array_of_blklens[i];
            }
        } else if (blklen_attr == DTPI_ATTR_HINDEXED_BLKLEN__LARGE) {
            for (int i = 0; i < attr->u.hindexed.numblks; i++) {
                attr->u.hindexed.array_of_blklens[i] = DTPI_high_count(count);
                total_blklen += attr->u.hindexed.array_of_blklens[i];
            }
        } else if (blklen_attr == DTPI_ATTR_HINDEXED_BLKLEN__UNEVEN) {
            for (int i = 0; i < attr->u.hindexed.numblks; i++) {
                if (i % 2 == 0)
                    attr->u.hindexed.array_of_blklens[i] = DTPI_low_count(count) + 1;
                else
                    attr->u.hindexed.array_of_blklens[i] = DTPI_low_count(count) - 1;
                total_blklen += attr->u.hindexed.array_of_blklens[i];
            }
            if (attr->u.hindexed.numblks % 2) {
                /* if we have an odd number of blocks, adjust the counts */
                int idx = attr->u.hindexed.numblks - 1;
                attr->u.hindexed.array_of_blklens[idx]--;
                total_blklen--;
            }
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count *= attr->u.hindexed.numblks;
        count /= total_blklen;

        DTPI_ALLOC_OR_FAIL(attr->u.hindexed.array_of_displs,
                           attr->u.hindexed.numblks * sizeof(MPI_Aint), rc);

        /* FIXME: we should detect the maximum alignment needed by the
         * compiler instead of arbitrarily incrementing the stride by
         * a constant value. */
        int64_t total_displ = 8;
        int displs_attr = DTPI_rand(dtpi) % DTPI_ATTR_HINDEXED_DISPLS__LAST;
        if (displs_attr == DTPI_ATTR_HINDEXED_DISPLS__SMALL) {
            for (int i = 0; i < attr->u.hindexed.numblks; i++) {
                attr->u.hindexed.array_of_displs[i] = (MPI_Aint) total_displ;
                total_displ +=
                    (int64_t) attr->child_type_extent * (attr->u.hindexed.array_of_blklens[i] + 1);
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_HINDEXED_DISPLS__LARGE) {
            for (int i = 0; i < attr->u.hindexed.numblks; i++) {
                attr->u.hindexed.array_of_displs[i] = (MPI_Aint) total_displ;
                total_displ +=
                    (int64_t) attr->child_type_extent * (attr->u.hindexed.array_of_blklens[i] * 4);
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_HINDEXED_DISPLS__REDUCING) {
            for (int i = 0; i < attr->u.hindexed.numblks; i++) {
                int idx = attr->u.hindexed.numblks - i - 1;
                attr->u.hindexed.array_of_displs[idx] = (MPI_Aint) total_displ;
                total_displ +=
                    (int64_t) attr->child_type_extent * (attr->u.hindexed.array_of_blklens[idx] +
                                                         1);
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_HINDEXED_DISPLS__UNEVEN) {
            for (int i = 0; i < attr->u.hindexed.numblks; i++) {
                attr->u.hindexed.array_of_displs[i] = (MPI_Aint) total_displ;
                total_displ +=
                    (int64_t) attr->child_type_extent * (attr->u.hindexed.array_of_blklens[i] + i);
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        /* calculate the extent for our datatype, to decide whether we
         * want to use this or try again */
        int max_displ_idx = -1;
        int min_displ_idx = -1;
        for (int i = 0; i < attr->u.hindexed.numblks; i++) {
            if (attr->u.hindexed.array_of_blklens[i]) {
                if (max_displ_idx == -1)
                    max_displ_idx = i;
                else if (attr->u.hindexed.array_of_displs[i] >
                         attr->u.hindexed.array_of_displs[max_displ_idx])
                    max_displ_idx = i;

                if (min_displ_idx == -1)
                    min_displ_idx = i;
                else if (attr->u.hindexed.array_of_displs[i] <
                         attr->u.hindexed.array_of_displs[min_displ_idx])
                    min_displ_idx = i;
            }
        }

        extent = (int64_t) attr->u.hindexed.array_of_displs[max_displ_idx] -
            (int64_t) attr->u.hindexed.array_of_displs[min_displ_idx] +
            (int64_t) attr->u.hindexed.array_of_blklens[max_displ_idx] * extent;

        if (VALUE_FITS_IN_AINT(extent * count))
            break;
    }

    rc = MPI_Type_create_hindexed(attr->u.hindexed.numblks,
                                  attr->u.hindexed.array_of_blklens,
                                  attr->u.hindexed.array_of_displs, type, newtype);
    DTPI_ERR_CHK_MPI_RC(rc);

    confirm_extent(*newtype, extent);

  fn_exit:
    *new_count = count;
    if (attr_tree_depth > 1) {
        rc = MPI_Type_free(&type);
        DTPI_ERR_CHK_MPI_RC(rc);
    }
    return rc;

  fn_fail:
    goto fn_exit;
}

static int construct_subarray(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s * attr,
                              MPI_Datatype * newtype, MPI_Aint * new_count)
{
    MPI_Datatype type;
    MPI_Aint count;
    DTPI_pool_s *dtpi = dtp.priv;
    int rc = DTP_SUCCESS;
    int64_t extent;

    /* setup the child type first */
    rc = DTPI_construct_datatype(dtp, attr_tree_depth - 1, &attr->child, &type, &count);
    DTPI_ERR_CHK_RC(rc);

    MPI_Aint tmp_lb;
    rc = MPI_Type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    int ndims_attr = DTPI_rand(dtpi) % DTPI_ATTR_SUBARRAY_NDIMS__LAST;

    if (ndims_attr == DTPI_ATTR_SUBARRAY_NDIMS__SMALL) {
        attr->u.subarray.ndims = 2;
    } else if (ndims_attr == DTPI_ATTR_SUBARRAY_NDIMS__LARGE) {
        attr->u.subarray.ndims = 4;
    } else {
        DTPI_ERR_ASSERT(0, rc);
    }

    DTPI_ALLOC_OR_FAIL(attr->u.subarray.array_of_sizes, attr->u.subarray.ndims * sizeof(int), rc);
    DTPI_ALLOC_OR_FAIL(attr->u.subarray.array_of_subsizes,
                       attr->u.subarray.ndims * sizeof(int), rc);
    DTPI_ALLOC_OR_FAIL(attr->u.subarray.array_of_starts, attr->u.subarray.ndims * sizeof(int), rc);

    MPI_Aint tmp_count = count;
    while (1) {
        extent = attr->child_type_extent;

        /* reset necessary variables in each iteration, in case we
         * need to go through this loop more than once */
        count = tmp_count;

        int subsizes_attr = DTPI_rand(dtpi) % DTPI_ATTR_SUBARRAY_SUBSIZES__LAST;
        if (subsizes_attr == DTPI_ATTR_SUBARRAY_SUBSIZES__ONE) {
            for (int i = 0; i < attr->u.subarray.ndims; i++) {
                attr->u.subarray.array_of_subsizes[i] = 1;
                count /= attr->u.subarray.array_of_subsizes[i];
            }
        } else if (subsizes_attr == DTPI_ATTR_SUBARRAY_SUBSIZES__SMALL) {
            for (int i = 0; i < attr->u.subarray.ndims; i++) {
                attr->u.subarray.array_of_subsizes[i] = DTPI_low_count(count);
                count /= attr->u.subarray.array_of_subsizes[i];
            }
        } else if (subsizes_attr == DTPI_ATTR_SUBARRAY_SUBSIZES__LARGE) {
            for (int i = 0; i < attr->u.subarray.ndims; i++) {
                attr->u.subarray.array_of_subsizes[i] = DTPI_high_count(count);
                count /= attr->u.subarray.array_of_subsizes[i];
            }
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        int sizes_attr = DTPI_rand(dtpi) % DTPI_ATTR_SUBARRAY_SIZES__LAST;
        if (sizes_attr == DTPI_ATTR_SUBARRAY_SIZES__ONE) {
            for (int i = 0; i < attr->u.subarray.ndims; i++) {
                attr->u.subarray.array_of_sizes[i] = attr->u.subarray.array_of_subsizes[i];
                attr->u.subarray.array_of_starts[i] = 0;
                extent *= attr->u.subarray.array_of_sizes[i];
            }
        } else if (sizes_attr == DTPI_ATTR_SUBARRAY_SIZES__SMALL) {
            for (int i = 0; i < attr->u.subarray.ndims; i++) {
                attr->u.subarray.array_of_sizes[i] = attr->u.subarray.array_of_subsizes[i] + 1;
                attr->u.subarray.array_of_starts[i] = 1;
                extent *= attr->u.subarray.array_of_sizes[i];
            }
        } else if (sizes_attr == DTPI_ATTR_SUBARRAY_SIZES__LARGE) {
            for (int i = 0; i < attr->u.subarray.ndims; i++) {
                attr->u.subarray.array_of_sizes[i] = attr->u.subarray.array_of_subsizes[i] * 4;
                attr->u.subarray.array_of_starts[i] = attr->u.subarray.array_of_subsizes[i];
                extent *= attr->u.subarray.array_of_sizes[i];
            }
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        if (VALUE_FITS_IN_AINT(extent * count))
            break;
    }

    int order_attr = DTPI_rand(dtpi) % DTPI_ATTR_SUBARRAY_ORDER__LAST;
    if (order_attr == DTPI_ATTR_SUBARRAY_ORDER__C) {
        attr->u.subarray.order = MPI_ORDER_C;
    } else if (order_attr == DTPI_ATTR_SUBARRAY_ORDER__FORTRAN) {
        attr->u.subarray.order = MPI_ORDER_FORTRAN;
    } else {
        DTPI_ERR_ASSERT(0, rc);
    }

    rc = MPI_Type_create_subarray(attr->u.subarray.ndims,
                                  attr->u.subarray.array_of_sizes,
                                  attr->u.subarray.array_of_subsizes,
                                  attr->u.subarray.array_of_starts,
                                  attr->u.subarray.order, type, newtype);
    DTPI_ERR_CHK_MPI_RC(rc);

    confirm_extent(*newtype, extent);

  fn_exit:
    *new_count = (MPI_Aint) count;
    if (attr_tree_depth > 1) {
        rc = MPI_Type_free(&type);
        DTPI_ERR_CHK_MPI_RC(rc);
    }
    return rc;

  fn_fail:
    goto fn_exit;
}

static int construct_struct(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s * attr,
                            MPI_Datatype * newtype, MPI_Aint * new_count)
{
    MPI_Datatype type;
    MPI_Aint count = 0;
    DTPI_pool_s *dtpi = dtp.priv;
    int rc = DTP_SUCCESS;
    int64_t extent;

    /* setup the child type first */
    rc = DTPI_construct_datatype(dtp, attr_tree_depth - 1, &attr->child, &type, &count);
    DTPI_ERR_CHK_RC(rc);

    MPI_Aint tmp_lb;
    rc = MPI_Type_get_extent(type, &tmp_lb, &attr->child_type_extent);
    DTPI_ERR_CHK_MPI_RC(rc);

    MPI_Aint tmp_count = count;
    attr->u.structure.array_of_blklens = NULL;
    attr->u.structure.array_of_displs = NULL;
    while (1) {
      retry:
        extent = attr->child_type_extent;

        /* reset necessary variables in each iteration, in case we
         * need to go through this loop more than once */
        count = tmp_count;
        if (attr->u.structure.array_of_blklens)
            DTPI_FREE(attr->u.structure.array_of_blklens);
        if (attr->u.structure.array_of_displs)
            DTPI_FREE(attr->u.structure.array_of_displs);

        int numblks_attr = DTPI_rand(dtpi) % DTPI_ATTR_STRUCTURE_NUMBLKS__LAST;
        if (numblks_attr == DTPI_ATTR_STRUCTURE_NUMBLKS__ONE) {
            attr->u.structure.numblks = 1;
        } else if (numblks_attr == DTPI_ATTR_STRUCTURE_NUMBLKS__SMALL) {
            attr->u.structure.numblks = DTPI_low_count(count);
        } else if (numblks_attr == DTPI_ATTR_STRUCTURE_NUMBLKS__LARGE) {
            attr->u.structure.numblks = DTPI_high_count(count);
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count /= attr->u.structure.numblks;

        DTPI_ALLOC_OR_FAIL(attr->u.structure.array_of_blklens,
                           attr->u.structure.numblks * sizeof(int), rc);

        int total_blklen = 0;
        int blklen_attr = DTPI_rand(dtpi) % DTPI_ATTR_STRUCTURE_BLKLEN__LAST;
        if (blklen_attr == DTPI_ATTR_STRUCTURE_BLKLEN__ONE) {
            for (int i = 0; i < attr->u.structure.numblks; i++) {
                attr->u.structure.array_of_blklens[i] = 1;
                total_blklen += attr->u.structure.array_of_blklens[i];
            }
        } else if (blklen_attr == DTPI_ATTR_STRUCTURE_BLKLEN__SMALL) {
            for (int i = 0; i < attr->u.structure.numblks; i++) {
                attr->u.structure.array_of_blklens[i] = DTPI_low_count(count);
                total_blklen += attr->u.structure.array_of_blklens[i];
            }
        } else if (blklen_attr == DTPI_ATTR_STRUCTURE_BLKLEN__LARGE) {
            for (int i = 0; i < attr->u.structure.numblks; i++) {
                attr->u.structure.array_of_blklens[i] = DTPI_high_count(count);
                total_blklen += attr->u.structure.array_of_blklens[i];
            }
        } else if (blklen_attr == DTPI_ATTR_STRUCTURE_BLKLEN__UNEVEN) {
            for (int i = 0; i < attr->u.structure.numblks; i++) {
                if (i % 2 == 0)
                    attr->u.structure.array_of_blklens[i] = DTPI_low_count(count) + 1;
                else
                    attr->u.structure.array_of_blklens[i] = DTPI_low_count(count) - 1;
                total_blklen += attr->u.structure.array_of_blklens[i];
            }
            if (attr->u.structure.numblks % 2) {
                /* if we have an odd number of blocks, adjust the counts */
                int idx = attr->u.structure.numblks - 1;
                attr->u.structure.array_of_blklens[idx]--;
                total_blklen--;
            }
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        count *= attr->u.structure.numblks;
        count /= total_blklen;

        DTPI_ALLOC_OR_FAIL(attr->u.structure.array_of_displs,
                           attr->u.structure.numblks * sizeof(MPI_Aint), rc);

        /* FIXME: we should detect the maximum alignment needed by the
         * compiler instead of arbitrarily incrementing the stride by
         * a constant value. */
        int64_t total_displ = 8;
        int displs_attr = DTPI_rand(dtpi) % DTPI_ATTR_STRUCTURE_DISPLS__LAST;
        if (displs_attr == DTPI_ATTR_STRUCTURE_DISPLS__SMALL) {
            for (int i = 0; i < attr->u.structure.numblks; i++) {
                attr->u.structure.array_of_displs[i] = (MPI_Aint) total_displ;
                total_displ +=
                    (int64_t) attr->child_type_extent * (attr->u.structure.array_of_blklens[i] + 1);
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_STRUCTURE_DISPLS__LARGE) {
            for (int i = 0; i < attr->u.structure.numblks; i++) {
                attr->u.structure.array_of_displs[i] = (MPI_Aint) total_displ;
                total_displ +=
                    (int64_t) attr->child_type_extent * (attr->u.structure.array_of_blklens[i] * 4);
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_STRUCTURE_DISPLS__REDUCING) {
            for (int i = 0; i < attr->u.structure.numblks; i++) {
                int idx = attr->u.structure.numblks - i - 1;
                attr->u.structure.array_of_displs[idx] = (MPI_Aint) total_displ;
                total_displ +=
                    (int64_t) attr->child_type_extent * (attr->u.structure.array_of_blklens[idx] +
                                                         1);
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else if (displs_attr == DTPI_ATTR_STRUCTURE_DISPLS__UNEVEN) {
            for (int i = 0; i < attr->u.structure.numblks; i++) {
                attr->u.structure.array_of_displs[i] = (MPI_Aint) total_displ;
                total_displ +=
                    (int64_t) attr->child_type_extent * (attr->u.structure.array_of_blklens[i] + i);
                if (!VALUE_FITS_IN_AINT(total_displ))
                    goto retry;
            }
        } else {
            DTPI_ERR_ASSERT(0, rc);
        }

        /* calculate the extent for our datatype, to decide whether we
         * want to use this or try again */
        int max_displ_idx = -1;
        int min_displ_idx = -1;
        for (int i = 0; i < attr->u.structure.numblks; i++) {
            if (attr->u.structure.array_of_blklens[i]) {
                if (max_displ_idx == -1)
                    max_displ_idx = i;
                else if (attr->u.structure.array_of_displs[i] >
                         attr->u.structure.array_of_displs[max_displ_idx])
                    max_displ_idx = i;

                if (min_displ_idx == -1)
                    min_displ_idx = i;
                else if (attr->u.structure.array_of_displs[i] <
                         attr->u.structure.array_of_displs[min_displ_idx])
                    min_displ_idx = i;
            }
        }

        extent =
            (int64_t) attr->u.structure.array_of_displs[max_displ_idx] -
            (int64_t) attr->u.structure.array_of_displs[min_displ_idx] +
            (int64_t) attr->u.structure.array_of_blklens[max_displ_idx] * extent;

        if (VALUE_FITS_IN_AINT(extent * count))
            break;
    }

    {
        MPI_Datatype *array_of_types;

        DTPI_ALLOC_OR_FAIL(array_of_types, attr->u.structure.numblks * sizeof(MPI_Datatype), rc);
        for (int i = 0; i < attr->u.structure.numblks; i++)
            array_of_types[i] = type;

        rc = MPI_Type_create_struct(attr->u.structure.numblks,
                                    attr->u.structure.array_of_blklens,
                                    attr->u.structure.array_of_displs, array_of_types, newtype);
        DTPI_ERR_CHK_MPI_RC(rc);

        DTPI_FREE(array_of_types);
    }

    confirm_extent(*newtype, extent);

  fn_exit:
    *new_count = count;
    if (attr_tree_depth > 1) {
        rc = MPI_Type_free(&type);
        DTPI_ERR_CHK_MPI_RC(rc);
    }
    return rc;

  fn_fail:
    goto fn_exit;
}

int DTPI_construct_datatype(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s ** attr_tree,
                            MPI_Datatype * newtype, MPI_Aint * new_count)
{
    DTPI_pool_s *dtpi = dtp.priv;
    int rc = DTP_SUCCESS;

    if (attr_tree_depth == 0) {
        *attr_tree = NULL;
        *newtype = dtp.DTP_base_type;
        *new_count = dtpi->base_type_count;
        goto fn_exit;
    }

    DTPI_Attr_s *attr;

    DTPI_ALLOC_OR_FAIL(attr, sizeof(DTPI_Attr_s), rc);
    *attr_tree = attr;

    attr->kind = DTPI_rand(dtpi) % DTPI_DATATYPE_KIND__LAST;

    switch (attr->kind) {
        case DTPI_DATATYPE_KIND__CONTIG:
            rc = construct_contig(dtp, attr_tree_depth, attr, newtype, new_count);
            DTPI_ERR_CHK_RC(rc);
            break;

        case DTPI_DATATYPE_KIND__DUP:
            rc = construct_dup(dtp, attr_tree_depth, attr, newtype, new_count);
            DTPI_ERR_CHK_RC(rc);
            break;

        case DTPI_DATATYPE_KIND__RESIZED:
            rc = construct_resized(dtp, attr_tree_depth, attr, newtype, new_count);
            DTPI_ERR_CHK_RC(rc);
            break;

        case DTPI_DATATYPE_KIND__VECTOR:
            rc = construct_vector(dtp, attr_tree_depth, attr, newtype, new_count);
            DTPI_ERR_CHK_RC(rc);
            break;

        case DTPI_DATATYPE_KIND__HVECTOR:
            rc = construct_hvector(dtp, attr_tree_depth, attr, newtype, new_count);
            DTPI_ERR_CHK_RC(rc);
            break;

        case DTPI_DATATYPE_KIND__BLKINDX:
            rc = construct_blkindx(dtp, attr_tree_depth, attr, newtype, new_count);
            DTPI_ERR_CHK_RC(rc);
            break;

        case DTPI_DATATYPE_KIND__BLKHINDX:
            rc = construct_blkhindx(dtp, attr_tree_depth, attr, newtype, new_count);
            DTPI_ERR_CHK_RC(rc);
            break;

        case DTPI_DATATYPE_KIND__INDEXED:
            rc = construct_indexed(dtp, attr_tree_depth, attr, newtype, new_count);
            DTPI_ERR_CHK_RC(rc);
            break;

        case DTPI_DATATYPE_KIND__HINDEXED:
            rc = construct_hindexed(dtp, attr_tree_depth, attr, newtype, new_count);
            DTPI_ERR_CHK_RC(rc);
            break;

        case DTPI_DATATYPE_KIND__SUBARRAY:
            rc = construct_subarray(dtp, attr_tree_depth, attr, newtype, new_count);
            DTPI_ERR_CHK_RC(rc);
            break;

        case DTPI_DATATYPE_KIND__STRUCT:
            rc = construct_struct(dtp, attr_tree_depth, attr, newtype, new_count);
            DTPI_ERR_CHK_RC(rc);
            break;

        default:
            DTPI_ERR_ASSERT(0, rc);
    }

  fn_exit:
    return rc;

  fn_fail:
    goto fn_exit;
}

static int attr_tree_free(DTPI_Attr_s * attr)
{
    int rc = DTP_SUCCESS;

    if (attr->child) {
        rc = attr_tree_free(attr->child);
        DTPI_ERR_CHK_RC(rc);
    }

    switch (attr->kind) {
        case DTPI_DATATYPE_KIND__BLKINDX:
            DTPI_FREE(attr->u.blkindx.array_of_displs);
            break;

        case DTPI_DATATYPE_KIND__BLKHINDX:
            DTPI_FREE(attr->u.blkhindx.array_of_displs);
            break;

        case DTPI_DATATYPE_KIND__INDEXED:
            DTPI_FREE(attr->u.indexed.array_of_blklens);
            DTPI_FREE(attr->u.indexed.array_of_displs);
            break;

        case DTPI_DATATYPE_KIND__HINDEXED:
            DTPI_FREE(attr->u.hindexed.array_of_blklens);
            DTPI_FREE(attr->u.hindexed.array_of_displs);
            break;

        case DTPI_DATATYPE_KIND__SUBARRAY:
            DTPI_FREE(attr->u.subarray.array_of_sizes);
            DTPI_FREE(attr->u.subarray.array_of_subsizes);
            DTPI_FREE(attr->u.subarray.array_of_starts);
            break;

        case DTPI_DATATYPE_KIND__STRUCT:
            DTPI_FREE(attr->u.structure.array_of_blklens);
            DTPI_FREE(attr->u.structure.array_of_displs);
            break;

        default:
            break;
    }

    DTPI_FREE(attr);

  fn_exit:
    return rc;

  fn_fail:
    goto fn_exit;
}

int DTPI_obj_free(DTPI_obj_s * obj_priv)
{
    int rc = DTP_SUCCESS;

    if (obj_priv->attr_tree) {
        rc = attr_tree_free(obj_priv->attr_tree);
        DTPI_ERR_CHK_RC(rc);
    }

  fn_exit:
    return rc;

  fn_fail:
    goto fn_exit;
}
