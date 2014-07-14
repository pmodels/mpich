/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

int MPIR_T_init_balance = 0;

#ifdef MPICH_IS_THREADED
MPIU_Thread_mutex_t mpi_t_mutex;
int MPIR_T_is_threaded;
#endif

/* These variables must be initialized in MPI_T_initthread. Especially,
 * hash table pointers must be initialized to NULL.
 */
int cat_stamp;
UT_array *enum_table;
UT_array *cat_table;
UT_array *cvar_table;
UT_array *pvar_table;
name2index_hash_t *cat_hash;
name2index_hash_t *cvar_hash;
name2index_hash_t *pvar_hashs[MPIR_T_PVAR_CLASS_NUMBER];

/* Create an enum.
 * IN: enum_name, name of the enum
 * OUT: handle, handle of the enum
 */
void MPIR_T_enum_create(const char *enum_name, MPI_T_enum *handle)
{
    MPIR_T_enum_t *e;
    static const UT_icd enum_item_icd = {sizeof(enum_item_t), NULL, NULL, NULL};

    MPIU_Assert(enum_name);
    MPIU_Assert(handle);

    utarray_extend_back(enum_table);
    e = (MPIR_T_enum_t *)utarray_back(enum_table);
    e->name = MPIU_Strdup(enum_name);
    MPIU_Assert(e->name);
#ifdef HAVE_ERROR_CHECKING
    e->kind = MPIR_T_ENUM_HANDLE;
#endif
    utarray_new(e->items, &enum_item_icd);
    (*handle) = e;
}

/* Add an item to an exisiting enum.
 * IN: handle, handle to the enum
 * IN: item_name, name of the item
 * IN: item_value, value associated with item_name
 */
void MPIR_T_enum_add_item(MPI_T_enum handle, const char *item_name, int item_value)
{
    enum_item_t *item;

    MPIU_Assert(handle);
    MPIU_Assert(item_name);

    utarray_extend_back(handle->items);
    item = (enum_item_t *)utarray_back(handle->items);
    item->name = MPIU_Strdup(item_name);
    MPIU_Assert(item->name);
    item->value = item_value;
}

/* Create a new category with name <cat_name>.
 * The new category is pushed at the back of cat_table.
 * Aslo, a new hash entry is added for the category in cat_hash.
 * Return the newly created category.
 */
static cat_table_entry_t *MPIR_T_cat_create(const char *cat_name)
{
    int cat_idx;
    cat_table_entry_t *cat;
    name2index_hash_t *hash_entry;

    /* New a category */
    utarray_extend_back(cat_table);
    cat =(cat_table_entry_t *)utarray_back(cat_table);
    cat->name = MPIU_Strdup(cat_name);
    cat->desc = NULL;
    utarray_new(cat->cvar_indices, &ut_int_icd);
    utarray_new(cat->pvar_indices, &ut_int_icd);
    utarray_new(cat->subcat_indices, &ut_int_icd);

    /* Record <cat_name, cat_idx> in cat_hash */
    cat_idx = utarray_len(cat_table) - 1;
    hash_entry = MPIU_Malloc(sizeof(name2index_hash_t));
    MPIU_Assert(hash_entry);
    /* Need not to Strdup cat_name, since cat_table and cat_hash co-exist */
    hash_entry->name = cat_name;
    hash_entry->idx = cat_idx;
    HASH_ADD_KEYPTR(hh, cat_hash, hash_entry->name,
                    strlen(hash_entry->name), hash_entry);

    return cat;
}

/* Add a pvar to an existing or new category
 * IN: cat_name, name of the category
 * IN: pvar_index, index of the pvar as defined by MPI_T_pvar_handle_alloc()
 * If cat_name is NULL or a empty string, nothing happpens.
 */
