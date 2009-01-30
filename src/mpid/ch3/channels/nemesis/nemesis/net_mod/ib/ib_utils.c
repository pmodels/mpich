/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"
#include "ib_utils.h"


/* hash - A simple hash function
 *  Too simple really, but this
 *  works great for now
 */

static uint32_t hash(uint64_t key, uint32_t size)
{
    uint32_t hash_value;

    hash_value = key % size;

    return hash_value;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_init_hash_table
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_init_hash_table(
        MPID_nem_ib_hash_table_ptr_t table,
        uint32_t nentries)
{
    int mpi_errno = MPI_SUCCESS;

    table->entries = MPIU_Malloc(
            sizeof(MPID_nem_ib_hash_elem_t) * nentries);
    table->num_entries = nentries;

    if(NULL == table->entries) {
        MPIU_CHKMEM_SETERR(mpi_errno, 
                sizeof(MPID_nem_ib_hash_elem_t) * nentries, 
                "IB Module Hash Table");
    }

    memset(table->entries, 0,
            sizeof(MPID_nem_ib_hash_elem_t) * nentries);

    pthread_mutex_init(&table->hash_table_lock, NULL);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_insert_hash_elem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_insert_hash_elem(
        MPID_nem_ib_hash_table_ptr_t table,
        uint64_t key, void *data, uint32_t uniq)
{
    int mpi_errno = MPI_SUCCESS;
    uint32_t hash_index;
    MPID_nem_ib_hash_elem_ptr_t start_elem;
    MPID_nem_ib_hash_elem_ptr_t elem, new_elem;

    MPIU_Assert(NULL != table);

    pthread_mutex_lock(&table->hash_table_lock);

    hash_index = hash(key, table->num_entries);

    /* Note that the first element is allocated
     * at the beginning, so this is guaranteed
     * to be non-null */
    start_elem = &table->entries[hash_index];

    MPIU_Assert(elem != NULL);

    /* Walk to end of list in this hash slot */
    elem = start_elem;
    while(elem->next != NULL) {
        elem = elem->next;
    }

    /* Insert the element */
    new_elem = MPIU_Malloc(sizeof(MPID_nem_ib_hash_elem_t));

    if(NULL == new_elem) {
        MPIU_CHKMEM_SETERR(mpi_errno, 
                sizeof(MPID_nem_ib_hash_elem_t), 
                "IB Module Hash Table New Element");
    }

    memset(new_elem, 0, sizeof(MPID_nem_ib_hash_elem_t));

    new_elem->data = data;
    new_elem->uniq = uniq;
    new_elem->key = key;
    new_elem->next = NULL;
    new_elem->prev = elem;

    elem->next = new_elem;

    NEM_IB_DBG("Inserted elem key %lu, uniq %u, hash index %u", 
            key, uniq, hash_index);

    pthread_mutex_unlock(&table->hash_table_lock);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_lookup_hash_table
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

void* MPID_nem_ib_lookup_hash_table(
        MPID_nem_ib_hash_table_ptr_t table,
        uint64_t key, uint32_t unique_id)
{
    uint32_t hash_index;
    void *data = NULL;
    MPID_nem_ib_hash_elem_ptr_t start_elem;
    MPID_nem_ib_hash_elem_ptr_t elem;

    pthread_mutex_lock(&table->hash_table_lock);

    hash_index = hash(key, table->num_entries);

    NEM_IB_DBG("Got hash_index %u, key %lu, uniq %u", 
            hash_index, key, unique_id);

    start_elem = &table->entries[hash_index];

    NEM_IB_DBG("");

    /* The first element is just a place holder */
    elem = start_elem->next;

    while(elem != NULL) {

        if((elem->key == key) && (elem->uniq == unique_id)) {
            data = elem->data;
            break;
        }
        elem = elem->next;
    }

    pthread_mutex_unlock(&table->hash_table_lock);

    return data;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_finalize_hash_table
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

void MPID_nem_ib_finalize_hash_table(
        MPID_nem_ib_hash_table_ptr_t table)
{
    int i;
    MPID_nem_ib_hash_elem_ptr_t start_elem;
    MPID_nem_ib_hash_elem_ptr_t elem, next_elem;

    pthread_mutex_lock(&table->hash_table_lock);

    MPIU_Assert(table->entries != NULL);

    for(i = 0; i < table->num_entries; i++) {

        start_elem = &table->entries[i];

        /* Walk through the list freeing
         * elements as we go */
        elem = start_elem->next;

        while(elem != NULL) {
            next_elem = elem->next;
            MPIU_Free(elem);
            elem = next_elem;
        }
    }

    pthread_mutex_unlock(&table->hash_table_lock);
    
    MPIU_Free(table->entries);
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_queue_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_queue_init(
        MPID_nem_ib_queue_t **q)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIU_Assert(NULL != q);

    *q = MPIU_Malloc(sizeof(MPID_nem_ib_queue_t));

    if(NULL == *q) {
        MPIU_CHKMEM_SETERR(mpi_errno, 
                sizeof(MPID_nem_ib_queue_t), 
                "IB Module Queue");
    }

    memset(*q, 0, sizeof(MPID_nem_ib_queue_t));

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_queue_new_elem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_queue_new_elem(
        MPID_nem_ib_queue_elem_t **e, void *init_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    (*e) = MPIU_Malloc(sizeof(MPID_nem_ib_queue_elem_t));

    if (NULL == *e) {
        MPIU_CHKMEM_SETERR (mpi_errno, 
                sizeof(MPID_nem_ib_queue_elem_t), 
                "IB module queue elem");
    }

    (*e)->data = init_ptr;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_queue_empty
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_queue_empty(
        MPID_nem_ib_queue_t *q)
{
    return (NULL == q->head);
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_queue_dequeue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

void MPID_nem_ib_queue_dequeue(
        MPID_nem_ib_queue_t *q,
        MPID_nem_ib_queue_elem_t **e)
{
    *e = q->head;

    if(*e) {
        q->head = q->head->next;

        if(NULL == q->head) {
            q->tail = NULL;
        }
    }
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_queue_enqueue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

void MPID_nem_ib_queue_enqueue(
        MPID_nem_ib_queue_t *q,
        MPID_nem_ib_queue_elem_t *e)
{
    if(NULL == q->tail) {
        q->head = e;
    } else {
        q->tail->next = e;
    }

    q->tail = e;
    e->next = NULL;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_queue_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

void MPID_nem_ib_queue_free(
        MPID_nem_ib_queue_t *q,
        MPID_nem_ib_queue_elem_t *e)
{
    e->next = q->free_queue;
    q->free_queue = e;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_queue_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_queue_alloc(
        MPID_nem_ib_queue_t *q,
        MPID_nem_ib_queue_elem_t **e)
{
    int mpi_errno = MPI_SUCCESS;

    if(q->free_queue) {
        *e = q->free_queue;
        q->free_queue = q->free_queue->next;
    } else {
        *e = MPIU_Malloc(sizeof(MPID_nem_ib_queue_elem_t));
        if(NULL == *e) {
            MPIU_CHKMEM_SETERR(mpi_errno, 
                    sizeof(MPID_nem_ib_queue_elem_t), 
                    "IB Module Queue Element");
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_queue_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

void MPID_nem_ib_queue_finalize(
        MPID_nem_ib_queue_t *q)
{
    MPID_nem_ib_queue_elem_t *e;

    while(!MPID_nem_ib_queue_empty(q)) {

        MPID_nem_ib_queue_dequeue(q, &e);

        MPIU_Free(e);
    }

    MPIU_Free(q);
}
