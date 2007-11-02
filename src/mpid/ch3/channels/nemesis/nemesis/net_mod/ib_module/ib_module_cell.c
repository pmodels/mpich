/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define _GNU_SOURCE
#include "mpidimpl.h"
#include "ib_module.h"
#include "ib_device.h"
#include "ib_utils.h"
#include "ib_module_cm.h"
#include "ib_module_priv.h"
#include "ib_module_impl.h"

static MPID_nem_ib_module_queue_ptr_t alloc_cells_queue;

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_add_cells
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_module_add_cells(int n)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPID_nem_ib_module_cell_pool_t *pool;
    MPID_nem_ib_module_cell_elem_t *ce;
    MPID_nem_ib_module_queue_elem_t *qe;
    MPID_nem_ib_module_queue_elem_t *cell_ptr_elem;

    pool = &MPID_nem_ib_module_cell_pool;

    ce = MPIU_Malloc(sizeof(MPID_nem_ib_module_cell_elem_t) * n);

    if(NULL == ce) {
        MPIU_CHKMEM_SETERR(mpi_errno, 
                sizeof(MPID_nem_ib_module_cell_elem_t) * n, 
                "IB Module Cell Elements");
    }

    memset(ce, 0, sizeof(MPID_nem_ib_module_cell_elem_t) * n);

    MPID_nem_ib_module_queue_alloc(alloc_cells_queue, &cell_ptr_elem);

    cell_ptr_elem->data = ce;

    MPID_nem_ib_module_queue_enqueue(alloc_cells_queue, cell_ptr_elem);

    /* Allocate the queue elements and enqueue */

    for(i = 0; i < n; i++) {

        mpi_errno = MPID_nem_ib_module_queue_new_elem(&qe, &ce[i]);

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        MPID_nem_ib_module_queue_enqueue(pool->queue, qe);

        /* Record the Queue element pointer */
        ce[i].qe = qe;
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_init_cell_pool
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_module_init_cell_pool(int n)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ib_module_cell_pool_t *pool;

    pool = &MPID_nem_ib_module_cell_pool;

    MPID_nem_ib_module_queue_init(&pool->queue);

    MPID_nem_ib_module_queue_init(&alloc_cells_queue);

    mpi_errno = MPID_nem_ib_module_add_cells(n);

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    pthread_spin_init(&pool->lock, 0);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_finalize_cell_pool
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

void MPID_nem_ib_module_finalize_cell_pool()
{
    MPID_nem_ib_module_queue_elem_t *e;
    MPID_nem_ib_module_cell_elem_t *ce;

    while(!MPID_nem_ib_module_queue_empty(
                MPID_nem_ib_module_cell_pool.queue)) {

        MPID_nem_ib_module_queue_dequeue(
                MPID_nem_ib_module_cell_pool.queue, &e);

        MPIU_Free(e);
    }

    while(!MPID_nem_ib_module_queue_empty(
                alloc_cells_queue)) {

        MPID_nem_ib_module_queue_dequeue(
                alloc_cells_queue, &e);

        MPIU_Free(e->data);

        MPIU_Free(e);
    }
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_get_cell
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_module_get_cell(
        MPID_nem_ib_module_cell_elem_t **e)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ib_module_queue_elem_t *qe;

    pthread_spin_lock(&MPID_nem_ib_module_cell_pool.lock);

    if(!MPID_nem_ib_module_queue_empty(
                MPID_nem_ib_module_cell_pool.queue)) {

        MPID_nem_ib_module_queue_dequeue(
                MPID_nem_ib_module_cell_pool.queue,
                &qe);

        *e = qe->data;

        MPIU_Assert(NULL != (*e));

        (*e)->vc = NULL;

    } else {

        NEM_IB_DBG("Ran out of cells, allocating new ones");

        mpi_errno = MPID_nem_ib_module_add_cells(
                MPID_nem_ib_dev_param_ptr->sec_pool_size);

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        /* Atleast one cell is available now! */

        MPID_nem_ib_module_queue_dequeue(
                MPID_nem_ib_module_cell_pool.queue,
                &qe);

        *e = qe->data;

        (*e)->vc = NULL;

        MPIU_Assert(NULL != (*e));
    }

    pthread_spin_unlock(&MPID_nem_ib_module_cell_pool.lock);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_return_cell
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

void MPID_nem_ib_module_return_cell(
        MPID_nem_ib_module_cell_elem_t *ce)
{
    pthread_spin_lock(&MPID_nem_ib_module_cell_pool.lock);

    ce->vc = NULL;
    ce->nem_cell = NULL;

    MPID_nem_ib_module_queue_enqueue(
            MPID_nem_ib_module_cell_pool.queue,
            ce->qe);

    pthread_spin_unlock(&MPID_nem_ib_module_cell_pool.lock);
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_prep_cell_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

void MPID_nem_ib_module_prep_cell_recv(
        MPID_nem_ib_module_cell_elem_t *ce,
        void* buf)
{
    ce->desc.sg_list.addr = (uintptr_t) buf;
    ce->desc.sg_list.length = MPID_NEM_MAX_PACKET_LEN;
    ce->desc.sg_list.lkey = module_elements_mr->lkey;

    ce->desc.u.r_wr.next = NULL;
    ce->desc.u.r_wr.wr_id = (uint64_t) ce;
    ce->desc.u.r_wr.sg_list = &ce->desc.sg_list;
    ce->desc.u.r_wr.num_sge = 1;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_prep_cell_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

void MPID_nem_ib_module_prep_cell_send(
        MPID_nem_ib_module_cell_elem_t *ce,
        void* buf, uint32_t len)
{
    ce->desc.sg_list.addr = (uintptr_t) buf;
    ce->desc.sg_list.length = len;
    ce->desc.sg_list.lkey = proc_elements_mr->lkey;

    ce->desc.u.s_wr.next = NULL;
    ce->desc.u.s_wr.wr_id = (uint64_t) ce;
    ce->desc.u.s_wr.sg_list = &ce->desc.sg_list;
    ce->desc.u.s_wr.num_sge = 1;

    ce->desc.u.s_wr.opcode = IBV_WR_SEND;

    if(len < MPID_nem_ib_dev_param_ptr->max_inline_size) {
        ce->desc.u.s_wr.send_flags = IBV_SEND_SIGNALED | 
            IBV_SEND_INLINE;
    } else {
        ce->desc.u.s_wr.send_flags = IBV_SEND_SIGNALED;
    }
}
