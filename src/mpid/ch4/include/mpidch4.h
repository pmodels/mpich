/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef MPIDCH4_H_INCLUDED
#define MPIDCH4_H_INCLUDED

/* We need to define the static inlines right away to avoid
 * any implicit prototype generation and subsequent warnings
 * This allows us to make ADI up calls from within a direct
 * netmod.
 */
#define MPIDI_CH4I_API(rc,fcnname,...)            \
  MPL_STATIC_INLINE_PREFIX rc MPIDI_##fcnname(__VA_ARGS__) MPL_STATIC_INLINE_SUFFIX

MPIDI_CH4I_API(int, Init, int *, char ***, int, int *, int *, int *);
MPIDI_CH4I_API(int, InitCompleted, void);
MPIDI_CH4I_API(int, Abort, MPIR_Comm *, int, int, const char *);
MPIDI_CH4I_API(int, Cancel_recv, MPIR_Request *);
MPIDI_CH4I_API(int, Cancel_send, MPIR_Request *);
MPIDI_CH4I_API(int, Comm_disconnect, MPIR_Comm *);
MPIDI_CH4I_API(int, Comm_spawn_multiple, int, char *[], char **[], const int[], MPIR_Info *[], int,
               MPIR_Comm *, MPIR_Comm **, int[]);
MPIDI_CH4I_API(int, Comm_failure_get_acked, MPIR_Comm *, MPIR_Group **);
MPIDI_CH4I_API(int, Comm_get_all_failed_procs, MPIR_Comm *, MPIR_Group **, int);
MPIDI_CH4I_API(int, Comm_revoke, MPIR_Comm *, int);
MPIDI_CH4I_API(int, Comm_failure_ack, MPIR_Comm *);
MPIDI_CH4I_API(int, Comm_AS_enabled, MPIR_Comm *);
MPIDI_CH4I_API(int, Comm_get_lpid, MPIR_Comm *, int, int *, MPL_bool);
MPIDI_CH4I_API(int, Finalize, void);
MPIDI_CH4I_API(int, Get_universe_size, int *);
MPIDI_CH4I_API(int, Get_processor_name, char *, int, int *);
MPIDI_CH4I_API(int, Iprobe, int, int, MPIR_Comm *, int, int *, MPI_Status *);
MPIDI_CH4I_API(int, Irecv, void *, int, MPI_Datatype, int, int, MPIR_Comm *, int, MPIR_Request **);
MPIDI_CH4I_API(int, Isend, const void *, int, MPI_Datatype, int, int, MPIR_Comm *, int,
               MPIR_Request **);
MPIDI_CH4I_API(int, Issend, const void *, int, MPI_Datatype, int, int, MPIR_Comm *, int,
               MPIR_Request **);
MPIDI_CH4I_API(int, Mrecv, void *, int, MPI_Datatype, MPIR_Request *, MPI_Status *);
MPIDI_CH4I_API(int, Imrecv, void *, int, MPI_Datatype, MPIR_Request *, MPIR_Request **);
MPIDI_CH4I_API(int, Open_port, MPIR_Info *, char *);
MPIDI_CH4I_API(int, Close_port, const char *);
MPIDI_CH4I_API(int, Comm_accept, const char *, MPIR_Info *, int, MPIR_Comm *, MPIR_Comm **);
MPIDI_CH4I_API(int, Comm_connect, const char *, MPIR_Info *, int, MPIR_Comm *, MPIR_Comm **);
MPIDI_CH4I_API(int, Probe, int, int, MPIR_Comm *, int, MPI_Status *);
MPIDI_CH4I_API(int, Mprobe, int, int, MPIR_Comm *, int, MPIR_Request **, MPI_Status *);
MPIDI_CH4I_API(int, Improbe, int, int, MPIR_Comm *, int, int *, MPIR_Request **, MPI_Status *);
MPIDI_CH4I_API(int, Progress_test, void);
MPIDI_CH4I_API(int, Progress_poke, void);
MPIDI_CH4I_API(void, Progress_start, MPID_Progress_state *);
MPIDI_CH4I_API(void, Progress_end, MPID_Progress_state *);
MPIDI_CH4I_API(int, Progress_wait, MPID_Progress_state *);
MPIDI_CH4I_API(int, Progress_register, int (*progress_fn) (int *), int *id);
MPIDI_CH4I_API(int, Progress_deregister, int id);
MPIDI_CH4I_API(int, Progress_activate, int id);
MPIDI_CH4I_API(int, Progress_deactivate, int id);
MPIDI_CH4I_API(int, Recv, void *, int, MPI_Datatype, int, int, MPIR_Comm *, int, MPI_Status *,
               MPIR_Request **);
