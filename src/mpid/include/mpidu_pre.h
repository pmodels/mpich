/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_PRE_H_INCLUDED
#define MPIDU_PRE_H_INCLUDED



/* some common MPI forward declarations */

struct MPIR_Request;
struct MPIR_Comm;

/* Scheduling forward declarations */

struct MPIDU_Sched;
typedef struct MPIDU_Sched *MPIR_Sched_t;

typedef int (MPIR_Sched_cb_t) (struct MPIR_Comm * comm, int tag, void *state);
typedef int (MPIR_Sched_cb2_t) (struct MPIR_Comm * comm, int tag, void *state, void *state2);

#endif /* MPIDU_PRE_H_INCLUDED */
