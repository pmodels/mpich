/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run datatype_simple_pack_external2
int run(const char *arg);
#endif

static const char *datarep = "external32";

#define UINT_COUNT (2)
#define DBLE_COUNT (24)

int run(const char *arg)
{
    unsigned *uint_data = calloc(UINT_COUNT, sizeof(unsigned));
    double *dble_data = calloc(DBLE_COUNT, sizeof(double));
    MPI_Aint uint_pack_size, dble_pack_size;
    MPI_Aint pack_size;
    void *pack_buffer;
    MPI_Aint position = 0;

    MPI_Pack_external_size(datarep, UINT_COUNT, MPI_UNSIGNED, &uint_pack_size);
    MPI_Pack_external_size(datarep, DBLE_COUNT, MPI_DOUBLE, &dble_pack_size);

    pack_size = uint_pack_size + dble_pack_size;
    pack_buffer = malloc(pack_size);

    MPI_Pack_external(datarep, uint_data, UINT_COUNT, MPI_UNSIGNED, pack_buffer, pack_size,
                      &position);
    MPI_Pack_external(datarep, dble_data, DBLE_COUNT, MPI_DOUBLE, pack_buffer, pack_size,
                      &position);

    free(pack_buffer);
    free(dble_data);
    free(uint_data);

    return 0;
}
