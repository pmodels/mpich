/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include "yaksa.h"
#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri.h"
#include "yutlist.h"

/* When there is wait kernel, treat unregistered host buffer the same as
 * registered host buffer to avoid potential deadlock */
#define AVOID_UNREGISTERED_HOST(ptr_type_, gpudriver_id_) \
    do { \
        if (yaksuri_global.has_wait_kernel && gpudriver_id_ == YAKSURI_GPUDRIVER_ID__CUDA) { \
            if (ptr_type_ == YAKSUR_PTR_TYPE__UNREGISTERED_HOST) { \
                ptr_type_ = YAKSUR_PTR_TYPE__REGISTERED_HOST; \
            } \
        } \
    } while (0)

static yaksuri_request_s *pending_reqs = NULL;
static pthread_mutex_t progress_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool buf_is_aligned(const void *buf, yaksi_type_s * type)
{
    return !((uintptr_t) buf % type->alignment);
}

static yaksi_type_s *get_base_type(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;
    yaksi_type_s *ret = NULL;

    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            ret = type;
            break;

        case YAKSI_TYPE_KIND__CONTIG:
            ret = get_base_type(type->u.contig.child);
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            ret = get_base_type(type->u.resized.child);
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            ret = get_base_type(type->u.hvector.child);
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            ret = get_base_type(type->u.blkhindx.child);
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            ret = get_base_type(type->u.hindexed.child);
            break;

        case YAKSI_TYPE_KIND__SUBARRAY:
            ret = get_base_type(type->u.subarray.primary);
            break;

        case YAKSI_TYPE_KIND__STRUCT:
            /* structs do not have a single base type, so we treat it
             * as a collection of bytes.  this will only work for
             * contiguous + REPLACE operations, but those are the only
             * cases that we should be seeing struct types here
             * anyway. */
            assert(type->is_contig);
            rc = yaksi_type_get(YAKSA_TYPE__BYTE, &ret);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        default:
            ret = NULL;
            break;
    }

  fn_exit:
    return ret;
  fn_fail:
    goto fn_exit;
}

