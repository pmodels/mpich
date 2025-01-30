/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include "yaksa.h"
#include "dtpools.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>

uintptr_t maxbufsize = 512 * 1024 * 1024;

enum {
    IOV_ORDER__UNSET,
    IOV_ORDER__NORMAL,
    IOV_ORDER__REVERSE,
    IOV_ORDER__RANDOM,
};

enum {
    OVERLAP__NONE,
    OVERLAP__REGULAR,
    OVERLAP__IRREGULAR,
};

#define MAX_DTP_BASESTRLEN (1024)

#define MIN(x, y)  (x < y ? x : y)

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

DTP_pool_s *dtp;
int basecount = -1;
int seed = -1;
int iters = -1;
int max_segments = -1;
int iov_order = IOV_ORDER__UNSET;
int overlap = -1;

void *runtest(void *arg)
{
    uintptr_t tid = (uintptr_t) arg;
    uintptr_t *segment_starts = (uintptr_t *) malloc(max_segments * sizeof(uintptr_t));
    uintptr_t *segment_lengths = (uintptr_t *) malloc(max_segments * sizeof(uintptr_t));
    DTP_obj_s sobj, dobj;
    int rc;

    for (int i = 0; i < iters; i++) {
        dprintf("==== iter %d ====\n", i);

        /* create the source object */
        rc = DTP_obj_create(dtp[tid], &sobj, maxbufsize);
        assert(rc == DTP_SUCCESS);

        char *sbuf = (char *) malloc(sobj.DTP_bufsize);
        assert(sbuf);

        if (verbose) {
            char *desc;
            rc = DTP_obj_get_description(sobj, &desc);
            assert(rc == DTP_SUCCESS);
            dprintf("==> sbuf %p (with offset) %p, sobj (count: %zu):\n%s\n",
                    sbuf, sbuf + sobj.DTP_buf_offset, sobj.DTP_type_count, desc);
            free(desc);
        }

        rc = DTP_obj_buf_init(sobj, sbuf, 0, 1, basecount);
        assert(rc == DTP_SUCCESS);

        uintptr_t ssize;
        rc = yaksa_type_get_size(sobj.DTP_datatype, &ssize);
        assert(rc == YAKSA_SUCCESS);


        /* create the destination object */
        rc = DTP_obj_create(dtp[tid], &dobj, maxbufsize);
        assert(rc == DTP_SUCCESS);

        char *dbuf = (char *) malloc(dobj.DTP_bufsize);
        assert(dbuf);

        if (verbose) {
            char *desc;
            rc = DTP_obj_get_description(dobj, &desc);
            assert(rc == DTP_SUCCESS);
            dprintf("==> dbuf %p (with offset) %p, dobj (count: %zu):\n%s\n",
                    dbuf, dbuf + dobj.DTP_buf_offset, dobj.DTP_type_count, desc);
            free(desc);
        }

        rc = DTP_obj_buf_init(dobj, dbuf, -1, -1, basecount);
        assert(rc == DTP_SUCCESS);

        uintptr_t dsize;
        rc = yaksa_type_get_size(dobj.DTP_datatype, &dsize);
        assert(rc == YAKSA_SUCCESS);


        /* the source and destination objects should have the same
         * signature */
        assert(ssize * sobj.DTP_type_count == dsize * dobj.DTP_type_count);


        /* figure out the iov lengths of each segment */
        uintptr_t sobj_iov_len;
        rc = yaksa_iov_len(sobj.DTP_type_count, sobj.DTP_datatype, &sobj_iov_len);
        assert(rc == YAKSA_SUCCESS);

        uintptr_t dobj_iov_len;
        rc = yaksa_iov_len(dobj.DTP_type_count, dobj.DTP_datatype, &dobj_iov_len);
        assert(rc == YAKSA_SUCCESS);


        uintptr_t soffset = 0, doffset = 0;
        int j = 0, k = 0;

        uintptr_t actual_iov_len;
        struct iovec *dobj_iov = (struct iovec *) malloc(dobj_iov_len * sizeof(struct iovec));
        rc = yaksa_iov(dbuf + dobj.DTP_buf_offset, dobj.DTP_type_count, dobj.DTP_datatype,
                       0, dobj_iov, dobj_iov_len, &actual_iov_len);
        assert(rc == YAKSA_SUCCESS);
        assert(actual_iov_len == dobj_iov_len);

        int segments = max_segments;
        while (sobj_iov_len % segments)
            segments--;

        /* figure out the lengths and offsets of each segment */
        uintptr_t offset = 0;
        for (int m = 0; m < segments; m++) {
            segment_starts[m] = offset;

            uintptr_t eqlength = sobj_iov_len / segments;

            if (overlap == OVERLAP__NONE) {
                segment_lengths[m] = eqlength;
                offset += eqlength;
            } else if (overlap == OVERLAP__REGULAR) {
                segment_lengths[m] = 2 * eqlength;
                offset += eqlength;
            } else {
                if (m == segments - 1) {
                    if (sobj_iov_len > offset)
                        segment_lengths[m] = sobj_iov_len - offset;
                    else
                        segment_lengths[m] = 0;
                    segment_lengths[m] += rand() % eqlength;
                } else {
                    segment_lengths[m] = rand() % (sobj_iov_len - offset + eqlength);
                }

                offset += rand() % (segment_lengths[m] + 1);
                if (offset > sobj_iov_len)
                    offset = sobj_iov_len;
            }
        }

        /* update the order in which we access the segments */
        if (iov_order == IOV_ORDER__NORMAL) {
            /* nothing to do */
        } else if (iov_order == IOV_ORDER__REVERSE) {
            for (int m = 0; m < segments / 2; m++) {
                swap_segments(segment_starts, segment_lengths, m, segments - m - 1);
            }
        } else if (iov_order == IOV_ORDER__RANDOM) {
            for (int m = 0; m < 1000; m++) {
                int x = rand() % segments;
                int y = rand() % segments;
                swap_segments(segment_starts, segment_lengths, x, y);
            }
        }

        /* update the segment starts and lengths, so a basic type is
         * not split across multiple segments */
        uintptr_t basic_iov_len;
        rc = yaksa_iov_len(1, dtp[tid].DTP_base_type, &basic_iov_len);
        assert(rc == YAKSA_SUCCESS);

        for (int m = 0; m < segments; m++) {
            while (segment_starts[m] % basic_iov_len) {
                segment_starts[m]--;
                segment_lengths[m]++;
            }
            while (segment_lengths[m] % basic_iov_len) {
                segment_lengths[m]++;
            }
        }

        /* create the sobj iov and fill it with the segments */
        struct iovec *sobj_iov = (struct iovec *) malloc(sobj_iov_len * sizeof(struct iovec));
        struct iovec *sobj_tmp_iov = (struct iovec *) malloc(sobj_iov_len * sizeof(struct iovec));

        for (uintptr_t m = 0; m < sobj_iov_len; m++) {
            sobj_iov[m].iov_base = NULL;
            sobj_iov[m].iov_len = 0;
        }

        for (int m = 0; m < segments; m++) {
            if (segment_starts[m] >= sobj_iov_len)
                continue;

            rc = yaksa_iov(sbuf + sobj.DTP_buf_offset, sobj.DTP_type_count, sobj.DTP_datatype,
                           segment_starts[m], sobj_tmp_iov, segment_lengths[m], &actual_iov_len);
            assert(rc == YAKSA_SUCCESS);

            for (int n = segment_starts[m]; n < segment_starts[m] + actual_iov_len; n++) {
                sobj_iov[n] = sobj_tmp_iov[n - segment_starts[m]];
            }
        }

        for (uintptr_t m = 0; m < sobj_iov_len; m++) {
            assert(sobj_iov[m].iov_base);
        }

        /* the actual iov loop */
        for (j = 0, k = 0; j < sobj_iov_len && k < dobj_iov_len;) {
            char *s = (char *) sobj_iov[j].iov_base + soffset;
            char *d = (char *) dobj_iov[k].iov_base + doffset;
            uintptr_t copy_size = MIN(sobj_iov[j].iov_len - soffset, dobj_iov[k].iov_len - doffset);

            memcpy(d, s, copy_size);

            soffset += copy_size;
            if (soffset == sobj_iov[j].iov_len) {
                j++;
                soffset = 0;
            }

            doffset += copy_size;
            if (doffset == dobj_iov[k].iov_len) {
                k++;
                doffset = 0;
            }
        }

        rc = DTP_obj_buf_check(dobj, dbuf, 0, 1, basecount);
        assert(rc == DTP_SUCCESS);


        /* free allocated buffers and objects */
        free(sbuf);
        free(dbuf);
        free(sobj_tmp_iov);
        free(sobj_iov);
        free(dobj_iov);
        DTP_obj_free(sobj);
        DTP_obj_free(dobj);
    }

    free(segment_lengths);
    free(segment_starts);

    return NULL;
}

