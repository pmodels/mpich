/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TSP_IMPL_H_INCLUDED
#define TSP_IMPL_H_INCLUDED

/* transport initialization */
int MPII_TSP_init(void);

/* transport cleanup */
int MPII_TSP_finalize(void);

/* communicator-specific initializtion */
int MPII_TSP_comm_init(MPIR_Comm * comm_ptr);

/* communicator-specific cleanup */
int MPII_TSP_comm_cleanup(MPIR_Comm * comm_ptr);

/* check if there are any pending schedules */
int MPII_TSP_scheds_are_pending(void);

/* TSP sched interfaces */
typedef void *MPIR_TSP_sched_t;

typedef int (*MPIR_TSP_sched_issue_fn) (void *data, int *done);
typedef int (*MPIR_TSP_sched_complete_fn) (void *data, int *is_completed);
typedef int (*MPIR_TSP_sched_free_fn) (void *data);

/* callback function for CB vertex type */
typedef int (*MPIR_TSP_cb_t) (struct MPIR_Comm * comm, int tag, void *data);

/* Transport function to initialize a new schedule */
int MPIR_TSP_sched_create(MPIR_TSP_sched_t * sched, bool is_persistent);

/* Transport function to free a schedule */
int MPIR_TSP_sched_free(MPIR_TSP_sched_t sched);

int MPIR_TSP_sched_new_type(MPIR_TSP_sched_t s, MPIR_TSP_sched_issue_fn issue_fn,
                            MPIR_TSP_sched_complete_fn complete_fn, MPIR_TSP_sched_free_fn free_fn);

int MPIR_TSP_sched_generic(int type_id, void *data,
                           MPIR_TSP_sched_t sched, int n_in_vtcs, int *in_vtcs, int *vtx_id);

/* Transport function to schedule an isend vertex */
int MPIR_TSP_sched_isend(const void *buf,
                         MPI_Aint count,
                         MPI_Datatype dt,
                         int dest,
                         int tag,
                         MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched, int n_in_vtcs, int *in_vtcs,
                         int *vtx_id);

/* Transport function to schedule a issend vertex */
int MPIR_TSP_sched_issend(const void *buf,
                          MPI_Aint count,
                          MPI_Datatype dt,
                          int dest,
                          int tag,
                          MPIR_Comm * comm_ptr,
                          MPIR_TSP_sched_t sched, int n_in_vtcs, int *in_vtcs, int *vtx_id);

/* Transport function to schedule an irecv vertex */
int MPIR_TSP_sched_irecv(void *buf,
                         MPI_Aint count,
                         MPI_Datatype dt,
                         int source,
                         int tag,
                         MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched, int n_in_vtcs, int *in_vtcs,
                         int *vtx_id);

/* Transport function to schedule a irecv with status vertex */
int MPIR_TSP_sched_irecv_status(void *buf,
                                MPI_Aint count,
                                MPI_Datatype dt,
                                int source,
                                int tag,
                                MPIR_Comm * comm_ptr, MPI_Status * status,
                                MPIR_TSP_sched_t sched, int n_in_vtcs, int *in_vtcs, int *vtx_id);

/* Transport function to schedule an imcast vertex */
int MPIR_TSP_sched_imcast(const void *buf,
                          MPI_Aint count,
                          MPI_Datatype dt,
                          int *dests,
                          int num_dests,
                          int tag,
                          MPIR_Comm * comm_ptr,
                          MPIR_TSP_sched_t sched, int n_in_vtcs, int *in_vtcs, int *vtx_id);


/* Transport function to schedule a local reduce vertex */
int MPIR_TSP_sched_reduce_local(const void *inbuf, void *inoutbuf, MPI_Aint count,
                                MPI_Datatype datatype, MPI_Op op, MPIR_TSP_sched_t sched,
                                int n_in_vtcs, int *in_vtcs, int *vtx_id);

/* Transport function to schedule a local data copy */
int MPIR_TSP_sched_localcopy(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                             MPIR_TSP_sched_t sched, int n_in_vtcs, int *in_vtcs, int *vtx_id);

/* Transport function that adds a callback type vertex in the graph */
int MPIR_TSP_sched_cb(MPIR_TSP_cb_t cb_p, void *cb_data, MPIR_TSP_sched_t sched,
                      int n_in_vtcs, int *in_vtcs, int *vtx_id);

/* Transport function to schedule a vertex that completes when all the incoming vertices have
 * completed */
int MPIR_TSP_sched_selective_sink(MPIR_TSP_sched_t sched, int n_in_vtcs, int *invtcs, int *vtx_id);

/* Transport function to allocate memory required for schedule execution */
void *MPIR_TSP_sched_malloc(size_t size, MPIR_TSP_sched_t sched);

/* Transport function to schedule a sub-schedule */
int MPIR_TSP_sched_sub_sched(MPIR_TSP_sched_t sched, MPIR_TSP_sched_t subsched,
                             int n_in_vtcs, int *in_vtcs, int *vtx_id);

/* Transport function to enqueue and kick start a non-blocking
 * collective */
int MPIR_TSP_sched_start(MPIR_TSP_sched_t sched, MPIR_Comm * comm, MPIR_Request ** request);

/* Transport function to reset a completed persistent collective */
int MPIR_TSP_sched_reset(MPIR_TSP_sched_t sched);

/* Transport function to schedule a sink */
int MPIR_TSP_sched_sink(MPIR_TSP_sched_t sched, int *vtx_id);

/* Transport function to schedule a fence */
int MPIR_TSP_sched_fence(MPIR_TSP_sched_t sched);

#endif /* TSP_IMPL_H_INCLUDED */
