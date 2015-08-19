/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/mpid_init.c
 * \brief Normal job startup code
 */

#include <stdlib.h>
#include <string.h>

#include <mpidimpl.h>
#include "mpidi_platform.h"
#include "onesided/mpidi_onesided.h"

#include "mpidi_util.h"

#ifdef DYNAMIC_TASKING
#define PAMIX_CLIENT_DYNAMIC_TASKING 1032
#define PAMIX_CLIENT_WORLD_TASKS     1033
#define MAX_JOBID_LEN                1024
int     world_rank;
int     world_size;
extern int (*mp_world_exiting_handler)(int);
extern int _mpi_world_exiting_handler(int);
#endif
int mpidi_dynamic_tasking = 0;

#if TOKEN_FLOW_CONTROL
  extern int MPIDI_mm_init(int,uint *,unsigned long *);
  extern int MPIDI_tfctrl_enabled;
#endif

#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
  pami_extension_t pe_extension;
#endif

pami_client_t    MPIDI_Client;
pami_context_t   MPIDI_Context[MPIDI_MAX_CONTEXTS];

MPIDI_Process_t  MPIDI_Process = {
  .verbose               = 0,
  .statistics            = 0,

#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
  .avail_contexts        = MPIDI_MAX_CONTEXTS,
  .async_progress = {
    .active              = 0,
    .mode                = ASYNC_PROGRESS_MODE_DEFAULT,
  },
  .perobj = {
    .context_post = {
      .requested         = (ASYNC_PROGRESS_MODE_DEFAULT == ASYNC_PROGRESS_MODE_LOCKED),
      .active            = 0,
    },
  },
#else
  .avail_contexts        = 1,
  .async_progress = {
    .active              = 0,
    .mode                = ASYNC_PROGRESS_MODE_DISABLED,
  },
  .perobj = {
    .context_post = {
      .requested         = 0,
      .active            = 0,
    },
  },
#endif
  .pt2pt = {
    .limits = {
      .application = {
        .eager = {
          .remote        = MPIDI_EAGER_LIMIT,
          .local         = MPIDI_EAGER_LIMIT_LOCAL,
        },
        .immediate = {
          .remote        = MPIDI_SHORT_LIMIT,
          .local         = MPIDI_SHORT_LIMIT,
        },
      },
      .internal = {
        .eager = {
          .remote        = MPIDI_EAGER_LIMIT,
          .local         = MPIDI_EAGER_LIMIT_LOCAL,
        },
        .immediate = {
          .remote        = MPIDI_SHORT_LIMIT,
          .local         = MPIDI_SHORT_LIMIT,
        },
      },
    },
  },
  .disable_internal_eager_scale = MPIDI_DISABLE_INTERNAL_EAGER_SCALE,
#if TOKEN_FLOW_CONTROL
  .mp_buf_mem          = BUFFER_MEM_DEFAULT,
  .mp_buf_mem_max      = BUFFER_MEM_DEFAULT,
  .is_token_flow_control_on = 0,
#endif
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
  .mp_infolevel          = 0,
  .mp_statistics         = 0,
  .mp_printenv           = 0,
#endif
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
  .queue_binary_search_support_on = 0,
#endif
#if CUDA_AWARE_SUPPORT
  .cuda_aware_support_on = 0,
#endif
  .rma_pending           = 1000,
  .shmem_pt2pt           = 1,
  .smp_detect            = MPIDI_SMP_DETECT_DEFAULT,
  .optimized = {
    .collectives         = MPIDI_OPTIMIZED_COLLECTIVE_DEFAULT,
    .subcomms            = 1,
    .select_colls        = 2,
    .memory              = 0,
    .num_requests        = 16,
  },

  .mpir_nbc              = 1,
  .numTasks              = 0,
  .typed_onesided        = 0,
};


struct protocol_t
{
  pami_dispatch_p2p_function func;
  size_t                     dispatch;
  size_t                     immediate_min;
  pami_dispatch_hint_t       options;
};
static struct
{
  struct protocol_t Short;
  struct protocol_t ShortSync;
  struct protocol_t Eager;
  struct protocol_t RVZ;
  struct protocol_t Cancel;
  struct protocol_t Control;
  struct protocol_t WinCtrl;
  struct protocol_t WinAccum;
  struct protocol_t RVZ_zerobyte;
  struct protocol_t WinGetAccum;
  struct protocol_t WinGetAccumAck;
  struct protocol_t WinAtomic;
  struct protocol_t WinAtomicAck;
#ifdef DYNAMIC_TASKING
  struct protocol_t Dyntask;
  struct protocol_t Dyntask_disconnect;
#endif
} proto_list = {
  .Short = {
    .func = MPIDI_RecvShortAsyncCB,
    .dispatch = MPIDI_Protocols_Short,
    .options = {
      .consistency     = USE_PAMI_CONSISTENCY,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_ENABLE,
      .use_rdma        = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_MsgInfo),
  },
  .ShortSync = {
    .func = MPIDI_RecvShortSyncCB,
    .dispatch = MPIDI_Protocols_ShortSync,
    .options = {
      .consistency     = USE_PAMI_CONSISTENCY,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_ENABLE,
      .use_rdma        = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_MsgInfo),
  },
  .Eager = {
    .func = MPIDI_RecvCB,
    .dispatch = MPIDI_Protocols_Eager,
    .options = {
      .consistency     = USE_PAMI_CONSISTENCY,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_contiguous = PAMI_HINT_ENABLE,
      .recv_copy =       PAMI_HINT_ENABLE,
    },
    .immediate_min     = sizeof(MPIDI_MsgInfo),
  },
  .RVZ = {
    .func = MPIDI_RecvRzvCB,
    .dispatch = MPIDI_Protocols_RVZ,
    .options = {
      .consistency     = USE_PAMI_CONSISTENCY,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_ENABLE,
      .use_rdma        = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_MsgEnvelope),
  },
  .Cancel = {
    .func = MPIDI_ControlCB,
    .dispatch = MPIDI_Protocols_Cancel,
    .options = {
      .consistency     = PAMI_HINT_ENABLE,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_ENABLE,
      .use_rdma        = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_MsgInfo),
  },
  .Control = {
    .func = MPIDI_ControlCB,
    .dispatch = MPIDI_Protocols_Control,
    .options = {
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_ENABLE,
      .use_rdma        = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_MsgInfo),
  },
  .WinCtrl = {
    .func = MPIDI_WinControlCB,
    .dispatch = MPIDI_Protocols_WinCtrl,
    .options = {
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_ENABLE,
      .use_rdma        = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_Win_control_t),
  },
  .WinAccum = {
    .func = MPIDI_WinAccumCB,
    .dispatch = MPIDI_Protocols_WinAccum,
    .options = {
      .consistency     = PAMI_HINT_ENABLE,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_MsgInfo),
  },
  .RVZ_zerobyte = {
    .func = MPIDI_RecvRzvCB_zerobyte,
    .dispatch = MPIDI_Protocols_RVZ_zerobyte,
    .options = {
      .consistency     = USE_PAMI_CONSISTENCY,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_ENABLE,
      .use_rdma        = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_MsgEnvelope),
  },
  .WinGetAccum = {
    .func = MPIDI_WinGetAccumCB,
    .dispatch = MPIDI_Protocols_WinGetAccum,
    .options = {
      .consistency    = PAMI_HINT_ENABLE,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_Win_GetAccMsgInfo),
  },
  .WinGetAccumAck = {
    .func = MPIDI_WinGetAccumAckCB,
    .dispatch = MPIDI_Protocols_WinGetAccumAck,
    .options = {
      .consistency    = PAMI_HINT_ENABLE,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_Win_GetAccMsgInfo),
  },
  .WinAtomic = {
    .func = MPIDI_WinAtomicCB,
    .dispatch = MPIDI_Protocols_WinAtomic,
    .options = {
      .consistency     = USE_PAMI_CONSISTENCY,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_ENABLE,
      .use_rdma        = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_AtomicHeader_t),
  },
  .WinAtomicAck = {
    .func = MPIDI_WinAtomicAckCB,
    .dispatch = MPIDI_Protocols_WinAtomicAck,
    .options = {
      .consistency     = USE_PAMI_CONSISTENCY,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_ENABLE,
      .use_rdma        = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_AtomicHeader_t),
  },
#ifdef DYNAMIC_TASKING
  .Dyntask = {
    .func = MPIDI_Recvfrom_remote_world,
    .dispatch = MPIDI_Protocols_Dyntask,
    .options = {
      .consistency     = USE_PAMI_CONSISTENCY,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_ENABLE,
      .use_rdma        = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_MsgInfo),
  },
  .Dyntask_disconnect = {
    .func = MPIDI_Recvfrom_remote_world_disconnect,
    .dispatch = MPIDI_Protocols_Dyntask_disconnect,
    .options = {
      .consistency     = USE_PAMI_CONSISTENCY,
      .long_header     = PAMI_HINT_DISABLE,
      .recv_immediate  = PAMI_HINT_ENABLE,
      .use_rdma        = PAMI_HINT_DISABLE,
    },
    .immediate_min     = sizeof(MPIDI_MsgInfo),
  },
#endif
};


