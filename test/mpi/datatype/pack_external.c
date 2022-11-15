/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mpi.h"
#include "mpitest.h"

/* #define TEST_COMPLEX */
/* #define TEST_LONG_DOUBLE */

#ifdef TEST_COMPLEX
#include <complex.h>
#endif

struct dt {
    MPI_Datatype mpi_type;
    int length;
};

static struct dt dt_list[] = {
    {MPI_PACKED, 1},
    {MPI_BYTE, 1},
    {MPI_CHAR, 1},
    {MPI_UNSIGNED_CHAR, 1},
    {MPI_SIGNED_CHAR, 1},
    {MPI_WCHAR, 2},
    {MPI_SHORT, 2},
    {MPI_UNSIGNED_SHORT, 2},
    {MPI_INT, 4},
    {MPI_UNSIGNED, 4},
    {MPI_LONG, 4},
    {MPI_UNSIGNED_LONG, 4},
    {MPI_LONG_LONG_INT, 8},
    {MPI_UNSIGNED_LONG_LONG, 8},
    {MPI_FLOAT, 4},
    {MPI_DOUBLE, 8},
    {MPI_LONG_DOUBLE, 16},
    {MPI_C_BOOL, 1},
    {MPI_INT8_T, 1},
    {MPI_INT16_T, 2},
    {MPI_INT32_T, 4},
    {MPI_INT64_T, 8},
    {MPI_UINT8_T, 1},
    {MPI_UINT16_T, 2},
    {MPI_UINT32_T, 4},
    {MPI_UINT64_T, 8},
    {MPI_AINT, 8},
    {MPI_COUNT, 8},
    {MPI_OFFSET, 8},
    {MPI_C_COMPLEX, 2 * 4},
    {MPI_C_FLOAT_COMPLEX, 2 * 4},
    {MPI_C_DOUBLE_COMPLEX, 2 * 8},
    {MPI_C_LONG_DOUBLE_COMPLEX, 2 * 16},
    {MPI_CHARACTER, 1},
    {MPI_LOGICAL, 4},
    {MPI_INTEGER, 4},
    {MPI_REAL, 4},
    {MPI_DOUBLE_PRECISION, 8},
    {MPI_COMPLEX, 2 * 4},
    {MPI_DOUBLE_COMPLEX, 2 * 8},
};

#define N_DT (sizeof(dt_list) / sizeof(struct dt))

#define COUNT 2
#define OFFSET 0

/* first, input data */
#define BOOL_DATA {2, 0}
#define INT_DATA {1, -2}
#define UINT_DATA {1, 2}
/* TODO: test NaN */
#define FLOAT_DATA {1.0, 2.0}
#define COMPLEX_DATA {1.0, 2.0 + I}

int int_data[COUNT] = INT_DATA;
char int_pack[8] = { 0, 0, 0, 1, 0xff, 0xff, 0xff, 0xfe };

long long_data[COUNT] = INT_DATA;
char long_pack[16] = { 0, 0, 0, 1, 0xff, 0xff, 0xff, 0xfe };

long long long_long_data[COUNT] = INT_DATA;
char long_long_pack[16] =
    { 0, 0, 0, 0, 0, 0, 0, 1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe };

float float_data[COUNT] = FLOAT_DATA;
char float_pack[8] = { 0x3f, 0x80, 0, 0, 0x40, 0, 0, 0 };

