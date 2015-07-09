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
 * \file src/mpid_util.c
 * \brief Normal job startup code
 */


/*
 * \brief
 */
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <nl_types.h>
#include <mpidimpl.h>
#include "mpidi_util.h"

#define PAMI_TUNE_MAX_ITER 2000
/* Short hand for sizes */
#define ONE  (1)
#define ONEK (1<<10)
#define ONEM (1<<20)
#define ONEG (1<<30)

#define PAMI_ASYNC_EXT_ATTR 2000

#if CUDA_AWARE_SUPPORT
void * pamidCudaPtr = NULL;
#endif
#if (MPIDI_PRINTENV || MPIDI_STATISTICS || MPIDI_BANNER)
MPIDI_printenv_t  *mpich_env=NULL;
extern char* mp_euilib;
char mp_euidevice[20];
extern pami_extension_t pe_extension;
extern pamix_extension_info_t PAMIX_Extensions;
 typedef enum {
        /* Attribute       usage : type : default : description   */
         PAMI_ASYNC_ATTR
           = PAMI_ASYNC_EXT_ATTR,
         PAMI_CONTEXT_TIMER_INTERVAL,   /**<   U: size_t : N/A : current timer interval in PAMI context */
       } pamix_attribute_name_async_t;


void MPIDI_Set_mpich_env(int rank, int size) {
     static char polling_buf[32]="";
     int rc;
     pami_configuration_t config;

     mpich_env->this_task = rank;
     mpich_env->nprocs  = size;
     mpich_env->eager_limit=MPIDI_Process.pt2pt.limits.application.eager.remote;
     mpich_env->use_token_flow_control=MPIDI_Process.is_token_flow_control_on;
     mpich_env->mp_statistics=MPIDI_Process.mp_statistics;
     if (mpich_env->polling_interval == 0) {
         memset(&config, 0, sizeof(config));
         config.name = (pami_attribute_name_t)PAMI_CONTEXT_TIMER_INTERVAL;
         rc= PAMI_Context_query(MPIDI_Context[0], &config, 1);
            mpich_env->polling_interval = config.value.intval;;
            sprintf(polling_buf, "MP_POLLING_INTERVAL=%d",
                    mpich_env->polling_interval); /* microseconds */
            rc = putenv(polling_buf);
     }
     if (mpich_env->retransmit_interval == 0) {
         memset(&config, 0, sizeof(config));
         config.name = (pami_attribute_name_ext_t)PAMI_CONTEXT_RETRANSMIT_INTERVAL;
         rc= PAMI_Context_query(MPIDI_Context[0], &config, 1);
            mpich_env->retransmit_interval = config.value.intval;
            sprintf(polling_buf, "MP_RETRANSMIT_INTERVAL=%d",
                    mpich_env->retransmit_interval); /* microseconds */
            rc = putenv(polling_buf);
     }
     mpich_env->buffer_mem=MPIDI_Process.mp_buf_mem;
     mpich_env->buffer_mem_max=MPIDI_Process.mp_buf_mem_max;
}



void MPIDI_Setup_networkenv()
{
      char *network;
      char delimiter;
      int rc;
      pami_configuration_t config;


      delimiter=':';
      mpich_env->instances = 1;
      mpich_env->strip_type = NO_STRIPING;
      mpich_env->transport_type = IS_IP;
      mpich_env->network_string = NULL;

/*
 *  Always try the MPI environment variables first
 *   then the PAMI env variable
 *   then query the PAMI network environment string
 */
      if ((network = getenv("MP_CHILD_INET_ADDR")) != NULL ) {
           mpich_env->transport_type = IS_IP;
      } else if ((network = getenv("MP_MPI_NETWORK"))  != NULL) {
           mpich_env->transport_type = IS_US;
      } else {
         network=NULL;
         memset(&config,0, sizeof(config));
         config.name = (pami_attribute_name_t)PAMI_CONTEXT_NETWORK;
         rc= PAMI_Context_query(MPIDI_Context[0], &config, 1);
         network=(char *) config.value.chararray;
         if ((rc != PAMI_SUCCESS) || (network== NULL)) {
             TRACE_ERR("PAMI network query returns %d", rc);
             network=NULL;
             return;
         } else {
            if ( strchr(network, ':') ||
                 strchr(network, '.')) {
                 mpich_env->transport_type = IS_IP;
            } else {
                 mpich_env->transport_type = IS_US;
            }
         }
      }

      if ( NULL != network )  {
           mpich_env->network_string = (char *) MPIU_Malloc(strlen(network)+1);
           if( NULL !=   mpich_env->network_string )  {
              memcpy(mpich_env->network_string,network,strlen(network)+1);
           }
           mpich_env->instances = atoi(network+1);
           if (mpich_env->instances > 1 ) {
               mpich_env->strip_type = IS_PACKET_STRIPING;
           }
      }
      /*
       *  These should match the PAMI defaults. They are provided so that the
       *  mpi_printenv variable can print them out.
       */
      if ( mpich_env->retransmit_interval == 0 ) {
          memset(&config, 0, sizeof(config));
          config.name = (pami_attribute_name_t)PAMI_CONTEXT_RETRANSMIT_INTERVAL;
          rc= PAMI_Context_query(MPIDI_Context[0], &config, 1);
             mpich_env->retransmit_interval = config.value.intval;
      }

}


int  MPIDI_Update_mpenv()
{
    char *buf;
    int  rc=0;
    int i, val;         /* for loop index */
    long bdfrsize;
    int SendBufSize;
    int CopySendBufSize;
    char *cp,*p;
    int  infolevel, max_pkt_size;
    uint tmp_val;
    size_t  bulkXfer=0;
    pami_configuration_t     *pami_config,config;

      MPIDI_Setup_networkenv();
      memset(&config,0, sizeof(config));
      config.name = PAMI_DISPATCH_SEND_IMMEDIATE_MAX;
      rc = PAMI_Dispatch_query(MPIDI_Context[0], (size_t)0, &config, 1);
      if ( rc == PAMI_SUCCESS ) {
           TRACE_ERR("PAMI_DISPATCH_SEND_IMMEDIATE_MAX=%d.\n", config.value.intval, rc);
           SendBufSize = config.value.intval;
      } else {
           TRACE_ERR((" Attention: PAMI_Client_query(DISPATCH_SEND_IMMEDIATE_MAX=%d) rc=%d\n",
            config.name, rc));
            SendBufSize = 256;         /* This is PAMI SendBufSize. */
      }
        CopySendBufSize = SendBufSize - sizeof(MPIDI_MsgInfo);
        mpich_env->copySendBufSize = CopySendBufSize;

        pami_config = (pami_configuration_t *) MPIU_Malloc(10 * sizeof(pami_configuration_t));
        memset((void *) pami_config,0, sizeof(pami_config));
        pami_config[0].name = (pami_attribute_name_t)PAMI_CONTEXT_MAX_PKT_SZ;
        pami_config[1].name = (pami_attribute_name_t)PAMI_CONTEXT_RFIFO_SZ;
        pami_config[2].name = (pami_attribute_name_t)PAMI_CONTEXT_SHM_ENABLED;
        pami_config[3].name = (pami_attribute_name_t)PAMI_CONTEXT_BULK_XFER;
        pami_config[4].name = (pami_attribute_name_t)PAMI_CONTEXT_BULK_MIN_MSG_SZ;
        rc= PAMI_Context_query(MPIDI_Context[0], pami_config, 5);
        if (rc != PAMI_SUCCESS) {
             printf("ERROR PAMI_Context_query() failed rc=%d\n",rc); fflush(stdout);
             return rc;
        }
        max_pkt_size = pami_config[0].value.intval;
        mpich_env->recv_fifo_sz = pami_config[1].value.intval;
        mpich_env->max_pkt_size = max_pkt_size;
        mpich_env->max_pkts_out = 31;
        mpich_env->use_shmem = pami_config[2].value.intval;
        bulkXfer=pami_config[3].value.intval;
        if (bulkXfer) {
           mpich_env->use_rdma=PAMI_HINT_ENABLE;
        } else {
           /* No bulk xfer in PAMI */
            if (mpich_env->this_task == 0) {
                TRACE_ERR("MPI-PAMI: Attention: No bulk xfer for the job\n");
            }
            mpich_env->use_rdma=PAMI_HINT_DISABLE;      /* in sync with PAMI */
        }
        /* obtain minimum message size for bulk xfer (rdma)  */
        mpich_env->rdma_min_msg_size = (long) pami_config[4].value.intval;
        MPIU_Free(pami_config);
}


/*  Print Environment                                                          */
/* _mpidi_printenv collects the relevant MPI and internal variables to task 0  */
/* and then prints out the differences                                         */
/* Returns: 0 - OK; possibly some ATTENTION messages issued;                   */
/*          1 - ERROR; user must call exit(1), or risk the application hanging */




#define MATCHI(x,y) {                                                             \
        first = 0; last = task_count-1; cflag = ' ';                              \
        for (i=1;i<=task_count;i++) {                                             \
                if ((i==task_count) || (gatherer[i].x != gatherer[first].x)) {    \
                        last = i-1;                                               \
                        if (last != task_count-1) cflag = '*';                    \
                        printf("Task %1c%4d-%4d:%s %d\n", cflag,first, last, y,   \
                        gatherer[first].x);                                       \
                        first = i;                                                \
                  }                                                               \
          }                                                                       \
        }
