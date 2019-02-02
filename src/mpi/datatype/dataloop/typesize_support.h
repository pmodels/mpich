/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef TYPESIZE_SUPPORT_H_INCLUDED
#define TYPESIZE_SUPPORT_H_INCLUDED

#include "dataloop.h"

#define MPII_Dataloop_type_footprint MPIDU_Type_footprint

typedef struct MPIDU_Type_footprint_s {
    MPI_Aint size, extent;

    /* these are only needed for calculating footprint of types
     * built using this type. no reason to expose these.
     */
    MPI_Aint lb, ub, alignsz;
    MPI_Aint true_lb, true_ub;
    int has_sticky_lb;
    int has_sticky_ub;
} MPII_Dataloop_type_footprint;

void MPIDU_Type_calc_footprint(MPI_Datatype type, MPII_Dataloop_type_footprint * tfp);

#endif /* TYPESIZE_SUPPORT_H_INCLUDED */
