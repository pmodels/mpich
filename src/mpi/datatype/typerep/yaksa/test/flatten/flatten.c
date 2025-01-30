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

#define MAX_DTP_BASESTRLEN (1024)

static int verbose = 0;

#define dprintf(...)                            \
    do {                                        \
        if (verbose)                            \
            printf(__VA_ARGS__);                \
    } while (0)

char typestr[MAX_DTP_BASESTRLEN + 1];
int basecount = -1;
int seed = -1;
int iters = -1;
DTP_pool_s *dtp;

void *runtest(void *arg);
void *runtest(void *arg)
{
    DTP_obj_s sobj;
    int rc;
    uintptr_t tid = (uintptr_t) arg;

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
            dprintf("==> sbuf %p, sobj (count: %zu):\n%s\n", sbuf, sobj.DTP_type_count, desc);
            free(desc);
        }

        rc = DTP_obj_buf_init(sobj, sbuf, 0, 1, basecount);
        assert(rc == DTP_SUCCESS);

        uintptr_t ssize;
        rc = yaksa_type_get_size(sobj.DTP_datatype, &ssize);
        assert(rc == YAKSA_SUCCESS);


        /* create a new type by flattening and unflattening */
        uintptr_t flatten_size;
        rc = yaksa_flatten_size(sobj.DTP_datatype, &flatten_size);
        assert(rc == YAKSA_SUCCESS);

        void *flatbuf = malloc(flatten_size);
        assert(flatbuf);

        rc = yaksa_flatten(sobj.DTP_datatype, flatbuf);
        assert(rc == YAKSA_SUCCESS);

        yaksa_type_t newtype;
        rc = yaksa_unflatten(&newtype, flatbuf);
        assert(rc == YAKSA_SUCCESS);


        /* pack with the original type and unpack with the new type */
        void *tbuf = malloc(ssize * sobj.DTP_type_count);
        assert(tbuf);

        uintptr_t actual_pack_bytes;
        yaksa_request_t request;
        rc = yaksa_ipack(sbuf + sobj.DTP_buf_offset, sobj.DTP_type_count, sobj.DTP_datatype,
                         0, tbuf, ssize * sobj.DTP_type_count, &actual_pack_bytes, NULL,
                         YAKSA_OP__REPLACE, &request);
        assert(rc == YAKSA_SUCCESS);
        assert(actual_pack_bytes == ssize * sobj.DTP_type_count);

        if (request != YAKSA_REQUEST__NULL) {
            rc = yaksa_request_wait(request);
            assert(rc == YAKSA_SUCCESS);
        }

        /* reinitialize sbuf with random data */
        rc = DTP_obj_buf_init(sobj, sbuf, -1, -1, basecount);
        assert(rc == DTP_SUCCESS);

        uintptr_t actual_unpack_bytes;
        rc = yaksa_iunpack(tbuf, actual_pack_bytes, sbuf + sobj.DTP_buf_offset,
                           sobj.DTP_type_count, newtype, 0, &actual_unpack_bytes, NULL,
                           YAKSA_OP__REPLACE, &request);
        assert(rc == YAKSA_SUCCESS);

        if (request != YAKSA_REQUEST__NULL) {
            rc = yaksa_request_wait(request);
            assert(rc == YAKSA_SUCCESS);
        }

        rc = yaksa_type_free(newtype);
        assert(rc == YAKSA_SUCCESS);


        /* check if the content is correct */
        rc = DTP_obj_buf_check(sobj, sbuf, 0, 1, basecount);
        assert(rc == DTP_SUCCESS);


        /* free allocated buffers and objects */
        free(tbuf);
        free(flatbuf);
        free(sbuf);
        DTP_obj_free(sobj);
    }

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
    if (typestr == NULL || basecount <= 0 || seed < 0 || iters <= 0 || num_threads <= 0) {
        fprintf(stderr, "Usage: ./flatten {options}\n");
        fprintf(stderr, "   -datatype    base datatype to use, e.g., int\n");
        fprintf(stderr, "   -count       number of base datatypes in the signature\n");
        fprintf(stderr, "   -seed        random seed (changes the datatypes generated)\n");
        fprintf(stderr, "   -iters       number of iterations\n");
        fprintf(stderr, "   -verbose     verbose output\n");
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
