/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_PSET_H_INCLUDED
#define MPIR_PSET_H_INCLUDED

#include "mpiimpl.h"

struct MPIR_Pset {
    char *uri;                  /* Name of the process set */
    int size;                   /* Number of processes in the set */
    bool is_valid;              /* True if process set is valid, false if not */
    MPIR_Pset_member *members;  /* Array of members of the process set sorted in ascending order
                                 * TODO: This should be optimized using compressed methods of process
                                 * representation to improve scalability */
};

int MPIR_Pset_init(void);
int MPIR_Pset_finalize(void);
int MPIR_Pset_count(void);
int MPIR_Pset_by_name(const char *pset_name, MPIR_Pset ** pset);
int MPIR_Pset_by_idx(int idx, MPIR_Pset ** pset);
int MPIR_Pset_add(MPIR_Pset * pset);
int MPIR_Pset_invalidate(char *pset_name);
int MPIR_Pset_member_compare(void const *a, void const *b);
int MPIR_Pset_member_is_self(MPIR_Pset_member * member);

#endif /* MPIR_PSET_H_INCLUDED */
