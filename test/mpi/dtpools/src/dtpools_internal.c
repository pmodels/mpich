/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <sys/types.h>
#include <stdint.h>
#include "dtpools_internal.h"
#include "dtpoolsconf.h"

#define TYPE_NAME_MAXLEN (256)
#define BASIC_TYPE_NAME_MAXLEN (64)

#define DTPI_OBJ_INIT_BUF(c_type, type_ptr)                                  \
    do {                                                                     \
        type_ptr = (c_type *) buf_ptr;                                       \
        k = par->user.val_start;                                             \
        for (i = 0; i < par->core.type_totlen; i += par->core.type_stride) { \
            for (j = 0; j < par->core.type_blklen; j++) {                    \
                type_ptr[i+j] = (c_type) k;                                  \
                k += par->user.val_stride;                                   \
                if (--count == 0) {                                          \
                    return;                                                  \
                }                                                            \
            }                                                                \
        }                                                                    \
    } while (0)

#define DTPI_OBJ_INIT_COMP_BUF(c_type, a_type, b_type, type_ptr)             \
    do {                                                                     \
        type_ptr = (c_type *) buf_ptr;                                       \
        k = par->user.val_start;                                             \
        for (i = 0; i < par->core.type_totlen; i += par->core.type_stride) { \
            for (j = 0; j < par->core.type_blklen; j++) {                    \
                type_ptr[i+j].a = (a_type) k;                                \
                type_ptr[i+j].b = (b_type) k;                                \
                k += par->user.val_stride;                                   \
                if (--count == 0) {                                          \
                    return;                                                  \
                }                                                            \
            }                                                                \
        }                                                                    \
    } while (0)

#define DTPI_OBJ_CHECK_BUF_AND_JUMP(c_type, type_ptr)                        \
    do {                                                                     \
        type_ptr = (c_type *) buf_ptr;                                       \
        k = par->user.val_start;                                             \
        for (i = 0; i < par->core.type_totlen; i += par->core.type_stride) { \
            for (j = 0; j < par->core.type_blklen; j++) {                    \
                if (type_ptr[i+j] != (c_type) k) {                           \
                    FPRINTF(stdout, "recv buf[%d]=%d != %d\n",               \
                            i+j, (int) type_ptr[i+j], k);                    \
                    goto fn_fail;                                            \
                }                                                            \
                k += par->user.val_stride;                                   \
                if (--count == 0) {                                          \
                    goto fn_exit;                                            \
                }                                                            \
            }                                                                \
        }                                                                    \
    } while (0)

#define DTPI_OBJ_CHECK_COMP_BUF_AND_JUMP(c_type, a_type, b_type, type_ptr)   \
    do {                                                                     \
        type_ptr = (c_type *) buf_ptr;                                       \
        k = par->user.val_start;                                             \
        for (i = 0; i < par->core.type_totlen; i += par->core.type_stride) { \
            for (j = 0; j < par->core.type_blklen; j++) {                    \
                if (type_ptr[i+j].a != (a_type) k ||                         \
                    type_ptr[i+j].b != (b_type) k) {                         \
                    FPRINTF(stdout, "recv buf[%d].{a=%d,b=%d} != %d\n", i+j, \
                            (int) type_ptr[i+j].a, (int) type_ptr[i+j].b, k);\
                    goto fn_fail;                                            \
                }                                                            \
                k += par->user.val_stride;                                   \
                if (--count == 0) {                                          \
                    goto fn_exit;                                            \
                }                                                            \
            }                                                                \
        }                                                                    \
    } while (0)

/*
 * Composite types
 */
typedef struct {
    float a;
    float b;
} dtp_c_complex;

typedef dtp_c_complex dtp_c_float_complex;

typedef struct {
    double a;
    double b;
} dtp_c_double_complex;

typedef struct {
    long double a;
    long double b;
} dtp_c_long_double_complex;

typedef struct {
    float a;
    int b;
} dtp_float_int;

typedef struct {
    double a;
    int b;
} dtp_double_int;

typedef struct {
    long a;
    int b;
} dtp_long_int;

typedef struct {
    int a;
    int b;
} dtp_2int;

typedef struct {
    short a;
    int b;
} dtp_short_int;

typedef struct {
    long double a;
    int b;
} dtp_long_double_int;

/*
 * Casting pointers
 */
union DTPI_Cast_ptr {
    char *char_ptr;
    wchar_t *wchar_ptr;
    short int *short_ptr;
    int *int_ptr;
    long int *long_ptr;
    long long int *long_long_ptr;
    unsigned char *uchar_ptr;
    unsigned short int *ushort_ptr;
    unsigned int *uint_ptr;
    unsigned long int *ulong_ptr;
    unsigned long long int *ulong_long_ptr;
    float *float_ptr;
    double *double_ptr;
    long double *long_double_ptr;
    int8_t *int8_ptr;
    int16_t *int16_ptr;
    int32_t *int32_ptr;
    int64_t *int64_ptr;
    uint8_t *uint8_ptr;
    uint16_t *uint16_ptr;
    uint32_t *uint32_ptr;
    uint64_t *uint64_ptr;
    /* composite types */
    dtp_c_complex *c_complex_ptr;
    dtp_c_float_complex *c_float_complex_ptr;
    dtp_c_double_complex *c_double_complex_ptr;
    dtp_c_long_double_complex *c_long_double_complex_ptr;
    dtp_float_int *float_int_ptr;
    dtp_double_int *double_int_ptr;
    dtp_long_int *long_int_ptr;
    dtp_2int *int_int_ptr;
    dtp_short_int *short_int_ptr;
    dtp_long_double_int *long_double_int_ptr;
    /* TODO: add remaining types */
};

/* --------------------------------------------------------- */
/* Utility Functions                                         */
/* --------------------------------------------------------- */

void DTPI_Print_error(int errcode)
{
    int errclass, slen;
    char string[ERR_STRING_MAX_LEN];

    MPI_Error_class(errcode, &errclass);
    MPI_Error_string(errcode, string, &slen);

    fprintf(stdout, "Error class %d (%s)\n", errclass, string);
    fflush(stdout);
}

void DTPI_Init_creators(DTPI_Creator * creators)
{
    memset(creators, 0, sizeof(*creators));
    creators[DTPI_OBJ_LAYOUT_SIMPLE__BASIC] = DTPI_Basic_create;
    creators[DTPI_OBJ_LAYOUT_SIMPLE__CONTIG] = DTPI_Contig_create;
    creators[DTPI_OBJ_LAYOUT_SIMPLE__VECTOR] = DTPI_Vector_create;
    creators[DTPI_OBJ_LAYOUT_SIMPLE__HVECTOR] = DTPI_Hvector_create;
    creators[DTPI_OBJ_LAYOUT_SIMPLE__INDEXED] = DTPI_Indexed_create;
    creators[DTPI_OBJ_LAYOUT_SIMPLE__HINDEXED] = DTPI_Hindexed_create;
    creators[DTPI_OBJ_LAYOUT_SIMPLE__BLOCK_INDEXED] = DTPI_Block_indexed_create;
    creators[DTPI_OBJ_LAYOUT_SIMPLE__BLOCK_HINDEXED] = DTPI_Block_hindexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK__VECTOR] = DTPI_Vector_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK__HVECTOR] = DTPI_Hvector_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK__INDEXED] = DTPI_Indexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK__HINDEXED] = DTPI_Hindexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK__BLOCK_INDEXED] = DTPI_Block_indexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK__BLOCK_HINDEXED] = DTPI_Block_hindexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK__SUBARRAY_C] = DTPI_Subarray_c_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK__SUBARRAY_F] = DTPI_Subarray_f_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT__VECTOR] = DTPI_Vector_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT__HVECTOR] = DTPI_Hvector_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT__INDEXED] = DTPI_Indexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT__HINDEXED] = DTPI_Hindexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT__BLOCK_INDEXED] = DTPI_Block_indexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT__BLOCK_HINDEXED] = DTPI_Block_hindexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT__SUBARRAY_C] = DTPI_Subarray_c_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT__SUBARRAY_F] = DTPI_Subarray_f_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__VECTOR] = DTPI_Vector_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__HVECTOR] = DTPI_Hvector_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__INDEXED] = DTPI_Indexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__HINDEXED] = DTPI_Hindexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__BLOCK_INDEXED] = DTPI_Block_indexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__BLOCK_HINDEXED] = DTPI_Block_hindexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__SUBARRAY_C] = DTPI_Subarray_c_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_BLK_STRD__SUBARRAY_F] = DTPI_Subarray_f_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__VECTOR] = DTPI_Vector_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__HVECTOR] = DTPI_Hvector_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__INDEXED] = DTPI_Indexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__HINDEXED] = DTPI_Hindexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__BLOCK_INDEXED] = DTPI_Block_indexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__BLOCK_HINDEXED] = DTPI_Block_hindexed_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__SUBARRAY_C] = DTPI_Subarray_c_create;
    creators[DTPI_OBJ_LAYOUT_LARGE_CNT_STRD__SUBARRAY_F] = DTPI_Subarray_f_create;
}

