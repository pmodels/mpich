/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* SMPD MS HPC JOB Scheduler bootstrap server */
#include "smpd_hpc_js.h"

#undef FCNAME
#define FCNAME "smpd_hpc_js_bs_init"
int smpd_hpc_js_bs_init(smpd_hpc_js_handle_t hnd)
{
    smpd_enter_fn(FCNAME);
    /* DO NOTHING */
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_hpc_js_bs_finalize"
int smpd_hpc_js_bs_finalize(smpd_hpc_js_handle_t hnd)
{
    smpd_enter_fn(FCNAME);
    /* DO NOTHING */
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_hpc_js_bs_launch_procs"
int smpd_hpc_js_bs_launch_procs(smpd_hpc_js_handle_t hnd, smpd_launch_node_t *head)
{
    int result;
    smpd_hpc_js_ctxt_t ctxt = (smpd_hpc_js_ctxt_t )hnd;
    ISchedulerJob *pjob;

    smpd_enter_fn(FCNAME);
    result = smpd_hpc_js_create_job(ctxt, head, &pjob);
    if(result != SMPD_SUCCESS){
        smpd_err_printf("ERROR: Unable to create job\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    
    result = smpd_hpc_js_submit_job(ctxt, pjob);
    if(result != SMPD_SUCCESS){
        smpd_err_printf("ERROR: Unable to create job\n");
        pjob->Release();
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    pjob->Release();
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
