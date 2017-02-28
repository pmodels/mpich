/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_QUEUE_TYPES_H
#define _ZM_QUEUE_TYPES_H
#include "common/zm_common.h"
#include <pthread.h>
#include <limits.h>

/* glqueue*/
typedef struct zm_glqueue zm_glqueue_t;
typedef struct zm_glqnode zm_glqnode_t;

struct zm_glqnode {
    void *data ZM_ALLIGN_TO_CACHELINE;
    zm_ptr_t next;
};

struct zm_glqueue {
    pthread_mutex_t lock;
    zm_ptr_t head ZM_ALLIGN_TO_CACHELINE;
    zm_ptr_t tail ZM_ALLIGN_TO_CACHELINE;
};

/* swpqueue */
typedef struct zm_msqueue zm_swpqueue_t;
typedef struct zm_msqnode zm_swpqnode_t;

/* msqueue */
typedef struct zm_msqueue zm_msqueue_t;
typedef struct zm_msqnode zm_msqnode_t;

struct zm_msqnode {
    void *data ZM_ALLIGN_TO_CACHELINE;
    zm_atomic_ptr_t next;
};

struct zm_msqueue {
    zm_atomic_ptr_t head ZM_ALLIGN_TO_CACHELINE;
    zm_atomic_ptr_t tail ZM_ALLIGN_TO_CACHELINE;
};

/* faqueue */

#define ZM_MAX_FAQUEUE_SIZE     ULONG_MAX
#define ZM_MAX_FASEG_SIZE       1024
#define ZM_FAQUEUE_ALPHA        (void*)ULLONG_MAX /* meaning: queue cell empty */

typedef struct zm_faseg     zm_faseg_t;
typedef struct zm_faqueue   zm_faqueue_t;
typedef struct zm_facell    zm_facell_t;

/* data inside a cell */
struct zm_facell {
    void* data ZM_ALLIGN_TO_CACHELINE;
};

/* segment */
struct zm_faseg {
    zm_ulong_t id;
    zm_facell_t cells[ZM_MAX_FASEG_SIZE];
    zm_atomic_ptr_t next;
};

struct zm_faqueue {
    zm_ulong_t          head ZM_ALLIGN_TO_CACHELINE;
    zm_atomic_ulong_t   tail ZM_ALLIGN_TO_CACHELINE;
    zm_ptr_t            seg_head;
    zm_atomic_ptr_t     seg_tail;
};

#endif /* _ZM_QUEUE_TYPES_H */
