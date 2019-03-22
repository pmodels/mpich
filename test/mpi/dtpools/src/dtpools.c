/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <limits.h>
#include <assert.h>
#include "dtpools_internal.h"

#define DTPI_MAX_FACTOR (64)

/* get list of MPI datatypes at compilation time */
MPI_Datatype DTP_Basic_type[] = { DTP_MPI_DATATYPE };

/* pointer to create functions for datatypes */
static DTPI_Creator creators[DTPI_OBJ_TYPE__NUM];
static DTPI_Destructor destructors[DTPI_OBJ_TYPE__NUM];
static DTPI_Checker checkers[DTPI_OBJ_TYPE__NUM];
static int DTPI_Initialized = 0;

/* return factor of count */
static int DTPI_Get_max_fact(int count)
{
    int i = 2;

    if ((count / 2) < DTPI_MAX_FACTOR && (count / 2) > 0) {
        i = (count / 2) + 1;
    } else if ((count / 2) >= DTPI_MAX_FACTOR) {
        i = DTPI_MAX_FACTOR + 1;
    }

    while ((count % --i) != 0 && i > 1);

    return i;
}

/* default number of objects to keep the testing time reasonable */
#define DEFAULT_NUM_OBJS  (5)

static void get_obj_id_range(int real_max_idx, int *min_obj_idx_, int *max_obj_idx_)
{
    char *obj_id_str = NULL;
    int min_obj_idx, max_obj_idx;

    /* get number of objects from environment */
    obj_id_str = getenv("DTP_MIN_OBJ_ID");
    if (obj_id_str) {
        min_obj_idx = atoi(obj_id_str);
        assert(min_obj_idx >= 0);
    } else {
        min_obj_idx = 0;
    }
    if (min_obj_idx > real_max_idx)
        min_obj_idx = real_max_idx;

    obj_id_str = getenv("DTP_MAX_OBJ_ID");
    if (obj_id_str) {
        max_obj_idx = atoi(obj_id_str);
    } else {
        /* if the user did not give a max, pick a small count to keep
         * the tests run in a reasonable amount of time */
        max_obj_idx = min_obj_idx + DEFAULT_NUM_OBJS - 1;
    }
    if (max_obj_idx > real_max_idx)
        max_obj_idx = real_max_idx;

    assert(max_obj_idx >= min_obj_idx);

    *min_obj_idx_ = min_obj_idx;
    *max_obj_idx_ = max_obj_idx;
}

/*
 * DTP_pool_create - create a new datatype pool with specified signature
 *
 * basic_type:        signature's basic datatype
 * basic_type_count:  signature's basic datatype count
 * dtp:               returned datatype pool object
 */
