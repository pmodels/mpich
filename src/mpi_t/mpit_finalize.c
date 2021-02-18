/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

static void MPIR_T_enum_env_finalize(void)
{
    unsigned int i, j;
    MPIR_T_enum_t *e;
    enum_item_t *item;

    if (enum_table) {
        /* Free all entries */
        for (i = 0; i < utarray_len(enum_table); i++) {
            e = (MPIR_T_enum_t *) utarray_eltptr(enum_table, i);
            MPL_free((void *) e->name);

            /* Free items in this enum */
            for (j = 0; j < utarray_len(e->items); j++) {
                item = (enum_item_t *) utarray_eltptr(e->items, j);
                MPL_free((void *) item->name);
            }

            utarray_free(e->items);
        }

        /* Free enum_table itself */
        utarray_free(enum_table);
        enum_table = NULL;
    }
}

static void MPIR_T_cat_env_finalize(void)
{
    unsigned int i;
    cat_table_entry_t *cat;

    if (cat_table) {
        /* Free all entries */
        for (i = 0; i < utarray_len(cat_table); i++) {
            cat = (cat_table_entry_t *) utarray_eltptr(cat_table, i);
            MPL_free((void *) cat->name);
            MPL_free((void *) cat->desc);
            utarray_free(cat->cvar_indices);
            utarray_free(cat->pvar_indices);
            utarray_free(cat->subcat_indices);
            utarray_free(cat->event_indices);
        }

        /* Free cat_table itself */
        utarray_free(cat_table);
        cat_table = NULL;
    }

    if (cat_hash) {
        name2index_hash_t *current, *tmp;
        /* Free all entries */
        HASH_ITER(hh, cat_hash, current, tmp) {
            HASH_DEL(cat_hash, current);
            MPL_free(current);
        }

        /* Free cat_hash itself */
        HASH_CLEAR(hh, cat_hash);
        cat_hash = NULL;
    }
}


static void MPIR_T_cvar_env_finalize(void)
{
    unsigned int i;
    cvar_table_entry_t *cvar;

    MPIR_T_cvar_finalize();

    if (cvar_table) {
        /* Free all entries */
        for (i = 0; i < utarray_len(cvar_table); i++) {
            cvar = (cvar_table_entry_t *) utarray_eltptr(cvar_table, i);
            MPL_free((void *) cvar->name);
            MPL_free((void *) cvar->desc);
            if (cvar->datatype == MPI_CHAR)
                MPL_free(cvar->addr);
        }

        /* Free pvar_table itself */
        utarray_free(cvar_table);
        cvar_table = NULL;
    }

    if (cvar_hash) {
        name2index_hash_t *current, *tmp;
        /* Free all entries */
        HASH_ITER(hh, cvar_hash, current, tmp) {
            HASH_DEL(cvar_hash, current);
            MPL_free(current);
        }

        /* Free cvar_hash itself */
        HASH_CLEAR(hh, cvar_hash);
        cvar_hash = NULL;
    }
}

static void MPIR_T_pvar_env_finalize(void)
{
    unsigned int i;
    pvar_table_entry_t *pvar;

    if (pvar_table) {
        /* Free all entries */
        for (i = 0; i < utarray_len(pvar_table); i++) {
            pvar = (pvar_table_entry_t *) utarray_eltptr(pvar_table, i);
            MPL_free((void *) pvar->name);
            MPL_free((void *) pvar->desc);
        }

        /* Free pvar_table itself */
        utarray_free(pvar_table);
        pvar_table = NULL;
    }

    for (i = 0; i < MPIR_T_PVAR_CLASS_NUMBER; i++) {
        if (pvar_hashs[i]) {
            name2index_hash_t *current, *tmp;
            /* Free all entries */
            HASH_ITER(hh, pvar_hashs[i], current, tmp) {
                HASH_DEL(pvar_hashs[i], current);
                MPL_free(current);
            }

            /* Free pvar_hashs[i] itself */
            HASH_CLEAR(hh, pvar_hashs[i]);
            pvar_hashs[i] = NULL;
        }
    }
}

void MPIR_T_env_finalize(void)
{
    MPIR_T_enum_env_finalize();
    MPIR_T_cvar_env_finalize();
    MPIR_T_pvar_env_finalize();
    MPIR_T_cat_env_finalize();
    MPIR_T_events_finalize();
}
