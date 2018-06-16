/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef TSP_GENTRAN_H_INCLUDED
#define TSP_GENTRAN_H_INCLUDED

#include "mpiimpl.h"
#include "tsp_gentran_types.h"

#define MPIR_TSP_TRANSPORT_NAME           Gentran_

/* Transport data structures */
#define MPIR_TSP_sched_t                  MPII_Genutil_sched_t

/* General transport API */
#define MPIR_TSP_sched_create              MPII_Genutil_sched_create
#define MPIR_TSP_sched_isend               MPII_Genutil_sched_isend
#define MPIR_TSP_sched_irecv               MPII_Genutil_sched_irecv
#define MPIR_TSP_sched_imcast              MPII_Genutil_sched_imcast
#define MPIR_TSP_sched_reduce_local        MPII_Genutil_sched_reduce_local
#define MPIR_TSP_sched_localcopy           MPII_Genutil_sched_localcopy
#define MPIR_TSP_sched_selective_sink      MPII_Genutil_sched_selective_sink
#define MPIR_TSP_sched_sink                MPII_Genutil_sched_sink
#define MPIR_TSP_sched_fence               MPII_Genutil_sched_fence
#define MPIR_TSP_sched_malloc              MPII_Genutil_sched_malloc
#define MPIR_TSP_sched_start               MPII_Genutil_sched_start

extern MPII_Coll_queue_t coll_queue;
extern int MPII_Genutil_progress_hook_id;

/* Transport function to initialize a new schedule */
int MPII_Genutil_sched_create(MPII_Genutil_sched_t * sched);

/* Transport function to schedule a isend vertex */
int MPII_Genutil_sched_isend(const void *buf,
                             int count,
                             MPI_Datatype dt,
                             int dest,
                             int tag,
                             MPIR_Comm * comm_ptr,
                             MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs);

/* Transport function to schedule a irecv vertex */
int MPII_Genutil_sched_irecv(void *buf,
                             int count,
                             MPI_Datatype dt,
                             int source,
                             int tag,
                             MPIR_Comm * comm_ptr,
                             MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs);

/* Transport function to schedule a imcast vertex */
int MPII_Genutil_sched_imcast(const void *buf,
                              int count,
                              MPI_Datatype dt,
                              UT_array * dests,
                              int num_dests,
                              int tag,
                              MPIR_Comm * comm_ptr,
                              MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs);


/* Transport function to schedule a local reduce vertex */
int MPII_Genutil_sched_reduce_local(const void *inbuf, void *inoutbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, MPII_Genutil_sched_t * sched,
                                    int n_in_vtcs, int *in_vtcs);

/* Transport function to schedule a local data copy */
int MPII_Genutil_sched_localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                 MPII_Genutil_sched_t * sched, int n_in_vtcs, int *in_vtcs);

/* Transport function to schedule a vertex that completes when all the incoming vertices have
 * completed */
int MPII_Genutil_sched_selective_sink(MPII_Genutil_sched_t * sched, int n_in_vtcs, int *invtcs);

/* Transport function to allocate memory required for schedule execution */
void *MPII_Genutil_sched_malloc(size_t size, MPII_Genutil_sched_t * sched);

/* Transport function to enqueue and kick start a non-blocking
 * collective */
int MPII_Genutil_sched_start(MPII_Genutil_sched_t * sched, MPIR_Comm * comm,
                             MPIR_Request ** request);


/* Transport function to schedule a sink */
int MPII_Genutil_sched_sink(MPII_Genutil_sched_t * sched);

/* Transport function to schedule a fence */
void MPII_Genutil_sched_fence(MPII_Genutil_sched_t * sched);

#endif /* TSP_GENTRAN_H_INCLUDED */
