/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include "yaksa_config.h"
#include "yaksa.h"
#include "dtpools.h"
#include "pack-common.h"

uintptr_t maxbufsize = 512 * 1024 * 1024;

enum {
    PACK_ORDER__UNSET,
    PACK_ORDER__NORMAL,
    PACK_ORDER__REVERSE,
    PACK_ORDER__RANDOM,
};

enum {
    OVERLAP__NONE,
    OVERLAP__REGULAR,
    OVERLAP__IRREGULAR,
};

enum {
    PACK_KIND__NONBLOCKING,
    PACK_KIND__BLOCKING,
    PACK_KIND__STREAM,
};

#define MAX_DTP_BASESTRLEN (1024)

static int verbose = 0;

#define dprintf(...)                            \
    do {                                        \
        if (verbose)                            \
            printf(__VA_ARGS__);                \
    } while (0)

static void swap_segments(uintptr_t * starts, uintptr_t * lengths, int x, int y)
{
    uintptr_t tmp = starts[x];
    starts[x] = starts[y];
    starts[y] = tmp;

    tmp = lengths[x];
    lengths[x] = lengths[y];
    lengths[y] = tmp;
}

static void host_only_get_ptr_attr(yaksa_info_t * info, int iter)
{
    if (iter % 2 == 0) {
        int rc;

        rc = yaksa_info_create(info);
        assert(rc == YAKSA_SUCCESS);

        rc = yaksa_info_keyval_append(*info, "yaksa_gpu_driver", "nogpu", strlen("nogpu"));
        assert(rc == YAKSA_SUCCESS);
    } else {
        *info = NULL;
    }
}

char typestr[MAX_DTP_BASESTRLEN + 1] = { 0 };

int seed = -1;
int basecount = -1;
int iters = -1;
int max_segments = -1;
int pack_order = PACK_ORDER__UNSET;
int overlap = -1;
int use_subdevices = 0;
int pack_kind = PACK_KIND__NONBLOCKING;
DTP_pool_s *dtp;

/* For -stream tests */
/* TODO: test multiple streams */
void *stream;

#define MAX_DEVID_LIST   (1024)
int **device_ids;

#define MAX_MEMTYPE_LIST (1024)
mem_type_e **memtypes;

yaksa_op_t int_ops[] =
    { YAKSA_OP__MAX, YAKSA_OP__MIN, YAKSA_OP__SUM, YAKSA_OP__PROD, YAKSA_OP__LAND,
    YAKSA_OP__BAND, YAKSA_OP__LOR, YAKSA_OP__BOR, YAKSA_OP__LXOR, YAKSA_OP__BXOR,
    YAKSA_OP__REPLACE
};

yaksa_op_t float_ops[] =
    { YAKSA_OP__MAX, YAKSA_OP__MIN, YAKSA_OP__SUM, YAKSA_OP__PROD, YAKSA_OP__REPLACE };

yaksa_op_t complex_ops[] = { YAKSA_OP__SUM, YAKSA_OP__PROD, YAKSA_OP__REPLACE };

yaksa_op_t single_op = YAKSA_OP__REPLACE;
yaksa_op_t *op_list = &single_op;
int op_list_len = 1;

#define MAX_OP_LIST  (1024)

yaksa_op_t **ops;

