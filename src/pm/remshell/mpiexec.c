/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* OWNER=gropp */

/* An example mpiexec program that uses a remote shell program to create
   new processes on the selected hosts.

   This code also shows how to use the pmutil routines (in ../util) 
   to provide many of the services required by mpiexec 

   Steps:
   1. Read and process that command line.  Build a ProcessList.  (A ProcessList
   may have one entry for a request to create n separate processes)
   
   2. Convert the ProcessList into a ProcessTable.  In the forker mpiexec,
   this simply expands the requested number of processes into an 
   array with one entry per process.  These entries contain information
   on both the setup of the processes and the file descriptors used for
   stdin,out,err, and for the PMI calls.

   3. (Optionally) allow the forked processes to use a host:port to 
   contact this program, rather than just sharing a pipe.  This allows the
   forker to start other programs, such as debuggers.

   4. Establish a signal handler for SIGCHLD.  This will allow us to 
   get information about process termination; in particular, the exit
   status.

   5. Start the programs.

   6. Process input from the programs; send stdin given to this process 
   to the selected processes (usually rank 0 or everyone).  Handle all 
   PMI commands, including spawn.  Another "input" is the expiration of the
   specified timelimit for the run, if any.

   7. Process rundown commands and handle any abnormal termination.  

   8. Wait for any processes to exit; gather the exit status and reason
   for exit (if abnormal, such as signaled with SEGV or BUS)

   9. Release all resources and compute the exit status for this program
   (using one of several approaches, such as taking the maximum of the
   exit statuses).

  Special Case to support Singleton Init:
  To support a singleton init of a process that then wants to 
  create processes with MPI_Comm_spawn(_multiple), a special form of
  mpiexec is supported:
  
     mpiexec -pmi_args <port> <interfacename> <securitykey> <pid>

  The singleton process (in a routine in simple_pmi.c) forks a process and
  execs mpiexe with these arguments, where port is the port to which 
  mpiexec should connect, interfacename is the name of the network interface
  (BUG: may not be correctly set as mpd currently ignores it), securitykey
  is a place-holder for a key used by the singleton init process to verify
  that the process connecting on the port is the one that was intended, and
  pid is the pid of the singleton init process.

  FIXME: The above has not been implemented yet.
*/

#include "remshellconf.h"
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>

#include "pmutil.h"
#include "process.h"
#include "cmnargs.h"
#include "pmiserv.h"
#include "ioloop.h"
#include "labelout.h"
#include "rm.h"
#include "simple_pmiutil.h"
#include "env.h"             /* MPIE_Putenv */
/* mpimem.h contains prototypes for MPIU_Strncpy etc. */
/* We no longer can use these because they are MPI device specific */
/* #include "mpimem.h" */

typedef struct { PMISetup pmiinfo; IOLabelSetup labelinfo; } SetupInfo;

/* Forward declarations */
int mypreamble( void *, ProcessState* );
int mypostfork( void *, void *, ProcessState* );
int mypostamble( void *, void *, ProcessState* );
int myspawn( ProcessWorld *, void * );

static int AddEnvSetToCmdLine( const char *, const char *, const char ** );

/* Set printFailure to 1 to get an explanation of the failure reason
   for each process when a process fails */
static int printFailure = 0;

#ifndef MAX_PORT_STRING
#define MAX_PORT_STRING 1024
#endif

