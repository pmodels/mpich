/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydt_ftb.h"

static FTB_client_handle_t ch;

static FTB_event_info_t event_info[] = {
    {"FTB_MPI_PROCS_RESTARTED", "info"},
    {"FTB_MPI_PROCS_RESTART_FAIL", "error"},
    {"FTB_MPI_PROCS_CKPTED", "info"},
    {"FTB_MPI_PROCS_CKPT_FAILED", "error"},
    {"FTB_MPI_PROCS_DEAD", "error"}
};

HYD_status HYDT_ftb_init(void)
{
    int ret;
    FTB_client_t ci;
    HYD_status status = HYD_SUCCESS;

    MPL_strncpy(ci.event_space, "ftb.mpi.hydra", sizeof(ci.event_space));
    MPL_strncpy(ci.client_name, "hydra " HYDRA_VERSION, sizeof(ci.client_name));
    MPL_strncpy(ci.client_subscription_style, "FTB_SUBSCRIPTION_NONE",
                sizeof(ci.client_subscription_style));
    ci.client_polling_queue_len = -1;

    ret = FTB_Connect(&ci, &ch);
    if (ret)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "ftb connect\n");

    ret = FTB_Declare_publishable_events(ch, NULL, event_info,
                                         sizeof(event_info) / sizeof(event_info[0]));
    if (ret)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "ftb declare publishable\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_ftb_finalize(void)
{
    int ret;
    HYD_status status = HYD_SUCCESS;

    ret = FTB_Disconnect(ch);
    if (ret)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "ftb disconnect\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_ftb_publish(const char *event_name, const char *event_payload)
{
    FTB_event_properties_t event_prop;
    FTB_event_handle_t event_handle;
    int ret;
    HYD_status status = HYD_SUCCESS;

    event_prop.event_type = 1;
    MPL_strncpy(event_prop.event_payload, event_payload, sizeof(event_prop.event_payload));

    ret = FTB_Publish(ch, event_name, &event_prop, &event_handle);
    if (ret)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "ftb publish\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