static void DTPI_Type_init_buf(struct DTPI_Par *par, MPI_Datatype basic_type, void *buf)
{
    int i, j, k;
    int count;
    char *buf_ptr;
    union DTPI_Cast_ptr ptrs;

    FPRINTF(stdout, "init type_displ=%li\n", par->core.type_displ);
    FPRINTF(stdout, "init type_totlen=%li\n", par->core.type_totlen);
    FPRINTF(stdout, "init type_stride=%li\n", par->core.type_stride);
    FPRINTF(stdout, "init type_blklen=%li\n", par->core.type_blklen);
    FPRINTF(stdout, "init user count=%d\n", par->user.val_count);

    if (par->user.val_count <= 0) {
        return;
    }

    count = par->user.val_count;

    buf_ptr = (char *) buf + par->core.type_displ;
    if (basic_type == MPI_CHAR || basic_type == MPI_BYTE) {
        DTPI_OBJ_INIT_BUF(char, ptrs.char_ptr);
    } else if (basic_type == MPI_WCHAR) {
        DTPI_OBJ_INIT_BUF(wchar_t, ptrs.wchar_ptr);
    } else if (basic_type == MPI_SHORT) {
        DTPI_OBJ_INIT_BUF(short int, ptrs.short_ptr);
    } else if (basic_type == MPI_INT) {
        DTPI_OBJ_INIT_BUF(int, ptrs.int_ptr);
    } else if (basic_type == MPI_LONG) {
        DTPI_OBJ_INIT_BUF(long int, ptrs.long_ptr);
    } else if (basic_type == MPI_LONG_LONG_INT) {
        DTPI_OBJ_INIT_BUF(long long int, ptrs.long_long_ptr);
    } else if (basic_type == MPI_UNSIGNED_CHAR) {
        DTPI_OBJ_INIT_BUF(unsigned char, ptrs.uchar_ptr);
    } else if (basic_type == MPI_UNSIGNED_SHORT) {
        DTPI_OBJ_INIT_BUF(unsigned short int, ptrs.ushort_ptr);
    } else if (basic_type == MPI_UNSIGNED) {
        DTPI_OBJ_INIT_BUF(unsigned int, ptrs.uint_ptr);
    } else if (basic_type == MPI_UNSIGNED_LONG) {
        DTPI_OBJ_INIT_BUF(unsigned long int, ptrs.ulong_ptr);
    } else if (basic_type == MPI_UNSIGNED_LONG_LONG) {
        DTPI_OBJ_INIT_BUF(unsigned long long int, ptrs.ulong_long_ptr);
    } else if (basic_type == MPI_FLOAT) {
        DTPI_OBJ_INIT_BUF(float, ptrs.float_ptr);
    } else if (basic_type == MPI_DOUBLE) {
        DTPI_OBJ_INIT_BUF(double, ptrs.double_ptr);
    } else if (basic_type == MPI_LONG_DOUBLE) {
        DTPI_OBJ_INIT_BUF(long double, ptrs.long_double_ptr);
    } else if (basic_type == MPI_INT8_T) {
        DTPI_OBJ_INIT_BUF(int8_t, ptrs.int8_ptr);
    } else if (basic_type == MPI_INT16_T) {
        DTPI_OBJ_INIT_BUF(int16_t, ptrs.int16_ptr);
    } else if (basic_type == MPI_INT32_T) {
        DTPI_OBJ_INIT_BUF(int32_t, ptrs.int32_ptr);
    } else if (basic_type == MPI_INT64_T) {
        DTPI_OBJ_INIT_BUF(int64_t, ptrs.int64_ptr);
    } else if (basic_type == MPI_UINT8_T) {
        DTPI_OBJ_INIT_BUF(uint8_t, ptrs.uint8_ptr);
    } else if (basic_type == MPI_UINT16_T) {
        DTPI_OBJ_INIT_BUF(uint16_t, ptrs.uint16_ptr);
    } else if (basic_type == MPI_UINT32_T) {
        DTPI_OBJ_INIT_BUF(uint32_t, ptrs.uint32_ptr);
    } else if (basic_type == MPI_UINT64_T) {
        DTPI_OBJ_INIT_BUF(uint64_t, ptrs.uint64_ptr);
    } else if (basic_type == MPI_C_COMPLEX) {   /* composite types */
        DTPI_OBJ_INIT_COMP_BUF(dtp_c_complex, float, float, ptrs.c_complex_ptr);
    } else if (basic_type == MPI_C_FLOAT_COMPLEX) {
        DTPI_OBJ_INIT_COMP_BUF(dtp_c_float_complex, float, float, ptrs.c_float_complex_ptr);
    } else if (basic_type == MPI_C_DOUBLE_COMPLEX) {
        DTPI_OBJ_INIT_COMP_BUF(dtp_c_double_complex, double, double, ptrs.c_double_complex_ptr);
    } else if (basic_type == MPI_C_LONG_DOUBLE_COMPLEX) {
        DTPI_OBJ_INIT_COMP_BUF(dtp_c_long_double_complex, long double, long double,
                               ptrs.c_long_double_complex_ptr);
    } else if (basic_type == MPI_FLOAT_INT) {
        DTPI_OBJ_INIT_COMP_BUF(dtp_float_int, float, int, ptrs.float_int_ptr);
    } else if (basic_type == MPI_DOUBLE_INT) {
        DTPI_OBJ_INIT_COMP_BUF(dtp_double_int, double, int, ptrs.double_int_ptr);
    } else if (basic_type == MPI_LONG_INT) {
        DTPI_OBJ_INIT_COMP_BUF(dtp_long_int, long, int, ptrs.long_int_ptr);
    } else if (basic_type == MPI_2INT) {
        DTPI_OBJ_INIT_COMP_BUF(dtp_2int, int, int, ptrs.int_int_ptr);
    } else if (basic_type == MPI_SHORT_INT) {
        DTPI_OBJ_INIT_COMP_BUF(dtp_short_int, short, int, ptrs.short_int_ptr);
    } else if (basic_type == MPI_LONG_DOUBLE_INT) {
        DTPI_OBJ_INIT_COMP_BUF(dtp_long_double_int, long double, int, ptrs.long_double_int_ptr);
    }
}

