/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmip_pmi.h"

static HYD_status fn_init(int fd, char *args[])
{
    int pmi_version, pmi_subversion;
    const char *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    pmi_version = atoi(strtok(NULL, "="));
    strtok(args[1], "=");
    pmi_subversion = atoi(strtok(NULL, "="));

    if (pmi_version == 1 && pmi_subversion <= 1)
        tmp = "cmd=response_to_init pmi_version=1 pmi_subversion=1 rc=0\n";
    else if (pmi_version == 2 && pmi_subversion == 0)
        tmp = "cmd=response_to_init pmi_version=2 pmi_subversion=0 rc=0\n";
    else        /* PMI version mismatch */
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "PMI version mismatch; %d.%d\n", pmi_version, pmi_subversion);

    status = HYDU_sock_write(fd, tmp, strlen(tmp));
    HYDU_ERR_POP(status, "error writing PMI line\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static struct HYD_pmcd_pmip_pmi_handle pmi_v1_handle_fns_foo[] = {
    {"init", fn_init},
    {"\0", NULL}
};

struct HYD_pmcd_pmip_pmi_handle *HYD_pmcd_pmip_pmi_v1 = pmi_v1_handle_fns_foo;
