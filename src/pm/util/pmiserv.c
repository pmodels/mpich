/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This is a simple PMI server implementation.  This file implements
 * the PMI calls, including the PMI key value spaces.  This implements the
 * "server" end of the interface defined in mpich/src/pmi/simple .
 */

#include "mpichconf.h"
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "pmutil.h"
#include "process.h"
#include "cmnargs.h"
#include "ioloop.h"
#include "pmiserv.h"
#include "env.h"        /* For MPIE_Putenv */

/* We need socket to create a socket pair */
#include <sys/socket.h>

/* ??? */
#include "simple_pmiutil.h"

#ifdef HAVE_SNPRINTF
#define MPL_snprintf snprintf
#ifdef NEEDS_SNPRINTF_DECL
int snprintf(char *, size_t, const char *, ...);
#endif
#endif

/* isascii is an extension, so define it if it isn't defined */
#ifndef isascii
#define isascii(c) (((c)&~0x7f)==0)
#endif

/* These need to be imported from the pmiclient */
#define MAXPMICMD   256         /* max length of a PMI command */

/* Sizes of info keys and values (should match MPI versions in mpi.h) */
#define PMI_MAX_INFO_KEY       256
#define PMI_MAX_INFO_VAL      1025

/* There is only a single PMI master, so we allocate it here */
static PMIMaster pmimaster = { 0, 0, 0 };
/* Allow the user to register a routine to be used for the PMI spawn 
   command */
static int (*userSpawner)( ProcessWorld *, void *) = 0;
static void *userSpawnerData = 0;
static int pmidebug = 0;

/* Functions that handle PMI requests */
static int fPMI_Handle_finalize( PMIProcess * );
static int fPMI_Handle_abort( PMIProcess * );
static int fPMI_Handle_barrier( PMIProcess * );
static int fPMI_Handle_create_kvs( PMIProcess * );
static int fPMI_Handle_destroy_kvs( PMIProcess * );
static int fPMI_Handle_put( PMIProcess * );
static int fPMI_Handle_get( PMIProcess * );
static int fPMI_Handle_get_my_kvsname( PMIProcess * );
static int fPMI_Handle_init( PMIProcess * );
static int fPMI_Handle_get_maxes( PMIProcess * );
static int fPMI_Handle_getbyidx( PMIProcess * );
static int fPMI_Handle_init_port( PMIProcess * );
static int fPMI_Handle_spawn( PMIProcess * );
static int fPMI_Handle_get_universe_size( PMIProcess * );
static int fPMI_Handle_get_appnum( PMIProcess * );

static PMIKVSpace *fPMIKVSAllocate( void );

static int fPMIInfoKey( ProcessApp *, const char [], const char [] );

int PMIServHandleInput( int, int, void * );

static int PMIUBufferedReadLine( PMIProcess *, char *, int );

/*
 * All PMI commands are handled by calling a routine that is associated with
 * the command.  New PMI commands can be added by updating this table.
 */
typedef struct {
    const char *cmdName;
    int (*handler)( PMIProcess * );
} PMICmdMap;

static PMICmdMap pmiCommands[] = { 
    { "barrier_in",     fPMI_Handle_barrier },
    { "finalize",       fPMI_Handle_finalize },
    { "abort",          fPMI_Handle_abort },
    { "create_kvs",     fPMI_Handle_create_kvs },
    { "destroy_kvs",    fPMI_Handle_destroy_kvs }, 
    { "put",            fPMI_Handle_put },
    { "get",            fPMI_Handle_get },
    { "get_my_kvsname", fPMI_Handle_get_my_kvsname },
    { "init",           fPMI_Handle_init },
    { "get_maxes",      fPMI_Handle_get_maxes },
    { "getbyidx",       fPMI_Handle_getbyidx },
    { "initack",        fPMI_Handle_init_port },
    { "spawn",          fPMI_Handle_spawn },
    { "get_universe_size", fPMI_Handle_get_universe_size },
    { "get_appnum",     fPMI_Handle_get_appnum },
    { "\0",             0 },                     /* Sentinal for end of list */
};

/* ------------------------------------------------------------------------- */

/*
 * Create a socket fd and setup the handler on that fd.
 *
 * You must also call 
 *    PMISetupInClient (in the child process)
 * and
 *    PMISetupFinishInServer (in the originating process, also called the 
 *                            parent)
 * You must also pass those routines the same value for usePort.
 * If you use a port, call PMIServSetupPort to get the port and set the
 * portName field in PMISetup.
 */
int PMISetupSockets( int usePort, PMISetup *pmiinfo )
{
    if (usePort == 0) {
	socketpair( AF_UNIX, SOCK_STREAM, 0, pmiinfo->fdpair );
    }
    else {
	/* If we are using a port, the connection is set up only
	   after the process is created */
	pmiinfo->fdpair[0] = -1;
	pmiinfo->fdpair[1] = -1;
	/* Check for a non-null portName */
	if (!pmiinfo->portName || !pmiinfo->portName[0]) return 1;
    }
    return 0;
}

/* 
 * This is the client side of the PMIserver setup.  It communicates to the
 * client the information needed to connect to the server (currently the
 * FD of a pre-existing socket).
 *
 * The env_pmi_fd and port must be static because putenv doesn't make a copy
 * of them.  It is ok to use static variable since this is called only within
 * the client; this routine will be called only once (in the forked process, 
 * before the exec).
 *
 * Another wrinkle is that in order to support -(g)envnone (no environment
 * variables in context of created process), we need to add the environment
 * variables to the ones set *after* environment variables are removed, rather
 * than using putenv.
 */