/* Note that envp is common but not standard */
int main( int argc, char *argv[], char *envp[] )
{
    int          rc;
    int          erc = 0;  /* Other (exceptional) return codes */
    int          reason, signaled = 0;
    SetupInfo    s;
    char         portString[MAX_PORT_STRING];

    /* MPIE_ProcessInit initializes the global pUniv */
    MPIE_ProcessInit();
    /* Set a default for the universe size */
    pUniv.size = 64;

    /* Set defaults for any arguments that are options.  Also check the
       environment for special options, such as debugging.  Set 
       some defaults in pUniv */
    MPIE_CheckEnv( &pUniv, 0, 0 );
    IOLabelCheckEnv( );

    /* Handle the command line arguments.  Use the routine from util/cmnargs.c
       to fill in the universe */
    MPIE_Args( argc, argv, &pUniv, 0, 0 );
    /* If there were any soft arguments, we need to handle them now */
    rc = MPIE_InitWorldWithSoft( &pUniv.worlds[0], pUniv.size );
    if (!rc) {
	MPIU_Error_printf( "Unable to process soft arguments\n" );
	exit(1);
    }

    if (pUniv.fromSingleton) {
	/* The MPI process is already running.  We create a simple entry
	   for a single process rather than creating the process */
	MPIE_SetupSingleton( &pUniv );
    }


    rc = MPIE_ChooseHosts( &pUniv.worlds[0], MPIE_ReadMachines, 0 );
    if (rc) {
	MPIU_Error_printf( "Unable to assign hosts to processes\n" );
	exit(1);
    }

    if (MPIE_Debug) MPIE_PrintProcessUniverse( stdout, &pUniv );

    DBG_PRINTF( ("timeout_seconds = %d\n", pUniv.timeout) );

    /* Get the common port for creating PMI connections to the created
       processes */
    rc = PMIServSetupPort( &pUniv, portString, sizeof(portString) );
    if (rc) {
	MPIU_Error_printf( "Unable to setup port for listener\n" );
	exit(1);
    }
    s.pmiinfo.portName = portString;

#ifdef USE_MPI_STAGE_EXECUTABLES
    /* Hook for later use in staging executables */
    if (?stageExes) {
	rc = MPIE_StageExecutables( &pUniv.worlds[0] );
	if (!rc) ...;
    }
#endif

    PMIServInit(myspawn,&s);
    s.pmiinfo.pWorld = &pUniv.worlds[0];
    PMISetupNewGroup( pUniv.worlds[0].nProcess, 0 );
    MPIE_ForwardCommonSignals(); 
    if (!pUniv.fromSingleton) {
	MPIE_ForkProcesses( &pUniv.worlds[0], envp, mypreamble, &s,
			mypostfork, 0, mypostamble, 0 );
    }
    else {
	/* FIXME: The singleton code goes here */
	MPIU_Error_printf( "Singleton init not supported\n" );
	exit(1);
    }
    reason = MPIE_IOLoop( pUniv.timeout );

    if (reason == IOLOOP_TIMEOUT) {
	/* Exited due to timeout.  Generate an error message and
	   terminate the children */
	if (pUniv.timeout > 60) {
	    MPIU_Error_printf( "Timeout of %d minutes expired; job aborted\n",
			       pUniv.timeout / 60 );
	}
	else {
	    MPIU_Error_printf( "Timeout of %d seconds expired; job aborted\n",
			       pUniv.timeout );
	}
	erc = 1;
	MPIE_KillUniverse( &pUniv );
    }

    /* Wait for all processes to exit and gather information on them.
       We do this through the SIGCHLD handler. We also bound the length
       of time that we wait to 2 seconds.
    */
    MPIE_WaitForProcesses( &pUniv, 2 );

    /* Compute the return code (max for now) */
    rc = MPIE_ProcessGetExitStatus( &signaled );

    /* Optionally provide detailed information about failed processes */
    if ( (rc && printFailure) || signaled) 
	MPIE_PrintFailureReasons( stderr );

    /* If the processes exited normally (or were already gone) but we
       had an exceptional exit, such as a timeout, use the erc value */
    if (!rc && erc) rc = erc;

    return( rc );
}

void mpiexec_usage( const char *msg )
{
    if (msg) {
	MPIU_Error_printf( msg );
	if (msg[strlen(msg)-1] != '\n') {
	    MPIU_Error_printf( "\n" );
	}
    }
    MPIU_Usage_printf( "Usage: mpiexec %s\n", MPIE_ArgDescription() );
    exit( -1 );
}

