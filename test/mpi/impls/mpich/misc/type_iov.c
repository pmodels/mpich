/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"
#include "stdlib.h"
#include "dtpools.h"
#include <assert.h>

/* Test MPIX_Type_iov and MPIX_Type_iov_len */

static int test_type_iovs(DTP_obj_s obj);

int main(int argc, char *argv[])
{
    int err, errs = 0;

    MTest_Init(&argc, &argv);

    int seed, testsize;
    char *basic_type;
    MPI_Aint count, maxbufsize;

    MTestArgList *head = MTestArgListCreate(argc, argv);
    seed = MTestArgListGetInt(head, "seed");
    testsize = MTestArgListGetInt(head, "testsize");
    count = MTestArgListGetLong(head, "count");
    basic_type = MTestArgListGetString(head, "type");

    maxbufsize = MTestDefaultMaxBufferSize();

    DTP_pool_s dtp;
    err = DTP_pool_create(basic_type, count, seed, &dtp);
    MTestArgListDestroy(head);
    if (err != DTP_SUCCESS) {
        errs++;
        printf("Error while creating pool (%s,%ld)\n", basic_type, count);
        goto fn_exit;
    }

    for (int obj_idx = 0; obj_idx < testsize; obj_idx++) {
        DTP_obj_s obj;
        err = DTP_obj_create(dtp, &obj, maxbufsize);
        if (err != DTP_SUCCESS) {
            errs++;
            break;
        }

        MTestPrintfMsg(1, "obj_idx=%d, %s\n", obj_idx, DTP_obj_get_description(obj));

        errs += test_type_iovs(obj);
        DTP_obj_free(obj);
        if (errs > 0)
            break;
    }

    DTP_pool_free(dtp);

  fn_exit:
    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            errs++; \
            if (errs < 10) { \
                printf("Check failure (%s) at line %d\n", #cond, __LINE__); \
                printf("    %s\n", DTP_obj_get_description(obj)); \
            } \
            goto fn_exit; \
        } \
    } while (0)

static int test_type_iovs(DTP_obj_s obj)
{
    int errs = 0;
    MPI_Datatype datatype = obj.DTP_datatype;

    MPI_Count num_iovs, type_size;

    MPI_Type_size_c(datatype, &type_size);

    /* Get total MPIX_Type_iov_len */
    MPI_Count actual_bytes;
    MPIX_Type_iov_len(datatype, type_size, &num_iovs, &actual_bytes);
    CHECK(num_iovs > 0);
    CHECK(actual_bytes == type_size);

    /* get entire iovs array */
    MPIX_Iov *iovs;
    MPI_Count *byte_offsets;
    iovs = malloc(num_iovs * sizeof(MPIX_Iov));
    byte_offsets = malloc((num_iovs + 1) * sizeof(MPI_Count));

    MPI_Count actual_num_iovs;
    MPIX_Type_iov(datatype, 0, iovs, num_iovs, &actual_num_iovs);
    CHECK(actual_num_iovs == num_iovs);

    byte_offsets[0] = 0;
    for (MPI_Count i = 0; i < num_iovs; i++) {
        CHECK(iovs[i].iov_len > 0);
        byte_offsets[i + 1] = byte_offsets[i] + iovs[i].iov_len;
    }
    CHECK(byte_offsets[num_iovs] == type_size);

    /* Test pack/unpack -- assume pack_size is exact type size */
    MPI_Aint bufsize;
    bufsize = obj.DTP_bufsize;
    int pack_size;
    MPI_Pack_size(1, datatype, MPI_COMM_WORLD, &pack_size);

    char *buf = malloc(bufsize);
    char *check_buf = malloc(bufsize);
    char *pack_buf = malloc(pack_size);
    char *check_pack_buf = malloc(pack_size);
    assert(buf && pack_buf && check_buf && check_pack_buf);

    char *unpack_buf = buf + obj.DTP_buf_offset;
    char *check_unpack_buf = check_buf + obj.DTP_buf_offset;

    /* let unpacked buffer be segments of 0 and 1 */
    memset(check_buf, 0, bufsize);
    for (MPI_Count i = 0; i < num_iovs; i++) {
        memset(check_unpack_buf + (MPI_Aint) iovs[i].iov_base, 1, iovs[i].iov_len);
    }
    /* let packed buffer be all 1 */
    memset(check_pack_buf, 1, pack_size);

    int pos;
    pos = 0;
    MPI_Pack(check_unpack_buf, 1, datatype, pack_buf, pack_size, &pos, MPI_COMM_WORLD);
    assert(pos == pack_size);
    CHECK(memcmp(pack_buf, check_pack_buf, pack_size) == 0);

    memset(buf, 0, bufsize);
    pos = 0;
    MPI_Unpack(check_pack_buf, pack_size, &pos, unpack_buf, 1, datatype, MPI_COMM_WORLD);
    assert(pos == pack_size);
    CHECK(memcmp(buf, check_buf, bufsize) == 0);

    /* Test random MPIX_Type_iov_len */
    for (int i = 0; i < 10; i++) {
        MPI_Count offset = type_size * 3 / 2 * rand() / RAND_MAX;
        MPI_Count idx;
        MPIX_Type_iov_len(datatype, offset, &idx, &actual_bytes);
        CHECK(idx >= 0);
        CHECK(idx <= num_iovs);
        if (offset >= type_size) {
            CHECK(idx == num_iovs);
            CHECK(actual_bytes == type_size);
        } else {
            CHECK(actual_bytes == byte_offsets[idx]);
            CHECK(offset >= byte_offsets[idx]);
            CHECK(idx < num_iovs);
            CHECK(offset < byte_offsets[idx + 1]);

            MPIX_Iov iov;
            MPIX_Type_iov(datatype, idx, &iov, 1, &actual_num_iovs);
            CHECK(actual_num_iovs == 1);
            CHECK(iov.iov_base == iovs[idx].iov_base);
            CHECK(iov.iov_len == iovs[idx].iov_len);
        }
    }

    free(iovs);
    free(byte_offsets);
    free(buf);
    free(check_buf);
    free(pack_buf);
    free(check_pack_buf);
  fn_exit:
    return errs;
}