int PMISetupInClient( int usePort, PMISetup *pmiinfo )
{
    static char env_pmi_fd[100];
    static char env_pmi_port[1024];

    if (usePort == 0) {
	close( pmiinfo->fdpair[0] );
	MPL_snprintf( env_pmi_fd, sizeof(env_pmi_fd), "PMI_FD=%d" , 
		       pmiinfo->fdpair[1] );
	if (MPIE_Putenv( pmiinfo->pWorld, env_pmi_fd )) {
	    MPL_internal_error_printf( "Could not set environment PMI_FD" );
	    return 1;
	}
    }
    else {
	/* We must communicate the port name to the process */
	if (pmiinfo->portName) {
	    MPL_snprintf( env_pmi_port, sizeof(env_pmi_port), "PMI_PORT=%s",
			   pmiinfo->portName );
	    if (MPIE_Putenv( pmiinfo->pWorld, env_pmi_port )) {
		MPL_internal_error_printf( "Could not set environment PMI_PORT" );
		perror( "Reason: " );
		return 1;
	    }
	}
	else {
	    MPL_internal_error_printf( "Required portname was not defined\n" );
	    return 1;
	}
	
    }
    /* Indicate that this is a spawned process */
    /* MPIE_Putenv( pmiinfo->pWorld, "PMI_SPAWNED=1" ); */
    return 0;
}

/* Finish setting up the server end of the PMI interface */
int PMISetupFinishInServer( int usePort, 
			    PMISetup *pmiinfo, ProcessState *pState )
{
    if (usePort == 0) {
	PMIProcess *pmiprocess;

	/* Close the other end of the socket pair */
	close( pmiinfo->fdpair[1] );
	
	/* We must initialize this process in the list of PMI processes. We
	   pass the PMIProcess information to the handler */
	pmiprocess = PMISetupNewProcess( pmiinfo->fdpair[0], pState );
	MPIE_IORegister( pmiinfo->fdpair[0], IO_READ, PMIServHandleInput, 
			 pmiprocess );
    }
    else {
	/* We defer the step of setting up the process until the client
	   contacts the server.  See PMIServAcceptFromPort for the 
	   creation of the pmiprocess structure and the initialization of 
	   the IO handler for the PMI communication */
	/* FIXME: We may need to record some information, such as the 
	   curPMIGroup, in the pState or pmiprocess entry */
	;
    }
    return 0;
}

static PMIGroup *curPMIGroup = 0;
static int       curNprocess = 0;
/*
  Create a new PMIProcess and initialize it.

  If there is an allocation failure, return non zero.
*/
PMIProcess *PMISetupNewProcess( int fd, ProcessState *pState )
{
    PMIProcess *pmiprocess;

    pmiprocess = (PMIProcess *)MPIU_Malloc( sizeof(PMIProcess) );
    if (!pmiprocess) return 0;
    pmiprocess->fd           = fd;
    pmiprocess->nextChar     = pmiprocess->readBuf;
    pmiprocess->endChar      = pmiprocess->readBuf;
    pmiprocess->group        = curPMIGroup;
    pmiprocess->pState       = pState;
    pmiprocess->spawnApp     = 0;
    pmiprocess->spawnAppTail = 0;
    pmiprocess->spawnKVS     = 0;
    pmiprocess->spawnWorld   = 0;

    /* Add this process to the curPMIGroup */
    curPMIGroup->pmiProcess[curNprocess++] = pmiprocess;

    return pmiprocess;
}

/*
  Initialize a new PMI group that will be the parent of all
  PMIProcesses until the next group is created 
  Each group also starts with a KV Space 
  
  If there is an allocation failure, return non zero.
*/
int PMISetupNewGroup( int nProcess, PMIKVSpace *kvs )
{
    PMIGroup *g;
    curPMIGroup = (PMIGroup *)MPIU_Malloc( sizeof(PMIGroup) );
    if (!curPMIGroup) return 1;

    curPMIGroup->nProcess   = nProcess;
    curPMIGroup->groupID    = pmimaster.nGroups++;
    curPMIGroup->nInBarrier = 0;
    curPMIGroup->pmiProcess = (PMIProcess **)MPIU_Malloc( 
					 sizeof(PMIProcess*) * nProcess );
    if (!curPMIGroup->pmiProcess) return 1;
    curPMIGroup->nextGroup  = 0;
    curNprocess             = 0;

    /* Add to PMIMaster */
    g = pmimaster.groups;
    if (!g) {
	pmimaster.groups = curPMIGroup;
    }
    else {
	while (g) {
	    if (!g->nextGroup) {
		g->nextGroup = curPMIGroup;
		break;
	    }
	    g = g->nextGroup;
	}
    }
    if (kvs) {
	curPMIGroup->kvs = kvs;
    }
    else {
	curPMIGroup->kvs = fPMIKVSAllocate();
    }

    return 0;
}

/* 
 * Process input from the socket connecting the mpiexec process to the
 * child process.
 *
 * The return status is interpreted by the IOLoop code in ioloop.c ;
 * a zero is a normal exit.
 */
int PMIServHandleInput( int fd, int rdwr, void *extra )
{
    PMIProcess *pentry = (PMIProcess *)extra;
    int        rc;
    int        returnCode = 0;
    char       inbuf[PMIU_MAXLINE], cmd[MAXPMICMD];
    PMICmdMap *p;
    int        cmdtype;

    DBG_PRINTFCOND(pmidebug,("Handling PMI input\n") );
    if ( ( rc = PMIUBufferedReadLine( pentry, inbuf, PMIU_MAXLINE ) ) > 0 ) {
	DBG_PRINTFCOND(pmidebug,
		       ("Entering PMIServHandleInputFd %s\n", inbuf) );

	PMIU_parse_keyvals( inbuf );
	cmdtype = PMIGetCommand( cmd, sizeof(cmd) );
	DBG_PRINTFCOND(pmidebug,( "cmd = %s\n", cmd ));
	/* Look for the command and execute the related function */
	p = pmiCommands;
	while (p->handler) {
	    if (strncmp( cmd, p->cmdName, MAXPMICMD ) == 0) {
		rc = (p->handler)( pentry );
		break;
	    }
	    p++;
	}
	if (!p->handler) {
	    PMIU_printf( 1, "unknown cmd %s\n", cmd );
	}
    }
    else {                        /* lost contact with client */
	DBG_PRINTFCOND(pmidebug,("EOF on PMI input\n"));
	/* returning a 1 causes the IO loop code to close the socket */
	returnCode = 1;
    }
    return returnCode;
}

/* ------------------------------------------------------------------------- */
/*
 * Perform any initialization.
 * Input
 *   spawner - A routine to spawn processes
 *   spawnerData - data passed to spawner
 */
