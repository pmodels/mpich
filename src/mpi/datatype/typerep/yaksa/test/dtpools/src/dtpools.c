/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "dtpools_internal.h"


/*
 * DTP_pool_create - create a new datatype pool with specified signature
 *
 * base_type_str:    signature's base datatype string
 * base_type_count:  signature's base datatype count
 * dtp:              returned datatype pool object
 */
int DTP_pool_create(const char *base_type_str, uintptr_t base_type_count, int seed,
                    DTP_pool_s * dtp)
{
    int rc = DTP_SUCCESS;

    DTPI_FUNC_ENTER();

    DTPI_ERR_ARG_CHECK(!dtp, rc);
    DTPI_ERR_ARG_CHECK(base_type_count <= 0, rc);

    DTPI_ALLOC_OR_FAIL(dtp->priv, sizeof(DTPI_pool_s), rc);
    DTPI_pool_s *dtpi = dtp->priv;

    rc = DTPI_parse_base_type_str(dtp, base_type_str);
    DTPI_ERR_CHK_RC(rc);

    /* setup the random number generation parameters */
    srand(seed);
    for (int i = 0; i < DTPI_RAND_LIST_SIZE; i++)
        dtpi->rand_list[i] = rand();
    dtpi->rand_idx = 0;

    dtpi->base_type_count = base_type_count;

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    if (dtp) {
        DTPI_FREE(dtp);
        dtp = NULL;
    }
    goto fn_exit;
}


/*
 * DTP_pool_free - free previously created pool
 */
int DTP_pool_free(DTP_pool_s dtp)
{
    int rc = DTP_SUCCESS;
    DTPI_pool_s *dtpi = dtp.priv;

    DTPI_FUNC_ENTER();

    DTPI_ERR_ARG_CHECK(!dtpi, rc);

    if (dtpi->base_type_is_struct) {
        rc = yaksa_type_free(dtp.DTP_base_type);
        DTPI_ERR_CHK_RC(rc);

        DTPI_FREE(dtpi->base_type_attrs.array_of_blklens);
        DTPI_FREE(dtpi->base_type_attrs.array_of_displs);
        DTPI_FREE(dtpi->base_type_attrs.array_of_types);
    }
    DTPI_FREE(dtpi);

    DTPI_FUNC_EXIT();

  fn_exit:
    return rc;

  fn_fail:
    goto fn_exit;
}

#define DTPI_DEFAULT_MAX_TREE_DEPTH (3)
#define DTPI_DEFAULT_MAX_BUFSIZE    (1024 * 1024 * 1024)

/*
 * DTP_obj_create - create dtp object inside the pool
 *
 * dtp:         datatype pool handle
 * obj:         created object handle
 */
