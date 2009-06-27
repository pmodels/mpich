/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "pmi_handle.h"
#include "pmi_handle_common.h"
#include "pmi_handle_v1.h"
#include "pmi_handle_v2.h"

HYD_Status HYD_PMCD_pmi_handle_init(int fd, char *args[])
{
    int pmi_version, pmi_subversion;
    char *tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    pmi_version = atoi(strtok(NULL, "="));
    strtok(args[1], "=");
    pmi_subversion = atoi(strtok(NULL, "="));

    if (pmi_version == 1 && pmi_subversion <= 1) {
        tmp = "cmd=response_to_init pmi_version=1 pmi_subversion=1 rc=0\n";
        status = HYDU_sock_writeline(fd, tmp, strlen(tmp));
        HYDU_ERR_POP(status, "error writing PMI line\n");
        HYD_PMCD_pmi_handle = HYD_PMCD_pmi_v1;
    }
    else if (pmi_version == 2 && pmi_subversion == 0) {
        tmp = "cmd=response_to_init pmi_version=2 pmi_subversion=0 rc=0\n";
        status = HYDU_sock_writeline(fd, tmp, strlen(tmp));
        HYDU_ERR_POP(status, "error writing PMI line\n");
        HYD_PMCD_pmi_handle = HYD_PMCD_pmi_v2;
    }
    else {
        /* PMI version mismatch */
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "PMI version mismatch; %d.%d\n", pmi_version, pmi_subversion);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
