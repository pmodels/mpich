/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpi.h>
#include "mpitest.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define my_assert(cond_)                                                  \
    do {                                                                  \
        if (!(cond_)) {                                                   \
            fprintf(stderr, "assertion (%s) failed, aborting\n", #cond_); \
            MPI_Abort(MPI_COMM_WORLD, 1);                                 \
        }                                                                 \
    } while (0)

static const int SQ_LIMIT = 10;
static int SQ_COUNT = 0;
static int SQ_VERBOSE = 0;

#define SQUELCH(X) \
    do { \
        if (SQ_COUNT < SQ_LIMIT || SQ_VERBOSE) { \
            SQ_COUNT++; \
            X; \
        } \
    } while (0)

#define BUF_LEN_ZERO  (64*1024)
#define BUF_LEN_ONE   (4*1024)
#define BUF_LEN_TOTAL (BUF_LEN_ZERO+BUF_LEN_ONE)
#define BUF_VAL_ZERO  (0x42)
#define BUF_VAL_ONE   (0x13)

#define STAGE_BUF (0)
#define MAIN_BUF  (1)

static const char bufvals[2] = { BUF_VAL_ZERO, BUF_VAL_ONE };

#if defined(HAVE_CUDA) || defined(HAVE_ZE) || defined(HAVE_HIP)
#define MEMTYPE_COUNT (2)
static mtest_mem_type_e memtypes[MEMTYPE_COUNT] = {
    MTEST_MEM_TYPE__UNREGISTERED_HOST,
    MTEST_MEM_TYPE__DEVICE
};
#else
#define MEMTYPE_COUNT (1)
static mtest_mem_type_e memtypes[MEMTYPE_COUNT] = {
    MTEST_MEM_TYPE__UNREGISTERED_HOST,
};
#endif

static void create_and_unpack_datatype(void *inbuf, int blocks[2],
                                       MPI_Aint displs[2], MPI_Datatype types[2])
{
    MPI_Datatype tmp_type;
    int position = 0;
    int packsize = 0;

    /* create the datatype and verify the packsize */
    MPI_Type_create_struct(2, blocks, displs, types, &tmp_type);
    MPI_Type_commit(&tmp_type);
    MPI_Pack_size(1, tmp_type, MPI_COMM_WORLD, &packsize);
    my_assert(packsize == BUF_LEN_TOTAL);

    /* do the actual packing */
    MPI_Unpack(inbuf, packsize, &position, MPI_BOTTOM, 1, tmp_type, MPI_COMM_WORLD);

    /* free the datatype and the input buffers */
    MPI_Type_free(&tmp_type);

}


static
void alloc_buffers(void *outbufs[2][2], MPI_Aint displs[2], int blocks[2],
                   mtest_mem_type_e memtypes[2])
{
    for (int outbuf = 0; outbuf < 2; ++outbuf) {
        MTestCalloc(blocks[outbuf], memtypes[outbuf],
                    (void **) &outbufs[outbuf][STAGE_BUF], (void **) &outbufs[outbuf][MAIN_BUF], 0);

        MPI_Get_address(outbufs[outbuf][MAIN_BUF], &displs[outbuf]);
    }
}


static
void free_and_check_buffers(void *outbufs[2][2], mtest_mem_type_e memtypes[2],
                            int blocks[2], int *errs)
{
    /* check the output buffer */
    for (int cur_outbuf = 0; cur_outbuf < 2; ++cur_outbuf) {
        MTestCopyContent(outbufs[cur_outbuf][MAIN_BUF],
                         outbufs[cur_outbuf][STAGE_BUF], blocks[cur_outbuf], memtypes[cur_outbuf]);

        char *check_buf = (char *) outbufs[cur_outbuf][STAGE_BUF];

        for (int i = 0; i < blocks[cur_outbuf]; ++i) {
            if (check_buf[i] != bufvals[cur_outbuf]) {
                SQUELCH(printf("Error: check_buf[%d] = %x != %x\n", i,
                               check_buf[i], bufvals[cur_outbuf]));
                ++(*errs);
            }
        }
        SQ_COUNT = 0;

        /* free the output buffers */
        MTestFree(memtypes[cur_outbuf], outbufs[cur_outbuf][STAGE_BUF],
                  outbufs[cur_outbuf][MAIN_BUF]);
    }
}


static const char *memtype_str(mtest_mem_type_e memtype)
{
    static char buf[64];

    switch (memtype) {
        case MTEST_MEM_TYPE__UNREGISTERED_HOST:
            return "host";
#ifdef HAVE_CUDA
        case MTEST_MEM_TYPE__DEVICE:
            return "cuda";
#elif HAVE_HIP
        case MTEST_MEM_TYPE__DEVICE:
            return "hip";
#elif HAVE_ZE
        case MTEST_MEM_TYPE__DEVICE:
            return "ze";
#endif
    }

    snprintf(buf, sizeof(buf), "%d", memtype);
    return buf;
}


int main(int argc, char *argv[])
{
    int errs = 0;
    int verbose = 0;

    void *inbufs[2];
    void *outbufs[2][2];

    MPI_Datatype types[2] = { MPI_BYTE, MPI_BYTE };
    int blocks[2] = { BUF_LEN_ZERO, BUF_LEN_ONE };
    MPI_Aint displs[2] = { 0, 0 };
    mtest_mem_type_e outbuf_mtypes[2];
    mtest_mem_type_e inbuf_mtype;

    MTest_Init(&argc, &argv);

    /* iterate all combinations of input and output memory types */
    for (int h = 0; h < MEMTYPE_COUNT; ++h) {
        inbuf_mtype = memtypes[h];

        /* allocate the input buffer */
        MTestCalloc(BUF_LEN_TOTAL, inbuf_mtype, (void **) &inbufs[STAGE_BUF],
                    (void **) &inbufs[1], 0);

        /* initialize the input buffer */
        memset(inbufs[STAGE_BUF], bufvals[0], BUF_LEN_ZERO);
        memset(inbufs[STAGE_BUF] + BUF_LEN_ZERO, bufvals[1], BUF_LEN_ONE);
        MTestCopyContent(inbufs[STAGE_BUF], inbufs[MAIN_BUF], BUF_LEN_TOTAL, inbuf_mtype);

        for (int i = 0; i < MEMTYPE_COUNT; ++i) {
            outbuf_mtypes[0] = memtypes[i];

            for (int j = 0; j < MEMTYPE_COUNT; ++j) {
                outbuf_mtypes[1] = memtypes[j];

                alloc_buffers(outbufs, displs, blocks, outbuf_mtypes);

                if (verbose) {
                    printf("Unpack: { %s (%16p) -> %s (%16p); %s (%16p) }\n",
                           memtype_str(inbuf_mtype), inbufs[1],
                           memtype_str(outbuf_mtypes[0]), outbufs[0][MAIN_BUF],
                           memtype_str(outbuf_mtypes[1]), outbufs[1][MAIN_BUF]);
                }

                create_and_unpack_datatype(inbufs[MAIN_BUF], blocks, displs, types);

                free_and_check_buffers(outbufs, outbuf_mtypes, blocks, &errs);

            }
        }

        MTestFree(inbuf_mtype, inbufs[STAGE_BUF], inbufs[MAIN_BUF]);
    }

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