#undef FUNCNAME
#define FUNCNAME split_type
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int split_type(MPID_Comm * comm_ptr, int stype, int key,
                      MPID_Info *info_ptr, MPID_Comm ** newcomm_ptr)
{
    MPID_Node_id_t id;
    int nid;
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_Get_node_id(comm_ptr, comm_ptr->rank, &id);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    nid = (stype == MPI_COMM_TYPE_SHARED) ? id : MPI_UNDEFINED;
    mpi_errno = MPIR_Comm_split_impl(comm_ptr, nid, key, newcomm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

static MPID_CommOps comm_fns = {
    split_type
};


/* ------------------------------ */
/* Collective selection extension */
/* ------------------------------ */
pami_extension_t MPIDI_Collsel_extension;
advisor_t        MPIDI_Collsel_advisor;
advisor_table_t  MPIDI_Collsel_advisor_table;
advisor_params_t MPIDI_Collsel_advisor_params;
char            *MPIDI_Collsel_output_file;
pami_extension_collsel_advise MPIDI_Pamix_collsel_advise;
static void
MPIDI_PAMI_client_init(int* rank, int* size, int* mpidi_dynamic_tasking, char **world_tasks)
{
  /* ------------------------------------ */
  /*  Initialize the MPICH->PAMI Client  */
  /* ------------------------------------ */
  pami_result_t        rc = PAMI_ERROR;
  
  pami_configuration_t config[2];
  size_t numconfigs = 0;

  /* Set the status for memory optimized collectives */
  {
    char* env = getenv("PAMID_COLLECTIVES_MEMORY_OPTIMIZED");
    if (env != NULL)
      MPIDI_atoi(env,&MPIDI_Process.optimized.memory);
  }

#ifdef HAVE_PAMI_CLIENT_NONCONTIG
  config[0].name = PAMI_CLIENT_NONCONTIG;
  if(MPIDI_Process.optimized.memory & MPID_OPT_LVL_NONCONTIG) 
    config[0].value.intval = 0; // Disable non-contig, pamid doesn't use pami for non-contig data collectives so save memory
  else
    config[0].value.intval = 1; // Enable non-contig even though pamid doesn't use pami for non-contig data collectives, 
                                // we still possibly want those collectives for other reasons.
  ++numconfigs;
#endif
#ifdef HAVE_PAMI_CLIENT_MEMORY_OPTIMIZE
  if(MPIDI_Process.optimized.memory) 
  {
    config[numconfigs].name = PAMI_CLIENT_MEMORY_OPTIMIZE;
    config[numconfigs].value.intval = MPIDI_Process.optimized.memory;
    ++numconfigs;
  }
#endif

  rc = PAMI_Client_create("MPI", &MPIDI_Client, config, numconfigs);
  MPID_assert_always(rc == PAMI_SUCCESS);
  PAMIX_Initialize(MPIDI_Client);


  *mpidi_dynamic_tasking=0;
#ifdef DYNAMIC_TASKING
  *world_tasks = NULL;
  pami_result_t status = PAMI_ERROR;

  typedef pami_result_t (*dyn_task_query_fn) (
             pami_client_t          client,
             pami_configuration_t   config[],
             size_t                 num_configs);
  dyn_task_query_fn  dyn_task_query = NULL;

  pami_extension_t extension;
  status = PAMI_Extension_open (MPIDI_Client, "PE_dyn_task", &extension);
  if(status != PAMI_SUCCESS)
  {
    TRACE_ERR("Error. The PE_dyn_task extension is not implemented. result = %d\n", status);
  }

  dyn_task_query =  (dyn_task_query_fn) PAMI_Extension_symbol(extension, "query");
  if (dyn_task_query == (void*)NULL) {
    TRACE_ERR("Err: the Dynamic Tasking extension function dyn_task_query is not implememted.\n");

  } else {
    pami_configuration_t config2[] =
    {
       {PAMI_CLIENT_TASK_ID, -1},
       {PAMI_CLIENT_NUM_TASKS, -1},
       {(pami_attribute_name_t)PAMIX_CLIENT_DYNAMIC_TASKING},
       {(pami_attribute_name_t)PAMIX_CLIENT_WORLD_TASKS},
    };

    dyn_task_query(MPIDI_Client, config2, 4);
    TRACE_ERR("dyn_task_query: task_id %d num_tasks %d dynamic_tasking %d world_tasks %s\n",
              config2[0].value.intval,
              config2[1].value.intval,
              config2[2].value.intval,
              config2[3].value.chararray);
    *rank = world_rank = config2[0].value.intval;
    *size = world_size = config2[1].value.intval;
    *mpidi_dynamic_tasking  = config2[2].value.intval;
    *world_tasks = config2[3].value.chararray;
  }

  status = PAMI_Extension_close (extension);
  if(status != PAMI_SUCCESS)
  {
    TRACE_ERR("Error. The PE_dyn_task extension could not be closed. result = %d\n", status);
  }
#endif

  if(*mpidi_dynamic_tasking == 0) {
     /* ---------------------------------- */
     /*  Get my rank and the process size  */
     /* ---------------------------------- */
     *rank = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_TASK_ID  ).value.intval;
     MPIR_Process.comm_world->rank = *rank; /* Set the rank early to make tracing better */
     *size = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_NUM_TASKS).value.intval;
  }

  /* --------------------------------------------------------------- */
  /* Determine if the eager point-to-point protocol for internal mpi */
  /* operations should be disabled.                                  */
  /* --------------------------------------------------------------- */
  {
    char * env = getenv("PAMID_DISABLE_INTERNAL_EAGER_TASK_LIMIT");
    if (env != NULL)
      {
        size_t n = strlen(env);
        char * tmp = (char *) MPIU_Malloc(n+1);
        strncpy(tmp,env,n);
        if (n>0) tmp[n]=0;

        MPIDI_atoi(tmp, &MPIDI_Process.disable_internal_eager_scale);

        MPIU_Free(tmp);
      }

    if (MPIDI_Process.disable_internal_eager_scale <= *size)
      {
        MPIDI_Process.pt2pt.limits.internal.eager.remote     = 0;
        MPIDI_Process.pt2pt.limits.internal.eager.local      = 0;
        MPIDI_Process.pt2pt.limits.internal.immediate.remote = 0;
        MPIDI_Process.pt2pt.limits.internal.immediate.local  = 0;
      }
  }
}

