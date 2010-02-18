/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* SMPD MS HPC JOB Scheduler Resource management kernel */
/* Make sure that we don't include the opaque defns that are exported */
#include "smpd_hpc_js.h"

#undef FCNAME
#define FCNAME "smpd_hpc_js_rmk_init"
int smpd_hpc_js_rmk_init(smpd_hpc_js_handle_t *phnd)
{
    int result;
    smpd_hpc_js_ctxt_t *pctxt = (smpd_hpc_js_ctxt_t *)phnd;
    smpd_enter_fn(FCNAME);

    result = smpd_hpc_js_init(pctxt);
    if(result != SMPD_SUCCESS){
        smpd_err_printf("ERROR: Unable to initialize hpc job scheduler\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_hpc_js_rmk_finalize"
int smpd_hpc_js_rmk_finalize(smpd_hpc_js_handle_t *phnd)
{
    int result;
    smpd_hpc_js_ctxt_t *pctxt = (smpd_hpc_js_ctxt_t *)phnd;
    smpd_enter_fn(FCNAME);
    result = smpd_hpc_js_finalize(pctxt);
    if(result != SMPD_SUCCESS){
        smpd_err_printf("ERROR: Unable to finalize hpc job scheduler\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_hpc_js_rmk_alloc_nodes"
int smpd_hpc_js_rmk_alloc_nodes(smpd_hpc_js_handle_t hnd, smpd_launch_node_t *head)
{
    HRESULT hr;
    smpd_hpc_js_ctxt_t ctxt = (smpd_hpc_js_ctxt_t )hnd;

    hr = (ctxt->pscheduler)->CreateStringCollection(&(ctxt->pnode_names));
    if(FAILED(hr)){
        smpd_err_printf("ERROR: Unable to create node string coll, 0x%x\n", hr);
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    /* Lazy allocation */
    while(head){
        wchar_t hostnamew[SMPD_MAX_HOST_LENGTH];
        mbstowcs(hostnamew, head->hostname, SMPD_MAX_HOST_LENGTH);
        {
            long num_items=0;
            smpd_dbg_printf("Node string collection has (%d) items\n", (ctxt->pnode_names)->get_Count(&num_items));
        }
        hr = (ctxt->pnode_names)->Add(_bstr_t(hostnamew));
        if(FAILED(hr)){
            /* FIXME: We don't use the node collection right now */
            smpd_dbg_printf("ERROR: Unable to add hostname (%s) to node coll, 0x%x\n", head->hostname, hr);
        }
        head = head->next;
    }

    smpd_enter_fn(FCNAME);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

/* FIXME: Implement the query func */
#undef FCNAME
#define FCNAME "smpd_hpc_js_rmk_query_node_list"
int smpd_hpc_js_rmk_query_node_list(smpd_hpc_js_handle_t hnd, smpd_host_node_t **head)
{
    smpd_enter_fn(FCNAME);
    SMPDU_Assert(0);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
