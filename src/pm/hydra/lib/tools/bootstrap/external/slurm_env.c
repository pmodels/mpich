/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bscd_slurm_query_env_inherit(const char *env_name, int *should_inherit)
{
    HYDU_FUNC_ENTER();

    /* check if envvar starts with SBATCH_, SLURMD_, or SLURM_ */
    if (strncmp(env_name, "SBATCH_", 7) == 0 || strncmp(env_name, "SLURMD_", 7) == 0 ||
        strncmp(env_name, "SLURM_", 6) == 0) {
        *should_inherit = 0;
    } else {
        *should_inherit = 1;
    }

    HYDU_FUNC_EXIT();

    return HYD_SUCCESS;
}