void MPIDI_Init_collsel_extension()
{
  pami_result_t status = PAMI_ERROR;
  status = PAMI_Extension_open (MPIDI_Client, "EXT_collsel", &MPIDI_Collsel_extension);
  if(status == PAMI_SUCCESS)
  {
    if(MPIDI_Process.optimized.auto_select_colls == MPID_AUTO_SELECT_COLLS_TUNE)
    {
      advisor_configuration_t configuration[1];
      pami_extension_collsel_init pamix_collsel_init =
         (pami_extension_collsel_init) PAMI_Extension_symbol (MPIDI_Collsel_extension, "Collsel_init");
      status = pamix_collsel_init (MPIDI_Client, configuration, 1, &MPIDI_Context[0], 1, &MPIDI_Collsel_advisor);
      if(status != PAMI_SUCCESS)
      {
        fprintf (stderr, "Error. The collsel_init failed. result = %d\n", status);
        MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_NONE;
      }

    }
    else if(MPIDI_Process.optimized.auto_select_colls == MPID_AUTO_SELECT_COLLS_ALL)
    {
      pami_extension_collsel_initialized pamix_collsel_initialized =
         (pami_extension_collsel_initialized) PAMI_Extension_symbol(MPIDI_Collsel_extension,
                                                                    "Collsel_initialized");
      if(pamix_collsel_initialized(MPIDI_Client, &MPIDI_Collsel_advisor) == 1)
      {
        char *collselfile;
        collselfile = getenv("MP_COLLECTIVE_SELECTION_FILE");
        pami_extension_collsel_table_load pamix_collsel_table_load =
           (pami_extension_collsel_table_load) PAMI_Extension_symbol(MPIDI_Collsel_extension,
                                                                       "Collsel_table_load");
        if(collselfile != NULL)
          status = pamix_collsel_table_load(MPIDI_Collsel_advisor, collselfile, &MPIDI_Collsel_advisor_table);
        else
          status = pamix_collsel_table_load(MPIDI_Collsel_advisor, "pami_tune_results.xml", &MPIDI_Collsel_advisor_table);
          if (status == PAMI_SUCCESS)
          {
            pami_xfer_type_t *collsel_collectives = NULL;
            unsigned          num_collectives;
            pami_extension_collsel_get_collectives pamix_collsel_get_collectives =
               (pami_extension_collsel_get_collectives) PAMI_Extension_symbol(MPIDI_Collsel_extension,
                                                                              "Collsel_get_collectives");
            status = pamix_collsel_get_collectives(MPIDI_Collsel_advisor_table, &collsel_collectives, &num_collectives);
            MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_NONE;
            if(collsel_collectives != NULL)
            {
              unsigned i = 0;
              for(i = 0; i < num_collectives; i++)
              {
                switch(collsel_collectives[i])
                {
                  case PAMI_XFER_BROADCAST:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_BCAST;
                    break;
                  case PAMI_XFER_ALLREDUCE:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_ALLREDUCE;
                    break;
                  case PAMI_XFER_REDUCE:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_REDUCE;
                    break;
                  case PAMI_XFER_ALLGATHER:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_ALLGATHER;
                    break;
                  case PAMI_XFER_ALLGATHERV:
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
                    if(MPIDI_Process.mp_infolevel >= 1)
                      fprintf(stderr,"WARNING: MPICH (collective selection) doesn't support ALLGATHERV, only ALLGATHERV_INT is supported\n");
#endif
                    break;
                  case PAMI_XFER_ALLGATHERV_INT:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_ALLGATHERV;
                    break;
                  case PAMI_XFER_SCATTER:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_SCATTER;
                    break;
                  case PAMI_XFER_SCATTERV:
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
                    if(MPIDI_Process.mp_infolevel >= 1)
                      fprintf(stderr,"WARNING: MPICH (collective selection) doesn't support SCATTERV, only SCATTERV_INT is supported\n");
#endif
                    break;
                  case PAMI_XFER_SCATTERV_INT:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_SCATTERV;
                    break;
                  case PAMI_XFER_GATHER:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_GATHER;
                    break;
                  case PAMI_XFER_GATHERV:
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
                    if(MPIDI_Process.mp_infolevel >= 1)
                      fprintf(stderr,"WARNING: MPICH (collective selection) doesn't support GATHERV, only GATHERV_INT is supported\n");
#endif
                    break;
                  case PAMI_XFER_GATHERV_INT:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_GATHERV;
                    break;
                  case PAMI_XFER_BARRIER:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_BARRIER;
                    break;
                  case PAMI_XFER_ALLTOALL:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_ALLTOALL;
                    break;
                  case PAMI_XFER_ALLTOALLV:
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
                    if(MPIDI_Process.mp_infolevel >= 1)
                      fprintf(stderr,"WARNING: MPICH (collective selection) doesn't support ALLTOALLV, only ALLTOALLV_INT is supported\n");
#endif
                    break;
                  case PAMI_XFER_ALLTOALLV_INT:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_ALLTOALLV;
                    break;
                  case PAMI_XFER_SCAN:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_SCAN;
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_EXSCAN;
                    break;
                  case PAMI_XFER_REDUCE_SCATTER:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_REDUCE_SCATTER;
                    break;
                  default:
                    MPIDI_Process.optimized.auto_select_colls |= MPID_AUTO_SELECT_COLLS_NONE;
                }
              }
              MPIU_Free(collsel_collectives);
            }
          }
          else
          {
            fprintf (stderr, "Error. Collsel_table_load failed. result = %d\n", status);
            MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_NONE;
          }
      }
      else
        MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_NONE;
    }
    else
      PAMI_Extension_close(MPIDI_Collsel_extension);
  }
  else
    MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_NONE;

#ifndef __BGQ__
  //If collective selection will be disabled, check on fca and CUDA if both not required, disable pami alltogether
  if(MPIDI_Process.optimized.auto_select_colls == MPID_AUTO_SELECT_COLLS_NONE && MPIDI_Process.optimized.collectives != MPID_COLL_FCA && MPIDI_Process.optimized.collectives != MPID_COLL_CUDA)
    MPIDI_Process.optimized.collectives = MPID_COLL_OFF;
#endif
}

void MPIDI_Collsel_table_generate()
{
  external_geometry_ops_t external_ops;
  external_ops.geometry_create     = MPIDI_Comm_create_from_pami_geom;
  external_ops.geometry_destroy    = MPIDI_Comm_destroy_external;
  external_ops.register_algorithms = MPIDI_Register_algorithms_ext;
  pami_result_t status = PAMI_SUCCESS;
  pami_extension_collsel_table_generate pamix_collsel_table_generate =
    (pami_extension_collsel_table_generate) PAMI_Extension_symbol (MPIDI_Collsel_extension, "Collsel_table_generate");

  status = pamix_collsel_table_generate (MPIDI_Collsel_advisor, MPIDI_Collsel_output_file, &MPIDI_Collsel_advisor_params, &external_ops, 1);
  if(status != PAMI_SUCCESS)
  {
    fprintf (stderr, "Error. The collsel_table_generate failed. result = %d\n", status);
  }

}


