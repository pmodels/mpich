/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include <libftb.h>
#ifndef USE_PMI2_API
#include "pmi.h"
#else
#include "pmi2.h"
#endif

static FTB_client_handle_t client_handle;

static FTB_event_info_t event_info[] = {
    { MPIDU_FTB_EV_OTHER,         "error" },
    { MPIDU_FTB_EV_RESOURCES,     "error" },
    { MPIDU_FTB_EV_UNREACHABLE,   "error" },
    { MPIDU_FTB_EV_COMMUNICATION, "error" },
    { MPIDU_FTB_EV_ABORT,         "fatal" }
};

#ifdef DEBUG_MPIDU_FTB
#define CHECK_FTB_ERROR(x) do { MPIU_Assertp(x); } while(0)
#else
#define CHECK_FTB_ERROR(x) (void)x
#endif

#undef FUNCNAME
#define FUNCNAME MPIDU_Ftb_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_Ftb_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    FTB_client_t ci;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_FTB_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_FTB_INIT);

    MPIU_Strncpy(ci.event_space, "ftb.mpi.mpich", sizeof(ci.event_space));
    MPIU_Strncpy(ci.client_name, "mpich " MPICH_VERSION, sizeof(ci.client_name));
    MPIU_Strncpy(ci.client_subscription_style, "FTB_SUBSCRIPTION_NONE", sizeof(ci.client_subscription_style));
    ci.client_polling_queue_len = -1;
    
#ifdef USE_PMI2_API
    ret = PMI2_Job_GetId(ci.client_jobid, sizeof(ci.client_jobid));
    MPIR_ERR_CHKANDJUMP(ret, mpi_errno, MPI_ERR_OTHER, "**pmi_jobgetid");
#else
    ret = PMI_KVS_Get_my_name(ci.client_jobid, sizeof(ci.client_jobid));
    MPIR_ERR_CHKANDJUMP(ret, mpi_errno, MPI_ERR_OTHER, "**pmi_get_id");
#endif
    
    ret = FTB_Connect(&ci, &client_handle);
    MPIR_ERR_CHKANDJUMP(ret, mpi_errno, MPI_ERR_OTHER, "**ftb_connect");

    ret = FTB_Declare_publishable_events(client_handle, NULL, event_info, sizeof(event_info) / sizeof(event_info[0]));
    MPIR_ERR_CHKANDJUMP(ret, mpi_errno, MPI_ERR_OTHER, "**ftb_declare_publishable_events");

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_FTB_INIT);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* We don't return an error code for MPIDU_Ftb_publish, because if
   we're calling this, we're probably already reporting an error.
   Also, a broken error reporting mechanism should not cause an app
   fail. */

#undef FUNCNAME
#define FUNCNAME MPIDU_Ftb_publish
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDU_Ftb_publish(const char *event_name, const char *event_payload)
{
    FTB_event_properties_t event_prop;
    FTB_event_handle_t event_handle;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_FTB_PUBLISH);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_FTB_PUBLISH);

    event_prop.event_type = 1;
    MPIU_Strncpy(event_prop.event_payload, event_payload, sizeof(event_prop.event_payload));
    
    CHECK_FTB_ERROR(FTB_Publish(client_handle, event_name, &event_prop, &event_handle));

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_FTB_PUBLISH);
    return;
}

/* convenience function for publishing events associated with a particular vc */
#undef FUNCNAME
#define FUNCNAME MPIDU_Ftb_publish_vc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDU_Ftb_publish_vc(const char *event_name, struct MPIDI_VC *vc)
{
    char payload[FTB_MAX_PAYLOAD_DATA] = "";

    if (vc && vc->pg)  /* pg can be null for temp VCs (dynamic processes) */
        MPL_snprintf(payload, sizeof(payload), "[id: {%s:{%d}}]", (char*)vc->pg->id, vc->pg_rank);
    MPIDU_Ftb_publish(event_name, payload);
    return;
}

/* convenience function for publishing events associated with this process */
#undef FUNCNAME
#define FUNCNAME MPIDU_Ftb_publish_me
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDU_Ftb_publish_me(const char *event_name)
{
    char payload[FTB_MAX_PAYLOAD_DATA] = "";

    MPL_snprintf(payload, sizeof(payload), "[id: {%s:{%d}}]", (char *)MPIDI_Process.my_pg->id, MPIDI_Process.my_pg_rank);
    MPIDU_Ftb_publish(event_name, payload);
    return;
}


/* MPIDU_Ftb_finalize has no return code for the same reasons that
   MPIDU_Ftb_publish doesn't. */

#undef FUNCNAME
#define FUNCNAME MPIDU_Ftb_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIDU_Ftb_finalize(void)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_FTB_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_FTB_FINALIZE);

    CHECK_FTB_ERROR(FTB_Disconnect(client_handle));

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_FTB_FINALIZE);
    return;
}