static int icopy(yaksuri_gpudriver_id_e id, const void *inbuf, void *outbuf, uintptr_t count,
                 yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op, int device, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksi_type_s *base_type = get_base_type(type);

    if (stream) {
        rc = yaksuri_global.gpudriver[id].hooks->pack_with_stream(inbuf, outbuf,
                                                                  count * type->size /
                                                                  base_type->size, base_type, info,
                                                                  op, device, stream);
    } else {
        rc = yaksuri_global.gpudriver[id].hooks->ipack(inbuf, outbuf,
                                                       count * type->size / base_type->size,
                                                       base_type, info, op, device);
    }
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int ipack(yaksuri_gpudriver_id_e id, const void *inbuf, void *outbuf, uintptr_t count,
                 yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op, int device, void *stream)
{
    int rc = YAKSA_SUCCESS;

    if (stream) {
        rc = yaksuri_global.gpudriver[id].hooks->pack_with_stream(inbuf, outbuf, count, type, info,
                                                                  op, device, stream);
    } else {
        rc = yaksuri_global.gpudriver[id].hooks->ipack(inbuf, outbuf, count, type, info, op,
                                                       device);
    }
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int iunpack(yaksuri_gpudriver_id_e id, const void *inbuf, void *outbuf, uintptr_t count,
                   yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op, int device,
                   void *stream)
{
    int rc = YAKSA_SUCCESS;

    if (stream) {
        rc = yaksuri_global.gpudriver[id].hooks->unpack_with_stream(inbuf, outbuf, count, type,
                                                                    info, op, device, stream);
    } else {
        rc = yaksuri_global.gpudriver[id].hooks->iunpack(inbuf, outbuf, count, type, info, op,
                                                         device);
    }
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static bool check_p2p_comm(yaksuri_gpudriver_id_e id, int indev, int outdev)
{
    return yaksuri_global.gpudriver[id].hooks->check_p2p_comm(indev, outdev);
}

static int event_record(yaksuri_gpudriver_id_e id, int device, void **event)
{
    int rc = YAKSA_SUCCESS;

    rc = yaksuri_global.gpudriver[id].hooks->event_record(device, event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int event_query(yaksuri_gpudriver_id_e id, void *event, int *completed)
{
    int rc = YAKSA_SUCCESS;

    rc = yaksuri_global.gpudriver[id].hooks->event_query(event, completed);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int add_dependency(yaksuri_gpudriver_id_e id, int device1, int device2)
{
    int rc = YAKSA_SUCCESS;

    rc = yaksuri_global.gpudriver[id].hooks->add_dependency(device1, device2);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int alloc_chunk(yaksuri_gpudriver_id_e id, yaksuri_request_s * reqpriv,
                       yaksuri_subreq_s * subreq, int num_tmpbufs, int *devices,
                       yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_tmpbuf_s tmpbufs[YAKSURI_SUBREQ_CHUNK_MAX_TMPBUFS];

    assert(subreq);
    assert(subreq->kind == YAKSURI_SUBREQ_KIND__MULTI_CHUNK);

    *chunk = NULL;

    for (int i = 0; i < num_tmpbufs; i++) {
        void *buf;
        if (devices[i] >= 0) {
            rc = yaksu_buffer_pool_elem_alloc(yaksuri_global.gpudriver[id].device[devices[i]],
                                              &buf);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = yaksu_buffer_pool_elem_alloc(yaksuri_global.gpudriver[id].host, &buf);
            YAKSU_ERR_CHECK(rc, fn_fail);
        }

        if (buf == NULL) {
            for (int j = 0; j < i; j++) {
                rc = yaksu_buffer_pool_elem_free(tmpbufs[j].pool, tmpbufs[j].buf);
                YAKSU_ERR_CHECK(rc, fn_fail);
            }
            goto fn_exit;
        } else {
            tmpbufs[i].buf = buf;
            if (devices[i] >= 0) {
                tmpbufs[i].pool = yaksuri_global.gpudriver[id].device[devices[i]];
            } else {
                tmpbufs[i].pool = yaksuri_global.gpudriver[id].host;
            }
        }
    }

    /* allocate the chunk */
    *chunk = (yaksuri_subreq_chunk_s *) malloc(sizeof(yaksuri_subreq_chunk_s));

    (*chunk)->count_offset = subreq->u.multiple.issued_count;
    uintptr_t count_per_chunk;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / subreq->u.multiple.type->size;
    if ((*chunk)->count_offset + count_per_chunk <= subreq->u.multiple.count) {
        (*chunk)->count = count_per_chunk;
    } else {
        (*chunk)->count = subreq->u.multiple.count - (*chunk)->count_offset;
    }

    (*chunk)->num_tmpbufs = num_tmpbufs;
    memcpy((*chunk)->tmpbufs, tmpbufs, YAKSURI_SUBREQ_CHUNK_MAX_TMPBUFS * sizeof(yaksuri_tmpbuf_s));
    (*chunk)->event = NULL;

    DL_APPEND(subreq->u.multiple.chunks, (*chunk));

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int simple_release(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                          yaksuri_subreq_chunk_s * chunk)
{
    int rc = YAKSA_SUCCESS;

    /* cleanup */
    for (int i = 0; i < chunk->num_tmpbufs; i++) {
        rc = yaksu_buffer_pool_elem_free(chunk->tmpbufs[i].pool, chunk->tmpbufs[i].buf);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    DL_DELETE(subreq->u.multiple.chunks, chunk);
    free(chunk);

    if (subreq->u.multiple.chunks == NULL &&
        subreq->u.multiple.issued_count == subreq->u.multiple.count) {
        DL_DELETE(reqpriv->subreqs, subreq);
        yaksi_type_free(subreq->u.multiple.type);
        free(subreq);
    }
    if (reqpriv->subreqs == NULL) {
        HASH_DEL(pending_reqs, reqpriv);
        yaksu_atomic_decr(&reqpriv->request->cc);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* Stream callback versions of alloc_chunk and simple_release */
/* Assumption: for each stream, only one set of tempbufs need be active at a
 * time */
struct stream_buf {
    yaksuri_gpudriver_id_e id;
    void *stream;
    int device;
    void *buf;
    yaksu_buffer_pool_s pool;
    bool is_free;
};

struct stream_buf *stream_buf_list = NULL;
int stream_buf_list_count = 0;
int stream_buf_list_capacity = 0;

static int stream_buf_list_grow(void)
{
    int rc = YAKSA_SUCCESS;
    if (stream_buf_list_capacity == 0) {
        stream_buf_list = malloc(10 * sizeof(struct stream_buf));
        if (stream_buf_list == NULL) {
            rc = YAKSA_ERR__OUT_OF_MEM;
            goto fn_fail;
        }
        stream_buf_list_capacity = 10;
    } else {
        int new_capacity = stream_buf_list_capacity * 2;
        void *new_list = realloc(stream_buf_list, new_capacity * sizeof(struct stream_buf));
        if (new_list == NULL) {
            rc = YAKSA_ERR__OUT_OF_MEM;
            goto fn_fail;
        }
        stream_buf_list = new_list;
        stream_buf_list_capacity = new_capacity;
    }
  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static void stream_buf_list_free(void)
{
    if (stream_buf_list_capacity > 0) {
        for (int i = 0; i < stream_buf_list_count; i++) {
            int rc ATTRIBUTE((unused)) =
                yaksu_buffer_pool_elem_free(stream_buf_list[i].pool, stream_buf_list[i].buf);
            assert(rc == YAKSA_SUCCESS);
        }
        free(stream_buf_list);
        stream_buf_list_count = 0;
        stream_buf_list_capacity = 0;
    }
}

/* NOTE: we could use a hash to optimize; however, practically we don't expect
 * the list to get long and the allocation is off the critical path anyway */
/* Return index to stream_buf_list.
 * Return -1 on error */
static int alloc_stream_buf(yaksuri_gpudriver_id_e id, int device, void *stream)
{
    int rc = YAKSA_SUCCESS;

    /* first check for existing allocations */
    if (stream_buf_list_count > 0) {
        for (int i = 0; i < stream_buf_list_count; i++) {
            if (stream_buf_list[i].is_free && stream_buf_list[i].id == id &&
                stream_buf_list[i].device == device && stream_buf_list[i].stream == stream) {
                stream_buf_list[i].is_free = false;
                return i;
            }
        }
    }
    /* potentially grow the capacity */
    if (stream_buf_list_count == stream_buf_list_capacity) {
        rc = stream_buf_list_grow();
        YAKSU_ERR_CHECK(rc, fn_fail);
    }
    /* get a new buffer */
    void *buf;
    yaksu_buffer_pool_s pool;
    if (device >= 0) {
        pool = yaksuri_global.gpudriver[id].device[device];
    } else {
        pool = yaksuri_global.gpudriver[id].host;
    }
    rc = yaksu_buffer_pool_elem_alloc(pool, &buf);
    YAKSU_ERR_CHECK(rc, fn_fail);

    int idx;
    idx = stream_buf_list_count++;
    stream_buf_list[idx].buf = buf;
    stream_buf_list[idx].pool = pool;
    stream_buf_list[idx].id = id;
    stream_buf_list[idx].stream = stream;
    stream_buf_list[idx].device = device;
    stream_buf_list[idx].is_free = false;

    return idx;
  fn_fail:
    return -1;
}

static void *get_stream_buf(int idx)
{
    return stream_buf_list[idx].buf;
}

static void free_stream_buf(int idx)
{
    stream_buf_list[idx].is_free = true;
}

static int alloc_tmpbufs(yaksuri_gpudriver_id_e id,
                         int *tmpbufs, int num_tmpbufs, int *devices, void *stream)
{
    int rc = YAKSA_SUCCESS;

    for (int i = 0; i < num_tmpbufs; i++) {
        tmpbufs[i] = alloc_stream_buf(id, devices[i], stream);
        if (tmpbufs[i] < 0) {
            rc = YAKSA_ERR__OUT_OF_MEM;
            goto fn_fail;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* data used by stream callbacks */
struct callback_chunk {
    int err;
    int tmpbufs[YAKSURI_SUBREQ_CHUNK_MAX_TMPBUFS];      /* indexes to stream_buf_list */
    int num_tmpbufs;
    /* following only needed by per-chunk callbacks */
    const void *inbuf;
    void *outbuf;
    intptr_t count;
    yaksi_type_s *type;
    yaksa_op_t op;
    yaksi_info_s *info;
    /* offset will start as 0, incremented by each callbacks */
    intptr_t count_offset;
};

/* called when we schedule chunk_free_stream_cb. This allows next stream
 * operation to reuse the tmpbufs. */
static void tmpbufs_stream_release(struct callback_chunk *chunk)
{
    for (int i = 0; i < chunk->num_tmpbufs; i++) {
        free_stream_buf(chunk->tmpbufs[i]);
    }
}

static void chunk_free_stream_cb(void *data)
{
    struct callback_chunk *chunk = data;
    yaksi_type_free(chunk->type);
    free(data);
}

/* Subroutines for multi-chunk pack
 *   pack_d2d_p2p_acquire
 *   pack_d2d_nop2p_acquire
 *   pack_d2d_unaligned_acquire
 *   pack_d2rh_acquire, pack_d2rh_release
 *   pack_d2urh_acquire, pack_d2urh_release
 *   pack_h2d_acquire
 */
static int pack_d2d_p2p_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                                yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    yaksa_op_t op = subreq->u.multiple.op;

    assert(reqpriv->request->backend.inattr.device != reqpriv->request->backend.outattr.device);

    *chunk = NULL;

    void *base_addr = (char *) subreq->u.multiple.outbuf + subreq->u.multiple.type->true_lb;

    if (op == YAKSA_OP__REPLACE) {
        int devices[] = { reqpriv->request->backend.inattr.device };
        rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else if (buf_is_aligned(base_addr, subreq->u.multiple.type)) {
        int devices[] = { reqpriv->request->backend.inattr.device,
            reqpriv->request->backend.outattr.device
        };
        rc = alloc_chunk(id, reqpriv, subreq, 2, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        int devices[] = { reqpriv->request->backend.inattr.device,
            reqpriv->request->backend.outattr.device,
            reqpriv->request->backend.outattr.device
        };
        rc = alloc_chunk(id, reqpriv, subreq, 3, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    if (*chunk == NULL)
        goto fn_exit;

    void *src_d_buf, *dst_d_buf, *dst_d_buf2;
    src_d_buf = (*chunk)->tmpbufs[0].buf;
    dst_d_buf = (*chunk)->tmpbufs[1].buf;
    dst_d_buf2 = (*chunk)->tmpbufs[2].buf;

    const char *sbuf;
    char *dbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->extent;
    dbuf =
        (char *) subreq->u.multiple.outbuf + (*chunk)->count_offset * subreq->u.multiple.type->size;

    rc = ipack(id, sbuf, src_d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
               YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (op == YAKSA_OP__REPLACE) {
        rc = icopy(id, src_d_buf, dbuf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = event_record(id, reqpriv->request->backend.inattr.device, &(*chunk)->event);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else if (buf_is_aligned(base_addr, subreq->u.multiple.type)) {
        rc = icopy(id, src_d_buf, dst_d_buf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = add_dependency(id, reqpriv->request->backend.inattr.device,
                            reqpriv->request->backend.outattr.device);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, dst_d_buf, dbuf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, op, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = event_record(id, reqpriv->request->backend.outattr.device, &(*chunk)->event);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        rc = icopy(id, src_d_buf, dst_d_buf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, dbuf, dst_d_buf2, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device,
                   NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = add_dependency(id, reqpriv->request->backend.inattr.device,
                            reqpriv->request->backend.outattr.device);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, dst_d_buf, dst_d_buf2, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, op, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, dst_d_buf2, dbuf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device,
                   NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = event_record(id, reqpriv->request->backend.outattr.device, &(*chunk)->event);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_d2d_nop2p_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                                  yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    yaksa_op_t op = subreq->u.multiple.op;
    yaksi_type_s *type = subreq->u.multiple.type;

    assert(reqpriv->request->backend.inattr.device != reqpriv->request->backend.outattr.device);

    *chunk = NULL;

    void *base_addr = (char *) subreq->u.multiple.outbuf + subreq->u.multiple.type->true_lb;

    if (op == YAKSA_OP__REPLACE && type->is_contig) {
        int devices[] = { -1 };
        rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else if (op == YAKSA_OP__REPLACE) {
        int devices[] = { -1, reqpriv->request->backend.inattr.device };
        rc = alloc_chunk(id, reqpriv, subreq, 2, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else if (buf_is_aligned(base_addr, subreq->u.multiple.type)) {
        int devices[] = { -1, reqpriv->request->backend.inattr.device,
            reqpriv->request->backend.outattr.device
        };
        rc = alloc_chunk(id, reqpriv, subreq, 3, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        int devices[] = { -1, reqpriv->request->backend.inattr.device,
            reqpriv->request->backend.outattr.device,
            reqpriv->request->backend.outattr.device
        };
        rc = alloc_chunk(id, reqpriv, subreq, 4, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    if (*chunk == NULL)
        goto fn_exit;

    void *src_d_buf, *rh_buf, *dst_d_buf, *dst_d_buf2;
    rh_buf = (*chunk)->tmpbufs[0].buf;
    src_d_buf = (*chunk)->tmpbufs[1].buf;
    dst_d_buf = (*chunk)->tmpbufs[2].buf;
    dst_d_buf2 = (*chunk)->tmpbufs[3].buf;

    const char *sbuf;
    char *dbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->extent;
    dbuf =
        (char *) subreq->u.multiple.outbuf + (*chunk)->count_offset * subreq->u.multiple.type->size;

    if (op == YAKSA_OP__REPLACE && type->is_contig) {
        rc = ipack(id, sbuf, rh_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        rc = ipack(id, sbuf, src_d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, src_d_buf, rh_buf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    rc = add_dependency(id, reqpriv->request->backend.inattr.device,
                        reqpriv->request->backend.outattr.device);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (op == YAKSA_OP__REPLACE) {
        rc = icopy(id, rh_buf, dbuf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device,
                   NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else if (buf_is_aligned(base_addr, subreq->u.multiple.type)) {
        rc = icopy(id, rh_buf, dst_d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, dst_d_buf, dbuf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, op, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        rc = icopy(id, rh_buf, dst_d_buf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device,
                   NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, dbuf, dst_d_buf2, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device,
                   NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, dst_d_buf, dst_d_buf2, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, op, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, dst_d_buf2, dbuf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device,
                   NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    rc = event_record(id, reqpriv->request->backend.outattr.device, &(*chunk)->event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_d2d_unaligned_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                                      yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    yaksa_op_t op = subreq->u.multiple.op;

    *chunk = NULL;

    int devices[] = { reqpriv->request->backend.inattr.device };
    rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (*chunk == NULL)
        goto fn_exit;

    void *d_buf;
    d_buf = (*chunk)->tmpbufs[0].buf;

    const char *sbuf;
    char *dbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->extent;
    dbuf =
        (char *) subreq->u.multiple.outbuf + (*chunk)->count_offset * subreq->u.multiple.type->size;

    rc = icopy(id, dbuf, d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
               YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = ipack(id, sbuf, d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
               op, reqpriv->request->backend.inattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = icopy(id, d_buf, dbuf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
               YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = event_record(id, reqpriv->request->backend.inattr.device, &(*chunk)->event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_d2rh_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                             yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    yaksa_op_t op = subreq->u.multiple.op;

    *chunk = NULL;

    if (op == YAKSA_OP__REPLACE) {
        int devices[] = { reqpriv->request->backend.inattr.device };
        rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        int devices[] = { reqpriv->request->backend.inattr.device, -1 };
        rc = alloc_chunk(id, reqpriv, subreq, 2, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    if (*chunk == NULL)
        goto fn_exit;

    void *d_buf, *rh_buf;
    d_buf = (*chunk)->tmpbufs[0].buf;
    rh_buf = (*chunk)->tmpbufs[1].buf;

    const char *sbuf;
    char *dbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->extent;
    dbuf = (char *) subreq->u.multiple.outbuf + (*chunk)->count_offset *
        subreq->u.multiple.type->size;

    rc = ipack(id, sbuf, d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
               YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (op == YAKSA_OP__REPLACE) {
        rc = icopy(id, d_buf, dbuf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        rc = icopy(id, d_buf, rh_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    rc = event_record(id, reqpriv->request->backend.inattr.device, &(*chunk)->event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_d2rh_release(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                             yaksuri_subreq_chunk_s * chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksa_op_t op = subreq->u.multiple.op;
    yaksi_type_s *type = subreq->u.multiple.type;

    if (op != YAKSA_OP__REPLACE) {
        yaksi_type_s *base_type = get_base_type(type);
        char *dbuf = (char *) subreq->u.multiple.outbuf + chunk->count_offset *
            subreq->u.multiple.type->size;

        rc = yaksuri_seq_ipack(chunk->tmpbufs[1].buf, dbuf,
                               chunk->count * type->size / base_type->size, base_type,
                               reqpriv->info, subreq->u.multiple.op);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    rc = simple_release(reqpriv, subreq, chunk);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_d2urh_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                              yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    yaksa_op_t op = subreq->u.multiple.op;
    yaksi_type_s *type = subreq->u.multiple.type;

    *chunk = NULL;

    void *d_buf = NULL, *rh_buf;
    /* no need to bounce to device buffer if type is contig and copy only */
    if (op == YAKSA_OP__REPLACE && type->is_contig) {
        int devices[] = { -1 };
        rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);

        if (*chunk == NULL)
            goto fn_exit;

        rh_buf = (*chunk)->tmpbufs[0].buf;
    } else {
        int devices[] = { -1, reqpriv->request->backend.inattr.device };
        rc = alloc_chunk(id, reqpriv, subreq, 2, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);

        if (*chunk == NULL)
            goto fn_exit;

        rh_buf = (*chunk)->tmpbufs[0].buf;
        d_buf = (*chunk)->tmpbufs[1].buf;
    }

    const char *sbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->extent;

    if (d_buf) {
        rc = ipack(id, sbuf, d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, d_buf, rh_buf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        rc = ipack(id, sbuf, rh_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    rc = event_record(id, reqpriv->request->backend.inattr.device, &(*chunk)->event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_d2urh_release(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                              yaksuri_subreq_chunk_s * chunk)
{
    int rc = YAKSA_SUCCESS;
    char *dbuf = (char *) subreq->u.multiple.outbuf + chunk->count_offset *
        subreq->u.multiple.type->size;
    yaksi_type_s *type = subreq->u.multiple.type;
    yaksi_type_s *base_type = get_base_type(type);

    rc = yaksuri_seq_ipack(chunk->tmpbufs[0].buf, dbuf,
                           chunk->count * type->size / base_type->size, base_type,
                           reqpriv->info, subreq->u.multiple.op);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = simple_release(reqpriv, subreq, chunk);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_h2d_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                            yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    yaksa_op_t op = subreq->u.multiple.op;

    *chunk = NULL;

    void *base_addr = (char *) subreq->u.multiple.outbuf + subreq->u.multiple.type->true_lb;

    if (op == YAKSA_OP__REPLACE) {
        int devices[] = { -1 };
        rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else if (buf_is_aligned(base_addr, subreq->u.multiple.type)) {
        int devices[] = { -1, reqpriv->request->backend.outattr.device };
        rc = alloc_chunk(id, reqpriv, subreq, 2, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        int devices[] = { -1, reqpriv->request->backend.outattr.device,
            reqpriv->request->backend.outattr.device
        };
        rc = alloc_chunk(id, reqpriv, subreq, 3, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    if (*chunk == NULL)
        goto fn_exit;

    void *rh_buf, *d_buf, *d_buf2;
    rh_buf = (*chunk)->tmpbufs[0].buf;
    d_buf = (*chunk)->tmpbufs[1].buf;
    d_buf2 = (*chunk)->tmpbufs[2].buf;

    const char *sbuf;
    char *dbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->extent;
    dbuf = (char *) subreq->u.multiple.outbuf + (*chunk)->count_offset *
        subreq->u.multiple.type->size;

    rc = yaksuri_seq_ipack(sbuf, rh_buf, (*chunk)->count, subreq->u.multiple.type,
                           reqpriv->info, YAKSA_OP__REPLACE);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (op == YAKSA_OP__REPLACE) {
        rc = icopy(id, rh_buf, dbuf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else if (buf_is_aligned(base_addr, subreq->u.multiple.type)) {
        rc = icopy(id, rh_buf, d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, d_buf, dbuf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   op, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        rc = icopy(id, rh_buf, d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, dbuf, d_buf2, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, d_buf, d_buf2, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   op, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, d_buf2, dbuf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    rc = event_record(id, reqpriv->request->backend.outattr.device, &(*chunk)->event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* Subroutines for multi-chunk unpack
 *   unpack_d2d_p2p_acquire
 *   unpack_d2d_nop2p_acquire
 *   unpack_d2d_unaligned_acquire
 *   unpack_rh2d_acquire
 *   unpack_urh2d_acquire
 *   unpack_d2h_acquire, unpack_d2h_release
 */

static int unpack_d2d_p2p_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                                  yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;

    assert(reqpriv->request->backend.inattr.device != reqpriv->request->backend.outattr.device);

    *chunk = NULL;

    int devices[] = { reqpriv->request->backend.outattr.device };
    rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (*chunk == NULL)
        goto fn_exit;

    void *d_buf;
    d_buf = (*chunk)->tmpbufs[0].buf;

    const char *sbuf;
    char *dbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->size;
    dbuf = (char *) subreq->u.multiple.outbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->extent;

    rc = icopy(id, sbuf, d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
               YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = add_dependency(id, reqpriv->request->backend.inattr.device,
                        reqpriv->request->backend.outattr.device);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = iunpack(id, d_buf, dbuf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                 subreq->u.multiple.op, reqpriv->request->backend.outattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = event_record(id, reqpriv->request->backend.outattr.device, &(*chunk)->event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_d2d_nop2p_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                                    yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    yaksa_op_t op = subreq->u.multiple.op;
    yaksi_type_s *type = subreq->u.multiple.type;

    assert(reqpriv->request->backend.inattr.device != reqpriv->request->backend.outattr.device);

    *chunk = NULL;

    if (op == YAKSA_OP__REPLACE && type->is_contig) {
        int devices[] = { -1 };
        rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        int devices[] = { -1, reqpriv->request->backend.outattr.device };
        rc = alloc_chunk(id, reqpriv, subreq, 2, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    if (*chunk == NULL)
        goto fn_exit;

    void *d_buf, *rh_buf;
    rh_buf = (*chunk)->tmpbufs[0].buf;
    d_buf = (*chunk)->tmpbufs[1].buf;

    const char *sbuf;
    char *dbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->size;
    dbuf = (char *) subreq->u.multiple.outbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->extent;

    rc = icopy(id, sbuf, rh_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
               YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = add_dependency(id, reqpriv->request->backend.inattr.device,
                        reqpriv->request->backend.outattr.device);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (op == YAKSA_OP__REPLACE && type->is_contig) {
        rc = iunpack(id, rh_buf, dbuf, (*chunk)->count, subreq->u.multiple.type,
                     reqpriv->info, subreq->u.multiple.op, reqpriv->request->backend.outattr.device,
                     NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        rc = icopy(id, rh_buf, d_buf, (*chunk)->count, subreq->u.multiple.type,
                   reqpriv->info, YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device,
                   NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = iunpack(id, d_buf, dbuf, (*chunk)->count, subreq->u.multiple.type,
                     reqpriv->info, subreq->u.multiple.op, reqpriv->request->backend.outattr.device,
                     NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    rc = event_record(id, reqpriv->request->backend.outattr.device, &(*chunk)->event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_d2d_unaligned_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                                        yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;

    *chunk = NULL;

    int devices[] = { reqpriv->request->backend.inattr.device };
    rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (*chunk == NULL)
        goto fn_exit;

    void *d_buf;
    d_buf = (*chunk)->tmpbufs[0].buf;

    const char *sbuf;
    char *dbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->size;
    dbuf = (char *) subreq->u.multiple.outbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->extent;

    rc = icopy(id, sbuf, d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
               YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = iunpack(id, d_buf, dbuf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                 subreq->u.multiple.op, reqpriv->request->backend.inattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = event_record(id, reqpriv->request->backend.inattr.device, &(*chunk)->event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_rh2d_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                               yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;

    *chunk = NULL;

    int devices[] = { reqpriv->request->backend.outattr.device };
    rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (*chunk == NULL)
        goto fn_exit;

    void *d_buf;
    d_buf = (*chunk)->tmpbufs[0].buf;

    const char *sbuf;
    char *dbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->size;
    dbuf = (char *) subreq->u.multiple.outbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->extent;

    rc = icopy(id, sbuf, d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
               YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = iunpack(id, d_buf, dbuf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                 subreq->u.multiple.op, reqpriv->request->backend.outattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = event_record(id, reqpriv->request->backend.outattr.device, &(*chunk)->event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_urh2d_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                                yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    yaksa_op_t op = subreq->u.multiple.op;
    yaksi_type_s *type = subreq->u.multiple.type;

    *chunk = NULL;

    /* no need to bounce to device buffer if type is contig and copy only */
    if (op == YAKSA_OP__REPLACE && type->is_contig) {
        int devices[] = { -1 };
        rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        int devices[] = { -1, reqpriv->request->backend.outattr.device };
        rc = alloc_chunk(id, reqpriv, subreq, 2, devices, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }
    if (*chunk == NULL)
        goto fn_exit;

    void *d_buf, *rh_buf;
    rh_buf = (*chunk)->tmpbufs[0].buf;
    d_buf = (*chunk)->tmpbufs[1].buf;

    const char *sbuf;
    char *dbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->size;
    dbuf = (char *) subreq->u.multiple.outbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->extent;

    yaksi_type_s *byte_type;
    rc = yaksi_type_get(YAKSA_TYPE__BYTE, &byte_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksuri_seq_ipack(sbuf, rh_buf, (*chunk)->count * subreq->u.multiple.type->size,
                           byte_type, reqpriv->info, YAKSA_OP__REPLACE);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (op == YAKSA_OP__REPLACE && type->is_contig) {
        rc = iunpack(id, rh_buf, dbuf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                     op, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        rc = icopy(id, rh_buf, d_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                   YAKSA_OP__REPLACE, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = iunpack(id, d_buf, dbuf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
                     subreq->u.multiple.op, reqpriv->request->backend.outattr.device, NULL);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    rc = event_record(id, reqpriv->request->backend.outattr.device, &(*chunk)->event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_d2h_acquire(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                              yaksuri_subreq_chunk_s ** chunk)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;

    *chunk = NULL;

    int devices[] = { -1 };
    rc = alloc_chunk(id, reqpriv, subreq, 1, devices, chunk);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (*chunk == NULL)
        goto fn_exit;

    void *rh_buf;
    rh_buf = (*chunk)->tmpbufs[0].buf;

    const char *sbuf;
    sbuf = (const char *) subreq->u.multiple.inbuf +
        (*chunk)->count_offset * subreq->u.multiple.type->size;

    rc = icopy(id, sbuf, rh_buf, (*chunk)->count, subreq->u.multiple.type, reqpriv->info,
               YAKSA_OP__REPLACE, reqpriv->request->backend.inattr.device, NULL);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = event_record(id, reqpriv->request->backend.inattr.device, &(*chunk)->event);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_d2h_release(yaksuri_request_s * reqpriv, yaksuri_subreq_s * subreq,
                              yaksuri_subreq_chunk_s * chunk)
{
    int rc = YAKSA_SUCCESS;

    char *dbuf;
    dbuf = (char *) subreq->u.multiple.outbuf + chunk->count_offset *
        subreq->u.multiple.type->extent;
    rc = yaksuri_seq_iunpack(chunk->tmpbufs[0].buf, dbuf, chunk->count, subreq->u.multiple.type,
                             reqpriv->info, subreq->u.multiple.op);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = simple_release(reqpriv, subreq, chunk);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int set_multichunk_subreq(const void *inbuf, void *outbuf, uintptr_t count,
                                 yaksi_type_s * type, yaksa_op_t op, yaksuri_subreq_s ** subreq_ptr)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_subreq_s *subreq = (yaksuri_subreq_s *) malloc(sizeof(yaksuri_subreq_s));
    *subreq_ptr = subreq;

    /* we can only take on types where at least one count of the type
     * fits into our temporary buffers. */
    if (type->size > YAKSURI_TMPBUF_EL_SIZE) {
        return YAKSA_ERR__NOT_SUPPORTED;
    }

    subreq->kind = YAKSURI_SUBREQ_KIND__MULTI_CHUNK;
    subreq->u.multiple.inbuf = inbuf;
    subreq->u.multiple.outbuf = outbuf;
    subreq->u.multiple.count = count;
    subreq->u.multiple.type = type;
    subreq->u.multiple.op = op;
    subreq->u.multiple.issued_count = 0;
    subreq->u.multiple.chunks = NULL;

    yaksu_atomic_incr(&type->refcount);
    return rc;
}

/* Subroutines for multi-chunk stream pack
 *   pack_d2d_p2p_stream
 *   pack_d2d_nop2p_stream
 *   pack_d2d_unaligned_stream
 *   pack_d2rh_stream_cb, pack_d2rh_stream
 *   pack_d2urh_stream_cb, pack_d2urh_stream
 *   pack_h2d_stream_cb, pack_h2d_stream
 */

static int pack_d2d_p2p_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                               uintptr_t count, yaksi_type_s * type, yaksa_op_t op, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int in_device = reqpriv->request->backend.inattr.device;
    int out_device = reqpriv->request->backend.outattr.device;
    assert(in_device != out_device);

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    /* allocate tmpbufs */
    int devices[] = { in_device, out_device, out_device };

    void *base_addr = (char *) outbuf + type->true_lb;
    if (op == YAKSA_OP__REPLACE) {
        chunk->num_tmpbufs = 1;
    } else if (buf_is_aligned(base_addr, type)) {
        chunk->num_tmpbufs = 2;
    } else {
        chunk->num_tmpbufs = 3;
    }
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *src_d_buf, *dst_d_buf, *dst_d_buf2;
    src_d_buf = get_stream_buf(chunk->tmpbufs[0]);
    dst_d_buf = NULL;
    dst_d_buf2 = NULL;
    if (chunk->num_tmpbufs > 1) {
        dst_d_buf = get_stream_buf(chunk->tmpbufs[1]);
    }
    if (chunk->num_tmpbufs > 2) {
        dst_d_buf2 = get_stream_buf(chunk->tmpbufs[2]);
    }

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        const char *sbuf = (const char *) inbuf + count_offset * type->extent;
        char *dbuf = (char *) outbuf + count_offset * type->size;

        rc = ipack(id, sbuf, src_d_buf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        if (op == YAKSA_OP__REPLACE) {
            rc = icopy(id, src_d_buf, dbuf, chunk_count, type,
                       reqpriv->info, YAKSA_OP__REPLACE, in_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else if (buf_is_aligned(base_addr, type)) {
            rc = icopy(id, src_d_buf, dst_d_buf, chunk_count, type,
                       reqpriv->info, YAKSA_OP__REPLACE, in_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, dst_d_buf, dbuf, chunk_count, type,
                       reqpriv->info, op, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = icopy(id, src_d_buf, dst_d_buf, chunk_count, type,
                       reqpriv->info, YAKSA_OP__REPLACE, in_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, dbuf, dst_d_buf2, chunk_count, type,
                       reqpriv->info, YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, dst_d_buf, dst_d_buf2, chunk_count, type,
                       reqpriv->info, op, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, dst_d_buf2, dbuf, chunk_count, type,
                       reqpriv->info, YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        }
        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_d2d_nop2p_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                                 uintptr_t count, yaksi_type_s * type, yaksa_op_t op, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int in_device = reqpriv->request->backend.inattr.device;
    int out_device = reqpriv->request->backend.outattr.device;
    assert(in_device != out_device);

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    /* allocate tmpbufs */
    int devices[] = { in_device, -1, out_device, out_device };

    void *base_addr = (char *) outbuf + type->true_lb;
    if (op == YAKSA_OP__REPLACE) {
        chunk->num_tmpbufs = 2;
    } else if (buf_is_aligned(base_addr, type)) {
        chunk->num_tmpbufs = 3;
    } else {
        chunk->num_tmpbufs = 4;
    }
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *src_d_buf, *rh_buf, *dst_d_buf, *dst_d_buf2;
    src_d_buf = get_stream_buf(chunk->tmpbufs[0]);
    rh_buf = get_stream_buf(chunk->tmpbufs[1]);
    dst_d_buf = NULL;
    dst_d_buf2 = NULL;
    if (chunk->num_tmpbufs > 2) {
        dst_d_buf = get_stream_buf(chunk->tmpbufs[2]);
    }
    if (chunk->num_tmpbufs > 3) {
        dst_d_buf2 = get_stream_buf(chunk->tmpbufs[3]);
    }

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        const char *sbuf = (const char *) inbuf + count_offset * type->extent;
        char *dbuf = (char *) outbuf + count_offset * type->size;

        rc = ipack(id, sbuf, src_d_buf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, src_d_buf, rh_buf, chunk_count, type,
                   reqpriv->info, YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        if (op == YAKSA_OP__REPLACE) {
            rc = icopy(id, rh_buf, dbuf, chunk_count, type,
                       reqpriv->info, YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else if (buf_is_aligned(base_addr, type)) {
            rc = icopy(id, rh_buf, dst_d_buf, chunk_count, type, reqpriv->info,
                       YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, dst_d_buf, dbuf, chunk_count, type,
                       reqpriv->info, op, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = icopy(id, rh_buf, dst_d_buf, chunk_count, type,
                       reqpriv->info, YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, dbuf, dst_d_buf2, chunk_count, type,
                       reqpriv->info, YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, dst_d_buf, dst_d_buf2, chunk_count, type,
                       reqpriv->info, op, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, dst_d_buf2, dbuf, chunk_count, type,
                       reqpriv->info, YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        }
        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int pack_d2d_unaligned_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                                     uintptr_t count, yaksi_type_s * type, yaksa_op_t op,
                                     void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int in_device = reqpriv->request->backend.inattr.device;

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    int devices[] = { in_device };
    chunk->num_tmpbufs = 1;
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *d_buf;
    d_buf = get_stream_buf(chunk->tmpbufs[0]);

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        const char *sbuf = (const char *) inbuf + count_offset * type->extent;
        char *dbuf = (char *) outbuf + count_offset * type->size;

        rc = icopy(id, dbuf, d_buf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = ipack(id, sbuf, d_buf, chunk_count, type, reqpriv->info, op, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, d_buf, dbuf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static void pack_d2rh_stream_cb(void *data)
{
    struct callback_chunk *chunk = data;

    yaksa_op_t op = chunk->op;
    yaksi_type_s *type = chunk->type;

    assert(op != YAKSA_OP__REPLACE);

    yaksi_type_s *base_type = get_base_type(type);
    char *dbuf = (char *) chunk->outbuf + chunk->count_offset * type->size;

    intptr_t count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    intptr_t chunk_count = YAKSU_MIN(count_per_chunk, chunk->count - chunk->count_offset);

    void *rh_buf;
    rh_buf = get_stream_buf(chunk->tmpbufs[1]);

    int rc ATTRIBUTE((unused));
    rc = yaksuri_seq_ipack(rh_buf, dbuf,
                           chunk_count * type->size / base_type->size, base_type, chunk->info, op);
    assert(rc == YAKSA_SUCCESS);
    chunk->count_offset += chunk_count;
}

static int pack_d2rh_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                            uintptr_t count, yaksi_type_s * type, yaksa_op_t op, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int in_device = reqpriv->request->backend.inattr.device;

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    int devices[] = { in_device, -1 };
    if (op == YAKSA_OP__REPLACE) {
        chunk->num_tmpbufs = 1;
    } else {
        chunk->num_tmpbufs = 2;
    }
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *d_buf, *rh_buf;
    d_buf = get_stream_buf(chunk->tmpbufs[0]);
    if (chunk->num_tmpbufs > 1) {
        rh_buf = get_stream_buf(chunk->tmpbufs[1]);
    } else {
        rh_buf = NULL;
    }

    /* setup chunk for pack_d2rh_stream_cb */
    if (op != YAKSA_OP__REPLACE) {
        chunk->inbuf = inbuf;
        chunk->outbuf = outbuf;
        chunk->op = op;
        chunk->info = reqpriv->info;
        chunk->count = count;
        chunk->count_offset = 0;
    }

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        const char *sbuf = (const char *) inbuf + count_offset * type->extent;
        char *dbuf = (char *) outbuf + count_offset * type->size;

        rc = ipack(id, sbuf, d_buf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        if (op == YAKSA_OP__REPLACE) {
            rc = icopy(id, d_buf, dbuf, chunk_count, type, reqpriv->info,
                       YAKSA_OP__REPLACE, in_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = icopy(id, d_buf, rh_buf, chunk_count, type, reqpriv->info,
                       YAKSA_OP__REPLACE, in_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        }
        /* pack op on the host buffer */
        if (op != YAKSA_OP__REPLACE) {
            rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, pack_d2rh_stream_cb,
                                                                   chunk);
            YAKSU_ERR_CHECK(rc, fn_fail);
        }

        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static void pack_d2urh_stream_cb(void *data)
{
    struct callback_chunk *chunk = data;

    yaksa_op_t op = chunk->op;
    yaksi_type_s *type = chunk->type;

    yaksi_type_s *base_type = get_base_type(type);
    char *dbuf = (char *) chunk->outbuf + chunk->count_offset * type->size;

    intptr_t count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    intptr_t chunk_count = YAKSU_MIN(count_per_chunk, chunk->count - chunk->count_offset);

    void *rh_buf;
    rh_buf = get_stream_buf(chunk->tmpbufs[1]);

    int rc ATTRIBUTE((unused));
    rc = yaksuri_seq_ipack(rh_buf, dbuf,
                           chunk_count * type->size / base_type->size, base_type, chunk->info, op);
    assert(rc == YAKSA_SUCCESS);
    chunk->count_offset += chunk_count;
}

static int pack_d2urh_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                             uintptr_t count, yaksi_type_s * type, yaksa_op_t op, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int in_device = reqpriv->request->backend.inattr.device;

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    int devices[] = { in_device, -1 };
    chunk->num_tmpbufs = 2;
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *d_buf, *rh_buf;
    d_buf = get_stream_buf(chunk->tmpbufs[0]);
    rh_buf = get_stream_buf(chunk->tmpbufs[1]);

    /* setup chunk for pack_d2urh_stream_cb */
    chunk->inbuf = inbuf;
    chunk->outbuf = outbuf;
    chunk->op = op;
    chunk->info = reqpriv->info;
    chunk->count = count;
    chunk->count_offset = 0;

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        const char *sbuf = (const char *) inbuf + count_offset * type->extent;

        rc = ipack(id, sbuf, d_buf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, d_buf, rh_buf, chunk_count, type,
                   reqpriv->info, YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, pack_d2urh_stream_cb, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);

        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static void pack_h2d_stream_cb(void *data)
{
    int rc ATTRIBUTE((unused));
    struct callback_chunk *chunk = data;

    yaksi_type_s *type = chunk->type;

    yaksi_type_s *byte_type;
    rc = yaksi_type_get(YAKSA_TYPE__BYTE, &byte_type);
    assert(rc == YAKSA_SUCCESS);

    const char *sbuf = (const char *) chunk->inbuf + chunk->count_offset * type->size;

    intptr_t count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    intptr_t chunk_count = YAKSU_MIN(count_per_chunk, chunk->count - chunk->count_offset);

    void *rh_buf;
    rh_buf = get_stream_buf(chunk->tmpbufs[0]);

    rc = yaksuri_seq_ipack(sbuf, rh_buf, chunk_count, type, chunk->info, YAKSA_OP__REPLACE);
    assert(rc == YAKSA_SUCCESS);
    chunk->count_offset += chunk_count;
}

static int pack_h2d_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                           uintptr_t count, yaksi_type_s * type, yaksa_op_t op, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int out_device = reqpriv->request->backend.outattr.device;

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    int devices[] = { -1, out_device, out_device };
    void *base_addr = (char *) outbuf + type->true_lb;
    if (op == YAKSA_OP__REPLACE) {
        chunk->num_tmpbufs = 1;
    } else if (buf_is_aligned(base_addr, type)) {
        chunk->num_tmpbufs = 2;
    } else {
        chunk->num_tmpbufs = 3;
    }
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *rh_buf, *d_buf, *d_buf2;
    rh_buf = get_stream_buf(chunk->tmpbufs[0]);
    d_buf = NULL;
    d_buf2 = NULL;
    if (chunk->num_tmpbufs > 1) {
        d_buf = get_stream_buf(chunk->tmpbufs[1]);
    }
    if (chunk->num_tmpbufs > 2) {
        d_buf2 = get_stream_buf(chunk->tmpbufs[2]);
    }

    /* setup chunk for pack_h2d_stream_cb */
    chunk->inbuf = inbuf;
    chunk->outbuf = outbuf;
    chunk->op = op;
    chunk->info = reqpriv->info;
    chunk->count = count;
    chunk->count_offset = 0;

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        char *dbuf = (char *) outbuf + count_offset * type->size;

        rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, pack_h2d_stream_cb, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);

        if (op == YAKSA_OP__REPLACE) {
            rc = icopy(id, rh_buf, dbuf, chunk_count, type, reqpriv->info,
                       YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else if (buf_is_aligned(base_addr, type)) {
            rc = icopy(id, rh_buf, d_buf, chunk_count, type, reqpriv->info,
                       YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, d_buf, dbuf, chunk_count, type, reqpriv->info, op, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = icopy(id, rh_buf, d_buf, chunk_count, type, reqpriv->info,
                       YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, dbuf, d_buf2, chunk_count, type, reqpriv->info,
                       YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, d_buf, d_buf2, chunk_count, type, reqpriv->info, op, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = icopy(id, d_buf2, dbuf, chunk_count, type, reqpriv->info,
                       YAKSA_OP__REPLACE, out_device, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        }

        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* Subroutines for multi-chunk stream unpack
 *   unpack_d2d_p2p_stream
 *   unpack_d2d_nop2p_stream
 *   unpack_d2d_unaligned_stream
 *   unpack_rh2d_stream
 *   unpack_urh2d_stream
 *   unpack_d2h_stream_cb, unpack_d2h_stream
 */

static int unpack_d2d_p2p_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                                 uintptr_t count, yaksi_type_s * type, yaksa_op_t op, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int in_device = reqpriv->request->backend.inattr.device;
    int out_device = reqpriv->request->backend.outattr.device;
    assert(in_device != out_device);

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    int devices[] = { out_device };
    chunk->num_tmpbufs = 1;
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *d_buf;
    d_buf = get_stream_buf(chunk->tmpbufs[0]);

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        const char *sbuf = (const char *) inbuf + count_offset * type->size;
        char *dbuf = (char *) outbuf + count_offset * type->extent;

        rc = icopy(id, sbuf, d_buf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = iunpack(id, d_buf, dbuf, chunk_count, type, reqpriv->info, op, out_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_d2d_nop2p_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                                   uintptr_t count, yaksi_type_s * type, yaksa_op_t op,
                                   void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int in_device = reqpriv->request->backend.inattr.device;
    int out_device = reqpriv->request->backend.outattr.device;
    assert(in_device != out_device);

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    int devices[] = { out_device, -1 };
    chunk->num_tmpbufs = 2;
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *d_buf, *rh_buf;
    d_buf = get_stream_buf(chunk->tmpbufs[0]);
    rh_buf = get_stream_buf(chunk->tmpbufs[1]);

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        const char *sbuf = (const char *) inbuf + count_offset * type->size;
        char *dbuf = (char *) outbuf + count_offset * type->extent;

        rc = icopy(id, sbuf, rh_buf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, rh_buf, d_buf, chunk_count, type,
                   reqpriv->info, YAKSA_OP__REPLACE, out_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = iunpack(id, d_buf, dbuf, chunk_count, type, reqpriv->info, op, out_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_d2d_unaligned_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                                       uintptr_t count, yaksi_type_s * type, yaksa_op_t op,
                                       void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int in_device = reqpriv->request->backend.inattr.device;

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    int devices[] = { in_device };
    chunk->num_tmpbufs = 1;
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *d_buf;
    d_buf = get_stream_buf(chunk->tmpbufs[0]);

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        const char *sbuf = (const char *) inbuf + count_offset * type->size;
        char *dbuf = (char *) outbuf + count_offset * type->extent;

        rc = icopy(id, sbuf, d_buf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = iunpack(id, d_buf, dbuf, chunk_count, type, reqpriv->info, op, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int unpack_rh2d_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                              uintptr_t count, yaksi_type_s * type, yaksa_op_t op, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int out_device = reqpriv->request->backend.outattr.device;

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    int devices[] = { out_device };
    chunk->num_tmpbufs = 1;
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *d_buf;
    d_buf = get_stream_buf(chunk->tmpbufs[0]);

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        const char *sbuf = (const char *) inbuf + count_offset * type->size;
        char *dbuf = (char *) outbuf + count_offset * type->extent;

        rc = icopy(id, sbuf, d_buf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, out_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = iunpack(id, d_buf, dbuf, chunk_count, type, reqpriv->info, op, out_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static void unpack_urh2d_stream_cb(void *data)
{
    int rc ATTRIBUTE((unused));
    struct callback_chunk *chunk = data;

    yaksi_type_s *type = chunk->type;

    yaksi_type_s *byte_type;
    rc = yaksi_type_get(YAKSA_TYPE__BYTE, &byte_type);
    assert(rc == YAKSA_SUCCESS);

    const char *sbuf = (const char *) chunk->inbuf + chunk->count_offset * type->size;

    intptr_t count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    intptr_t chunk_count = YAKSU_MIN(count_per_chunk, chunk->count - chunk->count_offset);

    void *rh_buf;
    rh_buf = get_stream_buf(chunk->tmpbufs[1]);

    rc = yaksuri_seq_ipack(sbuf, rh_buf, chunk_count * type->size, byte_type,
                           chunk->info, YAKSA_OP__REPLACE);
    assert(rc == YAKSA_SUCCESS);
    chunk->count_offset += chunk_count;
}

static int unpack_urh2d_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                               uintptr_t count, yaksi_type_s * type, yaksa_op_t op, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int out_device = reqpriv->request->backend.outattr.device;

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    int devices[] = { out_device, -1 };
    chunk->num_tmpbufs = 2;
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *d_buf, *rh_buf;
    d_buf = get_stream_buf(chunk->tmpbufs[0]);
    rh_buf = get_stream_buf(chunk->tmpbufs[1]);

    /* setup chunk for unpack_urh2d_stream_cb */
    chunk->inbuf = inbuf;
    chunk->outbuf = outbuf;
    chunk->info = reqpriv->info;
    chunk->count = count;
    chunk->count_offset = 0;

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        char *dbuf = (char *) outbuf + count_offset * type->extent;

        rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, unpack_urh2d_stream_cb,
                                                               chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = icopy(id, rh_buf, d_buf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, out_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = iunpack(id, d_buf, dbuf, chunk_count, type, reqpriv->info, op, out_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static void unpack_d2h_stream_cb(void *data)
{
    struct callback_chunk *chunk = data;

    yaksa_op_t op = chunk->op;
    yaksi_type_s *type = chunk->type;

    char *dbuf = (char *) chunk->outbuf + chunk->count_offset * type->size;

    intptr_t count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    intptr_t chunk_count = YAKSU_MIN(count_per_chunk, chunk->count - chunk->count_offset);

    void *rh_buf;
    rh_buf = get_stream_buf(chunk->tmpbufs[0]);

    int rc ATTRIBUTE((unused));
    rc = yaksuri_seq_iunpack(rh_buf, dbuf, chunk_count, type, chunk->info, op);
    assert(rc == YAKSA_SUCCESS);
    chunk->count_offset += chunk_count;
}

static int unpack_d2h_stream(yaksuri_request_s * reqpriv, const void *inbuf, void *outbuf,
                             uintptr_t count, yaksi_type_s * type, yaksa_op_t op, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    int in_device = reqpriv->request->backend.inattr.device;

    struct callback_chunk *chunk = malloc(sizeof(struct callback_chunk));
    assert(chunk);

    chunk->type = type;
    yaksu_atomic_incr(&type->refcount);

    int devices[] = { -1 };
    chunk->num_tmpbufs = 1;
    rc = alloc_tmpbufs(id, chunk->tmpbufs, chunk->num_tmpbufs, devices, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    void *rh_buf;
    rh_buf = get_stream_buf(chunk->tmpbufs[0]);

    /* setup chunk for unpack_d2h_stream_cb */
    chunk->inbuf = inbuf;
    chunk->outbuf = outbuf;
    chunk->op = op;
    chunk->info = reqpriv->info;
    chunk->count = count;
    chunk->count_offset = 0;

    intptr_t count_per_chunk, count_offset;
    count_per_chunk = YAKSURI_TMPBUF_EL_SIZE / type->size;
    count_offset = 0;
    while (count_offset < count) {
        intptr_t chunk_count = YAKSU_MIN(count_per_chunk, count - count_offset);
        const char *sbuf = (const char *) inbuf + count_offset * type->size;

        rc = icopy(id, sbuf, rh_buf, chunk_count, type, reqpriv->info,
                   YAKSA_OP__REPLACE, in_device, stream);
        YAKSU_ERR_CHECK(rc, fn_fail);

        rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, unpack_d2h_stream_cb, chunk);
        YAKSU_ERR_CHECK(rc, fn_fail);

        count_offset += chunk_count;
    }
    tmpbufs_stream_release(chunk);
    rc = yaksuri_global.gpudriver[id].hooks->launch_hostfn(stream, chunk_free_stream_cb, chunk);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* Fast-path pack subroutine */
static int singlechunk_pack(yaksuri_gpudriver_id_e id, int device, const void *inbuf, void *outbuf,
                            uintptr_t count, yaksi_type_s * type, yaksi_info_s * info,
                            yaksa_op_t op, yaksi_request_s * request,
                            yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;

    rc = ipack(id, inbuf, outbuf, count, type, info, op, device, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (stream) {
        /* all done */
    } else if (request->kind == YAKSI_REQUEST_KIND__BLOCKING &&
               /* Try to complete immediately for blocking request to avoid overhead
                * caused by subreq enqueue and event management */
               yaksuri_global.gpudriver[id].hooks->synchronize) {
        yaksuri_global.gpudriver[id].hooks->synchronize(device);
    } else {
        yaksuri_subreq_s *subreq = (yaksuri_subreq_s *) malloc(sizeof(yaksuri_subreq_s));
        subreq->kind = YAKSURI_SUBREQ_KIND__SINGLE_CHUNK;

        rc = event_record(id, device, &subreq->u.single.event);
        *subreq_ptr = subreq;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* Fast-path unpack subroutine */
static int singlechunk_unpack(yaksuri_gpudriver_id_e id, int device, const void *inbuf,
                              void *outbuf, uintptr_t count, yaksi_type_s * type,
                              yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request,
                              yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;

    rc = iunpack(id, inbuf, outbuf, count, type, info, op, device, stream);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (stream) {
        /* all done */
    } else if (request->kind == YAKSI_REQUEST_KIND__BLOCKING &&
               /* Try to complete immediately for blocking request to avoid overhead
                * caused by subreq enqueue and event management */
               yaksuri_global.gpudriver[id].hooks->synchronize) {
        yaksuri_global.gpudriver[id].hooks->synchronize(device);
    } else {
        yaksuri_subreq_s *subreq = (yaksuri_subreq_s *) malloc(sizeof(yaksuri_subreq_s));
        subreq->kind = YAKSURI_SUBREQ_KIND__SINGLE_CHUNK;

        rc = event_record(id, device, &subreq->u.single.event);
        *subreq_ptr = subreq;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* determine if this is a device-to-device fastpath case */
static bool is_d2d_fastpath(yaksi_info_s * info, yaksa_op_t op,
                            yaksi_request_s * request, yaksuri_request_s * reqpriv, int *use_dev)
{
    /* all reduction operations on the same device */
    if (request->backend.inattr.device == request->backend.outattr.device) {
        *use_dev = reqpriv->request->backend.inattr.device;
        return true;
    } else if (op == YAKSA_OP__REPLACE) {
        int sdev = reqpriv->request->backend.inattr.device;
        int ddev = reqpriv->request->backend.outattr.device;

        if (info) {
            yaksuri_info_s *infopriv = (yaksuri_info_s *) info->backend.priv;
            if (infopriv->mapped_device >= 0 &&
                infopriv->mapped_device != reqpriv->request->backend.inattr.device) {
                sdev = infopriv->mapped_device;
                ddev = reqpriv->request->backend.inattr.device;
            }
        }

        /* REPLACE operations on devices with peer access enabled */
        if (check_p2p_comm(reqpriv->gpudriver_id, sdev, ddev)) {
            *use_dev = sdev;
            return true;
        }
    }

    return false;
}

/* Subroutines to setup subreq for pack with different buffer types
 *   set_subreq_pack_d2d
 *   set_subreq_pack_d2m
 *   set_subreq_pack_d2rh
 *   set_subreq_pack_from_device
 *   set_subreq_pack_from_managed
 *   set_subreq_pack_from_rhost
 */
static int set_subreq_pack_d2d(const void *inbuf, void *outbuf, uintptr_t count,
                               yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                               yaksi_request_s * request, yaksuri_request_s * reqpriv,
                               yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    bool aligned = buf_is_aligned(outbuf, type);

    int use_dev;
    if (aligned && is_d2d_fastpath(info, op, request, reqpriv, &use_dev)) {
        rc = singlechunk_pack(id, use_dev, inbuf, outbuf, count,
                              type, info, op, request, subreq_ptr, stream);
        goto fn_exit;
    }

    /* Slow paths */
    if (request->backend.inattr.device == request->backend.outattr.device) {
        if (stream) {
            rc = pack_d2d_unaligned_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            (*subreq_ptr)->u.multiple.acquire = pack_d2d_unaligned_acquire;
            (*subreq_ptr)->u.multiple.release = simple_release;
        }
    } else {
        bool is_enabled = check_p2p_comm(id, reqpriv->request->backend.inattr.device,
                                         reqpriv->request->backend.outattr.device);
        if (stream) {
            if (is_enabled) {
                rc = pack_d2d_p2p_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            } else {
                rc = pack_d2d_nop2p_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            }
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            if (is_enabled) {
                (*subreq_ptr)->u.multiple.acquire = pack_d2d_p2p_acquire;
            } else {
                (*subreq_ptr)->u.multiple.acquire = pack_d2d_nop2p_acquire;
            }
            (*subreq_ptr)->u.multiple.release = simple_release;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int set_subreq_pack_d2m(const void *inbuf, void *outbuf, uintptr_t count,
                               yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                               yaksi_request_s * request, yaksuri_request_s * reqpriv,
                               yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;

    /* Fast path for all reduce operations with aligned outbuf on host or the same device */
    if ((request->backend.outattr.device == -1 ||
         request->backend.inattr.device == request->backend.outattr.device) &&
        buf_is_aligned(outbuf, type)) {
        rc = singlechunk_pack(id, request->backend.inattr.device, inbuf, outbuf, count, type, info,
                              op, request, subreq_ptr, stream);
    }
    /* Slow paths */
    else if (request->backend.outattr.device == -1 ||
             request->backend.inattr.device == request->backend.outattr.device) {
        if (stream) {
            rc = pack_d2d_unaligned_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            (*subreq_ptr)->u.multiple.acquire = pack_d2d_unaligned_acquire;
            (*subreq_ptr)->u.multiple.release = simple_release;
        }
    } else {
        if (stream) {
            rc = pack_d2urh_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            (*subreq_ptr)->u.multiple.acquire = pack_d2urh_acquire;
            (*subreq_ptr)->u.multiple.release = pack_d2urh_release;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int set_subreq_pack_d2rh(const void *inbuf, void *outbuf, uintptr_t count,
                                yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                yaksi_request_s * request, yaksuri_request_s * reqpriv,
                                yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    uintptr_t threshold = yaksuri_global.gpudriver[id].hooks->get_iov_pack_threshold(info);

    /* Fast path for REPLACE with contig type or noncontig type with large contig chunk */
    if (op == YAKSA_OP__REPLACE && (type->is_contig || type->size / type->num_contig >= threshold)) {
        rc = singlechunk_pack(id, request->backend.inattr.device, inbuf, outbuf, count,
                              type, info, op, request, subreq_ptr, stream);
    }
    /* Slow path */
    else {
        if (stream) {
            rc = pack_d2rh_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            (*subreq_ptr)->u.multiple.acquire = pack_d2rh_acquire;
            (*subreq_ptr)->u.multiple.release = pack_d2rh_release;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int set_subreq_pack_from_device(const void *inbuf, void *outbuf, uintptr_t count,
                                       yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                       yaksi_request_s * request, yaksuri_request_s * reqpriv,
                                       yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;

    int ptr_type = request->backend.outattr.type;
    AVOID_UNREGISTERED_HOST(ptr_type, reqpriv->gpudriver_id);
    switch (ptr_type) {
        case YAKSUR_PTR_TYPE__GPU:
            rc = set_subreq_pack_d2d(inbuf, outbuf, count, type, info, op, request, reqpriv,
                                     subreq_ptr, stream);
            break;
        case YAKSUR_PTR_TYPE__MANAGED:
            rc = set_subreq_pack_d2m(inbuf, outbuf, count, type, info, op, request, reqpriv,
                                     subreq_ptr, stream);
            break;
        case YAKSUR_PTR_TYPE__REGISTERED_HOST:
            rc = set_subreq_pack_d2rh(inbuf, outbuf, count, type, info, op, request, reqpriv,
                                      subreq_ptr, stream);
            break;
        case YAKSUR_PTR_TYPE__UNREGISTERED_HOST:
        default:
            if (stream) {
                rc = pack_d2urh_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
                YAKSU_ERR_CHECK(rc, fn_fail);
            } else {
                rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
                YAKSU_ERR_CHECK(rc, fn_fail);

                (*subreq_ptr)->u.multiple.acquire = pack_d2urh_acquire;
                (*subreq_ptr)->u.multiple.release = pack_d2urh_release;
            }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int set_subreq_pack_from_managed(const void *inbuf, void *outbuf, uintptr_t count,
                                        yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                        yaksi_request_s * request, yaksuri_request_s * reqpriv,
                                        yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;

    /* Fast path for all reduce operations with aligned outbuf from host or the same device. */
    if ((request->backend.inattr.device == -1 ||
         request->backend.inattr.device == request->backend.outattr.device) &&
        buf_is_aligned(outbuf, type)) {
        rc = singlechunk_pack(id, request->backend.outattr.device, inbuf, outbuf, count, type, info,
                              op, request, subreq_ptr, stream);
    }
    /* Slow path */
    else {
        if (stream) {
            rc = pack_h2d_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            (*subreq_ptr)->u.multiple.acquire = pack_h2d_acquire;
            (*subreq_ptr)->u.multiple.release = simple_release;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int set_subreq_pack_from_rhost(const void *inbuf, void *outbuf, uintptr_t count,
                                      yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                      yaksi_request_s * request, yaksuri_request_s * reqpriv,
                                      yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    uintptr_t threshold = yaksuri_global.gpudriver[id].hooks->get_iov_pack_threshold(info);

    /* Fast path for REPLACE with contig type or noncontig type with large contig chunk */
    if (op == YAKSA_OP__REPLACE && (type->is_contig || type->size / type->num_contig >= threshold)) {
        rc = singlechunk_pack(id, request->backend.outattr.device, inbuf, outbuf, count,
                              type, info, op, request, subreq_ptr, stream);
    }
    /* Slow path */
    else {
        if (stream) {
            rc = pack_h2d_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            (*subreq_ptr)->u.multiple.acquire = pack_h2d_acquire;
            (*subreq_ptr)->u.multiple.release = simple_release;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* Subroutines to setup subreq for unpack with different buffer types
 *   set_subreq_unpack_d2d
 *   set_subreq_unpack_d2m
 *   set_subreq_unpack_d2rh
 *   set_subreq_unpack_from_device
 *   set_subreq_unpack_from_managed
 *   set_subreq_unpack_from_rhost
 */
static int set_subreq_unpack_d2d(const void *inbuf, void *outbuf, uintptr_t count,
                                 yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                 yaksi_request_s * request, yaksuri_request_s * reqpriv,
                                 yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    bool aligned = buf_is_aligned(inbuf, type);

    int use_dev;
    if (aligned && is_d2d_fastpath(info, op, request, reqpriv, &use_dev)) {
        rc = singlechunk_unpack(id, use_dev, inbuf, outbuf, count,
                                type, info, op, request, subreq_ptr, stream);
        goto fn_exit;
    }

    /* Slow paths */
    if (request->backend.inattr.device == request->backend.outattr.device) {
        if (stream) {
            rc = unpack_d2d_unaligned_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            (*subreq_ptr)->u.multiple.acquire = unpack_d2d_unaligned_acquire;
            (*subreq_ptr)->u.multiple.release = simple_release;
        }
    } else {
        bool is_enabled = check_p2p_comm(id, reqpriv->request->backend.inattr.device,
                                         reqpriv->request->backend.outattr.device);
        if (stream) {
            if (is_enabled) {
                rc = unpack_d2d_p2p_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            } else {
                rc = unpack_d2d_nop2p_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            }
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            if (is_enabled) {
                (*subreq_ptr)->u.multiple.acquire = unpack_d2d_p2p_acquire;
            } else {
                (*subreq_ptr)->u.multiple.acquire = unpack_d2d_nop2p_acquire;
            }
            (*subreq_ptr)->u.multiple.release = simple_release;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int set_subreq_unpack_d2m(const void *inbuf, void *outbuf, uintptr_t count,
                                 yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                 yaksi_request_s * request, yaksuri_request_s * reqpriv,
                                 yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;

    /* Fast path for all reduce operations with aligned inbuf to host or the same device */
    if ((request->backend.outattr.device == -1 ||
         request->backend.inattr.device == request->backend.outattr.device) &&
        buf_is_aligned(inbuf, type)) {
        rc = singlechunk_unpack(id, request->backend.inattr.device, inbuf, outbuf, count,
                                type, info, op, request, subreq_ptr, stream);
    }
    /* Slow path */
    else {
        if (stream) {
            rc = unpack_d2h_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            (*subreq_ptr)->u.multiple.acquire = unpack_d2h_acquire;
            (*subreq_ptr)->u.multiple.release = unpack_d2h_release;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int set_subreq_unpack_d2rh(const void *inbuf, void *outbuf, uintptr_t count,
                                  yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                  yaksi_request_s * request, yaksuri_request_s * reqpriv,
                                  yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    uintptr_t threshold = yaksuri_global.gpudriver[id].hooks->get_iov_unpack_threshold(info);

    /* Fast path for REPLACE with contig type or noncontig type with large contig chunk */
    if (op == YAKSA_OP__REPLACE && (type->is_contig || type->size / type->num_contig >= threshold)) {
        rc = singlechunk_unpack(id, request->backend.inattr.device, inbuf, outbuf, count,
                                type, info, op, request, subreq_ptr, stream);
    }
    /* Slow path */
    else {
        if (stream) {
            rc = unpack_d2h_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            (*subreq_ptr)->u.multiple.acquire = unpack_d2h_acquire;
            (*subreq_ptr)->u.multiple.release = unpack_d2h_release;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int set_subreq_unpack_from_device(const void *inbuf, void *outbuf, uintptr_t count,
                                         yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                         yaksi_request_s * request, yaksuri_request_s * reqpriv,
                                         yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;

    int ptr_type = request->backend.outattr.type;
    AVOID_UNREGISTERED_HOST(ptr_type, reqpriv->gpudriver_id);
    switch (ptr_type) {
        case YAKSUR_PTR_TYPE__GPU:
            rc = set_subreq_unpack_d2d(inbuf, outbuf, count, type, info, op, request, reqpriv,
                                       subreq_ptr, stream);
            break;
        case YAKSUR_PTR_TYPE__MANAGED:
            rc = set_subreq_unpack_d2m(inbuf, outbuf, count, type, info, op, request, reqpriv,
                                       subreq_ptr, stream);
            break;
        case YAKSUR_PTR_TYPE__REGISTERED_HOST:
            rc = set_subreq_unpack_d2rh(inbuf, outbuf, count, type, info, op, request, reqpriv,
                                        subreq_ptr, stream);
            break;
        case YAKSUR_PTR_TYPE__UNREGISTERED_HOST:
        default:
            if (stream) {
                rc = unpack_d2h_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
                YAKSU_ERR_CHECK(rc, fn_fail);
            } else {
                rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
                YAKSU_ERR_CHECK(rc, fn_fail);

                (*subreq_ptr)->u.multiple.acquire = unpack_d2h_acquire;
                (*subreq_ptr)->u.multiple.release = unpack_d2h_release;
            }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int set_subreq_unpack_from_managed(const void *inbuf, void *outbuf, uintptr_t count,
                                          yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                          yaksi_request_s * request, yaksuri_request_s * reqpriv,
                                          yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;

    /* Fast path for all reduce operations with aligned inbuf from host or the same device. */
    if ((request->backend.inattr.device == -1 ||
         request->backend.inattr.device == request->backend.outattr.device) &&
        buf_is_aligned(inbuf, type)) {
        rc = singlechunk_unpack(id, request->backend.outattr.device, inbuf, outbuf, count,
                                type, info, op, request, subreq_ptr, stream);
    }
    /* Slow path */
    else {
        if (stream) {
            rc = unpack_urh2d_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            (*subreq_ptr)->u.multiple.acquire = unpack_urh2d_acquire;
            (*subreq_ptr)->u.multiple.release = simple_release;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

static int set_subreq_unpack_from_rhost(const void *inbuf, void *outbuf, uintptr_t count,
                                        yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                        yaksi_request_s * request, yaksuri_request_s * reqpriv,
                                        yaksuri_subreq_s ** subreq_ptr, void *stream)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;
    uintptr_t threshold = yaksuri_global.gpudriver[id].hooks->get_iov_unpack_threshold(info);

    /* Fast path for REPLACE with contig type or noncontig type with large contig chunk */
    if (op == YAKSA_OP__REPLACE && (type->is_contig || type->size / type->num_contig >= threshold)) {
        rc = singlechunk_unpack(id, request->backend.outattr.device, inbuf, outbuf, count,
                                type, info, op, request, subreq_ptr, stream);
    }
    /* Slow path */
    else {
        if (stream) {
            rc = unpack_rh2d_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, subreq_ptr);
            YAKSU_ERR_CHECK(rc, fn_fail);

            (*subreq_ptr)->u.multiple.acquire = unpack_rh2d_acquire;
            (*subreq_ptr)->u.multiple.release = simple_release;
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* Progress functions */

int yaksuri_progress_enqueue(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                             yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_subreq_s *subreq = NULL;
    yaksuri_request_s *reqpriv = (yaksuri_request_s *) request->backend.priv;
    yaksuri_gpudriver_id_e id = reqpriv->gpudriver_id;

    assert(yaksuri_global.gpudriver[id].hooks);
    assert(request->backend.inattr.type == YAKSUR_PTR_TYPE__GPU ||
           request->backend.outattr.type == YAKSUR_PTR_TYPE__GPU);

    reqpriv->info = info;

    /* if the GPU reqpriv cannot support this type, return */
    bool is_supported;
    rc = yaksuri_global.gpudriver[id].hooks->pup_is_supported(type, op, &is_supported);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (!is_supported) {
        rc = YAKSA_ERR__NOT_SUPPORTED;
        goto fn_exit;
    }

    void *stream;
    if (request->kind == YAKSI_REQUEST_KIND__GPU_STREAM) {
        assert(id == YAKSURI_GPUDRIVER_ID__CUDA || id == YAKSURI_GPUDRIVER_ID__HIP);
        assert(request->stream != NULL);
        stream = request->stream;
    } else {
        stream = NULL;
    }

    int ptr_type;
    ptr_type = request->backend.inattr.type;
    AVOID_UNREGISTERED_HOST(ptr_type, reqpriv->gpudriver_id);
    if (reqpriv->optype == YAKSURI_OPTYPE__PACK) {
        switch (ptr_type) {
            case YAKSUR_PTR_TYPE__GPU:
                rc = set_subreq_pack_from_device(inbuf, outbuf, count, type, info, op, request,
                                                 reqpriv, &subreq, stream);
                YAKSU_ERR_CHECK(rc, fn_fail);
                break;
            case YAKSUR_PTR_TYPE__MANAGED:
                rc = set_subreq_pack_from_managed(inbuf, outbuf, count, type, info, op, request,
                                                  reqpriv, &subreq, stream);
                YAKSU_ERR_CHECK(rc, fn_fail);
                break;
            case YAKSUR_PTR_TYPE__REGISTERED_HOST:
                rc = set_subreq_pack_from_rhost(inbuf, outbuf, count, type, info, op, request,
                                                reqpriv, &subreq, stream);
                YAKSU_ERR_CHECK(rc, fn_fail);
                break;
            case YAKSUR_PTR_TYPE__UNREGISTERED_HOST:
            default:
                if (stream) {
                    rc = pack_h2d_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
                    YAKSU_ERR_CHECK(rc, fn_fail);
                } else {
                    rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, &subreq);
                    YAKSU_ERR_CHECK(rc, fn_fail);

                    subreq->u.multiple.acquire = pack_h2d_acquire;
                    subreq->u.multiple.release = simple_release;
                }
                break;
        }
    } else {
        switch (ptr_type) {
            case YAKSUR_PTR_TYPE__GPU:
                rc = set_subreq_unpack_from_device(inbuf, outbuf, count, type, info, op, request,
                                                   reqpriv, &subreq, stream);
                YAKSU_ERR_CHECK(rc, fn_fail);
                break;
            case YAKSUR_PTR_TYPE__MANAGED:
                rc = set_subreq_unpack_from_managed(inbuf, outbuf, count, type, info, op, request,
                                                    reqpriv, &subreq, stream);
                YAKSU_ERR_CHECK(rc, fn_fail);
                break;
            case YAKSUR_PTR_TYPE__REGISTERED_HOST:
                rc = set_subreq_unpack_from_rhost(inbuf, outbuf, count, type, info, op, request,
                                                  reqpriv, &subreq, stream);
                YAKSU_ERR_CHECK(rc, fn_fail);
                break;
            case YAKSUR_PTR_TYPE__UNREGISTERED_HOST:
            default:
                if (stream) {
                    rc = unpack_urh2d_stream(reqpriv, inbuf, outbuf, count, type, op, stream);
                    YAKSU_ERR_CHECK(rc, fn_fail);
                } else {
                    rc = set_multichunk_subreq(inbuf, outbuf, count, type, op, &subreq);
                    YAKSU_ERR_CHECK(rc, fn_fail);

                    subreq->u.multiple.acquire = unpack_urh2d_acquire;
                    subreq->u.multiple.release = simple_release;
                }
                break;
        }
    }

    /* If pack/unpack is already completed, no subreq is returned. Thus, skip enqueue. */
    if (!subreq) {
        goto fn_exit;
    }

    /* id (i.e., reqpriv->gpudriver_id) may differ between different calls to yaksuri_progress_enqueue() */
    subreq->gpudriver_id = id;

    pthread_mutex_lock(&progress_mutex);
    DL_APPEND(reqpriv->subreqs, subreq);

    /* if the request is not in our pending list, add it */
    yaksuri_request_s *req;
    HASH_FIND_PTR(pending_reqs, &request, req);
    if (req == NULL) {
        HASH_ADD_PTR(pending_reqs, request, reqpriv);
        yaksu_atomic_incr(&request->cc);
    }
    pthread_mutex_unlock(&progress_mutex);

    rc = yaksuri_progress_poke();
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    free(subreq);
    goto fn_exit;
}

int yaksuri_progress_poke(void)
{
    int rc = YAKSA_SUCCESS;
    yaksuri_gpudriver_id_e id = YAKSURI_GPUDRIVER_ID__UNSET;

    /* A progress poke is in two steps.  In the first step, we check
     * for event completions, finish any post-processing and retire
     * any temporary resources.  In the second steps, we issue out any
     * pending operations. */

    pthread_mutex_lock(&progress_mutex);

    yaksi_type_s *byte_type;
    rc = yaksi_type_get(YAKSA_TYPE__BYTE, &byte_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    /**********************************************************************/
    /* Step 1: Check for completions */
    /**********************************************************************/
    yaksuri_request_s *reqpriv, *tmp;
    HASH_ITER(hh, pending_reqs, reqpriv, tmp) {
        assert(reqpriv->subreqs);

        yaksuri_subreq_s *subreq, *tmp2;
        DL_FOREACH_SAFE(reqpriv->subreqs, subreq, tmp2) {
            id = subreq->gpudriver_id;
            if (subreq->kind == YAKSURI_SUBREQ_KIND__SINGLE_CHUNK) {
                int completed;
                rc = event_query(id, subreq->u.single.event, &completed);
                YAKSU_ERR_CHECK(rc, fn_fail);

                if (!completed)
                    continue;

                DL_DELETE(reqpriv->subreqs, subreq);
                free(subreq);
                if (reqpriv->subreqs == NULL) {
                    HASH_DEL(pending_reqs, reqpriv);
                    yaksu_atomic_decr(&reqpriv->request->cc);
                }
            } else {
                yaksuri_subreq_chunk_s *chunk, *tmp3;
                DL_FOREACH_SAFE(subreq->u.multiple.chunks, chunk, tmp3) {
                    int completed;
                    rc = event_query(id, chunk->event, &completed);
                    YAKSU_ERR_CHECK(rc, fn_fail);

                    if (!completed)
                        continue;

                    rc = subreq->u.multiple.release(reqpriv, subreq, chunk);
                    YAKSU_ERR_CHECK(rc, fn_fail);
                }
            }
        }
    }

    /**********************************************************************/
    /* Step 2: Issue new operations */
    /**********************************************************************/
    HASH_ITER(hh, pending_reqs, reqpriv, tmp) {
        assert(reqpriv->subreqs);

        yaksuri_subreq_s *subreq, *tmp2;
        DL_FOREACH_SAFE(reqpriv->subreqs, subreq, tmp2) {
            id = subreq->gpudriver_id;
            if (subreq->kind == YAKSURI_SUBREQ_KIND__SINGLE_CHUNK)
                continue;

            while (subreq->u.multiple.issued_count < subreq->u.multiple.count) {
                yaksuri_subreq_chunk_s *chunk;

                rc = subreq->u.multiple.acquire(reqpriv, subreq, &chunk);
                YAKSU_ERR_CHECK(rc, fn_fail);

                if (chunk == NULL)
                    goto flush;

                subreq->u.multiple.issued_count += chunk->count;
            }
        }
    }

  flush:
    /* if we issued any operations, call flush_all, so the driver
     * layer can flush the kernels. */
    if (id != YAKSURI_GPUDRIVER_ID__UNSET) {
        rc = yaksuri_global.gpudriver[id].hooks->flush_all();
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    pthread_mutex_unlock(&progress_mutex);
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksuri_progress_init(void)
{
    return YAKSA_SUCCESS;
}

int yaksuri_progress_finalize(void)
{
    stream_buf_list_free();
    return YAKSA_SUCCESS;
}
