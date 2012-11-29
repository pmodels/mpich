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

#include <sys/time.h>
#include <limits.h>
#include <nl_types.h>
#include <mpidimpl.h>
#include "mpidi_util.h"

/* Short hand for sizes */
#define ONE  (1)
#define ONEK (1<<10)
#define ONEM (1<<20)
#define ONEG (1<<30)



#if (MPIDI_PRINTENV || MPIDI_STATISTICS || MPIDI_BANNER)
MPIDI_printenv_t  *mpich_env;
extern char* mp_euilib;
char mp_euidevice[20];
extern pami_extension_t pe_extension;

void MPIDI_Set_mpich_env(int rank, int size) {
     static char polling_buf[32]="";
     int rc;

     mpich_env->this_task = rank;
     mpich_env->nprocs  = size;
     mpich_env->eager_limit=MPIDI_Process.pt2pt.limits.application.eager.remote;
     mpich_env->use_token_flow_control=MPIDI_Process.is_token_flow_control_on;
     mpich_env->mp_statistics=MPIDI_Process.mp_statistics;
     if (mpich_env->polling_interval == 0) {
            mpich_env->polling_interval = 400000;
            sprintf(polling_buf, "MP_POLLING_INTERVAL=%d",
                    mpich_env->polling_interval); /* microseconds */
            rc = putenv(polling_buf);
     }
     if (mpich_env->retransmit_interval == 0) {
            mpich_env->retransmit_interval = 400000;
            sprintf(polling_buf, "MP_POLLING_INTERVAL=%d",
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
         if (mpich_env->transport_type == IS_US)  {  /* default for IP is 10000 loops */
             mpich_env->retransmit_interval = 4000000;
         } else {
             mpich_env->retransmit_interval = 10000;
         }
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
                  if (gatherer[first].x == 1) {                                      \
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
        int  errflag;

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
#ifdef __32BIT__
        sender.mode_64 = 0;
#else
        sender.mode_64 = 1;
#endif
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

        #ifdef _DEBUG
        printf("task_count = %d\n", task_count);
        printf("calling _mpi_gather(%p,%d,%d,%p,%d,%d,%d,%d,%p,%d)\n",
            &sender,sizeof(MPIDI_printenv_t),MPI_BYTE,gatherer,sizeof(MPIDI_printenv_t),MPI_BYTE,
            0, MPI_COMM_WORLD,NULL,0);
        fflush(stdout);
        #endif

        mpi_errno = MPI_SUCCESS;
        MPID_Comm_get_ptr( comm, comm_ptr );
        mpi_errno = MPIR_Gather_impl(&sender, sizeof(MPIDI_printenv_t), MPI_BYTE, gatherer,
                                    sizeof(MPIDI_printenv_t),MPI_BYTE, 0,comm_ptr,
                                    (int *) &errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            errflag = TRUE;
        }

        #ifdef _DEBUG
        printf("returned from _mpi_gather\n");
        fflush(stdout);
        #endif

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
           if (mpid_statp) MPIU_Free(mpid_statp);
       }
    if (MPIDI_Process.mp_printenv) {
        if (mpich_env)  MPIU_Free(mpich_env);
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