static int DTPI_Type_check_buf(struct DTPI_Par *par, MPI_Datatype basic_type, void *buf)
{
    int i, j, k, err = DTP_SUCCESS;
    int count;
    char *buf_ptr;
    union DTPI_Cast_ptr ptrs;

    FPRINTF(stdout, "check type_displ=%li\n", par->core.type_displ);
    FPRINTF(stdout, "check type_totlen=%li\n", par->core.type_totlen);
    FPRINTF(stdout, "check type_stride=%li\n", par->core.type_stride);
    FPRINTF(stdout, "check type_blklen=%li\n", par->core.type_blklen);
    FPRINTF(stdout, "check user count=%d\n", par->user.val_count);

    count = par->user.val_count;

    buf_ptr = (char *) buf + par->core.type_displ;
    if (basic_type == MPI_CHAR || basic_type == MPI_BYTE) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(char, ptrs.char_ptr);
    } else if (basic_type == MPI_WCHAR) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(wchar_t, ptrs.wchar_ptr);
    } else if (basic_type == MPI_SHORT) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(short int, ptrs.short_ptr);
    } else if (basic_type == MPI_INT) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(int, ptrs.int_ptr);
    } else if (basic_type == MPI_LONG) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(long int, ptrs.long_ptr);
    } else if (basic_type == MPI_LONG_LONG_INT) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(long long int, ptrs.long_long_ptr);
    } else if (basic_type == MPI_UNSIGNED_CHAR) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(unsigned char, ptrs.uchar_ptr);
    } else if (basic_type == MPI_UNSIGNED_SHORT) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(unsigned short int, ptrs.ushort_ptr);
    } else if (basic_type == MPI_UNSIGNED) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(unsigned int, ptrs.uint_ptr);
    } else if (basic_type == MPI_UNSIGNED_LONG) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(unsigned long int, ptrs.ulong_ptr);
    } else if (basic_type == MPI_UNSIGNED_LONG_LONG) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(unsigned long long int, ptrs.ulong_long_ptr);
    } else if (basic_type == MPI_FLOAT) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(float, ptrs.float_ptr);
    } else if (basic_type == MPI_DOUBLE) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(double, ptrs.double_ptr);
    } else if (basic_type == MPI_LONG_DOUBLE) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(long double, ptrs.long_double_ptr);
    } else if (basic_type == MPI_INT8_T) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(int8_t, ptrs.int8_ptr);
    } else if (basic_type == MPI_INT16_T) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(int16_t, ptrs.int16_ptr);
    } else if (basic_type == MPI_INT32_T) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(int32_t, ptrs.int32_ptr);
    } else if (basic_type == MPI_INT64_T) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(int64_t, ptrs.int64_ptr);
    } else if (basic_type == MPI_UINT8_T) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(uint8_t, ptrs.uint8_ptr);
    } else if (basic_type == MPI_UINT16_T) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(uint16_t, ptrs.uint16_ptr);
    } else if (basic_type == MPI_UINT32_T) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(uint32_t, ptrs.uint32_ptr);
    } else if (basic_type == MPI_UINT64_T) {
        DTPI_OBJ_CHECK_BUF_AND_JUMP(uint64_t, ptrs.uint64_ptr);
    } else if (basic_type == MPI_C_COMPLEX) {
        DTPI_OBJ_CHECK_COMP_BUF_AND_JUMP(dtp_c_complex, float, float, ptrs.c_complex_ptr);
    } else if (basic_type == MPI_C_FLOAT_COMPLEX) {
        DTPI_OBJ_CHECK_COMP_BUF_AND_JUMP(dtp_c_float_complex, float, float,
                                         ptrs.c_float_complex_ptr);
    } else if (basic_type == MPI_C_DOUBLE_COMPLEX) {
        DTPI_OBJ_CHECK_COMP_BUF_AND_JUMP(dtp_c_double_complex, double, double,
                                         ptrs.c_double_complex_ptr);
    } else if (basic_type == MPI_C_LONG_DOUBLE_COMPLEX) {
        DTPI_OBJ_CHECK_COMP_BUF_AND_JUMP(dtp_c_long_double_complex, long double, long double,
                                         ptrs.c_long_double_complex_ptr);
    } else if (basic_type == MPI_FLOAT_INT) {
        DTPI_OBJ_CHECK_COMP_BUF_AND_JUMP(dtp_float_int, float, int, ptrs.float_int_ptr);
    } else if (basic_type == MPI_DOUBLE_INT) {
        DTPI_OBJ_CHECK_COMP_BUF_AND_JUMP(dtp_double_int, double, int, ptrs.double_int_ptr);
    } else if (basic_type == MPI_LONG_INT) {
        DTPI_OBJ_CHECK_COMP_BUF_AND_JUMP(dtp_long_int, long, int, ptrs.long_int_ptr);
    } else if (basic_type == MPI_2INT) {
        DTPI_OBJ_CHECK_COMP_BUF_AND_JUMP(dtp_2int, int, int, ptrs.int_int_ptr);
    } else if (basic_type == MPI_SHORT_INT) {
        DTPI_OBJ_CHECK_COMP_BUF_AND_JUMP(dtp_short_int, short, int, ptrs.short_int_ptr);
    } else if (basic_type == MPI_LONG_DOUBLE_INT) {
        DTPI_OBJ_CHECK_COMP_BUF_AND_JUMP(dtp_long_double_int, long double, int,
                                         ptrs.long_double_int_ptr);
    }

  fn_exit:
    return err;

  fn_fail:
    err = DTP_ERR_OTHER;
    goto fn_exit;
}

/* --------------------------------------------------------- */
/* Datatype Pool Object Create Functions                     */
/* --------------------------------------------------------- */

