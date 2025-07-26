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
    int is_valid;
} MPIR_Pset;

int MPIR_Pset_init(void);
int MPIR_Pset_finalize(void);
bool MPIR_Pset_add(MPIR_Pset * pset);
void MPIR_Pset_invalidate(char *pset_name);
int MPIR_Pset_add_defaults(MPIR_Session * session_ptr);
int MPIR_Pset_update_list(MPIR_Session * session_ptr);

#endif /* MPIR_PSET_H_INCLUDED */
