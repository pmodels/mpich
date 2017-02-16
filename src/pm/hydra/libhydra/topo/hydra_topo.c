/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_topo.h"
#include "hydra_topo_internal.h"
#include "hydra_err.h"

#if defined HAVE_HWLOC
#include "hwloc/hydra_topo_hwloc.h"
#endif /* HAVE_HWLOC */

struct HYDI_topo_info HYDI_topo_info = { 0 };

HYD_status HYD_topo_init(char *user_binding, char *user_mapping, char *user_membind)
{
    const char *binding = NULL, *mapping = NULL, *membind = NULL;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    if (user_binding)
        binding = user_binding;
    else if (MPL_env2str("HYDRA_BINDING", &binding) == 0)
        binding = NULL;

    if (user_mapping)
        mapping = user_mapping;
    else if (MPL_env2str("HYDRA_MAPPING", &mapping) == 0)
        mapping = NULL;

    if (user_membind)
        membind = user_membind;
    else if (MPL_env2str("HYDRA_MEMBIND", &membind) == 0)
        membind = NULL;

    if (MPL_env2bool("HYDRA_TOPO_DEBUG", &HYDI_topo_info.debug) == 0)
        HYDI_topo_info.debug = 0;

    /* Initialize the topology library requested by the user */
#if defined HAVE_HWLOC
    status = HYDI_topo_hwloc_init(binding, mapping, membind);
    HYD_ERR_POP(status, "unable to initialize hwloc\n");
#endif /* HAVE_HWLOC */

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_topo_bind(int idx)
{
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    if (idx < 0)
        goto fn_exit;

#if defined HAVE_HWLOC
    status = HYDI_topo_hwloc_bind(idx);
    HYD_ERR_POP(status, "HWLOC failure binding process to core\n");
#else
    HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "no topology library available\n");
#endif /* HAVE_HWLOC */

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_topo_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* Finalize the topology library requested by the user */
#if defined HAVE_HWLOC
    status = HYDI_topo_hwloc_finalize();
    HYD_ERR_POP(status, "unable to finalize hwloc\n");
#endif /* HAVE_HWLOC */

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
