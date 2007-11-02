/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

/* Generalize queues: flexible queue object upon which lists, stacks, fifos, hash tables, etc can be built */

#if !defined(MPICH2_MPIG_BDS_GENQ_I_INCLUDED) || defined(MPIG_EMIT_INLINE_FUNCS)
#define MPICH2_MPIG_BDS_GENQ_I_INCLUDED

#if !defined(MPIG_EMIT_INLINE_FUNCS) /* don't declare types and functions twice */

typedef void (*mpig_genq_destroy_entry_fn_t)(mpig_genq_entry_t * entry);

typedef bool_t (*mpig_genq_compare_values_fn_t)(const void * value1, const void * value2);

#endif /* !defined(MPIG_EMIT_INLINE_FUNCS) */

#if defined(MPIG_INLINE_HDECL)

MPIG_INLINE_HDECL int mpig_genq_entry_create(mpig_genq_entry_t ** entry_p);

MPIG_INLINE_HDECL void mpig_genq_entry_destroy(mpig_genq_entry_t * entry);

MPIG_INLINE_HDECL void mpig_genq_entry_construct(mpig_genq_entry_t * entry);

MPIG_INLINE_HDECL void mpig_genq_entry_destruct(mpig_genq_entry_t * entry);

MPIG_INLINE_HDECL mpig_genq_entry_t * mpig_genq_entry_get_prev(const mpig_genq_entry_t * entry);

MPIG_INLINE_HDECL mpig_genq_entry_t * mpig_genq_entry_get_next(const mpig_genq_entry_t * entry);

MPIG_INLINE_HDECL void * mpig_genq_entry_get_value(const mpig_genq_entry_t * entry);

MPIG_INLINE_HDECL void mpig_genq_entry_set_value(mpig_genq_entry_t * entry, void * value);

MPIG_INLINE_HDECL void mpig_genq_construct(mpig_genq_t * queue);

MPIG_INLINE_HDECL void mpig_genq_destruct(mpig_genq_t * queue, mpig_genq_destroy_entry_fn_t destroy_fn);

MPIG_INLINE_HDECL bool_t mpig_genq_is_empty(const mpig_genq_t * queue);

MPIG_INLINE_HDECL void mpig_genq_enqueue_head_entry(mpig_genq_t * queue, mpig_genq_entry_t * entry);

MPIG_INLINE_HDECL void mpig_genq_enqueue_tail_entry(mpig_genq_t * queue, mpig_genq_entry_t * entry);

MPIG_INLINE_HDECL mpig_genq_entry_t * mpig_genq_peek_head_entry(const mpig_genq_t * queue);

MPIG_INLINE_HDECL mpig_genq_entry_t * mpig_genq_peek_tail_entry(const mpig_genq_t * queue);

MPIG_INLINE_HDECL mpig_genq_entry_t * mpig_genq_dequeue_head_entry(mpig_genq_t * queue);

MPIG_INLINE_HDECL mpig_genq_entry_t * mpig_genq_dequeue_tail_entry(mpig_genq_t * queue);

MPIG_INLINE_HDECL mpig_genq_entry_t * mpig_genq_find_entry(const mpig_genq_t * queue, const void * value,
    mpig_genq_compare_values_fn_t compare_fn);

MPIG_INLINE_HDECL void mpig_genq_remove_entry(mpig_genq_t * queue, mpig_genq_entry_t * entry);

#endif /* defined(MPIG_INLINE_HDECL) */

