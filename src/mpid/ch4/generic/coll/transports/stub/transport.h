/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef STUBTRANSPORT_H_INCLUDED
#define STUBTRANSPORT_H_INCLUDED

#include "../../src/tsp_namespace_pre.h"

#ifndef TSP_NAMESPACE
#error "TSP_NAMESPACE must be defined before including a collective transport"
#endif

static inline int TSP_init()
{
    return 0;
}

static inline int TSP_op_init(TSP_op_t * op, void * base)
{
    return 0;
}

static inline int TSP_dt_init(TSP_dt_t * dt, void * base)
{
    return 0;
}

static inline int TSP_comm_init(TSP_comm_t * comm, void * base)
{
    return 0;
}

static inline int TSP_comm_cleanup(TSP_comm_t * comm)
{
    return 0;
}


static inline int TSP_sched_init(TSP_sched_t * sched)
{
    return 0;
}

static inline int TSP_sched_commit(TSP_sched_t * sched)
{
    return 0;
}

static inline int TSP_sched_start(TSP_sched_t * sched)
{
    return 0;
}

static inline int TSP_sched_finalize(TSP_sched_t * sched)
{
    return 0;
}

static inline int TSP_init_control_dt(TSP_dt_t * dt)
{
    return 0;
}


static inline void TSP_opinfo(TSP_op_t * op, int *is_commutative)
{
}

static inline int TSP_isinplace(void *buf)
{
    return 0;
}

static inline void TSP_dtinfo(TSP_dt_t * dt,
                              int *iscontig,
                              size_t * size, size_t * out_extent, size_t * lower_bound)
{
}

static inline int TSP_fence(TSP_sched_t * sched)
{

}

static inline int TSP_wait(TSP_sched_t * sched)
{
}

static inline int TSP_wait4(TSP_sched_t * sched, int n_invtcs, int *invtcs)
{
}

static inline void TSP_addref_dt(TSP_dt_t * dt, int up)
{

}

static inline void TSP_addref_op(TSP_op_t * op, int up)
{
}

static inline int TSP_addref_dt_nb(TSP_dt_t * dt,
                                   int up, TSP_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int TSP_addref_op_nb(TSP_op_t * op,
                                   int up, TSP_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int TSP_send(const void *buf,
                           int count,
                           TSP_dt_t * dt,
                           int dest,
                           int tag,
                           TSP_comm_t * comm_ptr, TSP_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int TSP_send_accumulate(const void *buf,
                                      int count,
                                      TSP_dt_t * dt,
                                      TSP_op_t * op,
                                      int dest,
                                      int tag,
                                      TSP_comm_t * comm_ptr, TSP_sched_t * sched,
                                      int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int TSP_recv(void *buf,
                           int count,
                           TSP_dt_t * datatype,
                           int source,
                           int tag,
                           TSP_comm_t * comm_ptr, TSP_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int TSP_recv_reduce(void *buf,
                                  int count,
                                  TSP_dt_t * datatype,
                                  TSP_op_t * op,
                                  int source,
                                  int tag,
                                  TSP_comm_t * comm_ptr, uint64_t flags, TSP_sched_t * sched,
                                  int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int TSP_test(TSP_sched_t * sched)
{
    return 0;
}

static inline int TSP_rank(TSP_comm_t * comm_ptr)
{
    return 0;
}

static inline int TSP_size(TSP_comm_t * comm_ptr)
{
    return 0;
}

static inline int TSP_reduce_local(const void *inbuf,
                                   void *inoutbuf,
                                   int count,
                                   TSP_dt_t * datatype, TSP_op_t * operation, TSP_sched_t * sched)
{
    return 0;
}

static inline int TSP_dtcopy_nb(void *tobuf,
                                int tocount,
                                TSP_dt_t * totype,
                                const void *frombuf,
                                int fromcount,
                                TSP_dt_t * fromtype, TSP_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int TSP_dtcopy(void *tobuf,
                             int tocount,
                             TSP_dt_t * totype,
                             const void *frombuf, int fromcount, TSP_dt_t * fromtype)
{
    return 0;
}

static inline void *TSP_allocate_mem(size_t size)
{
    return NULL;
}

static inline void* TSP_allocate_buffer(size_t size, TSP_sched_t *s){
    return NULL;
}

static inline void TSP_free_mem(void *ptr)
{
}

static inline void TSP_free_buffers(TSP_sched_t *s){
}

static inline int TSP_free_mem_nb(void *ptr, TSP_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

#endif
