/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DTPOOLS_INTERNAL_H_INCLUDED
#define DTPOOLS_INTERNAL_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpi.h"
#include "dtpools.h"

#define ERR_STRING_MAX_LEN (512)

#ifdef DEBUG_DTPOOLS
#define FPRINTF(fd,...)         \
    do {                        \
        fprintf(fd,__VA_ARGS__);\
        fflush(fd);             \
    } while (0)
#else
#define FPRINTF(...)
#endif

#define DTPI_OBJ_ALLOC_OR_FAIL(obj, size)                           \
    do {                                                            \
        obj = malloc(size);                                         \
        if (!obj) {                                                 \
            err = DTP_ERR_OTHER;                                    \
            fprintf(stdout, "Out of memory in %s\n", __FUNCTION__); \
            fflush(stdout);                                         \
            goto fn_fail;                                           \
        }                                                           \
        memset(obj, 0, size);                                       \
    } while (0)

/*
 * Simple derived datatype layouts:
 * - block length = 1
 * - stride       = 2
 * - count        = N
 */
enum {
    DTPI_OBJ_LAYOUT_SIMPLE__BASIC,
    DTPI_OBJ_LAYOUT_SIMPLE__CONTIG,
    DTPI_OBJ_LAYOUT_SIMPLE__VECTOR,
    DTPI_OBJ_LAYOUT_SIMPLE__INDEXED,
    DTPI_OBJ_LAYOUT_SIMPLE__BLOCK_INDEXED,
    DTPI_OBJ_LAYOUT_SIMPLE__HVECTOR,
    DTPI_OBJ_LAYOUT_SIMPLE__HINDEXED,
    DTPI_OBJ_LAYOUT_SIMPLE__BLOCK_HINDEXED,
    DTPI_OBJ_LAYOUT_SIMPLE__NUM
};

/*
 * Complex derived datatype layouts:
 * - block length = implementation defined
 * - stride       = implementation defined
 * - count        = implementation defined
 */
enum {
    DTPI_OBJ_LAYOUT_LARGE_BLK__VECTOR = DTPI_OBJ_LAYOUT_SIMPLE__NUM,
    DTPI_OBJ_LAYOUT_LARGE_BLK__INDEXED,
    DTPI_OBJ_LAYOUT_LARGE_BLK__BLOCK_INDEXED,
    DTPI_OBJ_LAYOUT_LARGE_BLK__HVECTOR,
    DTPI_OBJ_LAYOUT_LARGE_BLK__HINDEXED,
    DTPI_OBJ_LAYOUT_LARGE_BLK__BLOCK_HINDEXED,
    DTPI_OBJ_LAYOUT_LARGE_BLK__SUBARRAY_C,
    DTPI_OBJ_LAYOUT_LARGE_BLK__SUBARRAY_F,
    DTPI_OBJ_LAYOUT_LARGE_CNT__VECTOR,
    DTPI_OBJ_LAYOUT_LARGE_CNT__INDEXED,
    DTPI_OBJ_LAYOUT_LARGE_CNT__BLOCK_INDEXED,
    DTPI_OBJ_LAYOUT_LARGE_CNT__HVECTOR,
    DTPI_OBJ_LAYOUT_LARGE_CNT__HINDEXED,
    DTPI_OBJ_LAYOUT_LARGE_CNT__BLOCK_HINDEXED,
    DTPI_OBJ_LAYOUT_LARGE_CNT__SUBARRAY_C,
    DTPI_OBJ_LAYOUT_LARGE_CNT__SUBARRAY_F,
    DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__VECTOR,
    DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__INDEXED,
    DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__BLOCK_INDEXED,
    DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__HVECTOR,
    DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__HINDEXED,
    DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__BLOCK_HINDEXED,
    DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__SUBARRAY_C,
    DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__SUBARRAY_F,
    DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__VECTOR,
    DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__INDEXED,
    DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__BLOCK_INDEXED,
    DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__HVECTOR,
    DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__HINDEXED,
    DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__BLOCK_HINDEXED,
    DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__SUBARRAY_C,
    DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__SUBARRAY_F,
    /* TODO: add LOWER BOUND use case */
    DTPI_OBJ_LAYOUT_LARGE__NUM
};

