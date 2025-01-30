/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <yaksa.h>
#include <assert.h>
#include <stdlib.h>
#include "matrix_util.h"

#define BLKLEN (4)

int main()
{
    int rc;
    int input_matrix[SIZE];
    int pack_buf[SIZE];
    int unpack_buf[SIZE];
    yaksa_type_t indexed_block;
    intptr_t array_of_displacements[ROWS] = {
        4, 12, 20, 28,
        32, 40, 48, 56
    };

    yaksa_init(NULL);   /* before any yaksa API is called the library
                         * must be initialized */

    init_matrix(input_matrix, ROWS, COLS);
    set_matrix(pack_buf, ROWS, COLS, 0);
    set_matrix(unpack_buf, ROWS, COLS, 0);

    rc = yaksa_type_create_indexed_block(ROWS, BLKLEN, array_of_displacements, YAKSA_TYPE__INT,
                                         NULL, &indexed_block);
    assert(rc == YAKSA_SUCCESS);

    uintptr_t flatten_size;
    yaksa_flatten_size(indexed_block, &flatten_size);

    void *flatten_type = malloc(flatten_size);
    yaksa_flatten(indexed_block, flatten_type);
    yaksa_type_free(indexed_block);

    yaksa_type_t unflatten_type;
    rc = yaksa_unflatten(&unflatten_type, flatten_type);
    assert(rc == YAKSA_SUCCESS);

    uintptr_t size;
    yaksa_type_get_size(unflatten_type, &size);

    yaksa_request_t request;
    uintptr_t actual_pack_bytes;
    rc = yaksa_ipack(input_matrix, 1, unflatten_type, 0, pack_buf, ROWS * BLKLEN * sizeof(int),
                     &actual_pack_bytes, NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    uintptr_t actual_unpack_bytes;
    rc = yaksa_iunpack(pack_buf, size, unpack_buf, 1, unflatten_type, 0, &actual_unpack_bytes,
                       NULL, YAKSA_OP__REPLACE, &request);
    assert(rc == YAKSA_SUCCESS);
    rc = yaksa_request_wait(request);
    assert(rc == YAKSA_SUCCESS);

    print_matrix(input_matrix, ROWS, COLS, "input_matrix=");
    print_matrix(unpack_buf, ROWS, COLS, "unpack_buf=");

    yaksa_type_free(unflatten_type);
    free(flatten_type);
    yaksa_finalize();
    return 0;
}
