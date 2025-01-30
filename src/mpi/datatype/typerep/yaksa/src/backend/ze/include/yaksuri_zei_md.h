/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSURI_ZEI_MD_H_INCLUDED
#define YAKSURI_ZEI_MD_H_INCLUDED

typedef struct yaksuri_zei_md_s {
    union {
        struct {
            long count;
            long stride;
            struct yaksuri_zei_md_s *child;
        } contig;
        struct {
            struct yaksuri_zei_md_s *child;
        } dup;
        struct {
            struct yaksuri_zei_md_s *child;
        } resized;
        struct {
            long count;
            long blocklength;
            long stride;
            struct yaksuri_zei_md_s *child;
        } hvector;
        struct {
            long count;
            long blocklength;
            long *array_of_displs;
            struct yaksuri_zei_md_s *child;
        } blkhindx;
        struct {
            long count;
            long *array_of_blocklengths;
            long *array_of_displs;
            struct yaksuri_zei_md_s *child;
        } hindexed;
    } u;

    unsigned long extent;
    unsigned long num_elements;
    unsigned long true_lb;
} yaksuri_zei_md_s;

#endif /* YAKSURI_ZEI_H_INCLUDED */