MPIDI_CH4I_API(int, Recv_init, void *, int, MPI_Datatype, int, int, MPIR_Comm *, int,
               MPIR_Request **);
MPIDI_CH4I_API(void, Request_set_completed, MPIR_Request *);
MPIDI_CH4I_API(int, Request_complete, MPIR_Request *);
MPIDI_CH4I_API(int, Request_is_anysource, MPIR_Request *);
MPIDI_CH4I_API(int, Send, const void *, int, MPI_Datatype, int, int, MPIR_Comm *, int,
               MPIR_Request **);
MPIDI_CH4I_API(int, Ssend, const void *, int, MPI_Datatype, int, int, MPIR_Comm *, int,
               MPIR_Request **);
MPIDI_CH4I_API(int, Rsend, const void *, int, MPI_Datatype, int, int, MPIR_Comm *, int,
               MPIR_Request **);
MPIDI_CH4I_API(int, Irsend, const void *, int, MPI_Datatype, int, int, MPIR_Comm *, int,
               MPIR_Request **);
MPIDI_CH4I_API(int, Send_init, const void *, int, MPI_Datatype, int, int, MPIR_Comm *, int,
               MPIR_Request **);
MPIDI_CH4I_API(int, Ssend_init, const void *, int, MPI_Datatype, int, int, MPIR_Comm *, int,
               MPIR_Request **);
MPIDI_CH4I_API(int, Bsend_init, const void *, int, MPI_Datatype, int, int, MPIR_Comm *, int,
               MPIR_Request **);
MPIDI_CH4I_API(int, Rsend_init, const void *, int, MPI_Datatype, int, int, MPIR_Comm *, int,
               MPIR_Request **);
MPIDI_CH4I_API(int, Startall, int, MPIR_Request *[]);
MPIDI_CH4I_API(int, Accumulate, const void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype,
               MPI_Op, MPIR_Win *);
MPIDI_CH4I_API(int, Win_create, void *, MPI_Aint, int, MPIR_Info *, MPIR_Comm *, MPIR_Win **);
MPIDI_CH4I_API(int, Win_fence, int, MPIR_Win *);
MPIDI_CH4I_API(int, Win_free, MPIR_Win **);
MPIDI_CH4I_API(int, Get, void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPIR_Win *);
MPIDI_CH4I_API(int, Win_get_info, MPIR_Win *, MPIR_Info **);
MPIDI_CH4I_API(int, Win_lock, int, int, int, MPIR_Win *);
MPIDI_CH4I_API(int, Win_unlock, int, MPIR_Win *);
MPIDI_CH4I_API(int, Win_start, MPIR_Group *, int, MPIR_Win *);
MPIDI_CH4I_API(int, Win_complete, MPIR_Win *);
MPIDI_CH4I_API(int, Win_post, MPIR_Group *, int, MPIR_Win *);
MPIDI_CH4I_API(int, Win_wait, MPIR_Win *);
MPIDI_CH4I_API(int, Win_test, MPIR_Win *, int *);
MPIDI_CH4I_API(int, Put, const void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype,
               MPIR_Win *);
MPIDI_CH4I_API(int, Win_set_info, MPIR_Win *, MPIR_Info *);
MPIDI_CH4I_API(int, Comm_reenable_anysource, MPIR_Comm *, MPIR_Group **);
MPIDI_CH4I_API(int, Comm_remote_group_failed, MPIR_Comm *, MPIR_Group **);
MPIDI_CH4I_API(int, Comm_group_failed, MPIR_Comm *, MPIR_Group **);
MPIDI_CH4I_API(int, Win_attach, MPIR_Win *, void *, MPI_Aint);
MPIDI_CH4I_API(int, Win_allocate_shared, MPI_Aint, int, MPIR_Info *, MPIR_Comm *, void **,
               MPIR_Win **);