int DTP_obj_create(DTP_pool_s dtp, DTP_obj_s * obj, uintptr_t maxbufsize)
{
    int rc = DTP_SUCCESS;
    DTPI_pool_s *dtpi = dtp.priv;
    int max_tree_depth;
    DTPI_obj_s *obj_priv = NULL;

    DTPI_FUNC_ENTER();

    DTPI_ERR_ARG_CHECK(!dtpi, rc);

    /* The code will enter deadloop if maxbufsize is too small or overflown to negative.
     * Use 100000 as an arbitrary sanity guard.
     */
    DTPI_ERR_ARG_CHECK(maxbufsize < 100000, rc);

    /* find number of nestings */
    if (getenv("DTP_MAX_TREE_DEPTH"))
        max_tree_depth = atoi(getenv("DTP_MAX_TREE_DEPTH"));
    else
        max_tree_depth = DTPI_DEFAULT_MAX_TREE_DEPTH;

    int attr_tree_depth = DTPI_rand(dtpi) % (max_tree_depth + 1);

    while (1) {
        DTPI_ALLOC_OR_FAIL(obj->priv, sizeof(DTPI_obj_s), rc);
        obj_priv = obj->priv;

        obj_priv->dtp = dtp;

        rc = DTPI_construct_datatype(dtp, attr_tree_depth, &obj_priv->attr_tree,
                                     &obj->DTP_datatype, &obj->DTP_type_count);
        DTPI_ERR_CHK_RC(rc);

        /* find the buffer size that we need */
        intptr_t true_lb;
        intptr_t true_extent;
        intptr_t lb;
        intptr_t extent;

        rc = yaksa_type_get_true_extent(obj->DTP_datatype, &true_lb, &true_extent);
        DTPI_ERR_CHK_RC(rc);

        rc = yaksa_type_get_extent(obj->DTP_datatype, &lb, &extent);
        DTPI_ERR_CHK_RC(rc);

        obj->DTP_bufsize = (extent * obj->DTP_type_count) + true_extent - extent;

        /* if the true_lb occurs after the actual buffer start, make
         * sure we allocate some additional bytes to accommodate for
         * the data.  if the true_lb occurs before the actual buffer
         * start, we need to give an offset to the user to start
         * sending or receiving from. */
        if (true_lb > 0) {
            obj->DTP_bufsize += true_lb;
            obj->DTP_buf_offset = 0;
        } else {
            obj->DTP_buf_offset = -true_lb;
        }

        if (obj->DTP_bufsize <= maxbufsize) {
            break;
        } else {
            rc = DTP_obj_free(*obj);
            DTPI_ERR_CHK_RC(rc);
        }
    }

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

int DTP_obj_create_custom(DTP_pool_s dtp, DTP_obj_s * obj, const char *desc)
{
    int rc = DTP_SUCCESS;
    DTPI_FUNC_ENTER();

    DTPI_pool_s *dtpi = dtp.priv;
    DTPI_ERR_ARG_CHECK(!dtpi, rc);

    DTPI_ALLOC_OR_FAIL(obj->priv, sizeof(DTPI_obj_s), rc);
    DTPI_obj_s *obj_priv = obj->priv;
    obj_priv->dtp = dtp;
    if (desc == NULL) {
        obj_priv->attr_tree = NULL;
        obj->DTP_datatype = dtp.DTP_base_type;
        obj->DTP_type_count = dtpi->base_type_count;
    } else {
        const char *desc_parts[10];
        int num_parts;
        rc = DTPI_parse_desc(desc, desc_parts, &num_parts, 10);
        DTPI_ERR_CHK_RC(rc);
        rc = DTPI_custom_datatype(dtp, &obj_priv->attr_tree, &obj->DTP_datatype,
                                  &obj->DTP_type_count, desc_parts, num_parts);
        DTPI_ERR_CHK_RC(rc);
    }

    intptr_t true_lb;
    intptr_t true_extent;
    intptr_t lb;
    intptr_t extent;

    rc = yaksa_type_get_true_extent(obj->DTP_datatype, &true_lb, &true_extent);
    DTPI_ERR_CHK_RC(rc);
    rc = yaksa_type_get_extent(obj->DTP_datatype, &lb, &extent);
    DTPI_ERR_CHK_RC(rc);

    obj->DTP_bufsize = (extent * obj->DTP_type_count) + true_extent - extent;
    if (true_lb > 0) {
        obj->DTP_bufsize += true_lb;
        obj->DTP_buf_offset = 0;
    } else {
        obj->DTP_buf_offset = -true_lb;
    }

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

int DTP_obj_get_description(DTP_obj_s obj, char **desc)
{
    DTPI_obj_s *obj_priv = obj.priv;
    int rc = DTP_SUCCESS;

    rc = DTPI_populate_dtp_desc(obj_priv, obj_priv->dtp.priv, desc);
    DTPI_ERR_CHK_RC(rc);

  fn_exit:
    return rc;

  fn_fail:
    goto fn_exit;
}

/*
 * DTP_obj_free - free previously created datatype idx
 */
int DTP_obj_free(DTP_obj_s obj)
{
    DTPI_obj_s *obj_priv = (DTPI_obj_s *) obj.priv;
    int rc = DTP_SUCCESS;

    DTPI_FUNC_ENTER();

    if (obj_priv->dtp.DTP_base_type != obj.DTP_datatype) {
        rc = yaksa_type_free(obj.DTP_datatype);
        DTPI_ERR_CHK_RC(rc);
    }

    DTPI_obj_free(obj_priv);
    DTPI_FREE(obj.priv);

    DTPI_FUNC_EXIT();

  fn_exit:
    return rc;

  fn_fail:
    goto fn_exit;
}

/*
 * DTP_obj_buf_init - initialize buffer pattern
 *
 * obj:         dtpool object handle
 * val_start:   start value for checking of buffer
 * val_stride:  increment between one buffer element and the following
 * val_count:   number of elements to check in buffer
 */
int DTP_obj_buf_init(DTP_obj_s obj, void *buf, int val_start, int val_stride, uintptr_t val_count)
{
    intptr_t lb;
    intptr_t extent;
    int val = val_start;
    int rem_val_count = val_count;
    DTPI_obj_s *obj_priv = obj.priv;
    int rc = DTP_SUCCESS;

    DTPI_FUNC_ENTER();

    yaksa_type_get_extent(obj.DTP_datatype, &lb, &extent);
    for (int i = 0; i < obj.DTP_type_count; i++) {
        rc = DTPI_init_verify(obj_priv->dtp, obj, buf, obj_priv->attr_tree,
                              obj.DTP_buf_offset + i * extent, &val, val_stride, &rem_val_count, 0);
        DTPI_ERR_CHK_RC(rc);
    }

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

/*
 * DTP_obj_buf_check - check buffer pattern
 *
 * obj:         dtpool object handle
 * val_start:   start value for checking of buffer
 * val_stride:  increment between one buffer element and the following
 * val_count:   number of elements to check in buffer
 */
int DTP_obj_buf_check(DTP_obj_s obj, void *buf, int val_start, int val_stride, uintptr_t val_count)
{
    intptr_t lb;
    intptr_t extent;
    int val = val_start;
    int rem_val_count = val_count;
    DTPI_obj_s *obj_priv = obj.priv;
    int rc = DTP_SUCCESS;

    DTPI_FUNC_ENTER();

    yaksa_type_get_extent(obj.DTP_datatype, &lb, &extent);
    for (int i = 0; i < obj.DTP_type_count; i++) {
        rc = DTPI_init_verify(obj_priv->dtp, obj, buf, obj_priv->attr_tree,
                              obj.DTP_buf_offset + i * extent, &val, val_stride, &rem_val_count, 1);
        DTPI_ERR_CHK_RC(rc);
    }

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}