static void
MPIDI_PAMI_context_init(int* threading, int *size)
{
#ifdef TRACE_ON
  int requested_thread_level;
  requested_thread_level = *threading;
#endif
  int  numTasks;

#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)
  /*
   * ASYNC_PROGRESS_MODE_LOCKED requires context post because the async thread
   * will hold the context lock indefinitely; the only option for an application
   * thread to interact with the context is to use context post. See discussion
   * in src/mpid/pamid/src/mpid_progress.h for more information.
   *
   * There are three possible resolutions for the situation when context post is
   * disabled and async progess mode is 'locked':
   *  1. abort
   *  2. silently enable context post
   *  3. silently demote async progress mode to ASYNC_PROGRESS_MODE_TRIGGER
   *
   * For now this configuration is considered erroneous and mpi will abort.
   */
  if (MPIDI_Process.async_progress.mode == ASYNC_PROGRESS_MODE_LOCKED &&
      MPIDI_Process.perobj.context_post.requested == 0)
    MPID_Abort (NULL, 0, 1, "'locking' async progress requires context post");

#else /* MPICH_THREAD_GRANULARITY != MPICH_THREAD_GRANULARITY_PER_OBJECT */
  /*
   * ASYNC_PROGRESS_MODE_LOCKED is not applicable in the "global lock" thread
   * mode. See discussion in src/mpid/pamid/src/mpid_progress.h for more
   * information.
   *
   * This configuration is considered erroneous and mpi will abort.
   */
  if (MPIDI_Process.async_progress.mode == ASYNC_PROGRESS_MODE_LOCKED)
    MPID_Abort (NULL, 0, 1, "'locking' async progress not applicable");
#endif

  /* ----------------------------------
   *  Figure out the context situation
   * ---------------------------------- */
#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_PER_OBJECT)

  /* Limit the number of requested contexts by the maximum number of contexts
   * allowed.  The default number of requested contexts depends on the mpich
   * lock mode, 'global' or 'perobj', and may be changed before this point
   * by an environment variables.
   */
  if (MPIDI_Process.avail_contexts > MPIDI_MAX_CONTEXTS)
    MPIDI_Process.avail_contexts = MPIDI_MAX_CONTEXTS;

  unsigned same = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_CONST_CONTEXTS).value.intval;
  if (same)
    {
      /* Determine the maximum number of contexts supported; limit the number of
       * requested contexts by this value.
       */
      unsigned possible_contexts = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_NUM_CONTEXTS).value.intval;
      TRACE_ERR("PAMI allows up to %u contexts; MPICH allows up to %u\n",
                possible_contexts, MPIDI_Process.avail_contexts);
      if (MPIDI_Process.avail_contexts > possible_contexts)
        MPIDI_Process.avail_contexts = possible_contexts;
    }
  else
    {
      /* If PAMI didn't give all nodes the same number of contexts, all bets
       * are off for now */
      MPIDI_Process.avail_contexts = 1;
    }

  /* The number of contexts must be a power-of-two, as required by the
   * MPIDI_Context_hash() function. Decrement until we hit a power-of-two */
  while(MPIDI_Process.avail_contexts & (MPIDI_Process.avail_contexts-1))
    --MPIDI_Process.avail_contexts;
  MPID_assert_always(MPIDI_Process.avail_contexts);

#else /* (MPICH_THREAD_GRANULARITY != MPICH_THREAD_GRANULARITY_PER_OBJECT) */

  /* Only a single context is supported in the 'global' mpich lock mode.
   *
   * A multi-context application will always perform better with the
   * 'per object' mpich lock mode - regardless of whether async progress is
   * enabled or not. This is because all threads, application and async
   * progress, must acquire the single global lock which effectively serializes
   * the threads and negates any benefit of multiple contexts.
   *
   * This single context limitation removes code and greatly simplifies logic.
   */
  MPIDI_Process.avail_contexts = 1;

#endif

  TRACE_ERR ("Thread-level=%d, requested=%d\n", *threading, requested_thread_level);

  MPIDI_Process.numTasks= numTasks = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_NUM_TASKS).value.intval;
#ifdef OUT_OF_ORDER_HANDLING
  MPIDI_In_cntr = MPIU_Calloc0(numTasks, MPIDI_In_cntr_t);
  if(MPIDI_In_cntr == NULL)
    MPID_abort();
  MPIDI_Out_cntr = MPIU_Calloc0(numTasks, MPIDI_Out_cntr_t);
  if(MPIDI_Out_cntr == NULL)
    MPID_abort();
  memset((void *) MPIDI_In_cntr,0, sizeof(MPIDI_In_cntr_t));
  memset((void *) MPIDI_Out_cntr,0, sizeof(MPIDI_Out_cntr_t));
#endif


#ifdef MPIDI_TRACE
      int i;
      MPIDI_Trace_buf = MPIU_Calloc0(numTasks, MPIDI_Trace_buf_t);
      if(MPIDI_Trace_buf == NULL) MPID_abort();
      memset((void *) MPIDI_Trace_buf,0, sizeof(MPIDI_Trace_buf_t));
      for (i=0; i < numTasks; i++) {
          MPIDI_Trace_buf[i].R=MPIU_Calloc0(N_MSGS, recv_status);
          if (MPIDI_Trace_buf[i].R==NULL) MPID_abort();
          MPIDI_Trace_buf[i].PR=MPIU_Calloc0(N_MSGS, posted_recv);
          if (MPIDI_Trace_buf[i].PR ==NULL) MPID_abort();
          MPIDI_Trace_buf[i].S=MPIU_Calloc0(N_MSGS, send_status);
          if (MPIDI_Trace_buf[i].S ==NULL) MPID_abort();
      }
#endif

  /* ----------------------------------- */
  /*  Create the communication contexts  */
  /* ----------------------------------- */
  TRACE_ERR("Creating %d contexts\n", MPIDI_Process.avail_contexts);
  pami_result_t rc = PAMI_ERROR;
  pami_configuration_t config[3];
  int  cfgval=0;
  config[cfgval].name = PAMI_CLIENT_CONST_CONTEXTS,
  config[cfgval].value.intval = 1;
  cfgval++;
#ifndef HAVE_ERROR_CHECKING
#ifdef OUT_OF_ORDER_HANDLING
  /* disable parameter checking in PAMI - fast library only */
  config[cfgval].name = PAMI_CONTEXT_CHECK_PARAM;
  config[cfgval].value.intval = 0;
  cfgval++;
#endif
#endif
  rc = PAMI_Context_createv(MPIDI_Client, config, cfgval, MPIDI_Context, MPIDI_Process.avail_contexts);

  MPID_assert_always(rc == PAMI_SUCCESS);

  /* --------------------------------------------- */
  /* Get collective selection advisor and cache it */
  /* --------------------------------------------- */
  /* Context is created, i.e. collective selection extension is initialized in PAMI. Now I can get the
     advisor if I am not in TUNE mode. If in TUNE mode, I can init collsel and generate the table.
     This is not supported on BGQ.
  */
#ifndef __BGQ_
  MPIDI_Init_collsel_extension();
#endif

#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
  MPIDI_open_pe_extension();
#endif
}


static void
MPIDI_PAMI_dispath_set(size_t              dispatch,
                       struct protocol_t * proto,
                       unsigned          * immediate_max)
{
  size_t im_max = 0;
  pami_dispatch_callback_function Recv = {.p2p = proto->func};
  MPID_assert_always(dispatch == proto->dispatch);

  if (MPIDI_Process.shmem_pt2pt == 0)
    proto->options.use_shmem = PAMI_HINT_DISABLE;

  PAMIX_Dispatch_set(MPIDI_Context,
                     MPIDI_Process.avail_contexts,
                     proto->dispatch,
                     Recv,
                     proto->options,
                     &im_max);
  TRACE_ERR("Immediate-max query:  dispatch=%zu  got=%zu  required=%zu\n",
            dispatch, im_max, proto->immediate_min);
  MPID_assert_always(proto->immediate_min <= im_max);
  if ((immediate_max != NULL) && (im_max < *immediate_max))
    *immediate_max = im_max;
}


static void
MPIDI_PAMI_dispath_init()
{
#ifdef OUT_OF_ORDER_HANDLING
  {
    pami_configuration_t config;
    pami_result_t        rc = PAMI_ERROR;

    memset(&config, 0, sizeof(config));
    config.name = PAMI_DISPATCH_SEND_IMMEDIATE_MAX;
    rc = PAMI_Dispatch_query(MPIDI_Context[0], (size_t)0, &config, 1);
    if ( rc == PAMI_SUCCESS )
      {
        TRACE_ERR("PAMI_DISPATCH_SEND_IMMEDIATE_MAX=%d.\n", config.value.intval, rc);
        MPIDI_Process.pt2pt.limits_array[2] = config.value.intval;
      }
    else
      {
        TRACE_ERR(" Attention: PAMI_Client_query(DISPATCH_SEND_IMMEDIATE_MAX=%d) rc=%d\n", config.name, rc);
        MPIDI_Process.pt2pt.limits_array[2] = 256;
      }

    MPIDI_Process.pt2pt.limits_array[3] = MPIDI_Process.pt2pt.limits_array[2];
    MPIDI_Process.pt2pt.limits_array[6] = MPIDI_Process.pt2pt.limits_array[2];
    MPIDI_Process.pt2pt.limits_array[7] = MPIDI_Process.pt2pt.limits_array[2];
  }
#endif
  /* ------------------------------------ */
  /*  Set up the communication protocols  */
  /* ------------------------------------ */
  unsigned send_immediate_max_bytes = (unsigned) -1;
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_Short,     &proto_list.Short,     &send_immediate_max_bytes);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_ShortSync, &proto_list.ShortSync, &send_immediate_max_bytes);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_Eager,     &proto_list.Eager,     NULL);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_RVZ,       &proto_list.RVZ,       NULL);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_Cancel,    &proto_list.Cancel,    NULL);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_Control,   &proto_list.Control,   NULL);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_WinCtrl,   &proto_list.WinCtrl,   NULL);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_WinAccum,  &proto_list.WinAccum,  NULL);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_RVZ_zerobyte, &proto_list.RVZ_zerobyte, NULL);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_WinGetAccum, &proto_list.WinGetAccum, NULL);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_WinGetAccumAck, &proto_list.WinGetAccumAck, NULL);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_WinAtomic, &proto_list.WinAtomic,   NULL);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_WinAtomicAck, &proto_list.WinAtomicAck,   NULL);