#define MATCHLL(x,y) {                                                            \
        first = 0; last = task_count-1; cflag = ' ';                              \
        for (i=1;i<=task_count;i++) {                                             \
                if ((i==task_count) || (gatherer[i].x != gatherer[first].x)) {    \
                        last = i-1;                                               \
                        if (last != task_count-1) cflag = '*';                    \
                        printf("Task %1c%4d-%4d:%s %lld\n", cflag,first, last,    \
                                 y, gatherer[first].x);                           \
                        first = i;                                                \
                        }                                                         \
                }                                                                 \
        }
#define MATCHD(x,y) {                                                             \
        first = 0; last = task_count-1; cflag = ' ';                              \
        for (i=1;i<=task_count;i++) {                                             \
             if ((i==task_count) || (gatherer[i].x != gatherer[first].x)) {       \
                  last = i-1;                                                     \
                  if (last != task_count-1) cflag = '*';                          \
                  printf("Task %1c%4d-%4d:%s %lf\n", cflag, first, last,          \
                          y, gatherer[first].x);                                  \
                     first = i;                                                   \
               }                                                                  \
             }                                                                    \
        }

#define MATCHC(x,y,m) {                                                              \
        first = 0; last = task_count-1; cflag = ' ';                                 \
        gatherer[first].x[m-1]='\0';                                                 \
        for (i=1;i<=task_count;i++) {                                                \
            if (i<task_count) gatherer[i].x[m-1]='\0';                               \
               if ((i==task_count) || strncmp(gatherer[i].x, gatherer[first].x, m)) {\
                   last = i-1;                                                       \
                   if (last != task_count-1) cflag = '*';                            \
                   printf("Task %1c%4d-%4d:%s %s\n", cflag, first, last, y,          \
                           gatherer[first].x);                                       \
                   first = i;                                                        \
                }                                                                    \
             }                                                                       \
        }
#define MATCHB(x,y) {                                                                \
        first = 0; last = task_count-1; cflag = ' ';                                 \
        for (i=1;i<=task_count;i++) {                                                \
             if ((i==task_count) || (gatherer[i].x != gatherer[first].x)) {          \
                  last = i-1;                                                        \
                  if (last != task_count-1) cflag = '*';                             \
                  if (gatherer[first].x > 0) {                                       \
                    printf("Task %1c%4d-%4d:%s %s\n", cflag, first, last,            \
                           y, "YES");                                                \
                  } else if (gatherer[first].x == 0) {                               \
                              printf("Task %1c%4d-%4d:%s %s\n", cflag, first,        \
                                      last, y, "NO");                                \
                  }  else {                                                          \
                              printf("Task %1c%4d-%4d:%s %d\n", cflag, first, last,  \
                                      y, gatherer[first].x);                         \
                   }                                                                 \
                        first = i;                                                   \
                }                                                                    \
           }                                                                         \
        }
/* A modification on MATCHB, it is used to check the environment variables
 * that take one of YES, NO or CONFIRM, This is used in particular for checking the value
 * passed to MP_SINGLE_THREAD */
#define MATCHT(x,y) {                                                                \
        first = 0; last = task_count-1; cflag = ' ';                                 \
        for (i=1;i<=task_count;i++) {                                                \
                if ((i==task_count) || (gatherer[i].x != gatherer[first].x)) {       \
                     last = i-1;                                                     \
                     if (last != task_count-1) cflag = '*';                          \
                     if (gatherer[first].x == 0) {                                   \
                            printf("Task %1c%4d-%4d:%s %s\n", cflag, first, last, y, \
                            "NO");                                                   \
                      } else if (gatherer[first].x == 1) {                           \
                            printf("Task %1c%4d-%4d:%s %s\n", cflag, first, last,    \
                                    y, "YES");                                       \
                      } else {                                                       \
                           printf("Task %1c%4d-%4d:%s %d\n", cflag, first, last, y,  \
                           gatherer[first].x);                                       \
	               }                                                             \
                       first = i;                                                    \
                    }                                                                \
                }                                                                    \
        }

#define NOTAPPLICABLE(y) {                                                           \
        printf("Task  %4d-%4d:%s NA\n", 0, task_count-1, y);                         \
        }



/*
 * Parse Network string value
 * Format = @instance_number:netAddr,adapter_type:.....
 * or
 * Format = @instance_number;netAddr,adapter_type;.....
 * netAddr is for either Inet address (IP) or window ID (US)
 * adapter is for adapter type
 *
 * Newwork_ID is exported by POE
 *
 * This function will fill in the given buffers as much as possible
 * will start to drop the (whole) info when the buffers are getting
 * full.
 */
void MPIDI_Get_net_param(char *cp, int MPI_only, int  my_taskid, int total_task,
                 char *netAddr,   long addrSize,
                 char *adapter,   long adptSize,
                 char *networkId, long networkIdSize)
{
    /* parse Network string, works with either user space or IP */
    /* removing the window numbers but keeping the device names */
    char *cp1, *cp2, *cp3, *dummy1, *dummy2, *dummy3;
    long nc, addrSpace=addrSize, addrLen=0, adptSpace=adptSize, adptLen=0;
    int  taskid, inst, i, n_comma;
    char delimiter = ':';
    char msgBuf[80];

    *netAddr = '\0';                       /* Make sure strcat starts   */
    *adapter = '\0';                       /* from the beginning.       */

    if ( strchr(cp, ';') ) {
       delimiter = ';';
    }


    if ( *cp == '@' ) {
       /* PE 4.1: MP_MPI_NETWORK = @inst:addr,adapter:...,... */
       /* Skip the instance number and get to the first colon */
       cp1 = strchr(cp,delimiter);

       while (cp1) {
          cp1++;                           /* Next address               */
          cp2 = strchr(cp1,',') + 1;       /* Address ends at a comma    */
          nc = cp2 - cp1;                  /* Add 1 to pick up the comma */
          if ( nc <= addrSpace ) {         /* Yes test  nc <= addrSpace  */
             strncat(netAddr, cp1, nc);    /* We will take care of the   */
             addrLen += nc;                /* last 'comma' later.        */
             addrSpace -= nc;
          }
          else
             break;
          if ( (cp1 = strchr(cp2,delimiter)) != NULL ) {  /* Adapter ends at a colon */
             nc = cp1 - cp2 + 1;            /* Add 1 to pick up colon  */
             if ( nc < adptSpace ) {
                strncat(adapter, cp2, nc);
                adptLen += nc;
                adptSpace -= nc;
             }
             else {
                strncat(adapter, cp2, adptSpace-1);
                adptLen += (adptSpace-1);
                break;
             }
          }
          else {
             /*  Last one */
             nc = strlen(cp2);
             if ( nc < adptSpace ) {
                strcat(adapter, cp2);
                adptLen += nc;
             }
             else {
                strncat(adapter, cp2, adptSpace-1);
                adptLen += (adptSpace-1);
             }
          }
       }
       /* Terminate netAddr and adapter with NULL */
       if ( addrLen > 1 ) netAddr[addrLen-1] = '\0';  /* get rid of the last comma */
       if ( adptLen > 0 ) {
           if ( adapter[adptLen-1] == delimiter ) adapter[adptLen-1] = '\0';
           else                             adapter[adptLen] = '\0';
       }
    } else {
       /* MP_MPI_NETWORK = window_number:device_number  */
       cp1 = strchr(cp,':');
       nc = cp1 - cp;                      /* Don't include ':' */
       strncpy(netAddr, cp, nc);
       netAddr[nc] = '\0';
       strcpy(adapter, cp1+1);
    }
}


