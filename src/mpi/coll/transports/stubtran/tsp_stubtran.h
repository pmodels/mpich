/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef TSP_STUBTRAN_H_INCLUDED
#define TSP_STUBTRAN_H_INCLUDED

#define MPIR_TSP_TRANSPORT_NAME             Stubtran_

/* Stub transport data structures */
#define MPIR_TSP_sched_t                    MPII_Stubutil_sched_t

/* Stub transport API */
#define MPIR_TSP_sched_create               MPII_Stubutil_sched_create
#define MPIR_TSP_sched_isend                MPII_Stubutil_sched_isend
#define MPIR_TSP_sched_irecv                MPII_Stubutil_sched_irecv
#define MPIR_TSP_sched_imcast               MPII_Stubutil_sched_imcast
#define MPIR_TSP_sched_start                MPII_Stubutil_sched_start

int MPII_Stubutil_sched_create(MPII_Stubutil_sched_t * sched, int tag);
int MPII_Stubutil_sched_isend(const void *buf, int count, MPI_Datatype dt, int dest, int tag,
                              MPIR_Comm * comm_ptr, MPII_Stubutil_sched_t * sched,
                              int n_invtcs, int *invtcs);
int MPII_Stubutil_sched_imcast(const void *buf, int count, MPI_Datatype dt, int *destinations,
                               int num_destinations, int tag, MPIR_Comm * comm_ptr,
                               MPII_Stubutil_sched_t * sched, int n_invtcs, int *invtcs);
int MPII_Stubutil_sched_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                              MPIR_Comm * comm_ptr, MPII_Stubutil_sched_t * sched,
                              int n_invtcs, int *invtcs);
int MPII_Stubutil_sched_start(MPII_Stubutil_sched_t * sched, MPIR_Comm * comm,
                              MPII_Coll_req_t ** request);

#endif /* TSP_STUBTRAN_H_INCLUDED */