void *runtest(void *arg);
void *runtest(void *arg)
{
    DTP_obj_s sobj, dobj;
    int rc;
    uintptr_t tid = (uintptr_t) arg;
    int device_id_idx = 0;
    int memtype_idx = 0;
    int op_idx = 0;
    yaksa_request_t request;

    uintptr_t *segment_starts = (uintptr_t *) malloc(max_segments * sizeof(uintptr_t));
    uintptr_t *segment_lengths = (uintptr_t *) malloc(max_segments * sizeof(uintptr_t));

    for (int i = 0; i < iters; i++) {
        dprintf("==== iter %d ====\n", i);

        mem_type_e sbuf_memtype = memtypes[tid][memtype_idx];
        memtype_idx = (memtype_idx + 1) % MAX_MEMTYPE_LIST;
        mem_type_e dbuf_memtype = memtypes[tid][memtype_idx++];
        memtype_idx = (memtype_idx + 1) % MAX_MEMTYPE_LIST;
        mem_type_e tbuf_memtype = memtypes[tid][memtype_idx++];
        memtype_idx = (memtype_idx + 1) % MAX_MEMTYPE_LIST;

        int sbuf_devid = device_ids[tid][device_id_idx];
        device_id_idx = (device_id_idx + 1) % MAX_DEVID_LIST;
        int dbuf_devid = device_ids[tid][device_id_idx];
        device_id_idx = (device_id_idx + 1) % MAX_DEVID_LIST;
        int tbuf_devid = device_ids[tid][device_id_idx];
        device_id_idx = (device_id_idx + 1) % MAX_DEVID_LIST;

        yaksa_op_t op = ops[tid][op_idx];
        op_idx = (op_idx + 1) % MAX_OP_LIST;

        dprintf("sbuf: %s (%d), dbuf: %s (%d), tbuf: %s (%d), op: %" PRIu64 "\n",
                memtype_str[sbuf_memtype], sbuf_devid, memtype_str[dbuf_memtype],
                dbuf_devid, memtype_str[tbuf_memtype], tbuf_devid, op);

        /* some ops are not compatible with overlapping segments */
        if (overlap != OVERLAP__NONE &&
            (op == YAKSA_OP__SUM || op == YAKSA_OP__PROD || op == YAKSA_OP__LXOR ||
             op == YAKSA_OP__BXOR))
            continue;

        /* create the source object */
        rc = DTP_obj_create(dtp[tid], &sobj, maxbufsize);
        assert(rc == DTP_SUCCESS);

        char *sbuf_h = NULL, *sbuf_d = NULL;
        pack_alloc_mem(sbuf_devid, sobj.DTP_bufsize, sbuf_memtype, (void **) &sbuf_h,
                       (void **) &sbuf_d);
        assert(sbuf_h);
        assert(sbuf_d);

        if (verbose) {
            char *desc;
            rc = DTP_obj_get_description(sobj, &desc);
            assert(rc == DTP_SUCCESS);
            dprintf("==> sbuf_h %p, sbuf_d %p, sobj (count: %zu):\n%s\n", sbuf_h, sbuf_d,
                    sobj.DTP_type_count, desc);
            free(desc);
        }

        uintptr_t ssize;
        rc = yaksa_type_get_size(sobj.DTP_datatype, &ssize);
        assert(rc == YAKSA_SUCCESS);


        /* create the destination object */
        rc = DTP_obj_create(dtp[tid], &dobj, maxbufsize);
        assert(rc == DTP_SUCCESS);

        char *dbuf_h, *dbuf_d;
        pack_alloc_mem(dbuf_devid, dobj.DTP_bufsize, dbuf_memtype, (void **) &dbuf_h,
                       (void **) &dbuf_d);
        assert(dbuf_h);
        assert(dbuf_d);

        if (verbose) {
            char *desc;
            rc = DTP_obj_get_description(dobj, &desc);
            assert(rc == DTP_SUCCESS);
            dprintf("==> dbuf_h %p, dbuf_d %p, dobj (count: %zu):\n%s\n", dbuf_h, dbuf_d,
                    dobj.DTP_type_count, desc);
            free(desc);
        }

        uintptr_t dsize;
        rc = yaksa_type_get_size(dobj.DTP_datatype, &dsize);
        assert(rc == YAKSA_SUCCESS);


        /* create the temporary buffers */
        void *tbuf_h, *tbuf_d;
        uintptr_t tbufsize = ssize * sobj.DTP_type_count;
        pack_alloc_mem(tbuf_devid, tbufsize, tbuf_memtype, &tbuf_h, &tbuf_d);
        assert(tbuf_h);
        assert(tbuf_d);


        /* initialize buffers */
        uintptr_t type_size;
        rc = yaksa_type_get_size(dtp[tid].DTP_base_type, &type_size);
        assert(rc == YAKSA_SUCCESS);

        switch (op) {
            case YAKSA_OP__SUM:
            case YAKSA_OP__REPLACE:
                rc = DTP_obj_buf_init(sobj, sbuf_h, 0, 1, basecount);
                assert(rc == DTP_SUCCESS);

                rc = DTP_obj_buf_init(dobj, dbuf_h, -1, -1, basecount);
                assert(rc == DTP_SUCCESS);

                break;

            case YAKSA_OP__PROD:
                rc = DTP_obj_buf_init(sobj, sbuf_h, 0, 1, basecount);
                assert(rc == DTP_SUCCESS);

                if (strstr(typestr, "complex") != NULL) {
                    rc = DTP_obj_buf_init(dobj, dbuf_h, 0, 0, basecount);
                    assert(rc == DTP_SUCCESS);
                } else {
                    rc = DTP_obj_buf_init(dobj, dbuf_h, -1, 0, basecount);
                    assert(rc == DTP_SUCCESS);
                }

                break;

            default:
                rc = DTP_obj_buf_init(sobj, sbuf_h, 1, 0, basecount);
                assert(rc == DTP_SUCCESS);

                rc = DTP_obj_buf_init(dobj, dbuf_h, -1, 0, basecount);
                assert(rc == DTP_SUCCESS);
        }

        /* pack the destination buffer into the temporary buffer, so
         * their contents are equivalent -- this is useful for
         * correctness in the accumulate operations */
        uintptr_t actual_pack_bytes;
        if (pack_kind == PACK_KIND__BLOCKING) {
            rc = yaksa_pack(dbuf_h + dobj.DTP_buf_offset, dobj.DTP_type_count, dobj.DTP_datatype,
                            0, tbuf_h, tbufsize, &actual_pack_bytes, NULL, YAKSA_OP__REPLACE);
            assert(rc == YAKSA_SUCCESS);
        } else if (pack_kind == PACK_KIND__STREAM) {
            rc = yaksa_pack_stream(dbuf_h + dobj.DTP_buf_offset, dobj.DTP_type_count,
                                   dobj.DTP_datatype, 0, tbuf_h, tbufsize, &actual_pack_bytes, NULL,
                                   YAKSA_OP__REPLACE, stream);
            assert(rc == YAKSA_SUCCESS);
            pack_stream_synchronize(stream);
        } else {
            rc = yaksa_ipack(dbuf_h + dobj.DTP_buf_offset, dobj.DTP_type_count, dobj.DTP_datatype,
                             0, tbuf_h, tbufsize, &actual_pack_bytes, NULL, YAKSA_OP__REPLACE,
                             &request);
            assert(rc == YAKSA_SUCCESS);

            rc = yaksa_request_wait(request);
            assert(rc == YAKSA_SUCCESS);
        }

        pack_copy_content(tid, sbuf_h, sbuf_d, sobj.DTP_bufsize, sbuf_memtype);
        pack_copy_content(tid, dbuf_h, dbuf_d, dobj.DTP_bufsize, dbuf_memtype);
        pack_copy_content(tid, tbuf_h, tbuf_d, tbufsize, tbuf_memtype);


        /* the source and destination objects should have the same
         * signature */
        assert(ssize * sobj.DTP_type_count == dsize * dobj.DTP_type_count);


        /* pack from the source object to a temporary buffer and
         * unpack into the destination object */

        /* figure out the lengths and offsets of each segment */
        int segments = max_segments;
        while (((ssize * sobj.DTP_type_count) / type_size) % segments)
            segments--;

        uintptr_t offset = 0;
        for (int j = 0; j < segments; j++) {
            segment_starts[j] = offset;

            uintptr_t eqlength = ssize * sobj.DTP_type_count / segments;

            /* make sure eqlength is a multiple of type_size */
            eqlength = (eqlength / type_size) * type_size;

            if (overlap == OVERLAP__NONE) {
                segment_lengths[j] = eqlength;
                offset += eqlength;
            } else if (overlap == OVERLAP__REGULAR) {
                if (offset + 2 * eqlength <= ssize * sobj.DTP_type_count)
                    segment_lengths[j] = 2 * eqlength;
                else
                    segment_lengths[j] = eqlength;
                offset += eqlength;
            } else {
                if (j == segments - 1) {
                    if (ssize * sobj.DTP_type_count > offset)
                        segment_lengths[j] = ssize * sobj.DTP_type_count - offset;
                    else
                        segment_lengths[j] = 0;
                    segment_lengths[j] += rand() % eqlength;
                } else {
                    segment_lengths[j] = rand() % (ssize * sobj.DTP_type_count - offset + eqlength);
                }

                offset += ((rand() % (segment_lengths[j] + 1)) / type_size) * type_size;
                if (offset > ssize * sobj.DTP_type_count)
                    offset = ssize * sobj.DTP_type_count;
            }
        }

        /* update the order in which we access the segments */
        if (pack_order == PACK_ORDER__NORMAL) {
            /* nothing to do */
        } else if (pack_order == PACK_ORDER__REVERSE) {
            for (int j = 0; j < segments / 2; j++) {
                swap_segments(segment_starts, segment_lengths, j, segments - j - 1);
            }
        } else if (pack_order == PACK_ORDER__RANDOM) {
            for (int j = 0; j < 1000; j++) {
                int x = rand() % segments;
                int y = rand() % segments;
                swap_segments(segment_starts, segment_lengths, x, y);
            }
        }

        /* the actual pack/unpack loop */
        yaksa_info_t pack_info, unpack_info;
        if (sbuf_memtype != MEM_TYPE__DEVICE && tbuf_memtype != MEM_TYPE__DEVICE) {
            host_only_get_ptr_attr(&pack_info, i);
        } else {
            pack_get_ptr_attr(sbuf_d + sobj.DTP_buf_offset, tbuf_d, &pack_info, i);
        }
        if (tbuf_memtype != MEM_TYPE__DEVICE && dbuf_memtype != MEM_TYPE__DEVICE) {
            host_only_get_ptr_attr(&unpack_info, i);
        } else {
            pack_get_ptr_attr(tbuf_d, dbuf_d + dobj.DTP_buf_offset, &unpack_info, i);
        }

        yaksa_op_t pack_op = (i % 2 == 0) ? YAKSA_OP__REPLACE : op;
        yaksa_op_t unpack_op = (i % 2) ? YAKSA_OP__REPLACE : op;
        for (int j = 0; j < segments; j++) {
            uintptr_t actual_pack_bytes;

            if (pack_kind == PACK_KIND__BLOCKING) {
                rc = yaksa_pack(sbuf_d + sobj.DTP_buf_offset, sobj.DTP_type_count,
                                sobj.DTP_datatype, segment_starts[j],
                                (char *) tbuf_d + segment_starts[j], segment_lengths[j],
                                &actual_pack_bytes, pack_info, pack_op);
                assert(rc == YAKSA_SUCCESS);
            } else if (pack_kind == PACK_KIND__STREAM) {
                rc = yaksa_pack_stream(sbuf_d + sobj.DTP_buf_offset, sobj.DTP_type_count,
                                       sobj.DTP_datatype, segment_starts[j],
                                       (char *) tbuf_d + segment_starts[j], segment_lengths[j],
                                       &actual_pack_bytes, pack_info, pack_op, stream);
                assert(rc == YAKSA_SUCCESS);
                pack_stream_synchronize(stream);
            } else {
                rc = yaksa_ipack(sbuf_d + sobj.DTP_buf_offset, sobj.DTP_type_count,
                                 sobj.DTP_datatype, segment_starts[j],
                                 (char *) tbuf_d + segment_starts[j], segment_lengths[j],
                                 &actual_pack_bytes, pack_info, pack_op, &request);
                assert(rc == YAKSA_SUCCESS);

                rc = yaksa_request_wait(request);
                assert(rc == YAKSA_SUCCESS);
            }

            assert(actual_pack_bytes <= segment_lengths[j]);
            if (j == segments - 1) {
                DTP_obj_free(sobj);
            }

            uintptr_t actual_unpack_bytes;
            if (pack_kind == PACK_KIND__BLOCKING) {
                rc = yaksa_unpack((char *) tbuf_d + segment_starts[j], actual_pack_bytes,
                                  dbuf_d + dobj.DTP_buf_offset, dobj.DTP_type_count,
                                  dobj.DTP_datatype, segment_starts[j], &actual_unpack_bytes,
                                  unpack_info, unpack_op);
                assert(rc == YAKSA_SUCCESS);
            } else if (pack_kind == PACK_KIND__STREAM) {
                rc = yaksa_unpack_stream((char *) tbuf_d + segment_starts[j], actual_pack_bytes,
                                         dbuf_d + dobj.DTP_buf_offset, dobj.DTP_type_count,
                                         dobj.DTP_datatype, segment_starts[j], &actual_unpack_bytes,
                                         unpack_info, unpack_op, stream);
                assert(rc == YAKSA_SUCCESS);
                pack_stream_synchronize(stream);
            } else {
                rc = yaksa_iunpack((char *) tbuf_d + segment_starts[j], actual_pack_bytes,
                                   dbuf_d + dobj.DTP_buf_offset, dobj.DTP_type_count,
                                   dobj.DTP_datatype, segment_starts[j], &actual_unpack_bytes,
                                   unpack_info, unpack_op, &request);
                assert(rc == YAKSA_SUCCESS);

                rc = yaksa_request_wait(request);
                assert(rc == YAKSA_SUCCESS);
            }

            assert(actual_pack_bytes == actual_unpack_bytes);
        }

        if (pack_info) {
            rc = yaksa_info_free(pack_info);
            assert(rc == YAKSA_SUCCESS);
        }

        if (unpack_info) {
            rc = yaksa_info_free(unpack_info);
            assert(rc == YAKSA_SUCCESS);
        }

        pack_copy_content(tid, dbuf_d, dbuf_h, dobj.DTP_bufsize, dbuf_memtype);

        switch (op) {
            case YAKSA_OP__REPLACE:
                rc = DTP_obj_buf_check(dobj, dbuf_h, 0, 1, basecount);
                assert(rc == DTP_SUCCESS);

                break;

            case YAKSA_OP__MIN:
                rc = DTP_obj_buf_check(dobj, dbuf_h, -1, 0, basecount);
                assert(rc == DTP_SUCCESS);

                break;

            case YAKSA_OP__SUM:
                rc = DTP_obj_buf_check(dobj, dbuf_h, -1, 0, basecount);
                assert(rc == DTP_SUCCESS);

                break;

            case YAKSA_OP__PROD:
                if (strstr(typestr, "complex") != NULL) {
                    rc = DTP_obj_buf_check(dobj, dbuf_h, 0, 0, basecount);
                } else {
                    rc = DTP_obj_buf_check(dobj, dbuf_h, 0, -1, basecount);
                }
                assert(rc == DTP_SUCCESS);

                break;

            case YAKSA_OP__MAX:
            case YAKSA_OP__LAND:
            case YAKSA_OP__BAND:
            case YAKSA_OP__LOR:
                rc = DTP_obj_buf_check(dobj, dbuf_h, 1, 0, basecount);
                assert(rc == DTP_SUCCESS);

                break;

            case YAKSA_OP__BOR:
                rc = DTP_obj_buf_check(dobj, dbuf_h, -1, 0, basecount);
                assert(rc == DTP_SUCCESS);

                break;

            case YAKSA_OP__LXOR:
                rc = DTP_obj_buf_check(dobj, dbuf_h, 0, 0, basecount);
                assert(rc == DTP_SUCCESS);

                break;

            case YAKSA_OP__BXOR:
                rc = DTP_obj_buf_check(dobj, dbuf_h, -2, 0, basecount);
                assert(rc == DTP_SUCCESS);

                break;

            default:
                assert(0);
        }


        /* free allocated buffers and objects */
        pack_free_mem(sbuf_memtype, sbuf_h, sbuf_d);
        pack_free_mem(dbuf_memtype, dbuf_h, dbuf_d);
        pack_free_mem(tbuf_memtype, tbuf_h, tbuf_d);

        DTP_obj_free(dobj);
    }

    free(segment_lengths);
    free(segment_starts);

    return NULL;
}