int PMIServInit( int (*spawner)(ProcessWorld *, void*), void * spawnerData )
{
    userSpawner     = spawner;
    userSpawnerData = spawnerData;

    return 0;
}

/*
 * Set the debug flag for the pmiserver routines.  Returns the old
 * value of the flag.  0 turns off debugging, non-zero turns it on.
 */
int PMISetDebug( int flag )
{
    int oldflag = pmidebug;
    pmidebug = flag;
    return oldflag;
}
/* ------------------------------------------------------------------------ */
/* Additional service routines                                              */
/* ------------------------------------------------------------------------ */
/*
 * Get either a cmd=val or mcmd=val.  return 0 if cmd, 1 if mcmd, and -1 
 * if neither (an error, since all PMI messages should contain one of
 * these).
 */
int PMIGetCommand( char *cmd, int cmdlen )
{
    char *valptr;
    int  cmdtype = 0;
    
    valptr = PMIU_getval( "cmd", cmd, cmdlen );
    if (!valptr) {
	valptr = PMIU_getval( "mcmd", cmd, cmdlen );
	if (valptr) cmdtype = 1;
	else        cmdtype = -1;
    }
    return cmdtype;
}

/* ------------------------------------------------------------------------- */
/* The rest of these routines are internal                                   */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------- */
static int fPMI_Handle_finalize( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];
    
    pentry->pState->status = PROCESS_FINALIZED;

    /* send back an acknowledgement to release the process */
    MPL_snprintf(outbuf, PMIU_MAXLINE, "cmd=finalize_ack\n");
    PMIWriteLine(pentry->fd, outbuf);

    return 0;
}
static int fPMI_Handle_abort( PMIProcess *pentry )
{
    char exitcodestr[MAXVALLEN];
    int  exitcode = 1; /* non-zero default */

    exitcode = atoi(PMIU_getval( "exitcode", exitcodestr, MAXVALLEN ));

    /* cleanup all MPI processes */
    MPIE_KillUniverse( &pUniv );

    exit(exitcode);

    /* should never reach here */
    return 1;
}
/* 
 * Handle an incoming "barrier" command
 *
 * Need a structure that has the fds for all members of a pmi group
 */
static int fPMI_Handle_barrier( PMIProcess *pentry )
{
    int i;
    PMIGroup *group = pentry->group;

    DBG_PRINTFCOND(pmidebug,("Entering PMI_Handle_barrier for group %d\n", 
		    group->groupID) );

    group->nInBarrier++;
    if (group->nInBarrier == group->nProcess) {
	for ( i=0; i<group->nProcess; i++) {
	    PMIWriteLine(group->pmiProcess[i]->fd, "cmd=barrier_out\n" );
	}
	group->nInBarrier = 0;
    }
    return 0;
}

/* Create a kvs and return a pointer to it */
static PMIKVSpace *fPMIKVSAllocate( void )
{
    PMIKVSpace *kvs, **kPrev, *k;
    int        rc;
    static int kvsnum = 0;    /* Used to generate names */

    /* Create the space */
    kvs = (PMIKVSpace *)MPIU_Malloc( sizeof(PMIKVSpace) );
    if (!kvs) {
	MPL_internal_error_printf( "too many kvs's\n" );
	return 0;
    }
    /* We include the pid of the PMI server as a way to allow multiple
       PMI servers to coexist.  This is needed to support connect/accept
       operations when multiple mpiexec's are used, and the KVS space
       is served directly by mpiexec (it should really have the 
       hostname as well, just to avoid getting the same pid on two
       different hosts, but this is probably good enough for most
       uses) */
    MPL_snprintf( (char *)(kvs->kvsname), MAXNAMELEN, "kvs_%d_%d", 
		   (int)getpid(), kvsnum++ );
    kvs->pairs     = 0;
    kvs->lastByIdx = 0;
    kvs->lastIdx   = -1;

    /* Insert into the list of KV spaces */
    kPrev = &pmimaster.kvSpaces;
    k     = pmimaster.kvSpaces;
    while (k) {
	rc = strcmp( k->kvsname, kvs->kvsname );
	if (rc > 0) break;
	kPrev = &k->nextKVS;
	k     = k->nextKVS;
    }
    kvs->nextKVS = k;
    *kPrev = kvs;

    return kvs;
}
/* Create a kvs and generate its name; return that name as the argument */
static int fPMIKVSGetNewSpace( char kvsname[], int maxlen )
{
    PMIKVSpace *kvs;

    kvs = fPMIKVSAllocate( );
    MPIU_Strncpy( kvsname, kvs->kvsname, maxlen );
    return 0;
}
static int fPMIKVSFindKey( PMIKVSpace *kvs, 
			   const char key[], char val[], int maxval )
{
    PMIKVPair *p;
    int       rc;

    p = kvs->pairs;
    while (p) {
	rc = strcmp( p->key, key );
	if (rc == 0) {
	    /* Found it.  Get the value and return success */
	    MPIU_Strncpy( val, p->val, maxval );
	    return 0;
	}
	if (rc > 0) {
	    /* We're past the point in the sorted list where the
	       key could be found */
	    return 1;
	}
	p = p->nextPair;
    }
    return 1;
}

static int fPMIKVSAddPair( PMIKVSpace *kvs, 
			   const char key[], const char val[] )
{
    PMIKVPair *pair, *p, **pprev;
    int       rc;

    /* Find the location in which to insert the pair (if the
       same key already exists, that is an error) */
    p = kvs->pairs;
    pprev = &(kvs->pairs);
    while (p) {
	rc = strcmp( p->key, key );
	if (rc == 0) {
	    /* Duplicate.  Indicate an error */
	    return 1;
	}
	if (rc > 0) {
	    /* We've found the location (after pprev, before p) */
	    break;
	}
	pprev = &(p->nextPair);
	p = p->nextPair;
    }
    pair = (PMIKVPair *)MPIU_Malloc( sizeof(PMIKVPair) );
    if (!pair) {
	return -1;
    }
    MPIU_Strncpy( pair->key, key, sizeof(pair->key) );
    MPIU_Strncpy( pair->val, val, sizeof(pair->val) );

    /* Insert into the list */
    pair->nextPair = p;
    *pprev         = pair;

    /* Since the list has been modified, clear the index helpers */
    kvs->lastByIdx = 0;
    kvs->lastIdx   = -1;

    return 0;
}