int MPIDI_Print_mpenv(int rank,int size)
{
        MPI_Comm comm = MPI_COMM_WORLD;
        MPID_Comm *comm_ptr;
        MPIDI_printenv_t sender;
        MPIDI_printenv_t *gatherer = NULL;
        int mytask;
        int task_count;
        MPIDI_TransportType transport_type;
        MPIDI_StripingType striping_type;
        int instances;
        int  is_mpi=1;
        char *cp;
        char *cp1;
        int i, j, k;
        int first, last;
        int len, tmp_val;
        char *userbuf = NULL;
        int *recvcount = NULL, *recvdispl = NULL;
        char **recvwork = NULL;
        int max_output_size = 0;
        FILE *fd;
        char *ts;
        char *ck;
        int ssi = 0;
        int current_size;
        int increment;
        int m, m1;
        char cflag;
        int  rc;
        pami_configuration_t     *pami_config, config;
        char *p;
        char popenstr[]={ "/bin/ksh -c "  };
        char *popenptr;
        char tempstr[128];
        int  mpi_errno;
        int  errflag=0;

        MPIDI_Set_mpich_env(rank,size);
        memset(&sender,0,sizeof(MPIDI_printenv_t));
        MPIDI_Update_mpenv();
        /* decoding strings */
        char *clock_string[] = {
#ifndef __LINUX__
                "AIX",
#else
                "OS",
#endif
                "Switch"        };
        char *develop_string[] = {
                "Minimum",
                "Normal",
                "Debug",
                "Develop"       };
        char *transport_type_string[] = {
                "ip",
                "us" };
        char *striping_type_set[] = {
                "Striping off",
                "Packet striping on" };

	/* Get MPI Library Version                */
	MPIDI_Banner((char*)&sender.version);
	/* Remove the trailing newline character */
	for (i=strlen(sender.version);i>0;i--) {
	if (sender.version[i-1] == '\n') sender.version[i-1] = '\0'; }

        gethostname(sender.hostname, MAXHOSTNAMELEN+1);
        /* Get task geometry information */
        cp = getenv("MP_PARTITION");     /* MP_PARTITION is always there */
        sender.partition = atol(cp);     /* Checking is not needed.      */
        sender.pid = getpid();
        mytask = mpich_env->this_task;
        sender.nprocs = mpich_env->nprocs;
        sender.single_thread = mpich_env->single_thread;
        sender.max_pkts_out = mpich_env->max_pkts_out;
        sender.recv_fifo_sz = mpich_env->recv_fifo_sz;
        task_count = sender.nprocs;
        strcpy(sender.wait_mode,"NOT SET"); /* will be updated in the future */
        strcpy(sender.clock_source,"OS");
        cp = getenv("MP_NODES");
        if (cp) strncpy(sender.nnodes, cp, 8);
        else strcpy(sender.nnodes, "NOT SET");
        cp = getenv("MP_TASKS_PER_NODE");
        if (cp) strncpy(sender.tasks_node, cp, 8);
        else strcpy(sender.tasks_node, "NOT SET");

        if(cp=getenv("MP_EUIDEVELOP"))  {
          strcpy((void *) &sender.develop,(void *)cp);
        } else
          memcpy(&sender.develop,"NOT SET ",8);

        if(cp=getenv("MP_EUIDEVICE"))  {
          strcpy((void *) &mp_euidevice,(void *)cp);
        } else
          memcpy(&mp_euidevice,"NOT SET ",8);
        strncpy(sender.device, mp_euidevice, 8);

        /* Get protocol info */
        transport_type = mpich_env->transport_type;
        switch (transport_type)
        {
            case IS_IP:
                strcpy(sender.euilib, transport_type_string[transport_type]);
                cp = NULL;
                cp = (char *) mpich_env->network_string;
                if (cp) {
                   strcpy(sender.window_id, "NA");
                   MPIDI_Get_net_param(cp, is_mpi, mytask, task_count,
                               tempstr, sizeof(tempstr),
                               sender.adapter_type, sizeof(sender.adapter_type),
                               sender.network_id, sizeof(sender.network_id));
                   /* Override network_id when network device is en0 or ml0 */
                   if ( !strcasecmp(sender.device, "en0") ||
                        !strcasecmp(sender.device, "ml0") ) {
                      strcpy(sender.network_id, "NA");
                   }
                } else {
                   strcpy(sender.window_id, "NOT_SET");
                   strcpy(sender.adapter_type, "NOT SET");
                   strcpy(sender.network_id, "NOT SET");
                }
                break;
            case IS_US:
                strcpy(sender.euilib, transport_type_string[transport_type]);
                cp = NULL;
                cp = (char *) mpich_env->network_string;
                if (cp) {
                   MPIDI_Get_net_param(cp, is_mpi, mytask, task_count,
                               sender.window_id, sizeof(sender.window_id),
                               sender.adapter_type, sizeof(sender.adapter_type),
                               sender.network_id, sizeof(sender.network_id));
                } else {
                   strcpy(sender.window_id, "NOT SET");
                   strcpy(sender.adapter_type, "NOT SET");
                   strcpy(sender.network_id, "NOT SET");
                }
                break;
            default:
                sprintf(tempstr, "_mpi_printenv: Get unknown transport type [%d]!\n", transport_type);
                TRACE_ERR("%s \n",tempstr);
                strcpy(sender.euilib, "UNKNOWN");
                strcpy(sender.window_id, "UNKNOWN");
                strcpy(sender.adapter_type, "UNKNOWN");
                strcpy(sender.network_id, "UNKNOWN");
                break;
        }
        /* Get run time setup */
        cp = getenv("MP_MSG_API");
        if (cp) strncpy(sender.protocol, cp, 16);
        else strcpy(sender.protocol, "MPI");
        cp = getenv("LIBPATH");
        if (cp) strncpy(sender.libpath, cp, MAXPATHLEN-1);
        else strcpy(sender.libpath, "NOT SET");
        getcwd(sender.directory, FILENAME_MAX+1);

        if(sizeof(void*) == 4)
          sender.mode_64 = 0;
        else
          sender.mode_64 = 1;
        
        sender.threaded = 1;         /* Always 1 */
        cp = getenv("AIXTHREAD_SCOPE");
        if (cp) strncpy(sender.thread_scope, cp, 8);
        else strcpy(sender.thread_scope,"NOT SET");

        cp = getenv("MP_CPU_USE");
        if (cp) strncpy(sender.cpu_use, cp, 10);
        else strcpy(sender.cpu_use, "NOT SET");
        cp = getenv("MP_ADAPTER_USE");
        if (cp) strncpy(sender.adapter_use, cp, 10);
        else strcpy(sender.adapter_use, "NOT SET");
#ifndef LINUX
        cp = getenv("MP_PRIORITY");
        if (cp) strncpy(sender.priority, cp, 24);
        else strcpy(sender.priority, "NOT SET");
#else
        strcpy(sender.priority, "NA");
#endif
	cp = getenv("MP_TIMEOUT");
	if (cp) {
	  sender.timeout = atoi(cp);
	  if(sender.timeout <= 0) {
	    sender.timeout = 150;   /* default MP_TIMEOUT is 150s */
	  }
	} else {
	    sender.timeout = 150;
	}

        sender.interrupts = mpich_env->interrupts;
        sender.mp_statistics = mpich_env->mp_statistics;
        sender.polling_interval = mpich_env->polling_interval;
        sender.eager_limit = mpich_env->eager_limit;
        sender.use_token_flow_control=MPIDI_Process.is_token_flow_control_on;
        sender.retransmit_interval = mpich_env->retransmit_interval;
        sender.buffer_mem = mpich_env->buffer_mem;
        sender.buffer_mem_max = mpich_env->buffer_mem_max;

        /* Get shared memory  */
        sender.shmem_pt2pt = MPIDI_Process.shmem_pt2pt;
        sender.use_shmem = mpich_env->use_shmem;
#ifndef LINUX
        cp = getenv("MEMORY_AFFINITY");
        if ( cp ) strncpy(sender.mem_affinity, cp, 8);
        else strcpy(sender.mem_affinity, "NOT SET");
#else
        strcpy(sender.mem_affinity, "NA");
#endif
        sender.instances = mpich_env->instances;
        strcpy(sender.striping_type, striping_type_set[(mpich_env->strip_type)]);


        pami_config = (pami_configuration_t *) MPIU_Malloc(10 * sizeof(pami_configuration_t));
        memset((void *) pami_config,0, sizeof(pami_config));
        pami_config[0].name = (pami_attribute_name_t)PAMI_CONTEXT_ACK_THRESH;
        pami_config[1].name = (pami_attribute_name_t)PAMI_CONTEXT_REXMIT_BUF_CNT;
        pami_config[2].name = (pami_attribute_name_t)PAMI_CONTEXT_REXMIT_BUF_SZ;
        pami_config[3].name = (pami_attribute_name_t)PAMI_CONTEXT_RC_MAX_QP;
        pami_config[4].name = (pami_attribute_name_t)PAMI_CONTEXT_RC_USE_LMC;
        rc= PAMI_Context_query(MPIDI_Context[0], pami_config, 5);
        if (rc != PAMI_SUCCESS) {
             printf("mp_printenv ERROR PAMI_Context_query() failed rc=%d\n",rc); fflush(stdout);
             return rc;
        }
        sender.ack_thresh = pami_config[0].value.intval;
        sender.rexmit_buf_cnt = pami_config[1].value.intval;
        sender.rexmit_buf_size = pami_config[2].value.intval;
        sender.rc_max_qp = pami_config[3].value.intval;
        if (pami_config[4].value.intval)
            strcpy(sender.rc_qp_use_lmc, "YES");
        else
            strcpy(sender.rc_qp_use_lmc, "NO");
        MPIU_Free(pami_config);

        /* - Begin Bulk transfer/RDMA & IB settings */
        if ( mpich_env->use_rdma == PAMI_HINT_DISABLE )
           strcpy(sender.use_bulk_xfer, "NO");
        else
           strcpy(sender.use_bulk_xfer, "YES");

        sprintf(sender.bulk_min_msg_size, "%d", mpich_env->bulk_min_msg_size);


        /* M28,M32 -- End Bulk transfer/RDMA & IB settings */

        if ( cp = getenv("MP_DEBUG_NOTIMEOUT") )
           strcpy(sender.debug_notimeout, cp);
        else
           strcpy(sender.debug_notimeout, "NOT_SET");

        sender.statistics = mpich_env->statistics;
        cp = getenv("MP_STDINMODE");
        if ( cp ) strncpy(sender.stdinmode, cp, 10);
        else strcpy(sender.stdinmode, "NOT SET");
        cp = getenv("MP_STDOUTMODE");
        if ( cp ) strncpy(sender.stdoutmode, cp, 10);
        else strcpy(sender.stdoutmode, "NOT SET");


        /* Check if there are any environment variables of the form MP_S_, and count them */
        {
                extern char **environ;
                char **envp = environ;
                int env_count = 0;
                while (*envp) {
                   if (!strncmp(*envp, "MP_S_",5)) env_count++;
                   if (!strncmp(*envp, "MP_CCL_",7)) env_count++;   /* MP_CCL_TIMING is also service */
                        envp++;
                }
                sender.service_variables = env_count;
        }

        if (mytask == 0) {  /* allocate a receive buffer for the gather of the base structure */
                gatherer = (MPIDI_printenv_t*) MPIU_Malloc(task_count * sizeof(MPIDI_printenv_t));
                if (!gatherer) {
                        TRACE_ERR("_mpi_printenv gatherer MPIU_Malloc failed rc=%d\n",rc);
                        return 1;
                }
                memset(gatherer,0,task_count*sizeof(MPIDI_printenv_t));
        }


        mpi_errno = MPI_SUCCESS;
        MPID_Comm_get_ptr( comm, comm_ptr );
        mpi_errno = MPIR_Gather_impl(&sender, sizeof(MPIDI_printenv_t), MPI_BYTE, gatherer,
                                    sizeof(MPIDI_printenv_t),MPI_BYTE, 0,comm_ptr,
                                    (int *) &errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            errflag = TRUE;
        }

        /* work through results and compare */

        if (mytask == 0) {
		MATCHC(version,"Library:",128);
                MATCHC(hostname,"Hostname:",MAXHOSTNAMELEN+1);
                MATCHI(partition,"Job ID (MP_PARTITION):");
                MATCHI(nprocs,"Number of Tasks (MP_PROCS):");
                MATCHC(nnodes,"Number of Nodes (MP_NODES):",8);
                MATCHC(tasks_node,"Number of Tasks per Node (MP_TASKS_PER_NODE):",8);
                MATCHC(euilib,"Library Specifier (MP_EUILIB):",8);
                MATCHC(adapter_type,"Adapter Name:",32);
                MATCHC(window_id,"Window ID:",128);
                if ( getenv("MP_S_STRIPING_DBG") ) MATCHC(network_id,"Network ID:",32);
                MATCHC(device,"Device Name (MP_EUIDEVICE):",8);
#ifndef LINUX   /* M27 */
                MATCHI(instances,"Window Instances (MP_INSTANCES * # of networks):");
                MATCHC(striping_type,"Striping Setup:",40);
#endif
                MATCHC(protocol,"Protocols in Use (MP_MSG_API):",16);
                MATCHC(libpath,"Effective Libpath (LIBPATH):",MAXPATHLEN);
                MATCHC(directory,"Current Directory:",FILENAME_MAX+1);
                MATCHB(mode_64,"64 Bit Mode:");
#ifndef LINUX   /* M27 */
                MATCHB(threaded,"Threaded Library:");
                MATCHC(thread_scope,"Requested Thread Scope (AIXTHREAD_SCOPE):",8);
#endif
                MATCHC(cpu_use,"CPU Use (MP_CPU_USE):",10);
                MATCHC(adapter_use,"Adapter Use (MP_ADAPTER_USE):",10);
                MATCHC(clock_source,"Clock Source (MP_CLOCK_SOURCE):",8);
                MATCHC(priority,"Priority Class (MP_PRIORITY):",24);
                MATCHI(timeout,"Connection Timeout (MP_TIMEOUT/sec):");
                MATCHB(interrupts,"Adapter Interrupts Enabled (MP_CSS_INTERRUPT):");
                MATCHI(polling_interval,"Polling Interval (MP_POLLING_INTERVAL/usec):");
                MATCHI(buffer_mem,"Buffer Memory (MP_BUFFER_MEM/Bytes):");
                MATCHLL(buffer_mem_max,"Max. Buffer Memory (MP_BUFFER_MEM_MAX/Bytes):");
                MATCHI(eager_limit,"Message Eager Limit (MP_EAGER_LIMIT/Bytes):");
                MATCHI(use_token_flow_control,"Use token flow control:");
                MATCHC(wait_mode,"Message Wait Mode(MP_WAIT_MODE):",8);
                MATCHI(retransmit_interval,"Retransmit Interval (MP_RETRANSMIT_INTERVAL/count):");
                MATCHB(use_shmem,"Shared Memory Enabled (MP_SHARED_MEMORY):");
                MATCHB(shmem_pt2pt,"Intranode pt2pt Shared Memory Enabled (MP_SHMEM_PT2PT):");
                MATCHB(mp_statistics,"Statistics Collection Enabled (MP_STATISTICS):");
                MATCHC(mem_affinity,"MEMORY_AFFINITY:",8);
                MATCHT(single_thread,"Single Thread Usage(MP_SINGLE_THREAD):");
                MATCHI(recv_fifo_sz,"DMA Receive FIFO Size (Bytes):");
                MATCHI(max_pkts_out,"Max outstanding packets:");
                MATCHC(develop,"Develop Mode (MP_EUIDEVELOP):",16);
                MATCHC(stdinmode,"Standard Input Mode (MP_STDINMODE):",12);
                MATCHC(stdoutmode,"Standard Output Mode (MP_STDOUTMODE):",12);
                MATCHI(service_variables,"Number of Service Variables set (MP_S_*):");
                printf("--------------------End of MPI Environment Report-------------------------\n");
                fflush(stdout);
        }  /* task 0 specific stuff */


        if (mytask == 0) {
            MPIU_Free(gatherer);
        }

        return 0;
}

  /* ------------------------- */
  /* if (MP_STATISTICS==yes)   */
  /* print statistics data     */
  /* ------------------------- */
