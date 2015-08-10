/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* ----------------------------------------------------------------------- */
/* A simple resource manager.  
 * This file implements a simple resource manager.  By implementing the 
 * interfaces in this file, other resource managers can be used.
 *
 * The interfaces are:
 * int MPIE_ChooseHosts( ProcessList *plist, int nplist, 
 *                         ProcessTable *ptable )
 *    Given the list of processes in plist, set the host field for each of the 
 *    processes in the ptable (ptable is already allocated)
 *
 * int MPIE_RMProcessArg( int argc, char *argv[], void *extra )
 *    Called by the top-level argument processor for unrecognized arguments;
 *    allows the resource manager to use the command line.  If no command
 *    line options are allowed, this routine simply returns zero.
 *
 */
/* ----------------------------------------------------------------------- */

#include "mpichconf.h"

#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "pmutil.h"
#include "process.h"
#include "rm.h"

#ifndef isascii
#define isascii(c) (((c)&~0x7f)==0)
#endif

/* ----------------------------------------------------------------------- */
/* Determine the hosts                                                     */
/*                                                                         */
/* For each requested process that does not have an assigned host yet,     */
/* use information from a machines file to fill in the choices             */
/* ----------------------------------------------------------------------- */
/* These structures are used as part of the code to assign machines to 
   processes */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Choose the hosts for the processes in the ProcessList.  In
   addition, expand the list into a table with one entry per process.
   
   The function "readDB" is used to read information from a database
   of available machines and return a "machine table" that is used to
   supply hosts.  A default routine, MPIE_ReadMachines, is available.
*/
int MPIE_ChooseHosts( ProcessWorld *pWorld, 
		      MachineTable* (*readDB)(const char *, int, void *), 
		      void *readDBdata )
{
    int i, nNeeded=0;
    MachineTable *mt = 0;
    const char *curMtArch = 0;
    int         curHost = 0;
    ProcessApp   *app;
    ProcessState *pState;

    /* First, determine how many processes require host names */
    app = pWorld->apps;
    while (app) {
	if (!app->pState) {
	    pState = (ProcessState *)MPIU_Malloc( 
		app->nProcess * sizeof(ProcessState) );
	    if (!pState) {
		return -1;
	    }
	    /* Set the defaults (this should be in process.c?) */
	    for (i=0; i<app->nProcess; i++) {
		pState[i].hostname    = 0;
		pState[i].app         = app;
		pState[i].wRank       = -1;  /* Unassigned */
		pState[i].initWithEnv = -1;
		pState[i].pid         = -1;
		pState[i].status      = PROCESS_UNINITIALIZED;
		pState[i].exitStatus.exitReason = EXIT_NOTYET;
	    }
	    app->pState = pState;
	}
	pState = app->pState;
	for (i=0; i<app->nProcess; i++) {
	    if (!pState[i].hostname) nNeeded++;
	}
	app = app->nextApp;
    }
    if (nNeeded == 0) return 0;

    /* Now, for each app, find the hostnames by reading the
       machine table associated with the architecture */
    app = pWorld->apps;
    while (app && nNeeded > 0) {
	int nForApp = 0;

	pState = app->pState;
	for (i=0; i<app->nProcess; i++) {
	    if (!pState[i].hostname) nForApp++;
	}
	
	if (nForApp) {
	    if (!mt || app->arch != curMtArch) {
		/* Only read machines file if we need to, even 
		   with multiple applications */
		if (mt) {
		    MPIE_FreeMachineTable( mt );
		}
		mt = (*readDB)( app->arch, nForApp, readDBdata );
		curMtArch = app->arch;
		curHost   = 0;
		if (!mt) {
		    MPL_error_printf( "Could not find machines for %s\n",
		       app->arch ? app->arch : "default architecture" );
		    return nNeeded;
		}
		/* We might want to consider using localhost for
		 a null arch if there is no machine file. */
	    }

	    /* Now that we have a table, make the assignments */
	    for (i=0; i<app->nProcess; i++) {
		if (!pState[i].hostname) {
		    if (curHost >= mt->nHosts) {
			/* We've run out of systems */
			break;
		    }
		    DBG_PRINTF(("Adding host %s for state %d\n", 
				mt->desc[curHost].hostname, i ));
		    nNeeded --;
		    nForApp--;
		    pState[i].hostname = MPIU_Strdup( mt->desc[curHost].hostname );
		    mt->desc[curHost].np--;
		    if (mt->desc[curHost].np == 0) 
			curHost++;
		}
	    }
	}
	
	app = app->nextApp;
    }
    if (mt) {
	MPIE_FreeMachineTable( mt );
    }
    return nNeeded != 0;   /* Return nonzero on failure */
}