#ifdef DYNAMIC_TASKING
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_Dyntask,   &proto_list.Dyntask,  NULL);
  MPIDI_PAMI_dispath_set(MPIDI_Protocols_Dyntask_disconnect,   &proto_list.Dyntask_disconnect,  NULL);
#endif

  /*
   * The first two protocols are our short protocols: they use
   * PAMI_Send_immediate() exclusively.  We get the short limit twice
   * because they could be different.
   *
   * - The returned value is the max amount of header+data.  We have
   *     to remove the header size.
   *
   * - We need to add one back, since we don't use "=" in the
   *     comparison.  We use "if (size < short_limit) ...".
   *
   * - We use the min of the results just to be safe.
   */
  send_immediate_max_bytes -= (sizeof(MPIDI_MsgInfo) - 1);

  if (MPIDI_Process.pt2pt.limits.application.immediate.remote > send_immediate_max_bytes)
    MPIDI_Process.pt2pt.limits.application.immediate.remote = send_immediate_max_bytes;

  if (MPIDI_Process.pt2pt.limits.application.immediate.local > send_immediate_max_bytes)
    MPIDI_Process.pt2pt.limits.application.immediate.local = send_immediate_max_bytes;

  if (MPIDI_Process.pt2pt.limits.internal.immediate.remote > send_immediate_max_bytes)
    MPIDI_Process.pt2pt.limits.internal.immediate.remote = send_immediate_max_bytes;

  if (MPIDI_Process.pt2pt.limits.internal.immediate.local > send_immediate_max_bytes)
    MPIDI_Process.pt2pt.limits.internal.immediate.local = send_immediate_max_bytes;

  if (TOKEN_FLOW_CONTROL_ON)
     {
       #if TOKEN_FLOW_CONTROL
        int i;
        MPIDI_mm_init(MPIDI_Process.numTasks,&MPIDI_Process.pt2pt.limits.application.eager.remote,&MPIDI_Process.mp_buf_mem);
        MPIDI_Token_cntr = MPIU_Calloc0(MPIDI_Process.numTasks, MPIDI_Token_cntr_t);
        memset((void *) MPIDI_Token_cntr,0, (sizeof(MPIDI_Token_cntr_t) * MPIDI_Process.numTasks));
        for (i=0; i < MPIDI_Process.numTasks; i++)
        {
          MPIDI_Token_cntr[i].tokens=MPIDI_tfctrl_enabled;
        }
        #else
         MPID_assert_always(0);
        #endif
     }
}



extern char **environ;
static void
printEnvVars(char *type)
{
   printf("The following %s* environment variables were specified:\n", type);
   char **env;
   for(env = environ; *env != 0 ; env++)
   {
      if(!strncasecmp(*env, type, strlen(type)))
        printf("  %s\n", *env);
   }
}


static void
MPIDI_PAMI_init(int* rank, int* size, int* threading)
{
  MPIDI_PAMI_context_init(threading, size);


  MPIDI_PAMI_dispath_init();


  if ( (*rank == 0) && (MPIDI_Process.verbose >= MPIDI_VERBOSE_SUMMARY_0) )
    {
      printf("MPIDI_Process.*\n"
             "  verbose               : %u\n"
             "  statistics            : %u\n"
             "  contexts              : %u\n"
             "  async_progress        : %u\n"
             "  context_post          : %u\n"
             "  pt2pt.limits\n"
             "    application\n"
             "      eager\n"
             "        remote, local   : %u, %u\n"
             "      short\n"
             "        remote, local   : %u, %u\n"
             "    internal\n"
             "      eager\n"
             "        remote, local   : %u, %u\n"
             "      short\n"
             "        remote, local   : %u, %u\n"
             "  rma_pending           : %u\n"
             "  shmem_pt2pt           : %u\n"
             "  disable_internal_eager_scale : %u\n"
#if TOKEN_FLOW_CONTROL
             "  mp_buf_mem               : %u\n"
             "  mp_buf_mem_max           : %u\n"
             "  is_token_flow_control_on : %u\n"
#endif
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
             "  mp_infolevel : %u\n"
             "  mp_statistics: %u\n"
             "  mp_printenv  : %u\n"
             "  mp_interrupts: %u\n"
#endif
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
             "  queue_binary_search_support_on : %u\n"
#endif
             "  optimized.collectives : %u\n"
             "  optimized.select_colls: %u\n"
             "  optimized.subcomms    : %u\n"
             "  optimized.memory      : %u\n"
             "  optimized.num_requests: %u\n"
             "  mpir_nbc              : %u\n" 
             "  numTasks              : %u\n",
             "  typed_onesided        : %u\n",
             MPIDI_Process.verbose,
             MPIDI_Process.statistics,
             MPIDI_Process.avail_contexts,
             MPIDI_Process.async_progress.mode,
             MPIDI_Process.perobj.context_post.requested,
             MPIDI_Process.pt2pt.limits_array[0],
             MPIDI_Process.pt2pt.limits_array[1],
             MPIDI_Process.pt2pt.limits_array[2],
             MPIDI_Process.pt2pt.limits_array[3],
             MPIDI_Process.pt2pt.limits_array[4],
             MPIDI_Process.pt2pt.limits_array[5],
             MPIDI_Process.pt2pt.limits_array[6],
             MPIDI_Process.pt2pt.limits_array[7],
             MPIDI_Process.rma_pending,
             MPIDI_Process.shmem_pt2pt,
             MPIDI_Process.disable_internal_eager_scale,
#if TOKEN_FLOW_CONTROL
             MPIDI_Process.mp_buf_mem,
             MPIDI_Process.mp_buf_mem_max,
             MPIDI_Process.is_token_flow_control_on,
#endif
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
             MPIDI_Process.mp_infolevel,
             MPIDI_Process.mp_statistics,
             MPIDI_Process.mp_printenv,
             (MPIDI_Process.async_progress.mode != ASYNC_PROGRESS_MODE_DISABLED),
#endif
#ifdef QUEUE_BINARY_SEARCH_SUPPORT
             MPIDI_Process.queue_binary_search_support_on,
#endif
             MPIDI_Process.optimized.collectives,
             MPIDI_Process.optimized.select_colls,
             MPIDI_Process.optimized.subcomms,
             MPIDI_Process.optimized.memory,
             MPIDI_Process.optimized.num_requests,
             MPIDI_Process.mpir_nbc, 
             MPIDI_Process.numTasks,
             MPIDI_Process.typed_onesided);
      switch (*threading)
        {
          case MPI_THREAD_MULTIPLE:
            printf("mpi thread level        : 'MPI_THREAD_MULTIPLE'\n");
            break;
          case MPI_THREAD_SERIALIZED:
            printf("mpi thread level        : 'MPI_THREAD_SERIALIZED'\n");
            break;
          case MPI_THREAD_FUNNELED:
            printf("mpi thread level        : 'MPI_THREAD_FUNNELED'\n");
            break;
          case MPI_THREAD_SINGLE:
            printf("mpi thread level        : 'MPI_THREAD_SINGLE'\n");
            break;
        }
      printf("MPICH_THREAD_GRANULARITY : '%s'\n",
             (MPICH_THREAD_GRANULARITY==MPICH_THREAD_GRANULARITY_PER_OBJECT)?"per object":"global");
#ifdef ASSERT_LEVEL
      printf("ASSERT_LEVEL            : %d\n", ASSERT_LEVEL);
#else
      printf("ASSERT_LEVEL            : not defined\n");
#endif
#ifdef MPICH_LIBDIR
      printf("MPICH_LIBDIR           : %s\n", MPICH_LIBDIR);
#else
      printf("MPICH_LIBDIR           : not defined\n");
#endif
      printEnvVars("MPICH_");
      printEnvVars("PAMID_");
      printEnvVars("PAMI_");
      printEnvVars("COMMAGENT_");
      printEnvVars("MUSPI_");
      printEnvVars("BG_");
    }
#ifdef MPIDI_BANNER
  if ((*rank == 0) && (MPIDI_Process.mp_infolevel >=1)) {
       char* buf = (char *) MPIU_Malloc(160);
       int   rc  = MPIDI_Banner(buf);
       if ( rc == 0 )
            fprintf(stderr, "%s\n", buf);
       else
            TRACE_ERR("mpid_banner() return code=%d task %d",rc,*rank);
       MPIU_Free(buf);
  }
#endif
}

