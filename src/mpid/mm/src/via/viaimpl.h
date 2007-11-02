/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef VIAIMPL_H
#define VIAIMPL_H

#include "mm_via.h"
#include "vipl.h"

/* Definitions */
#define VITIMEOUT                   10000           /* 10 seconds */
#define ADDR_LEN                    6
#define DESIRED_PACKET_LENGTH       64*1024
#define INITIAL_NUM_CQ_ENTRIES      64
#define CQ_ENTRIES_INCREMENT        32

#define NUM_DESCRIPTORS             30
#define NUM_RECV_DESCRIPTORS        NUM_DESCRIPTORS + 2
#define NUM_RECVS_PER_ACK           NUM_DESCRIPTORS / 2

#define MPIDI_VIA_EAGER_LIMIT       20000

#define MPIDI_VIA_REQUEST_TO_SEND_PACKET    1
#define MPIDI_VIA_OK_TO_SEND_PACKET         2
#define MPIDI_VIA_SEND_ACK_PACKET           3

#define VI_KEEP_CONNECTION      1
#define VI_REJECT_CONNECTION    0

/* These numbers are experimentally generated.
 * I don't know how to generate then dynamically.
 */
#define VI_STREAM_MIN   0x1000
#define VI_STREAM_MIN_N 12
#define VI_STREAM_MAX   0x400000
#define VI_STREAM_MAX_N 22
/*
#define VI_BANDWIDTH    600.0*1048576.0
#define VI_LATENCY      0.000022
#define VI_MULTIPLIER   10.0
*/
#define VI_BANDWIDTH    800.0*1048576.0
#define VI_LATENCY      0.000002
#define VI_MULTIPLIER   2.75

typedef struct VIA_PerProcess {
    MPID_Thread_lock_t lock;
                   int nViaEagerLimit;
                  char pszNicBaseName[100]; /* base name used to generate nic names - in case there are multiple nics per host */
        VIP_NIC_HANDLE hViNic;
         VIP_CQ_HANDLE hViCQ;
             VIP_ULONG viMTU;
         unsigned char ViDescriminator[16];
                   int nViDescriminator_len;
                   int nNumCQEntries;
     VIP_VI_ATTRIBUTES default_vi_attribs;
                  char pszDescriminatorPrefix[16];
} VIA_PerProcess;

extern VIA_PerProcess VIA_Process;

/* Function prototypes */
/*
int VI_Write(VI_Info *pInfo, char *pBuffer, unsigned int nLength, unsigned int nPacketSize);
int VI_SendPacket(VI_Info *pInfo, void *pBuffer, unsigned int length);
int VI_SendEagerPacket(VI_Info *pInfo, MPID_Request *pRequest);
int VI_SendDataPacket(VI_Info *pInfo, VI_Element *pElement);
int VI_SendImmediateData(VI_Info *pInfo, unsigned int immediatedata);
int VI_Read_data(VI_Info *pInfo, char *pBuffer, int nLength);
int VI_Read_packet(VI_Info *pInfo, MPID_Packet *pPacket);
int VI_Get_packet(VI_Info *pInfo, MPID_Packet *pPacket, char **ppData, VIP_DESCRIPTOR **ppRecvDesc);
int VI_Free_packet(VI_Info *pInfo, VIP_DESCRIPTOR *pRecvDesc);
int VI_Connect(int rank);
int VI_Setup_listener( void );
int VI_Shutdown_listener( void );

void VI_ErrorCallbackFunction(void *ctx, VIP_ERROR_DESCRIPTOR *d);
char * VI_DescriptorError(int r, VIP_DESCRIPTOR *d);
int AssertSuccess(int status, char *msg, VIP_DESCRIPTOR *desc);
VIP_DESCRIPTOR * get_descriptors(VIP_NIC_HANDLE nic, int num, int buflen, VIP_MEM_HANDLE *mh, void **ptr);
int CloseVi(VI_Info *pInfo);
*/

#endif