static PMIKVSpace *fPMIKVSFindSpace( const char kvsname[] )
{
    PMIKVSpace *kvs = pmimaster.kvSpaces;
    int rc;

    /* We require the kvs spaces to be stored in a sorted order */
    while (kvs) {
	rc = strcmp( kvs->kvsname, kvsname );
	if (rc == 0) {
	    /* Found */
	    return kvs;
	}
	if (rc > 0) {
	    /* Did not find (kvsname is sorted after kvs->kvsname) */
	    return 0;
	}
	kvs = kvs->nextKVS;
    }
    /* Did not find */
    return 0;
}
static int PMIKVSFree( PMIKVSpace *kvs )
{
    PMIKVPair *p, *pNext;
    PMIKVSpace **kPrev, *k;
    int        rc;
    
    /* Recover the pairs */
    p = kvs->pairs;
    while (p) {
	pNext = p->nextPair;
	MPIU_Free( p );
	p = pNext;
    }

    /* Recover the KVS space, and remove it from the master's list */
    kPrev = &pmimaster.kvSpaces;
    k     = pmimaster.kvSpaces;
    rc    = 1;
    while (k) {
	rc = strcmp( k->kvsname, kvs->kvsname );
	if (rc == 0) {
	    *kPrev = k->nextKVS;
	    MPIU_Free( k );
	    break;
	}
	kPrev = &(k->nextKVS);
	k = k->nextKVS;
    }
    
    /* Note that if we did not find the kvs, we have an internal 
       error, since all kv spaces are maintained within the pmimaster list */
    if (rc != 0) {
	MPL_internal_error_printf( "Could not find KV Space %s\n",
				    kvs->kvsname );
	return 1;
    }
    return 0;
}
/* 
 * Handle an incoming "create_kvs" command
 */
static int fPMI_Handle_create_kvs( PMIProcess *pentry )
{
    char kvsname[MAXKVSNAME], outbuf[PMIU_MAXLINE];
    int rc;

    rc = fPMIKVSGetNewSpace( kvsname, sizeof(kvsname) );
    if (rc) {
	/* PANIC - allocation failed */
	return 1;
    }
    MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=newkvs kvsname=%s\n", kvsname );
    PMIWriteLine( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug, ("Handle_create_kvs new name %s\n", kvsname ) );
    return 0;
}

/* 
 * Handle an incoming "destroy_kvs" command 
 */
static int fPMI_Handle_destroy_kvs( PMIProcess *pentry )
{
    int         rc=0;
    PMIKVSpace *kvs;
    char        kvsname[MAXKVSNAME];
    char        message[PMIU_MAXLINE], outbuf[PMIU_MAXLINE];
    
    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    kvs = fPMIKVSFindSpace( kvsname );
    if (kvs) {
	PMIKVSFree( kvs );
	MPL_snprintf( message, PMIU_MAXLINE,
		       "KVS_%s_successfully_destroyed", kvsname );
    }
    else {
	MPL_snprintf( message, PMIU_MAXLINE, "KVS %s not found", kvsname );
	rc = -1;
    }
    MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=kvs_destroyed rc=%d msg=%s\n",
	      rc, message );
    PMIWriteLine( pentry->fd, outbuf );
    return 0;
}

/* 
 * Handle an incoming "put" command
 */
static int fPMI_Handle_put( PMIProcess *pentry )
{
    int         rc=0;
    PMIKVSpace *kvs;
    char        kvsname[MAXKVSNAME];
    char        message[PMIU_MAXLINE], outbuf[PMIU_MAXLINE];
    char        key[MAXKEYLEN], val[MAXVALLEN];

    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    DBG_PRINTFCOND(pmidebug,( "Put: Finding kvs %s\n", kvsname ) );

    kvs = fPMIKVSFindSpace( kvsname );
    if (kvs) {
	/* should check here for duplicate key and raise error */
	PMIU_getval( "key", key, MAXKEYLEN );
	PMIU_getval( "value", val, MAXVALLEN );
	rc = fPMIKVSAddPair( kvs, key, val );
	if (rc == 1) {
	    rc = -1;          /* no duplicate keys allowed */
	    MPL_snprintf( message, PMIU_MAXLINE, "duplicate_key %s", key );
	}
	else if (rc == -1) {
	    rc = -1;
	    MPL_snprintf( message, PMIU_MAXLINE, "no_room_in_kvs_%s",
			   kvsname );
	}
	else {
	    rc = 0;
	    MPIU_Strncpy( message, "success", PMIU_MAXLINE );
	}
    }
    else {
	rc = -1;
	MPL_snprintf( message, PMIU_MAXLINE, "kvs_%s_not_found", kvsname );
    }
    MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=put_result rc=%d msg=%s\n",
	      rc, message );
    PMIWriteLine( pentry->fd, outbuf );
    return 0;
}

/*
 * Handle incoming "get" command
 */
static int fPMI_Handle_get( PMIProcess *pentry )
{
    PMIKVSpace *kvs;
    int         rc=0;
    char        kvsname[MAXKVSNAME];
    char        message[PMIU_MAXLINE], key[PMIU_MAXLINE], value[PMIU_MAXLINE];
    char        outbuf[PMIU_MAXLINE];
    
    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    DBG_PRINTFCOND(pmidebug,( "Get: Finding kvs %s\n", kvsname ) );

    kvs = fPMIKVSFindSpace( kvsname );
    if (kvs) {
	PMIU_getval( "key", key, PMIU_MAXLINE );
	/* Here we could intercept internal keys, e.g., 
	   pmiPrivate keys. */
	rc = fPMIKVSFindKey( kvs, key, value, sizeof(value) );
	if (rc == 0) {
	    rc = 0;
	    MPIU_Strncpy( message, "success", PMIU_MAXLINE );
	}
	else if (rc) {
	    rc = -1;
	    MPIU_Strncpy( value, "unknown", PMIU_MAXLINE );
	    MPL_snprintf( message, PMIU_MAXLINE, "key_%s_not_found", 
			   kvsname );
	}
    }
    else { 
	rc = -1;
	MPIU_Strncpy( value, "unknown", PMIU_MAXLINE );
	MPL_snprintf( message, PMIU_MAXLINE, "kvs_%s_not_found", kvsname );
    }
    MPL_snprintf( outbuf, PMIU_MAXLINE, 
		   "cmd=get_result rc=%d msg=%s value=%s\n",
		   rc, message, value );
    PMIWriteLine( pentry->fd, outbuf );
    DBG_PRINTFCOND( pmidebug, ("%s", outbuf ));
    return rc;
}