#ifndef DYNAMIC_TASKING
static void
MPIDI_VCRT_init(int rank, int size)
#else
static void
MPIDI_VCRT_init(int rank, int size, char *world_tasks, MPIDI_PG_t *pg)
#endif
{
  int i, rc;
  MPID_Comm * comm;
#ifdef DYNAMIC_TASKING
  int p, mpi_errno=0;
  char *world_tasks_save,*cp;
  char *pg_id;
#endif

  /* ------------------------------- */
  /* Initialize MPI_COMM_SELF object */
  /* ------------------------------- */
  comm = MPIR_Process.comm_self;
  comm->rank = 0;
  comm->remote_size = comm->local_size = 1;
  rc = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
  MPID_assert_always(rc == MPI_SUCCESS);
  rc = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
  MPID_assert_always(rc == MPI_SUCCESS);
  comm->vcr[0]->taskid= PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_TASK_ID  ).value.intval;

#ifdef DYNAMIC_TASKING
  if(mpidi_dynamic_tasking) {
    comm->vcr[0]->pg=pg->vct[rank].pg;
    comm->vcr[0]->pg_rank=pg->vct[rank].pg_rank;
    pg->vct[rank].taskid = comm->vcr[0]->taskid;
    if(comm->vcr[0]->pg) {
      TRACE_ERR("Adding ref for comm=%x vcr=%x pg=%x\n", comm, comm->vcr[0], comm->vcr[0]->pg);
      MPIDI_PG_add_ref(comm->vcr[0]->pg);
    }
    comm->local_vcr = comm->vcr;
  }
 
#endif

  /* -------------------------------- */
  /* Initialize MPI_COMM_WORLD object */
  /* -------------------------------- */
  comm = MPIR_Process.comm_world;
  comm->rank = rank;
  comm->remote_size = comm->local_size = size;
  rc = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
  MPID_assert_always(rc == MPI_SUCCESS);
  rc = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
  MPID_assert_always(rc == MPI_SUCCESS);

#ifdef DYNAMIC_TASKING
  if(mpidi_dynamic_tasking) {
    i=0;
    world_tasks_save = MPIU_Strdup(world_tasks);
    if(world_tasks != NULL) {
      comm->vcr[0]->taskid = atoi(strtok(world_tasks, ":"));
      while( (cp=strtok(NULL, ":")) != NULL) {
        comm->vcr[++i]->taskid= atoi(cp);
      }
    }
    MPIU_Free(world_tasks_save);

        /* This memory will be freed by the PG_Destroy if there is an error */
        pg_id = MPIU_Malloc(MAX_JOBID_LEN);

        mpi_errno = PMI2_Job_GetId(pg_id, MAX_JOBID_LEN);
        TRACE_ERR("PMI2_Job_GetId - pg_id=%s\n", pg_id);

    /* Initialize the connection table on COMM_WORLD from the process group's
       connection table */
    for (p = 0; p < comm->local_size; p++)
    {
	  comm->vcr[p]->pg=pg->vct[p].pg;
          comm->vcr[p]->pg_rank=pg->vct[p].pg_rank;
          pg->vct[p].taskid = comm->vcr[p]->taskid;
	  if(comm->vcr[p]->pg) {
            TRACE_ERR("Adding ref for comm=%x vcr=%x pg=%x\n", comm, comm->vcr[p], comm->vcr[p]->pg);
            MPIDI_PG_add_ref(comm->vcr[p]->pg);
	  }
       /* MPID_VCR_Dup(&pg->vct[p], &(comm->vcr[p]));*/
	  TRACE_ERR("comm->vcr[%d]->pg->id=%s comm->vcr[%d]->pg_rank=%d\n", p, comm->vcr[p]->pg->id, p, comm->vcr[p]->pg_rank);
	  TRACE_ERR("TASKID -- comm->vcr[%d]=%d\n", p, comm->vcr[p]->taskid);
    }

   comm->local_vcr = comm->vcr;
  }else {
	for (i=0; i<size; i++) {
	  comm->vcr[i]->taskid = i;
	  TRACE_ERR("comm->vcr[%d]=%d\n", i, comm->vcr[i]->taskid);
        }
	TRACE_ERR("MP_I_WORLD_TASKS not SET\n");
  }
#else
  for (i=0; i<size; i++) {
    comm->vcr[i]->taskid = i;
    TRACE_ERR("comm->vcr[%d]=%d\n", i, comm->vcr[i]->taskid);
  }
#endif
}


/**
 * \brief Initialize MPICH at ADI level.
 * \param[in,out] argc Unused
 * \param[in,out] argv Unused
 * \param[in]     requested The thread model requested by the user.
 * \param[out]    provided  The thread model provided to user.  It is the same as requested, except in VNM.
 * \param[out]    has_args  Set to TRUE
 * \param[out]    has_env   Set to TRUE
 * \return MPI_SUCCESS
 */
