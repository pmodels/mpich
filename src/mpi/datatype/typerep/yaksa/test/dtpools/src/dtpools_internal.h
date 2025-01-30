/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef DTPOOLS_INTERNAL_H_INCLUDED
#define DTPOOLS_INTERNAL_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "yaksa.h"
#include "dtpools.h"

extern int DTPI_func_nesting;

/* Error checking macros */
#define DTPI_ERR_SETANDJUMP(rc_, errcode_)      \
    do {                                        \
        rc_ = errcode_;                         \
        goto fn_fail;                           \
    } while (0)

#define DTPI_ERR_CHKANDJUMP(check_, rc_, errcode_)      \
    do {                                                \
        if (check_) {                                   \
            DTPI_ERR_SETANDJUMP(rc_, errcode_);         \
        }                                               \
    } while (0)

#define DTPI_ERR_CHK_RC(rc_)                    \
    do {                                        \
        if (rc_) {                              \
            goto fn_fail;                       \
        }                                       \
    } while (0)

#define DTPI_ERR_ARG_CHECK(check_, rc_)                                 \
    do {                                                                \
        if (check_) {                                                   \
            fprintf(stderr, "argument check failed in function %s, line %d\n", __func__, __LINE__); \
            DTPI_ERR_SETANDJUMP(rc_, DTP_ERR_ARG);                      \
        }                                                               \
    } while (0)

