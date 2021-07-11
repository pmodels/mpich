/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "dtpools_internal.h"
#include <assert.h>
#include <string.h>

#define MAX_TYPES (10)
#define MAX_TYPE_LEN (64)

#if defined NEEDS_STRDUP_DECL && !defined strdup
extern char *strdup(const char *);
#endif /* MPL_NEEDS_STRDUP_DECL */

int DTPI_func_nesting = -1;

static struct {
    const char *name;
    MPI_Datatype type;
} DTPI_type_list[] = {
    {
    "MPI_CHAR", MPI_CHAR}, {
    "MPI_BYTE", MPI_BYTE}, {
    "MPI_WCHAR", MPI_WCHAR}, {
    "MPI_SHORT", MPI_SHORT}, {
    "MPI_INT", MPI_INT}, {
    "MPI_LONG", MPI_LONG}, {
    "MPI_LONG_LONG_INT", MPI_LONG_LONG_INT}, {
    "MPI_UNSIGNED_CHAR", MPI_UNSIGNED_CHAR}, {
    "MPI_UNSIGNED_SHORT", MPI_UNSIGNED_SHORT}, {
    "MPI_UNSIGNED", MPI_UNSIGNED}, {
    "MPI_UNSIGNED_LONG", MPI_UNSIGNED_LONG}, {
    "MPI_UNSIGNED_LONG_LONG", MPI_UNSIGNED_LONG_LONG}, {
    "MPI_FLOAT", MPI_FLOAT}, {
    "MPI_DOUBLE", MPI_DOUBLE}, {
    "MPI_LONG_DOUBLE", MPI_LONG_DOUBLE}, {
    "MPI_INT8_T", MPI_INT8_T}, {
    "MPI_INT16_T", MPI_INT16_T}, {
    "MPI_INT32_T", MPI_INT32_T}, {
    "MPI_INT64_T", MPI_INT64_T}, {
    "MPI_UINT8_T", MPI_UINT8_T}, {
    "MPI_UINT16_T", MPI_UINT16_T}, {
    "MPI_UINT32_T", MPI_UINT32_T}, {
    "MPI_UINT64_T", MPI_UINT64_T}, {
    "MPI_C_COMPLEX", MPI_C_COMPLEX}, {
    "MPI_C_FLOAT_COMPLEX", MPI_C_FLOAT_COMPLEX}, {
    "MPI_C_DOUBLE_COMPLEX", MPI_C_DOUBLE_COMPLEX}, {
    "MPI_C_LONG_DOUBLE_COMPLEX", MPI_C_LONG_DOUBLE_COMPLEX}, {
    "MPI_FLOAT_INT", MPI_FLOAT_INT}, {
    "MPI_DOUBLE_INT", MPI_DOUBLE_INT}, {
    "MPI_LONG_INT", MPI_LONG_INT}, {
    "MPI_2INT", MPI_2INT}, {
    "MPI_SHORT_INT", MPI_SHORT_INT}, {
    "MPI_LONG_DOUBLE_INT", MPI_LONG_DOUBLE_INT}, {
    "MPI_DATATYPE_NULL", MPI_DATATYPE_NULL}
};

