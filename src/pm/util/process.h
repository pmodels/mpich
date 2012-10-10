/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PROCESS_H_INCLUDED
#define PROCESS_H_INCLUDED

#include <sys/types.h>

/*
Data structures and routines for managing processs

The data structures for managing processes following the hierarchy implied
by MPI-2 design, particularly the ":" syntax of mpiexec and the
arguments to MPI_Comm_spawn_multiple.

From the top, we have:

ProcessUniverse - All processes, consisting of a list of ProcessWorld
    ProcessWorld - All processes in an MPI_COMM_WORLD, containing an
                   list of apps (corresponding to the MPI_APPNUM
                   attribute)
        ProcessApp - A group of processes sharing the same executable
                     command line, and other parameters (such as
                     working dir), and an array of process descriptions
            ProcessState - Information for a specific process,
                     including host, pid, MPI rank, and state
                 ProcessExitStatus - exit status
The I/O for processes is handled separately.  I/O handlers for each fd
include an "extra-data" pointer that can store pointers to the process
structures as necessary.
 */

/* ProcessSoftSpec is the "soft" specification of desired processor counts.
   Format is in pseudo BNF:
   soft -> element[,element]
   element -> number | range
   range   -> number:number[:number]

   These are stored as 3 element tuples containing first:last:stride.
   A single number is first:first:1, e.g., 17 is represented as 17:17:1.
*/
typedef struct {
    int nelm;                        /* Number of tuples */
    int (*tuples)[3];                /* The tuples (allocated) */
} ProcessSoftSpec;

/* Record the return value from each process */
typedef enum { EXIT_NOTYET,     /* Not exited */
               EXIT_NORMAL,     /* Normal exit (possibly with nonzero status)*/
	       EXIT_SIGNALLED,  /* Process died on an uncaught signal (e.g., 
			           SIGSEGV) */
	       EXIT_NOFINALIZE, /* Process exited without calling finalize */
	       EXIT_ABORTED,    /* Process killed by mpiexec after a 
	                           PMI abort */
	       EXIT_KILLED      /* Process was killed by mpiexec */ 
             } ProcessExitState_t;

typedef enum { PROCESS_UNINITIALIZED=-1, /* Before process created */
	       PROCESS_ALIVE,          /* Process is (expected to be) alive */
	       PROCESS_COMMUNICATING,  /* Process is alive and using PMI */
	       PROCESS_FINALIZED,      /* Process is alive but has indicated 
					  that it has called PMI_Finalize */
	       /*PROCESS_EXITING, */       /* Process expected to exit */
	       PROCESS_GONE            /* Process has exited and its status 
					  collected */
} ProcessStatus_t;

typedef struct {
    ProcessExitState_t  exitReason;       /* how/why did the process exit */
    int                 exitSig;          /* exit signal, if any */
    int                 exitStatus;       /* exit statue */
    int                 exitOrder;        /* indicates order in which processes
			   	 	     exited */
} ProcessExitStatus;

typedef struct ProcessState {
    const char        *hostname;         /* Host for process */
    int               wRank;             /* Rank in COMM_WORLD */
    int               id;                /* An integer used to identify
					    process entries */
    pid_t             pid;               /* pid for process */
    int               initWithEnv;       /* true if PMI_FD, PMI_RANK etc.
					    passed to process to initialize
					    PMI connection */
    ProcessStatus_t   status;            /* what state the process is in */
    ProcessExitStatus exitStatus;        /* Exit status */
    struct ProcessApp *app;              /* Pointer to "parent" app */
} ProcessState;