MPIDI_CH4I_API(int, Rput, const void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype,
               MPIR_Win *, MPIR_Request **);
MPIDI_CH4I_API(int, Win_flush_local, int, MPIR_Win *);
MPIDI_CH4I_API(int, Win_detach, MPIR_Win *, const void *);
MPIDI_CH4I_API(int, Compare_and_swap, const void *, const void *, void *, MPI_Datatype, int,
               MPI_Aint, MPIR_Win *);
MPIDI_CH4I_API(int, Raccumulate, const void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype,
               MPI_Op, MPIR_Win *, MPIR_Request **);
MPIDI_CH4I_API(int, Rget_accumulate, const void *, int, MPI_Datatype, void *, int, MPI_Datatype,
               int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPIR_Win *, MPIR_Request **);
MPIDI_CH4I_API(int, Fetch_and_op, const void *, void *, MPI_Datatype, int, MPI_Aint, MPI_Op,
               MPIR_Win *);
MPIDI_CH4I_API(int, Win_shared_query, MPIR_Win *, int, MPI_Aint *, int *, void *);
MPIDI_CH4I_API(int, Win_allocate, MPI_Aint, int, MPIR_Info *, MPIR_Comm *, void *, MPIR_Win **);
MPIDI_CH4I_API(int, Win_flush, int, MPIR_Win *);
MPIDI_CH4I_API(int, Win_flush_local_all, MPIR_Win *);
MPIDI_CH4I_API(int, Win_unlock_all, MPIR_Win *);
MPIDI_CH4I_API(int, Win_create_dynamic, MPIR_Info *, MPIR_Comm *, MPIR_Win **);
MPIDI_CH4I_API(int, Rget, void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPIR_Win *,
               MPIR_Request **);
MPIDI_CH4I_API(int, Win_sync, MPIR_Win *);
MPIDI_CH4I_API(int, Win_flush_all, MPIR_Win *);
MPIDI_CH4I_API(int, Get_accumulate, const void *, int, MPI_Datatype, void *, int, MPI_Datatype, int,
               MPI_Aint, int, MPI_Datatype, MPI_Op, MPIR_Win *);
MPIDI_CH4I_API(int, Win_lock_all, int, MPIR_Win *);
MPIDI_CH4I_API(void *, Alloc_mem, size_t, MPIR_Info *);
MPIDI_CH4I_API(int, Free_mem, void *);
MPIDI_CH4I_API(int, Get_node_id, MPIR_Comm *, int rank, MPID_Node_id_t *);
MPIDI_CH4I_API(int, Get_max_node_id, MPIR_Comm *, MPID_Node_id_t *);
MPIDI_CH4I_API(int, Request_is_pending_failure, MPIR_Request *);
MPIDI_CH4I_API(MPI_Aint, Aint_add, MPI_Aint, MPI_Aint);
MPIDI_CH4I_API(MPI_Aint, Aint_diff, MPI_Aint, MPI_Aint);
MPIDI_CH4I_API(int, Intercomm_exchange_map, MPIR_Comm *, int, MPIR_Comm *, int, int *, int **, int *);
MPIDI_CH4I_API(int, Create_intercomm_from_lpids, MPIR_Comm *, int, const int[]);
MPIDI_CH4I_API(int, Comm_create_hook, MPIR_Comm *);
MPIDI_CH4I_API(int, Comm_free_hook, MPIR_Comm *);
MPIDI_CH4I_API(int, Barrier, MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Bcast, void *, int, MPI_Datatype, int, MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Allreduce, const void *, void *, int, MPI_Datatype, MPI_Op, MPIR_Comm *,
               MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Allgather, const void *, int, MPI_Datatype, void *, int, MPI_Datatype,
               MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Allgatherv, const void *, int, MPI_Datatype, void *, const int *, const int *,
               MPI_Datatype, MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Scatter, const void *, int, MPI_Datatype, void *, int, MPI_Datatype, int,
               MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Scatterv, const void *, const int *, const int *, MPI_Datatype, void *, int,
               MPI_Datatype, int, MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Gather, const void *, int, MPI_Datatype, void *, int, MPI_Datatype, int,
               MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Gatherv, const void *, int, MPI_Datatype, void *, const int *, const int *,
               MPI_Datatype, int, MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Alltoall, const void *, int, MPI_Datatype, void *, int, MPI_Datatype,
               MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Alltoallv, const void *, const int *, const int *, MPI_Datatype, void *,
               const int *, const int *, MPI_Datatype, MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Alltoallw, const void *, const int[], const int[], const MPI_Datatype[], void *,
               const int[], const int[], const MPI_Datatype[], MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Reduce, const void *, void *, int, MPI_Datatype, MPI_Op, int, MPIR_Comm *,
               MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Reduce_scatter, const void *, void *, const int[], MPI_Datatype, MPI_Op,
               MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Reduce_scatter_block, const void *, void *, int, MPI_Datatype, MPI_Op,
               MPIR_Comm *, MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Scan, const void *, void *, int, MPI_Datatype, MPI_Op, MPIR_Comm *,
               MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Exscan, const void *, void *, int, MPI_Datatype, MPI_Op, MPIR_Comm *,
               MPIR_Errflag_t *);