int DTPI_parse_base_type_str(DTP_pool_s * dtp, const char *str)
{
    MPI_Datatype *array_of_types = NULL;
    int *array_of_blklens = NULL;
    char **typestr = NULL;
    char **countstr = NULL;
    int num_types = 0;
    int i;
    MPI_Aint lb;
    DTPI_pool_s *dtpi = dtp->priv;
    int rc = DTP_SUCCESS;

    DTPI_FUNC_ENTER();

    DTPI_ERR_ASSERT(str, rc);

    dtpi->base_type_str = strdup(str);

    DTPI_ALLOC_OR_FAIL(typestr, MAX_TYPES * sizeof(char *), rc);
    DTPI_ALLOC_OR_FAIL(countstr, MAX_TYPES * sizeof(char *), rc);

    int stridx = 0;
    for (num_types = 0; num_types < MAX_TYPES; num_types++) {
        if (str[stridx] == '\0')
            break;

        DTPI_ALLOC_OR_FAIL(typestr[num_types], MAX_TYPE_LEN, rc);
        DTPI_ALLOC_OR_FAIL(countstr[num_types], MAX_TYPE_LEN, rc);

        for (i = 0; str[stridx] != '\0' && str[stridx] != ':' && str[stridx] != '+'; i++, stridx++)
            typestr[num_types][i] = str[stridx];
        typestr[num_types][i] = '\0';

        if (str[stridx] == ':') {
            stridx++;

            for (i = 0; str[stridx] != '\0' && str[stridx] != '+'; i++, stridx++)
                countstr[num_types][i] = str[stridx];
            countstr[num_types][i] = '\0';
        } else {
            countstr[num_types][0] = '\0';
        }

        if (str[stridx] == '+')
            stridx++;
    }
    DTPI_ERR_ASSERT(str[stridx] == '\0', rc);
    DTPI_ERR_ASSERT(num_types >= 1, rc);
    DTPI_ERR_ASSERT(num_types < MAX_TYPES, rc);

    DTPI_ALLOC_OR_FAIL(array_of_types, num_types * sizeof(MPI_Datatype), rc);
    DTPI_ALLOC_OR_FAIL(array_of_blklens, num_types * sizeof(int), rc);

    for (i = 0; i < num_types; i++) {
        array_of_types[i] = MPI_DATATYPE_NULL;
        for (int j = 0; strcmp(DTPI_type_list[j].name, "MPI_DATATYPE_NULL"); j++) {
            if (!strcmp(typestr[i], DTPI_type_list[j].name)) {
                array_of_types[i] = DTPI_type_list[j].type;
                break;
            }
        }
        DTPI_ERR_ASSERT(array_of_types[i] != MPI_DATATYPE_NULL, rc);

        if (strlen(countstr[i]) == 0)
            array_of_blklens[i] = 1;
        else
            array_of_blklens[i] = atoi(countstr[i]);
    }

    /* if it's a single basic datatype, just return it */
    if (num_types == 1 && array_of_blklens[0] == 1) {
        dtp->DTP_base_type = array_of_types[0];
        dtpi->base_type_is_struct = 0;
        DTPI_FREE(array_of_types);
        DTPI_FREE(array_of_blklens);
        goto fn_exit;
    }

    /* non-basic type, create a struct */
    MPI_Aint *array_of_displs;
    DTPI_ALLOC_OR_FAIL(array_of_displs, num_types * sizeof(MPI_Aint), rc);
    MPI_Aint displ = 0;

    for (i = 0; i < num_types; i++) {
        int size;
        rc = MPI_Type_size(array_of_types[i], &size);
        DTPI_ERR_CHK_MPI_RC(rc);

        array_of_displs[i] = displ;
        displ += (array_of_blklens[i] * size);
    }

    rc = MPI_Type_create_struct(num_types, array_of_blklens, array_of_displs, array_of_types,
                                &dtp->DTP_base_type);
    DTPI_ERR_CHK_MPI_RC(rc);

    rc = MPI_Type_commit(&dtp->DTP_base_type);
    DTPI_ERR_CHK_MPI_RC(rc);

    dtpi->base_type_is_struct = 1;
    dtpi->base_type_attrs.numblks = num_types;
    dtpi->base_type_attrs.array_of_blklens = array_of_blklens;
    dtpi->base_type_attrs.array_of_displs = array_of_displs;
    dtpi->base_type_attrs.array_of_types = array_of_types;

  fn_exit:
    MPI_Type_get_extent(dtp->DTP_base_type, &lb, &dtpi->base_type_extent);
    for (i = 0; i < num_types; i++) {
        DTPI_FREE(typestr[i]);
        DTPI_FREE(countstr[i]);
    }
    if (typestr)
        DTPI_FREE(typestr);
    if (countstr)
        DTPI_FREE(countstr);
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    if (array_of_types)
        DTPI_FREE(array_of_types);
    if (array_of_blklens)
        DTPI_FREE(array_of_blklens);
    goto fn_exit;
}

unsigned int DTPI_low_count(unsigned int count)
{
    int ret;

    DTPI_FUNC_ENTER();

    if (count == 1) {
        ret = count;
        goto fn_exit;
    }

    /* if 'count' is a prime number, return 1; else return the lowest
     * prime factor of 'count' */
    for (ret = 2; ret < count; ret++) {
        if (count % ret == 0)
            break;
    }

  fn_exit:
    DTPI_FUNC_EXIT();
    return ret == count ? 1 : ret;
}

unsigned int DTPI_high_count(unsigned int count)
{
    DTPI_FUNC_ENTER();

    DTPI_FUNC_EXIT();
    return count / DTPI_low_count(count);
}

void DTPI_rand_init(DTPI_pool_s * dtpi, int seed, int rand_count)
{
    dtpi->seed = seed;
    dtpi->rand_count = rand_count;
    dtpi->rand_idx = 0;

    srand(dtpi->seed);
    for (int i = 0; i < dtpi->rand_count; i++)
        rand();

    for (int i = 0; i < DTPI_RAND_LIST_SIZE; i++) {
        dtpi->rand_count++;
        dtpi->rand_list[i] = rand();
    }
}

int DTPI_rand(DTPI_pool_s * dtpi)
{
    int ret;
    int rc = DTP_SUCCESS;

    DTPI_FUNC_ENTER();

    DTPI_ERR_ASSERT(dtpi->rand_idx <= DTPI_RAND_LIST_SIZE, rc);

    if (dtpi->rand_idx == DTPI_RAND_LIST_SIZE) {
        dtpi->rand_idx = 0;
    }

    ret = dtpi->rand_list[dtpi->rand_idx];
    dtpi->rand_idx++;

  fn_exit:
    DTPI_FUNC_EXIT();
    assert(rc == DTP_SUCCESS);
    return ret;

  fn_fail:
    goto fn_exit;
}