int DTP_pool_create(MPI_Datatype basic_type, MPI_Aint basic_type_count, DTP_t * dtp)
{
    int i, err = DTP_SUCCESS;
    int num_objs;
    int min_obj_idx, max_obj_idx;
    int real_max_idx;
    int factor;
    int log_2 = 0;
    struct DTP_obj_array_s *obj_array = NULL;

    DTPI_OBJ_ALLOC_OR_FAIL(*dtp, sizeof(**dtp));

    /*
     * For nested types, e.g., MPI_Type_vector,
     * we use factor as blklen. The blklen at
     * level-i is a 2^i power, therefore factor
     * has to be a power of 2.
     * Check `dtpools_internal.c` for more details.
     */
    factor = DTPI_Get_max_fact(basic_type_count);
    while (factor > 1) {
        if (factor % 2 != 0)
            break;
        factor >>= 1;
        log_2++;
    }

    if (basic_type_count == 1) {
        real_max_idx = DTPI_OBJ_LAYOUT_SIMPLE__NUM - 1;
    } else if (log_2 >= DTPI_MAX_TYPE_NESTING) {
        real_max_idx = DTPI_OBJ_LAYOUT_LARGE_NESTED__NUM - 1;
    } else {
        real_max_idx = DTPI_OBJ_LAYOUT_LARGE__NUM - 1;
    }
    get_obj_id_range(real_max_idx, &min_obj_idx, &max_obj_idx);
    num_objs = max_obj_idx - min_obj_idx + 1;

    DTPI_OBJ_ALLOC_OR_FAIL(obj_array, sizeof(*obj_array) * num_objs);

    for (i = 0; i < num_objs; i++) {
        obj_array[i].DTP_obj_type = MPI_DATATYPE_NULL;
        obj_array[i].DTP_obj_count = 0;
        obj_array[i].DTP_obj_buf = NULL;
        obj_array[i].private_info = NULL;
    }

    (*dtp)->min_obj_idx = min_obj_idx;
    (*dtp)->max_obj_idx = max_obj_idx;
    (*dtp)->DTP_num_objs = num_objs;
    (*dtp)->DTP_pool_type = DTP_POOL_TYPE__BASIC;
    (*dtp)->DTP_type_signature.DTP_pool_basic.DTP_basic_type = basic_type;
    (*dtp)->DTP_type_signature.DTP_pool_basic.DTP_basic_type_count = basic_type_count;
    (*dtp)->DTP_obj_array = (struct DTP_obj_array_s *) obj_array;

    if (!DTPI_Initialized) {
        DTPI_Init_creators(creators);
        DTPI_Init_destructors(destructors);
        DTPI_Init_checkers(checkers);
        DTPI_Initialized = 1;
    }

  fn_exit:
    return err;

  fn_fail:
    if (*dtp) {
        free(*dtp);
    }
    if (obj_array) {
        free(obj_array);
    }
    goto fn_exit;
}

/*
 * DTP_pool_create_struct - create a new struct pool with specified signature
 *
 * num_types:          number of basic datatypes in struct
 * basic_types:        array of basic datatypes in struct
 * basic_type_counts:  array of counts for each of the
 *                     basic datatypes in struct
 * dtp:                returned datatype pool object
 */
int DTP_pool_create_struct(int num_types, MPI_Datatype * basic_types, int *basic_type_counts,
                           DTP_t * dtp)
{
    int i, err = DTP_SUCCESS;
    int num_objs;
    int min_obj_idx, max_obj_idx;
    struct DTP_obj_array_s *obj_array = NULL;

    DTPI_OBJ_ALLOC_OR_FAIL(*dtp, sizeof(**dtp));

    get_obj_id_range(DTPI_OBJ_LAYOUT__STRUCT_NUM - 1, &min_obj_idx, &max_obj_idx);
    num_objs = max_obj_idx - min_obj_idx + 1;

    DTPI_OBJ_ALLOC_OR_FAIL(obj_array, sizeof(*obj_array) * num_objs);

    for (i = 0; i < num_objs; i++) {
        obj_array[i].DTP_obj_type = MPI_DATATYPE_NULL;
        obj_array[i].DTP_obj_count = 0;
        obj_array[i].DTP_obj_buf = NULL;
        obj_array[i].private_info = NULL;
    }

    (*dtp)->DTP_num_objs = num_objs;
    (*dtp)->DTP_pool_type = DTP_POOL_TYPE__STRUCT;
    (*dtp)->DTP_type_signature.DTP_pool_struct.DTP_num_types = num_types;
    (*dtp)->DTP_type_signature.DTP_pool_struct.DTP_basic_type = basic_types;
    (*dtp)->DTP_type_signature.DTP_pool_struct.DTP_basic_type_count = basic_type_counts;
    (*dtp)->DTP_obj_array = (struct DTP_obj_array_s *) obj_array;

  fn_exit:
    return err;

  fn_fail:
    if (*dtp) {
        free(*dtp);
    }
    if (obj_array) {
        free(obj_array);
    }
    goto fn_exit;
}

/*
 * DTP_pool_free - free previously created pool
 */
int DTP_pool_free(DTP_t dtp)
{
    int err = DTP_SUCCESS;

    if (!dtp) {
        return DTP_ERR_OTHER;
    }
    free(dtp->DTP_obj_array);
    free(dtp);

    return err;
}

