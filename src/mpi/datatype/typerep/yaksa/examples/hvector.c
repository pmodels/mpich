/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <yaksa.h>
#include <assert.h>
#include "matrix_util.h"

int main()
{
    int rc;
    int input_matrix[SIZE];
    int pack_buf[SIZE];
    int unpack_buf[SIZE];
    yaksa_type_t hvector;

    yaksa_init(NULL);   /* before any yaksa API is called the library
                         * must be initialized */

    init_matrix(input_matrix, ROWS, COLS);
    set_matrix(pack_buf, ROWS, COLS, 0);
    set_matrix(unpack_buf, ROWS, COLS, 0);

    rc = yaksa_type_create_hvector(ROWS, 1, COLS * sizeof(int), YAKSA_TYPE__INT, NULL, &hvector);
    assert(rc == YAKSA_SUCCESS);

    yaksa_request_t request;
    uintptr_t actual_pack_bytes;

    rc = yaksa_ipack(input_matrix, 1, hvector, 0, pack_buf, ROWS * sizeof(int), &actual_pack_bytes,
                     NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    uintptr_t actual_unpack_bytes;
    rc = yaksa_iunpack(pack_buf, ROWS * sizeof(int), unpack_buf, 1, hvector, 0,
                       &actual_unpack_bytes, NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    print_matrix(input_matrix, ROWS, ROWS, "input_matrix=");
    print_matrix(unpack_buf, ROWS, ROWS, "unpack_buf=");

    set_matrix(unpack_buf, ROWS, COLS, 0);

    /* pack second column */
    rc = yaksa_ipack(input_matrix + 1, 1, hvector, 0, pack_buf, ROWS * sizeof(int),
                     &actual_pack_bytes, NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    rc = yaksa_iunpack(pack_buf, ROWS * sizeof(int), unpack_buf + 1, 1, hvector, 0,
                       &actual_unpack_bytes, NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    print_matrix(unpack_buf, ROWS, ROWS, "unpack_buf+1=");
    yaksa_type_free(hvector);

    /* matrix transposition using hvector */
    yaksa_type_t vector;

    rc = yaksa_type_create_vector(ROWS, 1, COLS, YAKSA_TYPE__INT, NULL, &vector);
    assert(rc == YAKSA_SUCCESS);

    rc = yaksa_type_create_hvector(COLS, 1, sizeof(int), vector, NULL, &hvector);
    assert(rc == YAKSA_SUCCESS);

    set_matrix(pack_buf, ROWS, COLS, 0);
    set_matrix(unpack_buf, ROWS, COLS, 0);

    rc = yaksa_ipack(input_matrix, 1, hvector, 0, pack_buf, 256, &actual_pack_bytes,
                     NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    rc = yaksa_iunpack(pack_buf, 256, unpack_buf, 1, hvector, 0, &actual_unpack_bytes,
                       NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    fprintf(stdout, "\nMatrix transposition:\n\n");
    print_matrix(pack_buf, ROWS, COLS, "pack_buf=");
    print_matrix(unpack_buf, ROWS, ROWS, "unpack_buf=");

    yaksa_type_free(vector);
    yaksa_type_free(hvector);
    yaksa_finalize();
    return 0;
}
