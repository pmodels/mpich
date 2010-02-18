/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef SMPD_HPC_JS_H_INCLUDED
#define SMPD_HPC_JS_H_INCLUDED

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/* The Microsoft.Hpc.Scheduler.tlb and Microsoft.Hpc.Scheduler.Properties.tlb type
    libraries are included in the Microsoft HPC Pack 2008 SDK. The type libraries are
    located in the "Microsoft HPC Pack 2008 SDK\Lib\i386" or \amd64 folder. Include the rename 
    attributes to avoid name collisions.
*/
#import <Microsoft.Hpc.Scheduler.tlb> named_guids no_namespace raw_interfaces_only \
    rename("SetEnvironmentVariable","SetHpcEnvironmentVariable") \
    rename("AddJob", "AddHpcJob")
#import <Microsoft.Hpc.Scheduler.Properties.tlb> named_guids no_namespace raw_interfaces_only 

#include "smpd.h"

typedef struct smpd_hpc_js_ctxt_{
    IScheduler* pscheduler;
    IStringCollection *pnode_names;
} *smpd_hpc_js_ctxt_t;

#define SMPDU_HPC_JS_CTXT_IS_VALID(ctxt) ((ctxt) && ((ctxt)->pscheduler))

/* Func prototypes */
int smpd_hpc_js_init(smpd_hpc_js_ctxt_t *pctxt);
int smpd_hpc_js_finalize(smpd_hpc_js_ctxt_t *pctxt);
int smpd_hpc_js_create_job(smpd_hpc_js_ctxt_t ctxt, smpd_launch_node_t *head, ISchedulerJob **pp_job);
int smpd_hpc_js_submit_job(smpd_hpc_js_ctxt_t ctxt, ISchedulerJob *pjob);

#endif /* SMPD_HPC_JS_H_INCLUDED */