int main(int argc, char **argv)
{
    char typestr[MAX_DTP_BASESTRLEN + 1];
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
                iov_order = IOV_ORDER__NORMAL;
            else if (!strcmp(*argv, "reverse"))
                iov_order = IOV_ORDER__REVERSE;
            else if (!strcmp(*argv, "random"))
                iov_order = IOV_ORDER__RANDOM;
            else {
                fprintf(stderr, "unknown iov order %s\n", *argv);
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
        } else if (!strcmp(*argv, "-verbose")) {
            verbose = 1;
        } else if (!strcmp(*argv, "-num-threads")) {
            --argc;
            ++argv;
            num_threads = atoi(*argv);
        } else {
            fprintf(stderr, "unknown argument %s\n", *argv);
            exit(1);
        }
    }
    if (typestr == NULL || basecount <= 0 || seed < 0 || iters <= 0 || max_segments < 0 ||
        iov_order == IOV_ORDER__UNSET || overlap < 0 || num_threads <= 0) {
        fprintf(stderr, "Usage: ./iov {options}\n");
        fprintf(stderr, "   -datatype    base datatype to use, e.g., int\n");
        fprintf(stderr, "   -count       number of base datatypes in the signature\n");
        fprintf(stderr, "   -seed        random seed (changes the datatypes generated)\n");
        fprintf(stderr, "   -iters       number of iterations\n");
        fprintf(stderr, "   -segments    number of segments to chop the iov into\n");
        fprintf(stderr, "   -ordering   iov order of segments (normal, reverse, random)\n");
        fprintf(stderr, "   -overlap     should iovs overlap (none, regular, irregular)\n");
        fprintf(stderr, "   -verbose     verbose output\n");
        fprintf(stderr, "   -num-threads number of threads to spawn\n");
        exit(1);
    }

    yaksa_init(NULL);

    dtp = (DTP_pool_s *) malloc(num_threads * sizeof(DTP_pool_s));
    for (uintptr_t i = 0; i < num_threads; i++) {
        int rc = DTP_pool_create(typestr, basecount, seed + i, &dtp[i]);
        assert(rc == DTP_SUCCESS);
    }

    pthread_t *threads = (pthread_t *) malloc(num_threads * sizeof(pthread_t));

    for (uintptr_t i = 0; i < num_threads; i++)
        pthread_create(&threads[i], NULL, runtest, (void *) i);

    for (uintptr_t i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);

    free(threads);

    for (uintptr_t i = 0; i < num_threads; i++) {
        int rc = DTP_pool_free(dtp[i]);
        assert(rc == DTP_SUCCESS);
    }
    free(dtp);

    yaksa_finalize();

    return 0;
}