int MPIR_T_cat_add_pvar(const char *cat_name, int pvar_index)
{
    int mpi_errno = MPI_SUCCESS;
    name2index_hash_t *hash_entry;
    cat_table_entry_t *cat;

    /* NULL or empty string are allowed */
    if (cat_name == NULL || *cat_name == '\0')
        goto fn_exit;

    HASH_FIND_STR(cat_hash, cat_name, hash_entry);

    if (hash_entry != NULL) {
        /* Found it, i.e., category already exists */
        int cat_idx = hash_entry->idx;
        cat = (cat_table_entry_t *)utarray_eltptr(cat_table, cat_idx);
        /* FIXME: Is it worth checking duplicated vars? Probably not */
        utarray_push_back(cat->pvar_indices, &pvar_index);
    } else {
        /* Not found, so create a new category */
        cat = MPIR_T_cat_create(cat_name);
        utarray_push_back(cat->pvar_indices, &pvar_index);
        /* Notify categories have been changed */
        cat_stamp++;
    }

fn_exit:
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

/* Add a cvar to an existing or new category
 * IN: cat_name, name of the category
 * IN: cvar_index, index of the cvar as defined by MPI_T_cvar_handle_alloc()
 * If cat_name is NULL or a empty string, nothing happpens.
 */
int MPIR_T_cat_add_cvar(const char *cat_name, int cvar_index)
{
    int mpi_errno = MPI_SUCCESS;
    name2index_hash_t *hash_entry;
    cat_table_entry_t *cat;

    /* NULL or empty string are allowed */
    if (cat_name == NULL || *cat_name == '\0')
        goto fn_exit;

    HASH_FIND_STR(cat_hash, cat_name, hash_entry);

    if (hash_entry != NULL) {
        /* Found it, i.e., category already exists */
        int cat_idx = hash_entry->idx;
        cat = (cat_table_entry_t *)utarray_eltptr(cat_table, cat_idx);
        /* FIXME: Is it worth checking duplicated vars? Probably not */
        utarray_push_back(cat->cvar_indices, &cvar_index);
    } else {
        /* Not found, so create a new category */
        cat = MPIR_T_cat_create(cat_name);
        utarray_push_back(cat->cvar_indices, &cvar_index);
        /* Notify categories have been changed */
        cat_stamp++;
    }

fn_exit:
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

/* Add a sub-category to an existing or new category
 * IN: parent_name, name of the parent category
 * IN: child_name, name of the child category
 */
int MPIR_T_cat_add_subcat(const char *parent_name, const char *child_name)
{
    int mpi_errno = MPI_SUCCESS;
    int parent_index, child_index;
    name2index_hash_t *hash_entry;
    cat_table_entry_t *parent;

    /* NULL or empty string are allowed */
    if (parent_name == NULL || *parent_name == '\0' ||
        child_name == NULL || *child_name == '\0')
    {
        goto fn_exit;
    }

    /* Find or create parent */
    HASH_FIND_STR(cat_hash, parent_name, hash_entry);
    if (hash_entry != NULL) {
        /* Found parent in cat_table */
        parent_index = hash_entry->idx;
    } else {
        /* parent is a new category */
        MPIR_T_cat_create(parent_name);
        parent_index = utarray_len(cat_table) - 1;
    }

    /* Find or create child */
    HASH_FIND_STR(cat_hash, child_name, hash_entry);
    if (hash_entry != NULL) {
        /* Found child in cat_table */
        child_index = hash_entry->idx;
    } else {
        /* child is a new category */
        MPIR_T_cat_create(child_name);
        child_index = utarray_len(cat_table) - 1;
    }

    /* Connect parent and child */
    parent = (cat_table_entry_t *)utarray_eltptr(cat_table, parent_index);
    utarray_push_back(parent->subcat_indices, &child_index);

    /* Notify categories have been changed */
    cat_stamp++;

fn_exit:
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

/* Add description to an existing or new category
 * IN: cat_name, name of the category
 * IN: cat_desc, description of the category
 */
int MPIR_T_cat_add_desc(const char *cat_name, const char *cat_desc)
{
    int cat_idx, mpi_errno = MPI_SUCCESS;
    name2index_hash_t *hash_entry;
    cat_table_entry_t *cat;

    /* NULL args are not allowed */
    MPIU_Assert(cat_name);
    MPIU_Assert(cat_desc);

    HASH_FIND_STR(cat_hash, cat_name, hash_entry);

    if (hash_entry != NULL) {
        /* Found it, i.e., category already exists */
        cat_idx = hash_entry->idx;
        cat = (cat_table_entry_t *)utarray_eltptr(cat_table, cat_idx);
        MPIU_Assert(cat->desc == NULL);
        cat->desc = MPIU_Strdup(cat_desc);
        MPIU_Assert(cat->desc);
    } else {
        /* Not found, so create a new category */
        cat = MPIR_T_cat_create(cat_name);
        cat->desc = MPIU_Strdup(cat_desc);
        MPIU_Assert(cat->desc);
        /* Notify categories have been changed */
        cat_stamp++;
    }

fn_exit:
    return mpi_errno;

fn_fail:
    goto fn_exit;
}

/* A low level, generic and internally used interface to register
 * a cvar to the MPIR_T.
 *
 * IN: dtype, MPI datatype for this cvar
 * IN: name, Name of the cvar
 * IN: addr, Pointer to the cvar if known at registeration, otherwise NULL.
 * IN: count, # of elements of this cvar if known at registeration, otherwise 0.
 * IN: etype, MPI_T_enum or MPI_T_ENUM_NULL
 * IN: verb, MPI_T_PVAR_VERBOSITY_*
 * IN: binding, MPI_T_BIND_*
 * IN: Scope, MPI_T_SCOPE_*
 * IN: get_addr, If not NULL, it is a callback to get address of the cvar.
 * IN: get_count, If not NULL, it is a callback to read count of the cvar.
 * IN: cat, Catogery name of the cvar
 * IN: desc, Description of the cvar
 */
void MPIR_T_CVAR_REGISTER_impl(
    MPI_Datatype dtype, const char* name, const void *addr, int count,
    MPIR_T_enum_t *etype, MPIR_T_verbosity_t verb, MPIR_T_bind_t binding,
    MPIR_T_scope_t scope, MPIR_T_cvar_get_addr_cb get_addr,
    MPIR_T_cvar_get_count_cb get_count, MPIR_T_cvar_value_t defaultval,
    const char *cat, const char * desc)
{
    name2index_hash_t *hash_entry;
    cvar_table_entry_t *cvar;
    int cvar_idx;

    /* Check whether this is a replicated cvar, whose name is unique. */
    HASH_FIND_STR(cvar_hash, name, hash_entry);

    if (hash_entry != NULL) {
        /* Found it, the cvar already exists */
        cvar_idx = hash_entry->idx;
        cvar = (cvar_table_entry_t *)utarray_eltptr(cvar_table, cvar_idx);
        /* Should never override an existing & active var */
        MPIU_Assert(cvar->active != TRUE);
        cvar->active = TRUE;
        /* FIXME: Do we need to check consistency between the old and new? */
    } else {
        /* Not found, so push the cvar to back of cvar_table */
        utarray_extend_back(cvar_table);
        cvar = (cvar_table_entry_t *)utarray_back(cvar_table);
        cvar->active = TRUE;
        cvar->datatype = dtype;
        cvar->name = MPIU_Strdup(name);
        MPIU_Assert(cvar->name);
        if (dtype != MPI_CHAR) {
            cvar->addr = (void *)addr;
        } else {
            cvar->addr = MPIU_Malloc(count);
            MPIU_Assert(cvar->addr);
            if (defaultval.str == NULL) {
                ((char *)(cvar->addr))[0] = '\0';
            } else {
                /* Use greater (>), since count includes the terminating '\0', but strlen does not */
                MPIU_Assert(count > strlen(defaultval.str));
                strcpy(cvar->addr, defaultval.str);
            }
        }
        cvar->count = count;
        cvar->verbosity = verb;
        cvar->bind = binding;
        cvar->scope = scope;
        cvar->get_addr = get_addr;
        cvar->get_count = get_count;
        cvar->defaultval = defaultval;
        cvar->desc = MPIU_Strdup(desc);
        MPIU_Assert(cvar->desc);

        /* Record <name, index> in hash table */
        cvar_idx = utarray_len(cvar_table) - 1;
        hash_entry = MPIU_Malloc(sizeof(name2index_hash_t));
        MPIU_Assert(hash_entry);
        /* Need not to Strdup name, since cvar_table and cvar_hash co-exist */
        hash_entry->name =name;
        hash_entry->idx = cvar_idx;
        HASH_ADD_KEYPTR(hh, cvar_hash, hash_entry->name,
                        strlen(hash_entry->name), hash_entry);

        /* Add the cvar to a category */
        MPIR_T_cat_add_cvar(cat, cvar_idx);
    }
}

/* A low level, generic and internally used interface to register
 * a pvar to MPIR_T. Other modules should use interfaces defined
 * for concrete pvar classes.
 *
 * IN: varclass, MPI_T_PVAR_CLASS_*
 * IN: dtype, MPI datatype for this pvar
 * IN: name, Name of the pvar
 * IN: addr, Pointer to the pvar if known at registeration, otherwise NULL.
 * IN: count, # of elements of this pvar if known at registeration, otherwise 0.
 * IN: etype, MPI_T_enum or MPI_T_ENUM_NULL
 * IN: verb, MPI_T_PVAR_VERBOSITY_*
 * IN: binding, MPI_T_BIND_*
 * IN: flags, Bitwise OR of MPIR_T_R_PVAR_FLAGS_{}
 * IN: get_value, If not NULL, it is a callback to read the pvar.
 * IN: get_count, If not NULL, it is a callback to read count of the pvar.
 * IN: cat, Catogery name of the pvar
 * IN: desc, Description of the pvar
 */
void MPIR_T_PVAR_REGISTER_impl(
    int varclass, MPI_Datatype dtype, const char* name, void *addr, int count,
    MPIR_T_enum_t *etype, int verb, int binding, int flags,
    MPIR_T_pvar_get_value_cb get_value, MPIR_T_pvar_get_count_cb get_count,
    const char * cat, const char * desc)
{
    name2index_hash_t *hash_entry;
    pvar_table_entry_t *pvar;
    int pvar_idx;
    int seq = varclass - MPIR_T_PVAR_CLASS_FIRST;

    /* Check whether this is a replicated pvar, whose name is unique per class */
    HASH_FIND_STR(pvar_hashs[seq], name, hash_entry);

    if (hash_entry != NULL) {
        /* Found it, the pvar already exists */
        pvar_idx = hash_entry->idx;
        pvar = (pvar_table_entry_t *)utarray_eltptr(pvar_table, pvar_idx);
        /* Should never override an existing & active var */
        MPIU_Assert(pvar->active != TRUE);
        pvar->active = TRUE;
        /* FIXME: Do we need to check consistency between the old and new? */
    } else {
        /* Not found, so push the pvar to back of pvar_table */
        utarray_extend_back(pvar_table);
        pvar = (pvar_table_entry_t *)utarray_back(pvar_table);
        pvar->active = TRUE;
        pvar->varclass = varclass;
        pvar->datatype = dtype;
        pvar->name = MPIU_Strdup(name);
        MPIU_Assert(pvar->name);
        pvar->addr = addr;
        pvar->count = count;
        pvar->enumtype = etype;
        pvar->verbosity = verb;
        pvar->bind = binding;
        pvar->flags = flags;
        pvar->get_value = get_value;
        pvar->get_count = get_count;
        pvar->desc = MPIU_Strdup(desc);
        MPIU_Assert(pvar->desc);

        /* Record <name, index> in hash table */
        pvar_idx = utarray_len(pvar_table) - 1;
        hash_entry = MPIU_Malloc(sizeof(name2index_hash_t));
        MPIU_Assert(hash_entry);
        /* Need not to Strdup name, since pvar_table and pvar_hashs co-exist */
        hash_entry->name = name;
        hash_entry->idx = pvar_idx;
        HASH_ADD_KEYPTR(hh, pvar_hashs[seq], hash_entry->name,
                        strlen(hash_entry->name), hash_entry);

        /* Add the pvar to a category */
        MPIR_T_cat_add_pvar(cat, utarray_len(pvar_table)-1);
    }
}

/* Implements an MPI_T-style strncpy.  Here is the description from the draft
 * standard:
 *
 *   Several MPI tool information interface functions return one or more
 *   strings. These functions have two arguments for each string to be returned:
 *   an OUT parameter that identifies a pointer to the buffer in which the
 *   string will be returned, and an IN/OUT parameter to pass the length of the
 *   buffer. The user is responsible for the memory allocation of the buffer and
 *   must pass the size of the buffer (n) as the length argument. Let n be the
 *   length value specified to the function. On return, the function writes at
 *   most n - 1 of the string's characters into the buffer, followed by a null
 *   terminator. If the returned string's length is greater than or equal to n,
 *   the string will be truncated to n - 1 characters. In this case, the length
 *   of the string plus one (for the terminating null character) is returned in
 *   the length argument. If the user passes the null pointer as the buffer
 *   argument or passes 0 as the length argument, the function does not return
 *   the string and only returns the length of the string plus one in the length
 *   argument. If the user passes the null pointer as the length argument, the
 *   buffer argument is ignored and nothing is returned.
 *
 * So this routine copies up to (*len)-1 characters from src to dst and then
 * sets *len to (strlen(dst)+1).  If dst==NULL, just return (strlen(src)+1) in
 * *len.
 *
 * This routine does not follow MPICH error handling conventions.
 */
void MPIR_T_strncpy(char *dst, const char *src, int *len)
{
    /* std. says if len arg is NULL, dst is ignored and nothing is returned (MPI-3, p.563) */
    if (len) {
        /* If dst is NULL or *len is 0, just return src length + 1 */
        if (!dst || !*len) {
            *len = (src == NULL) ? 1 : strlen(src) + 1;
        }
        else {
            /* MPL_strncpy will always terminate the string */
            MPIU_Assert(*len > 0);
            if (src != NULL) {
                MPL_strncpy(dst, src, *len);
                *len = (int)strlen(dst) + 1;
            } else {
                /* As if an empty string is copied */
                *dst = '\0';
                *len = 1;
            }
        }
    }
}

