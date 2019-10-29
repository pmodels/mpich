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

#ifndef PIP_IMPL_H_INCLUDED
#define PIP_IMPL_H_INCLUDED

#include "pip_pre.h"
#include "../posix/posix_impl.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_Task_safe_enqueue(MPIDI_PIP_task_queue_t * task_queue,
                                                          MPIDI_PIP_task_t * task);
MPL_STATIC_INLINE_PREFIX int MPIDI_PIP_Compl_task_enqueue(MPIDI_PIP_task_queue_t * task_queue,
                                                          MPIDI_PIP_task_t * task);
MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_fflush_compl_task(MPIDI_PIP_task_queue_t * compl_queue);

MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_create_tasks(MPIR_Request * req)
{

    /* only enqueue tasks instead of doing it */
    MPIDI_POSIX_cell_ptr_t cell = NULL;
    size_t exit_thsd = MPIDI_POSIX_EAGER_THRESHOLD_32KB << 1;
    size_t data_sz = MPIDI_POSIX_REQUEST(req)->data_sz;
    size_t addr_offset = MPIDI_POSIX_REQUEST(req)->addr_offset;
    size_t sz_thsd;
    MPIDI_PIP_task_t *task = NULL;
    int grank = MPIDI_CH4U_rank_to_lpid(MPIDI_POSIX_REQUEST(req)->dest, req->comm);
    while (1) {
        if (data_sz > exit_thsd && !MPIDI_POSIX_queue_empty(MPIDI_POSIX_mem_region.my_freeQ)) {
            MPIDI_POSIX_queue_dequeue(MPIDI_POSIX_mem_region.my_freeQ, &cell);
            MPIDI_POSIX_ENVELOPE_GET(MPIDI_POSIX_REQUEST(req), cell->rank, cell->tag,
                                     cell->context_id);
            cell->pending = NULL;
            char *recv_buffer = (char *) cell->pkt.mpich.p.payload;
            if (data_sz < MPIDI_POSIX_CELL_SWITCH_THRESHOLD) {
                /* 32KB cell size */
                sz_thsd = MPIDI_POSIX_EAGER_THRESHOLD_32KB;
            } else {
                /* 64KB cell size */
                sz_thsd = MPIDI_POSIX_EAGER_THRESHOLD;
            }
            cell->addr_offset = addr_offset;
            cell->pkt.mpich.datalen = sz_thsd;

            data_sz -= sz_thsd;
            addr_offset += sz_thsd;
            /* need more LMT send calls */
            cell->pkt.mpich.type = MPIDI_POSIX_TYPELMT;
            task = (MPIDI_PIP_task_t *) MPIR_Handle_obj_alloc(&MPIDI_Task_mem);
            task->cell = cell;

            task->compl_flag = 0;
            task->asym_addr = (MPI_Aint) MPIDI_POSIX_asym_base_addr;

            task->send_flag = 1;
            task->next = NULL;
            task->compl_next = NULL;
            task->unexp_req = NULL;
            task->local_rank = pip_global.local_rank;
            task->data_sz = sz_thsd;

            // task->cur_task_id = pip_global.shm_send_counter + dest_local;
            // task->task_id = pip_global.local_send_counter[dest_local]++;
            task->cell_queue = MPIDI_POSIX_mem_region.RecvQ[grank];
            if (MPIDI_POSIX_REQUEST(req)->segment_ptr) {
                /* non-contig */
                task->segp = (DLOOP_Segment *) MPIR_Handle_obj_alloc(&MPIDI_Segment_mem);
                task->segment_first = MPIDI_POSIX_REQUEST(req)->segment_first;
                MPIR_Memcpy(task->segp, MPIDI_POSIX_REQUEST(req)->segment_ptr,
                            sizeof(DLOOP_Segment));
                // MPIR_Segment_manipulate(MPIDI_POSIX_REQUEST(req)->segment_ptr, MPIDI_POSIX_REQUEST(req)->segment_first, &last, NULL,      /* contig fn */
                // NULL,       /* vector fn */
                // NULL,       /* blkidx fn */
                // NULL,       /* index fn */
                // NULL, NULL);
                MPIDI_POSIX_REQUEST(req)->segment_first =
                    MPIDI_POSIX_REQUEST(req)->segment_first + sz_thsd;

                task->src = NULL;
                task->dest = recv_buffer;
            } else {
                task->segp = NULL;
                task->src = MPIDI_POSIX_REQUEST(req)->user_buf;
                task->dest = recv_buffer;
                MPIDI_POSIX_REQUEST(req)->user_buf += sz_thsd;
            }
            MPIDI_PIP_Task_safe_enqueue(&pip_global.task_queue[cell->socket_id], task);
            MPIDI_PIP_Compl_task_enqueue(pip_global.local_compl_queue, task);

            if (pip_global.local_compl_queue->task_num >= MPIDI_MAX_TASK_THREASHOLD) {
                MPIDI_PIP_fflush_compl_task(pip_global.local_compl_queue);
            }
        } else
            break;
    }
    MPIDI_POSIX_REQUEST(req)->data_sz = data_sz;
    MPIDI_POSIX_REQUEST(req)->addr_offset = addr_offset;
    return;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_Task_safe_enqueue(MPIDI_PIP_task_queue_t * task_queue,
                                                          MPIDI_PIP_task_t * task)
{
    int err;
    MPID_Thread_mutex_lock(&task_queue->lock, &err);
    if (task_queue->tail) {
        task_queue->tail->next = task;
        task_queue->tail = task;
    } else {
        task_queue->head = task_queue->tail = task;
    }
    task_queue->task_num++;
    MPID_Thread_mutex_unlock(&task_queue->lock, &err);
    return;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_Task_safe_dequeue(MPIDI_PIP_task_queue_t * task_queue,
                                                          MPIDI_PIP_task_t ** task)
{
    int err;

    MPIDI_PIP_task_t *old_head;
    MPID_Thread_mutex_lock(&task_queue->lock, &err);
    old_head = task_queue->head;
    if (old_head) {
        task_queue->head = old_head->next;
        if (task_queue->head == NULL)
            task_queue->tail = NULL;
        task_queue->task_num--;
    }
    MPID_Thread_mutex_unlock(&task_queue->lock, &err);

    *task = old_head;

    return;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_PIP_Compl_task_enqueue(MPIDI_PIP_task_queue_t * task_queue,
                                                          MPIDI_PIP_task_t * task)
{
    int mpi_errno = MPI_SUCCESS, err;

    if (task_queue->tail) {
        task_queue->tail->compl_next = task;
        task_queue->tail = task;
    } else {
        task_queue->head = task_queue->tail = task;
    }
    task_queue->task_num++;

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_PIP_Compl_task_delete_head(MPIDI_PIP_task_queue_t * task_queue)
{
    int mpi_errno = MPI_SUCCESS, err;

    MPIDI_PIP_task_t *old_head = task_queue->head;
    if (old_head) {
        task_queue->head = old_head->compl_next;
        if (task_queue->head == NULL)
            task_queue->tail = NULL;
        task_queue->task_num--;
    }
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_fflush_compl_task(MPIDI_PIP_task_queue_t * compl_queue)
{
    MPIDI_PIP_task_t *task = compl_queue->head;
    while (task && task->compl_flag) {
        MPIDI_PIP_Compl_task_delete_head(compl_queue);
        if (task->segp)
            MPIR_Handle_obj_free(&MPIDI_Segment_mem, task->segp);
        if (task->unexp_req) {
            MPIR_Request *sreq = task->unexp_req;
            MPIDI_POSIX_REQUEST(sreq)->pending = NULL;
            MPL_free(MPIDI_POSIX_REQUEST(sreq)->user_buf);
            MPIDI_POSIX_REQUEST_COMPLETE(sreq);
        }
        MPIR_Handle_obj_free(&MPIDI_Task_mem, task);
        task = compl_queue->head;
    }
    return;
}


#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_PIP_do_task_copy)
MPL_STATIC_INLINE_PREFIX int MPIDI_PIP_do_task_copy(MPIDI_PIP_task_t * task)
{
    int mpi_errno = MPI_SUCCESS;
    // int task_id = task->task_id;
    // void *recv_buffer;
    // struct timespec start, end;
    // clock_gettime(CLOCK_MONOTONIC, &start);
    if (task->segp) {
        DLOOP_Segment *segp = task->segp;
        size_t last = task->segment_first + task->data_sz;

        segment_seek(segp, task->segment_first, NULL);
        if (task->send_flag) {
            MPIR_Segment_pack(segp, task->segment_first, (MPI_Aint *) & last, task->dest);
        } else {
            MPIR_Segment_unpack(segp, task->segment_first, (MPI_Aint *) & last, task->src);
        }
        /* non-contig */
    } else {
        /* contig */
        // if (task->send_flag == 0){
        //     printf("task->dest %p, task->src %p, task->data_sz %ld\n", task->dest, task->src, task->data_sz);
        //     fflush(stdout);
        // }
        MPIR_Memcpy(task->dest, task->src, task->data_sz);
    }

    pip_global.copy_size += task->data_sz;
    // task->next = NULL;
    MPIDI_POSIX_cell_ptr_t cell = task->cell;
    // __sync_sub_and_fetch(&pip_global.shm_pip_global[0]->cur_parallelism, 1);

    // if (task->send_flag) {
    //     // while (task_id != *task->cur_task_id);
    //     MPIDI_POSIX_PIP_queue_enqueue(task->cell_queue, cell, task->asym_addr);
    //     // *task->cur_task_id = task_id + 1;
    //     // OPA_write_barrier();
    // } else {
    if (cell)
        MPIDI_POSIX_PIP_queue_enqueue(task->cell_queue, cell, task->asym_addr);
    else
        OPA_write_barrier();
    // }

    // *task->cur_task_id = task_id + 1;
    // OPA_write_barrier();
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // double time = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
    // printf("rank %d - copy data from %d, size %ld, time %.3lfus\n", pip_global.local_rank, task->rank,
    //        task->data_sz, time);
    // fflush(stdout);
    task->compl_flag = 1;

    return mpi_errno;
}


#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_PIP_compl_one_task)
MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_compl_one_task(MPIDI_PIP_task_queue_t * compl_queue)
{
    MPIDI_PIP_task_t *task = compl_queue->head;
    if (task && task->compl_flag) {
        MPIDI_PIP_Compl_task_delete_head(compl_queue);
        if (task->segp)
            MPIR_Handle_obj_free(&MPIDI_Segment_mem, task->segp);
        if (task->unexp_req) {
            MPIR_Request *sreq = task->unexp_req;
            MPIDI_POSIX_REQUEST(sreq)->pending = NULL;
            MPL_free(MPIDI_POSIX_REQUEST(sreq)->user_buf);
            MPIDI_POSIX_REQUEST_COMPLETE(sreq);
        }
        MPIR_Handle_obj_free(&MPIDI_Task_mem, task);
    }
    return;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_PIP_exec_one_task)
MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_exec_one_task(MPIDI_PIP_task_queue_t * task_queue)
{
    MPIDI_PIP_task_t *task;

    if (task_queue->head) {
        MPIDI_PIP_Task_safe_dequeue(task_queue, &task);
        /* find my own task */
        if (task) {
            MPIDI_PIP_do_task_copy(task);
        }
    }
    return;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_PIP_fflush_task)
MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_exec_task(MPIDI_PIP_task_queue_t * task_queue)
{
    MPIDI_PIP_task_t *task;

    if (task_queue->head) {
        MPIDI_PIP_Task_safe_dequeue(task_queue, &task);
        /* find my own task */
        if (task) {
            MPIDI_PIP_do_task_copy(task);
            MPIDI_PIP_compl_one_task(pip_global.local_compl_queue);
        }
    }
    return;
}


#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_PIP_fflush_task)
MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_fflush_task()
{
    MPIDI_PIP_task_t *task;
    int i;
    for (i = 0; i < pip_global.numa_max_node; ++i) {
        while (pip_global.task_queue[i].head) {
            MPIDI_PIP_Task_safe_dequeue(&pip_global.task_queue[i], &task);
            /* find my own task */
            if (task) {
                MPIDI_PIP_do_task_copy(task);
            }
        }
    }
    return;
}



#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_PIP_do_ucx_task_copy)
MPL_STATIC_INLINE_PREFIX int MPIDI_PIP_do_ucx_task_copy(MPIDI_PIP_task_t * task)
{
    int mpi_errno = MPI_SUCCESS;
    // int task_id = task->task_id;
    // void *recv_buffer;
    // struct timespec start, end;
    // clock_gettime(CLOCK_MONOTONIC, &start);

    if (task->send_flag) {
        MPIR_Segment_pack(task->segp, task->segment_first, (MPI_Aint *) & task->last, task->dest);
    } else {
        MPIR_Segment_unpack(task->segp, task->segment_first, (MPI_Aint *) & task->last, task->src);
    }

    pip_global.copy_size += task->data_sz;

    OPA_write_barrier();

    task->compl_flag = 1;

    return mpi_errno;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_PIP_ucx_fflush_task)
MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_ucx_fflush_task(MPIDI_PIP_task_queue_t * task_queue)
{
    MPIDI_PIP_task_t *task;
    int i;
    while (task_queue->head) {
        MPIDI_PIP_Task_safe_dequeue(task_queue, &task);
        /* find my own task */
        if (task) {
            MPIDI_PIP_do_ucx_task_copy(task);
        }
    }
    return;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_PIP_exec_ucx_one_task)
MPL_STATIC_INLINE_PREFIX void MPIDI_PIP_exec_ucx_one_task(MPIDI_PIP_task_queue_t * task_queue)
{
    MPIDI_PIP_task_t *task;
    int i;
    if (task_queue->head) {
        MPIDI_PIP_Task_safe_dequeue(task_queue, &task);
        /* find my own task */
        if (task) {
            MPIDI_PIP_do_ucx_task_copy(task);
        }
    }
    return;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_PIP_steal_task)
MPL_STATIC_INLINE_PREFIX int MPIDI_PIP_steal_task()
{
    int victim = rand() % pip_global.num_local;
    MPIDI_PIP_task_t *task = NULL;
    if (victim != pip_global.local_rank) {
#ifdef MPI_PIP_SHM_TASK_STEAL
        MPIDI_PIP_task_queue_t *victim_queue =
            &pip_global.shm_task_queue[victim][pip_global.local_numa_id];
        if (victim_queue->head) {
            // __sync_add_and_fetch(&pip_global.shm_in_proc[victim], 1);
            MPIDI_PIP_Task_safe_dequeue(victim_queue, &task);
            // __sync_sub_and_fetch(&pip_global.shm_in_proc[victim], 1);
            if (task)
                MPIDI_PIP_do_task_copy(task);
        }
        // pip_global.try_steal++;
        // pip_global.esteal_try[victim]++;

        // pip_global.esteal_done[victim]++;
        // pip_global.suc_steal++;
        // printf
        //     ("Process %d steal task from victim %d, task %p, data size %ld, remaining task# %d, queue %p\n",
        //      pip_global.local_rank, victim, task, task->data_sz,
        //      pip_global.shm_task_queue[victim]->task_num, pip_global.shm_task_queue[victim]);
        // fflush(stdout);
#ifdef MPI_PIP_NM_TASK_STEAL
        /* netmod stealing */
        else {
            victim_queue = pip_global.shm_ucx_task_queue[victim];
            if (victim_queue->head) {
                MPIDI_PIP_Task_safe_dequeue(victim_queue, &task);
                if (task)
                    MPIDI_PIP_do_ucx_task_copy(task);
            }
        }
#endif

#endif


    }
}



#endif