/*
 * DTP_obj_create - create obj_idx datatype inside the pool
 *
 * dtp:         datatype pool handle
 * obj_idx:     index of the created datatype inside the pool
 * val_start:   start value for initialization of buffer
 * val_stride:  increment between one buffer element and the following
 * val_count:   number of elements to initialize in buffer
 *
 * NOTE1: for pools created with `DTP_pool_create_struct` `val_count` is
 *        disregarded, `basic_type_counts` passed to the pool create
 *        function is used instead for buffer initialization.
 * NOTE2: for receive datatypes 'val_count' can be set to 0. Every buffer
 *        element in any case is initialized to 0 using 'memset'.
 *        'val_count' should be set to > 0 only for send datatypes.
 *
 */
int DTP_obj_create(DTP_t dtp, int user_obj_idx, int val_start, int val_stride, MPI_Aint val_count)
{
    int err = DTP_SUCCESS;
    int basic_type_count;
    int factor;
    int obj_type;
    int obj_idx = user_obj_idx + dtp->min_obj_idx;
    struct DTPI_Par par;

    /* init user defined params */
    par.user.val_start = val_start;
    par.user.val_stride = val_stride;
    par.user.val_count = val_count;
    par.user.obj_idx = user_obj_idx;

    if (dtp->DTP_pool_type == DTP_POOL_TYPE__BASIC) {
        /* get type signature for pool */
        basic_type_count = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type_count;

        /* get biggest factor for count */
        factor = DTPI_Get_max_fact(basic_type_count);

        switch (obj_idx) {
            case DTPI_OBJ_LAYOUT_SIMPLE__BASIC:
            case DTPI_OBJ_LAYOUT_SIMPLE__CONTIG:
                par.core.type_count = basic_type_count;
                par.core.type_blklen = 1;
                par.core.type_stride = 1;
                break;
            case DTPI_OBJ_LAYOUT_SIMPLE__VECTOR:
            case DTPI_OBJ_LAYOUT_SIMPLE__HVECTOR:
            case DTPI_OBJ_LAYOUT_SIMPLE__INDEXED:
            case DTPI_OBJ_LAYOUT_SIMPLE__HINDEXED:
            case DTPI_OBJ_LAYOUT_SIMPLE__BLOCK_INDEXED:
            case DTPI_OBJ_LAYOUT_SIMPLE__BLOCK_HINDEXED:
                par.core.type_count = basic_type_count;
                par.core.type_blklen = 1;
                par.core.type_stride = 2;
                break;
            case DTPI_OBJ_LAYOUT_LARGE_BLK__VECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__HVECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__BLOCK_INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__BLOCK_HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__SUBARRAY_C:
            case DTPI_OBJ_LAYOUT_LARGE_BLK__SUBARRAY_F:
                if (factor > (basic_type_count / factor)) {
                    par.core.type_count = basic_type_count / factor;
                    par.core.type_blklen = factor;
                } else {
                    par.core.type_count = factor;
                    par.core.type_blklen = basic_type_count / factor;
                }
                par.core.type_stride = par.core.type_blklen + 1;
                break;
            case DTPI_OBJ_LAYOUT_LARGE_CNT__VECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__HVECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__BLOCK_INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__BLOCK_HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__SUBARRAY_C:
            case DTPI_OBJ_LAYOUT_LARGE_CNT__SUBARRAY_F:
                if (factor > (basic_type_count / factor)) {
                    par.core.type_count = factor;
                    par.core.type_blklen = basic_type_count / factor;
                } else {
                    par.core.type_count = basic_type_count / factor;
                    par.core.type_blklen = factor;
                }
                par.core.type_stride = par.core.type_blklen + 1;
                break;
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__VECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__HVECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__BLOCK_INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__BLOCK_HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__SUBARRAY_C:
            case DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__SUBARRAY_F:
                if (factor > (basic_type_count / factor)) {
                    par.core.type_count = basic_type_count / factor;
                    par.core.type_blklen = factor;
                } else {
                    par.core.type_count = factor;
                    par.core.type_blklen = basic_type_count / factor;
                }
                par.core.type_stride = par.core.type_blklen * 4;
                break;
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__VECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__HVECTOR:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__BLOCK_INDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__BLOCK_HINDEXED:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__SUBARRAY_C:
            case DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__SUBARRAY_F:
                if (factor > (basic_type_count / factor)) {
                    par.core.type_count = factor;
                    par.core.type_blklen = basic_type_count / factor;
                } else {
                    par.core.type_count = basic_type_count / factor;
                    par.core.type_blklen = factor;
                }
                par.core.type_stride = par.core.type_blklen * 4;
                break;
            case DTPI_OBJ_LAYOUT_LARGE_BLK__NESTED_VECTOR_3L:
                /*
                 * NOTE 1: this is a three level nested vector type.
                 *         This means that, including level 0, we
                 *         have four (4) calls to `MPI_Type_vector`.
                 * NOTE 2: right now max factor is 64, thus we can
                 *         nest up to 6 vector calls. To increase
                 *         the nesting level max factor should be
                 *         increased first.
                 */
                par.core.type_count = basic_type_count / factor;
                par.core.type_blklen = factor;
                par.core.type_stride = par.core.type_blklen + 1;
                par.core.type_nesting = 3;
                break;
            default:
                err = DTP_ERR_OTHER;
                fprintf(stdout, "Type layout %d is not defined\n", obj_idx);
                fflush(stdout);
                goto fn_exit;
        }

        /* get type of object for creator array */
        DTPI_GET_BASIC_OBJ_TYPE_FROM_IDX(obj_idx, obj_type);

        err = creators[obj_type] (&par, dtp);
    } else {
        switch (obj_idx) {
            case DTPI_OBJ_LAYOUT_SIMPLE__STRUCT:
                /* TODO: add other struct layouts here */
                break;
            default:
                err = DTP_ERR_OTHER;
                fprintf(stdout, "Type layout %d is not defined\n", obj_idx);
                fflush(stdout);
                goto fn_exit;
        }
        err = DTPI_Struct_create(&par, dtp);
    }

  fn_exit:
    return err;
}

