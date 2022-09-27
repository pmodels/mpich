/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDCH4_H_INCLUDED
#define MPIDCH4_H_INCLUDED

/* We need to define the static inlines right away to avoid
 * any implicit prototype generation and subsequent warnings
 * This allows us to make ADI up calls from within a direct
 * netmod.
 */

int MPID_Init(int, int *);
int MPID_InitCompleted(void);
int MPID_Allocate_vci(int *vci);
int MPID_Deallocate_vci(int vci);
MPL_STATIC_INLINE_PREFIX int MPID_Cancel_recv(MPIR_Request *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Cancel_send(MPIR_Request *) MPL_STATIC_INLINE_SUFFIX;
int MPID_Comm_disconnect(MPIR_Comm *);
int MPID_Comm_spawn_multiple(int, char *[], char **[], const int[], MPIR_Info *[], int, MPIR_Comm *,
                             MPIR_Comm **, int[]);
int MPID_Comm_failure_get_acked(MPIR_Comm *, MPIR_Group **);
int MPID_Comm_get_all_failed_procs(MPIR_Comm *, MPIR_Group **, int);
int MPID_Comm_revoke(MPIR_Comm *, int);
int MPID_Comm_failure_ack(MPIR_Comm *);
MPL_STATIC_INLINE_PREFIX int MPID_Comm_AS_enabled(MPIR_Comm *) MPL_STATIC_INLINE_SUFFIX;
int MPID_Comm_get_lpid(MPIR_Comm *, int, uint64_t *, bool);
int MPID_CS_finalize(void);
int MPID_Finalize(void);
int MPID_Get_universe_size(int *);
int MPID_Get_processor_name(char *, int, int *);
MPL_STATIC_INLINE_PREFIX int MPID_Iprobe(int, int, MPIR_Comm *, int, int *,
                                         MPI_Status *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Irecv(void *, MPI_Aint, MPI_Datatype, int, int, MPIR_Comm *, int,
                                        MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Isend(const void *, MPI_Aint, MPI_Datatype, int, int, MPIR_Comm *,
                                        int, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Issend(const void *, MPI_Aint, MPI_Datatype, int, int,
                                         MPIR_Comm *, int,
                                         MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Mrecv(void *, MPI_Aint, MPI_Datatype, MPIR_Request *,
                                        MPI_Status *, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Imrecv(void *, MPI_Aint, MPI_Datatype, MPIR_Request *,
                                         MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
int MPID_Open_port(MPIR_Info *, char *);
int MPID_Close_port(const char *);
int MPID_Comm_accept(const char *, MPIR_Info *, int, MPIR_Comm *, MPIR_Comm **);
int MPID_Comm_connect(const char *, MPIR_Info *, int, MPIR_Comm *, MPIR_Comm **);
MPL_STATIC_INLINE_PREFIX int MPID_Probe(int, int, MPIR_Comm *, int,
                                        MPI_Status *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Mprobe(int, int, MPIR_Comm *, int, MPIR_Request **,
                                         MPI_Status *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Improbe(int, int, MPIR_Comm *, int, int *, MPIR_Request **,
                                          MPI_Status *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Progress_test(MPID_Progress_state *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Progress_poke(void) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPID_Progress_start(MPID_Progress_state *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPID_Progress_end(MPID_Progress_state *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Progress_wait(MPID_Progress_state *) MPL_STATIC_INLINE_SUFFIX;
int MPID_Progress_register(int (*progress_fn) (int *), int *id);
int MPID_Progress_deregister(int id);
int MPID_Progress_activate(int id);
int MPID_Progress_deactivate(int id);
MPL_STATIC_INLINE_PREFIX int MPID_Recv(void *, MPI_Aint, MPI_Datatype, int, int, MPIR_Comm *, int,
                                       MPI_Status *, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Recv_init(void *, MPI_Aint, MPI_Datatype, int, int, MPIR_Comm *,
                                            int, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPID_Request_set_completed(MPIR_Request *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Request_complete(MPIR_Request *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Request_is_anysource(MPIR_Request *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPID_Prequest_free_hook(MPIR_Request *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX void MPID_Part_request_free_hook(MPIR_Request *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Send(const void *, MPI_Aint, MPI_Datatype, int, int, MPIR_Comm *,
                                       int, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ssend(const void *, MPI_Aint, MPI_Datatype, int, int, MPIR_Comm *,
                                        int, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Rsend(const void *, MPI_Aint, MPI_Datatype, int, int, MPIR_Comm *,
                                        int, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Irsend(const void *, MPI_Aint, MPI_Datatype, int, int,
                                         MPIR_Comm *, int,
                                         MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Send_init(const void *, MPI_Aint, MPI_Datatype, int, int,
                                            MPIR_Comm *, int,
                                            MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ssend_init(const void *, MPI_Aint, MPI_Datatype, int, int,
                                             MPIR_Comm *, int,
                                             MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Bsend_init(const void *, MPI_Aint, MPI_Datatype, int, int,
                                             MPIR_Comm *, int,
                                             MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Rsend_init(const void *, MPI_Aint, MPI_Datatype, int, int,
                                             MPIR_Comm *, int,
                                             MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Startall(int, MPIR_Request *[]) MPL_STATIC_INLINE_SUFFIX;

int MPID_Psend_init(const void *, int, MPI_Aint, MPI_Datatype, int, int, MPIR_Comm *,
                    MPIR_Info *, MPIR_Request **);
int MPID_Precv_init(void *, int, MPI_Aint, MPI_Datatype, int, int, MPIR_Comm *, MPIR_Info *,
                    MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPID_Pready_range(int, int, MPIR_Request *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Pready_list(int, const int[],
                                              MPIR_Request *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Parrived(MPIR_Request * rreq, int partition, int *flag);

MPL_STATIC_INLINE_PREFIX int MPID_Accumulate(const void *, MPI_Aint, MPI_Datatype, int,
                                             MPI_Aint, MPI_Aint, MPI_Datatype, MPI_Op,
                                             MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
int MPID_Win_create(void *, MPI_Aint, MPI_Aint, MPIR_Info *, MPIR_Comm *, MPIR_Win **);
MPL_STATIC_INLINE_PREFIX int MPID_Win_fence(int, MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
int MPID_Win_free(MPIR_Win **);
MPL_STATIC_INLINE_PREFIX int MPID_Get(void *, MPI_Aint, MPI_Datatype, int, MPI_Aint, MPI_Aint,
                                      MPI_Datatype, MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
int MPID_Win_get_info(MPIR_Win *, MPIR_Info **);
MPL_STATIC_INLINE_PREFIX int MPID_Win_lock(int, int, int, MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_unlock(int, MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_start(MPIR_Group *, int, MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_complete(MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_post(MPIR_Group *, int, MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_wait(MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_test(MPIR_Win *, int *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Put(const void *, MPI_Aint, MPI_Datatype, int, MPI_Aint, MPI_Aint,
                                      MPI_Datatype, MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
int MPID_Win_set_info(MPIR_Win *, MPIR_Info *);
int MPID_Comm_reenable_anysource(MPIR_Comm *, MPIR_Group **);
int MPID_Comm_remote_group_failed(MPIR_Comm *, MPIR_Group **);
int MPID_Comm_group_failed(MPIR_Comm *, MPIR_Group **);
int MPID_Win_attach(MPIR_Win *, void *, MPI_Aint);
int MPID_Win_allocate_shared(MPI_Aint, MPI_Aint, MPIR_Info *, MPIR_Comm *, void **, MPIR_Win **);
MPL_STATIC_INLINE_PREFIX int MPID_Rput(const void *, MPI_Aint, MPI_Datatype, int, MPI_Aint,
                                       MPI_Aint, MPI_Datatype, MPIR_Win *,
                                       MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_local(int, MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
int MPID_Win_detach(MPIR_Win *, const void *);
MPL_STATIC_INLINE_PREFIX int MPID_Compare_and_swap(const void *, const void *, void *, MPI_Datatype,
                                                   int, MPI_Aint,
                                                   MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Raccumulate(const void *, MPI_Aint, MPI_Datatype, int, MPI_Aint,
                                              MPI_Aint, MPI_Datatype, MPI_Op, MPIR_Win *,
                                              MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Rget_accumulate(const void *, MPI_Aint, MPI_Datatype, void *,
                                                  MPI_Aint, MPI_Datatype, int, MPI_Aint, MPI_Aint,
                                                  MPI_Datatype, MPI_Op, MPIR_Win *,
                                                  MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Fetch_and_op(const void *, void *, MPI_Datatype, int, MPI_Aint,
                                               MPI_Op, MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_shared_query(MPIR_Win *, int, MPI_Aint *, int *,
                                                   void *) MPL_STATIC_INLINE_SUFFIX;
int MPID_Win_allocate(MPI_Aint, MPI_Aint, MPIR_Info *, MPIR_Comm *, void *, MPIR_Win **);
MPL_STATIC_INLINE_PREFIX int MPID_Win_flush(int, MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_local_all(MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_unlock_all(MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
int MPID_Win_create_dynamic(MPIR_Info *, MPIR_Comm *, MPIR_Win **);
MPL_STATIC_INLINE_PREFIX int MPID_Rget(void *, MPI_Aint, MPI_Datatype, int, MPI_Aint, MPI_Aint,
                                       MPI_Datatype, MPIR_Win *,
                                       MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_sync(MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_flush_all(MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Get_accumulate(const void *, MPI_Aint, MPI_Datatype, void *,
                                                 MPI_Aint, MPI_Datatype, int, MPI_Aint, MPI_Aint,
                                                 MPI_Datatype, MPI_Op,
                                                 MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Win_lock_all(int, MPIR_Win *) MPL_STATIC_INLINE_SUFFIX;
void *MPID_Alloc_mem(MPI_Aint, MPIR_Info *);
int MPID_Free_mem(void *);
int MPID_Get_node_id(MPIR_Comm *, int rank, int *);
int MPID_Get_max_node_id(MPIR_Comm *, int *);
MPL_STATIC_INLINE_PREFIX int MPID_Request_is_pending_failure(MPIR_Request *)
    MPL_STATIC_INLINE_SUFFIX;
MPI_Aint MPID_Aint_add(MPI_Aint, MPI_Aint);
MPI_Aint MPID_Aint_diff(MPI_Aint, MPI_Aint);
int MPID_Type_commit_hook(MPIR_Datatype *);
int MPID_Type_free_hook(MPIR_Datatype *);
int MPID_Op_commit_hook(MPIR_Op *);
int MPID_Op_free_hook(MPIR_Op *);
int MPID_Intercomm_exchange_map(MPIR_Comm *, int, MPIR_Comm *, int, int *, uint64_t **, int *);
int MPID_Create_intercomm_from_lpids(MPIR_Comm *, int, const uint64_t[]);
int MPID_Comm_commit_pre_hook(MPIR_Comm *);
int MPID_Comm_free_hook(MPIR_Comm *);
int MPID_Comm_set_hints(MPIR_Comm *, MPIR_Info *);
int MPID_Comm_commit_post_hook(MPIR_Comm *);
int MPID_Stream_create_hook(MPIR_Stream * stream);
int MPID_Stream_free_hook(MPIR_Stream * stream);
MPL_STATIC_INLINE_PREFIX int MPID_Barrier(MPIR_Comm *, MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Bcast(void *, MPI_Aint, MPI_Datatype, int, MPIR_Comm *,
                                        MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Allreduce(const void *, void *, MPI_Aint, MPI_Datatype, MPI_Op,
                                            MPIR_Comm *, MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Allgather(const void *, MPI_Aint, MPI_Datatype, void *, MPI_Aint,
                                            MPI_Datatype, MPIR_Comm *,
                                            MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Allgatherv(const void *, MPI_Aint, MPI_Datatype, void *,
                                             const MPI_Aint *, const MPI_Aint *, MPI_Datatype,
                                             MPIR_Comm *,
                                             MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Scatter(const void *, MPI_Aint, MPI_Datatype, void *, MPI_Aint,
                                          MPI_Datatype, int, MPIR_Comm *,
                                          MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Scatterv(const void *, const MPI_Aint *, const MPI_Aint *,
                                           MPI_Datatype, void *, MPI_Aint, MPI_Datatype, int,
                                           MPIR_Comm *, MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Gather(const void *, MPI_Aint, MPI_Datatype, void *, MPI_Aint,
                                         MPI_Datatype, int, MPIR_Comm *,
                                         MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Gatherv(const void *, MPI_Aint, MPI_Datatype, void *,
                                          const MPI_Aint *, const MPI_Aint *, MPI_Datatype, int,
                                          MPIR_Comm *, MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Alltoall(const void *, MPI_Aint, MPI_Datatype, void *, MPI_Aint,
                                           MPI_Datatype, MPIR_Comm *,
                                           MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Alltoallv(const void *, const MPI_Aint *, const MPI_Aint *,
                                            MPI_Datatype, void *, const MPI_Aint *,
                                            const MPI_Aint *, MPI_Datatype, MPIR_Comm *,
                                            MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Alltoallw(const void *, const MPI_Aint[], const MPI_Aint[],
                                            const MPI_Datatype[], void *, const MPI_Aint[],
                                            const MPI_Aint[], const MPI_Datatype[], MPIR_Comm *,
                                            MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Reduce(const void *, void *, MPI_Aint, MPI_Datatype, MPI_Op, int,
                                         MPIR_Comm *, MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter(const void *, void *, const MPI_Aint[],
                                                 MPI_Datatype, MPI_Op, MPIR_Comm *,
                                                 MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter_block(const void *, void *, MPI_Aint, MPI_Datatype,
                                                       MPI_Op, MPIR_Comm *,
                                                       MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Scan(const void *, void *, MPI_Aint, MPI_Datatype, MPI_Op,
                                       MPIR_Comm *, MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Exscan(const void *, void *, MPI_Aint, MPI_Datatype, MPI_Op,
                                         MPIR_Comm *, MPIR_Errflag_t *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgather(const void *, MPI_Aint, MPI_Datatype, void *,
                                                     MPI_Aint, MPI_Datatype,
                                                     MPIR_Comm *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgatherv(const void *, MPI_Aint, MPI_Datatype, void *,
                                                      const MPI_Aint[], const MPI_Aint[],
                                                      MPI_Datatype,
                                                      MPIR_Comm *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallv(const void *, const MPI_Aint[],
                                                     const MPI_Aint[], MPI_Datatype, void *,
                                                     const MPI_Aint[], const MPI_Aint[],
                                                     MPI_Datatype,
                                                     MPIR_Comm *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallw(const void *, const MPI_Aint[],
                                                     const MPI_Aint[], const MPI_Datatype[], void *,
                                                     const MPI_Aint[], const MPI_Aint[],
                                                     const MPI_Datatype[],
                                                     MPIR_Comm *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoall(const void *, MPI_Aint, MPI_Datatype, void *,
                                                    MPI_Aint, MPI_Datatype,
                                                    MPIR_Comm *) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgather(const void *, MPI_Aint, MPI_Datatype, void *,
                                                      MPI_Aint, MPI_Datatype, MPIR_Comm *,
                                                      MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgatherv(const void *, MPI_Aint, MPI_Datatype, void *,
                                                       const MPI_Aint[], const MPI_Aint[],
                                                       MPI_Datatype, MPIR_Comm *,
                                                       MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoall(const void *, MPI_Aint, MPI_Datatype, void *,
                                                     MPI_Aint, MPI_Datatype, MPIR_Comm *,
                                                     MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallv(const void *, const MPI_Aint[],
                                                      const MPI_Aint[], MPI_Datatype, void *,
                                                      const MPI_Aint[], const MPI_Aint[],
                                                      MPI_Datatype, MPIR_Comm *,
                                                      MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallw(const void *, const MPI_Aint[],
                                                      const MPI_Aint[], const MPI_Datatype[],
                                                      void *, const MPI_Aint[], const MPI_Aint[],
                                                      const MPI_Datatype[], MPIR_Comm *,
                                                      MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ibarrier(MPIR_Comm *, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ibcast(void *, MPI_Aint, MPI_Datatype, int, MPIR_Comm *,
                                         MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Iallgather(const void *, MPI_Aint, MPI_Datatype, void *, MPI_Aint,
                                             MPI_Datatype, MPIR_Comm *,
                                             MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Iallgatherv(const void *, MPI_Aint, MPI_Datatype, void *,
                                              const MPI_Aint *, const MPI_Aint *, MPI_Datatype,
                                              MPIR_Comm *,
                                              MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Iallreduce(const void *, void *, MPI_Aint, MPI_Datatype, MPI_Op,
                                             MPIR_Comm *, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ialltoall(const void *, MPI_Aint, MPI_Datatype, void *, MPI_Aint,
                                            MPI_Datatype, MPIR_Comm *,
                                            MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallv(const void *, const MPI_Aint[], const MPI_Aint[],
                                             MPI_Datatype, void *, const MPI_Aint[],
                                             const MPI_Aint[], MPI_Datatype, MPIR_Comm *,
                                             MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallw(const void *, const MPI_Aint[], const MPI_Aint[],
                                             const MPI_Datatype[], void *, const MPI_Aint[],
                                             const MPI_Aint[], const MPI_Datatype[], MPIR_Comm *,
                                             MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Iexscan(const void *, void *, MPI_Aint, MPI_Datatype, MPI_Op,
                                          MPIR_Comm *, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Igather(const void *, MPI_Aint, MPI_Datatype, void *, MPI_Aint,
                                          MPI_Datatype, int, MPIR_Comm *,
                                          MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Igatherv(const void *, MPI_Aint, MPI_Datatype, void *,
                                           const MPI_Aint *, const MPI_Aint *, MPI_Datatype, int,
                                           MPIR_Comm *, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter_block(const void *, void *, MPI_Aint,
                                                        MPI_Datatype, MPI_Op, MPIR_Comm *,
                                                        MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter(const void *, void *, const MPI_Aint[],
                                                  MPI_Datatype, MPI_Op, MPIR_Comm *,
                                                  MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Ireduce(const void *, void *, MPI_Aint, MPI_Datatype, MPI_Op, int,
                                          MPIR_Comm *, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Iscan(const void *, void *, MPI_Aint, MPI_Datatype, MPI_Op,
                                        MPIR_Comm *, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Iscatter(const void *, MPI_Aint, MPI_Datatype, void *, MPI_Aint,
                                           MPI_Datatype, int, MPIR_Comm *,
                                           MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPID_Iscatterv(const void *, const MPI_Aint *, const MPI_Aint *,
                                            MPI_Datatype, void *, MPI_Aint, MPI_Datatype, int,
                                            MPIR_Comm *, MPIR_Request **) MPL_STATIC_INLINE_SUFFIX;
int MPID_Send_enqueue(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                      int dest, int tag, MPIR_Comm * comm_ptr);
int MPID_Recv_enqueue(void *buf, MPI_Aint count, MPI_Datatype datatype,
                      int source, int tag, MPIR_Comm * comm_ptr, MPI_Status * status);
int MPID_Isend_enqueue(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                       int dest, int tag, MPIR_Comm * comm_ptr, MPIR_Request ** req);
int MPID_Irecv_enqueue(void *buf, MPI_Aint count, MPI_Datatype datatype,
                       int source, int tag, MPIR_Comm * comm_ptr, MPIR_Request ** req);
int MPID_Wait_enqueue(MPIR_Request * req_ptr, MPI_Status * status);
int MPID_Waitall_enqueue(int count, MPI_Request * array_of_requests,
                         MPI_Status * array_of_statuses);
int MPID_Abort(struct MPIR_Comm *comm, int mpi_errno, int exit_code, const char *error_msg);

/* This function is not exposed to the upper layers but functions in a way
 * similar to the functions above. Other CH4-level functions should call this
 * function to query locality. This function will determine whether to call the
 * netmod or MPIDIU locality functions. */
MPL_STATIC_INLINE_PREFIX int MPIDI_rank_is_local(int rank, MPIR_Comm * comm);
MPL_STATIC_INLINE_PREFIX int MPIDI_av_is_local(MPIDI_av_entry_t * av);

/* Include netmod prototypes */
#include <netmod.h>
#ifndef MPIDI_CH4_DIRECT_NETMOD
#include "shm.h"
#endif

/* Declare request functions here so netmods can refer to
   them in the NETMOD_INLINE mode */
#include "ch4_request.h"

/* vci is used by both generic and lower layer, need come early */
#include "ch4_vci.h"

/* Active message and generic implementatiions */
#include "mpidig.h"
#include "mpidch4r.h"

/* Include netmod and shm implementations  */
/* Prototypes are split from impl to avoid */
/* circular dependencies                   */
#include <netmod_impl.h>
#ifndef MPIDI_CH4_DIRECT_NETMOD
#include "shm_impl.h"
#endif

#include "ch4_probe.h"
#include "ch4_send.h"
#include "ch4_startall.h"
#include "ch4_recv.h"
#include "ch4_comm.h"
#include "ch4_win.h"
#include "ch4_rma.h"
#include "ch4_progress.h"
#include "ch4_proc.h"
#include "ch4_coll.h"
#include "ch4_wait.h"
#include "ch4_part.h"

#define MPIDI_MAX_NETMOD_STRING_LEN 64
extern int MPIDI_num_netmods;
#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_CH4_DBG_GENERAL;
extern MPL_dbg_class MPIDI_CH4_DBG_MAP;
extern MPL_dbg_class MPIDI_CH4_DBG_COMM;
extern MPL_dbg_class MPIDI_CH4_DBG_MEMORY;
#endif /* MPL_USE_DBG_LOGGING */

/* routines only used during init */
int MPIDI_create_init_comm(MPIR_Comm ** comm_ptr);
void MPIDI_destroy_init_comm(MPIR_Comm ** comm_ptr);

#endif /* MPIDCH4_H_INCLUDED */