void MPIDI_print_statistics() {
  if ((MPIDI_Process.mp_statistics) ||
       (MPIDI_Process.mp_printenv)) {
       if (MPIDI_Process.mp_statistics) {
           MPIDI_Statistics_write(stdout);
           if (mpid_statp) {
               MPIU_Free(mpid_statp);
               mpid_statp=NULL;
           }
       }
    if (MPIDI_Process.mp_printenv) {
        if (mpich_env) {
            MPIU_Free(mpich_env);
            mpich_env=NULL;
        }
    }
  }
}

#endif  /* MPIDI_PRINTENV || MPIDI_STATISTICS         */

/**
 * \brief validate whether a lpid is in a given group
 *
 * Searches the group lpid list for a match.
 *
 * \param[in] lpid  World rank of the node in question
 * \param[in] grp   Group to validate against
 * \return TRUE is lpid is in group
 */

int MPIDI_valid_group_rank(int lpid, MPID_Group *grp) {
        int size = grp->size;
        int z;

        for (z = 0; z < size &&
                lpid != grp->lrank_to_lpid[z].lpid; ++z);
        return (z < size);
}

/****************************************************************/
/* function MPIDI_uppers converts a passed string to upper case */
/****************************************************************/
void MPIDI_toupper(char *s)
{
   int i;
   if (s != NULL) {
      for(i=0;i<strlen(s);i++) s[i] = toupper(s[i]);
   }
}

