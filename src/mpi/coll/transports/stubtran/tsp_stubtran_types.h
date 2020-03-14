/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TSP_STUBTRAN_TYPES_H_INCLUDED
#define TSP_STUBTRAN_TYPES_H_INCLUDED

typedef struct MPII_Stubutil_sched_t {
    /* empty structures are invalid on some systems/compilers */
    int dummy;
} MPII_Stubutil_sched_t;

typedef int (*MPII_Stubutil_sched_issue_fn) (void *vtxp, int *done);
typedef int (*MPII_Stubutil_sched_complete_fn) (void *vtxp, int *is_completed);
typedef int (*MPII_Stubutil_sched_free_fn) (void *vtxp);

#endif /* TSP_STUBTRAN_TYPES_H_INCLUDED */