/* Handle an incoming get_my_kvsname command */
static int fPMI_Handle_get_my_kvsname( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];
    PMIKVSpace *kvs;

    kvs = pentry->group->kvs;
    if (kvs && kvs->kvsname) {
	MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=my_kvsname kvsname=%s\n",
		       kvs->kvsname );
    }
    else {
	MPL_internal_error_printf( "Group has no associated KVS" );
	return -1;
    }
    PMIWriteLine( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return 0;
}

/* Handle an incoming get_universe_size command */
static int fPMI_Handle_get_universe_size( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];
    /* Import the universe size from the process structures */
    MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=universe_size size=%d\n",
		   pUniv.size );
    PMIWriteLine( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return 0;
}

/* Handle an incoming get_appnum command */
static int fPMI_Handle_get_appnum( PMIProcess *pentry )
{
    ProcessApp *app = pentry->pState->app;
    char outbuf[PMIU_MAXLINE];
    MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=appnum appnum=%d\n",
		   app->myAppNum );		
    PMIWriteLine( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return 0;
}

/* Handle an incoming "init" command */
static int fPMI_Handle_init( PMIProcess *pentry )
{
    char version[PMIU_MAXLINE];
    char subversion[PMIU_MAXLINE];
    char outbuf[PMIU_MAXLINE];
    int rc;

    /* check version compatibility with PMI client library */
    PMIU_getval( "pmi_version", version, PMIU_MAXLINE );
    PMIU_getval( "pmi_subversion", subversion, PMIU_MAXLINE );
    if (PMI_VERSION == atoi(version) && PMI_SUBVERSION >= atoi(subversion))
	rc = 0;
    else
	rc = -1;

    pentry->pState->status = PROCESS_COMMUNICATING;

    MPL_snprintf( outbuf, PMIU_MAXLINE,
	   "cmd=response_to_init pmi_version=%d pmi_subversion=%d rc=%d\n",
		   PMI_VERSION, PMI_SUBVERSION, rc);
    PMIWriteLine( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return 0;
}

/* Handle an incoming "get_maxes" command */
static int fPMI_Handle_get_maxes( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];
    MPL_snprintf( outbuf, PMIU_MAXLINE,
	      "cmd=maxes kvsname_max=%d keylen_max=%d vallen_max=%d\n",
	      MAXKVSNAME, MAXKEYLEN, MAXVALLEN );
    PMIWriteLine( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return 0;
}

/*
 * Handle incoming "getbyidx" command
 */
static int fPMI_Handle_getbyidx( PMIProcess *pentry )
{
    int j, jNext, rc=0;
    PMIKVSpace *kvs;
    char kvsname[MAXKVSNAME], j_char[8], outbuf[PMIU_MAXLINE];
    PMIKVPair *p;

    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    kvs = fPMIKVSFindSpace( kvsname );
    if (kvs) {
	PMIU_getval( "idx", j_char, sizeof(j_char) );
	j = atoi( j_char );
	jNext = j+1;
	if (kvs->lastIdx >= 0 && j >= kvs->lastIdx) {
	    for (p = kvs->lastByIdx, j-= kvs->lastIdx; j-- > 0 && p; 
		 p = p->nextPair );
	}
	else {
	    for (p = kvs->pairs; j-- > 0 && p; p = p->nextPair) ;
	}
	if (p) {
	    MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=getbyidx_results "
			   "rc=0 nextidx=%d key=%s val=%s\n",
			   jNext, p->key, p->val );
	    kvs->lastIdx   = jNext-1;
	    kvs->lastByIdx = p;
	}
	else {
	    MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=getbyidx_results rc=-1 "
			   "reason=no_more_keyvals\n" );
	    kvs->lastIdx   = -1;
	    kvs->lastByIdx = 0;
	}
    }
    else {
	rc = -1;
	MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=getbyidx_results rc=-1 "
		  "reason=kvs_%s_not_found\n", kvsname );
    }

    PMIWriteLine( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return rc;
}

/* ------------------------------------------------------------------------- */
/* Using a host:port instead of a pre-existing FD to establish communication.
 * Steps:
 * The server sets up a listener with 
 *      PMIServSetupPort - also returns the host:port name (in that form)
 * When the server creates the process, the routine
 *      PMISetupSockets( 1, &pmiinfo )
 * will check that there is a host/port name, and initialize the fd for
 * communication to the PMI server to -1 (to indicate that no connection is
 * set up)
 * After the fork, the child will call
 *      PMISetupInClient( 1, &pmiinfo )
 * This adds the PMI_PORT and PMI_ID values to the enviroment
 * The parent also calls
 *      PMISetupFinishInServer( 1, &pmiinfo, pState )
 * ? What should this do, since there is no connection yet?
 * ? Remember about the process in pmiinfo (curPMIGroup, pState?)
 *
 * The connection process is as follows:
 *    The client (the created process), if using simple_pmi, does the
 *    following:
 *    PMI_Init checks for the environment variables PMI_FD and PMI_PORT,
 *      If PMI_FD is not set but PMI_PORT is set, then call 
 *        PMII_Connect_to_pm( hostname, portnum )
 *          This returns an open FD that will be used for further
 *          communication.  (the server side handles this through
 *          the listener)
 *      Next, use the value in the environment variable PMI_ID and
 *      call
 *        PMII_Set_from_port
 *           First, write cmd=initack pmiid=<value>
 *           Read line, require cmd=initack
 *           (This completes the handshake)
 *           Next, read a sequence of 3 commands in this order
 *           cmd=set size=<value>
 *           cmd=set rank=<value>
 *           cmd=set debug=<value>
 *        At this point, the connection is in the same state (with respect
 *        to the PMI protocol) as a new if PMI_FD was set.  In particular,
 *        the next command will be cmd=init.  Think of the port setup as
 *        a prefix on the wire protocol.
 *
 * To handle this in the server
 *       
 *
 */
