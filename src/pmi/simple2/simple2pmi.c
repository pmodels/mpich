/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Local types */

/* Parse commands are in this structure.  Fields in this structure are 
   dynamically allocated as necessary */
typedef struct {
    char *key;
    char *value;
    int   valueLen;  /* Length of a value (values may contain nulls, so
			we need this) */
    int   isCopy;    /* The value is a copy (and will need to be freed) 
			if this is true, otherwise, 
			it is a null-terminated string in the original
			buffer */
} PMI_Keyvalpair;

typedef struct {
    int            nPairs;       /* Number of key=value pairs */
    char           *command;   /* Overall command buffer */
    PMI_Keyvalpair **pairs;  /* Array of pointers to pairs */
} PMI_Command;

/* FIXME: This is a copy of the MPID_Info type in mpiimpl.h .  We should
   use a common header.  Note that few routines need info, so this
   could be in a separate header file, and in fact could be within the
   info module. */
typedef struct MPID_Info {
    int                handle;
    volatile int       ref_count;  /* FIXME: ref_count isn't needed by Info 
				      objects, but MPIU_Info_free does not 
				      work correctly unless MPID_Info and 
				      MPIU_Handle_common have the next 
				      pointer in the same location */
    struct MPID_Info   *next;
    char               *key;
    char               *value;
} MPID_Info;

/* These are the items that will be enqueued */
#define MAX_OPID 32
typedef struct PMI_Opitem {
    volatile int flag;              /* Used to signal completion */
    char opid[MAX_OPID];            /* id of request */
    struct PMI_Opitem *next, *prev; /* links to neighbors in queue */
    char   *Command;                /* String buffer corresponding to
				       opid (i.e., the response string 
				       for that opid) */
} PMI_Opitem;

typedef struct PMI_Connect_comm {
    int (*read)( void *buf, int maxlen, void *ctx );
    int (*write)( const void *buf, int len, void *ctx );
    void *ctx;
    int  isMaster;
} PMI_Connect_comm_t;

static PMI_Opitem *itemQueueHead = 0, *itemQueueTail = 0;

/* ------------------------------------------------------------------------- */
/* PMI API Routines */
/* ------------------------------------------------------------------------- */
int PMI_Init( int *spawned, int *size, int *rank, int *appnum )
{
}

int PMI_Finalize(void)
{
}

int PMI_Initialized(void)
{
}

int PMI_Abort( int flag, const char msg[] )
{
}

int PMI_Job_Spawn( int count, const char * cmds[], const char ** argvs[],
                   const int maxprocs[], 
                   const int info_keyval_sizes[],
                   const MPID_Info *info_keyval_vectors[],
                   int preput_keyval_size,
                   const MPID_Info *preput_keyval_vector[],
                   char jobId[], int jobIdSize,      
                   int errors[])
{
}

int PMI_Job_GetId( char jobId[], int jobIdSize )
{
}

int PMI_Job_Connect( const char jobId[], PMI_Connect_comm_t *conn )
{
}

int PMI_Job_Disconnect( const char jobId[] )
{
}

int PMI_KVS_Put( const char key[], const char value[] )
{
}

int PMI_KVS_Fence( void )
{
}

int PMI_KVS_Get( const char *jobId, const char key[], char value [], 
		 int maxValue, int *valLen )
{
}

int PMI_Info_GetNodeAttr( const char name[], char value[], int valueLen, 
			  int *flag, int waitFor )
{
}

int PMI_Info_PutNodeAttr( const char name[], const char value[] )
{
}

int PMI_Info_GetJobAttr( const char name[], char value[], int valueLen, 
			 int *flag )
{
}

int PMI_Nameserv_publish( const char service_name[], const MPID_Info *info_ptr,
			  const char port[] )
{
}

int PMI_Nameserv_lookup( const char service_name[], const MPID_Info *info_ptr,
			 char port[], int portLen )
{
}

int PMI_Nameserv_unpublish( const char service_name[], 
			    const MPID_Info *info_ptr )
{
}

/* ------------------------------------------------------------------------- */
/* Service routines */
/*
 * PMIi_ReadCommand - Reads an entire command from the PMI socket.  This
 * routine blocks the thread until the command is read.
 *
 * PMIi_WriteSimpleCommand - Write a simple command to the PMI socket; this 
 * allows printf - style arguments.  This blocks the thread until the buffer
 * has been written (for fault-tolerance, we may want to keep it around 
 * in case of PMI failure).
 *
 * PMIi_WaitFor - Wait for a particular PMI command request to complete.  
 * In a multithreaded environment, this may 
 */
/* ------------------------------------------------------------------------- */

/* */
/* Note that we fill in the fields in a command that is provided.
   We may want to share these routines with the PMI version 2 server */
int PMIi_ReadCommand( int fd, PMI_Command *cmd )
{
    /* Read the command length.  Allocate a buffer of that size +1.
       Remainder of line is 
       cmd=name;opid=string;...
       For each key=value pair, identify the key and the value part,
       using a null-terminated pointer into the command string *unless*
       the value contains a null, in which case, allocate and duplicate 
       the value string.
    */
    
}
int PMIi_WriteSimpleCommand( int fd, const char cmd[], const char opid[], ... )
{
    /* Form the command string */

    /* Compute the length */

    /* Write it out, checking for EINTR and errors */
}

