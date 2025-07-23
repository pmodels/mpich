/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_PSET_H_INCLUDED
#define MPIR_PSET_H_INCLUDED

#include "mpiimpl.h"

typedef struct MPIR_Pset {
    char *name;
    struct MPIR_Group *group;
} MPIR_Pset;

int MPIR_Session_add_default_psets(MPIR_Session * session_ptr);
int MPIR_Session_update_psets(MPIR_Session * session_ptr);

int MPIR_Pset_init(void);
int MPIR_Pset_finalize(void);
bool MPIR_Pset_add(MPIR_Pset * pset);


#endif /* MPIR_PSET_H_INCLUDED */
