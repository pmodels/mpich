/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "dtpools_internal.h"
#include <assert.h>
#include <string.h>
#include "yaksa.h"
#include "yaksa_config.h"

#define MAX_TYPES (10)
#define MAX_TYPE_LEN (64)

int DTPI_func_nesting = -1;

static int type_alignment(yaksa_type_t type)
{
    if (type == YAKSA_TYPE___BOOL)
        return ALIGNOF__BOOL;
    else if (type == YAKSA_TYPE__CHAR)
        return ALIGNOF_CHAR;
    else if (type == YAKSA_TYPE__SIGNED_CHAR)
        return ALIGNOF_SIGNED_CHAR;
    else if (type == YAKSA_TYPE__UNSIGNED_CHAR)
        return ALIGNOF_UNSIGNED_CHAR;
    else if (type == YAKSA_TYPE__WCHAR_T)
        return ALIGNOF_WCHAR_T;
    else if (type == YAKSA_TYPE__INT8_T)
        return ALIGNOF_INT8_T;
    else if (type == YAKSA_TYPE__INT16_T)
        return ALIGNOF_INT16_T;
    else if (type == YAKSA_TYPE__INT32_T)
        return ALIGNOF_INT32_T;
    else if (type == YAKSA_TYPE__INT64_T)
        return ALIGNOF_INT64_T;
    else if (type == YAKSA_TYPE__UINT8_T)
        return ALIGNOF_UINT8_T;
    else if (type == YAKSA_TYPE__UINT16_T)
        return ALIGNOF_UINT16_T;
    else if (type == YAKSA_TYPE__UINT32_T)
        return ALIGNOF_UINT32_T;
    else if (type == YAKSA_TYPE__UINT64_T)
        return ALIGNOF_UINT64_T;
    else if (type == YAKSA_TYPE__INT)
        return ALIGNOF_INT;
    else if (type == YAKSA_TYPE__UNSIGNED)
        return ALIGNOF_UNSIGNED;
    else if (type == YAKSA_TYPE__SHORT)
        return ALIGNOF_SHORT;
    else if (type == YAKSA_TYPE__UNSIGNED_SHORT)
        return ALIGNOF_UNSIGNED_SHORT;
    else if (type == YAKSA_TYPE__LONG)
        return ALIGNOF_LONG;
    else if (type == YAKSA_TYPE__UNSIGNED_LONG)
        return ALIGNOF_UNSIGNED_LONG;
    else if (type == YAKSA_TYPE__LONG_LONG)
        return ALIGNOF_LONG_LONG;
    else if (type == YAKSA_TYPE__UNSIGNED_LONG_LONG)
        return ALIGNOF_UNSIGNED_LONG_LONG;
    else if (type == YAKSA_TYPE__INT_FAST8_T)
        return ALIGNOF_INT_FAST8_T;
    else if (type == YAKSA_TYPE__INT_FAST16_T)
        return ALIGNOF_INT_FAST16_T;
    else if (type == YAKSA_TYPE__INT_FAST32_T)
        return ALIGNOF_INT_FAST32_T;
    else if (type == YAKSA_TYPE__INT_FAST64_T)
        return ALIGNOF_INT_FAST64_T;
    else if (type == YAKSA_TYPE__UINT_FAST8_T)
        return ALIGNOF_UINT_FAST8_T;
    else if (type == YAKSA_TYPE__UINT_FAST16_T)
        return ALIGNOF_UINT_FAST16_T;
    else if (type == YAKSA_TYPE__UINT_FAST32_T)
        return ALIGNOF_UINT_FAST32_T;
    else if (type == YAKSA_TYPE__UINT_FAST64_T)
        return ALIGNOF_UINT_FAST64_T;
    else if (type == YAKSA_TYPE__INT_LEAST8_T)
        return ALIGNOF_INT_LEAST8_T;
    else if (type == YAKSA_TYPE__INT_LEAST16_T)
        return ALIGNOF_INT_LEAST16_T;
    else if (type == YAKSA_TYPE__INT_LEAST32_T)
        return ALIGNOF_INT_LEAST32_T;
    else if (type == YAKSA_TYPE__INT_LEAST64_T)
        return ALIGNOF_INT_LEAST64_T;
    else if (type == YAKSA_TYPE__UINT_LEAST8_T)
        return ALIGNOF_UINT_LEAST8_T;
    else if (type == YAKSA_TYPE__UINT_LEAST16_T)
        return ALIGNOF_UINT_LEAST16_T;
    else if (type == YAKSA_TYPE__UINT_LEAST32_T)
        return ALIGNOF_UINT_LEAST32_T;
    else if (type == YAKSA_TYPE__UINT_LEAST64_T)
        return ALIGNOF_UINT_LEAST64_T;
    else if (type == YAKSA_TYPE__BYTE)
        return ALIGNOF_BYTE;
    else if (type == YAKSA_TYPE__INTMAX_T)
        return ALIGNOF_INTMAX_T;
    else if (type == YAKSA_TYPE__UINTMAX_T)
        return ALIGNOF_UINTMAX_T;
    else if (type == YAKSA_TYPE__SIZE_T)
        return ALIGNOF_SIZE_T;
    else if (type == YAKSA_TYPE__INTPTR_T)
        return ALIGNOF_INTPTR_T;
    else if (type == YAKSA_TYPE__UINTPTR_T)
        return ALIGNOF_UINTPTR_T;
    else if (type == YAKSA_TYPE__PTRDIFF_T)
        return ALIGNOF_PTRDIFF_T;
    else if (type == YAKSA_TYPE__FLOAT)
        return ALIGNOF_FLOAT;
    else if (type == YAKSA_TYPE__DOUBLE)
        return ALIGNOF_DOUBLE;
    else if (type == YAKSA_TYPE__LONG_DOUBLE)
        return ALIGNOF_LONG_DOUBLE;
    else if (type == YAKSA_TYPE__C_COMPLEX)
        return ALIGNOF_FLOAT;
    else if (type == YAKSA_TYPE__C_DOUBLE_COMPLEX)
        return ALIGNOF_DOUBLE;
    else if (type == YAKSA_TYPE__C_LONG_DOUBLE_COMPLEX)
        return ALIGNOF_LONG_DOUBLE;
    else if (type == YAKSA_TYPE__FLOAT_INT)
        return ALIGNOF_FLOAT > ALIGNOF_INT ? ALIGNOF_FLOAT : ALIGNOF_INT;
    else if (type == YAKSA_TYPE__DOUBLE_INT)
        return ALIGNOF_DOUBLE > ALIGNOF_INT ? ALIGNOF_DOUBLE : ALIGNOF_INT;
    else if (type == YAKSA_TYPE__LONG_INT)
        return ALIGNOF_LONG;
    else if (type == YAKSA_TYPE__2INT)
        return ALIGNOF_INT;
    else if (type == YAKSA_TYPE__SHORT_INT)
        return ALIGNOF_INT;
    else if (type == YAKSA_TYPE__LONG_DOUBLE_INT)
        return ALIGNOF_LONG_DOUBLE > ALIGNOF_INT ? ALIGNOF_LONG_DOUBLE : ALIGNOF_INT;

    return -1;
}