/* ------------------------------------------------------------------------- */
/*
 * These routines are called when communication is established through 
 * a port instead of an fd, and no information is communicated
 * through environment variables.
 */
static int fPMI_Handle_init_port( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];

    DBG_PRINTFCOND(pmidebug,
		   ( "Entering fPMI_Handle_init_port to start connection\n" ));

    /* simple_pmi wants to see cmd=initack after the initack request before
       the other data */
    PMIWriteLine( pentry->fd, "cmd=initack\n" );
    MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=set size=%d\n", 
		   pentry->group->nProcess );
    PMIWriteLine( pentry->fd, outbuf );
    MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=set rank=%d\n", 
		   pentry->pState->wRank );
    PMIWriteLine( pentry->fd, outbuf );
    MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=set debug=%d\n", pmidebug );
    PMIWriteLine( pentry->fd, outbuf );
    return 0;
}
/* ------------------------------------------------------------------------- */

/* Handle a spawn command.  This makes use of the spawner routine that
   was set during PMIServInit
   
   Spawn command in the simple PMI is handled as multiple lines with the
   subcommands:
   nprocs=%d
   execname=%s
   totspawns=%d
   spawnssofar=%d
   argcnt=%d
   arg%d=%s
   preput_num=%d
   preput_key_%d=%s
   preput_val_%d=%s
   info_num=%d
   info_key_%d=%s
   info_val_%d=%s
   endcmd

   After all of the data is collected, a ProcessWorld structure is
   created and passed to the user's spawner command.  That command
   is responsible for adding the ProcessWorld to the pUniv (ProcessUniverse)
   structure (in case there is an error and the spawn fails, the user
   can decide whether to included the failed world in the pUniv structure).


   Note that this routine must create the new "current" PMI group so that
   there will be a place for the associated KV Space.
  
*/