/*
  -----------------------------------------------------------------
  Name:           MPID_scan_str()

  Function:       Scan a flag string for 2 out of 3 possible
                  characters (K, M, G). Return a 1 if neither
                  character is found otherwise return the character
                  along with a buffer containing the string without
                  the character.
                  value are valid. If they are valid, the
                  multiplication of the number and the units
                  will be returned as an unsigned int. If the
                  number and units are invalid, a 1 will be returned.

  Description:    search string for character or end of string
                  if string contains either entered character
                    check which char it is, set multiplier
                  if no chars found, return error

  Parameters:     A0 = MPIDI_scan_str(A1, A2, A3, A4, A5)

                  A1    string to scan                char *
                  A2    first char to scan for        char
                  A3    second char to scan for       char
                  A4    multiplier                    char *
                  A5    returned string               char *

                  A0    Return Code                   int


  Return Codes:   0 OK
                  1 input chars not found
  ------------------------------------------------------------
*/
int MPIDI_scan_str(char *my_str, char fir_c, char sec_c, char *multiplier, char *tempbuf)
{
   int str_ptr;           /*index counter into string*/
   int found;             /*indicates whether one of input chars found*/
   int len_my_str;        /*length of string with size and units*/

   str_ptr = 0;           /*start at beginning of string*/
   found = 0;             /*no chars found yet*/

   len_my_str = strlen(my_str);

   /* first check if all 'characters' of *my_str are digits,  */
   /* str_ptr points to the first occurrence of a character   */
   for (str_ptr=0; str_ptr<len_my_str; str_ptr++) {
      if (str_ptr == 0) {   /* there can be a '+' or a '-' in the first position   */
                            /* but I do not allow a negative value because there's */
                            /* no negative amount of memory...                     */
         if (my_str[0] == '+') {
            tempbuf[0] = my_str[0];  /* copy sign */
            /* this is ok but a digit MUST follow */
            str_ptr++;
            /* If only a '+' was entered the next character is '\0'. */
            /* This is not a digit so the error message shows up     */
         }
      }
      if (!isdigit(my_str[str_ptr])) {
         break;
      }
      tempbuf[str_ptr] = my_str[str_ptr]; /* copy to return string */
   } /* endfor */

   tempbuf[str_ptr] = 0x00;       /* terminate return string, this was NOT done before this modification! */

   if((my_str[str_ptr] == fir_c) || (my_str[str_ptr] == sec_c)) {
      /*check which char it is, then set multiplier and indicate char found*/
      switch(my_str[str_ptr]) {
        case 'K':
          *multiplier = 'K';
          found++;
          break;
        case 'M':
          *multiplier = 'M';
          found++;
          break;
        case 'G':
          *multiplier = 'G';
          found++;
          break;
      }
  /*    my_str[str_ptr] = 0; */  /*change char in string to end of string char*/
   }
  if (found == 0) {             /*if input chars not found, indicate error*/
    return(1); }
  else {
    /* K, M or G should be the last character, something like 64M55 is invalid */
    if (str_ptr == len_my_str-1) {
       return(0);                 /*if input chars found, return good status*/
    } else {
       /* I only allow a 'B' to follow. This is not documented but reflects the */
       /* behaviour of earlier poe parsing. 64MB is valid, but after 'B' the    */
       /* string must end */
       if (my_str[str_ptr+1] == 'B' && (str_ptr+1) == (len_my_str-1)) {
          return(0);                 /*if input chars found, return good status*/
       } else {
          return(1);
       } /* endif */
    } /* endif */
  }
}
/*
  -----------------------------------------------------------------
  Name:           MPIDI_scan_str3()

  Function:       Scan a flag string for 3 out of 3 possible
                  characters (K, M, G). Return a 1 if neither
                  character is found otherwise return the character
                  along with a buffer containing the string without
                  the character.
                  value are valid. If they are valid, the
                  multiplication of the number and the units
                  will be returned as an unsigned int. If the
                  number and units are invalid, a 1 will be returned.

  Description:    search string for character or end of string
                  if string contains either entered character
                    check which char it is, set multiplier
                  if no chars found, return error

  Parameters:     A0 = MPIDI_scan_str(A1, A2, A3, A4, A5, A6)

                  A1    string to scan                char *
                  A2    first char to scan for        char
                  A3    second char to scan for       char
                  A4    third char to scan for        char
                  A5    multiplier                    char *
                  A6    returned string               char *

                  A0    Return Code                   int

  Return Codes:   0 OK
                  1 input chars not found
  ------------------------------------------------------------
*/
int MPIDI_scan_str3(char *my_str, char fir_c, char sec_c, char thr_c, char *multiplier, char *tempbuf)
{

   int str_ptr;           /*index counter into string*/
   int found;             /*indicates whether one of input chars found*/
   int len_my_str;        /*length of string with size and units*/

   str_ptr = 0;           /*start at beginning of string*/
   found = 0;             /*no chars found yet*/

   len_my_str = strlen(my_str);

   /* first check if all 'characters' of *my_str are digits,  */
   /* str_ptr points to the first occurrence of a character   */
   for (str_ptr=0; str_ptr<len_my_str; str_ptr++) {
      if (str_ptr == 0) {   /* there can be a '+' or a '-' in the first position   */
                            /* but I do not allow a negative value because there's */
                            /* no negative amount of memory...                     */
         if (my_str[0] == '+') {
            tempbuf[0] = my_str[0];  /* copy sign */
            /* this is ok but a digit MUST follow */
            str_ptr++;
            /* If only a '+' was entered the next character is '\0'. */
            /* This is not a digit so the error message shows up     */
         }
      }
      if (!isdigit(my_str[str_ptr])) {
         break;
      }
      tempbuf[str_ptr] = my_str[str_ptr]; /* copy to return string */
   } /* endfor */

   tempbuf[str_ptr] = 0x00;       /* terminate return string, this was NOT done before this modification! */

   if((my_str[str_ptr] == fir_c) || (my_str[str_ptr] == sec_c) || (my_str[str_ptr] == thr_c)) {
      /*check which char it is, then set multiplier and indicate char found*/
      switch(my_str[str_ptr]) {
        case 'K':
          *multiplier = 'K';
          found++;
          break;
        case 'M':
          *multiplier = 'M';
          found++;
          break;
        case 'G':
          *multiplier = 'G';
          found++;
          break;
      }
  /*    my_str[str_ptr] = 0; */  /*change char in string to end of string char*/
   }
  if (found == 0) {             /*if input chars not found, indicate error*/
    return(1); }
  else {
    /* K, M or G should be the last character, something like 64M55 is invalid */
    if (str_ptr == len_my_str-1) {
       return(0);                 /*if input chars found, return good status*/
    } else {
       /* I only allow a 'B' to follow. This is not documented but reflects the */
       /* behaviour of earlier poe parsing. 64MB is valid, but after 'B' the    */
       /* string must end */
       if (my_str[str_ptr+1] == 'B' && (str_ptr+1) == (len_my_str-1)) {
          return(0);                 /*if input chars found, return good status*/
       } else {
          return(1);
       } /* endif */
    } /* endif */
  }
}

/*
  -----------------------------------------------------------------
  Name:           MPIDI_checkit()

  Function:       Determine whether a given number and units
                  value are valid. If they are valid, the
                  multiplication of the number and the units
                  will be returned as an unsigned int. If the
                  number and units are invalid, a 1 will be returned.

  Description:    if units is G
                    if value is > 4 return error
                    else multiplier is 1G
                  else if units is M
                    if value is > 4K return error
                    else multiplier is 1M
                  else if units is K
                    if value is > 4M return error
                    else multiplier is 1K
                  if value < 1 return error
                  else
                    multiply value by multiplier
                    return result

  Parameters:     A0 = MPIDI_checkit(A1, A2, A3)

                  A1    given value                   int
                  A2    given units                   char *
                  A3    result                        unsigned int *

                  A0    Return Code                   int

  Return Codes:   0 OK
                  1 bad value
  ------------------------------------------------------------
*/
int MPIDI_checkit(int myval, char myunits, unsigned int *mygoodval)
{
  int multiplier = ONE;             /*units multiplier for entered value*/

  if (myunits == 'G') {             /*if units is G*/
    if (myval>4) return 1;          /*entered value can't be greater than 4*/
    else multiplier = ONEG;         /*if OK, mult value by units*/
  }
  else if (myunits == 'M') {        /*if units is M*/
    if (myval > (4*ONEK)) return 1;   /*value can't be > 4096*/
    else multiplier = ONEM;         /*if OK, mult value by units*/
  }
  else if (myunits == 'K') {        /*if units is K*/
    if (myval > (4*ONEM)) return 1; /*value can't be > 4M*/
    else multiplier = ONEK;         /*if OK, mult value by units*/
  }
  if (myval < 1) return 1;          /*value can't be less than 1*/

  *mygoodval = myval * multiplier;  /*do multiplication*/
  return 0;                         /*good return*/

}



 /***************************************************************************
 Function Name: MPIDI_atoi

 Description:   Convert a string into an interger.  The string can be all
                digits or includes symbols 'K', 'M'.

 Parameters:    char * -- string to be converted
                unsigned int  * -- result val (caller to cast to int* or long*)

 Return:        int    0 if AOK
                       number of errors.
 ***************************************************************************/
int MPIDI_atoi(char* str_in, unsigned int* val)
{
   char tempbuf[256];
   char size_mult;                 /* multiplier for size strings */
   int  i, tempval;
   int  letter=0, retval=0;

   /***********************************/
   /* Check for letter                */
   /***********************************/
   for (i=0; i<strlen(str_in); i++) {
      if (!isdigit(str_in[i])) {
         letter = 1;
         break;
      }
   }
   if (!letter) {    /* only digits */
      errno = 0;     /*  should set errno to 0 before atoi() call */
      *val = atoi(str_in);
      if (errno) {   /* no check for negative integer, there's no '-' in the string */
         retval = errno;
      }
   }
   else {
      /***********************************/
      /* Check for K or M.               */
      /***********************************/
      MPIDI_toupper(str_in);
      retval = MPIDI_scan_str(str_in, 'M', 'K', &size_mult, tempbuf);

      if ( retval == 0) {
         tempval = atoi(tempbuf);

         /***********************************/
         /* If 0 K or 0 M entered, set to 0 */
         /* otherwise, do conversion.       */
         /***********************************/
         if (tempval != 0)
            retval = MPIDI_checkit(tempval, size_mult, (unsigned int*)val);
         else
            *val = 0;
      }

      if (retval == 0) {
         tempval = atoi(tempbuf);
         retval = MPIDI_checkit(tempval, size_mult, (unsigned int*)val);
      }
      else
         *val = 0;
   }

   return retval;
}

 /***************************************************************************
  Name:           MPIDI_checkll()
  
  Function:       Determine whether a given number and units
                  value are valid. If they are valid, the
                  multiplication of the number and the units
                  will be returned as an unsigned int. If the
                  number and units are invalid, a 1 will be returned.

  Description:    if units is G
                    multiplier is 1G
                  else if units is M
                    multiplier is 1M
                  else if units is K
                    multiplier is 1K
                  else
                    return error

                    multiply value by multiplier
                    return result
  Parameters:     A0 = MPIDI_checkll(A1, A2, A3)

                  A1    given value                   int
                  A2    given units                   char *
                  A3    result                        long long *

                  A0    Return Code                   int

  Return Codes:   0 OK
                  1 bad value
 ***************************************************************************/

