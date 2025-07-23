/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_PSET_H_INCLUDED
#define MPIR_PSET_H_INCLUDED

#include "mpiimpl.h"

#define MPIR_PSET_WORLD_NAME "mpi://WORLD"
#define MPIR_PSET_SELF_NAME "mpi://SELF"

typedef struct MPIR_Pset {
    char *name;
    struct MPIR_Group *group;
} MPIR_Pset;

int MPIR_Session_add_default_psets(MPIR_Session * session_ptr);

#endif /* MPIR_PSET_H_INCLUDED */
