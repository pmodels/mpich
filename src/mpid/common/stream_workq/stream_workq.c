/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "stream_workq.h"
#include "utarray.h"
#include "src/mpid/common/genq/mpidu_genq.h"

#define MAX_NUM_VCIS REQUEST_POOL_MAX

static UT_array *workq_list;
static MPIDU_genq_private_pool_t event_pool;

static UT_icd icd_workq_list = { sizeof(void *), NULL, NULL, NULL };
static UT_icd icd_wait_list = { sizeof(MPIDU_stream_workq_wait_item_t), NULL, NULL, NULL };

static void *host_alloc_buffer_registered(uintptr_t size)
{
    void *ptr = MPL_malloc(size, MPL_MEM_BUFFER);
    MPIR_Assert(ptr);
    MPIR_gpu_register_host(ptr, size);
    return ptr;
}

static void host_free_buffer_registered(void *ptr)
{
    MPIR_gpu_unregister_host(ptr);
    MPL_free(ptr);
}

/* called from MPID_Init */
int MPIDU_stream_workq_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    utarray_new(workq_list, &icd_workq_list, MPL_MEM_OTHER);

    mpi_errno = MPIDU_genq_private_pool_create(sizeof(MPL_gpu_event_t),
                                               1024, 0,
                                               host_alloc_buffer_registered,
                                               host_free_buffer_registered, &event_pool);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* called from MPID_Finalize */
int MPIDU_stream_workq_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    utarray_free(workq_list);
    MPIDU_genq_private_pool_destroy(event_pool);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDU_stream_workq_alloc(MPIDU_stream_workq_t ** workq_out, int vci)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDU_stream_workq_t *workq;
    workq = MPL_malloc(sizeof(*workq), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!workq, mpi_errno, MPI_ERR_OTHER, "**nomem");

    workq->vci = vci;
    workq->op_head = NULL;
    workq->op_tail = NULL;

    utarray_new(workq->wait_list, &icd_wait_list, MPL_MEM_OTHER);

    /* append to workq_list (it's a pointer list) */
    utarray_push_back(workq_list, &workq, MPL_MEM_OTHER);

    *workq_out = workq;
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_stream_workq_dealloc(MPIDU_stream_workq_t * workq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_Assert(workq->op_head == NULL);
    MPIR_Assert(utarray_len(workq->wait_list) == 0);
    utarray_free(workq->wait_list);

    /* locate workq and remove from workq_list */
    void **ptr_array = ut_ptr_array(workq_list);
    for (int i = 0; i < utarray_len(workq_list); i++) {
        if (workq == ptr_array[i]) {
            utarray_erase(workq_list, i, 1);
            break;
        }
    }

    MPL_free(workq);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* called by MPIX_Xxx_enqueue, e.g. MPIX_Send_enqueue */
int MPIDU_stream_workq_enqueue(MPIDU_stream_workq_t * workq, MPIDU_stream_workq_op_t * op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    LL_APPEND(workq->op_head, workq->op_tail, op);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDU_stream_workq_alloc_event(MPL_gpu_event_t ** event_out)
{
    int mpi_errno = MPIDU_genq_private_pool_alloc_cell(event_pool, (void **) event_out);
    MPL_gpu_event_init_count(*event_out, 1);
    return mpi_errno;
}

int MPIDU_stream_workq_free_event(MPL_gpu_event_t * event)
{
    return MPIDU_genq_private_pool_free_cell(event_pool, (void *) event);
}

/* NOTE: to avoid vci contention, we progress all workqs with the same vci */
void MPIDU_stream_workq_progress_ops(int vci)
{
    void **ptr_array = ut_ptr_array(workq_list);
    for (int i = 0; i < utarray_len(workq_list); i++) {
        MPIDU_stream_workq_t *workq = ptr_array[i];
        if (workq->vci != vci) {
            continue;
        }
        while (workq->op_head) {
            MPIDU_stream_workq_op_t *op = workq->op_head;
            if (MPL_gpu_event_is_complete(op->trigger_event)) {
                LL_DELETE(workq->op_head, workq->op_tail, op);
                /* issue callback. cb can be NULL, e.g. wait_enqueue */
                if (op->cb) {
                    op->cb(op->data);
                }
                /* optionally enqueue to waitlist */
                if (op->request) {
                    MPIDU_stream_workq_wait_item_t item;
                    item.done_event = op->done_event;
                    item.request = *(op->request);
                    item.status = op->status;
                    utarray_push_back(workq->wait_list, &item, MPL_MEM_OTHER);
                }
                /* free op */
                MPL_free(op->data);
                MPL_free(op);
            } else {
                /* each workq is a serial stream */
                break;
            }
        }
    }
}

void MPIDU_stream_workq_progress_wait_list(int vci)
{
    void **ptr_array = ut_ptr_array(workq_list);
    for (int i = 0; i < utarray_len(workq_list); i++) {
        MPIDU_stream_workq_t *workq = ptr_array[i];
        if (workq->vci != vci) {
            continue;
        }
        if (utarray_len(workq->wait_list) > 0) {
            int count = utarray_len(workq->wait_list);
            MPIDU_stream_workq_wait_item_t *arr = ut_type_array(workq->wait_list,
                                                                MPIDU_stream_workq_wait_item_t *);

            int i2 = 0;
            for (int i1 = 0; i1 < count; i1++) {
                if (MPIR_Request_is_complete(arr[i1].request)) {
                    if (arr[i1].status) {
                        *(arr[i1].status) = arr[i1].request->status;
                    }
                    MPIR_Request_free(arr[i1].request);
                    if (arr[i1].done_event) {
                        MPL_gpu_event_complete(arr[i1].done_event);
                    }
                } else {
                    if (i1 > i2) {
                        arr[i2] = arr[i1];
                    }
                    i2++;
                }
            }
            if (i2 < count) {
                utarray_resize(workq->wait_list, i2, MPL_MEM_OTHER);
            }
        }
    }
}
