/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSURI_HIPI_BASE_H_INCLUDED
#define YAKSURI_HIPI_BASE_H_INCLUDED

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <hip/hip_runtime_api.h>

#define YAKSURI_HIPI_HIP_ERR_CHECK(cerr)                              \
    do {                                                                \
        if (cerr != hipSuccess) {                                      \
            fprintf(stderr, "HIP Error (%s:%s,%d): %s\n", __func__, __FILE__, __LINE__, hipGetErrorString(cerr)); \
        }                                                               \
    } while (0)

typedef struct {
    int ndevices;
    hipStream_t *stream;
    bool **p2p;
} yaksuri_hipi_global_s;
extern yaksuri_hipi_global_s yaksuri_hipi_global;

typedef struct yaksuri_hipi_md_s {
    union {
        struct {
            intptr_t count;
            intptr_t stride;
            struct yaksuri_hipi_md_s *child;
        } contig;
        struct {
            struct yaksuri_hipi_md_s *child;
        } resized;
        struct {
            intptr_t count;
            intptr_t blocklength;
            intptr_t stride;
            struct yaksuri_hipi_md_s *child;
        } hvector;
        struct {
            intptr_t count;
            intptr_t blocklength;
            intptr_t *array_of_displs;
            struct yaksuri_hipi_md_s *child;
        } blkhindx;
        struct {
            intptr_t count;
            intptr_t *array_of_blocklengths;
            intptr_t *array_of_displs;
            struct yaksuri_hipi_md_s *child;
        } hindexed;
    } u;

    uintptr_t extent;
    uintptr_t num_elements;
} yaksuri_hipi_md_s;

#endif /* YAKSURI_HIPI_BASE_H_INCLUDED */