MPIDI_CH4I_API(int, Neighbor_allgather, const void *, int, MPI_Datatype, void *, int, MPI_Datatype,
               MPIR_Comm *);
MPIDI_CH4I_API(int, Neighbor_allgatherv, const void *, int, MPI_Datatype, void *, const int[],
               const int[], MPI_Datatype, MPIR_Comm *);
MPIDI_CH4I_API(int, Neighbor_alltoallv, const void *, const int[], const int[], MPI_Datatype,
               void *, const int[], const int[], MPI_Datatype, MPIR_Comm *);
MPIDI_CH4I_API(int, Neighbor_alltoallw, const void *, const int[], const MPI_Aint[],
               const MPI_Datatype[], void *, const int[], const MPI_Aint[], const MPI_Datatype[],
               MPIR_Comm *);
MPIDI_CH4I_API(int, Neighbor_alltoall, const void *, int, MPI_Datatype, void *, int, MPI_Datatype,
               MPIR_Comm *);
MPIDI_CH4I_API(int, Ineighbor_allgather, const void *, int, MPI_Datatype, void *, int, MPI_Datatype,
               MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Ineighbor_allgatherv, const void *, int, MPI_Datatype, void *, const int[],
               const int[], MPI_Datatype, MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Ineighbor_alltoall, const void *, int, MPI_Datatype, void *, int, MPI_Datatype,
               MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Ineighbor_alltoallv, const void *, const int[], const int[], MPI_Datatype,
               void *, const int[], const int[], MPI_Datatype, MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Ineighbor_alltoallw, const void *, const int[], const MPI_Aint[],
               const MPI_Datatype[], void *, const int[], const MPI_Aint[], const MPI_Datatype[],
               MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Ibarrier, MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Ibcast, void *, int, MPI_Datatype, int, MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Iallgather, const void *, int, MPI_Datatype, void *, int, MPI_Datatype,
               MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Iallgatherv, const void *, int, MPI_Datatype, void *, const int *, const int *,
               MPI_Datatype, MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Iallreduce, const void *, void *, int, MPI_Datatype, MPI_Op, MPIR_Comm *,
               MPI_Request *);
MPIDI_CH4I_API(int, Ialltoall, const void *, int, MPI_Datatype, void *, int, MPI_Datatype,
               MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Ialltoallv, const void *, const int[], const int[], MPI_Datatype, void *,
               const int[], const int[], MPI_Datatype, MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Ialltoallw, const void *, const int[], const int[], const MPI_Datatype[],
               void *, const int[], const int[], const MPI_Datatype[], MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Iexscan, const void *, void *, int, MPI_Datatype, MPI_Op, MPIR_Comm *,
               MPI_Request *);