int MPIDI_checkll(int myval, char myunits, long long *mygoodval)
{
  int multiplier;                   /* units multiplier for entered value */

  if (myunits == 'G') {             /* if units is G */
     multiplier = ONEG;
  }
  else if (myunits == 'M') {        /* if units is M */
     multiplier = ONEM;
  }
  else if (myunits == 'K') {        /* if units is K */
     multiplier = ONEK;
  }
  else
     return 1;                      /* Unkonwn unit */

  *mygoodval = (long long) myval * multiplier;  /* do multiplication */
  return 0;                         /* good return */
}


int MPIDI_atoll(char* str_in, long long* val)
{
   char tempbuf[256];
   char size_mult;                 /* multiplier for size strings */
   int  i, tempval;
   int  letter=0, retval=0;

   /*****************************************/
   /* Check for letter, if none, MPIDI_atoi */
   /*****************************************/
   for (i=0; i<strlen(str_in); i++) {
      if (!isdigit(str_in[i])) {
         letter = 1;
         break;
      }
   }
   if (!letter) {   /* only digits */
      errno = 0;
      *val = atoll(str_in);
      if (errno) {
         retval = errno;
      }
   }
   else {
      /***********************************/
      /* Check for K or M.               */
      /***********************************/
      MPIDI_toupper(str_in);
      retval= MPIDI_scan_str3(str_in, 'G', 'M', 'K', &size_mult, tempbuf);

      if ( retval == 0) {
         tempval = atoi(tempbuf);

         /***********************************/
         /* If 0 K or 0 M entered, set to 0 */
         /* otherwise, do conversion.       */
         /***********************************/
         if (tempval != 0)
            retval = MPIDI_checkll(tempval, size_mult, val);
         else
            *val = 0;
      }
   }

   return retval;
}



/****************************************************/
/*     Collective Selection Benchmarking utils      */
/****************************************************/

const char *         xfer_array_str[PAMI_XFER_COUNT];
int task_id;
int num_tasks;

static int MPIDI_collsel_print_usage()
{
  if(!task_id)
    fputs("Usage: pami_tune (MPICH) [options]\n\
Options:\n\
  -c            Comma separated list of collectives to benchmark\n\
                Valid options are: \n\
                   allgather, allgatherv_int, allreduce, alltoall,\n\
                   alltoallv_int, ambroadcast, amgather, amreduce,\n\
                   amscatter, barrier, broadcast, gather, gatherv_int,\n\
                   reduce, reduce_scatter, scan, scatter, scatterv_int\n\
                   (Default: all collectives)\n\n\
  -m            Comma separated list of message sizes to benchmark\n\
                (Default: 1 to 2^k, where k <= 20)\n\n\
  -g            Comma separated list of geometry sizes to benchmark\n\
                (Default: Powers of 2 (plus and minus one as well))\n\n\
  -i            Number of benchmark iterations per algorithm\n\
                (Default: 1000. Maximum is 2000)\n\n\
  -f <file>     Input INI file containing benchmark parameters\n\
                You can override a parameter with a command line argument\n\n\
  -o <file>     Output XML file containing benchmark results\n\
                (Default: pami_tune_results.xml)\n\n\
  -d            Diagnostics mode. Verify correctness of collective algorithms\n\
                (Default: Disabled)\n\n\
  -p            Print the Collective Algorithm performance data to standard output\n\
                (Default: Disabled)\n\n\
  -h            Print this help message\n\n", stdout);

  return 0;
}


static void MPIDI_collsel_init_advisor_params(advisor_params_t *params)
{
  params->num_collectives = 0;
  params->num_procs_per_node = 0;
  params->num_geometry_sizes = 0;
  params->num_message_sizes = 0;
  params->collectives = NULL;
  params->procs_per_node = NULL;
  params->geometry_sizes = NULL;
  params->message_sizes = NULL;
  /* Set the following to -1, so that we can
     check if the user has set them or not */
  params->iter = -1;
  params->verify = -1;
  params->verbose = -1;
  params->checkpoint = -1;
}


static void MPIDI_collsel_free_advisor_params(advisor_params_t *params)
{
  if(params->collectives) MPIU_Free(params->collectives);
  if(params->procs_per_node) MPIU_Free(params->procs_per_node);
  if(params->geometry_sizes) MPIU_Free(params->geometry_sizes);
  if(params->message_sizes) MPIU_Free(params->message_sizes);
}


static void MPIDI_collsel_init_xfer_tables()
{
  xfer_array_str[PAMI_XFER_BROADCAST]     ="broadcast";
  xfer_array_str[PAMI_XFER_ALLREDUCE]     ="allreduce";
  xfer_array_str[PAMI_XFER_REDUCE]        ="reduce";
  xfer_array_str[PAMI_XFER_ALLGATHER]     ="allgather";
  xfer_array_str[PAMI_XFER_ALLGATHERV]    ="allgatherv";
  xfer_array_str[PAMI_XFER_ALLGATHERV_INT]="allgatherv_int";
  xfer_array_str[PAMI_XFER_SCATTER]       ="scatter";
  xfer_array_str[PAMI_XFER_SCATTERV]      ="scatterv";
  xfer_array_str[PAMI_XFER_SCATTERV_INT]  ="scatterv_int";
  xfer_array_str[PAMI_XFER_GATHER]        ="gather";
  xfer_array_str[PAMI_XFER_GATHERV]       ="gatherv";
  xfer_array_str[PAMI_XFER_GATHERV_INT]   ="gatherv_int";
  xfer_array_str[PAMI_XFER_BARRIER]       ="barrier";
  xfer_array_str[PAMI_XFER_ALLTOALL]      ="alltoall";
  xfer_array_str[PAMI_XFER_ALLTOALLV]     ="alltoallv";
  xfer_array_str[PAMI_XFER_ALLTOALLV_INT] ="alltoallv_int";
  xfer_array_str[PAMI_XFER_SCAN]          ="scan";
  xfer_array_str[PAMI_XFER_REDUCE_SCATTER]="reduce_scatter";
  xfer_array_str[PAMI_XFER_AMBROADCAST]   ="ambroadcast";
  xfer_array_str[PAMI_XFER_AMSCATTER]     ="amscatter";
  xfer_array_str[PAMI_XFER_AMGATHER]      ="amgather";
  xfer_array_str[PAMI_XFER_AMREDUCE]      ="amreduce";
}

static int MPIDI_collsel_process_collectives(char *coll_arg, advisor_params_t *params)
{
  int i, ret = 0, arg_len = strlen(coll_arg);
  char *collectives = (char *) MPIU_Malloc(arg_len + 1);
  char *coll;
  int infolevel = 0;
#if (MPIDI_STATISTICS || MPIDI_PRINTENV)
  if(MPIDI_Process.mp_infolevel >= 1)
    infolevel = 1;
#endif
  /* if already set via config file, free it */
  if(params->collectives)
  {
    MPIU_Free(params->collectives);
    params->num_collectives = 0;
  }
  /* Allocating some extra space should be fine */
  params->collectives = (pami_xfer_type_t *)MPIU_Malloc(sizeof(pami_xfer_type_t)*PAMI_XFER_COUNT);

  strcpy(collectives, coll_arg);
  coll = strtok(collectives,",");
  while (coll != NULL)
  {
    int invalid_collective = 0;
    for(i=0; i<PAMI_XFER_COUNT; i++)
    {
      if(strcmp(coll, xfer_array_str[i]) == 0)
      {
        if(i == 4)
        {
          if(infolevel >= 1) 
            fprintf(stderr,"WARNING: MPICH (pami_tune) doesn't support tuning for ALLGATHERV. ALLGATHERV tuning will be skipped.\nTune for ALLGATHERV_INT instead\n");
          invalid_collective = 1;
          break;
        }
        else if(i == 7)
        {
          if(infolevel >= 1)
            fprintf(stderr,"WARNING: MPICH (pami_tune) doesn't support tuning for SCATTERV. SCATTERV tuning will be skipped.\nTune for SCATTERV_INT instead\n");
          invalid_collective = 1;
          break;
        }
        else if(i == 10)
        {
          if(infolevel >= 1)
            fprintf(stderr,"WARNING: MPICH (pami_tune) doesn't support tuning for GATHERV. GATHERV tuning will be skipped.\nTune for GATHERV_INT instead\n");
          invalid_collective = 1;
          break;
        }
        else if(i == 14)
        {
          if(infolevel >= 1)
            fprintf(stderr,"WARNING: MPICH (pami_tune) doesn't support tuning for ALLTOALLV. ALLTOALLV tuning will be skipped.\nTune for ALLTOALLV_INT instead\n");
          invalid_collective = 1;
          break;
        }
        else
        {
          params->collectives[params->num_collectives++] = i;
          break;
        }
      }
    }
    /* arg did not match any collective */
    if(i == PAMI_XFER_COUNT || invalid_collective)
    {
      if(!task_id)
      {
        fprintf(stderr, "Invalid collective: %s\n", coll);
      }
      break;
    }
    coll = strtok(NULL,",");
  }
  if(params->num_collectives == 0)
  {
    MPIU_Free(params->collectives);
    params->collectives = NULL;
    ret = 1;
  }

  MPIU_Free(collectives);
  return ret;
}