/* Redirect stdout and stderr to a handler */
int mypreamble( void *data, ProcessState *pState )
{
    SetupInfo *s = (SetupInfo *)data;
    int       rc;

    IOLabelSetupFDs( &s->labelinfo );
    rc = PMISetupSockets( 1, &s->pmiinfo );
    /* We must use communication over the socket, rather than the 
       environment, to pass initialization data */
    pState->initWithEnv = 0;
    
    return rc;
}

/* Close one side of each pipe pair and replace stdout/err with the pipes */
int mypostfork( void *predata, void *data, ProcessState *pState )
{
    SetupInfo *s = (SetupInfo *)predata;
    int curarg=0;

    IOLabelSetupInClient( &s->labelinfo );
    PMISetupInClient( 1, &s->pmiinfo );

    /* Now, we *also* change the process state to insert the 
       interposed remote shell routine.  This is probably not
       where we want this in the final version (because MPIE_ExecProgram
       does a lot under the assumption that the started program will
       know what to do with new environment variables), but this
       will allow us to start. */
    {
	ProcessApp *app = pState->app;
	const char **newargs = 0;
	char *pmiDebugStr = 0;
	int j;
	char rankStr[12];

	/* Insert into app->args */
	newargs = (const char **) MPIU_Malloc( (app->nArgs + 14 + 1) * 
					  sizeof(char *) );
	if (!pState->hostname) {
	    MPIU_Error_printf( "No hostname avaliable for %s\n", app->exename );
	    exit(1);
	}

	snprintf( rankStr, sizeof(rankStr)-1, "%d", pState->id );
	rankStr[12-1] = 0;
	curarg = 0;
        newargs[curarg++] = MPIU_Strdup( "-Y" );

	newargs[curarg++] = pState->hostname;
	curarg += AddEnvSetToCmdLine( "PMI_PORT", s->pmiinfo.portName, 
				      newargs + curarg );
	curarg += AddEnvSetToCmdLine( "PMI_ID", rankStr, newargs + curarg );
	pmiDebugStr = getenv( "PMI_DEBUG" );
	if (pmiDebugStr) {
	    /* Use this to help debug the connection process */
	    curarg += AddEnvSetToCmdLine( "PMI_DEBUG", pmiDebugStr, 
					  newargs + curarg );
	}

	newargs[curarg++] = app->exename;
	for (j=0; j<app->nArgs; j++) {
	    newargs[j+curarg] = app->args[j];
	}
	newargs[j+curarg] = 0;
	app->exename = MPIU_Strdup( "/usr/bin/ssh" );

	app->args = newargs;
	app->nArgs += curarg;

	if (MPIE_Debug) {
	    printf( "cmd = %s\n", app->exename ); fflush(stdout);
	    printf( "Number of args = %d\n", app->nArgs );
	    for (j=0; j<app->nArgs; j++) {
		printf( "argv[%d] = %s\n", j, app->args[j] ); fflush(stdout);
	    }
	}
    }

    return 0;
}

/* Close one side of the pipe pair and register a handler for the I/O */
int mypostamble( void *predata, void *data, ProcessState *pState )
{
    SetupInfo *s = (SetupInfo *)predata;

    IOLabelSetupFinishInServer( &s->labelinfo, pState );
    PMISetupFinishInServer( 1, &s->pmiinfo, pState );

    return 0;
}

int myspawn( ProcessWorld *pWorld, void *data )
{
    SetupInfo    *s = (SetupInfo *)data;
    ProcessWorld *p, **pPtr;

    p = pUniv.worlds;
    pPtr = &(pUniv.worlds);
    while (p) {
	pPtr = &p->nextWorld;
	p    = *pPtr;
    }
    *pPtr = pWorld;

    /* Fork Processes may call a routine that is passed s but not pWorld;
       this makes sure that all routines can access the current world */
    s->pmiinfo.pWorld = pWorld;

    /* FIXME: This should be part of the PMI initialization in the clients */
    MPIE_Putenv( pWorld, "PMI_SPAWNED=1" );

    MPIE_ForkProcesses( pWorld, 0, mypreamble, s,
			mypostfork, 0, mypostamble, 0 );
    return 0;
}