int DTPI_Struct_create(struct DTPI_Par *par, DTP_t dtp)
{
    int i;
    int err = DTP_SUCCESS;
    int num_types = 0;
    int obj_idx;
    int *basic_type_counts;
    MPI_Aint *basic_type_sizes = NULL;
    MPI_Aint lb, extent;
    MPI_Aint displs = 0;
    MPI_Aint *basic_type_displs = NULL;
    MPI_Datatype *basic_types;
    void *buf = NULL;
    DTPI_t *dtpi = NULL;
    char type_name[TYPE_NAME_MAXLEN] = { 0 };

    /* get object index */
    obj_idx = par->user.obj_idx;

    num_types = dtp->DTP_type_signature.DTP_pool_struct.DTP_num_types;
    DTPI_OBJ_ALLOC_OR_FAIL(basic_type_sizes, sizeof(*basic_type_sizes) * num_types);
    DTPI_OBJ_ALLOC_OR_FAIL(basic_type_displs, sizeof(*basic_type_displs) * num_types);

    /* compute struct size */
    basic_types = dtp->DTP_type_signature.DTP_pool_struct.DTP_basic_type;
    basic_type_counts = dtp->DTP_type_signature.DTP_pool_struct.DTP_basic_type_count;
    for (i = 0; i < num_types; i++) {
        err = MPI_Type_get_extent(basic_types[i], &lb, &basic_type_sizes[i]);
        if (err) {
            DTPI_Print_error(err);
            goto fn_fail;
        }
        basic_type_displs[i] = displs;
        displs += (basic_type_sizes[i] * basic_type_counts[i]);
    }

    /* create struct datatype */
    err =
        MPI_Type_create_struct(num_types, basic_type_counts, basic_type_displs, basic_types,
                               &dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    err = MPI_Type_commit(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    dtp->DTP_obj_array[obj_idx].DTP_obj_count = 1;

    /* get type extent */
    err = MPI_Type_get_extent(dtp->DTP_obj_array[obj_idx].DTP_obj_type, &lb, &extent);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate buffer */
    DTPI_OBJ_ALLOC_OR_FAIL(buf, lb + extent);

    /* initialize every elem in struct separately */
    for (i = 0; i < num_types; i++) {
        /* user count is ignored */
        par->user.val_count = basic_type_counts[i];

        /* contiguous arrays */
        par->core.type_stride = 1;
        par->core.type_blklen = 1;
        par->core.type_totlen = (MPI_Aint) basic_type_counts[i];
        par->core.type_displ = basic_type_displs[i];
        DTPI_Type_init_buf(par, basic_types[i], buf);
    }

    /* allocate space for private datatype info */
    DTPI_OBJ_ALLOC_OR_FAIL(dtpi, sizeof(*dtpi));

    /* initialize private datatype info data */
    dtpi->obj_type = DTPI_OBJ_TYPE__STRUCT;
    dtpi->type_extent = extent;
    dtpi->type_lb = lb;
    dtpi->type_ub = lb + extent;
    dtpi->u.structure.displs = basic_type_displs;       /* freed in DTP_obj_free() */

    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = buf;
    dtp->DTP_obj_array[obj_idx].private_info = dtpi;

    /* set type name for debug information */
    memset(type_name, 0, sizeof(type_name));
    sprintf(type_name, "%s (%d elements)", "struct", num_types);
    err = MPI_Type_set_name(dtp->DTP_obj_array[obj_idx].DTP_obj_type, (char *) type_name);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

  fn_exit:
    if (basic_type_sizes) {
        free(basic_type_sizes);
    }

    return err;

  fn_fail:
    /* cleanup datatype */
    if (dtp->DTP_obj_array[obj_idx].DTP_obj_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    }

    /* cleanup buffers */
    if (basic_type_displs) {
        free(basic_type_displs);
    }
    if (dtpi) {
        free(dtpi);
    }
    if (buf) {
        free(buf);
    }

    goto fn_exit;
}

int DTPI_Basic_create(struct DTPI_Par *par, DTP_t dtp)
{
    int err = DTP_SUCCESS;
    int obj_idx;
    int len;
    MPI_Aint lb, basic_type_size, basic_type_count;
    MPI_Datatype basic_type;
    void *buf = NULL;
    DTPI_t *dtpi = NULL;
    char type_name[TYPE_NAME_MAXLEN] = { 0 };
    char basic_type_name[BASIC_TYPE_NAME_MAXLEN] = { 0 };

    /* get type signature for pool */
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;
    basic_type_count = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type_count;

    /* get object index in the pool */
    obj_idx = par->user.obj_idx;

    err = MPI_Type_get_extent(basic_type, &lb, &basic_type_size);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* make a copy of the basic datatype passed by the user */
    err = MPI_Type_dup(basic_type, &dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    dtp->DTP_obj_array[obj_idx].DTP_obj_count = basic_type_count;

    /* allocate buffer */
    DTPI_OBJ_ALLOC_OR_FAIL(buf, basic_type_size * basic_type_count);

    /* initialize totlen and displ for buf init */
    par->core.type_totlen = basic_type_count;
    par->core.type_displ = 0;

    /* initialize buffer with requested DTPI_Par */
    DTPI_Type_init_buf(par, basic_type, buf);

    /* allocate space for private datatype info */
    DTPI_OBJ_ALLOC_OR_FAIL(dtpi, sizeof(*dtpi));

    /* initialize private datatype info data */
    dtpi->obj_type = DTPI_OBJ_TYPE__BASIC;
    dtpi->type_basic_size = basic_type_size;
    dtpi->type_extent = basic_type_size;
    dtpi->type_lb = 0;
    dtpi->type_ub = basic_type_size;

    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = buf;
    dtp->DTP_obj_array[obj_idx].private_info = dtpi;

    /* set type name for debug information */
    err = MPI_Type_get_name(basic_type, basic_type_name, &len);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }
    sprintf(type_name, "%s (%s basic_type %li count)", "basic", basic_type_name, basic_type_count);
    err = MPI_Type_set_name(dtp->DTP_obj_array[obj_idx].DTP_obj_type, (char *) type_name);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

  fn_exit:
    return err;

  fn_fail:
    /* cleanup datatype */
    if (dtp->DTP_obj_array[obj_idx].DTP_obj_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    }

    /* cleanup buffers */
    if (dtpi) {
        free(dtpi);
    }
    if (buf) {
        free(buf);
    }

    goto fn_exit;
}

int DTPI_Contig_create(struct DTPI_Par *par, DTP_t dtp)
{
    int err = DTP_SUCCESS;
    int len;
    int obj_idx;
    MPI_Aint basic_type_count;
    MPI_Aint lb, extent, basic_type_size;
    MPI_Datatype basic_type;
    void *buf = NULL;
    DTPI_t *dtpi = NULL;
    char type_name[TYPE_NAME_MAXLEN] = { 0 };
    char basic_type_name[BASIC_TYPE_NAME_MAXLEN] = { 0 };

    /* get type signature for pool */
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;
    basic_type_count = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type_count;

    /* get object index in the pool */
    obj_idx = par->user.obj_idx;

    err = MPI_Type_get_extent(basic_type, &lb, &basic_type_size);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* create contiguous datatype */
    err = MPI_Type_contiguous(basic_type_count, basic_type,
                              &dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    err = MPI_Type_commit(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    dtp->DTP_obj_array[obj_idx].DTP_obj_count = 1;

    err = MPI_Type_get_extent(dtp->DTP_obj_array[obj_idx].DTP_obj_type, &lb, &extent);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate buffer */
    DTPI_OBJ_ALLOC_OR_FAIL(buf, lb + extent);

    /* initialize totlen and displ for buf init */
    /* NOTE: rounding to ceiling is needed for composed types */
    par->core.type_totlen = (extent + basic_type_size - 1) / basic_type_size;
    par->core.type_displ = 0;

    /* initialize buffer with requested DTPI_Par */
    DTPI_Type_init_buf(par, basic_type, buf);

    /* allocate space for private datatype info */
    DTPI_OBJ_ALLOC_OR_FAIL(dtpi, sizeof(*dtpi));

    /* initialize private datatype info data */
    dtpi->obj_type = DTPI_OBJ_TYPE__CONTIG;
    dtpi->type_basic_size = basic_type_size;
    dtpi->type_extent = extent;
    dtpi->type_lb = lb;
    dtpi->type_ub = lb + extent;
    dtpi->u.contig.stride = par->core.type_stride;
    dtpi->u.contig.blklen = par->core.type_blklen;

    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = buf;
    dtp->DTP_obj_array[obj_idx].private_info = dtpi;

    /* set type name for debug information */
    err = MPI_Type_get_name(basic_type, basic_type_name, &len);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }
    sprintf(type_name, "%s (%s basic_type %li count)", "contig", basic_type_name, basic_type_count);
    err = MPI_Type_set_name(dtp->DTP_obj_array[obj_idx].DTP_obj_type, (char *) type_name);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

  fn_exit:
    return err;

  fn_fail:
    /* cleanup datatype */
    if (dtp->DTP_obj_array[obj_idx].DTP_obj_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    }

    /* cleanup buffers */
    if (dtpi) {
        free(dtpi);
    }
    if (buf) {
        free(buf);
    }

    goto fn_exit;
}

int DTPI_Vector_create(struct DTPI_Par *par, DTP_t dtp)
{
    int err = DTP_SUCCESS;
    int len;
    int obj_idx;
    MPI_Aint lb, extent, basic_type_size;
    MPI_Datatype basic_type;
    void *buf = NULL;
    DTPI_t *dtpi = NULL;
    char type_name[TYPE_NAME_MAXLEN] = { 0 };
    char basic_type_name[BASIC_TYPE_NAME_MAXLEN] = { 0 };

    /* get basic type for pool */
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    /* get object index in the pool */
    obj_idx = par->user.obj_idx;

    err = MPI_Type_get_extent(basic_type, &lb, &basic_type_size);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* create vector datatype */
    err =
        MPI_Type_vector(par->core.type_count, par->core.type_blklen,
                        par->core.type_stride, basic_type,
                        &dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    err = MPI_Type_commit(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    dtp->DTP_obj_array[obj_idx].DTP_obj_count = 1;

    err = MPI_Type_get_extent(dtp->DTP_obj_array[obj_idx].DTP_obj_type, &lb, &extent);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate buffer */
    DTPI_OBJ_ALLOC_OR_FAIL(buf, lb + extent);

    /* initialize totlen and displ for buf init */
    par->core.type_totlen = (extent + basic_type_size - 1) / basic_type_size;
    par->core.type_displ = 0;

    /* initialize buffer with requested DTPI_Par */
    DTPI_Type_init_buf(par, basic_type, buf);

    /* allocate space for private datatype info */
    DTPI_OBJ_ALLOC_OR_FAIL(dtpi, sizeof(*dtpi));

    /* initialize private datatype info data */
    dtpi->obj_type = DTPI_OBJ_TYPE__VECTOR;
    dtpi->type_basic_size = basic_type_size;
    dtpi->type_extent = extent;
    dtpi->type_lb = lb;
    dtpi->type_ub = lb + extent;
    dtpi->u.vector.stride = par->core.type_stride;
    dtpi->u.vector.blklen = par->core.type_blklen;

    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = buf;
    dtp->DTP_obj_array[obj_idx].private_info = dtpi;

    /* set type name for debug information */
    err = MPI_Type_get_name(basic_type, basic_type_name, &len);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }
    sprintf(type_name, "%s (%s basic_type %li count %li blocklen %li stride)", "vector",
            basic_type_name, par->core.type_count, par->core.type_blklen, par->core.type_stride);
    err = MPI_Type_set_name(dtp->DTP_obj_array[obj_idx].DTP_obj_type, (char *) type_name);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

  fn_exit:
    return err;

  fn_fail:
    /* cleanup datatype */
    if (dtp->DTP_obj_array[obj_idx].DTP_obj_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    }

    /* clean up buffers */
    if (dtpi) {
        free(dtpi);
    }
    if (buf) {
        free(buf);
    }

    goto fn_exit;
}

int DTPI_Hvector_create(struct DTPI_Par *par, DTP_t dtp)
{
    int err = DTP_SUCCESS;
    int len;
    int obj_idx;
    MPI_Aint lb, extent, basic_type_size;
    MPI_Datatype basic_type;
    void *buf = NULL;
    DTPI_t *dtpi = NULL;
    char type_name[TYPE_NAME_MAXLEN] = { 0 };
    char basic_type_name[BASIC_TYPE_NAME_MAXLEN] = { 0 };

    /* get basic type for pool */
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    /* get object index in the pool */
    obj_idx = par->user.obj_idx;

    err = MPI_Type_get_extent(basic_type, &lb, &basic_type_size);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* create hvector datatype */
    err =
        MPI_Type_create_hvector(par->core.type_count, par->core.type_blklen,
                                par->core.type_stride * basic_type_size, basic_type,
                                &dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    err = MPI_Type_commit(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    dtp->DTP_obj_array[obj_idx].DTP_obj_count = 1;

    /* get type true extent */
    err = MPI_Type_get_true_extent(dtp->DTP_obj_array[obj_idx].DTP_obj_type, &lb, &extent);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate buffer */
    FPRINTF(stdout, "hvector basic type size = %ld", basic_type_size);
    FPRINTF(stdout, "hvector extent = %ld\n", extent);
    DTPI_OBJ_ALLOC_OR_FAIL(buf, lb + extent);

    /* initialize totlen and displ for buf init */
    par->core.type_totlen = (extent + basic_type_size - 1) / basic_type_size;
    par->core.type_displ = 0;

    /* initialize buffer with requested DTPI_Par */
    DTPI_Type_init_buf(par, basic_type, buf);

    /* allocate space for private datatype info */
    DTPI_OBJ_ALLOC_OR_FAIL(dtpi, sizeof(*dtpi));

    /* initialize private datatype info data */
    dtpi->obj_type = DTPI_OBJ_TYPE__HVECTOR;
    dtpi->type_basic_size = basic_type_size;
    dtpi->type_extent = extent;
    dtpi->type_lb = lb;
    dtpi->type_ub = lb + extent;
    dtpi->u.hvector.stride = par->core.type_stride * basic_type_size;
    dtpi->u.hvector.blklen = par->core.type_blklen;

    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = buf;
    dtp->DTP_obj_array[obj_idx].private_info = dtpi;

    /* set type name for debug information */
    err = MPI_Type_get_name(basic_type, basic_type_name, &len);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }
    sprintf(type_name, "%s (%s basic_type %li count %li blocklen %li stride)", "hvector",
            basic_type_name, par->core.type_count, par->core.type_blklen, par->core.type_stride);
    err = MPI_Type_set_name(dtp->DTP_obj_array[obj_idx].DTP_obj_type, (char *) type_name);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

  fn_exit:
    return err;

  fn_fail:
    /* cleanup datatype */
    if (dtp->DTP_obj_array[obj_idx].DTP_obj_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    }

    /* clean up buffers */
    if (dtpi) {
        free(dtpi);
    }
    if (buf) {
        free(buf);
    }

    goto fn_exit;
}

int DTPI_Indexed_create(struct DTPI_Par *par, DTP_t dtp)
{
    int i;
    int err = DTP_SUCCESS;
    int len;
    int obj_idx;
    int *type_displs = NULL, *type_blklens = NULL;
    MPI_Aint lb, extent, basic_type_size;
    MPI_Datatype basic_type;
    void *buf = NULL;
    DTPI_t *dtpi = NULL;
    char type_name[TYPE_NAME_MAXLEN] = { 0 };
    char basic_type_name[BASIC_TYPE_NAME_MAXLEN] = { 0 };

    /* get basic type for pool */
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    /* get object index in the pool */
    obj_idx = par->user.obj_idx;

    DTPI_OBJ_ALLOC_OR_FAIL(type_displs, sizeof(*type_displs) * par->core.type_count);
    DTPI_OBJ_ALLOC_OR_FAIL(type_blklens, sizeof(*type_blklens) * par->core.type_count);

    err = MPI_Type_get_extent(basic_type, &lb, &basic_type_size);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    for (i = 0; i < par->core.type_count; i++) {
        type_blklens[i] = par->core.type_blklen;
        type_displs[i] = par->core.type_stride * i;
    }

    /* create indexed datatype */
    err =
        MPI_Type_indexed(par->core.type_count, type_blklens, type_displs, basic_type,
                         &dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    err = MPI_Type_commit(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    dtp->DTP_obj_array[obj_idx].DTP_obj_count = 1;

    /* get type extent */
    err = MPI_Type_get_extent(dtp->DTP_obj_array[obj_idx].DTP_obj_type, &lb, &extent);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate buffer */
    DTPI_OBJ_ALLOC_OR_FAIL(buf, lb + extent);

    /* initialize totlen and displ for buf init */
    par->core.type_totlen = (extent + basic_type_size - 1) / basic_type_size;
    par->core.type_displ = 0;

    /* initialize buffer with requested DTPI_Par */
    DTPI_Type_init_buf(par, basic_type, buf);

    /* allocate space for private datatype info */
    DTPI_OBJ_ALLOC_OR_FAIL(dtpi, sizeof(*dtpi));

    /* initialize private datatype info data */
    dtpi->obj_type = DTPI_OBJ_TYPE__INDEXED;
    dtpi->type_basic_size = basic_type_size;
    dtpi->type_extent = extent;
    dtpi->type_lb = lb;
    dtpi->type_ub = lb + extent;
    dtpi->u.indexed.stride = par->core.type_stride;
    dtpi->u.indexed.blklen = par->core.type_blklen;

    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = buf;
    dtp->DTP_obj_array[obj_idx].private_info = dtpi;

    /* set type name for debug information */
    err = MPI_Type_get_name(basic_type, basic_type_name, &len);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }
    sprintf(type_name, "%s (%s basic_type %li count %li blocklen %li stride)", "indexed",
            basic_type_name, par->core.type_count, par->core.type_blklen, par->core.type_stride);
    err = MPI_Type_set_name(dtp->DTP_obj_array[obj_idx].DTP_obj_type, (char *) type_name);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

  fn_exit:
    if (type_displs) {
        free(type_displs);
    }
    if (type_blklens) {
        free(type_blklens);
    }

    return err;

  fn_fail:
    /* cleanup datatype */
    if (dtp->DTP_obj_array[obj_idx].DTP_obj_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    }

    /* cleanup buffers */
    if (dtpi) {
        free(dtpi);
    }
    if (buf) {
        free(buf);
    }

    goto fn_exit;
}

int DTPI_Hindexed_create(struct DTPI_Par *par, DTP_t dtp)
{
    int i;
    int err = DTP_SUCCESS;
    int len;
    int obj_idx;
    int *type_blklens = NULL;
    MPI_Aint *type_displs = NULL;
    MPI_Aint lb, extent, basic_type_size;
    MPI_Datatype basic_type;
    void *buf = NULL;
    DTPI_t *dtpi = NULL;
    char type_name[TYPE_NAME_MAXLEN] = { 0 };
    char basic_type_name[BASIC_TYPE_NAME_MAXLEN] = { 0 };

    /* get basic type for pool */
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    /* get object index in the pool */
    obj_idx = par->user.obj_idx;

    DTPI_OBJ_ALLOC_OR_FAIL(type_displs, sizeof(*type_displs) * par->core.type_count);
    DTPI_OBJ_ALLOC_OR_FAIL(type_blklens, sizeof(*type_blklens) * par->core.type_count);

    err = MPI_Type_get_extent(basic_type, &lb, &basic_type_size);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    for (i = 0; i < par->core.type_count; i++) {
        type_blklens[i] = par->core.type_blklen;
        type_displs[i] = par->core.type_stride * basic_type_size * i;
    }

    /* create indexed datatype */
    err =
        MPI_Type_create_hindexed(par->core.type_count, type_blklens, type_displs,
                                 basic_type, &dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    err = MPI_Type_commit(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    dtp->DTP_obj_array[obj_idx].DTP_obj_count = 1;

    /* get type extent */
    err = MPI_Type_get_extent(dtp->DTP_obj_array[obj_idx].DTP_obj_type, &lb, &extent);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate buffer */
    DTPI_OBJ_ALLOC_OR_FAIL(buf, lb + extent);

    /* initialize totlen and displ for buf init */
    par->core.type_totlen = (extent + basic_type_size - 1) / basic_type_size;
    par->core.type_displ = 0;

    /* initialize buffer with requested DTPI_Par */
    DTPI_Type_init_buf(par, basic_type, buf);

    /* allocate space for private datatype info */
    DTPI_OBJ_ALLOC_OR_FAIL(dtpi, sizeof(*dtpi));

    /* initialize private datatype info data */
    dtpi->obj_type = DTPI_OBJ_TYPE__HINDEXED;
    dtpi->type_basic_size = basic_type_size;
    dtpi->type_extent = extent;
    dtpi->type_lb = lb;
    dtpi->type_ub = lb + extent;
    dtpi->u.hindexed.stride = par->core.type_stride * basic_type_size;
    dtpi->u.hindexed.blklen = par->core.type_blklen;

    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = buf;
    dtp->DTP_obj_array[obj_idx].private_info = dtpi;

    /* set type name for debug information */
    err = MPI_Type_get_name(basic_type, basic_type_name, &len);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }
    sprintf(type_name, "%s (%s basic_type %li count %li blocklen %li stride)", "hindexed",
            basic_type_name, par->core.type_count, par->core.type_blklen, par->core.type_stride);
    err = MPI_Type_set_name(dtp->DTP_obj_array[obj_idx].DTP_obj_type, (char *) type_name);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

  fn_exit:
    if (type_displs) {
        free(type_displs);
    }
    if (type_blklens) {
        free(type_blklens);
    }

    return err;

  fn_fail:
    /* cleanup datatype */
    if (dtp->DTP_obj_array[obj_idx].DTP_obj_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    }

    /* cleanup buffers */
    if (dtpi) {
        free(dtpi);
    }
    if (buf) {
        free(buf);
    }

    goto fn_exit;
}

int DTPI_Block_indexed_create(struct DTPI_Par *par, DTP_t dtp)
{
    int i;
    int err = DTP_SUCCESS;
    int len;
    int obj_idx;
    int *type_displs = NULL;
    MPI_Aint lb, extent, basic_type_size;
    MPI_Datatype basic_type;
    void *buf = NULL;
    DTPI_t *dtpi = NULL;
    char type_name[TYPE_NAME_MAXLEN] = { 0 };
    char basic_type_name[BASIC_TYPE_NAME_MAXLEN] = { 0 };

    /* get type signature for pool */
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    /* get object index in the pool */
    obj_idx = par->user.obj_idx;

    DTPI_OBJ_ALLOC_OR_FAIL(type_displs, sizeof(*type_displs) * par->core.type_count);

    err = MPI_Type_get_extent(basic_type, &lb, &basic_type_size);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    for (i = 0; i < par->core.type_count; i++) {
        type_displs[i] = par->core.type_stride * i;
    }

    /* create block indexed datatype */
    err =
        MPI_Type_create_indexed_block(par->core.type_count, par->core.type_blklen, type_displs,
                                      basic_type, &dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    err = MPI_Type_commit(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    dtp->DTP_obj_array[obj_idx].DTP_obj_count = 1;

    /* get type extent */
    err = MPI_Type_get_extent(dtp->DTP_obj_array[obj_idx].DTP_obj_type, &lb, &extent);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate buffer */
    DTPI_OBJ_ALLOC_OR_FAIL(buf, lb + extent);

    /* initialize totlen and displ for buf init */
    par->core.type_totlen = (extent + basic_type_size - 1) / basic_type_size;
    par->core.type_displ = 0;

    /* initialize buffer with requested DTPI_Par */
    DTPI_Type_init_buf(par, basic_type, buf);

    /* allocate space for private datatype info */
    DTPI_OBJ_ALLOC_OR_FAIL(dtpi, sizeof(*dtpi));

    /* initialize private datatype info data */
    dtpi->obj_type = DTPI_OBJ_TYPE__BLOCK_INDEXED;
    dtpi->type_basic_size = basic_type_size;
    dtpi->type_extent = extent;
    dtpi->type_lb = lb;
    dtpi->type_ub = lb + extent;
    dtpi->u.block_indexed.stride = par->core.type_stride;
    dtpi->u.block_indexed.blklen = par->core.type_blklen;

    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = buf;
    dtp->DTP_obj_array[obj_idx].private_info = dtpi;

    /* set type name for debug information */
    err = MPI_Type_get_name(basic_type, basic_type_name, &len);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }
    sprintf(type_name, "%s (%s basic_type %li count %li blocklen %li stride)",
            "block_indexed", basic_type_name, par->core.type_count,
            par->core.type_blklen, par->core.type_stride);
    err = MPI_Type_set_name(dtp->DTP_obj_array[obj_idx].DTP_obj_type, (char *) type_name);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

  fn_exit:
    if (type_displs) {
        free(type_displs);
    }

    return err;

  fn_fail:
    /* cleanup datatype */
    if (dtp->DTP_obj_array[obj_idx].DTP_obj_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    }

    /* cleanup buffers */
    if (dtpi) {
        free(dtpi);
    }
    if (buf) {
        free(buf);
    }

    goto fn_exit;
}

int DTPI_Block_hindexed_create(struct DTPI_Par *par, DTP_t dtp)
{
    int i;
    int err = DTP_SUCCESS;
    int len;
    int obj_idx;
    MPI_Aint *type_displs = NULL;
    MPI_Aint lb, extent, basic_type_size;
    MPI_Datatype basic_type;
    void *buf = NULL;
    DTPI_t *dtpi = NULL;
    char type_name[TYPE_NAME_MAXLEN] = { 0 };
    char basic_type_name[BASIC_TYPE_NAME_MAXLEN] = { 0 };

    /* get type signature for pool */
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    /* get object index in the pool */
    obj_idx = par->user.obj_idx;

    DTPI_OBJ_ALLOC_OR_FAIL(type_displs, sizeof(*type_displs) * par->core.type_count);

    err = MPI_Type_get_extent(basic_type, &lb, &basic_type_size);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    for (i = 0; i < par->core.type_count; i++) {
        type_displs[i] = par->core.type_stride * basic_type_size * i;
    }

    /* create block indexed datatype */
    err =
        MPI_Type_create_hindexed_block(par->core.type_count, par->core.type_blklen, type_displs,
                                       basic_type, &dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    err = MPI_Type_commit(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    dtp->DTP_obj_array[obj_idx].DTP_obj_count = 1;

    /* get type extent */
    err = MPI_Type_get_extent(dtp->DTP_obj_array[obj_idx].DTP_obj_type, &lb, &extent);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate buffer */
    DTPI_OBJ_ALLOC_OR_FAIL(buf, lb + extent);

    /* initialize totlen and displ for buf init */
    par->core.type_totlen = (extent + basic_type_size - 1) / basic_type_size;
    par->core.type_displ = 0;

    /* initialize buffer with requested DTPI_Par */
    DTPI_Type_init_buf(par, basic_type, buf);

    /* allocate space for private datatype info */
    DTPI_OBJ_ALLOC_OR_FAIL(dtpi, sizeof(*dtpi));

    /* initialize private datatype info data */
    dtpi->obj_type = DTPI_OBJ_TYPE__BLOCK_HINDEXED;
    dtpi->type_basic_size = basic_type_size;
    dtpi->type_extent = extent;
    dtpi->type_lb = lb;
    dtpi->type_ub = lb + extent;
    dtpi->u.block_hindexed.stride = par->core.type_stride * basic_type_size;
    dtpi->u.block_hindexed.blklen = par->core.type_blklen;

    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = buf;
    dtp->DTP_obj_array[obj_idx].private_info = dtpi;

    /* set type name for debug information */
    err = MPI_Type_get_name(basic_type, basic_type_name, &len);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }
    sprintf(type_name, "%s (%s basic_type %li count %li blocklen %li stride)",
            "block_hindexed", basic_type_name, par->core.type_count,
            par->core.type_blklen, par->core.type_stride);
    err = MPI_Type_set_name(dtp->DTP_obj_array[obj_idx].DTP_obj_type, (char *) type_name);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

  fn_exit:
    if (type_displs) {
        free(type_displs);
    }

    return err;

  fn_fail:
    /* cleanup datatype */
    if (dtp->DTP_obj_array[obj_idx].DTP_obj_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    }

    /* cleanup buffers */
    if (dtpi) {
        free(dtpi);
    }
    if (buf) {
        free(buf);
    }

    goto fn_exit;
}

int DTPI_Subarray_c_create(struct DTPI_Par *par, DTP_t dtp)
{
    int err = DTP_SUCCESS;
    int len;
    int obj_idx;
    MPI_Aint lb, extent, basic_type_size;
    MPI_Datatype basic_type;
    void *buf = NULL;
    char type_name[TYPE_NAME_MAXLEN] = { 0 };
    char basic_type_name[BASIC_TYPE_NAME_MAXLEN] = { 0 };
    DTPI_t *dtpi = NULL;

    /* get type signature for pool */
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    /* get object index in the pool */
    obj_idx = par->user.obj_idx;

    err = MPI_Type_get_extent(basic_type, &lb, &basic_type_size);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate space for private datatype info */
    DTPI_OBJ_ALLOC_OR_FAIL(dtpi, sizeof(*dtpi));

    dtpi->u.subarray.arr_starts[0] = par->core.type_count / 8;  /* use fix offset along x */
    dtpi->u.subarray.arr_starts[1] = par->core.type_stride - par->core.type_blklen;
    dtpi->u.subarray.arr_sizes[0] = par->core.type_count + dtpi->u.subarray.arr_starts[0];
    dtpi->u.subarray.arr_sizes[1] = par->core.type_stride;
    dtpi->u.subarray.arr_subsizes[0] = par->core.type_count;
    dtpi->u.subarray.arr_subsizes[1] = par->core.type_blklen;
    dtpi->u.subarray.order = MPI_ORDER_C;

    err = MPI_Type_create_subarray(2,
                                   dtpi->u.subarray.arr_sizes,
                                   dtpi->u.subarray.arr_subsizes,
                                   dtpi->u.subarray.arr_starts,
                                   dtpi->u.subarray.order,
                                   basic_type, &dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    err = MPI_Type_commit(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    dtp->DTP_obj_array[obj_idx].DTP_obj_count = 1;

    err = MPI_Type_get_extent(dtp->DTP_obj_array[obj_idx].DTP_obj_type, &lb, &extent);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate buffer */
    DTPI_OBJ_ALLOC_OR_FAIL(buf, lb + extent);

    /* recompute stride and displacement info for buffer initialization */
    par->core.type_displ =
        ((dtpi->u.subarray.arr_sizes[1] * dtpi->u.subarray.arr_starts[0]) +
         dtpi->u.subarray.arr_starts[1]) * basic_type_size;
    par->core.type_totlen = (extent - par->core.type_displ) / basic_type_size;

    /* initialize buffer with requested DTPI_Par */
    DTPI_Type_init_buf(par, basic_type, buf);

    /* initialize private datatype info data */
    dtpi->obj_type = DTPI_OBJ_TYPE__SUBARRAY_C;
    dtpi->type_basic_size = basic_type_size;
    dtpi->type_extent = extent;
    dtpi->type_lb = lb;
    dtpi->type_ub = lb + extent;

    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = buf;
    dtp->DTP_obj_array[obj_idx].private_info = dtpi;

    /* set type name for debug information */
    err = MPI_Type_get_name(basic_type, basic_type_name, &len);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }
    sprintf(type_name, "%s (%s basic_type full{%d,%d} sub{%d,%d} start{%d,%d})",
            "subarray-c", basic_type_name, dtpi->u.subarray.arr_sizes[0],
            dtpi->u.subarray.arr_sizes[1], dtpi->u.subarray.arr_subsizes[0],
            dtpi->u.subarray.arr_subsizes[1], dtpi->u.subarray.arr_starts[0],
            dtpi->u.subarray.arr_starts[1]);
    err = MPI_Type_set_name(dtp->DTP_obj_array[obj_idx].DTP_obj_type, (char *) type_name);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

  fn_exit:
    return err;

  fn_fail:
    /* cleanup datatype */
    if (dtp->DTP_obj_array[obj_idx].DTP_obj_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    }

    /* cleanup buffers */
    if (dtpi) {
        free(dtpi);
    }
    if (buf) {
        free(buf);
    }

    goto fn_exit;
}

int DTPI_Subarray_f_create(struct DTPI_Par *par, DTP_t dtp)
{
    int err = DTP_SUCCESS;
    int len;
    int obj_idx;
    MPI_Aint lb, extent, basic_type_size;
    MPI_Datatype basic_type;
    void *buf = NULL;
    char type_name[TYPE_NAME_MAXLEN] = { 0 };
    char basic_type_name[BASIC_TYPE_NAME_MAXLEN] = { 0 };
    DTPI_t *dtpi = NULL;

    /* get type signature for pool */
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    /* get object index in the pool */
    obj_idx = par->user.obj_idx;

    err = MPI_Type_get_extent(basic_type, &lb, &basic_type_size);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate space for private datatype info */
    DTPI_OBJ_ALLOC_OR_FAIL(dtpi, sizeof(*dtpi));

    dtpi->u.subarray.arr_starts[0] = par->core.type_count / 8;  /* use fix offset along x */
    dtpi->u.subarray.arr_starts[1] = par->core.type_stride - par->core.type_blklen;
    dtpi->u.subarray.arr_sizes[0] = par->core.type_count + dtpi->u.subarray.arr_starts[0];
    dtpi->u.subarray.arr_sizes[1] = par->core.type_stride;
    dtpi->u.subarray.arr_subsizes[0] = par->core.type_count;
    dtpi->u.subarray.arr_subsizes[1] = par->core.type_blklen;
    dtpi->u.subarray.order = MPI_ORDER_FORTRAN;

    err = MPI_Type_create_subarray(2,
                                   dtpi->u.subarray.arr_sizes,
                                   dtpi->u.subarray.arr_subsizes,
                                   dtpi->u.subarray.arr_starts,
                                   dtpi->u.subarray.order,
                                   basic_type, &dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    err = MPI_Type_commit(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    dtp->DTP_obj_array[obj_idx].DTP_obj_count = 1;

    err = MPI_Type_get_extent(dtp->DTP_obj_array[obj_idx].DTP_obj_type, &lb, &extent);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

    /* allocate buffer */
    DTPI_OBJ_ALLOC_OR_FAIL(buf, lb + extent);

    /* recompute stride and displacement info for buffer initialization */
    par->core.type_displ =
        ((dtpi->u.subarray.arr_sizes[0] * dtpi->u.subarray.arr_starts[1]) +
         dtpi->u.subarray.arr_starts[0]) * basic_type_size;
    par->core.type_stride = dtpi->u.subarray.arr_sizes[0];
    par->core.type_blklen = dtpi->u.subarray.arr_subsizes[0];
    par->core.type_totlen = (extent - par->core.type_displ) / basic_type_size;

    /* initialize buffer with requested DTPI_Par */
    DTPI_Type_init_buf(par, basic_type, buf);

    /* initialize private datatype info data */
    dtpi->obj_type = DTPI_OBJ_TYPE__SUBARRAY_F;
    dtpi->type_basic_size = basic_type_size;
    dtpi->type_extent = extent;
    dtpi->type_lb = lb;
    dtpi->type_ub = lb + extent;

    dtp->DTP_obj_array[obj_idx].DTP_obj_buf = buf;
    dtp->DTP_obj_array[obj_idx].private_info = dtpi;

    /* set type name for debug information */
    err = MPI_Type_get_name(basic_type, basic_type_name, &len);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }
    sprintf(type_name, "%s (%s basic_type full{%d,%d} sub{%d,%d} start{%d,%d})",
            "subarray-f", basic_type_name, dtpi->u.subarray.arr_sizes[0],
            dtpi->u.subarray.arr_sizes[1], dtpi->u.subarray.arr_subsizes[0],
            dtpi->u.subarray.arr_subsizes[1], dtpi->u.subarray.arr_starts[0],
            dtpi->u.subarray.arr_starts[1]);
    err = MPI_Type_set_name(dtp->DTP_obj_array[obj_idx].DTP_obj_type, (char *) type_name);
    if (err) {
        DTPI_Print_error(err);
        goto fn_fail;
    }

  fn_exit:
    return err;

  fn_fail:
    /* cleanup datatype */
    if (dtp->DTP_obj_array[obj_idx].DTP_obj_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&dtp->DTP_obj_array[obj_idx].DTP_obj_type);
    }

    /* cleanup buffers */
    if (dtpi) {
        free(dtpi);
    }
    if (buf) {
        free(buf);
    }

    goto fn_exit;
}


/* --------------------------------------------------------- */
/* Datatype Pool Buffer Check Functions                      */
/* --------------------------------------------------------- */

int DTPI_Struct_check_buf(struct DTPI_Par *par, DTP_t dtp)
{
    int i, err = DTP_SUCCESS;
    int obj_idx;
    int num_types;
    int *basic_type_counts;
    MPI_Aint *basic_type_displs;
    MPI_Datatype *basic_types;
    void *buf;
    DTPI_t *dtpi;

    obj_idx = par->user.obj_idx;
    buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    num_types = dtp->DTP_type_signature.DTP_pool_struct.DTP_num_types;
    basic_types = dtp->DTP_type_signature.DTP_pool_struct.DTP_basic_type;
    basic_type_counts = dtp->DTP_type_signature.DTP_pool_struct.DTP_basic_type_count;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    basic_type_displs = dtpi->u.structure.displs;

    /* check every elem in struct separately */
    for (i = 0; i < num_types; i++) {
        /* user count is ignored */
        par->user.val_count = basic_type_counts[i];

        /* contiguous arrays */
        par->core.type_stride = 1;
        par->core.type_blklen = 1;
        par->core.type_totlen = (MPI_Aint) basic_type_counts[i];
        par->core.type_displ = basic_type_displs[i];
        err = DTPI_Type_check_buf(par, basic_types[i], buf);
        if (err != DTP_SUCCESS) {
            break;
        }
    }

    return err;
}

int DTPI_Basic_check_buf(struct DTPI_Par *par, DTP_t dtp)
{
    int obj_idx;
    int basic_type_count;
    MPI_Datatype basic_type;
    void *buf;

    obj_idx = par->user.obj_idx;
    buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;
    basic_type_count = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type_count;

    par->core.type_stride = 1;
    par->core.type_blklen = 1;
    par->core.type_totlen = basic_type_count;
    par->core.type_displ = 0;

    return DTPI_Type_check_buf(par, basic_type, buf);
}

int DTPI_Contig_check_buf(struct DTPI_Par *par, DTP_t dtp)
{
    int obj_idx;
    MPI_Datatype basic_type;
    void *buf;
    DTPI_t *dtpi;

    obj_idx = par->user.obj_idx;
    buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    par->core.type_stride = dtpi->u.contig.stride;
    par->core.type_blklen = dtpi->u.contig.blklen;
    par->core.type_totlen = dtpi->type_extent / dtpi->type_basic_size;
    par->core.type_displ = 0;

    return DTPI_Type_check_buf(par, basic_type, buf);
}

int DTPI_Vector_check_buf(struct DTPI_Par *par, DTP_t dtp)
{
    int obj_idx;
    MPI_Datatype basic_type;
    void *buf;
    DTPI_t *dtpi;

    obj_idx = par->user.obj_idx;
    buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    par->core.type_stride = dtpi->u.vector.stride;
    par->core.type_blklen = dtpi->u.vector.blklen;
    par->core.type_totlen = dtpi->type_extent / dtpi->type_basic_size;
    par->core.type_displ = 0;

    return DTPI_Type_check_buf(par, basic_type, buf);
}

int DTPI_Hvector_check_buf(struct DTPI_Par *par, DTP_t dtp)
{
    int obj_idx;
    MPI_Datatype basic_type;
    void *buf;
    DTPI_t *dtpi;

    obj_idx = par->user.obj_idx;
    buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    par->core.type_stride = dtpi->u.hvector.stride / dtpi->type_basic_size;
    par->core.type_blklen = dtpi->u.hvector.blklen;
    par->core.type_totlen = dtpi->type_extent / dtpi->type_basic_size;
    par->core.type_displ = 0;

    return DTPI_Type_check_buf(par, basic_type, buf);
}

int DTPI_Indexed_check_buf(struct DTPI_Par *par, DTP_t dtp)
{
    int obj_idx;
    MPI_Datatype basic_type;
    void *buf;
    DTPI_t *dtpi;

    obj_idx = par->user.obj_idx;
    buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    par->core.type_stride = dtpi->u.indexed.stride;
    par->core.type_blklen = dtpi->u.indexed.blklen;
    par->core.type_totlen = dtpi->type_extent / dtpi->type_basic_size;
    par->core.type_displ = 0;

    return DTPI_Type_check_buf(par, basic_type, buf);
}

int DTPI_Hindexed_check_buf(struct DTPI_Par *par, DTP_t dtp)
{
    int obj_idx;
    MPI_Datatype basic_type;
    void *buf;
    DTPI_t *dtpi;

    obj_idx = par->user.obj_idx;
    buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    par->core.type_stride = dtpi->u.hindexed.stride / dtpi->type_basic_size;
    par->core.type_blklen = dtpi->u.hindexed.blklen;
    par->core.type_totlen = dtpi->type_extent / dtpi->type_basic_size;
    par->core.type_displ = 0;

    return DTPI_Type_check_buf(par, basic_type, buf);
}

int DTPI_Block_indexed_check_buf(struct DTPI_Par *par, DTP_t dtp)
{
    int obj_idx;
    MPI_Datatype basic_type;
    void *buf;
    DTPI_t *dtpi;

    obj_idx = par->user.obj_idx;
    buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    par->core.type_stride = dtpi->u.block_indexed.stride;
    par->core.type_blklen = dtpi->u.block_indexed.blklen;
    par->core.type_totlen = dtpi->type_extent / dtpi->type_basic_size;
    par->core.type_displ = 0;

    return DTPI_Type_check_buf(par, basic_type, buf);
}

int DTPI_Block_hindexed_check_buf(struct DTPI_Par *par, DTP_t dtp)
{
    int obj_idx;
    MPI_Datatype basic_type;
    void *buf;
    DTPI_t *dtpi;

    obj_idx = par->user.obj_idx;
    buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    par->core.type_stride = dtpi->u.block_hindexed.stride / dtpi->type_basic_size;
    par->core.type_blklen = dtpi->u.block_hindexed.blklen;
    par->core.type_totlen = dtpi->type_extent / dtpi->type_basic_size;
    par->core.type_displ = 0;

    return DTPI_Type_check_buf(par, basic_type, buf);
}

int DTPI_Subarray_c_check_buf(struct DTPI_Par *par, DTP_t dtp)
{
    int obj_idx;
    MPI_Datatype basic_type;
    void *buf;
    DTPI_t *dtpi;

    obj_idx = par->user.obj_idx;
    buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    par->core.type_displ =
        ((dtpi->u.subarray.arr_sizes[1] * dtpi->u.subarray.arr_starts[0]) +
         dtpi->u.subarray.arr_starts[1]) * dtpi->type_basic_size;
    par->core.type_totlen = (dtpi->type_extent - par->core.type_displ) / dtpi->type_basic_size;
    par->core.type_stride = dtpi->u.subarray.arr_sizes[1];
    par->core.type_blklen = dtpi->u.subarray.arr_subsizes[1];

    return DTPI_Type_check_buf(par, basic_type, buf);
}

int DTPI_Subarray_f_check_buf(struct DTPI_Par *par, DTP_t dtp)
{
    int obj_idx;
    MPI_Datatype basic_type;
    void *buf;
    DTPI_t *dtpi;

    obj_idx = par->user.obj_idx;
    buf = dtp->DTP_obj_array[obj_idx].DTP_obj_buf;
    basic_type = dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type;

    dtpi = (DTPI_t *) dtp->DTP_obj_array[obj_idx].private_info;
    par->core.type_displ =
        ((dtpi->u.subarray.arr_sizes[0] * dtpi->u.subarray.arr_starts[1]) +
         dtpi->u.subarray.arr_starts[0]) * dtpi->type_basic_size;
    par->core.type_totlen = (dtpi->type_extent - par->core.type_displ) / dtpi->type_basic_size;
    par->core.type_stride = dtpi->u.subarray.arr_sizes[0];
    par->core.type_blklen = dtpi->u.subarray.arr_subsizes[0];

    return DTPI_Type_check_buf(par, basic_type, buf);
}
