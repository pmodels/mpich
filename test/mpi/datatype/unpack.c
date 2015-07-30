/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"
#include <stdlib.h>
#include <string.h>

/* Test sent in by Avery Ching to report a bug in MPICH.
   Adding it as a regression test. */

/*
static void print_char_buf(char *buf_name, char *buf, int buf_len)
{
    int i;

    printf("print_char_buf: %s\n", buf_name);
    for (i = 0; i < buf_len; i++)
    {
        printf("%c ", buf[i]);
        if (((i + 1) % 10) == 0)
            printf("\n");
        else if (((i + 1) % 5) == 0)
            printf("  ");
    }
    printf("\n");
}
*/

char correct_buf[] = { 'a', '_', 'b', 'c', '_', '_', '_', '_', 'd', '_',
    'e', 'f', 'g', '_', 'h', 'i', 'j', '_', 'k', 'l',
    '_', '_', '_', '_', 'm', '_', 'n', 'o', 'p', '_',
    'q', 'r'
};

#define COUNT 2

int main(int argc, char **argv)
{
    int myid, numprocs, i;
    char *mem_buf = NULL, *unpack_buf = NULL;
    MPI_Datatype tmp_dtype, mem_dtype;
    MPI_Aint mem_dtype_ext = -1;
    int mem_dtype_sz = -1;
    int mem_buf_sz = -1, unpack_buf_sz = -1, buf_pos = 0;

    int blk_arr[COUNT] = { 1, 2 };
    int dsp_arr[COUNT] = { 0, 2 };
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    /* Creating the datatype to use for unpacking */
    MPI_Type_indexed(COUNT, blk_arr, dsp_arr, MPI_CHAR, &tmp_dtype);
    MPI_Type_commit(&tmp_dtype);
    MPI_Type_indexed(COUNT, blk_arr, dsp_arr, tmp_dtype, &mem_dtype);
    MPI_Type_free(&tmp_dtype);
    MPI_Type_commit(&mem_dtype);

    MPI_Type_size(mem_dtype, &mem_dtype_sz);
    MPI_Type_extent(mem_dtype, &mem_dtype_ext);

    mem_buf_sz = 2 * mem_dtype_ext;
    unpack_buf_sz = 2 * mem_dtype_sz;

    if ((mem_buf = (char *) malloc(mem_buf_sz)) == NULL) {
        fprintf(stderr, "malloc mem_buf of size %d failed\n", mem_buf_sz);
        return -1;
    }
    memset(mem_buf, '_', mem_buf_sz);

    if ((unpack_buf = (char *) malloc(unpack_buf_sz)) == NULL) {
        fprintf(stderr, "malloc unpack_buf of size %d failed\n", unpack_buf_sz);
        return -1;
    }

    for (i = 0; i < unpack_buf_sz; i++)
        unpack_buf[i] = 'a' + i;

    /* print_char_buf("mem_buf before unpack", mem_buf, 2 * mem_dtype_ext); */

    MPI_Unpack(unpack_buf, unpack_buf_sz, &buf_pos, mem_buf, 2, mem_dtype, MPI_COMM_SELF);
    /* Note: Unpack without a Pack is not technically correct, but should work
     * with MPICH. */

    /* print_char_buf("mem_buf after unpack", mem_buf, 2 * mem_dtype_ext);
     * print_char_buf("correct buffer should be",
     * correct_buf, 2 * mem_dtype_ext); */

    if (memcmp(mem_buf, correct_buf, 2 * mem_dtype_ext)) {
        printf("Unpacked buffer does not match expected buffer\n");
        errs++;
    }

    MPI_Type_free(&mem_dtype);

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