/* Maximum number of arguments */
#define PMI_MAX_ARGS 256
static int fPMI_Handle_spawn( PMIProcess *pentry )
{
    char          inbuf[PMIU_MAXLINE];
    char          *(args[PMI_MAX_ARGS]);
    char          key[MAXKEYLEN];
    char          outbuf[PMIU_MAXLINE];
    ProcessWorld *pWorld;
    ProcessApp   *app = 0;
    int           preputNum = 0, rc;
    int           i;
    int           totspawns=0, spawnnum=0;
    PMIKVSpace    *kvs = 0;
    /* Variables for info */
    char curInfoKey[PMI_MAX_INFO_KEY], curInfoVal[PMI_MAX_INFO_VAL];
    int  curInfoIdx = -1;

    DBG_PRINTFCOND(pmidebug,( "Entering fPMI_Handle_spawn\n" ));

    if (!pentry->spawnWorld) {
	pWorld = (ProcessWorld *)MPIU_Malloc( sizeof(ProcessWorld) );
	if (!pWorld) return 1;
	
	pentry->spawnWorld = pWorld;
	pWorld->apps       = 0;
	pWorld->nProcess   = 0;
	pWorld->nextWorld  = 0;
	pWorld->nApps      = 0;
	pWorld->worldNum   = pUniv.nWorlds++;
	/* FIXME: What should be the defaults for the spawned env? 
	   Should the default be the env ov the spawner? */
	pWorld->genv       = 0;
	pentry->spawnKVS   = fPMIKVSAllocate();
    }
    else {
	pWorld = pentry->spawnWorld;
    }
    kvs    = pentry->spawnKVS;

    /* Note that each mcmd=spawn creates an app.  When all apps
       are present, then then can be linked to a world.  A 
       spawnmultiple command makes use of multiple mcmd=spawn PMI
       commands */ 

    /* Create a new app */
    app = (ProcessApp *)MPIU_Malloc( sizeof(ProcessApp) );
    if (!app) return 1;
    app->myAppNum  = 0;
    app->exename   = 0;
    app->arch      = 0;
    app->path      = 0;
    app->wdir      = 0;
    app->hostname  = 0;
    app->args      = 0;
    app->nArgs     = 0;
    app->soft.nelm = 0;
    app->nProcess  = 0;
    app->pState    = 0;
    app->nextApp   = 0;
    app->env       = 0;
    app->pWorld    = pWorld;

    /* Add to the pentry spawn structure */
    if (pentry->spawnAppTail) {
	pentry->spawnAppTail->nextApp = app;
    }
    else {
	pentry->spawnApp = app;
	pWorld->apps     = app;
    }
    pentry->spawnAppTail = app;

    for (i=0; i<PMI_MAX_ARGS; i++) args[i] = 0;

    /* Get lines until we find either cmd or mcmd (an error) or endcmd 
       (expected end) */
    while ((rc = PMIUBufferedReadLine( pentry, inbuf, sizeof(inbuf) )) > 0) {
	char *cmdPtr, *valPtr, *p;

	/* Find the command = format */
	p = inbuf;
	/* Find first nonblank */
	while (*p && isascii(*p) && isspace(*p)) p++;
	if (!*p) {
	    /* Empty string.  Ignore */
	    continue;
	}
	cmdPtr = p++;
	/* Find '=' */
	while (*p && *p != '=') p++;
	if (!*p) {
	    /* No =.  Check for endcmd */
	    p--;
	    /* Trim spaces */
	    while (isascii(*p) && isspace(*p)) p--;
	    /* Add null to end */
	    *++p = 0;
	    if (strcmp( "endcmd", cmdPtr ) == 0) { break; }
	    /* FIXME: Otherwise, we have a problem */
	    MPL_error_printf( "Malformed PMI command (no endcmd seen\n" );
	    return 1;
	}
	else {
	    *p = 0;
	}
	
	/* Found an = .  value is the rest of the line */
	valPtr = ++p; 
	while (*p && *p != '\n') p++;
	if (*p) *p = 0;     /* Remove the newline */

	/* Now, process the cmd and value */
	if (strcmp( "nprocs", cmdPtr ) == 0) {
	    app->nProcess     = atoi(valPtr);
	    pWorld->nProcess += app->nProcess;
	}
	else if (strcmp( "execname", cmdPtr ) == 0) {
	    app->exename = MPIU_Strdup( valPtr );
	}
	else if (strcmp( "totspawns", cmdPtr ) == 0) {
	    /* This tells us how many separate spawn commands
	       we expect to see (e.g., for spawn multiple).
	       Each spawn command is a separate "app" */
	    totspawns = atoi(valPtr);
	}
	else if (strcmp( "spawnssofar", cmdPtr ) == 0) {
	    /* This tells us which app we are (starting from 1) */
	    spawnnum      = atoi(valPtr);
	    app->myAppNum = spawnnum - 1;
	}
	else if (strcmp( "argcnt", cmdPtr ) == 0) {
	    /* argcnt may not be set before the args */
	    app->nArgs = atoi(valPtr);
	}
	else if (strncmp( "arg", cmdPtr, 3 ) == 0) {
	    int argnum;
	    /* argcnt may not be set before the args */
	    /* Handle arg%d.  Values are 1 - origin */
	    argnum = atoi( cmdPtr + 3 ) - 1;
	    if (argnum < 0 || argnum >= PMI_MAX_ARGS) {
		MPL_error_printf( "Malformed PMI Spawn command; the index of an argument in the command is %d but must be between 0 and %d\n",
				   argnum, PMI_MAX_ARGS-1 );
		return 1;
	    }
	    args[argnum] = MPIU_Strdup( valPtr );
	}
	else if (strcmp( "preput_num", cmdPtr ) == 0) {
	    preputNum = atoi(valPtr);
	}
	else if (strncmp( "preput_key_", cmdPtr, 11 ) == 0) {
	    /* Save the key */
	    MPIU_Strncpy( key, valPtr, sizeof(key) );
	}
	else if (strncmp( "preput_val_", cmdPtr, 11 ) == 0) {
	    /* Place the key,val into the space associate with the current 
	       PMI group */
	    fPMIKVSAddPair( kvs, key, valPtr );
	}
	/* Info is on a per-app basis (it is an array of info items in
	   spawn multiple).  We can ignore most info values.
	   The ones that are handled are processed by a 
	   separate routine (not yet implemented).
	   simple_pmi.c sends (key,value), so we can keep just the
	   last key and pass the key/value to the registered info
	   handler, along with tha app structure.  Alternately,
	   we could save all info items and let the user's 
	   spawner handle it */
	else if (strcmp( "info_num", cmdPtr ) == 0) {
	    /* Number of info values */
	    ;
	}
	else if (strncmp( "info_key_", cmdPtr, 9 ) == 0) {
	    /* The actual name has a digit, which indicates *which* info 
	       key this is */
	    curInfoIdx = atoi( cmdPtr + 9 );
	    MPIU_Strncpy( curInfoKey, valPtr, sizeof(curInfoKey) );
	}
	else if (strncmp( "info_val_", cmdPtr, 9 ) == 0) {
	    /* The actual name has a digit, which indicates *which* info 
	       value this is */
	    int idx = atoi( cmdPtr + 9 );
	    if (idx != curInfoIdx) {
		MPL_error_printf( "Malformed PMI command: info keys and values not ordered as expected (expected value %d but got %d)\n", curInfoIdx, idx );
		return 1;
	    }
	    else {
		MPIU_Strncpy( curInfoVal, valPtr, sizeof(curInfoVal) );
		/* Apply this info item */
		fPMIInfoKey( app, curInfoKey, curInfoVal );
		/* printf( "Got info %s+%s\n", curInfoKey, curInfoVal ); */
	    }
	}
	else {
	    MPL_error_printf( "Unrecognized PMI subcommand on spawnmult: %s\n",
			       cmdPtr );
	    return 1;
	}
    }	

    if (app->nArgs > 0) {
	app->args  = (const char **)MPIU_Malloc( app->nArgs * sizeof(char *) );
	for (i=0; i<app->nArgs; i++) {
	    app->args[i] = args[i];
	    args[i]      = 0;
	}
    }

    pWorld->nApps ++;

    /* Now that we've read the commands, invoke the user's spawn command */
    if (totspawns == spawnnum) {
	PMISetupNewGroup( pWorld->nProcess, kvs );
	
	if (userSpawner) {
	    rc = (*userSpawner)( pWorld, userSpawnerData );
	}
	else {
	    MPL_error_printf( "Unable to spawn %s\n", app->exename );
	    rc = 1;
	    MPIE_PrintProcessWorld( stdout, pWorld );
	}
	
	MPL_snprintf( outbuf, PMIU_MAXLINE, "cmd=spawn_result rc=%d\n", rc );
	PMIWriteLine( pentry->fd, outbuf );
	DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));

	/* Clear for the next spawn */
	pentry->spawnApp     = 0;
	pentry->spawnAppTail = 0;
	pentry->spawnKVS     = 0;
	pentry->spawnWorld   = 0;
    }
    
    /* If totspawnnum != spawnnum, then we are expecting a 
       spawnmult with additional items */
    return 0;
}

/* ------------------------------------------------------------------------- */
/*  
 * FIXME:
 * Question: What does this need to do?
 * 1.  Is nproces in range?
 * 2.  Is the program executable?
 * 3.  Create the KVS group
 * 4.  Invoke startup -> the process startup procedure must have
 *     been registered by the calling program (e.g., usually by
 *     mpiexec, but possibly a separate pmiserver process)
 * 5. return kvsname; return code
 *    How do we handle soft (no specific return size required).
 *    Also, is part fo the group associated with these processes or
 *    another group (the spawner?) of processes?
 *
 * This should be called after receiving the cmd=initack from the client.
 */
void PMI_Init_remote_proc( int fd, PMIProcess *pentry )
{
    /* Everything else should be setup by PMISetupNewProcess */
    fPMI_Handle_init_port( pentry );
}

/* 
 * This is a special routine.  It accepts the first input from the
 * remote process, and returns the PMI_ID value.  -1 is returned on error 
 */
