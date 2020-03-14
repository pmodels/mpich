/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TSP_STUBTRAN_H_INCLUDED
#define TSP_STUBTRAN_H_INCLUDED

/* Undefine the previous definitions to avoid redefinition warnings */
#undef MPIR_TSP_TRANSPORT_NAME
#undef MPIR_TSP_sched_t
#undef MPIR_TSP_sched_create
#undef MPIR_TSP_sched_isend
#undef MPIR_TSP_sched_irecv
#undef MPIR_TSP_sched_imcast
#undef MPIR_TSP_sched_issend
#undef MPIR_TSP_sched_reduce_local
#undef MPIR_TSP_sched_localcopy
#undef MPIR_TSP_sched_selective_sink
#undef MPIR_TSP_sched_sink
#undef MPIR_TSP_sched_fence
#undef MPIR_TSP_sched_new_type
#undef MPIR_TSP_sched_generic
#undef MPIR_TSP_sched_malloc
#undef MPIR_TSP_sched_start
#undef MPIR_TSP_sched_free
#undef MPIR_TSP_sched_optimize
#undef MPIR_TSP_sched_reset

#define MPIR_TSP_TRANSPORT_NAME             Stubtran_

/* Stub transport data structures */
#define MPIR_TSP_sched_t                    MPII_Stubutil_sched_t

/* Stub transport API */
#define MPIR_TSP_sched_create               MPII_Stubutil_sched_create
#define MPIR_TSP_sched_isend                MPII_Stubutil_sched_isend
#define MPIR_TSP_sched_irecv                MPII_Stubutil_sched_irecv
#define MPIR_TSP_sched_imcast               MPII_Stubutil_sched_imcast
#define MPIR_TSP_sched_issend               MPII_Stubutil_sched_issend
#define MPIR_TSP_sched_reduce_local         MPII_Stubutil_sched_reduce_local
#define MPIR_TSP_sched_localcopy            MPII_Stubutil_sched_localcopy
#define MPIR_TSP_sched_selective_sink       MPII_Stubutil_sched_selective_sink
#define MPIR_TSP_sched_sink                 MPII_Stubutil_sched_sink
#define MPIR_TSP_sched_fence                MPII_Stubutil_sched_fence
#define MPIR_TSP_sched_new_type             MPII_Stubutil_sched_new_type
#define MPIR_TSP_sched_generic              MPII_Stubutil_sched_generic
#define MPIR_TSP_sched_malloc               MPII_Stubutil_sched_malloc
#define MPIR_TSP_sched_start                MPII_Stubutil_sched_start
#define MPIR_TSP_sched_free                 MPII_Stubutil_sched_free

int MPII_Stubutil_sched_create(MPII_Stubutil_sched_t * sched);
void MPII_Stubutil_sched_free(MPII_Stubutil_sched_t * sched);
int MPII_Stubutil_sched_new_type(MPII_Stubutil_sched_t * sched,
                                 MPII_Stubutil_sched_issue_fn issue_fn,
                                 MPII_Stubutil_sched_complete_fn complete_fn,
                                 MPII_Stubutil_sched_free_fn free_fn);

int MPII_Stubutil_sched_generic(int type_id, void *data,
                                MPII_Stubutil_sched_t * sched, int n_in_vtcs, int *in_vtcs,
                                int *vtx_id);
int MPII_Stubutil_sched_isend(const void *buf, int count, MPI_Datatype dt, int dest, int tag,
                              MPIR_Comm * comm_ptr, MPII_Stubutil_sched_t * sched,
                              int n_invtcs, int *invtcs);
int MPII_Stubutil_sched_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                              MPIR_Comm * comm_ptr, MPII_Stubutil_sched_t * sched,
                              int n_invtcs, int *invtcs);
int MPII_Stubutil_sched_imcast(const void *buf, int count, MPI_Datatype dt, UT_array * destinations,
                               int num_destinations, int tag, MPIR_Comm * comm_ptr,
                               MPII_Stubutil_sched_t * sched, int n_invtcs, int *invtcs);
int MPII_Stubutil_sched_issend(const void *buf, int count, MPI_Datatype dt, int dest, int tag,
                               MPIR_Comm * comm_ptr, MPII_Stubutil_sched_t * sched,
                               int n_invtcs, int *invtcs);
int MPII_Stubutil_sched_reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype dt,
                                     MPI_Op op, MPII_Stubutil_sched_t * sched, int n_invtcs,
                                     int *invtcs);
int MPII_Stubutil_sched_localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                  MPII_Stubutil_sched_t * sched, int n_in_vtcs, int *in_vtcs);
int MPII_Stubutil_sched_selective_sink(MPII_Stubutil_sched_t * sched, int n_in_vtcs, int *invtcs);
int MPII_Genutil_sched_sink(MPII_Genutil_sched_t * sched);
void MPII_Genutil_sched_fence(MPII_Genutil_sched_t * sched);
void *MPII_Stubutil_sched_malloc(size_t size, MPII_Stubutil_sched_t * sched);
int MPII_Stubutil_sched_start(MPII_Stubutil_sched_t * sched, MPIR_Comm * comm,
                              MPII_Coll_req_t ** request);

#endif /* TSP_STUBTRAN_H_INCLUDED */
