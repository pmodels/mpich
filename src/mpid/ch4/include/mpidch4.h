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
  MPL_STATIC_INLINE_PREFIX rc MPID_##fcnname(__VA_ARGS__) MPL_STATIC_INLINE_SUFFIX

MPIDI_CH4I_API(int, Init, int *, char ***, int, int *, int *, int *);
MPIDI_CH4I_API(int, InitCompleted, void);
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
MPIDI_CH4I_API(int, Intercomm_exchange_map, MPIR_Comm *, int, MPIR_Comm *, int, int *, int **,
               int *);
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

int MPID_Abort(struct MPIR_Comm *comm, int mpi_errno, int exit_code, const char *error_msg);

/* This function is not exposed to the upper layers but functions in a way
 * similar to the functions above. Other CH4-level functions should call this
 * function to query locality. This function will determine whether to call the
 * netmod or CH4U locality functions. */
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_rank_is_local(int rank, MPIR_Comm * comm);
MPL_STATIC_INLINE_PREFIX int MPIDI_av_is_local(MPIDI_av_entry_t *av);

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

#define MPIDI_MAX_NETMOD_STRING_LEN 64
extern int MPIDI_num_netmods;
#ifdef MPL_USE_DBG_LOGGING
extern MPL_dbg_class MPIDI_CH4_DBG_GENERAL;
extern MPL_dbg_class MPIDI_CH4_DBG_MAP;
extern MPL_dbg_class MPIDI_CH4_DBG_COMM;
extern MPL_dbg_class MPIDI_CH4_DBG_MEMORY;
#endif /* MPL_USE_DBG_LOGGING */




#endif /* MPIDCH4_H_INCLUDED */