#if defined(MPIG_INLINE_HDEF)

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_entry_create
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF int mpig_genq_entry_create(mpig_genq_entry_t ** const entry_p)
{
    mpig_genq_entry_t * entry;
    int mpi_errno = MPI_SUCCESS;

    entry = (mpig_genq_entry_t *) MPIU_Malloc(sizeof(mpig_genq_entry_t));
    MPIU_ERR_CHKANDJUMP1((entry == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "queue entry");

    mpig_genq_entry_construct(entry);
    *entry_p = entry;
    
  fn_return:
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_entry_destroy
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF void mpig_genq_entry_destroy(mpig_genq_entry_t * const entry)
{
    mpig_genq_entry_destruct(entry);
    MPIU_Free(entry);
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_entry_construct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF void mpig_genq_entry_construct(mpig_genq_entry_t * const entry)
{
#   if defined(MPIG_DEBUG)
    {
        entry->value = NULL;
        entry->prev = NULL;
        entry->next = NULL;
    }
#   endif
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_entry_destruct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF void mpig_genq_entry_destruct(mpig_genq_entry_t * const entry)
{
#   if defined(MPIG_DEBUG)
    {
        entry->value = MPIG_INVALID_PTR;
        entry->prev = MPIG_INVALID_PTR;
        entry->next = MPIG_INVALID_PTR;
    }
#   endif
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_entry_get_prev
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF mpig_genq_entry_t * mpig_genq_entry_get_prev(const mpig_genq_entry_t * const entry)
{
    return entry->prev;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_entry_get_next
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF mpig_genq_entry_t * mpig_genq_entry_get_next(const mpig_genq_entry_t * const entry)
{
    return entry->next;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_entry_get_value
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF void * mpig_genq_entry_get_value(const mpig_genq_entry_t * const entry)
{
    return entry->value;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_entry_set_value
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF void mpig_genq_entry_set_value(mpig_genq_entry_t * const entry, void * const value)
{
    entry->value = value;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_construct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF void mpig_genq_construct(mpig_genq_t * const queue)
{
    queue->head = NULL;
    queue->tail = NULL;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_destruct
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF void mpig_genq_destruct(mpig_genq_t * const queue, const mpig_genq_destroy_entry_fn_t destroy_entry_fn)
{
    mpig_genq_entry_t * entry;

    entry = queue->head;
    while(entry != NULL)
    {
        queue->head = entry->next;

        if ((destroy_entry_fn) != NULL) destroy_entry_fn(entry);

        entry = queue->head;
    }
    
    queue->head = MPIG_INVALID_PTR;
    queue->tail = MPIG_INVALID_PTR;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_is_empty
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF bool_t mpig_genq_is_empty(const mpig_genq_t * const queue)
{
    return (queue->head == NULL) ? TRUE : FALSE;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_enqueue_head_entry
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF void mpig_genq_enqueue_head_entry(mpig_genq_t * const queue, mpig_genq_entry_t * const entry)
{
    if (queue->head != NULL)
    {
        queue->head->prev = entry;
        entry->next = queue->head;
    }
    else
    {
        queue->tail = entry;
        entry->next = NULL;
    }
    
    queue->head = entry;
    entry->prev = NULL;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_enqueue_tail_entry
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF void mpig_genq_enqueue_tail_entry(mpig_genq_t * const queue, mpig_genq_entry_t * const entry)
{
    if (queue->tail != NULL)
    {
        queue->tail->next = entry;
        entry->prev = queue->tail;
    }
    else
    {
        queue->head = entry;
        entry->prev = NULL;
    }
    
    queue->tail = entry;
    entry->next = NULL;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_peek_head_entry
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF mpig_genq_entry_t * mpig_genq_peek_head_entry(const mpig_genq_t * const queue)
{
    mpig_genq_entry_t * entry;
    
    entry = queue->head;
    return entry;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_peek_tail_entry
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF mpig_genq_entry_t * mpig_genq_peek_tail_entry(const mpig_genq_t * const queue)
{
    mpig_genq_entry_t * entry;

    entry = queue->tail;
    return entry;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_deq_head_entry
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF mpig_genq_entry_t * mpig_genq_dequeue_head_entry(mpig_genq_t * const queue)
{
    mpig_genq_entry_t * entry;
    
    entry = queue->head;
    if (entry) mpig_genq_remove_entry(queue, entry);
    return entry;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_deq_tail_entry
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF mpig_genq_entry_t * mpig_genq_dequeue_tail_entry(mpig_genq_t * const queue)
{
    mpig_genq_entry_t * entry;

    entry = queue->tail;
    if (entry) mpig_genq_remove_entry(queue, entry);
    return entry;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_find_entry
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF mpig_genq_entry_t * mpig_genq_find_entry(const mpig_genq_t * const queue, const void * const value,
    const mpig_genq_compare_values_fn_t compare_fn)
{
    mpig_genq_entry_t * entry;
    
    entry = queue->head;
    while (entry != NULL)
    {
        if (compare_fn(value, entry->value))
        {
            break;
        }

        entry = entry->next;
    }

    return entry;
}

#undef FUNCNAME
#undef FCNAME
#define FUNCNAME mpig_genq_remove_entry
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_INLINE_HDEF void mpig_genq_remove_entry(mpig_genq_t * const queue, mpig_genq_entry_t * const entry)
{
    if (entry->prev != NULL)
    {
        entry->prev->next = entry->next;
    }
    else
    {
        queue->head = entry->next;
    }
    if (entry->next != NULL)
    {
        entry->next->prev = entry->prev;
    }
    else
    {
        queue->tail = entry->prev;
    }

#   if defined(MPIG_DEBUG)
    {
        entry->value = MPIG_INVALID_PTR;
        entry->prev = MPIG_INVALID_PTR;
        entry->next = MPIG_INVALID_PTR;
    }
#   endif
}

#undef FUNCNAME
#undef FCNAME

#endif /* defined(MPIG_INLINE_HDEF) */

#endif /* !defined(MPICH2_MPIG_BDS_GENQ_I_INCLUDED) || defined(MPIG_EMIT_INLINE_FUNCS) */
