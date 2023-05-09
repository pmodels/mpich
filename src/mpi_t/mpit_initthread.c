/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
 * Read global config files that have MPICH_ environment variables for us to
 * use
 */
static int read_config_files(const char *fname)
{
    int lineno = 0;
    char storage[1024], *line;
    FILE *f;

    if (!(f = fopen(fname, "r"))) {
        return FALSE;
    }

    while ((line = fgets(storage, sizeof(storage), f))) {
        char key[128], val[512], *spot;
        bool force = false;

        lineno++;

        if ('\n' == line[0] || '#' == line[0])
            continue;
        if ('\0' == line[0])
            break;

        if (2 > sscanf(line, "%127[^=]=%511s", key, val)) {
            fprintf(stderr, "Error parsing config file %s line %d: %s\n", fname, lineno, line);
            goto err;
        }
        /* If KEY:force=VALUE, then write prior values */
        if ((spot = strstr(key, ":force"))) {
            force = true;
            *spot = '\0';
        }

        if (setenv(key, val, force)) {
            fprintf(stderr, "Error setting %s from config file %s to %s\n", key, fname, val);
            goto err;
        }
    }
  err:
    fclose(f);
    return TRUE;
}

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

int MPIR_T_env_initialized = FALSE;

int MPIR_T_env_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    const char *config_filename = NULL;
#ifdef USE_CONFIGFILE
    config_filename = USE_CONFIGFILE;
    if (!read_config_files(config_filename)) {
        config_filename = NULL;
    }
#endif

    if (!MPIR_T_env_initialized) {
        MPIR_T_env_initialized = TRUE;
        MPIR_T_enum_env_init();
        MPIR_T_cat_env_init();
        mpi_errno = MPIR_T_cvar_env_init();
        MPIR_T_pvar_env_init();
        if (MPIR_CVAR_DEBUG_SUMMARY && config_filename) {
            printf("Global config file: %s\n", config_filename);
        }
    }
    return mpi_errno;
}
