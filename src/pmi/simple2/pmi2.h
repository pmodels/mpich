/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI2_H_INCLUDED
#define PMI2_H_INCLUDED

#define PMI_MAX_KEYLEN       64
#define PMI_MAX_VALLEN       1024

typedef struct PMI_Connect_comm {
    int (*read)( void *buf, int maxlen, void *ctx );
    int (*write)( const void *buf, int len, void *ctx );
    void *ctx;
    int  isMaster;
} PMI_Connect_comm_t;

struct MPID_Info;

int PMI2_Init(int *spawned, int *size, int *rank, int *appnum);
int PMI2_Finalize(void);
int PMI2_Initialized(void);
int PMI2_Abort(int flag, const char msg[]);
int PMI2_Job_Spawn(int count, const char * cmds[], const char ** argvs[],
                  const int maxprocs[], 
                  const int info_keyval_sizes[],
                  const struct MPID_Info *info_keyval_vectors[],
                  int preput_keyval_size,
                  const struct MPID_Info *preput_keyval_vector[],
                  char jobId[], int jobIdSize,      
                  int errors[]);
int PMI2_Job_GetId(char jobid[], int jobid_size);
int PMI2_Job_Connect(const char jobid[], PMI_Connect_comm_t *conn);
int PMI2_Job_Disconnect(const char jobid[]);
int PMI2_KVS_Put(const char key[], const char value[]);
int PMI2_KVS_Fence(void);
int PMI2_KVS_Get(const char *jobid, const char key[], char value [], int maxValue, int *valLen);
int PMI2_Info_GetNodeAttr(const char name[], char value[], int valuelen, int *flag, int waitfor);
int PMI2_Info_PutNodeAttr(const char name[], const char value[]);
int PMI2_Info_GetJobAttr(const char name[], char value[], int valuelen, int *flag);
int PMI2_Nameserv_publish(const char service_name[], const struct MPID_Info *info_ptr, const char port[]);
int PMI2_Nameserv_lookup(const char service_name[], const struct MPID_Info *info_ptr,
                        char port[], int portLen);
int PMI2_Nameserv_unpublish(const char service_name[], 
                           const struct MPID_Info *info_ptr);




#endif /* PMI2_H_INCLUDED */
