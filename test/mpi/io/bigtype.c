#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

//#define NUM_X 536870911
#define NUM_X 536870912
#define NUM_Y 1

//#define BIGDT 2147483643
#define BIGDT 2147483647

int main(int argc, char **argv)
{

    MPI_File fh;
    int i, j;
    size_t k;
    MPI_Datatype inner_type, rem_type, mem_type;
    MPI_Datatype int_type, file_type;
    int *buf_write, *buf_read;
    int rc;
    MPI_Aint disp[2];
    int block_len[2];
    MPI_Datatype type[2];

    MPI_Init(&argc, &argv);

    if (sizeof(MPI_Aint) <= sizeof(int)) {
        /* can't test on this platform... */
        goto exit;
    }

    k = 0;
    /* create a large buffer 2 */
    buf_write = malloc(NUM_X * NUM_Y * sizeof(int));
    if (buf_write == NULL) {
        fprintf(stderr, "not enough memory\n");
        exit(1);
    }
    buf_read = malloc(NUM_X * NUM_Y * sizeof(int));
    if (buf_read == NULL) {
        fprintf(stderr, "not enough memory\n");
        exit(1);
    }
    memset(buf_read, 0, NUM_X * NUM_Y * sizeof(int));

    for (i = 0; i < NUM_X; i++) {
        for (j = 0; j < NUM_Y; j++) {
            buf_write[k] = k;
            k++;
        }
    }

    /* Big Datatype (2^31 - 1 bytes) */
    MPI_Type_contiguous(BIGDT, MPI_BYTE, &inner_type);
    /* Small Datatype (1 byte) */
    MPI_Type_contiguous(1, MPI_BYTE, &rem_type);

    type[0] = inner_type;
    type[1] = rem_type;
    block_len[0] = 1;
    block_len[1] = 1;
    disp[0] = 0;
    disp[1] = BIGDT;

    /* combine both types */
    MPI_Type_struct(2, block_len, disp, type, &mem_type);

    MPI_Type_commit(&mem_type);
    MPI_Type_free(&rem_type);
    MPI_Type_free(&inner_type);

    MPI_Type_contiguous(4, MPI_BYTE, &int_type);
    {
        /* This creates a big type that is actually contituous, touching an
         * optimization that was at one point buggy  */
        MPI_Type_vector(1, NUM_X, 1, int_type, &file_type);
    }

    MPI_Type_commit(&file_type);
    MPI_Type_free(&int_type);

    if (MPI_File_open(MPI_COMM_WORLD, "testfile", MPI_MODE_RDWR | MPI_MODE_CREATE,
                      MPI_INFO_NULL, &fh) != 0) {
        fprintf(stderr, "Can't open file: %s\n", "testfile");
        exit(1);
    }

    if (MPI_SUCCESS != MPI_File_set_view(fh, 2144, MPI_BYTE, file_type, "native", MPI_INFO_NULL)) {
        fprintf(stderr, "ERROR SET VIEW\n");
        exit(1);
    }

    /* write everything */
    rc = MPI_File_write_at_all(fh, 0, buf_write, 1, mem_type, MPI_STATUS_IGNORE);
    if (rc != MPI_SUCCESS) {
        fprintf(stderr, "%d ERROR WRITE AT ALL\n", rc);
        exit(1);
    }

    if (MPI_SUCCESS != MPI_File_set_view(fh, 2144, MPI_BYTE, file_type, "native", MPI_INFO_NULL)) {
        fprintf(stderr, "ERROR SET VIEW\n");
        exit(1);
    }

    /* read everything */
    rc = MPI_File_read_at_all(fh, 0, buf_read, 1, mem_type, MPI_STATUS_IGNORE);
    if (rc != MPI_SUCCESS) {
        fprintf(stderr, "%d ERROR READ AT ALL\n", rc);
        exit(1);
    }

    for (k = 0; k < NUM_X * NUM_Y; k++) {
        if (buf_read[k] != buf_write[k]) {
            fprintf(stderr, "Verfiy Failed index %zu: expected %d found %d\n",
                    k, buf_write[k], buf_read[k]);
            assert(0);
        }
    }

    free(buf_write);
    free(buf_read);
    MPI_File_close(&fh);

    MPI_Type_free(&mem_type);
    MPI_Type_free(&file_type);

  exit:
    MPI_Finalize();
    printf(" No Errors\n");

    return 0;
}