static int MPIDI_collsel_process_msg_sizes(char *msg_sizes_arg, advisor_params_t *params)
{
  int ret = 0, arg_len = strlen(msg_sizes_arg);
  char *msg_sizes = (char *) MPIU_Malloc(arg_len + 1);
  char *msg_sz;
  size_t tmp;
  size_t array_size = 50;
  /* if already set via config file, free it */
  if(params->message_sizes)
  {
    MPIU_Free(params->message_sizes);
    params->num_message_sizes = 0;
  }
  /* Allocating some extra space should be fine */
  params->message_sizes = (size_t *)MPIU_Malloc(sizeof(size_t) * array_size);

  strcpy(msg_sizes, msg_sizes_arg);
  msg_sz = strtok(msg_sizes,",");
  while (msg_sz != NULL)
  {
    tmp = strtol(msg_sz, NULL, 10);
    if(tmp == 0)
    {
      MPIU_Free(params->message_sizes);
      params->message_sizes = NULL;
      if(!task_id)
      {
        fprintf(stderr, "Invalid message size: %s\n", msg_sz);
      }
      ret = 1;
      break;
    }

    if(params->num_message_sizes >= array_size)
    {
      array_size *= 2;
      params->message_sizes = (size_t *) MPIU_Realloc(params->message_sizes,
                                                      sizeof(size_t) * array_size);
    }
    params->message_sizes[params->num_message_sizes++] = tmp;
    msg_sz = strtok(NULL,",");
  }
  MPIU_Free(msg_sizes);
  return ret;
}


static int MPIDI_collsel_process_geo_sizes(char *geo_sizes_arg, advisor_params_t *params)
{
  int ret = 0, arg_len = strlen(geo_sizes_arg);
  char *geo_sizes = (char *) MPIU_Malloc(arg_len + 1);
  char *geo_sz;
  size_t tmp;
  size_t array_size = 50;
  /* if already set via config file, free it */
  if(params->geometry_sizes)
  {
    MPIU_Free(params->geometry_sizes);
    params->num_geometry_sizes = 0;
  }
  /* Allocating some extra space should be fine */
  params->geometry_sizes = (size_t *)MPIU_Malloc(sizeof(size_t) * array_size);

  strcpy(geo_sizes, geo_sizes_arg);
  geo_sz = strtok(geo_sizes,",");
  while (geo_sz != NULL)
  {
    tmp = strtol(geo_sz, NULL, 10);
    if(tmp < 2 || tmp > num_tasks)
    {
      MPIU_Free(params->geometry_sizes);
      params->geometry_sizes = NULL;
      if(!task_id)
      {
        fprintf(stderr, "Invalid geometry size: %s\n", geo_sz);
      }
      ret = 1;
      break;
    }

    if(params->num_geometry_sizes >= array_size)
    {
      array_size *= 2;
      params->geometry_sizes = (size_t *) MPIU_Realloc(params->geometry_sizes,
                                                       sizeof(size_t) * array_size);
    }
    params->geometry_sizes[params->num_geometry_sizes++] = tmp;
    geo_sz = strtok(NULL,",");
  }
  MPIU_Free(geo_sizes);
  return ret;
}


static int MPIDI_collsel_process_output_file(char *filename, char **out_file)
{
  char *newname;
  int i, filename_len, ret = 0;
  struct stat ost;

  filename_len = strlen(filename);

  /* Check if file already exists */
  if(stat(filename, &ost) == 0)
  {
    if(!S_ISREG(ost.st_mode))
    {
      fprintf(stderr, "Error: %s exists and is not a regular file\n", filename);
      return 1;
    }
    fprintf(stderr, "File %s already exists, renaming existing file\n", filename);

    newname = (char *) MPIU_Malloc(filename_len + 5);
    for (i = 0; i < 500; ++i)
    {
      sprintf(newname,"%s.%d", filename, i);
      if(!(access(newname, F_OK) == 0))
      {
        ret = rename(filename, newname);
        break;
      }
    }
    MPIU_Free(newname);
    if(i == 500 || ret != 0)
    {
      fprintf(stderr, "Error renaming file\n");
      return 1;
    }
  }
  /* if file name is already set via config file, free it */
  if(*out_file) MPIU_Free(*out_file);
  *out_file = (char *)MPIU_Malloc(filename_len + 1);
  strcpy(*out_file, filename);

  return ret;
}


static char* MPIDI_collsel_ltrim(char *line)
{
  while(*line && isspace(*line))
    ++line;

  return line;
}

static char* MPIDI_collsel_rtrim(char *line)
{
  char *end = line + strlen(line);
  while(end > line && isspace(*--end))
    *end = '\0';

  return line;
}

static int MPIDI_collsel_checkvalue(char *name, char *value, const char *filename, int *ival)
{
  int ret = 0;
  char *tmp;
  *ival = (int)strtol(value, &tmp, 10);
  if(*ival != 1)
  {
    if((*ival == 0 && value == tmp)|| *ival != 0)
    {
      if(!task_id)
        fprintf(stderr, "Invalid value for %s parameter: %s in file: %s\n", name, value, filename);
      ret = 1;
    }
  }
  return ret;
}


static int MPIDI_collsel_process_ini_file(const char *filename, advisor_params_t *params, char **out_file)
{
  char *line, *start, *name, *value;
  int ret = 0;
  struct stat ist;

  if(stat(filename, &ist) == 0)
  {
    if(!S_ISREG(ist.st_mode))
    {
      fprintf(stderr, "Error: %s is not a regular file\n", filename);
      return 1;
    }
  }

  FILE *file = fopen(filename, "r");
  if(!file)
  {
    fprintf(stderr, "Error: Cannot open file %s: %s\n",
            filename, strerror(errno));
    return 1;
  }
  line = (char *) MPIU_Malloc(2000);

  while (fgets(line, 2000, file) != NULL)
  {
    start = MPIDI_collsel_ltrim(MPIDI_collsel_rtrim(line));
    /* Skip comments and sections */
    if(*start == ';' || *start == '[' || *start == '#')
      continue;
    name = strtok(line, "=");
    if(name == NULL) continue;
    value = strtok(NULL, "=");
    if(value == NULL) continue;
    name  = MPIDI_collsel_rtrim(name);
    value = MPIDI_collsel_ltrim(value);
    /* Do not override command line values if they are set */
    if(strcmp(name, "collectives") == 0)
    {
      if(!params->collectives)
        ret = MPIDI_collsel_process_collectives(value, params);
    }
    else if(strcmp(name, "message_sizes") == 0)
    {
      if(!params->message_sizes)
        ret = MPIDI_collsel_process_msg_sizes(value, params);
    }
    else if(strcmp(name, "geometry_sizes") == 0)
    {
      if(!params->geometry_sizes)
        ret = MPIDI_collsel_process_geo_sizes(value, params);
    }
    else if(strcmp(name, "output_file") == 0)
    {
      if(!*out_file && !task_id) /* Only task 0 creates o/p file */
        ret = MPIDI_collsel_process_output_file(value, out_file);
    }
    else if(strcmp(name, "iterations") == 0)
    {
      if(params->iter == -1)
      {
        params->iter = atoi(value);
        if(params->iter <= 0 || params->iter > PAMI_TUNE_MAX_ITER)
        {
          if(!task_id)
            fprintf(stderr, "Invalid iteration count: %s in file: %s\n", value, filename);
          ret = 1;
        }
      }
    }
    else if(strcmp(name, "verbose") == 0)
    {
      if(params->verbose == -1)
        ret = MPIDI_collsel_checkvalue(name, value, filename, &params->verbose);
    }
    else if(strcmp(name, "diagnostics") == 0)
    {
      if(params->verify == -1)
        ret = MPIDI_collsel_checkvalue(name, value, filename, &params->verify);
    }
    /*else if(strcmp(name, "checkpoint") == 0)
    {
      if(params->checkpoint == -1)
        ret = MPIDI_collsel_checkvalue(name, value, filename, &params->checkpoint);
    }*/
    else
    {
      fprintf(stderr, "Invalid parameter: %s in file: %s\n", name, filename);
      ret = 1;
    }
    if(ret) break;
  }
  MPIU_Free(line);
  fclose(file);

  return ret;
}


