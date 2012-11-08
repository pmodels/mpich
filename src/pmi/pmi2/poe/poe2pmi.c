/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <dlfcn.h>
#include "mpichconf.h"
#include "pmi2.h"
#include "mpiimpl.h"

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#if defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#ifdef USE_PMI_PORT
#ifndef MAXHOSTNAME
#define MAXHOSTNAME 256
#endif
#endif

#define PMII_EXIT_CODE -1

#define PMI_VERSION    2
#define PMI_SUBVERSION 0

#define MAX_INT_STR_LEN 11 /* number of digits in MAX_UINT + 1 */

int (*mp_world_exiting_handler)(int) = NULL;
typedef enum { PMI2_UNINITIALIZED = 0, NORMAL_INIT_WITH_PM = 1 } PMI2State;
static PMI2State PMI2_initialized = PMI2_UNINITIALIZED;

static int PMI2_debug = 0;
static int PMI2_fd = -1;
static int PMI2_size = 1;
static int PMI2_rank = 0;

static int PMI2_debug_init = 0;    /* Set this to true to debug the init */

int PMI2_pmiverbose = 0;    /* Set this to true to print PMI debugging info */

#ifdef MPICH_IS_THREADED
static MPID_Thread_mutex_t mutex;
static int blocked = FALSE;
static MPID_Thread_cond_t cond;
#endif

extern int mpidi_finalized;
extern int (*mp_world_exiting_handler)(int);
extern int _mpi_world_exiting_handler(int);

void *poeptr = NULL;

/* ------------------------------------------------------------------------- */
/* PMI API Routines */
/* ------------------------------------------------------------------------- */
int PMI2_Init(int *spawned, int *size, int *rank, int *appnum)
{
    int pmi2_errno = PMI2_SUCCESS;
    char *p;
    char *jobid;
    char *pmiid;
    int ret;

    int (*pmi2_init)(int*, int*, int *, int*);

    poeptr = dlopen("libpoe.so",RTLD_NOW|RTLD_GLOBAL);
    if (poeptr == NULL) {
        TRACE_ERR("failed to open libpoe.so\n");
    }

    mp_world_exiting_handler = &(_mpi_world_exiting_handler);

    pmi2_init = (int (*)())dlsym(poeptr, "PMI2_Init");
    if (pmi2_init == NULL) {
        TRACE_ERR("failed to dlsym PMI2_Init\n");
    }

    return (*pmi2_init)(spawned, size, rank, appnum);
}

int PMI2_Finalize(void)
{
    int pmi2_errno = PMI2_SUCCESS;
    int rc;
    const char *errmsg;

    int (*pmi2_finalize)(void);

    pmi2_finalize = (int (*)())dlsym(poeptr, "PMI2_Finalize");
    if (pmi2_finalize == NULL) {
        TRACE_ERR("failed to dlsym PMI2_Finalize\n");
    }

    return (*pmi2_finalize)();

}

int PMI2_Initialized(void)
{
    /* Turn this into a logical value (1 or 0) .  This allows us
       to use PMI2_initialized to distinguish between initialized with
       an PMI service (e.g., via mpiexec) and the singleton init,
       which has no PMI service */
    return PMI2_initialized != 0;
}

int PMI2_Abort( int flag, const char msg[] )
{
    int (*pmi2_abort)(int, const char*);

    pmi2_abort = (int (*)())dlsym(poeptr, "PMI2_Abort");
    if (pmi2_abort == NULL) {
        TRACE_ERR("failed to dlsym pmi2_abort\n");
    }

    return (*pmi2_abort)(flag, msg);
}

int PMI2_Job_Spawn(int count, const char * cmds[],
                   int argcs[], const char ** argvs[],
                   const int maxprocs[],
                   const int info_keyval_sizes[],
                   const struct MPID_Info *info_keyval_vectors[],
                   int preput_keyval_size,
                   const struct MPID_Info *preput_keyval_vector[],
                   char jobId[], int jobIdSize,
                   int errors[])
{
    int  i,rc,spawncnt,total_num_processes,num_errcodes_found;
    int found;
    const char *jid;
    int jidlen;
    char *lead, *lag;
    int spawn_rc;
    const char *errmsg = NULL;
    int pmi2_errno = 0;

    int (*pmi2_job_spawn)(int , const char * [], int [], const char ** [],const int [],const int [],const struct MPID_Info *[],int ,const struct MPID_Info *[],char jobId[],int ,int []);

    pmi2_job_spawn = (int (*)())dlsym(poeptr, "PMI2_Job_Spawn");
    if (pmi2_job_spawn == NULL) {
        TRACE_ERR("failed to dlsym pmi2_job_spawn\n");
    }

    return (*pmi2_job_spawn)(count, cmds, argcs, argvs, maxprocs,
                             info_keyval_sizes, info_keyval_vectors,
                             preput_keyval_size, preput_keyval_vector,
                             jobId, jobIdSize, errors);

}

int PMI2_Job_GetId(char jobid[], int jobid_size)
{
    int pmi2_errno = PMI2_SUCCESS;
    int found;
    const char *jid;
    int jidlen;
    int rc;
    const char *errmsg;

    int (*pmi2_job_getid)(char*, int);

    pmi2_job_getid = (int (*)())dlsym(poeptr, "PMI2_Job_GetId");
    if (pmi2_job_getid == NULL) {
        TRACE_ERR("failed to dlsym pmi2_job_getid\n");
    }

    return (*pmi2_job_getid)(jobid, jobid_size);
}