MPIDI_CH4I_API(int, Igather, const void *, int, MPI_Datatype, void *, int, MPI_Datatype, int,
               MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Igatherv, const void *, int, MPI_Datatype, void *, const int *, const int *,
               MPI_Datatype, int, MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Ireduce_scatter_block, const void *, void *, int, MPI_Datatype, MPI_Op,
               MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Ireduce_scatter, const void *, void *, const int[], MPI_Datatype, MPI_Op,
               MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Ireduce, const void *, void *, int, MPI_Datatype, MPI_Op, int, MPIR_Comm *,
               MPI_Request *);
MPIDI_CH4I_API(int, Iscan, const void *, void *, int, MPI_Datatype, MPI_Op, MPIR_Comm *,
               MPI_Request *);
MPIDI_CH4I_API(int, Iscatter, const void *, int, MPI_Datatype, void *, int, MPI_Datatype, int,
               MPIR_Comm *, MPI_Request *);
MPIDI_CH4I_API(int, Iscatterv, const void *, const int *, const int *, MPI_Datatype, void *, int,
               MPI_Datatype, int, MPIR_Comm *, MPI_Request *);

/* This function is not exposed to the upper layers but functions in a way
 * similar to the functions above. Other CH4-level functions should call this
 * function to query locality. This function will determine whether to call the
 * netmod or CH4U locality functions. */
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_rank_is_local(int rank, MPIR_Comm * comm);

/* Include netmod prototypes */
#include <netmod.h>
#ifdef MPIDI_BUILD_CH4_SHM
#include "shm.h"
#endif

/* Declare request functions here so netmods can refer to
   them in the NETMOD_DIRECT mode */
#include "ch4_request.h"

/* Include netmod and shm implementations  */
/* Prototypes are split from impl to avoid */
/* circular dependencies                   */
#include <netmod_impl.h>
#ifdef MPIDI_BUILD_CH4_SHM
#include "shm_impl.h"
#endif

#include "ch4_init.h"
#include "ch4_probe.h"
#include "ch4_send.h"
#include "ch4_recv.h"
#include "ch4_comm.h"
#include "ch4_win.h"
#include "ch4_rma.h"
#include "ch4_progress.h"
#include "ch4_spawn.h"
#include "ch4_proc.h"
#include "ch4_coll.h"

