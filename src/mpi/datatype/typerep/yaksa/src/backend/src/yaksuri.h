/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSURI_H_INCLUDED
#define YAKSURI_H_INCLUDED

#include "yaksi.h"

typedef enum yaksuri_gpudriver_id_e {
    YAKSURI_GPUDRIVER_ID__UNSET = -1,
    YAKSURI_GPUDRIVER_ID__CUDA = 0,
    YAKSURI_GPUDRIVER_ID__ZE,
    YAKSURI_GPUDRIVER_ID__HIP,
    YAKSURI_GPUDRIVER_ID__LAST,
} yaksuri_gpudriver_id_e;

typedef enum yaksuri_pup_e {
    YAKSURI_OPTYPE__UNSET,
    YAKSURI_OPTYPE__PACK,
    YAKSURI_OPTYPE__UNPACK,
} yaksuri_optype_e;

#define YAKSURI_TMPBUF_EL_SIZE  (1024 * 1024)
#define YAKSURI_TMPBUF_NUM_EL   (16)

typedef struct {
    bool has_wait_kernel;
    struct {
        yaksu_buffer_pool_s host;
        yaksu_buffer_pool_s *device;
        yaksur_gpudriver_hooks_s *hooks;
        int ndevices;
    } gpudriver[YAKSURI_GPUDRIVER_ID__LAST];
} yaksuri_global_s;
extern yaksuri_global_s yaksuri_global;

#define YAKSURI_SUBREQ_CHUNK_MAX_TMPBUFS (4)

typedef struct yaksuri_tmpbuf {
    void *buf;
    yaksu_buffer_pool_s pool;
} yaksuri_tmpbuf_s;

typedef struct yaksuri_subreq_chunk {
    uintptr_t count_offset;
    uintptr_t count;

    int num_tmpbufs;
    yaksuri_tmpbuf_s tmpbufs[YAKSURI_SUBREQ_CHUNK_MAX_TMPBUFS];
    void *event;

    struct yaksuri_subreq_chunk *next;
    struct yaksuri_subreq_chunk *prev;
} yaksuri_subreq_chunk_s;

struct yaksuri_request;
typedef struct yaksuri_subreq {
    enum {
        YAKSURI_SUBREQ_KIND__SINGLE_CHUNK,
        YAKSURI_SUBREQ_KIND__MULTI_CHUNK,
    } kind;

    union {
        struct {
            void *event;
        } single;
        struct {
            const void *inbuf;
            void *outbuf;
            uintptr_t count;
            yaksi_type_s *type;
            yaksa_op_t op;

            uintptr_t issued_count;
            yaksuri_subreq_chunk_s *chunks;

            int (*acquire) (struct yaksuri_request * reqpriv, struct yaksuri_subreq * subreq,
                            struct yaksuri_subreq_chunk ** chunk);
            int (*release) (struct yaksuri_request * reqpriv, struct yaksuri_subreq * subreq,
                            struct yaksuri_subreq_chunk * chunk);
        } multiple;
    } u;

    yaksuri_gpudriver_id_e gpudriver_id;

    struct yaksuri_subreq *next;
    struct yaksuri_subreq *prev;
} yaksuri_subreq_s;

typedef struct yaksuri_request {
    yaksi_request_s *request;

    yaksi_info_s *info;
    yaksuri_optype_e optype;

    yaksuri_gpudriver_id_e gpudriver_id;

    yaksuri_subreq_s *subreqs;

    UT_hash_handle hh;
} yaksuri_request_s;

typedef struct {
    yaksuri_gpudriver_id_e gpudriver_id;
    int mapped_device;
    bool has_wait_kernel;       /* avoid gpu functions that may cause deadlocks with wait kernel */
} yaksuri_info_s;

int yaksuri_progress_enqueue(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                             yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request);
int yaksuri_progress_poke(void);
int yaksuri_progress_init(void);
int yaksuri_progress_finalize(void);

#endif /* YAKSURI_H_INCLUDED */
