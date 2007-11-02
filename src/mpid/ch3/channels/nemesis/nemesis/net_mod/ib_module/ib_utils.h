/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef _IB_UTILS_H
#define _IB_UTILS_H

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

#define MPID_NEM_IB_UD_QPN_KEY      "ud_qp_key"
#define MPID_NEM_IB_LID_KEY         "lid_key"
#define MPID_NEM_IB_GUID_KEY        "guid_key"

#define MPID_NEM_IB_BUF_READ_WRITE  (123)
#define MPID_NEM_IB_BUF_READ_ONLY   (321)

#define MPID_NEM_MAX_HOSTNAME_LEN   (256)

/* A description of the process
 * is kept in this data structure.
 * Its useful to dereference when
 * printing our error/debug messages
 */

typedef struct {
    char            hostname[MPID_NEM_MAX_HOSTNAME_LEN];
    int             rank;
} MPID_nem_ib_module_proc_desc_t;

MPID_nem_ib_module_proc_desc_t me;


/*
 * Simple data structures for a Hash
 * table implementation
 */

typedef struct _hash_elem {
    void                    *data;
    uint32_t                uniq;
    uint64_t                key;
    struct _hash_elem       *prev;
    struct _hash_elem       *next;
} MPID_nem_ib_module_hash_elem_t, *MPID_nem_ib_module_hash_elem_ptr_t;

typedef struct {
    MPID_nem_ib_module_hash_elem_ptr_t      entries;
    uint32_t                                num_entries;
    pthread_mutex_t                         hash_table_lock;
} MPID_nem_ib_module_hash_table_t, *MPID_nem_ib_module_hash_table_ptr_t;

typedef struct _ib_queue_elem {
    void                    *data;
    struct _ib_queue_elem   *next;
} MPID_nem_ib_module_queue_elem_t, *MPID_nem_ib_module_queue_elem_ptr_t;

typedef struct _ib_queue {
    MPID_nem_ib_module_queue_elem_t *head;
    MPID_nem_ib_module_queue_elem_t *tail;
    MPID_nem_ib_module_queue_elem_t *free_queue;
} MPID_nem_ib_module_queue_t, *MPID_nem_ib_module_queue_ptr_t;

extern MPID_nem_ib_module_queue_ptr_t MPID_nem_ib_module_vc_queue;

int MPID_nem_ib_module_init_hash_table(
        MPID_nem_ib_module_hash_table_ptr_t table,
        uint32_t nentries);

int MPID_nem_ib_module_insert_hash_elem(
        MPID_nem_ib_module_hash_table_ptr_t table,
        uint64_t key,
        void *data,
        uint32_t len);

void* MPID_nem_ib_module_lookup_hash_table(
        MPID_nem_ib_module_hash_table_ptr_t table,
        uint64_t key, uint32_t unique_id);

void MPID_nem_ib_module_finalize_hash_table(
        MPID_nem_ib_module_hash_table_ptr_t table);

int MPID_nem_ib_module_queue_init(MPID_nem_ib_module_queue_t**);

int MPID_nem_ib_module_queue_new_elem(
        MPID_nem_ib_module_queue_elem_t **, void *init_ptr);

int MPID_nem_ib_module_queue_empty(MPID_nem_ib_module_queue_t *q);

void MPID_nem_ib_module_queue_dequeue(
        MPID_nem_ib_module_queue_t *q,
        MPID_nem_ib_module_queue_elem_t **e);

void MPID_nem_ib_module_queue_enqueue(
        MPID_nem_ib_module_queue_t *q,
        MPID_nem_ib_module_queue_elem_t *e);

void MPID_nem_ib_module_queue_free(
        MPID_nem_ib_module_queue_t *q,
        MPID_nem_ib_module_queue_elem_t *e);

int MPID_nem_ib_module_queue_alloc(
        MPID_nem_ib_module_queue_t *q,
        MPID_nem_ib_module_queue_elem_t **e);

void MPID_nem_ib_module_queue_finalize(
        MPID_nem_ib_module_queue_t *q);

#define INIT_NEM_IB_PROC_DESC(_rank) {                          \
    if(gethostname(me.hostname, MPID_NEM_MAX_HOSTNAME_LEN)) {   \
        perror("gethostname");                                  \
    }                                                           \
    me.rank = _rank;                                            \
};


#define NEM_IB_ERR(message, args...) {                          \
    MPIU_Internal_error_printf("[%s:%d] [%s:%d] ",              \
            me.hostname, me.rank,                               \
            __FILE__, __LINE__);                                \
    MPIU_Internal_error_printf(message, ##args);                \
    MPIU_Internal_error_printf("\n");                           \
}

#ifdef DEBUG_NEM_IB
#define NEM_IB_DBG(message, args...) {                          \
    MPIU_Msg_printf("[%s:%d] [%s:%d] ", me.hostname, me.rank,   \
            __FILE__, __LINE__);                                \
    MPIU_Msg_printf(message, ##args);                           \
    MPIU_Msg_printf("\n");                                      \
}
#else
#define NEM_IB_DBG(message, args...)
#endif

#endif  /* _IB_UTILS_H */
