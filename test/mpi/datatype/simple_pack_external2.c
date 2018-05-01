#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include "mpitest.h"

char *datarep = "external32";

#define UINT_COUNT (2)
#define DBLE_COUNT (24)

int main(void)
{
    unsigned *uint_data = calloc(UINT_COUNT, sizeof(unsigned));
    double *dble_data = calloc(DBLE_COUNT, sizeof(double));
    MPI_Aint uint_pack_size, dble_pack_size;
    MPI_Aint pack_size;
    void *pack_buffer;
    MPI_Aint position = 0;

    MTest_Init(NULL, NULL);

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

    MTest_Finalize(0);

    return 0;
}
