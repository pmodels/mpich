/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

static inline void MPIR_T_enum_env_init(void)
{
    static const UT_icd enum_table_entry_icd = { sizeof(MPIR_T_enum_t), NULL, NULL, NULL };

    utarray_new(enum_table, &enum_table_entry_icd, MPL_MEM_MPIT);
}

static inline void MPIR_T_cat_env_init(void)
{
    static const UT_icd cat_table_entry_icd = { sizeof(cat_table_entry_t), NULL, NULL, NULL };

    utarray_new(cat_table, &cat_table_entry_icd, MPL_MEM_MPIT);
    cat_hash = NULL;
    cat_stamp = 0;
}

static inline int MPIR_T_cvar_env_init(void)
{
    static const UT_icd cvar_table_entry_icd = { sizeof(cvar_table_entry_t), NULL, NULL, NULL };

    utarray_new(cvar_table, &cvar_table_entry_icd, MPL_MEM_MPIT);
    cvar_hash = NULL;
    return MPIR_T_cvar_init();
}

int MPIR_T_env_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    static int initialized = FALSE;

    if (!initialized) {
        initialized = TRUE;
        MPIR_T_enum_env_init();
        MPIR_T_cat_env_init();
        mpi_errno = MPIR_T_cvar_env_init();
        MPIR_T_pvar_env_init();
    }
    return mpi_errno;
}
