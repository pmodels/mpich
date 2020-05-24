/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TYPESIZE_SUPPORT_H_INCLUDED
#define TYPESIZE_SUPPORT_H_INCLUDED

#include "dataloop_internal.h"

#define MPII_Dataloop_type_footprint MPIDU_Type_footprint

typedef struct MPIDU_Type_footprint_s {
    MPI_Aint size, extent;

    /* these are only needed for calculating footprint of types
     * built using this type. no reason to expose these.
     */
    MPI_Aint lb, ub, alignsz;
    MPI_Aint true_lb, true_ub;
} MPII_Dataloop_type_footprint;

void MPIDU_Type_calc_footprint(MPI_Datatype type, MPII_Dataloop_type_footprint * tfp);

#endif /* TYPESIZE_SUPPORT_H_INCLUDED */