int PMI_Init_port_connection( int fd )
{
    char message[PMIU_MAXLINE], cmd[MAXPMICMD];
    int pmiid = -1;

    DBG_PRINTFCOND(pmidebug,( "Beginning initial handshake read\n" ));
    PMIReadLine( fd, message, PMIU_MAXLINE );
    DBG_PRINTFCOND(pmidebug,( "received message %s\n", message ));

    PMIU_parse_keyvals( message );
    PMIU_getval( "cmd", cmd, MAXPMICMD );
    if (strcmp(cmd,"initack")) {
	PMIU_printf( 1, "Unexpected cmd %s, expected initack\n", cmd );
	return -1;
    }
    PMIU_getval( "pmiid", cmd, MAXPMICMD );
    pmiid = atoi(cmd);

    return pmiid;
}

/* Implement the singleton init handshake.  See the discussion in 
   simplepmi.c for the protocol */
int PMI_InitSingletonConnection( int fd, PMIProcess *pmiprocess )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int  rc;
    char version[PMIU_MAXLINE], subversion[PMIU_MAXLINE];
    
    /* We start with the singinit command, wait for the singinit from
       the client, and then send the singinit_info */
    MPL_snprintf( buf, PMIU_MAXLINE, 
   "cmd=singinit pmi_version=%d pmi_subversion=%d stdio=no authtype=none\n",
	   PMI_VERSION, PMI_SUBVERSION );
    PMIWriteLine( fd, buf );
    PMIReadLine( fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, MAXPMICMD );
    if (strcmp(cmd,"singinit")) {
	PMIU_printf( 1, "Unexpected cmd %s\n", cmd );
	return -1;
    }
    /* Could look at authtype */
    /* check version compatibility with PMI client library */
    PMIU_getval( "pmi_version", version, PMIU_MAXLINE );
    PMIU_getval( "pmi_subversion", subversion, PMIU_MAXLINE );
    if (PMI_VERSION == atoi(version) && PMI_SUBVERSION >= atoi(subversion))
	rc = 0;
    else
	rc = -1;
    
    MPL_snprintf( buf, PMIU_MAXLINE,
		   "cmd=singinit_info versionok=%s stdio=no kvsname=%s\n",
		   (rc == 0) ? "yes" : "no",  
		   (char *)(pmiprocess->group->kvs->kvsname) );
    PMIWriteLine( fd, buf );

    return 0;
}

/* Handle the default info values */
static int fPMIInfoKey( ProcessApp *app, const char key[], const char val[] )
{
    if (strcmp( key, "host" ) == 0) {
	app->hostname = MPIU_Strdup( val );
    }
    else if (strcmp( key, "arch" ) == 0) {
	app->arch     = MPIU_Strdup( val );
    }
    else if (strcmp( key, "wdir" ) == 0) {
	app->wdir     = MPIU_Strdup( val );
    }
    else if (strcmp( key, "path" ) == 0) {
	app->path     = MPIU_Strdup( val );
    }
    else if (strcmp( key, "soft" ) == 0) {
	MPIE_ParseSoftspec( val, &app->soft );
    }
    else {
	/* FIXME: call user-specified info handler, if any.
	   Unspecified info keys are ignored */
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
#ifndef PMIWriteLine
int PMIWriteLine( int fd, const char *buf )
{
    int rc;

    DBG_PRINTFCOND(pmidebug,( "Writing to fd %d the line :%s:\n", fd, buf ));
    rc = PMIU_writeline( fd, (char*)buf );
    DBG_PRINTFCOND(pmidebug&&rc<0,( "Write on fd %d returned rc %d\n", fd, rc ));

    return rc;
}
int PMIReadLine( int fd, char *buf, int maxlen )
{
    int rc;

    /* If no data is read, PMIU_readline doesn't place a null in the
       buffer.  We make sure that buf[0] is always valid */
    buf[0] = 0;
    rc = PMIU_readline( fd, buf, maxlen );
    if (pmidebug) {
	if (rc < 0) {
	    DBG_PRINTFCOND(1,( "Read on fd %d returned rc %d\n", fd, rc ) );
	}
	else {
	    DBG_PRINTFCOND(1,( "Read from fd %d the line :%s:\n", fd, buf ) );
	}
	DBG_COND(1,fflush(stdout));
    }

    return rc;
}
#endif
/* ------------------------------------------------------------------------- */
/* 
 * To handle multiple incoming command streams, we need to have a separate
 * states buffered input (replacing PMIU_readline from the client-side
 * code)
 */
/* ------------------------------------------------------------------------- */
/* 
 * Return the next newline-terminated string of maximum length maxlen.
 * This is a buffered version, and reads from fd as necessary.  A
 */
static int PMIUBufferedReadLine( PMIProcess *pentry, char *buf, int maxlen )
{
    int curlen, n;
    char *p, ch;
    int  fd = pentry->fd;
    char *readbuf  = pentry->readBuf;
    char *nextChar = pentry->nextChar;
    char *endChar  = pentry->endChar;

    p      = buf;
    curlen = 1;    /* Make room for the null */
    while (curlen < maxlen) {
	if (nextChar == endChar) {
	    do {
                /* Carefully read data into buffer.  This could be
                   written to read more at one time, but would then
                   need to know the size of the readbuf */
		n = (int)read( fd, readbuf, 1 );
	    } while (n == -1 && errno == EINTR);
	    if (n == 0) {
		/* EOF */
		break;
	    }
	    else if (n < 0) {
		/* Error.  Return a negative value if there is no
		   data.  Save the errno in case we need to return it
		   later. */
		if (curlen == 1) {
		    curlen = 0;
		}
		break;
	    }
	    nextChar = readbuf;
	    endChar  = readbuf + n;
	    pentry->endChar  = endChar;
	    /* Add a null at the end just to make it easier to print
	       the read buffer */
	    readbuf[n] = 0;
	    /* FIXME: Make this an optional output */
	    /* printf( "Readline %s\n", readbuf ); */
	}
	
	ch   = *nextChar++;
	*p++ = ch;
	curlen++;
	if (ch == '\n') break;
    }

    /* We null terminate the string for convenience in printing */
    *p = 0;

    /* Save the state of the buffer */
    pentry->nextChar = nextChar;

    /* Return the number of characters, not counting the null */
    return curlen-1;
}

void pmix_preput( const char *key, const char *val )
{
    fPMIKVSAddPair( curPMIGroup->kvs, key, val );
}