#define DTPI_ERR_ASSERT(assertion_, rc_)                                \
    do {                                                                \
        if (!(assertion_)) {                                            \
            fprintf(stderr, "assertion %s failed in function %s, line %d\n", #assertion_, __func__, __LINE__); \
            DTPI_ERR_SETANDJUMP(rc_, DTP_ERR_OTHER);                    \
        }                                                               \
    } while (0)


/* #define DEBUG_DTPOOLS */
#ifdef DEBUG_DTPOOLS
#define DTPI_debug(...)                                 \
    do {                                                \
        for (int i_ = 0; i_ < DTPI_func_nesting; i_++)  \
            printf("    ");                             \
        printf("[%s,%d] ", __func__, __LINE__);         \
        printf(__VA_ARGS__);                            \
    } while (0)
#else
#define DTPI_debug(...)
#endif

#define DTPI_FUNC_ENTER()                       \
    do {                                        \
        DTPI_func_nesting++;                    \
        DTPI_debug("entering function\n");      \
    } while (0)

#define DTPI_FUNC_EXIT()                        \
    do {                                        \
        DTPI_debug("exiting function\n");       \
        DTPI_func_nesting--;                    \
    } while (0)

#define TMP_STR_LEN      (1024)
#define DTPI_snprintf(rc_, tstr, cur_len, max_len, ...)                 \
    do {                                                                \
        char tmp_[TMP_STR_LEN];                                         \
        snprintf(tmp_, TMP_STR_LEN, __VA_ARGS__);                       \
        while (strlen(tmp_) >= max_len - cur_len - 1) {                 \
            char *t_, *t2_;                                             \
            max_len *= 2;                                               \
            DTPI_ALLOC_OR_FAIL(t_, max_len, rc_);                       \
            strncpy(t_, tstr, cur_len + 1);                             \
            t2_ = tstr;                                                 \
            tstr = t_;                                                  \
            DTPI_FREE(t2_);                                             \
        }                                                               \
        DTPI_ERR_ASSERT(strlen(tmp_) < max_len - cur_len - 1, rc_);     \
        strncpy(tstr + cur_len, tmp_, max_len - cur_len);               \
        cur_len += strlen(tmp_);                                        \
    } while (0)

#define DTPI_ALLOC_OR_FAIL(obj, size, rc_)                              \
    do {                                                                \
        obj = malloc(size);                                             \
        DTPI_ERR_CHKANDJUMP(!obj, rc_, DTP_ERR_OUT_OF_RESOURCES);       \
        memset(obj, 0, size);                                           \
    } while (0)

#define DTPI_FREE(obj)                          \
    do {                                        \
        free(obj);                              \
    } while (0)

#define DTPI_RAND_LIST_SIZE  (1024 * 1024)
#define DTPI_MAX_BASE_TYPE_STR_LEN  (128)

typedef struct {
    int rand_idx;
    int rand_list[DTPI_RAND_LIST_SIZE];

    char base_type_str[DTPI_MAX_BASE_TYPE_STR_LEN + 1];
    uintptr_t base_type_count;
    intptr_t base_type_extent;

    /* in case the base type is a struct */
    int base_type_is_struct;
    struct {
        intptr_t numblks;
        intptr_t *array_of_blklens;
        intptr_t *array_of_displs;
        yaksa_type_t *array_of_types;
    } base_type_attrs;
} DTPI_pool_s;

typedef enum {
    DTPI_DATATYPE_KIND__CONTIG = 0,
    DTPI_DATATYPE_KIND__DUP,
    DTPI_DATATYPE_KIND__RESIZED,
    DTPI_DATATYPE_KIND__VECTOR,
    DTPI_DATATYPE_KIND__HVECTOR,
    DTPI_DATATYPE_KIND__BLKINDX,
    DTPI_DATATYPE_KIND__BLKHINDX,
    DTPI_DATATYPE_KIND__INDEXED,
    DTPI_DATATYPE_KIND__HINDEXED,
    DTPI_DATATYPE_KIND__SUBARRAY,
    DTPI_DATATYPE_KIND__STRUCT,
    DTPI_DATATYPE_KIND__LAST,
} DTPI_Datatype_kind_e;


/* contig attributes */
enum {
    DTPI_ATTR_CONTIG_BLKLEN__ONE = 0,
    DTPI_ATTR_CONTIG_BLKLEN__SMALL,
    DTPI_ATTR_CONTIG_BLKLEN__LARGE,
    DTPI_ATTR_CONTIG_BLKLEN__LAST,
};


/* resized attributes */
enum {
    DTPI_ATTR_RESIZED_LB__PACKED = 0,
    DTPI_ATTR_RESIZED_LB__LOW,
    DTPI_ATTR_RESIZED_LB__VERY_LOW,
    DTPI_ATTR_RESIZED_LB__HIGH,
    DTPI_ATTR_RESIZED_LB__VERY_HIGH,
    DTPI_ATTR_RESIZED_LB__LAST,
};
enum {
    DTPI_ATTR_RESIZED_EXTENT__PACKED = 0,
    DTPI_ATTR_RESIZED_EXTENT__HIGH,
    DTPI_ATTR_RESIZED_EXTENT__VERY_HIGH,
    DTPI_ATTR_RESIZED_EXTENT__NEGATIVE_LARGE,
    DTPI_ATTR_RESIZED_EXTENT__NEGATIVE_SMALL,
    DTPI_ATTR_RESIZED_EXTENT__LAST,
};


/* vector attributes */
enum {
    DTPI_ATTR_VECTOR_NUMBLKS__ONE = 0,
    DTPI_ATTR_VECTOR_NUMBLKS__SMALL,
    DTPI_ATTR_VECTOR_NUMBLKS__LARGE,
    DTPI_ATTR_VECTOR_NUMBLKS__LAST,
};
enum {
    DTPI_ATTR_VECTOR_BLKLEN__ONE = 0,
    DTPI_ATTR_VECTOR_BLKLEN__SMALL,
    DTPI_ATTR_VECTOR_BLKLEN__LARGE,
    DTPI_ATTR_VECTOR_BLKLEN__LAST,
};
enum {
    DTPI_ATTR_VECTOR_STRIDE__SMALL = 0,
    DTPI_ATTR_VECTOR_STRIDE__LARGE,
    DTPI_ATTR_VECTOR_STRIDE__NEGATIVE_LARGE,
    DTPI_ATTR_VECTOR_STRIDE__NEGATIVE_SMALL,
    DTPI_ATTR_VECTOR_STRIDE__LAST,
};


/* hvector attributes */
enum {
    DTPI_ATTR_HVECTOR_NUMBLKS__ONE = 0,
    DTPI_ATTR_HVECTOR_NUMBLKS__SMALL,
    DTPI_ATTR_HVECTOR_NUMBLKS__LARGE,
    DTPI_ATTR_HVECTOR_NUMBLKS__LAST,
};
enum {
    DTPI_ATTR_HVECTOR_BLKLEN__ONE = 0,
    DTPI_ATTR_HVECTOR_BLKLEN__SMALL,
    DTPI_ATTR_HVECTOR_BLKLEN__LARGE,
    DTPI_ATTR_HVECTOR_BLKLEN__LAST,
};
enum {
    DTPI_ATTR_HVECTOR_STRIDE__SMALL = 0,
    DTPI_ATTR_HVECTOR_STRIDE__LARGE,
    DTPI_ATTR_HVECTOR_STRIDE__NEGATIVE_LARGE,
    DTPI_ATTR_HVECTOR_STRIDE__NEGATIVE_SMALL,
    DTPI_ATTR_HVECTOR_STRIDE__LAST,
};


/* block indexed attributes */
enum {
    DTPI_ATTR_BLKINDX_NUMBLKS__ONE = 0,
    DTPI_ATTR_BLKINDX_NUMBLKS__SMALL,
    DTPI_ATTR_BLKINDX_NUMBLKS__LARGE,
    DTPI_ATTR_BLKINDX_NUMBLKS__LAST,
};
enum {
    DTPI_ATTR_BLKINDX_BLKLEN__ONE = 0,
    DTPI_ATTR_BLKINDX_BLKLEN__SMALL,
    DTPI_ATTR_BLKINDX_BLKLEN__LARGE,
    DTPI_ATTR_BLKINDX_BLKLEN__LAST,
};
enum {
    DTPI_ATTR_BLKINDX_DISPLS__SMALL = 0,
    DTPI_ATTR_BLKINDX_DISPLS__LARGE,
    DTPI_ATTR_BLKINDX_DISPLS__REDUCING,
    DTPI_ATTR_BLKINDX_DISPLS__REVERSE,
    DTPI_ATTR_BLKINDX_DISPLS__UNEVEN,
    DTPI_ATTR_BLKINDX_DISPLS__LAST,
};


/* block hindexed attributes */
enum {
    DTPI_ATTR_BLKHINDX_NUMBLKS__ONE = 0,
    DTPI_ATTR_BLKHINDX_NUMBLKS__SMALL,
    DTPI_ATTR_BLKHINDX_NUMBLKS__LARGE,
    DTPI_ATTR_BLKHINDX_NUMBLKS__LAST,
};
enum {
    DTPI_ATTR_BLKHINDX_BLKLEN__ONE = 0,
    DTPI_ATTR_BLKHINDX_BLKLEN__SMALL,
    DTPI_ATTR_BLKHINDX_BLKLEN__LARGE,
    DTPI_ATTR_BLKHINDX_BLKLEN__LAST,
};
enum {
    DTPI_ATTR_BLKHINDX_DISPLS__SMALL = 0,
    DTPI_ATTR_BLKHINDX_DISPLS__LARGE,
    DTPI_ATTR_BLKHINDX_DISPLS__REDUCING,
    DTPI_ATTR_BLKHINDX_DISPLS__REVERSE,
    DTPI_ATTR_BLKHINDX_DISPLS__UNEVEN,
    DTPI_ATTR_BLKHINDX_DISPLS__LAST,
};


/* indexed attributes */
enum {
    DTPI_ATTR_INDEXED_NUMBLKS__ONE = 0,
    DTPI_ATTR_INDEXED_NUMBLKS__SMALL,
    DTPI_ATTR_INDEXED_NUMBLKS__LARGE,
    DTPI_ATTR_INDEXED_NUMBLKS__LAST,
};
enum {
    DTPI_ATTR_INDEXED_BLKLEN__ONE = 0,
    DTPI_ATTR_INDEXED_BLKLEN__SMALL,
    DTPI_ATTR_INDEXED_BLKLEN__LARGE,
    DTPI_ATTR_INDEXED_BLKLEN__UNEVEN,
    DTPI_ATTR_INDEXED_BLKLEN__LAST,
};
enum {
    DTPI_ATTR_INDEXED_DISPLS__SMALL = 0,
    DTPI_ATTR_INDEXED_DISPLS__LARGE,
    DTPI_ATTR_INDEXED_DISPLS__REDUCING,
    DTPI_ATTR_INDEXED_DISPLS__REVERSE,
    DTPI_ATTR_INDEXED_DISPLS__UNEVEN,
    DTPI_ATTR_INDEXED_DISPLS__LAST,
};


/* hindexed attributes */
enum {
    DTPI_ATTR_HINDEXED_NUMBLKS__ONE = 0,
    DTPI_ATTR_HINDEXED_NUMBLKS__SMALL,
    DTPI_ATTR_HINDEXED_NUMBLKS__LARGE,
    DTPI_ATTR_HINDEXED_NUMBLKS__LAST,
};
enum {
    DTPI_ATTR_HINDEXED_BLKLEN__ONE = 0,
    DTPI_ATTR_HINDEXED_BLKLEN__SMALL,
    DTPI_ATTR_HINDEXED_BLKLEN__LARGE,
    DTPI_ATTR_HINDEXED_BLKLEN__UNEVEN,
    DTPI_ATTR_HINDEXED_BLKLEN__LAST,
};
enum {
    DTPI_ATTR_HINDEXED_DISPLS__SMALL = 0,
    DTPI_ATTR_HINDEXED_DISPLS__LARGE,
    DTPI_ATTR_HINDEXED_DISPLS__REDUCING,
    DTPI_ATTR_HINDEXED_DISPLS__REVERSE,
    DTPI_ATTR_HINDEXED_DISPLS__UNEVEN,
    DTPI_ATTR_HINDEXED_DISPLS__LAST,
};


/* subarray attributes */
enum {
    DTPI_ATTR_SUBARRAY_NDIMS__SMALL = 0,
    DTPI_ATTR_SUBARRAY_NDIMS__LARGE,
    DTPI_ATTR_SUBARRAY_NDIMS__LAST
};
enum {
    DTPI_ATTR_SUBARRAY_SUBSIZES__ONE = 0,
    DTPI_ATTR_SUBARRAY_SUBSIZES__SMALL,
    DTPI_ATTR_SUBARRAY_SUBSIZES__LARGE,
    DTPI_ATTR_SUBARRAY_SUBSIZES__LAST,
};
enum {
    DTPI_ATTR_SUBARRAY_SIZES__ONE = 0,
    DTPI_ATTR_SUBARRAY_SIZES__SMALL,
    DTPI_ATTR_SUBARRAY_SIZES__LARGE,
    DTPI_ATTR_SUBARRAY_SIZES__LAST,
};
enum {
    DTPI_ATTR_SUBARRAY_ORDER__C = 0,
    DTPI_ATTR_SUBARRAY_ORDER__FORTRAN,
    DTPI_ATTR_SUBARRAY_ORDER__LAST,
};


/* struct attributes */
/* TODO: add an attribute for multiple child types */
enum {
    DTPI_ATTR_STRUCTURE_NUMBLKS__ONE = 0,
    DTPI_ATTR_STRUCTURE_NUMBLKS__SMALL,
    DTPI_ATTR_STRUCTURE_NUMBLKS__LARGE,
    DTPI_ATTR_STRUCTURE_NUMBLKS__LAST,
};
enum {
    DTPI_ATTR_STRUCTURE_BLKLEN__ONE = 0,
    DTPI_ATTR_STRUCTURE_BLKLEN__SMALL,
    DTPI_ATTR_STRUCTURE_BLKLEN__LARGE,
    DTPI_ATTR_STRUCTURE_BLKLEN__UNEVEN,
    DTPI_ATTR_STRUCTURE_BLKLEN__LAST,
};
enum {
    DTPI_ATTR_STRUCTURE_DISPLS__SMALL = 0,
    DTPI_ATTR_STRUCTURE_DISPLS__LARGE,
    DTPI_ATTR_STRUCTURE_DISPLS__REDUCING,
    DTPI_ATTR_STRUCTURE_DISPLS__REVERSE,
    DTPI_ATTR_STRUCTURE_DISPLS__UNEVEN,
    DTPI_ATTR_STRUCTURE_DISPLS__LAST,
};


/* This is a linear list of datatype attributes; this could
 * evolve into a tree in the future, but we don't need that right
 * now */
typedef struct DTPI_Attr {
    DTPI_Datatype_kind_e kind;
    intptr_t child_type_extent;
    struct DTPI_Attr *child;

    union {
        struct {
            intptr_t blklen;
        } contig;

        struct {
            intptr_t lb;
            intptr_t extent;
        } resized;

        struct {
            intptr_t numblks;
            intptr_t blklen;
            intptr_t stride;
        } vector;

        struct {
            intptr_t numblks;
            intptr_t blklen;
            intptr_t stride;
        } hvector;

        struct {
            intptr_t numblks;
            intptr_t blklen;
            intptr_t *array_of_displs;
        } blkindx;

        struct {
            intptr_t numblks;
            intptr_t blklen;
            intptr_t *array_of_displs;
        } blkhindx;

        struct {
            intptr_t numblks;
            intptr_t *array_of_blklens;
            intptr_t *array_of_displs;
        } indexed;

        struct {
            intptr_t numblks;
            intptr_t *array_of_blklens;
            intptr_t *array_of_displs;
        } hindexed;

        struct {
            intptr_t ndims;
            intptr_t *array_of_sizes;
            intptr_t *array_of_subsizes;
            intptr_t *array_of_starts;
            int order;
        } subarray;

        struct {
            intptr_t numblks;
            intptr_t *array_of_blklens;
            intptr_t *array_of_displs;
        } structure;
    } u;
} DTPI_Attr_s;

typedef struct {
    DTP_pool_s dtp;
    DTPI_Attr_s *attr_tree;
} DTPI_obj_s;

int DTPI_obj_free(DTPI_obj_s * obj_priv);
int DTPI_parse_base_type_str(DTP_pool_s * dtp, const char *str);
int DTPI_parse_desc(const char *desc, const char **desc_list, int *depth, int max_depth);
unsigned int DTPI_low_count(unsigned int count);
unsigned int DTPI_high_count(unsigned int count);
int DTPI_construct_datatype(DTP_pool_s dtp, int attr_tree_depth, DTPI_Attr_s ** attr_tree,
                            yaksa_type_t * newtype, uintptr_t * new_count);
int DTPI_custom_datatype(DTP_pool_s dtp, DTPI_Attr_s ** attr_tree, yaksa_type_t * newtype,
                         uintptr_t * new_count, const char **desc_list, int depth);
int DTPI_populate_dtp_desc(DTPI_obj_s * obj_priv, DTPI_pool_s * dtpi, char **desc);
int DTPI_rand(DTPI_pool_s * dtpi);
int DTPI_init_verify(DTP_pool_s dtp, DTP_obj_s obj, void *buf, DTPI_Attr_s * attr_tree,
                     uintptr_t buf_offset, int *val_start, int val_stride, int *rem_val_count,
                     int verify);

#endif /* DTPOOLS_INTERNAL_H_INCLUDED */
