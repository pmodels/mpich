/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_finalize */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_finalize = PMPI_T_finalize
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_finalize  MPI_T_finalize
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_finalize as PMPI_T_finalize
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_finalize(void) __attribute__((weak,alias("PMPI_T_finalize")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_finalize
#define MPI_T_finalize PMPI_T_finalize

/* any non-MPI functions go here, especially non-static ones */

static void MPIR_T_enum_env_finalize(void)
{
    int i, j;
    MPIR_T_enum_t *e;
    enum_item_t *item;

    if (enum_table) {
        /* Free all entries */
        for (i = 0; i < utarray_len(enum_table); i++) {
            e = (MPIR_T_enum_t *)utarray_eltptr(enum_table, i);
            MPIU_Free((void *)e->name);

            /* Free items in this enum */
            for (j = 0; j < utarray_len(e->items); j++) {
                item = (enum_item_t *)utarray_eltptr(e->items, j);
                MPIU_Free((void *)item->name);
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
    int i;
    cat_table_entry_t *cat;

    if (cat_table) {
        /* Free all entries */
        for (i = 0; i < utarray_len(cat_table); i++) {
            cat = (cat_table_entry_t *)utarray_eltptr(cat_table, i);
            MPIU_Free((void *)cat->name);
            MPIU_Free((void *)cat->desc);
            utarray_free(cat->cvar_indices);
            utarray_free(cat->pvar_indices);
            utarray_free(cat->subcat_indices);
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
            MPIU_Free(current);
        }

        /* Free cat_hash itself */
        HASH_CLEAR(hh, cat_hash);
        cat_hash = NULL;
    }
}


static void MPIR_T_cvar_env_finalize(void)
{
    int i;
    cvar_table_entry_t *cvar;

    MPIR_T_cvar_finalize();

    if (cvar_table) {
        /* Free all entries */
        for (i = 0; i < utarray_len(cvar_table); i++) {
            cvar = (cvar_table_entry_t *)utarray_eltptr(cvar_table, i);
            MPIU_Free((void *)cvar->name);
            MPIU_Free((void *)cvar->desc);
            if (cvar->datatype == MPI_CHAR) MPIU_Free(cvar->addr);
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
            MPIU_Free(current);
        }

        /* Free cvar_hash itself */
        HASH_CLEAR(hh, cvar_hash);
        cvar_hash = NULL;
    }
}

static void MPIR_T_pvar_env_finalize(void)
{
    int i;
    pvar_table_entry_t *pvar;

    if (pvar_table) {
        /* Free all entries */
        for (i = 0; i < utarray_len(pvar_table); i++) {
            pvar = (pvar_table_entry_t *)utarray_eltptr(pvar_table, i);
            MPIU_Free((void *)pvar->name);
            MPIU_Free((void *)pvar->desc);
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
                MPIU_Free(current);
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
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_T_finalize - Finalize the MPI tool information interface

Notes:
This routine may be called as often as the corresponding MPI_T_init_thread() routine
up to the current point of execution. Calling it more times returns a corresponding
error code. As long as the number of calls to MPI_T_finalize() is smaller than the
number of calls to MPI_T_init_thread() up to the current point of execution, the MPI
tool information interface remains initialized and calls to its routines are permissible.
Further, additional calls to MPI_T_init_thread() after one or more calls to MPI_T_finalize()
are permissible. Once MPI_T_finalize() is called the same number of times as the routine
MPI_T_init_thread() up to the current point of execution, the MPI tool information
interface is no longer initialized. The interface can be reinitialized by subsequent calls
to MPI_T_init_thread().

At the end of the program execution, unless MPI_Abort() is called, an application must
have called MPI_T_init_thread() and MPI_T_finalize() an equal number of times.

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED

.seealso MPI_T_init_thread
@*/
int MPI_T_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_FINALIZE);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_FINALIZE);

    /* ... body of routine ...  */

    --MPIR_T_init_balance;
    if (MPIR_T_init_balance < 0) {
        mpi_errno = MPI_T_ERR_NOT_INITIALIZED;
        goto fn_fail;
    }

    if (MPIR_T_init_balance == 0) {
        MPIR_T_THREAD_CS_FINALIZE();
        MPIR_T_env_finalize();
    }

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_FINALIZE);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_finalize", NULL);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
