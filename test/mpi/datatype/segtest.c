/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpiimpl.h"
#include <stdio.h>

/*
 * Simple segment test, including timing code
 */

/*
 * Build datatype structures
 *
 * Contiguous
 *     n = 1, 4, 16, 64, 128, 512, 2048, 8196 ints
 * Vector
 *     blocksize = 1, 4, 64 ints
 *     stride    = 1, 64, 127
 * Block Indexed
 *     blocksize = 1, 4 ints
 *     offsets   = i*24 for i = 0 to n, n = 0, 64, 512
 * Indexed
 *     blocksizes = 1, 2, 4, 3, 7, 5, 6
 *     offsets    = i*24 for i = 0 to n, n = 0, 4, 7, 64, 512
 *     (Wrap blocksizes to match offsets)
 *
 * Also need a few nested datatypes, such as vector of vectors
 * Do the versions in Using MPI
 *
 */

/*
 * Routines to create dataloops for basic dataloops
 */
/*
 *  Contig
 */
MPIR_Dataloop *MPIR_Dataloop_init_contig(int count)
{
    MPIR_Dataloop *ct;

    ct = (MPIR_Dataloop *) MPL_malloc(sizeof(MPIR_Dataloop));
    ct->kind = MPID_DTYPE_CONTIG | DATALOOP_FINAL_MASK;
    ct->loop_params.c_t.count = count;
    ct->loop_params.c_t.dataloop = 0;
    ct->extent = count;
    ct->handle = 0;

    return ct;
}

/*
 * Vector
 */
MPIR_Dataloop *MPIR_Dataloop_init_vector(int count, int blocksize, int stride)
{
    MPIR_Dataloop *v;

    v = (MPIR_Dataloop *) MPL_malloc(sizeof(MPIR_Dataloop));
    v->kind = MPID_DTYPE_VECTOR | DATALOOP_FINAL_MASK;
    v->loop_params.v_t.count = count;
    v->loop_params.v_t.blocksize = blocksize;
    v->loop_params.v_t.stride = stride;
    v->loop_params.v_t.dataloop = 0;
    v->extent = (count - 1) * stride + blocksize;
    v->handle = 0;

    return v;
}

/*
 * Block indexed
 */
MPIR_Dataloop *MPIR_Dataloop_init_blockindexed(int count, int blocksize, MPI_Aint * offset)
{
    MPIR_Dataloop *bi;
    MPI_Aint extent;
    int i;

    bi = (MPIR_Dataloop *) MPL_malloc(sizeof(MPIR_Dataloop));
    bi->kind = MPID_DTYPE_BLOCKINDEXED | DATALOOP_FINAL_MASK;
    bi->loop_params.bi_t.count = count;
    bi->loop_params.bi_t.blocksize = blocksize;
    bi->loop_params.bi_t.offset = (MPI_Aint *) MPL_malloc(sizeof(MPI_Aint) * count);
    for (i = 0; i < count; i++) {
        bi->loop_params.bi_t.offset[i] = offset[i];
        if (offset[i] + blocksize > extent)
            extent = offset[i] + blocksize;
    }
    bi->loop_params.bi_t.dataloop = 0;
    bi->extent = extent;
    bi->handle = 0;

    return bi;
}

/*
 * Indexed
 */
MPIR_Dataloop *MPIR_Dataloop_init_indexed(int count, int *blocksize, MPI_Aint * offset)
{
    MPIR_Dataloop *it;
    MPI_Aint extent = 0;
    int i;

    it = (MPIR_Dataloop *) MPL_malloc(sizeof(MPIR_Dataloop));
    it->kind = MPID_DTYPE_INDEXED | DATALOOP_FINAL_MASK;
    it->loop_params.i_t.count = count;
    it->loop_params.i_t.blocksize = (int *) MPL_malloc(sizeof(int) * count);
    it->loop_params.i_t.offset = (MPI_Aint *) MPL_malloc(sizeof(MPI_Aint) * count);
    for (i = 0; i < count; i++) {
        it->loop_params.i_t.offset[i] = offset[i];
        it->loop_params.i_t.blocksize[i] = blocksize[i];
        if (offset[i] + blocksize[i] > extent)
            extent = offset[i] + blocksize[i];
    }
    it->loop_params.i_t.dataloop = 0;
    it->extent = extent;
    it->handle = 0;

    return it;
}

int main(int argc, char **argv)
{
    /* MPIR_Dataloop *vecloop; */
    MPI_Datatype vectype;
    int count = 200, blocksize = 4, stride = 7 * 4;
    char *src_buf, *dest_buf;
    int i, j, k;
    double r1, r2;

    MPI_Init(&argc, &argv);

/*    vecloop = MPIR_Dataloop_init_vector(count, blocksize, stride); */

    MPI_Type_vector(count, 1, 7, MPI_INT, &vectype);

    /* Initialize the data */
    src_buf = (char *) MPL_malloc((count - 1) * stride + blocksize);
    for (i = 0; i < (count - 1) * stride + blocksize; i++)
        src_buf[i] = -i;
    for (i = 0; i < count; i++) {
        for (j = 0; j < blocksize; j++)
            src_buf[i * stride + j] = i * blocksize + j;
    }
    dest_buf = (char *) MPL_malloc(count * blocksize);
    for (i = 0; i < count * blocksize; i++) {
        dest_buf[i] = -i;
    }
    r1 = MPI_Wtime();
    for (i = 0; i < 100; i++) {
        int position = 0;
        /*MPIR_Segment_pack(vecloop, src_buf, dest_buf); */
        MPI_Pack(src_buf, count, vectype, dest_buf, count * blocksize, &position, MPI_COMM_WORLD);
    }
    r2 = MPI_Wtime();
    printf("Timer for vector pack is %e\n", (r2 - r1) / 100);
    for (i = 0; i < count * blocksize; i++) {
        if (dest_buf[i] != (char) i) {
            printf("Error at location %d\n", i);
        }
    }
    r1 = MPI_Wtime();
    for (k = 0; k < 100; k++) {
        char *dest = dest_buf, *src = src_buf;
        for (i = 0; i < count; i++) {
            for (j = 0; j < blocksize; j++)
                *dest++ = src[j];
            src += stride;
        }
    }
    r2 = MPI_Wtime();
    printf("Timer for hand vector pack is %e\n", (r2 - r1) / 100);

    r1 = MPI_Wtime();
    for (k = 0; k < 100; k++) {
        int *dest = (int *) dest_buf, *src = (int *) src_buf;
        int bsize = blocksize >> 2;
        int istride = stride >> 2;
        if (bsize == 1) {
            for (i = 0; i < count; i++) {
                *dest++ = *src;
                src += istride;
            }
        }
        else {
            for (i = 0; i < count; i++) {
                for (j = 0; j < bsize; j++)
                    *dest++ = src[j];
                src += istride;
            }
        }
    }
    r2 = MPI_Wtime();
    printf("Timer for hand vector pack (int) is %e\n", (r2 - r1) / 100);

    MPI_Finalize();
    return 0;
}

/*
 * Nested vector.
 *   The y-z subface is
 *   Type_vector(ey-sy+1, 1, nx, MPI_DOUBLE, &newx1);
 *   Type_hvector(ez-sz+1, 1, nx*ny_sizeof(double), newx1, &newx);
 * This gives the a(i,sy:ey,sz:ez) of a(nx,ny,nz) (in Fortran notation)
 */