#define MPID_Abort                       MPIDI_Abort
#define MPID_Accumulate                  MPIDI_Accumulate
#define MPID_Alloc_mem                   MPIDI_Alloc_mem
#define MPID_Bsend_init                  MPIDI_Bsend_init
#define MPID_Cancel_recv                 MPIDI_Cancel_recv
#define MPID_Cancel_send                 MPIDI_Cancel_send
#define MPID_Close_port                  MPIDI_Close_port
#define MPID_Comm_accept                 MPIDI_Comm_accept
#define MPID_Comm_connect                MPIDI_Comm_connect
#define MPID_Comm_disconnect             MPIDI_Comm_disconnect
#define MPID_Comm_group_failed           MPIDI_Comm_group_failed
#define MPID_Comm_reenable_anysource     MPIDI_Comm_reenable_anysource
#define MPID_Comm_remote_group_failed    MPIDI_Comm_remote_group_failed
#define MPID_Comm_spawn_multiple         MPIDI_Comm_spawn_multiple
#define MPID_Comm_failure_get_acked      MPIDI_Comm_failure_get_acked
#define MPID_Comm_get_all_failed_procs   MPIDI_Comm_get_all_failed_procs
#define MPID_Comm_revoke                 MPIDI_Comm_revoke
#define MPID_Comm_failure_ack            MPIDI_Comm_failure_ack
#define MPID_Comm_AS_enabled             MPIDI_Comm_AS_enabled
#define MPID_Comm_get_lpid               MPIDI_Comm_get_lpid
#define MPID_Compare_and_swap            MPIDI_Compare_and_swap
#define MPID_Fetch_and_op                MPIDI_Fetch_and_op
#define MPID_Finalize                    MPIDI_Finalize
#define MPID_Free_mem                    MPIDI_Free_mem
#define MPID_Get                         MPIDI_Get
#define MPID_Get_accumulate              MPIDI_Get_accumulate
#define MPID_Get_processor_name          MPIDI_Get_processor_name
#define MPID_Get_universe_size           MPIDI_Get_universe_size
#define MPID_Improbe                     MPIDI_Improbe
#define MPID_Imrecv                      MPIDI_Imrecv
#define MPID_Init                        MPIDI_Init
#define MPID_InitCompleted               MPIDI_InitCompleted
#define MPID_Iprobe                      MPIDI_Iprobe
#define MPID_Irecv                       MPIDI_Irecv
#define MPID_Irsend                      MPIDI_Irsend
#define MPID_Isend                       MPIDI_Isend
#define MPID_Issend                      MPIDI_Issend
#define MPID_Mprobe                      MPIDI_Mprobe
#define MPID_Mrecv                       MPIDI_Mrecv
#define MPID_Open_port                   MPIDI_Open_port
#define MPID_Probe                       MPIDI_Probe
#define MPID_Progress_end                MPIDI_Progress_end
#define MPID_Progress_poke               MPIDI_Progress_poke
#define MPID_Progress_start              MPIDI_Progress_start
#define MPID_Progress_test               MPIDI_Progress_test
#define MPID_Progress_wait               MPIDI_Progress_wait
#define MPID_Progress_register           MPIDI_Progress_register
#define MPID_Progress_deregister         MPIDI_Progress_deregister
#define MPID_Progress_activate           MPIDI_Progress_activate
#define MPID_Progress_deactivate         MPIDI_Progress_deactivate
#define MPID_Put                         MPIDI_Put
#define MPID_Raccumulate                 MPIDI_Raccumulate
#define MPID_Recv                        MPIDI_Recv
#define MPID_Recv_init                   MPIDI_Recv_init
#define MPID_Request_release             MPIDI_Request_release
#define MPID_Request_complete            MPIDI_Request_complete
#define MPID_Request_is_anysource        MPIDI_Request_is_anysource
#define MPID_Request_set_completed       MPIDI_Request_set_completed
#define MPID_Rget                        MPIDI_Rget
#define MPID_Rget_accumulate             MPIDI_Rget_accumulate
#define MPID_Rput                        MPIDI_Rput
#define MPID_Rsend                       MPIDI_Rsend
#define MPID_Rsend_init                  MPIDI_Rsend_init
#define MPID_Send                        MPIDI_Send
#define MPID_Send_init                   MPIDI_Send_init
#define MPID_Ssend                       MPIDI_Ssend
#define MPID_Ssend_init                  MPIDI_Ssend_init
#define MPID_Startall                    MPIDI_Startall
#define MPID_Win_allocate                MPIDI_Win_allocate
#define MPID_Win_allocate_shared         MPIDI_Win_allocate_shared
#define MPID_Win_attach                  MPIDI_Win_attach
#define MPID_Win_complete                MPIDI_Win_complete
#define MPID_Win_create                  MPIDI_Win_create
#define MPID_Win_create_dynamic          MPIDI_Win_create_dynamic
#define MPID_Win_detach                  MPIDI_Win_detach
#define MPID_Win_fence                   MPIDI_Win_fence
#define MPID_Win_flush                   MPIDI_Win_flush
#define MPID_Win_flush_all               MPIDI_Win_flush_all
#define MPID_Win_flush_local             MPIDI_Win_flush_local
#define MPID_Win_flush_local_all         MPIDI_Win_flush_local_all
#define MPID_Win_free                    MPIDI_Win_free
#define MPID_Win_get_info                MPIDI_Win_get_info
#define MPID_Win_lock                    MPIDI_Win_lock
#define MPID_Win_lock_all                MPIDI_Win_lock_all
#define MPID_Win_post                    MPIDI_Win_post
#define MPID_Win_set_info                MPIDI_Win_set_info
#define MPID_Win_shared_query            MPIDI_Win_shared_query
#define MPID_Win_start                   MPIDI_Win_start
#define MPID_Win_sync                    MPIDI_Win_sync
#define MPID_Win_test                    MPIDI_Win_test
#define MPID_Win_unlock                  MPIDI_Win_unlock
#define MPID_Win_unlock_all              MPIDI_Win_unlock_all
#define MPID_Win_wait                    MPIDI_Win_wait
#define MPID_Get_node_id                 MPIDI_Get_node_id
#define MPID_Get_max_node_id             MPIDI_Get_max_node_id
#define MPID_Request_is_pending_failure  MPIDI_Request_is_pending_failure
#define MPID_Aint_add                    MPIDI_Aint_add
#define MPID_Aint_diff                   MPIDI_Aint_diff
#define MPID_Intercomm_exchange_map      MPIDI_Intercomm_exchange_map
#define MPID_Create_intercomm_from_lpids MPIDI_Create_intercomm_from_lpids
/* Variables */
#define MPID_Barrier                     MPIDI_Barrier
#define MPID_Bcast                       MPIDI_Bcast
#define MPID_Allreduce                   MPIDI_Allreduce
#define MPID_Allgather                   MPIDI_Allgather
#define MPID_Allgatherv                  MPIDI_Allgatherv
#define MPID_Scatter                     MPIDI_Scatter
#define MPID_Scatterv                    MPIDI_Scatterv
#define MPID_Gather                      MPIDI_Gather
#define MPID_Gatherv                     MPIDI_Gatherv
#define MPID_Alltoall                    MPIDI_Alltoall
#define MPID_Alltoallv                   MPIDI_Alltoallv
#define MPID_Alltoallw                   MPIDI_Alltoallw
#define MPID_Reduce                      MPIDI_Reduce
#define MPID_Reduce_scatter              MPIDI_Reduce_scatter
#define MPID_Reduce_scatter_block        MPIDI_Reduce_scatter_block
#define MPID_Scan                        MPIDI_Scan
#define MPID_Exscan                      MPIDI_Exscan
#define MPID_Neighbor_allgather          MPIDI_Neighbor_allgather
#define MPID_Neighbor_allgatherv         MPIDI_Neighbor_allgatherv
#define MPID_Neighbor_alltoallv          MPIDI_Neighbor_alltoallv
#define MPID_Neighbor_alltoallw          MPIDI_Neighbor_alltoallw
#define MPID_Neighbor_alltoall           MPIDI_Neighbor_alltoall
#define MPID_Ineighbor_allgather         MPIDI_Ineighbor_allgather
#define MPID_Ineighbor_allgatherv        MPIDI_Ineighbor_allgatherv
#define MPID_Ineighbor_alltoall          MPIDI_Ineighbor_alltoall
#define MPID_Ineighbor_alltoallv         MPIDI_Ineighbor_alltoallv
#define MPID_Ineighbor_alltoallw         MPIDI_Ineighbor_alltoallw
#define MPID_Ibarrier                    MPIDI_Ibarrier
#define MPID_Ibcast                      MPIDI_Ibcast
#define MPID_Iallgather                  MPIDI_Iallgather
#define MPID_Iallgatherv                 MPIDI_Iallgatherv
#define MPID_Iallreduce                  MPIDI_Iallreduce
#define MPID_Ialltoall                   MPIDI_Ialltoall
#define MPID_Ialltoallv                  MPIDI_Ialltoallv
#define MPID_Ialltoallw                  MPIDI_Ialltoallw
#define MPID_Iexscan                     MPIDI_Iexscan
#define MPID_Igather                     MPIDI_Igather
#define MPID_Igatherv                    MPIDI_Igatherv
#define MPID_Ireduce_scatter_block       MPIDI_Ireduce_scatter_block
#define MPID_Ireduce_scatter             MPIDI_Ireduce_scatter
#define MPID_Ireduce                     MPIDI_Ireduce
#define MPID_Iscan                       MPIDI_Iscan
#define MPID_Iscatter                    MPIDI_Iscatter
#define MPID_Iscatterv                   MPIDI_Iscatterv

#define MPIDI_MAX_NETMOD_STRING_LEN 64
extern int MPIDI_num_netmods;
#if defined(MPL_USE_DBG_LOGGING)
extern MPL_dbg_class MPIDI_CH4_DBG_GENERAL;
extern MPL_dbg_class MPIDI_CH4_DBG_MAP;
extern MPL_dbg_class MPIDI_CH4_DBG_COMM;
extern MPL_dbg_class MPIDI_CH4_DBG_MEMORY;
#endif /* MPL_USE_DBG_LOGGING */




#endif /* MPIDCH4_H_INCLUDED */