typedef struct ProcessApp {
    int           myAppNum;          /* Appnum of this group */
    const char    *exename;          /* Executable to run */
    const char    *arch;             /* Architecture type */
    const char    *path;             /* Search path for executables */
    const char    *wdir;             /* Working directory */
    const char    *hostname;         /* Default host (can be overridded
					by each process in an App) */
    const char    **args;            /* Pointer into the array of args */
    int            nArgs;            /* Number of args (list is *not* null
					terminated) */
    struct EnvInfo *env;             /* Pointer to structure providing
					information on the process
					environment variables for this app */
    ProcessSoftSpec soft;            /* "soft" spec, if any */
    int            nProcess;         /* Number of processes in this app */
    ProcessState   *pState;          /* Array of process states */
    struct ProcessWorld   *pWorld;   /* World containing this app */

    struct ProcessApp *nextApp;      /* Next App */
} ProcessApp;

typedef struct ProcessWorld {
    int        nApps;                /* Number of Apps in this world */
    int        nProcess;             /* Number of processes in this world */
    int        worldNum;             /* Number of this world; initial
					world is 0 */
    struct EnvInfo *genv;            /* Pointer to structure providing
					information on the process
					environment variables for this world */
    ProcessApp *apps;                /* Array of Apps */
    struct ProcessWorld *nextWorld;
} ProcessWorld;

typedef struct ProcessUniverse {
    ProcessWorld *worlds;
    int          nWorlds;            /* Number of worlds */
    int          size;               /* Universe size */
    int          nLive;              /* Number of live processes */
    int          (*OnNone)(void);    /* Routine to call when nLive == 0 */
    int          timeout;            /* Timeout in seconds (-1 for none) */
    int          giveExitInfo;       /* True if info on error exit 
					should be printed to stderr */
    char         *portName;          /* Contact name for a port for PMI,
				        if used (null otherwise) 
				        (valid port number, as a string) */
    char         *portRange;         /* Character string defining a range
				        of valid ports */
    int          fromSingleton;      /* Set to true if this 
					universe was created from a singleton
					init */
    int          singletonPort;      /* Port number for singleton */
    char         *singletonIfname;   /* Network interface name of the 
					singleton's port (usually hostname) */
    char         *portKey;           /* Option key for confirming the
					singleton init port */
    pid_t        singletonPID;       /* PID of singleton inti process */
} ProcessUniverse;

/* There is only one universe */
extern ProcessUniverse pUniv;

/* Function prototypes */    
int MPIE_ForkProcesses( ProcessWorld *, char *[], 
			int (*)(void*,ProcessState*), void *,
			int (*)(void*,void*,ProcessState*), void *,
			int (*)(void*,void*,ProcessState*), void * );
int MPIE_ExecProgram( ProcessState *, char *[] );
ProcessState *MPIE_FindProcessByPid( pid_t );
void MPIE_ProcessInit( void );
void MPIE_SetupSigChld( void );
int MPIE_ProcessGetExitStatus( int * );
void MPIE_ProcessSetExitStatus( ProcessState *, int );
int MPIE_InitWorldWithSoft( ProcessWorld *, int );
void MPIE_PrintFailureReasons( FILE * );
int MPIE_WaitForProcesses( ProcessUniverse *, int );
int MPIE_OnAbend( ProcessUniverse * );

int MPIE_SetupSingleton( ProcessUniverse * );

int MPIE_HasAbended(void);
int MPIE_SignalWorld( ProcessWorld *, int );
int MPIE_KillWorld( ProcessWorld * );
int MPIE_KillUniverse( ProcessUniverse * );

int MPIE_ForwardSignal( int );
int MPIE_ForwardCommonSignals( void );
void MPIE_IgnoreSigPipe( void );

/* Currently, parse soft spec is in cmnargs */
int MPIE_ParseSoftspec( const char *, ProcessSoftSpec * );

/* This routine may be called to make information available for a debugger */
int MPIE_InitForDebugger( ProcessWorld * );
int MPIE_FreeFromDebugger( void );
int MPIE_PrintDebuggerInfo( FILE * );

/* Set the processor affinity to the processor with the given rank */
int MPIE_SetProcessorAffinity( int, int );

#endif
