/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMISERV_H_INCLUDED
#define PMISERV_H_INCLUDED

/*
 * This file contains the definitions and prototypes exported by the 
 * pmi server package
 */

#define MAX_READLINE 1024
/* 
   The basic item is the PMIProcess, which contains the fd used to 
   communicate with the PMI client code, a pointer to the PMI Group 
   to which this process belongs, and the process state (defined in 
   process.h)
*/
typedef struct PMIProcess {
    int                  fd;             /* fd to client */
    char                 readBuf[MAX_READLINE];
    char                *nextChar, *endChar;
 
    struct PMIGroup     *group;          /* PMI group to which this process 
					    belongs */
    struct ProcessState *pState;         /* ProcessState */
    /* The following are used to hold data used to handle
       spawn and spawn multiple commands */
    struct ProcessApp   *spawnApp, 
                        *spawnAppTail;  /* In progress spawn multiple 
					   data */
    struct ProcessWorld *spawnWorld;    /* In progress spawn multiple 
					   new world */
    struct PMIKVSpace   *spawnKVS;      /* In progres KV space for new world */
} PMIProcess;

typedef struct PMIGroup {
    int              nProcess;    /* Number of PMI processes in this group */
    int              groupID;     /* Numeric id of group */
    int              nInBarrier;  /* Number of group members currently
				     waiting in the PMIBarrier */
    struct PMIKVSpace   *kvs;     /* KVSpace for this group of processes */
    PMIProcess      **pmiProcess; /* Array of pointers to PMIProcess'es
				     in this group */
    struct PMIGroup *nextGroup;   /* Pointer to next group structure */
} PMIGroup;

/* The key-value space is described by a pairs of char pointers and a 
   header that provides the name of the space and links all of the spaces 
   together */
/* The PMI specification requires that the server be able to inform
   the client of the maximum key and value size that is supported.  For 
   simplicity, we define each KV pair with fixed-length entries of
   the maximum size */
/* FIXME: We need to specify a minimum length for both key and value.
 * The "business cards" used by the ch3 channel implementations for 
 * sockets and for sockets+shared memory can exceed 128 characters.
 */
#define MAXKEYLEN    64		/* max length of key in keyval space */
#define MAXVALLEN   256		/* max length of value in keyval space */
#define MAXNAMELEN  256         /* max length of various names */
#define MAXKVSNAME  MAXNAMELEN  /* max length of a kvsname */
typedef struct PMIKVPair {
    char key[MAXKEYLEN];
    char val[MAXVALLEN];
    struct PMIKVPair *nextPair;
} PMIKVPair;

typedef struct PMIKVSpace {
    const char        kvsname[MAXNAMELEN];   /* Name of this kvs space */
    PMIKVPair         *pairs;     /* Pointer to the pairs in this space */
    PMIKVPair         *lastByIdx; /* Pointer to the element corresponding
				     to the index lastIdx (improves
				     support for getbyidx method) */
    int               lastIdx;
    struct PMIKVSpace *nextKVS;   /* Pointer to the next KVS */
} PMIKVSpace;

/* 
   The master PMI structure contains the "shared" objects:
   groups and key-value spaces (kvs)
*/
typedef struct PMIMaster {
    int        nGroups;      /* Number of groups allocated (non-decreasing) */
    PMIGroup   *groups;      /* Pointer to allocated groups */
    PMIKVSpace *kvSpaces;    /* Pointer to allocated KV spaces */
} PMIMaster;

typedef struct {
    int        fdpair[2];    /* fd's used to communicate between the
				client and the server */
    char       *portName;    /* Optional portname if the fd is not provided
			        at startup */
    ProcessWorld *pWorld;    /* World to which this setup applies */
} PMISetup;

/* version to check with client PMI library */
#define PMI_VERSION    1
#define PMI_SUBVERSION 1

int PMIServInit( int (*)(ProcessWorld *, void*), void * );
PMIProcess *PMISetupNewProcess( int, ProcessState * );
int PMISetupSockets( int, PMISetup * );
int PMISetupInClient( int, PMISetup * );
int PMISetupFinishInServer( int, PMISetup *, ProcessState * );
int PMISetupNewGroup( int, PMIKVSpace * );
int PMI_InitSingletonConnection( int, PMIProcess * );
int PMIServHandleInput( int, int, void * );

/* Setup for initialization with a port */
void PMI_Init_remote_proc( int, PMIProcess * );
int PMI_Init_port_connection( int );
int PMIServEndPort( void );
int PMIServGetPort( int *, char *, int );

/* pmiport.c routines for working with host:ports*/
int PMIServAcceptFromPort( int, int, void * );
int PMIServSetupPort( ProcessUniverse *, char *, int );
int MPIE_GetMyHostName( char myname[], int );
int MPIE_ConnectToPort( char *hostname, int portnum );

int PMISetDebug( int );

/* FIXME: This is a temporary declaration (non-conforming name).  Who added 
   this, and why? */
void pmix_preput( const char *, const char * );

/* Should the following be an internal-only routine? */
int PMIGetCommand( char *cmd, int cmdlen );

/* define PMIWriteLine to use replace the routines that read and write to
   the PMI socket */
#ifndef PMIWriteLine
int PMIWriteLine( int, const char * );
int PMIReadLine( int, char *, int );
#endif

#endif