int PMI2_KVS_Put(const char key[], const char value[])
{
    int pmi2_errno = PMI2_SUCCESS;
    int rc;
    const char *errmsg;

    int (*pmi2_kvs_put)(const char*, const char*);

    pmi2_kvs_put = (int (*)())dlsym(poeptr, "PMI2_KVS_Put");
    if (pmi2_kvs_put == NULL) {
        TRACE_ERR("failed to dlsym pmi2_kvs_put\n");
    }

    return (*pmi2_kvs_put)(key, value);
}

int PMI2_KVS_Fence(void)
{
    int pmi2_errno = PMI2_SUCCESS;
    int rc;
    const char *errmsg;

    int (*pmi2_kvs_fence)(void);

    pmi2_kvs_fence = (int (*)())dlsym(poeptr, "PMI2_KVS_Fence");
    if (pmi2_kvs_fence == NULL) {
        TRACE_ERR("failed to dlsym pmi2_kvs_fence\n");
    }

    return (*pmi2_kvs_fence)();
}

int PMI2_KVS_Get(const char *jobid, int src_pmi_id, const char key[], char value [], int maxValue, int *valLen)
{
    int pmi2_errno = PMI2_SUCCESS;
    int found, keyfound;
    const char *kvsvalue;
    int kvsvallen;
    int ret;
    int rc;
    char src_pmi_id_str[256];
    const char *errmsg;

    int (*pmi2_kvs_get)(const char*, int, const char *, char *, int, int*);

    pmi2_kvs_get = (int (*)())dlsym(poeptr, "PMI2_KVS_Get");
    if (pmi2_kvs_get == NULL) {
        TRACE_ERR("failed to dlsym pmi2_kvs_get\n");
    }

    return (*pmi2_kvs_get)(jobid, src_pmi_id, key, value, maxValue, valLen);
}


int PMI2_Info_GetJobAttr(const char name[], char value[], int valuelen, int *flag)
{
    int pmi2_errno = PMI2_SUCCESS;
    int found;
    const char *kvsvalue;
    int kvsvallen;
    int rc;
    const char *errmsg;

    int (*pmi2_info_getjobattr)(const char*, char *, int, int*);

    pmi2_info_getjobattr = (int (*)())dlsym(poeptr, "PMI2_Info_GetJobAttr");
    if (pmi2_info_getjobattr == NULL) {
        TRACE_ERR("failed to dlsym pmi2_info_getjobattr\n");
    }

    return (*pmi2_info_getjobattr)(name, value, valuelen, flag);
}


/**
 * This is the mpi level of callback that get invoked when a task get notified
 * of a world's exiting
 */
int _mpi_world_exiting_handler(int world_id)
{
  /* check the reference count associated with that remote world
     if the reference count is zero, the task will call LAPI_Purge_totask on
     all tasks in that world,reset MPCI. It would also remove the world
     structure corresponding to that world ID
     if the reference count is not zero, it should call STOPALL
  */
  int rc,ref_count = -1;
  int  *taskid_list = NULL;
  int i;
  int my_state=FALSE,reduce_state=FALSE;
  char world_id_str[32];
  int mpi_errno = MPI_SUCCESS;
  pami_endpoint_t dest;

  if(!mpidi_finalized) {
    ref_count = MPIDI_get_refcnt_of_world(world_id);
    TRACE_ERR("_mpi_world_exiting_handler: invoked for world %d exiting ref_count=%d\n", world_id, ref_count);
    if(ref_count == 0) {
      taskid_list = MPIDI_get_taskids_in_world_id(world_id);
      if(taskid_list != NULL) {
        for(i=0;taskid_list[i]!=-1;i++) {
          PAMI_Endpoint_create(MPIDI_Client, taskid_list[i], 0, &dest);
	  MPIDI_OpState_reset(taskid_list[i]);
	  MPIDI_IpState_reset(taskid_list[i]);
	  TRACE_ERR("PAMI_Purge on taskid_list[%d]=%d\n", i,taskid_list[i]);
	  if(MPIDI_Context[0])
            PAMI_Purge(MPIDI_Context[0], &dest, 1);
        }
        MPIDI_delete_conn_record(world_id);
      }
      rc = -1;
    }
    my_state = TRUE;

/*  _mpi_reduce_for_dyntask(&my_state, &reduce_state); */
    if(MPIDI_Context[0])
      MPIR_Reduce_impl(&my_state,&reduce_state,1,
                       MPI_INT,MPI_LAND,0,MPIR_Process.comm_world,&mpi_errno);
    TRACE_ERR("_mpi_world_exiting_handler: Out of _mpi_reduce_for_dyntask for exiting world %d reduce_state=%d\n",world_id, reduce_state);
  }

  if(MPIR_Process.comm_world->rank == 0) {
    MPIU_Snprintf(world_id_str, sizeof(world_id_str), "%d", world_id);
    PMI2_Abort(0, world_id_str);
/*    _mp_send_exiting_ack(world_id); */
    if(MPIDI_Context[0] && (reduce_state != TRUE)) {
      TRACE_ERR("root is exiting with error\n");
      exit(-1);
    }
    TRACE_ERR("_mpi_world_exiting_handler: Root finished sending SSM_WORLD_EXITING to POE for exiting world %d\n",world_id);
  }

  if(ref_count != 0) {
    TRACE_ERR("STOPALL is sent by task %d\n", PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_TASK_ID  ).value.intval);
    PMI2_Abort(1, "STOPALL should be sent");
    rc = -2;
  }

  return rc;
}