static int MPIDI_collsel_process_arg(int argc, char *argv[], advisor_params_t *params, char ** out_file)
{
   int i,c,ret = 0;
   char *in_file = NULL;

   MPIDI_collsel_init_xfer_tables();
   params->verify = 0;

   opterr = 0;
   while ((c = getopt (argc, argv, "c:m:g:f:o:i:p::d::x::h::")) != -1)
   {
     switch (c)
     {
       case 'c':
         ret = MPIDI_collsel_process_collectives(optarg, params);
         break;
       case 'm':
         ret = MPIDI_collsel_process_msg_sizes(optarg, params);
         break;
       case 'g':
         ret = MPIDI_collsel_process_geo_sizes(optarg, params);
         break;
       case 'f':
         in_file = (char *) optarg;
         break;
       case 'o':
         if(!task_id) /* Only task 0 creates o/p file */
           ret = MPIDI_collsel_process_output_file(optarg, out_file);
         break;
       case 'i':
         params->iter = atoi(optarg);
         if(params->iter <= 0  || params->iter > PAMI_TUNE_MAX_ITER)
         {
           if(!task_id)
             fprintf(stderr, "Invalid iteration count %s\n", optarg);
           ret = 1;
         }
         break;
       case 'd':
         params->verify = 1;
         break;
       case 'p':
         params->verbose = 1;
         break;
       case 'x':
         /*params->checkpoint = 1;*/
         break;
       case 'h':
         ret = 2;
         break;
       case '?':
         if(!task_id)
         {
           if (optopt == 'c' || optopt == 'm' || optopt == 'g' ||
               optopt == 'f' || optopt == 'o' || optopt == 'i')
           {
             fprintf (stderr, "Option -%c requires an argument.\n", optopt);
           }
           else if (isprint (optopt))
           {
             fprintf (stderr, "Unknown option `-%c'.\n", optopt);
           }
           else
           {
             fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
           }
         }
         ret = 1;
         break;
       default:
         abort();
         break;
     }
     if(ret) return ret;
   }
   /* INI file is processed last as we don't wan't to override
    * command line parameters. Also any invalid value in the INI
    * file if overriden by a command line flag should not generate
    * an error
    */
   if(in_file != NULL)
   {
     ret = MPIDI_collsel_process_ini_file(in_file, params, out_file);
     if(ret) return ret;
   }

   if(!task_id)
   {
     if (optind < argc)
     {
       printf ("Non-option arguments: ");
       while (optind < argc)
         printf ("%s ", argv[optind++]);
       printf ("\n");
     }
   }

   /* If user did not specify any collectives, benchmark all */
   if(params->num_collectives == 0)
   {
     params->collectives = (pami_xfer_type_t *)MPIU_Malloc(sizeof(pami_xfer_type_t)*PAMI_XFER_COUNT);
     for(i = 0; i < PAMI_XFER_COUNT; i++)
     {
       if(i == 4 || i == 7 || i == 10 || i == 14)
         i++;
       params->collectives[params->num_collectives++] = i;
     }
   }
   /* If user did not set any of the following parameters, set default value */
   if(params->iter == -1) params->iter = 1000;
   if(params->verbose == -1) params->verbose = 0;
   if(params->verify == -1) params->verify = 0;
   if(params->checkpoint == -1) params->checkpoint = 0;
   return 0;
}


static void MPIDI_collsel_print_params(advisor_params_t *params, char *output_file)
{
  size_t i;
  printf("  Benchmark Parameters\n");
  printf("  --------------------\n");

  printf("  Collectives:\n   ");
  for(i=0; i<params->num_collectives; i++){
    printf(" %s", xfer_array_str[params->collectives[i]]);
  }
  printf("\n  Geometry sizes:\n   ");
  for(i=0; i<params->num_geometry_sizes; i++){
    printf(" %zu", params->geometry_sizes[i]);
  }
  printf("\n  Message sizes:\n   ");
  for(i=0; i<params->num_message_sizes; i++){
    printf(" %zu", params->message_sizes[i]);
  }
  printf("\n  Iterations       : %d\n", params->iter);
  printf("  Output file      : %s\n", output_file);
/*  printf("  Checkpoint mode  : %d\n", params->checkpoint); */
  printf("  Diagnostics mode : %d\n", params->verify);
  printf("  Verbose mode     : %d\n", params->verbose);
}


int MPIDI_collsel_pami_tune_parse_params(int argc, char ** argv)
{
  MPIDI_Collsel_output_file = NULL;
  num_tasks = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_NUM_TASKS).value.intval;
  task_id   = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_TASK_ID  ).value.intval;
  MPIDI_collsel_init_advisor_params(&MPIDI_Collsel_advisor_params);
  if(num_tasks < 2)
  {
    fprintf (stderr, "Error: pami_tune must be run with more than one task\n");
    fflush(stderr);
    MPIDI_collsel_print_usage();
    return PAMI_ERROR;
  }
  MPIDI_Collsel_advisor_params.procs_per_node = (size_t*)MPIU_Malloc(1 * sizeof(size_t));
  MPIDI_Collsel_advisor_params.num_procs_per_node = 1;
  MPIDI_Collsel_advisor_params.procs_per_node[0] = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_NUM_LOCAL_TASKS  ).value.intval;

  int result = MPIDI_collsel_process_arg(argc, argv, &MPIDI_Collsel_advisor_params, &MPIDI_Collsel_output_file);
  if(result)
  {
    MPIDI_collsel_print_usage();
    return PAMI_ERROR;
  }

  if(MPIDI_Collsel_output_file == NULL && !task_id)
  {
    if(MPIDI_collsel_process_output_file("pami_tune_results.xml", &MPIDI_Collsel_output_file))
    {
      return PAMI_ERROR;
    }
  }
  if(MPIDI_Collsel_advisor_params.verbose && !task_id)
    MPIDI_collsel_print_params(&MPIDI_Collsel_advisor_params, MPIDI_Collsel_output_file);

  return PAMI_SUCCESS;

}

void MPIDI_collsel_pami_tune_cleanup()
{
  MPIDI_collsel_free_advisor_params(&MPIDI_Collsel_advisor_params);
}



/**********************************************************/
/*                 CUDA Utilities                         */
/**********************************************************/
#if CUDA_AWARE_SUPPORT
int CudaMemcpy(void* dst, const void* src, size_t count, int kind)
{
  return (*pamidCudaMemcpy)(dst, src, count, kind);
}
int CudaPointerGetAttributes(struct cudaPointerAttributes* attributes, const void* ptr)
{
  return  (*pamidCudaPointerGetAttributes)(attributes, ptr);
}
const char*  CudaGetErrorString( int error)
{
  return (*pamidCudaGetErrorString)(error);
}
#endif

inline bool MPIDI_enable_cuda()
{
  bool result = false;
#if CUDA_AWARE_SUPPORT
  pamidCudaPtr = dlopen("libcudart.so", RTLD_NOW|RTLD_GLOBAL);
  if(pamidCudaPtr == NULL)
  {
    TRACE_ERR("failed to open libcudart.so error=%s\n", dlerror());
    return result;
  }
  else
  {
    pamidCudaMemcpy =  (int (*)())dlsym(pamidCudaPtr, "cudaMemcpy");
    if(pamidCudaMemcpy == NULL)
    {
      dlclose(pamidCudaPtr);
      return result;
    }
    pamidCudaPointerGetAttributes = (int (*)())dlsym(pamidCudaPtr, "cudaPointerGetAttributes");
    if(pamidCudaPointerGetAttributes == NULL)
    {
      dlclose(pamidCudaPtr);
      return result;
    }
    pamidCudaGetErrorString = (const char* (*)())dlsym(pamidCudaPtr, "cudaGetErrorString");
    if(pamidCudaGetErrorString == NULL)
    {
      dlclose(pamidCudaPtr);
      return result;
    }
    result = true;
  }
#endif
  return result;
}

inline bool MPIDI_cuda_is_device_buf(const void* ptr)
{
  bool result = false;
#if CUDA_AWARE_SUPPORT
  if(MPIDI_Process.cuda_aware_support_on)
  {
    if(ptr != MPI_IN_PLACE)
    {
      struct cudaPointerAttributes cuda_attr;
      cudaError_t e= CudaPointerGetAttributes  ( & cuda_attr, ptr);

      if (e != cudaSuccess)
          result = false;
      else if (cuda_attr.memoryType ==  cudaMemoryTypeDevice)
          result = true;
      else
          result = false;
    }
  }
#endif
  return result;
}


/**********************************************************/
/*                End CUDA Utilities                      */
/**********************************************************/

#if defined(MPID_USE_NODE_IDS)
#undef FUNCNAME
#define FUNCNAME MPID_Get_node_id
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int _g_max_node_id = -1;
int MPID_Get_node_id(MPID_Comm *comm, int rank, MPID_Node_id_t *id_p)
{
  int mpi_errno = MPI_SUCCESS;
  uint32_t node_id;
  uint32_t offset;
  uint32_t max_nodes;

  if(!PAMIX_Extensions.is_local_task.node_info)
  {
    *id_p = rank;
  }
  else
  {
    pami_result_t rc = PAMIX_Extensions.is_local_task.node_info(comm->vcr[rank]->taskid,
                                                                &node_id,&offset,&max_nodes);
    if(rc != PAMI_SUCCESS)
      *id_p = rank;
    else
      *id_p = node_id;
  }

  fn_fail:
  return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_Get_max_node_id
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Get_max_node_id(MPID_Comm *comm, MPID_Node_id_t *max_id_p)
{
  int mpi_errno = MPI_SUCCESS;
  uint32_t node_id;
  uint32_t offset;
  uint32_t max_nodes;
  if(_g_max_node_id == -1)
  {
    if(!PAMIX_Extensions.is_local_task.node_info)
      MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**notimpl");

    pami_result_t rc = PAMIX_Extensions.is_local_task.node_info(comm->vcr[0]->taskid,
                                                                &node_id,&offset,&max_nodes);
    if(rc != PAMI_SUCCESS)  MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    *max_id_p = max_nodes;
    _g_max_node_id = max_nodes;
  }
  else
    *max_id_p = _g_max_node_id;

  fn_fail:
  return mpi_errno;
}
#endif
#undef FUNCNAME