static yaksa_type_t name_to_type(const char *name)
{
    if (!strcmp(name, "_Bool") || !strcmp(name, "bool"))
        return YAKSA_TYPE___BOOL;
    else if (!strcmp(name, "char"))
        return YAKSA_TYPE__CHAR;
    else if (!strcmp(name, "signed_char"))
        return YAKSA_TYPE__SIGNED_CHAR;
    else if (!strcmp(name, "unsigned_char"))
        return YAKSA_TYPE__UNSIGNED_CHAR;
    else if (!strcmp(name, "wchar_t"))
        return YAKSA_TYPE__WCHAR_T;
    else if (!strcmp(name, "int8_t"))
        return YAKSA_TYPE__INT8_T;
    else if (!strcmp(name, "int16_t"))
        return YAKSA_TYPE__INT16_T;
    else if (!strcmp(name, "int32_t"))
        return YAKSA_TYPE__INT32_T;
    else if (!strcmp(name, "int64_t"))
        return YAKSA_TYPE__INT64_T;
    else if (!strcmp(name, "uint8_t"))
        return YAKSA_TYPE__UINT8_T;
    else if (!strcmp(name, "uint16_t"))
        return YAKSA_TYPE__UINT16_T;
    else if (!strcmp(name, "uint32_t"))
        return YAKSA_TYPE__UINT32_T;
    else if (!strcmp(name, "uint64_t"))
        return YAKSA_TYPE__UINT64_T;
    else if (!strcmp(name, "int"))
        return YAKSA_TYPE__INT;
    else if (!strcmp(name, "unsigned"))
        return YAKSA_TYPE__UNSIGNED;
    else if (!strcmp(name, "short"))
        return YAKSA_TYPE__SHORT;
    else if (!strcmp(name, "unsigned_short"))
        return YAKSA_TYPE__UNSIGNED_SHORT;
    else if (!strcmp(name, "long"))
        return YAKSA_TYPE__LONG;
    else if (!strcmp(name, "unsigned_long"))
        return YAKSA_TYPE__UNSIGNED_LONG;
    else if (!strcmp(name, "long_long"))
        return YAKSA_TYPE__LONG_LONG;
    else if (!strcmp(name, "unsigned_long_long"))
        return YAKSA_TYPE__UNSIGNED_LONG_LONG;
    else if (!strcmp(name, "int_fast8_t"))
        return YAKSA_TYPE__INT_FAST8_T;
    else if (!strcmp(name, "int_fast16_t"))
        return YAKSA_TYPE__INT_FAST16_T;
    else if (!strcmp(name, "int_fast32_t"))
        return YAKSA_TYPE__INT_FAST32_T;
    else if (!strcmp(name, "int_fast64_t"))
        return YAKSA_TYPE__INT_FAST64_T;
    else if (!strcmp(name, "uint_fast8_t"))
        return YAKSA_TYPE__UINT_FAST8_T;
    else if (!strcmp(name, "uint_fast16_t"))
        return YAKSA_TYPE__UINT_FAST16_T;
    else if (!strcmp(name, "uint_fast32_t"))
        return YAKSA_TYPE__UINT_FAST32_T;
    else if (!strcmp(name, "uint_fast64_t"))
        return YAKSA_TYPE__UINT_FAST64_T;
    else if (!strcmp(name, "int_least8_t"))
        return YAKSA_TYPE__INT_LEAST8_T;
    else if (!strcmp(name, "int_least16_t"))
        return YAKSA_TYPE__INT_LEAST16_T;
    else if (!strcmp(name, "int_least32_t"))
        return YAKSA_TYPE__INT_LEAST32_T;
    else if (!strcmp(name, "int_least64_t"))
        return YAKSA_TYPE__INT_LEAST64_T;
    else if (!strcmp(name, "uint_least8_t"))
        return YAKSA_TYPE__UINT_LEAST8_T;
    else if (!strcmp(name, "uint_least16_t"))
        return YAKSA_TYPE__UINT_LEAST16_T;
    else if (!strcmp(name, "uint_least32_t"))
        return YAKSA_TYPE__UINT_LEAST32_T;
    else if (!strcmp(name, "uint_least64_t"))
        return YAKSA_TYPE__UINT_LEAST64_T;
    else if (!strcmp(name, "byte"))
        return YAKSA_TYPE__BYTE;
    else if (!strcmp(name, "intmax_t"))
        return YAKSA_TYPE__INTMAX_T;
    else if (!strcmp(name, "uintmax_t"))
        return YAKSA_TYPE__UINTMAX_T;
    else if (!strcmp(name, "size_t"))
        return YAKSA_TYPE__SIZE_T;
    else if (!strcmp(name, "intptr_t"))
        return YAKSA_TYPE__INTPTR_T;
    else if (!strcmp(name, "uintptr_t"))
        return YAKSA_TYPE__UINTPTR_T;
    else if (!strcmp(name, "ptrdiff_t"))
        return YAKSA_TYPE__PTRDIFF_T;
    else if (!strcmp(name, "float"))
        return YAKSA_TYPE__FLOAT;
    else if (!strcmp(name, "double"))
        return YAKSA_TYPE__DOUBLE;
    else if (!strcmp(name, "long_double"))
        return YAKSA_TYPE__LONG_DOUBLE;
    else if (!strcmp(name, "c_complex"))
        return YAKSA_TYPE__C_COMPLEX;
    else if (!strcmp(name, "c_double_complex"))
        return YAKSA_TYPE__C_DOUBLE_COMPLEX;
    else if (!strcmp(name, "c_long_double_complex"))
        return YAKSA_TYPE__C_LONG_DOUBLE_COMPLEX;
    else if (!strcmp(name, "float_int"))
        return YAKSA_TYPE__FLOAT_INT;
    else if (!strcmp(name, "double_int"))
        return YAKSA_TYPE__DOUBLE_INT;
    else if (!strcmp(name, "long_int"))
        return YAKSA_TYPE__LONG_INT;
    else if (!strcmp(name, "2int"))
        return YAKSA_TYPE__2INT;
    else if (!strcmp(name, "short_int"))
        return YAKSA_TYPE__SHORT_INT;
    else if (!strcmp(name, "long_double_int"))
        return YAKSA_TYPE__LONG_DOUBLE_INT;

    return YAKSA_TYPE__NULL;
}