int MPID_Init(int * argc,
              char *** argv,
              int   requested,
              int * provided,
              int * has_args,
              int * has_env)
{
  int rank, size;
#ifdef DYNAMIC_TASKING
  int has_parent=0;
  MPIDI_PG_t * pg=NULL;
  int pg_rank=-1;
  int pg_size;
  int appnum,mpi_errno;
  MPID_Comm * comm;
  int i,j;
  pami_configuration_t config;
  int world_size;
#endif
  char *world_tasks;
  pami_result_t rc;

  /* Override split_type */
  MPID_Comm_fns = &comm_fns;

  /* ------------------------------------------------------------------------------- */
  /*  Initialize the pami client to get the process rank; needed for env var output. */
  /* ------------------------------------------------------------------------------- */
  MPIDI_PAMI_client_init(&rank, &size, &mpidi_dynamic_tasking, &world_tasks);
  TRACE_OUT("after MPIDI_PAMI_client_init rank=%d size=%d mpidi_dynamic_tasking=%d\n", rank, size, mpidi_dynamic_tasking);

  /* ------------------------------------ */
  /*  Get new defaults from the Env Vars  */
  /* ------------------------------------ */
  MPIDI_Env_setup(rank, requested);

  /* ----------------------------- */
  /* Initialize messager           */
  /* ----------------------------- */
  if ( (MPIDI_Process.async_progress.mode == ASYNC_PROGRESS_MODE_TRIGGER) || mpidi_dynamic_tasking)
  {
    /* The 'trigger' async progress mode requires MPI_THREAD_MULTIPLE.
     * Silently promote the thread level.
     *
     * See discussion in src/mpid/pamid/src/mpid_progress.h for more
     * information.
     */
    *provided = MPI_THREAD_MULTIPLE;
  }
  else
  {
    *provided = requested;
  }
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
   if (requested != MPI_THREAD_MULTIPLE)
       mpich_env->single_thread=1;
#endif
  MPIDI_PAMI_init(&rank, &size, provided);

#ifdef DYNAMIC_TASKING
  if (mpidi_dynamic_tasking) {

    /*
     * Perform PMI initialization
     */
    mpi_errno = MPIDI_InitPG( argc, argv,
			      has_args, has_env, &has_parent, &pg_rank, &pg );
    if (mpi_errno) {
	TRACE_ERR("MPIDI_InitPG returned with mpi_errno=%d\n", mpi_errno);
    }

    /* FIXME: Why are pg_size and pg_rank handled differently? */
    pg_size = MPIDI_PG_Get_size(pg);

    TRACE_ERR("MPID_Init - pg_size=%d\n", pg_size);
    MPIDI_Process.my_pg = pg;  /* brad : this is rework for shared memories
				* because they need this set earlier
                                * for getting the business card
                                */
    MPIDI_Process.my_pg_rank = pg_rank;

  }
#endif

  /* ------------------------- */
  /* initialize request queues */
  /* ------------------------- */
  MPIDI_Recvq_init();

  /* -------------------------------------- */
  /* Fill in some hardware structure fields */
  /* -------------------------------------- */
  extern void MPIX_Init();
  MPIX_Init();

  /* ------------------------------- */
  /* Set process attributes          */
  /* ------------------------------- */
  MPIR_Process.attrs.tag_ub = INT_MAX;
  MPIR_Process.attrs.wtime_is_global = 1;
  MPIR_Process.attrs.io   = MPI_ANY_SOURCE;


  /* ------------------------------- */
  /* Initialize communicator objects */
  /* ------------------------------- */
#ifndef DYNAMIC_TASKING
  MPIDI_VCRT_init(rank, size);
#else
  MPIDI_VCRT_init(rank, size, world_tasks, pg);
#endif

  /* ------------------------------- */
  /* Setup optimized communicators   */
  /* ------------------------------- */
  TRACE_ERR("creating world geometry\n");
  rc = PAMI_Geometry_world(MPIDI_Client, &MPIDI_Process.world_geometry);
  MPID_assert_always(rc == PAMI_SUCCESS);
  TRACE_ERR("calling comm_create on comm world %p\n", MPIR_Process.comm_world);
  MPIR_Process.comm_world->mpid.geometry = MPIDI_Process.world_geometry;
  MPIR_Process.comm_world->mpid.parent   = PAMI_GEOMETRY_NULL;
  MPIR_Comm_commit(MPIR_Process.comm_world);

#ifdef DYNAMIC_TASKING
  if (has_parent) {
     char * parent_port;

     /* FIXME: To allow just the "root" process to
        request the port and then use MPIR_Bcast_intra to
        distribute it to the rest of the processes,
        we need to perform the Bcast after MPI is
        otherwise initialized.  We could do this
        by adding another MPID call that the MPI_Init(_thread)
        routine would make after the rest of MPI is
        initialized, but before MPI_Init returns.
        In fact, such a routine could be used to
        perform various checks, including parameter
        consistency value (e.g., all processes have the
        same environment variable values). Alternately,
        we could allow a few routines to operate with
        predefined parameter choices (e.g., bcast, allreduce)
        for the purposes of initialization. */
	mpi_errno = MPIDI_GetParentPort(&parent_port);
	if (mpi_errno != MPI_SUCCESS) {
          TRACE_ERR("MPIDI_GetParentPort returned with mpi_errno=%d\n", mpi_errno);
	}

	mpi_errno = MPID_Comm_connect(parent_port, NULL, 0,
				      MPIR_Process.comm_world, &comm);
	if (mpi_errno != MPI_SUCCESS) {
	    TRACE_ERR("mpi_errno from Comm_connect=%d\n", mpi_errno);
	}

	MPIR_Process.comm_parent = comm;
	MPIU_Assert(MPIR_Process.comm_parent != NULL);
	MPIU_Strncpy(comm->name, "MPI_COMM_PARENT", MPI_MAX_OBJECT_NAME);

	/* FIXME: Check that this intercommunicator gets freed in MPI_Finalize
	   if not already freed.  */
   }
  mp_world_exiting_handler = &(_mpi_world_exiting_handler);
#endif
  /* ------------------------------- */
  /* Initialize timer data           */
  /* ------------------------------- */
  MPID_Wtime_init();


  /* ------------------------------- */
  /* ???                             */
  /* ------------------------------- */
  *has_args = TRUE;
  *has_env  = TRUE;
#ifdef MPIDI_PRINTENV
  if (MPIDI_Process.mp_printenv) {
      MPIDI_Print_mpenv(rank,size);
  }
#endif
  /* ----------------------------------------------- */
  /* parse params for pami_tune if in benchmark mode */
  /* ----------------------------------------------- */
  if(MPIDI_Process.optimized.auto_select_colls == MPID_AUTO_SELECT_COLLS_TUNE)
  {
    if(argc != NULL && argv != NULL)
    {
      if(MPIDI_collsel_pami_tune_parse_params(*argc, *argv) != PAMI_SUCCESS)
      {
        MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_NONE;
      }
    }
    else
    {
      if(MPIDI_collsel_pami_tune_parse_params(0, NULL) != PAMI_SUCCESS)
      {
        MPIDI_Process.optimized.auto_select_colls = MPID_AUTO_SELECT_COLLS_NONE;
      }
    }
  }
  return MPI_SUCCESS;
}


/*
 * \brief This is called by MPI to let us know that MPI_Init is done.
 */
int MPID_InitCompleted()
{
  MPIDI_NBC_init();
  MPIDI_Progress_init();
  /* ----------------------------------------------- */
  /*    Now all is ready.. call table generate       */
  /* ----------------------------------------------- */
  if(MPIDI_Process.optimized.auto_select_colls == MPID_AUTO_SELECT_COLLS_TUNE)
  {
    MPIDI_Collsel_table_generate();
    MPIDI_collsel_pami_tune_cleanup();
  }
  return MPI_SUCCESS;
}

#if (MPIDI_PRINTENV || MPIDI_STATISTICS || MPIDI_BANNER)
void MPIDI_open_pe_extension() {
    int rc;
     /* open PE extension       */
     memset(&pe_extension,0, sizeof(pami_extension_t));
     rc = PAMI_Extension_open (MPIDI_Client, "EXT_pe_extension", &pe_extension);
     TRACE_ERR("PAMI_Extension_open: rc %d\n", rc);
     if (rc != PAMI_SUCCESS) {
         TRACE_ERR("ERROR open PAMI_Extension_open failed rc %d", rc);
         MPID_assert_always(rc == PAMI_SUCCESS);
     }
}


int MPIDI_Banner(char * bufPtr) {
    char  *cp, *level=NULL;
    char buf[30];
    char *ASC_time;
    time_t  ltime;
    char msgBuf[60];
    char type[64], ver_buf[64];
    struct  tm  *tmx,*tm1;

    /* Note: The _ibm_release_version will be expanded to a full string  */
    /*       ONLY IF this file is extracted from CMVC.                   */
    /*       If this file is cloned from GIT the the string will be just */
    /*       "%W%.                                                       */
    if ( strncmp(_ibm_release_version_, "%W", 2) ) {
       /* IBMPE's expanded version string has release name like ppe_rbarlx */
       /* and that '_' in the name is what we are looking for.             */
       /* BGQ's version string does not have a '_' in it.                  */
       level = strrchr(_ibm_release_version_, '_');
       if ( level ) {
          level -=3;

          /* The version string generated by CMVC during weekly build has a */
          /* date which is the time when the mpidi_platform.h file is last  */
          /* updated.  This date can be quite old and is better removed.    */
          memset(ver_buf, 0, sizeof(ver_buf));
          strncpy(ver_buf, level, sizeof(ver_buf)-1);
          if ( cp = strchr(ver_buf, ',') ) *cp = '\0';
       }
    }

    if(sizeof(void*) == 8)
      strcpy(type, "64bit (MPI over PAMI)");
    else if(sizeof(int) == 4)
      strcpy(type, "32bit (MPI over PAMI)");
    else
      strcpy(type, "UNKNOWN-bit (MPI over PAMI)");

    sprintf(msgBuf,"MPICH library was compiled on");

    tmx=MPIU_Malloc(sizeof(struct tm));
    sprintf(buf,__DATE__" "__TIME__);

    if ((void *) NULL == strptime(buf, "%B %d %Y %T", tmx))
       return(1);

   /*  update isdst in tmx structure    */
    ltime=0;
    time(&ltime);
    tm1 = localtime(&ltime);
    tmx->tm_isdst=tm1->tm_isdst;

   /* localtime updates tm_wday in tmx structure  */
    ltime=mktime(tmx);
    tm1 = localtime(&ltime);
    tmx->tm_wday = tm1->tm_wday;
    ASC_time = asctime(tmx);

    if (level) {
       sprintf(bufPtr, "%s %s %s %s ", type, ver_buf, msgBuf, ASC_time);
    } else {
       sprintf(bufPtr, "%s %s %s ", type, msgBuf, ASC_time);

    }

    MPIU_Free(tmx);
    return MPI_SUCCESS;
}
#endif