long double long_double_data[COUNT] = FLOAT_DATA;
char long_double_pack[32] = {
    0x3f, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0x40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

bool bool_data[COUNT] = BOOL_DATA;
char bool_pack[2] = { 1, 0 };

#ifdef TEST_COMPLEX
float complex complex_data[COUNT] = COMPLEX_DATA;
char complex_pack[16] = {
    0x3f, 0x80, 0, 0, 0, 0, 0, 0,
    0x40, 0, 0, 0, 0x3f, 0x80, 0, 0
};
#endif

static const char *get_mpi_type_name(MPI_Datatype mpi_type)
{
    static char type_name[MPI_MAX_OBJECT_NAME];
    int name_len;
    MPI_Type_get_name(mpi_type, type_name, &name_len);
    return type_name;
}

static bool mpi_type_is_bool(MPI_Datatype mpi_type)
{
    return mpi_type == MPI_C_BOOL || mpi_type == MPI_LOGICAL;
}

static const void *get_in_buffer(struct dt dt)
{
    if (dt.mpi_type == MPI_INT) {
        return int_data;
    } else if (dt.mpi_type == MPI_LONG) {
        return long_data;
    } else if (dt.mpi_type == MPI_LONG_LONG_INT) {
        return long_long_data;
    } else if (dt.mpi_type == MPI_FLOAT) {
        return float_data;
    }
#ifdef TEST_LONG_DOUBLE
    else if (dt.mpi_type == MPI_LONG_DOUBLE) {
        return long_double_data;
    }
#endif
#ifdef TEST_COMPLEX
    else if (dt.mpi_type == MPI_C_COMPLEX) {
        return complex_data;
    }
#endif
    else if (dt.mpi_type == MPI_C_BOOL) {
        return bool_data;
    }
    /* TODO: add more datatypes */
    return NULL;
}

static const void *get_pack_buffer(struct dt dt)
{
    if (dt.mpi_type == MPI_INT) {
        return int_pack;
    } else if (dt.mpi_type == MPI_LONG) {
        return long_pack;
    } else if (dt.mpi_type == MPI_LONG_LONG_INT) {
        return long_long_pack;
    } else if (dt.mpi_type == MPI_FLOAT) {
        return float_pack;
    }
#ifdef TEST_LONG_DOUBLE
    else if (dt.mpi_type == MPI_LONG_DOUBLE) {
        return long_double_pack;
    }
#endif
#ifdef TEST_COMPLEX
    else if (dt.mpi_type == MPI_C_COMPLEX) {
        return complex_pack;
    }
#endif
    else if (dt.mpi_type == MPI_C_BOOL) {
        return bool_pack;
    }
    /* TODO: add more packtypes */
    return NULL;
}

static int get_bool_val(const void *buf, int dt_size)
{
    const char *p = buf;
    /* return 1 for any non-zero */
    for (int i = 0; i < dt_size; i++) {
        if (p[i]) {
            return 1;
        }
    }
    return 0;
}

static void get_elem_dump(const void *buf, int dt_size, char *str_out)
{
    const char *p = buf;
    /* NOTE: assumes str_out has enough space */
    char *s = str_out;
    for (int i = 0; i < dt_size; i++) {
        sprintf(s, "%02x", p[i] & 0xff);
        s += 2;
    }
    *s = '\0';
}

int main(int argc, char **argv)
{
    int errs = 0;

    MTest_Init(&argc, &argv);

    for (int i = 0; i < N_DT; i++) {
        const void *buf = get_in_buffer(dt_list[i]);
        if (!buf) {
            continue;
        }

        MPI_Aint size;
        MPI_Pack_external_size("external32", COUNT, dt_list[i].mpi_type, &size);
        if (size != COUNT * dt_list[i].length) {
            printf("MPI_Pack_external_size of %s: returns %d, expect %d\n",
                   get_mpi_type_name(dt_list[i].mpi_type), (int) size, COUNT * dt_list[i].length);
            errs++;
        }

        static char pack_buf[COUNT * 32 + OFFSET];
        MPI_Aint bufsize = COUNT * 32 + OFFSET;
        MPI_Aint pos = OFFSET;

        assert(size < COUNT * 32);
        MPI_Pack_external("external32", buf, COUNT, dt_list[i].mpi_type, pack_buf, bufsize, &pos);
        if (pos != size + OFFSET) {
            printf("MPI_Pack_external %s: updated pos to %d, expect %d\n",
                   get_mpi_type_name(dt_list[i].mpi_type), (int) pos, (int) size + OFFSET);
            errs++;
        }

        const char *ref = get_pack_buffer(dt_list[i]);
        if (mpi_type_is_bool(dt_list[i].mpi_type)) {
            /* Many C compilers will convert boolean values on assignment, e.g. 2->1,
             * but some compilers does not, e.g. Solaris suncc.
             * Current MPICH relies on compiler conversion. For other compilers, it is
             * probably not critical as long as the rount trip check (below) passes.
             * Therefore, we skip the check here. */
        } else if (memcmp(pack_buf + OFFSET, ref, size) != 0) {
            printf("MPI_Pack_external %s: results mismatch!\n",
                   get_mpi_type_name(dt_list[i].mpi_type));
            errs++;
            const char *p = pack_buf + OFFSET;
            for (int j = 0; j < size; j++) {
                if (ref[j] != p[j]) {
                    printf("    pack_buf[%d] is 0x%02x, expect 0x%02x\n", j, p[j] & 0xff,
                           ref[j] & 0xff);
                }
            }
        }

        static char unpack_buf[COUNT * 32];
        pos = OFFSET;
        MPI_Unpack_external("external32", pack_buf, bufsize, &pos, unpack_buf, COUNT,
                            dt_list[i].mpi_type);
        if (pos != size + OFFSET) {
            printf("MPI_Unpack_external %s: updated pos to %d, expect %d\n",
                   get_mpi_type_name(dt_list[i].mpi_type), (int) pos, (int) size + OFFSET);
            errs++;
        }

        int dt_size;
        MPI_Type_size(dt_list[i].mpi_type, &dt_size);
        for (int j = 0; j < COUNT; j++) {
            const void *p = (const char *) buf + j * dt_size;
            const void *q = (const char *) unpack_buf + j * dt_size;
            if (mpi_type_is_bool(dt_list[i].mpi_type)) {
                if (get_bool_val(p, dt_size) != get_bool_val(q, dt_size)) {
                    printf("MPI_Unpack_external %s: element %d mismatch, got %d, expect %d\n",
                           get_mpi_type_name(dt_list[i].mpi_type), j, get_bool_val(q, dt_size),
                           get_bool_val(p, dt_size));
                    errs++;
                }
            } else if (dt_list[i].mpi_type == MPI_LONG_DOUBLE) {
                if (*(long double *) p != *(long double *) q) {
                    printf("MPI_Unpack_external %s: element %d mismatch, got %Lf, expect %Lf\n",
                           get_mpi_type_name(dt_list[i].mpi_type), j, *(long double *) p,
                           *(long double *) q);
                    errs++;
                }
            } else {
                if (memcmp(p, q, dt_size)) {
                    char p_buf[100], q_buf[100];
                    get_elem_dump(p, dt_size, p_buf);
                    get_elem_dump(q, dt_size, q_buf);
                    printf("MPI_Unpack_external %s: element %d mismatch, got %s, expect %s\n",
                           get_mpi_type_name(dt_list[i].mpi_type), j, q_buf, p_buf);
                    errs++;
                }
            }
        }
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
