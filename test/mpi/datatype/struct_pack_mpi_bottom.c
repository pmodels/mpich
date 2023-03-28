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

static void create_and_pack_datatype(void *outbuf, int blocks[2],
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
    MPI_Pack(MPI_BOTTOM, 1, tmp_type, outbuf, packsize, &position, MPI_COMM_WORLD);

    /* free the datatype and the input buffers */
    MPI_Type_free(&tmp_type);

}


static
void alloc_and_init_buffers(void *inbufs[2][2], MPI_Aint displs[2], int blocks[2],
                            mtest_mem_type_e memtypes[2])
{
    for (int inbuf = 0; inbuf < 2; ++inbuf) {
        MTestCalloc(blocks[inbuf], memtypes[inbuf],
                    (void **) &inbufs[inbuf][STAGE_BUF], (void **) &inbufs[inbuf][MAIN_BUF], 0);

        memset(inbufs[inbuf][0], bufvals[inbuf], blocks[inbuf]);

        MTestCopyContent(inbufs[inbuf][STAGE_BUF], inbufs[inbuf][MAIN_BUF],
                         blocks[inbuf], memtypes[inbuf]);

        MPI_Get_address(inbufs[inbuf][MAIN_BUF], &displs[inbuf]);
    }
}


static
void free_and_check_buffers(void *inbufs[2][2], mtest_mem_type_e memtypes[2],
                            void *outbufs[2], mtest_mem_type_e outb_memtype, int *errs)
{
    char *check_buf = (char *) outbufs[STAGE_BUF];

    /* check the output buffer */
    MTestCopyContent(outbufs[MAIN_BUF], outbufs[STAGE_BUF], BUF_LEN_TOTAL, outb_memtype);

    for (int i = 0; i < BUF_LEN_ZERO; ++i) {
        if (check_buf[i] != BUF_VAL_ZERO) {
            SQUELCH(printf("Error: check_buf[%d] = %x != %x\n", i, check_buf[i], BUF_VAL_ZERO));
            ++(*errs);
        }
    }
    SQ_COUNT = 0;

    for (int i = 0; i < BUF_LEN_ONE; ++i) {
        int buf_pos = BUF_LEN_ZERO + i;
        if (check_buf[buf_pos] != BUF_VAL_ONE) {
            SQUELCH(printf("Error: check_buf[%d] = %x != %x\n",
                           buf_pos, check_buf[buf_pos], BUF_VAL_ONE));
            ++errs;
        }
    }
    SQ_COUNT = 0;

    /* free the input buffers */
    for (int cur_inbuf = 0; cur_inbuf < 2; ++cur_inbuf) {
        MTestFree(memtypes[cur_inbuf], inbufs[cur_inbuf][STAGE_BUF], inbufs[cur_inbuf][MAIN_BUF]);
    }

    /* reset the output buffer */
    memset(outbufs[STAGE_BUF], 0, BUF_LEN_TOTAL);
    MTestCopyContent(outbufs[STAGE_BUF], outbufs[MAIN_BUF], BUF_LEN_TOTAL, outb_memtype);
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

    void *inbufs[2][2];
    void *outbufs[2];

    MPI_Datatype types[2] = { MPI_BYTE, MPI_BYTE };
    int blocks[2] = { BUF_LEN_ZERO, BUF_LEN_ONE };
    MPI_Aint displs[2] = { 0, 0 };
    mtest_mem_type_e inbuf_mtypes[2];
    mtest_mem_type_e outb_memtype;

    MTest_Init(&argc, &argv);

    mtest_mem_type_e memtypes[2] = {
        MTEST_MEM_TYPE__UNREGISTERED_HOST,
        MTEST_MEM_TYPE__DEVICE
    };

    /* iterate all combinations of input and output memory types */
    for (int h = 0; h < MEMTYPE_COUNT; ++h) {
        outb_memtype = memtypes[h];

        /* allocate the output buffer */
        MTestCalloc(BUF_LEN_TOTAL, outb_memtype, (void **) &outbufs[STAGE_BUF],
                    (void **) &outbufs[MAIN_BUF], 0);

        for (int i = 0; i < MEMTYPE_COUNT; ++i) {
            inbuf_mtypes[0] = memtypes[i];

            for (int j = 0; j < MEMTYPE_COUNT; ++j) {
                inbuf_mtypes[1] = memtypes[j];

                alloc_and_init_buffers(inbufs, displs, blocks, inbuf_mtypes);

                if (verbose) {
                    printf("Pack: { %s (%16p); %s (%16p) } -> %s (%16p)\n",
                           memtype_str(inbuf_mtypes[0]), inbufs[0][MAIN_BUF],
                           memtype_str(inbuf_mtypes[1]), inbufs[1][MAIN_BUF],
                           memtype_str(outb_memtype), outbufs[MAIN_BUF]);
                }

                create_and_pack_datatype(outbufs[MAIN_BUF], blocks, displs, types);

                free_and_check_buffers(inbufs, inbuf_mtypes, outbufs, outb_memtype, &errs);

            }
        }

        MTestFree(outb_memtype, outbufs[STAGE_BUF], outbufs[MAIN_BUF]);
    }

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
