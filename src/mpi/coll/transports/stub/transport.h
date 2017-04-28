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

static inline int MPIC_STUB_init()
{
    return 0;
}

static inline int MPIC_STUB_comm_init(MPIC_STUB_comm_t * comm, void * base)
{
    return 0;
}

static inline int MPIC_STUB_comm_cleanup(MPIC_STUB_comm_t * comm)
{
    return 0;
}


static inline int MPIC_STUB_sched_init(MPIC_STUB_sched_t * sched, int tag)
{
    return 0;
}

static inline int MPIC_STUB_sched_commit(MPIC_STUB_sched_t * sched)
{
    return 0;
}

static inline int MPIC_STUB_sched_start(MPIC_STUB_sched_t * sched)
{
    return 0;
}

static inline int MPIC_STUB_sched_finalize(MPIC_STUB_sched_t * sched)
{
    return 0;
}

static inline int MPIC_STUB_init_control_dt(MPIC_STUB_dt_t dt)
{
    return 0;
}


static inline void MPIC_STUB_opinfo(MPIC_STUB_op_t op, int *is_commutative)
{
}

static inline int MPIC_STUB_isinplace(void *buf)
{
    return 0;
}

static inline void MPIC_STUB_dtinfo(MPIC_STUB_dt_t dt,
                              int *iscontig,
                              size_t * size, size_t * out_extent, size_t * lower_bound)
{
}

static inline int MPIC_STUB_fence(MPIC_STUB_sched_t * sched)
{

}

static inline int MPIC_STUB_wait(MPIC_STUB_sched_t * sched)
{
}

static inline void MPIC_STUB_addref_dt(MPIC_STUB_dt_t dt, int up)
{

}

static inline void MPIC_STUB_addref_op(MPIC_STUB_op_t op, int up)
{
}

static inline int MPIC_STUB_addref_dt_nb(MPIC_STUB_dt_t dt,
                                   int up, MPIC_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int MPIC_STUB_addref_op_nb(MPIC_STUB_op_t op,
                                   int up, MPIC_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int MPIC_STUB_send(const void *buf,
                           int count,
                           MPIC_STUB_dt_t dt,
                           int dest,
                           int tag,
                           MPIC_STUB_comm_t * comm_ptr, MPIC_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int MPIC_STUB_send_accumulate(const void *buf,
                                      int count,
                                      MPIC_STUB_dt_t dt,
                                      MPIC_STUB_op_t op,
                                      int dest,
                                      int tag,
                                      MPIC_STUB_comm_t * comm_ptr, MPIC_STUB_sched_t * sched,
                                      int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int MPIC_STUB_recv(void *buf,
                           int count,
                           MPIC_STUB_dt_t datatype,
                           int source,
                           int tag,
                           MPIC_STUB_comm_t * comm_ptr, MPIC_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int MPIC_STUB_recv_reduce(void *buf,
                                  int count,
                                  MPIC_STUB_dt_t datatype,
                                  MPIC_STUB_op_t op,
                                  int source,
                                  int tag,
                                  MPIC_STUB_comm_t * comm_ptr, uint64_t flags, MPIC_STUB_sched_t * sched,
                                  int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int MPIC_STUB_test(MPIC_STUB_sched_t * sched)
{
    return 0;
}

static inline int MPIC_STUB_rank(MPIC_STUB_comm_t * comm_ptr)
{
    return 0;
}

static inline int MPIC_STUB_size(MPIC_STUB_comm_t * comm_ptr)
{
    return 0;
}

static inline int MPIC_STUB_reduce_local(const void *inbuf,
                                   void *inoutbuf,
                                   int count,
                                   MPIC_STUB_dt_t datatype, MPIC_STUB_op_t operation, MPIC_STUB_sched_t * sched, int nvtcs, int *vtcs)
{
    return 0;
}

static inline int MPIC_STUB_dtcopy_nb(void *tobuf,
                                int tocount,
                                MPIC_STUB_dt_t totype,
                                const void *frombuf,
                                int fromcount,
                                MPIC_STUB_dt_t fromtype, MPIC_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

static inline int MPIC_STUB_dtcopy(void *tobuf,
                             int tocount,
                             MPIC_STUB_dt_t totype,
                             const void *frombuf, int fromcount, MPIC_STUB_dt_t fromtype)
{
    return 0;
}

static inline void *MPIC_STUB_allocate_mem(size_t size)
{
    return NULL;
}

static inline void* MPIC_STUB_allocate_buffer(size_t size, MPIC_STUB_sched_t *s){
    return NULL;
}

static inline void MPIC_STUB_free_mem(void *ptr)
{
}

static inline void MPIC_STUB_free_buffers(MPIC_STUB_sched_t *s){
}

static inline int MPIC_STUB_free_mem_nb(void *ptr, MPIC_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}
#endif
