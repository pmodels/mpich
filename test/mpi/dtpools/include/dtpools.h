/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DTPOOLS_H_INCLUDED
#define DTPOOLS_H_INCLUDED

/* errors */
#define DTP_SUCCESS   (0)
#define DTP_ERR_OTHER (1)

typedef enum {
    DTP_POOL_TYPE__BASIC,       /* basic type pool */
    DTP_POOL_TYPE__STRUCT,      /* struct type pool */
} DTP_pool_type;

/* MPI_DATATYPE_NULL terminated list of datatypes */
extern MPI_Datatype DTP_Basic_type[];

struct DTP_obj_array_s {
    MPI_Datatype DTP_obj_type;
    MPI_Aint DTP_obj_count;
    void *DTP_obj_buf;

    /* internal private structure to store datatype specific
     * information */
    void *private_info;
};

/* main DTP object */
typedef struct {
    DTP_pool_type DTP_pool_type;
    union {
        struct {
            MPI_Datatype DTP_basic_type;
            MPI_Aint DTP_basic_type_count;
        } DTP_pool_basic;
        struct {
            int DTP_num_types;
            MPI_Datatype *DTP_basic_type;
            int *DTP_basic_type_count;
        } DTP_pool_struct;
    } DTP_type_signature;

    int DTP_num_objs;
    struct DTP_obj_array_s *DTP_obj_array;
} *DTP_t;

/* DTP manipulation functions */
int DTP_pool_create(MPI_Datatype basic_type, MPI_Aint basic_type_count, DTP_t * dtp);
int DTP_pool_create_struct(int num_types, MPI_Datatype * basic_types, int *basic_type_counts,
                           DTP_t * dtp);
int DTP_pool_free(DTP_t dtp);
int DTP_obj_create(DTP_t dtp, int obj_idx, int val_start, int val_stride, MPI_Aint val_count);
int DTP_obj_free(DTP_t dtp, int obj_idx);
int DTP_obj_buf_check(DTP_t dtp, int obj_idx, int val_start, int val_stride, MPI_Aint val_count);

/* Define a list of MPI Datatypes.
   Used in both test/mpi/util/mtest.c and test/mpi/cxx/util/mtest.cxx.
*/
#define DTPOOLS_TYPE_LIST                               \
    {"MPI_CHAR", MPI_CHAR},                             \
    {"MPI_BYTE", MPI_BYTE},                             \
    {"MPI_WCHAR", MPI_WCHAR},                           \
    {"MPI_SHORT", MPI_SHORT},                           \
    {"MPI_INT", MPI_INT},                               \
    {"MPI_LONG", MPI_LONG},                             \
    {"MPI_LONG_LONG_INT", MPI_LONG_LONG_INT},           \
    {"MPI_UNSIGNED_CHAR", MPI_UNSIGNED_CHAR},           \
    {"MPI_UNSIGNED_SHORT", MPI_UNSIGNED_SHORT},         \
    {"MPI_UNSIGNED", MPI_UNSIGNED},                     \
    {"MPI_UNSIGNED_LONG", MPI_UNSIGNED_LONG},           \
    {"MPI_UNSIGNED_LONG_LONG", MPI_UNSIGNED_LONG_LONG}, \
    {"MPI_FLOAT", MPI_FLOAT},                           \
    {"MPI_DOUBLE", MPI_DOUBLE},                         \
    {"MPI_LONG_DOUBLE", MPI_LONG_DOUBLE},               \
    {"MPI_INT8_T", MPI_INT8_T},                         \
    {"MPI_INT16_T", MPI_INT16_T},                       \
    {"MPI_INT32_T", MPI_INT32_T},                       \
    {"MPI_INT64_T", MPI_INT64_T},                       \
    {"MPI_UINT8_T", MPI_UINT8_T},                       \
    {"MPI_UINT16_T", MPI_UINT16_T},                     \
    {"MPI_UINT32_T", MPI_UINT32_T},                     \
    {"MPI_UINT64_T", MPI_UINT64_T},                     \
    {"MPI_C_COMPLEX", MPI_C_COMPLEX},                   \
    {"MPI_C_FLOAT_COMPLEX", MPI_C_FLOAT_COMPLEX},       \
    {"MPI_C_DOUBLE_COMPLEX", MPI_C_DOUBLE_COMPLEX},     \
    {"MPI_C_LONG_DOUBLE_COMPLEX", MPI_C_LONG_DOUBLE_COMPLEX},   \
    {"MPI_FLOAT_INT", MPI_FLOAT_INT},                   \
    {"MPI_DOUBLE_INT", MPI_DOUBLE_INT},                 \
    {"MPI_LONG_INT", MPI_LONG_INT},                     \
    {"MPI_2INT", MPI_2INT},                             \
    {"MPI_SHORT_INT", MPI_SHORT_INT},                   \
    {"MPI_LONG_DOUBLE_INT", MPI_LONG_DOUBLE_INT},       \
    {"MPIX_C_FLOAT16", MPIX_C_FLOAT16},                 \
    {"MPI_DATATYPE_NULL", MPI_DATATYPE_NULL}

#endif /* DTPOOLS_H_INCLUDED */