int main(int argc, char **argv)
{
    int num_threads = 1;

    while (--argc && ++argv) {
        if (!strcmp(*argv, "-datatype")) {
            --argc;
            ++argv;
            strncpy(typestr, *argv, MAX_DTP_BASESTRLEN);
        } else if (!strcmp(*argv, "-count")) {
            --argc;
            ++argv;
            basecount = atoi(*argv);
        } else if (!strcmp(*argv, "-seed")) {
            --argc;
            ++argv;
            seed = atoi(*argv);
        } else if (!strcmp(*argv, "-iters")) {
            --argc;
            ++argv;
            iters = atoi(*argv);
        } else if (!strcmp(*argv, "-segments")) {
            --argc;
            ++argv;
            max_segments = atoi(*argv);
        } else if (!strcmp(*argv, "-ordering")) {
            --argc;
            ++argv;
            if (!strcmp(*argv, "normal"))
                pack_order = PACK_ORDER__NORMAL;
            else if (!strcmp(*argv, "reverse"))
                pack_order = PACK_ORDER__REVERSE;
            else if (!strcmp(*argv, "random"))
                pack_order = PACK_ORDER__RANDOM;
            else {
                fprintf(stderr, "unknown packing order %s\n", *argv);
                exit(1);
            }
        } else if (!strcmp(*argv, "-oplist")) {
            --argc;
            ++argv;
            if (!strcmp(*argv, "int")) {
                op_list = int_ops;
                op_list_len = sizeof(int_ops) / sizeof(yaksa_op_t);
            } else if (!strcmp(*argv, "float")) {
                op_list = float_ops;
                op_list_len = sizeof(float_ops) / sizeof(yaksa_op_t);
            } else if (!strcmp(*argv, "complex")) {
                op_list = complex_ops;
                op_list_len = sizeof(complex_ops) / sizeof(yaksa_op_t);
            } else if (!strcmp(*argv, "replace")) {
                single_op = YAKSA_OP__REPLACE;
                op_list = &single_op;
                op_list_len = 1;
            } else if (!strcmp(*argv, "sum")) {
                single_op = YAKSA_OP__SUM;
                op_list = &single_op;
                op_list_len = 1;
            } else if (!strcmp(*argv, "prod")) {
                single_op = YAKSA_OP__PROD;
                op_list = &single_op;
                op_list_len = 1;
            } else if (!strcmp(*argv, "max")) {
                single_op = YAKSA_OP__MAX;
                op_list = &single_op;
                op_list_len = 1;
            } else if (!strcmp(*argv, "min")) {
                single_op = YAKSA_OP__MIN;
                op_list = &single_op;
                op_list_len = 1;
            } else if (!strcmp(*argv, "land")) {
                single_op = YAKSA_OP__LAND;
                op_list = &single_op;
                op_list_len = 1;
            } else if (!strcmp(*argv, "lor")) {
                single_op = YAKSA_OP__LOR;
                op_list = &single_op;
                op_list_len = 1;
            } else if (!strcmp(*argv, "lxor")) {
                single_op = YAKSA_OP__LXOR;
                op_list = &single_op;
                op_list_len = 1;
            } else if (!strcmp(*argv, "band")) {
                single_op = YAKSA_OP__BAND;
                op_list = &single_op;
                op_list_len = 1;
            } else if (!strcmp(*argv, "bor")) {
                single_op = YAKSA_OP__BOR;
                op_list = &single_op;
                op_list_len = 1;
            } else if (!strcmp(*argv, "bxor")) {
                single_op = YAKSA_OP__BXOR;
                op_list = &single_op;
                op_list_len = 1;
            } else {
                fprintf(stderr, "unknown oplist type %s\n", *argv);
                exit(1);
            }
        } else if (!strcmp(*argv, "-overlap")) {
            --argc;
            ++argv;
            if (!strcmp(*argv, "none"))
                overlap = OVERLAP__NONE;
            else if (!strcmp(*argv, "regular"))
                overlap = OVERLAP__REGULAR;
            else if (!strcmp(*argv, "irregular"))
                overlap = OVERLAP__IRREGULAR;
            else {
                fprintf(stderr, "unknown overlap type %s\n", *argv);
                exit(1);
            }
        } else if (!strcmp(*argv, "-blocking")) {
            pack_kind = PACK_KIND__BLOCKING;
        } else if (!strcmp(*argv, "-stream")) {
            pack_kind = PACK_KIND__STREAM;
        } else if (!strcmp(*argv, "-verbose")) {
            verbose = 1;
        } else if (!strcmp(*argv, "-use-tiles")) {
            use_subdevices = 1;
        } else if (!strcmp(*argv, "-num-threads")) {
            --argc;
            ++argv;
            num_threads = atoi(*argv);
        } else {
            fprintf(stderr, "unknown argument %s\n", *argv);
            exit(1);
        }
    }
    if (strlen(typestr) == 0 || basecount <= 0 || seed < 0 || iters <= 0 || max_segments < 0 ||
        pack_order == PACK_ORDER__UNSET || overlap < 0 || num_threads <= 0) {
        fprintf(stderr, "Usage: ./pack {options}\n");
        fprintf(stderr, "   -datatype    base datatype to use, e.g., int\n");
        fprintf(stderr, "   -count       number of base datatypes in the signature\n");
        fprintf(stderr, "   -seed        random seed (changes the datatypes generated)\n");
        fprintf(stderr, "   -iters       number of iterations\n");
        fprintf(stderr, "   -segments    number of segments to chop the packing into\n");
        fprintf(stderr, "   -ordering  packing order of segments (normal, reverse, random)\n");
        fprintf(stderr, "   -overlap     should packing overlap (none, regular, irregular)\n");
        fprintf(stderr, "   -blocking    test blocking pack/unpack \n");
        fprintf(stderr, "   -stream      test pack_stream/unpack_stream \n");
        fprintf(stderr, "   -verbose     verbose output\n");
        fprintf(stderr, "   -num-threads number of threads to spawn\n");
        fprintf(stderr, "   -oplist      oplist type (int, float, complex)\n");
        fprintf(stderr, "   -use-tiles   allocate buffers on tiles (ZE only)\n");
        exit(1);
    }

    yaksa_init(NULL);
    pack_init_devices(num_threads);

    dtp = (DTP_pool_s *) malloc(num_threads * sizeof(DTP_pool_s));
    for (uintptr_t i = 0; i < num_threads; i++) {
        int rc = DTP_pool_create(typestr, basecount, seed + i, &dtp[i]);
        assert(rc == DTP_SUCCESS);
    }

    int ndevices = pack_get_ndevices();
    device_ids = (int **) malloc(num_threads * sizeof(int *));
    memtypes = (mem_type_e **) malloc(num_threads * sizeof(mem_type_e *));
    ops = (yaksa_op_t **) malloc(num_threads * sizeof(yaksa_op_t *));

    srand(seed + num_threads);
    for (uintptr_t i = 0; i < num_threads; i++) {
        device_ids[i] = (int *) malloc(MAX_DEVID_LIST * sizeof(int));
        memtypes[i] = (mem_type_e *) malloc(MAX_MEMTYPE_LIST * sizeof(mem_type_e));
        ops[i] = (yaksa_op_t *) malloc(MAX_OP_LIST * sizeof(yaksa_op_t));

        for (int j = 0; j < MAX_DEVID_LIST; j++) {
            if (ndevices > 0) {
                device_ids[i][j] = rand() % ndevices;
            } else {
                device_ids[i][j] = -1;
            }
        }
        for (int j = 0; j < MAX_MEMTYPE_LIST; j++) {
            if (ndevices > 0) {
                memtypes[i][j] = rand() % MEM_TYPE__NUM_MEMTYPES;
            } else {
                memtypes[i][j] = MEM_TYPE__UNREGISTERED_HOST;
            }
        }

        for (int j = 0; j < MAX_OP_LIST; j++) {
            ops[i][j] = op_list[rand() % op_list_len];
        }
    }

    pthread_t *threads = (pthread_t *) malloc(num_threads * sizeof(pthread_t));

    if (pack_kind == PACK_KIND__STREAM) {
        stream = pack_create_stream();
        assert(stream != NULL);
        /* The stream was created on device 0, force to use a single device */
        for (uintptr_t i = 0; i < num_threads; i++) {
            for (int j = 0; j < MAX_DEVID_LIST; j++) {
                device_ids[i][j] = 0;
            }
        }
    }

    for (uintptr_t i = 0; i < num_threads; i++)
        pthread_create(&threads[i], NULL, runtest, (void *) i);

    for (uintptr_t i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);

    free(threads);

    if (pack_kind == PACK_KIND__STREAM) {
        pack_destroy_stream(stream);
    }

    for (uintptr_t i = 0; i < num_threads; i++) {
        int rc = DTP_pool_free(dtp[i]);
        assert(rc == DTP_SUCCESS);
    }
    free(dtp);

    pack_finalize_devices();
    yaksa_finalize();

    return 0;
}