/*
 * Only one layouts for struct datatype.
 * TODO: extend struct with multiple
 *       additional layouts ... ?
 */
enum {
    DTPI_OBJ_LAYOUT_SIMPLE__STRUCT,
    DTPI_OBJ_LAYOUT__STRUCT_NUM
};

typedef enum {
    DTPI_OBJ_TYPE__BASIC,
    DTPI_OBJ_TYPE__CONTIG,
    DTPI_OBJ_TYPE__VECTOR,
    DTPI_OBJ_TYPE__HVECTOR,
    DTPI_OBJ_TYPE__BLOCK_INDEXED,
    DTPI_OBJ_TYPE__BLOCK_HINDEXED,
    DTPI_OBJ_TYPE__INDEXED,
    DTPI_OBJ_TYPE__HINDEXED,
    DTPI_OBJ_TYPE__SUBARRAY_C,
    DTPI_OBJ_TYPE__SUBARRAY_F,
    DTPI_OBJ_TYPE__STRUCT,
    DTPI_OBJ_TYPE__NUM
} DTPI_obj_type_e;

/*
 * Internal object information
 */
typedef struct {
    DTPI_obj_type_e obj_type;

    MPI_Aint type_basic_size;   /* e.g., sizeof(int) */
    MPI_Aint type_extent;       /* total extent of type in bytes */
    MPI_Aint type_lb;           /* lower bound in bytes */
    MPI_Aint type_ub;           /* upper bound in bytes */

    union {
        struct {
            int stride;
            int blklen;
        } contig;
        struct {
            int stride;         /* stride in basic types */
            int blklen;         /* # of blocks in stride */
        } vector;
        struct {
            MPI_Aint stride;
            int blklen;
        } hvector;
        struct {
            int stride;
            int blklen;
        } indexed;
        struct {
            MPI_Aint stride;
            int blklen;
        } hindexed;
        struct {
            int stride;
            int blklen;
        } block_indexed;
        struct {
            MPI_Aint stride;
            int blklen;
        } block_hindexed;
        struct {
            int order;
            int arr_sizes[2];
            int arr_subsizes[2];
            int arr_starts[2];
        } subarray;
        struct {
            MPI_Aint *displs;   /* displacement addresses in buf */
        } structure;
    } u;
} DTPI_t;

/*
 * Internal parameter structure used to pass
 * data around. Two types of parameters:
 * - User defined object information
 * - Implementation (core) defined object information
 *
 * NOTE: for pools created with `DTP_pool_create_struct`
 *       user defined count is disregarded and reset
 *       internally to `DTP_basic_type_count`.
 */
struct DTPI_Par {
    struct {
        int val_start;          /* start value to fill buf with */
        int val_stride;         /* increment between values */
        int val_count;          /* number of elements to init in buf */
        int obj_idx;            /* index of object in pool */
    } user;
    struct {
        MPI_Aint type_count;    /* # of elements */
        MPI_Aint type_blklen;   /* length of a block in # of basic types */
        MPI_Aint type_stride;   /* # of basic types between start of each block */
        MPI_Aint type_totlen;   /* tot # of basic types in datatype */
        MPI_Aint type_displ;    /* displ of first elem in buffer in bytes */
    } core;
};

typedef int (*DTPI_Creator) (struct DTPI_Par * par, DTP_t dtp);

int DTPI_Struct_create(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Basic_create(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Contig_create(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Vector_create(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Hvector_create(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Indexed_create(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Hindexed_create(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Block_indexed_create(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Block_hindexed_create(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Subarray_c_create(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Subarray_f_create(struct DTPI_Par *par, DTP_t dtp);

int DTPI_Struct_check_buf(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Basic_check_buf(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Contig_check_buf(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Vector_check_buf(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Hvector_check_buf(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Indexed_check_buf(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Hindexed_check_buf(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Block_indexed_check_buf(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Block_hindexed_check_buf(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Subarray_c_check_buf(struct DTPI_Par *par, DTP_t dtp);
int DTPI_Subarray_f_check_buf(struct DTPI_Par *par, DTP_t dtp);

void DTPI_Print_error(int errcode);
void DTPI_Init_creators(DTPI_Creator * creators);

#endif /* DTPOOLS_INTERNAL_H_INCLUDED */
