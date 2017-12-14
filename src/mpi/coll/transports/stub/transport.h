/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* refer to ../mpich/transport.h for documentation */
#ifndef STUBTRANSPORT_H_INCLUDED
#define STUBTRANSPORT_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_init()
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_comm_init(MPIR_COLL_STUB_comm_t * comm, void *base)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_comm_init_null(MPIR_COLL_STUB_comm_t * comm, void *base)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_comm_cleanup(MPIR_COLL_STUB_comm_t * comm)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX void MPIR_COLL_STUB_opinfo(MPIR_COLL_STUB_op_t op, int *is_commutative)
{
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_isinplace(void *buf)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX void MPIR_COLL_STUB_dtinfo(MPIR_COLL_STUB_dt_t dt, int *iscontig, size_t * size,
                                  size_t * out_extent, size_t * lower_bound)
{
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_fence(MPIR_COLL_STUB_sched_t * sched)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_wait(MPIR_COLL_STUB_sched_t * sched)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_wait_for(MPIR_COLL_STUB_sched_t * sched, int nvtcs, int *vtcs)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX void MPIR_COLL_STUB_addref_dt(MPIR_COLL_STUB_dt_t dt, int up)
{

}

MPL_STATIC_INLINE_PREFIX void MPIR_COLL_STUB_addref_op(MPIR_COLL_STUB_op_t op, int up)
{
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_addref_dt_nb(MPIR_COLL_STUB_dt_t dt, int up, MPIR_COLL_STUB_sched_t * sched,
                                       int n_invtcs, int *invtcs)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_addref_op_nb(MPIR_COLL_STUB_op_t op, int up, MPIR_COLL_STUB_sched_t * sched,
                                       int n_invtcs, int *invtcs)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_send(const void *buf, int count, MPIR_COLL_STUB_dt_t dt, int dest, int tag,
                               MPIR_COLL_STUB_comm_t * comm_ptr, MPIR_COLL_STUB_sched_t * sched,
                               int n_invtcs, int *invtcs)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_multicast(const void *buf, int count, MPIR_COLL_STUB_dt_t dt, int* destinations,
                               int num_destinations, int tag, MPIR_COLL_STUB_comm_t * comm_ptr,
                               MPIR_COLL_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_send_accumulate(const void *buf, int count, MPIR_COLL_STUB_dt_t dt,
                                          MPIR_COLL_STUB_op_t op, int dest, int tag,
                                          MPIR_COLL_STUB_comm_t * comm_ptr, MPIR_COLL_STUB_sched_t * sched,
                                          int n_invtcs, int *invtcs)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_recv(void *buf, int count, MPIR_COLL_STUB_dt_t datatype, int source, int tag,
                               MPIR_COLL_STUB_comm_t * comm_ptr, MPIR_COLL_STUB_sched_t * sched,
                               int n_invtcs, int *invtcs)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_recv_reduce(void *buf, int count, MPIR_COLL_STUB_dt_t datatype,
                                      MPIR_COLL_STUB_op_t op, int source, int tag,
                                      MPIR_COLL_STUB_comm_t * comm_ptr, uint64_t flags,
                                      MPIR_COLL_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_test(MPIR_COLL_STUB_sched_t * sched)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_rank(MPIR_COLL_STUB_comm_t * comm_ptr)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_size(MPIR_COLL_STUB_comm_t * comm_ptr)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_reduce_local(const void *inbuf, void *inoutbuf, int count,
                                       MPIR_COLL_STUB_dt_t datatype, MPIR_COLL_STUB_op_t operation,
                                       uint64_t flags, MPIR_COLL_STUB_sched_t * sched, int nvtcs,
                                       int *vtcs)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_dtcopy_nb(void *tobuf, int tocount, MPIR_COLL_STUB_dt_t totype,
                                    const void *frombuf, int fromcount, MPIR_COLL_STUB_dt_t fromtype,
                                    MPIR_COLL_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_dtcopy(void *tobuf, int tocount, MPIR_COLL_STUB_dt_t totype,
                                 const void *frombuf, int fromcount, MPIR_COLL_STUB_dt_t fromtype)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX void *MPIR_COLL_STUB_allocate_mem(size_t size)
{
    return NULL;
}

MPL_STATIC_INLINE_PREFIX void MPIR_COLL_STUB_free_mem(void *ptr)
{
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_free_mem_nb(void *ptr, MPIR_COLL_STUB_sched_t * sched, int n_invtcs,
                                      int *invtcs)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX MPIR_COLL_STUB_sched_t *MPIR_COLL_STUB_get_schedule(MPIR_COLL_STUB_comm_t * comm_ptr, void *key,
                                                      int key_len, int tag, int *is_new)
{
    *is_new = 1;
    return NULL;
}

MPL_STATIC_INLINE_PREFIX void MPIR_COLL_STUB_save_schedule(MPIR_COLL_STUB_comm_t * comm_ptr, void *key,
                                         int key_len, MPIR_COLL_STUB_sched_t * s)
{
}


MPL_STATIC_INLINE_PREFIX void *MPIR_COLL_STUB_allocate_buffer(size_t size, MPIR_COLL_STUB_sched_t * s)
{
    return NULL;
}

MPL_STATIC_INLINE_PREFIX void MPIR_COLL_STUB_free_buffers(MPIR_COLL_STUB_sched_t * s)
{
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_sched_commit(MPIR_COLL_STUB_sched_t * sched)
{
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_COLL_STUB_sched_finalize(MPIR_COLL_STUB_sched_t * sched)
{
    return 0;
}

#endif