/*
 * DTP_obj_free - free previously created datatype obj_idx
 */
int DTP_obj_free(DTP_t dtp, int user_obj_idx)
{
    int err = DTP_SUCCESS;
    int obj_type;
    int obj_idx = user_obj_idx + dtp->min_obj_idx;
    DTPI_t *dtpi;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[user_obj_idx].private_info;
    obj_type = dtpi->obj_type;

    if (dtp->DTP_pool_type == DTP_POOL_TYPE__BASIC) {
        err = destructors[obj_type] (dtp, user_obj_idx);
    } else {
        err = DTPI_Struct_free(dtp, obj_idx);
    }

    return err;
}

/*
 * DTP_obj_buf_check - check receive buffer to very correctness
 *
 * dtp:         datatype pool handle
 * obj_idx:     index of the datatype buffer to be checked
 * val_start:   start value for checking of buffer
 * val_stride:  increment between one buffer element and the following
 * val_count:   number of elements to check in buffer
 *
 * NOTE: for pools created with `DTP_pool_create_struct` `val_count` is
 *       disregarded, `basic_type_counts` passed to the pool create
 *       function is used instead to check buffer.
 */
int DTP_obj_buf_check(DTP_t dtp, int user_obj_idx, int val_start, int val_stride,
                      MPI_Aint val_count)
{
    int err = DTP_SUCCESS;
    struct DTPI_Par par;
    DTPI_t *dtpi;
    DTP_pool_type pool_type;
    DTPI_obj_type_e obj_type;
    int obj_idx = user_obj_idx + dtp->min_obj_idx;

    par.user.val_start = val_start;
    par.user.val_count = val_count;
    par.user.val_stride = val_stride;
    par.user.obj_idx = user_obj_idx;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[user_obj_idx].private_info;
    obj_type = dtpi->obj_type;

    if (dtp->DTP_pool_type == DTP_POOL_TYPE__BASIC) {
        err = checkers[obj_type] (&par, dtp);
    } else {
        err = DTPI_Struct_check_buf(&par, dtp);
    }

    return err;
}
