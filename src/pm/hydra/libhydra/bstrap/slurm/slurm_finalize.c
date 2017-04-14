#include "hydra_bstrap_slurm.h"
#include "hydra_err.h"

HYD_status HYDI_bstrap_slurm_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
