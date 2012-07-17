/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "topo.h"

#if defined HAVE_HWLOC
#include "hwloc/topo_hwloc.h"
#endif /* HAVE_HWLOC */

struct HYDT_topo_info HYDT_topo_info = { 0 };

static int ignore_binding = 0;

HYD_status HYDT_topo_init(char *user_binding, char *user_topolib)
{
    const char *binding = NULL, *topolib = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (user_binding)
        binding = user_binding;
    else if (MPL_env2str("HYDRA_BINDING", &binding) == 0)
        binding = NULL;

    if (user_topolib)
        topolib = user_topolib;
    else if (MPL_env2str("HYDRA_TOPOLIB", &topolib) == 0)
        topolib = HYDRA_DEFAULT_TOPOLIB;

    if (topolib)
        HYDT_topo_info.topolib = HYDU_strdup(topolib);
    else
        HYDT_topo_info.topolib = NULL;


    if (!binding || !strcmp(binding, "none")) {
        ignore_binding = 1;
        goto fn_exit;
    }

    /* Initialize the topology library requested by the user */
#if defined HAVE_HWLOC
    if (!strcmp(HYDT_topo_info.topolib, "hwloc")) {
        status = HYDT_topo_hwloc_init(binding);
        HYDU_ERR_POP(status, "unable to initialize hwloc\n");
    }
#endif /* HAVE_HWLOC */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_bind(int idx)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (idx < 0 || ignore_binding)
        goto fn_exit;

#if defined HAVE_HWLOC
    if (!strcmp(HYDT_topo_info.topolib, "hwloc")) {
        status = HYDT_topo_hwloc_bind(idx);
        HYDU_ERR_POP(status, "HWLOC failure binding process to core\n");
        goto fn_exit;
    }
#endif /* HAVE_HWLOC */

    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no topology library available\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_get_topomap(char **topomap)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

#if defined HAVE_HWLOC
    if (!strcmp(HYDT_topo_info.topolib, "hwloc")) {
        status = HYDT_topo_hwloc_get_topomap(topomap);
        HYDU_ERR_POP(status, "unable to get topomap\n");
        goto fn_exit;
    }
#endif /* HAVE_HWLOC */

    *topomap = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_get_processmap(char **processmap)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

#if defined HAVE_HWLOC
    if (!strcmp(HYDT_topo_info.topolib, "hwloc")) {
        status = HYDT_topo_hwloc_get_processmap(processmap);
        HYDU_ERR_POP(status, "unable to get topomap\n");
        goto fn_exit;
    }
#endif /* HAVE_HWLOC */

    *processmap = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_topo_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Finalize the topology library requested by the user */
#if defined HAVE_HWLOC
    if (!strcmp(HYDT_topo_info.topolib, "hwloc")) {
        status = HYDT_topo_hwloc_finalize();
        HYDU_ERR_POP(status, "unable to finalize hwloc\n");
    }
#endif /* HAVE_HWLOC */

    if (HYDT_topo_info.topolib)
        HYDU_FREE(HYDT_topo_info.topolib);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
