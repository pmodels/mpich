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
 * \file include/mpidi_util.h
 * \brief ???
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifndef __include_mpidi_util_h__
#define __include_mpidi_util_h__


#if (MPIDI_STATISTICS || MPIDI_PRINTENV || MPIDI_BANNER)
#include <pami_ext_pe.h>
#include <sys/time.h>
#include <sys/param.h>

#ifndef MAXHOSTNAMELEN
#ifdef LINUX
#define MAXHOSTNAMELEN 64
#else
#define MAXHOSTNAMELEN 256
#endif
#endif

extern int MPIDI_atoi(char* , unsigned int* );
extern int MPIDI_Banner(char *);
typedef enum {IS_IP, IS_US} MPIDI_TransportType;
typedef enum {NO_STRIPING=0, IS_PACKET_STRIPING} MPIDI_StripingType;

typedef struct {
        char version[128];   /* M19 */
        char hostname[MAXHOSTNAMELEN+1];
/* Start job info */
        unsigned int partition;
        int pid;
        int this_task;
        int nprocs;
        int  mp_statistics;
        char nnodes[8];
        char tasks_node[8];
        char euilib[8];
        char window_id[128];
        char network_id[32];
        char adapter_type[32];
        char device[8];
        char protocol[16];
        char libpath[MAXPATHLEN];         /* size=BUFSIZE (pm_common.c) */
        char directory[FILENAME_MAX+1];   /* size = 256 */
        int mode_64;
        int threaded;
        int copySendBufSize:16;
        char thread_scope[8];
        char cpu_use[10];
        char adapter_use[10];
        char clock_source[8];
        char priority[24];
        MPIDI_TransportType transport_type;
        MPIDI_StripingType strip_type;
        int   use_rdma;
        char *network_string;
        int max_pkts_out;
        long rdma_min_msg_size;
        int timeout;
        int interrupts;
        uint  polling_interval;
        unsigned long buffer_mem;
        long long buffer_mem_max;
        int eager_limit;
        int use_token_flow_control;
        char wait_mode[8];
        int use_shmem;
        uint retransmit_interval;
        int shmem_pt2pt;
        int shared_mem_pg_size;
        char mem_affinity[8];
        int single_thread;
        char checkpoint[8];       /*  NA    */
        char gang_scheduler[10];  /*  NA    */
        int instances;
        char striping_type[40];
        int ack_thresh;
        int recv_fifo_sz;
        int max_pkt_size;
        int rexmit_buf_size;
        int rexmit_buf_cnt;
        int max_atom_size;
        char bulk_min_msg_size[16];
        char use_bulk_xfer[8];
        char user_rdma_avail[16];
        char user_rdma_total[16];
        char debug_notimeout[8];
        char develop[16];
        char stdinmode[12];
        char stdoutmode[12];
        int statistics;
        int service_variables;
        int rc_max_qp;               /* M32 */
        char rc_qp_use_lmc[8];       /* M32 */
        char rc_qp_use_lru[8];       /* M32 */
        char rc_qp_init_setup[8];    /* M32 */
} MPIDI_printenv_t;



typedef struct {
    long sends;              /* Count of sends initiated       */
    long sendsComplete;      /* Count of sends completed (msg sent)  */
    long sendWaitsComplete;  /* Count of send waits completed
                                        (blocking & nonblocking) */
    long recvs;              /* Count of recvs initiated */
    long recvWaitsComplete;  /* Count of recv waits complete */
    long earlyArrivals;      /* Count of msgs received for which
                                        no receive was posted    */
    long earlyArrivalsMatched; /* Count of early arrivals for which
                                          a posted receive has been found  */
    long lateArrivals;       /* Count of msgs received for which a recv
                                        was posted                      */
    long unorderedMsgs;      /* Total number of out of order msgs  */
    long buffer_mem_hwmark;
    long pamid_reserve_10;
    long pamid_reserve_9;
    long pamid_reserve_8;
    long pamid_reserve_7;
    long pamid_reserve_6;
    long pamid_reserve_5;
    long pamid_reserve_4;
    long pamid_reserve_3;
    long pamid_reserve_2;
    long pamid_reserve_1;
} MPIX_stats_t;

extern MPIDI_printenv_t *mpich_env;
extern MPIX_stats_t *mpid_statp;
extern int   prtStat;
extern int   prtEnv;
extern void set_mpich_env(int *,int*);
extern void MPIDI_open_pe_extension();
extern void MPIDI_close_pe_extension();
extern MPIDI_Statistics_write(FILE *);

#if CUDA_AWARE_SUPPORT
int (*pamidCudaMemcpy)( void* dst, const void* src, size_t count, int kind );
int (*pamidCudaPointerGetAttributes)( struct cudaPointerAttributes* attributes, const void* ptr );
const char* (*pamidCudaGetErrorString)( int error );
extern void * pamidCudaPtr;
#endif
/*************************************************************
 *    MPIDI_STATISTICS
 *************************************************************/
/* It is not necessary to do fetch_and_and.  Statistics */
/* should be taken when holding a lock.                 */
#ifdef MPIDI_STATISTICS
 #ifdef _AIX_
    #ifndef __64BIT__
      #define MPID_NSTAT(cmd) if (prtStat) fetch_and_add(&(cmd),1);
    #else
      #define MPID_NSTAT(cmd) if (prtStat) fetch_and_addlp(&(cmd),1);
    #endif
 #else       /* Linux   */
    #define MPID_NSTAT(cmd) if (prtStat) (cmd)++;
 #endif      /* End Linux */
#else     /* MPIDI_STATISTICS not set*/
   #define MPID_NSTAT(cmd)
#endif    /* MPIDI_STATISTICS    */
#endif    /* MPIDI_PRINTENV || MPIDI_STATISTICS    */

#endif /* __include_mpidi_util_h__  */