#define MAXLINE 256
#ifndef DEFAULT_MACHINES_PATH
#define DEFAULT_MACHINES_PATH "."
#endif
static const char defaultMachinesPath[] = DEFAULT_MACHINES_PATH;

/* Read the associate machines file for the given architecture, returning
   a table with nNeeded machines.  The format of this file is

   # comments
   hostname
   
   hostname [ : [ nproc ] [ : [ login ] [ : [ netname ] ] ]

   Eventually, we may want to allow a more complex description,
   using key=value pairs

   The files are for the format:

   path/machines.<arch>
   or
   path/machines 
   (if no arch is specified)
   
   The files are found by looking in

   env{MPIEXEC_MACHINES_PATH}/machines.<archname>

   or, if archname is null, 

   env{MPIEXEC_MACHINES_PATH}/machines

   Question: We could set this with a -machinefile and -machinefilepath
   option, and pass this information in through the "data" argument.

   See the MPIE_RMProcessArg routine for a place to put the tests for the
   arguments.
*/

/* These allow a command-line option to override the path and filename
   for the machines file */
static char *machinefilePath = 0;
static char *machinefile = 0;

MachineTable *MPIE_ReadMachines( const char *arch, int nNeeded, 
				 void *data )
{
    FILE *fp=0;
    char buf[MAXLINE+1];
    char machinesfile[PATH_MAX];
    char dirname[PATH_MAX];
    const char *path=getenv("MPIEXEC_MACHINES_PATH");
    MachineTable *mt;
    size_t len;
    int    nFound = 0;
    
    /* Try to open the machines file.  arch may be null, in which 
       case we open the default file */
    /* FIXME: need path and external designation of file names */
    /* Partly done */
    if (!path) path = defaultMachinesPath;

    while (path) {
	char *next_path;
	/* Get next path member */
	next_path = strchr( path, ':' );
	if (next_path) 
	    len = next_path - path;
	else
	    len = strlen(path);
	
	/* Copy path into the file name */
	MPIU_Strncpy( dirname, path, len+1 );

	dirname[len] = 0;

	/* Construct the final path name */
	if (arch) {
	    MPL_snprintf( machinesfile, PATH_MAX, 
			   "%s/machines.%s", dirname, arch );
	}
	else {
	    MPIU_Strncpy( machinesfile, dirname, PATH_MAX );
	    MPIU_Strnapp( machinesfile, "/machines", PATH_MAX );
	}
	DBG_PRINTF( ("Attempting to open %s\n", machinesfile) );
	fp = fopen( machinesfile, "r" );
	if (fp) break;  /* Found one */

	if (next_path) 
	    path = next_path + 1;
	else
	    path = 0;
    }
	
    if (!fp) {
	MPL_error_printf( "Could not open machines file %s\n", machinesfile );
	return 0;
    }
    mt = (MachineTable *)MPIU_Malloc( sizeof(MachineTable) );
    if (!mt) {
	MPL_internal_error_printf( "Could not allocate machine table\n" );
	return 0;
    }
    
    /* This may be larger than needed if the machines file has
       fewer entries than nNeeded */
    mt->desc = (MachineDesc *)MPIU_Malloc( nNeeded * sizeof(MachineDesc) );
    if (!mt->desc) {
	return 0;
    }

    /* Order of fields
       hostname [ : [ nproc ] [ : [ login ] [ : [ netname ] ] ]
    */
    while (nNeeded) {
	char *name=0, *login=0, *netname=0, *npstring=0;
	char *p, *p1;
	if (!fgets( buf, MAXLINE, fp )) {
	    break;
	}
	DBG_PRINTF( ("line: %s", buf) );
	/* Skip comment lines */
	p = buf;
	p[MAXLINE] = 0;
	while (isascii(*p) && isspace(*p)) p++;
	if (*p == '#') continue;

	/* To simplify the handling of the end-of-line, remove any return
	   or newline chars.  Include \r for DOS files */
	p1 = p;
	while (*p1 && (*p1 != '\r' && *p1 != '\n')) p1++;
	*p1 = 0;
	
	/* Parse the line by 
	   setting pointers to the fields
	   replacing : by null
	*/
	name = p;

	/* Skip over the value */
	p1 = p;
	while (*p1 && !isspace(*p1) && *p1 != ':') p1++;
	if (*p1 == ':') *p1++ = 0;

	p = p1;
	while (isascii(*p) && isspace(*p)) p++;
	
	npstring = p;

	/* Skip over the value */
	p1 = p;
	while (*p1 && !isspace(*p1) && *p1 != ':') p1++;
	if (*p1 == ':') *p1++ = 0;

	p = p1;
	while (isascii(*p) && isspace(*p)) p++;
	
	login = p;

	/* Skip over the value */
	p1 = p;
	while (*p1 && !isspace(*p1) && *p1 != ':') p1++;
	if (*p1 == ':') *p1++ = 0;

	p = p1;
	while (isascii(*p) && isspace(*p)) p++;
	
	netname = p;

	/* Skip over the value */
	p1 = p;
	while (*p1 && !isspace(*p1) && *p1 != ':') p1++;
	if (*p1 == ':') *p1++ = 0;
	
	/* Save the names */

	/* Initialize the fields for this new entry */
	mt->desc[nFound].hostname    = MPIU_Strdup( name );
	if (login) 
	    mt->desc[nFound].login   = MPIU_Strdup( login );
	else
	    mt->desc[nFound].login = 0;
	if (npstring) {
	    char *newp;
	    size_t n = strtol( npstring, &newp, 0 );
	    if (newp == npstring) {
		/* This indicates an error in the file.  How do we
		   report that? */
		n = 1;
	    }
            /* Length of hostname will fit in an int. */
	    mt->desc[nFound].np      = (int)n;
	}
	else 
	    mt->desc[nFound].np      = 1;
	if (netname) 
	    mt->desc[nFound].netname = MPIU_Strdup( netname );
	else
	    mt->desc[nFound].netname = 0;

	nFound++;
	nNeeded--;
    }
    mt->nHosts = nFound;
    return mt;	
}

int MPIE_RMProcessArg( int argc, char *argv[], void *extra )
{
    char *cmd;
    int   incr = 0;
    if (strncmp( argv[0], "-machinefile", 12 ) == 0) {
	/* Check for machinefile and machinefilepath */
	cmd = argv[0] + 12;

	if (cmd[0] == 0) {
	    machinefile = MPIU_Strdup( argv[1] );
	    incr = 2;
	}
	else if (strcmp( cmd, "path" ) == 0) {
	    machinefilePath = MPIU_Strdup( argv[1] );
	    incr = 2;
	}
	/* else not an argument for this routine */
    }
    return incr;
}

/* Free the machine table (returned by MPIE_ReadMachines or similar routines) */
int MPIE_FreeMachineTable( MachineTable *mt )
{
    int i;
    for (i=0; i<mt->nHosts; i++) {
	MPIU_Free( mt->desc[i].hostname );
	if (mt->desc[i].login) {
	    MPIU_Free( mt->desc[i].login );
	}
	if (mt->desc[i].netname) {
	    MPIU_Free( mt->desc[i].netname );
	}
    }
    MPIU_Free( mt->desc );
    MPIU_Free( mt );

    return 0;
}