static inline void
static_assertions()
{
  MPID_assert_static(sizeof(void*) == sizeof(size_t));
  MPID_assert_static(sizeof(uintptr_t) == sizeof(size_t));
#ifdef __BGQ__
  /* MPID_VCR_GET_LPIDS relies on the VCR being a simple task list */
  MPID_VCR vcr=NULL;
  MPID_assert_static(sizeof(*vcr) == sizeof(pami_task_t));/* VCR is a simple task list */
  MPID_assert_static(sizeof(vcr->taskid) == sizeof(*vcr));/* VCR is a simple task list */

  MPID_assert_static(sizeof(MPIDI_MsgInfo) == 16);
  MPID_assert_static(sizeof(uint64_t) == sizeof(size_t));
#endif
}

#ifdef DYNAMIC_TASKING
/* FIXME: The PG code should supply these, since it knows how the
   pg_ids and other data are represented */
int MPIDI_PG_Compare_ids(void * id1, void * id2)
{
    return (strcmp((char *) id1, (char *) id2) == 0) ? TRUE : FALSE;
}

int MPIDI_PG_Destroy_id(MPIDI_PG_t * pg)
{
    if (pg->id != NULL)
    {
	TRACE_ERR("free pg id =%p pg=%p\n", pg->id, pg);
	MPIU_Free(pg->id);
	TRACE_ERR("done free pg id \n");
    }

    return MPI_SUCCESS;
}


int MPIDI_InitPG( int *argc, char ***argv,
	          int *has_args, int *has_env, int *has_parent,
	          int *pg_rank_p, MPIDI_PG_t **pg_p )
{
    int pmi_errno;
    int mpi_errno = MPI_SUCCESS;
    int pg_rank, pg_size, appnum, pg_id_sz;
    int usePMI=1;
    char *pg_id;
    MPIDI_PG_t *pg = 0;

    /* If we use PMI here, make the PMI calls to get the
       basic values.  Note that systems that return setvals == true
       do not make use of PMI for the KVS routines either (it is
       assumed that the discover connection information through some
       other mechanism */
    /* FIXME: We may want to allow the channel to ifdef out the use
       of PMI calls, or ask the channel to provide stubs that
       return errors if the routines are in fact used */
    if (usePMI) {
	/*
	 * Initialize the process manangement interface (PMI),
	 * and get rank and size information about our process group
	 */

#ifdef USE_PMI2_API
	TRACE_ERR("Calling PMI2_Init\n");
        mpi_errno = PMI2_Init(has_parent, &pg_size, &pg_rank, &appnum);
	TRACE_ERR("PMI2_Init - pg_size=%d pg_rank=%d\n", pg_size, pg_rank);
        /*if (mpi_errno) MPIU_ERR_POP(mpi_errno);*/
#else
	TRACE_ERR("Calling PMI_Init\n");
	pmi_errno = PMI_Init(has_parent);
	if (pmi_errno != PMI_SUCCESS) {
	/*    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_init",
			     "**pmi_init %d", pmi_errno); */
	}

	pmi_errno = PMI_Get_rank(&pg_rank);
	if (pmi_errno != PMI_SUCCESS) {
	    /*MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_rank",
			     "**pmi_get_rank %d", pmi_errno); */
	}

	pmi_errno = PMI_Get_size(&pg_size);
	if (pmi_errno != 0) {
	/*MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_size",
			     "**pmi_get_size %d", pmi_errno);*/
	}

	pmi_errno = PMI_Get_appnum(&appnum);
	if (pmi_errno != PMI_SUCCESS) {
/*	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_appnum",
				 "**pmi_get_appnum %d", pmi_errno); */
	}
#endif
	/* Note that if pmi is not availble, the value of MPI_APPNUM is
	   not set */
	if (appnum != -1) {
	    MPIR_Process.attrs.appnum = appnum;
	}

#ifdef USE_PMI2_API

        /* This memory will be freed by the PG_Destroy if there is an error */
	pg_id = MPIU_Malloc(MAX_JOBID_LEN);

        mpi_errno = PMI2_Job_GetId(pg_id, MAX_JOBID_LEN);
	TRACE_ERR("PMI2_Job_GetId - pg_id=%s\n", pg_id);
#else
	/* Now, initialize the process group information with PMI calls */
	/*
	 * Get the process group id
	 */
	pmi_errno = PMI_KVS_Get_name_length_max(&pg_id_sz);
	if (pmi_errno != PMI_SUCCESS) {
          TRACE_ERR("PMI_KVS_Get_name_length_max returned with pmi_errno=%d\n", pmi_errno);
	}

	/* This memory will be freed by the PG_Destroy if there is an error */
	pg_id = MPIU_Malloc(pg_id_sz + 1);

	/* Note in the singleton init case, the pg_id is a dummy.
	   We'll want to replace this value if we join an
	   Process manager */
	pmi_errno = PMI_KVS_Get_my_name(pg_id, pg_id_sz);
	if (pmi_errno != PMI_SUCCESS) {
          TRACE_ERR("PMI_KVS_Get_my_name returned with pmi_errno=%d\n", pmi_errno);
	}
#endif
    }
    else {
	/* Create a default pg id */
	pg_id = MPIU_Malloc(2);
	MPIU_Strncpy( pg_id, "0", 2 );
    }

	TRACE_ERR("pg_size=%d pg_id=%s\n", pg_size, pg_id);
    /*
     * Initialize the process group tracking subsystem
     */
    mpi_errno = MPIDI_PG_Init(argc, argv,
			     MPIDI_PG_Compare_ids, MPIDI_PG_Destroy_id);
    if (mpi_errno != MPI_SUCCESS) {
      TRACE_ERR("MPIDI_PG_Init returned with mpi_errno=%d\n", mpi_errno);
    }

    /*
     * Create a new structure to track the process group for our MPI_COMM_WORLD
     */
    TRACE_ERR("pg_size=%d pg_id=%p pg_id=%s\n", pg_size, pg_id, pg_id);
    mpi_errno = MPIDI_PG_Create(pg_size, pg_id, &pg);
    MPIU_Free(pg_id);
    if (mpi_errno != MPI_SUCCESS) {
      TRACE_ERR("MPIDI_PG_Create returned with mpi_errno=%d\n", mpi_errno);
    }

    /* FIXME: We can allow the channels to tell the PG how to get
       connection information by passing the pg to the channel init routine */
    if (usePMI) {
	/* Tell the process group how to get connection information */
        mpi_errno = MPIDI_PG_InitConnKVS( pg );
        if (mpi_errno)
          TRACE_ERR("MPIDI_PG_InitConnKVS returned with mpi_errno=%d\n", mpi_errno);
    }

    /* FIXME: has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env  = TRUE;

    *pg_p      = pg;
    *pg_rank_p = pg_rank;

 fn_exit:
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (pg) {
	MPIDI_PG_Destroy( pg );
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
#endif
