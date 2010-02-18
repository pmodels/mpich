/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* SMPD MS HPC JOB Scheduler utils */
#include "smpd_hpc_js.h"

#undef FCNAME
#define FCNAME "smpd_hpc_js_connect_to_head_node"
int smpd_hpc_js_connect_to_head_node(smpd_hpc_js_ctxt_t ctxt, smpd_host_node_t **head_node)
{
    HRESULT hr;

    smpd_enter_fn(FCNAME);
    if(!SMPDU_HPC_JS_CTXT_IS_VALID(ctxt)){
        smpd_err_printf("ERROR: Invalid handle to hpc job scheduler\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    /* FIXME: Currently we ignore the head node passed. Ideally
     * we should use AD to find the head node and connect to it.
     * We just connect to the localhost now
     */
    hr = (ctxt->pscheduler)->Connect(_bstr_t(L"localhost"));
    if(FAILED(hr)){
        smpd_err_printf("ERROR: Unable to connect to the head node of the cluster, 0x%x\n", hr);
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_hpc_js_init"
int smpd_hpc_js_init(smpd_hpc_js_ctxt_t *pctxt)
{
    HRESULT hr;
    int result;

    smpd_enter_fn(FCNAME);

    if(pctxt == NULL){
        smpd_err_printf("ERROR: Invalid pointer to js handle\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    /* Alloc memory for scheduler object */
    *pctxt = (smpd_hpc_js_ctxt_t )MPIU_Malloc(sizeof(smpd_hpc_js_ctxt_));
    if(*pctxt == NULL){
        smpd_err_printf("ERROR: Unable to allocate memory for js handle\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    (*pctxt)->pscheduler = NULL;
    (*pctxt)->pnode_names = NULL;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* Get an instance of the Scheduler object */
    hr = CoCreateInstance( __uuidof(Scheduler),
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           __uuidof(IScheduler),
                           reinterpret_cast<void **>(&((*pctxt)->pscheduler)));

    CoUninitialize();

    if (FAILED(hr)){
        smpd_err_printf("ERROR: CoCreateInstance(IScheduler) failed, 0x%x\n", hr);
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    /* Connect to the head node */
    result = smpd_hpc_js_connect_to_head_node(*pctxt, NULL);
    if(result != SMPD_SUCCESS){
        smpd_err_printf("Unable to connect to head node \n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_hpc_js_finalize"
int smpd_hpc_js_finalize(smpd_hpc_js_ctxt_t *pctxt)
{
    int result;
    smpd_enter_fn(FCNAME);

    if(pctxt == NULL){
        smpd_err_printf("ERROR: Invalid pointer to js ctxt\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    if(*pctxt == NULL){
        smpd_dbg_printf("Null js handle\n");
        smpd_exit_fn(FCNAME);
        return SMPD_SUCCESS;
    }

    /* Release the job scheduler object */
    if((*pctxt)->pscheduler){
        ((*pctxt)->pscheduler)->Release();
        (*pctxt)->pscheduler = NULL;
    }

    /* Free the job scheduler handle */
    MPIU_Free(*pctxt);
    *pctxt = NULL;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}


#undef FCNAME
#define FCNAME "smpd_hpc_js_task_setenv"
int smpd_hpc_js_task_setenv(ISchedulerTask *ptask, char *proc_encoded_env)
{
    char name[SMPD_MAX_ENV_LENGTH], equals[3], value[SMPD_MAX_ENV_LENGTH];
    wchar_t namew[SMPD_MAX_ENV_LENGTH], valuew[SMPD_MAX_ENV_LENGTH];
    HRESULT hr;

    smpd_enter_fn(FCNAME);
    if((ptask == NULL) || (proc_encoded_env == NULL)){
        smpd_err_printf("Invalid ptr to task or proc environment\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    for (;;){
        name[0] = '\0';
        equals[0] = '\0';
        value[0] = '\0';
        if (MPIU_Str_get_string(&proc_encoded_env, name, SMPD_MAX_ENV_LENGTH) != MPIU_STR_SUCCESS)
            break;
        if (name[0] == '\0')
            break;
        if (MPIU_Str_get_string(&proc_encoded_env, equals, 3) != MPIU_STR_SUCCESS)
            break;
        if (equals[0] == '\0')
            break;
        if (MPIU_Str_get_string(&proc_encoded_env, value, SMPD_MAX_ENV_LENGTH) != MPIU_STR_SUCCESS)
            break;
        smpd_dbg_printf("setting environment variable: <%s> = <%s>\n", name, value);
        mbstowcs(namew, name, SMPD_MAX_ENV_LENGTH);
        mbstowcs(valuew, value, SMPD_MAX_ENV_LENGTH);
        hr = ptask->SetHpcEnvironmentVariable(_bstr_t(namew), _bstr_t(valuew));
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_hpc_js_create_job"
int smpd_hpc_js_create_job(smpd_hpc_js_ctxt_t ctxt, smpd_launch_node_t *head, ISchedulerJob **pp_job)
{
    int result;
    HRESULT hr;
    ISchedulerJob *pjob;

    smpd_enter_fn(FCNAME);
    if(!SMPDU_HPC_JS_CTXT_IS_VALID(ctxt)){
        smpd_err_printf("ERROR: Invalid handle to hpc job scheduler\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    if(head == NULL){
        smpd_err_printf("ERROR: Invalid list of launch nodes\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    if(pp_job == NULL){
        smpd_err_printf("ERROR: Invalid ptr to ptr to job object\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    
    hr = (ctxt->pscheduler)->CreateJob(pp_job);
    if(FAILED(hr)){
        smpd_err_printf("ERROR: Creating job failed, 0x%x\n", hr);
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    
    pjob = *pp_job;

    /* Set the properties of the job */
    hr = pjob->put_Name(_bstr_t(L"MPICH2_JOB"));
    if(FAILED(hr)){
        smpd_dbg_printf("Unable to set the name of the job\n");
    }

    while(head){
        /* FIXME: We are not releasing these tasks allocated here */
        ISchedulerTask *ptask = NULL;
        wchar_t exe_namew[SMPD_MAX_EXE_LENGTH], wdir[SMPD_MAX_DIR_LENGTH];
        wchar_t filename[SMPD_MAX_EXE_LENGTH];

        hr = pjob->CreateTask(&ptask);
        if(FAILED(hr)){
            smpd_err_printf("ERROR: Creating task failed, 0x%x\n", hr);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }

        result = smpd_hpc_js_task_setenv(ptask, head->env_data);
        if(result != SMPD_SUCCESS){
            smpd_err_printf("Unable to add env variable to task\n");
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        mbstowcs(exe_namew, head->exe, SMPD_MAX_EXE_LENGTH);
        hr = ptask->put_CommandLine(_bstr_t(exe_namew));
        if(FAILED(hr)){
            smpd_err_printf("ERROR: Adding command to task failed, 0x%x\n", hr);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }

        /* Set the task properties */
        mbstowcs(wdir, head->dir, SMPD_MAX_DIR_LENGTH);
        hr = ptask->put_WorkDirectory(_bstr_t(wdir));
        if(FAILED(hr)){
            smpd_err_printf("ERROR: Unable to set the working directory for job\n");
        }

        _snwprintf_s(filename, SMPD_MAX_EXE_LENGTH, SMPD_MAX_EXE_LENGTH - 1, L"stdout_mpich2_%s_%d_%d.txt", exe_namew, head->iproc, head->nproc);
        hr = ptask->put_StdOutFilePath(_bstr_t(filename));
        if(FAILED(hr)){
            smpd_err_printf("ERROR: Unable to set the stdout file path\n");
        }

        _snwprintf_s(filename, SMPD_MAX_EXE_LENGTH, SMPD_MAX_EXE_LENGTH - 1, L"stderr_mpich2_%s_%d_%d.txt", exe_namew, head->iproc, head->nproc);
        hr = ptask->put_StdErrFilePath(_bstr_t(filename));
        if(FAILED(hr)){
            smpd_err_printf("ERROR: Unable to set the stderr file path\n");
        }

        hr = pjob->AddTask(ptask);
        if(FAILED(hr)){
            smpd_err_printf("ERROR: Adding task to job failed, 0x%x\n", hr);
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        head = head->next;
    }

    /* Set the number of cores required for the job */
    /* FIXME: We are not using the node collection right now */

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_hpc_js_submit_job"
int smpd_hpc_js_submit_job(smpd_hpc_js_ctxt_t ctxt, ISchedulerJob *pjob)
{
    int result;
    HRESULT hr;

    smpd_enter_fn(FCNAME);
    if(!SMPDU_HPC_JS_CTXT_IS_VALID(ctxt)){
        smpd_err_printf("ERROR: Invalid handle to hpc job scheduler\n");
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    hr = (ctxt->pscheduler)->SubmitJob(pjob, NULL, NULL);
    if(FAILED(hr)){
        smpd_err_printf("ERROR: Submitting job failed, 0x%x\n", hr);
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
