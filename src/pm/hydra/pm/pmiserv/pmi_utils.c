/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "pmci.h"
#include "pmi_handle.h"
#include "bsci.h"
#include "pmi_serv.h"

HYD_status HYD_pmcd_pmi_fill_in_proxy_args(char **proxy_args, char *control_port, int pgid)
{
    int i, arg;
    char *path_str[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    arg = 0;
    i = 0;
    path_str[i++] = HYDU_strdup(HYD_handle.base_path);
    path_str[i++] = HYDU_strdup("pmi_proxy");
    path_str[i] = NULL;
    status = HYDU_str_alloc_and_join(path_str, &proxy_args[arg++]);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(path_str);

    proxy_args[arg++] = HYDU_strdup("--control-port");
    proxy_args[arg++] = HYDU_strdup(control_port);

    if (HYD_handle.user_global.debug)
        proxy_args[arg++] = HYDU_strdup("--debug");

    proxy_args[arg++] = HYDU_strdup("--bootstrap");
    proxy_args[arg++] = HYDU_strdup(HYD_handle.user_global.bootstrap);

    if (HYD_handle.user_global.bootstrap_exec) {
        proxy_args[arg++] = HYDU_strdup("--bootstrap-exec");
        proxy_args[arg++] = HYDU_strdup(HYD_handle.user_global.bootstrap_exec);
    }

    proxy_args[arg++] = HYDU_strdup("--pgid");
    proxy_args[arg++] = HYDU_int_to_str(pgid);

    proxy_args[arg++] = HYDU_strdup("--proxy-id");
    proxy_args[arg++] = NULL;

    if (HYD_handle.user_global.debug) {
        HYDU_dump_noprefix(stdout, "\nProxy launch args: ");
        HYDU_print_strlist(proxy_args);
        HYDU_dump_noprefix(stdout, "\n");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