int DTPI_parse_base_type_str(DTP_pool_s * dtp, const char *str)
{
    yaksa_type_t *array_of_types = NULL;
    intptr_t *array_of_blklens = NULL;
    char **typestr = NULL;
    char **countstr = NULL;
    int num_types = 0;
    int i;
    intptr_t lb;
    DTPI_pool_s *dtpi = dtp->priv;
    int rc = DTP_SUCCESS;

    DTPI_FUNC_ENTER();

    DTPI_ERR_ASSERT(str, rc);

    strncpy(dtpi->base_type_str, str, DTPI_MAX_BASE_TYPE_STR_LEN);

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

    DTPI_ALLOC_OR_FAIL(array_of_types, num_types * sizeof(yaksa_type_t), rc);
    DTPI_ALLOC_OR_FAIL(array_of_blklens, num_types * sizeof(intptr_t), rc);

    for (i = 0; i < num_types; i++) {
        array_of_types[i] = name_to_type(typestr[i]);

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
    intptr_t *array_of_displs;
    DTPI_ALLOC_OR_FAIL(array_of_displs, num_types * sizeof(intptr_t), rc);
    intptr_t displ = 0;

    for (i = 0; i < num_types; i++) {
        uintptr_t size;
        rc = yaksa_type_get_size(array_of_types[i], &size);
        DTPI_ERR_CHK_RC(rc);

        int diff = displ % type_alignment(array_of_types[i]);
        if (diff) {
            displ += type_alignment(array_of_types[i]) - diff;
        }

        array_of_displs[i] = displ;
        displ += (array_of_blklens[i] * size);
    }

    rc = yaksa_type_create_struct(num_types, array_of_blklens, array_of_displs, array_of_types,
                                  NULL, &dtp->DTP_base_type);
    DTPI_ERR_CHK_RC(rc);

    dtpi->base_type_is_struct = 1;
    dtpi->base_type_attrs.numblks = num_types;
    dtpi->base_type_attrs.array_of_blklens = array_of_blklens;
    dtpi->base_type_attrs.array_of_displs = array_of_displs;
    dtpi->base_type_attrs.array_of_types = array_of_types;

  fn_exit:
    yaksa_type_get_extent(dtp->DTP_base_type, &lb, &dtpi->base_type_extent);
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

int DTPI_rand(DTPI_pool_s * dtpi)
{
    int ret;
    int rc = DTP_SUCCESS;

    DTPI_FUNC_ENTER();

    ret = dtpi->rand_list[dtpi->rand_idx];
    dtpi->rand_idx++;

  fn_exit:
    DTPI_FUNC_EXIT();
    assert(rc == DTP_SUCCESS);
    return ret;

  fn_fail:
    goto fn_exit;
}