/* Temp test for the replacement for the simple "spawn == fork" */

/*
 * Approach:
 * Processes are created using a remote shell program. This requires
 * changing the command line from
 *
 *  a.out args ...
 * 
 * to 
 *
 * remshell-program remshell-args /bin/sh -c PMI_PORT=string && 
 *            export PMI_PORT && PMI_ID=rank-in-world && export PMI_ID &&
 *            a.out args
 *
 * (the export PMI_PORT=string syntax is not valid in all versions of sh)
 *
 * Using PMI_ID ensures that we correctly identify each process (this was
 * a major problem in the setup used by the p4 device in MPICH1).  
 * Using environment variables instead of command line arguments keeps
 * the commaand line clean.  
 *
 * Two alternatives should be considered
 * 1) Use an intermediate manager.  This would allow us to set up the
 *    environment as well:
 *    remshell-program remshell-args manager -port string
 *    One possibilty for the manager is the mpd manager
 * 2) Use the secure server (even the same one as in MPICH1); then 
 *    there is no remote shell command.
 * 
 * We can handle the transformation of the command line by adding a
 * to the postfork routine; this is called after the fork but before the
 * exec, and it can change the command line by making a copy of the app 
 * structure, changing the command line, and setting the pState structure 
 * to point to this new app (after the fork, these changes are visable only
 * to the forked process).
 *
 * Enhancements: 
 * Allow the code to avoid the remote shell if the process is being created 
 * on the local host. 
 *
 * Handle the user of -l username and -n options to remshell
 * (-n makes stdin /dev/null, necessary for backgrounding).
 * (-l username allows login to hosts where the user's username is 
 * different)
 *
 * Provide an option to add a backslash before any - to deal with the
 * serious bug in the GNU inetutils remote shell programs that process
 * *all* arguments on the remote shell command line, even those for the
 * *program*!
 *
 * To best support the errcodes return from MPI_Comm_spawn, 
 * we need a way to communicate the array of error codes back to the
 * spawn and spawn multiple commands.  Query: how is that done in 
 * PMI?  
 * 
 */

static int AddEnvSetToCmdLine( const char *envName, const char *envValue, 
			       const char **args )
{
    int nArgs = 0;
    static int useCSHFormat = -1;
    
    /* Determine the Shell type the first time*/
    if (useCSHFormat == -1) {
	char *shell = getenv( "SHELL" ), *sname;
	if (shell) {
/* 	    printf( "Shell is %s\n", shell ); */
	    sname = strrchr( shell, '/' );
	    if (!sname) sname = shell;
	    else sname++;
/* 	    printf( "Sname is %s\n", sname ); */
	    if (strcmp( sname, "bash" ) == 0 || strcmp( sname, "sh" ) ||
		strcmp( sname, "ash" ) == 0) useCSHFormat = 0;
	    else 
		useCSHFormat = 1;
	}
	else {
	    /* Default is to assume csh (setenv) format */
	    useCSHFormat = 1;
	}
    }

    if (useCSHFormat) {
	args[nArgs++] = MPIU_Strdup( "setenv" );
	args[nArgs++] = MPIU_Strdup( envName );
	args[nArgs++] = MPIU_Strdup( envValue ); 
	args[nArgs++] = MPIU_Strdup( ";" );
    }
    else {
	char tmpBuf[1024];
	args[nArgs++] = MPIU_Strdup( "export" );
	MPIU_Strncpy( tmpBuf, envName, sizeof(tmpBuf) );
	MPIU_Strnapp( tmpBuf, "=", sizeof(tmpBuf) );
	MPIU_Strnapp( tmpBuf, envValue, sizeof(tmpBuf) );
	args[nArgs++] = MPIU_Strdup( tmpBuf );
	args[nArgs++] = MPIU_Strdup( ";" );
    }
    return nArgs;
}
