/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* OWNER=gropp */

/* Allow fprintf in informational routines */
/* style: allow:fprintf:8 sig:0 */

#include "pmutilconf.h"
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <ctype.h>

#include "pmutil.h"
#include "process.h"
#include "labelout.h"
#include "env.h"
#include "pmiserv.h"
#include "cmnargs.h"

/* ----------------------------------------------------------------------- */
/* Process options                                                         */
/* The process options steps loads up the processTable with entries,       */
/* including the hostname, for each process.  If no host is specified,     */
/* one will be provided in a subsequent step                               */
/* ----------------------------------------------------------------------- */
/*
-n num - number of processes
-host hostname
-arch architecture name
-wdir working directory (cd to this one BEFORE running executable?)
-path pathlist - use this to find the executable
-soft comma separated triplets (ignore for now)
-file name - implementation-defined specification file
-configfile name - file containing specifications of host/program, 
   one per line, with # as a comment indicator, e.g., the usual
   mpiexec input, but with ":" replaced with a newline.  That is,
   the configfile contains lines with -soft, -n etc.

 In addition, to enable singleton init, mpiexec should allow

   -pmi_args port interface-name key pid

 mpiexec starts, then connects to the interface-name:port for PMI communication
*/

/* Internal routines */
static int getInt( int, int, char *[] );
static int GetIntValue( const char [], int );
static int ReadConfigFile( const char *, ProcessUniverse * );

/*@ MPIE_Args - Process the arguments for mpiexec.  
  
    Input Parameters:
+   argc,argv - Argument count and vector
.   ProcessArg - Routine that is called with any unrecognized argument.  
    Returns 0 if the argument is successfully handled, non-zero otherwise.   
-   extraData - Pointer passed to 'ProcessArg' routine

    Output Parameter:
.   pUniv - The elements of the 'ProcessUniverse' structure are filled in.

    Notes:
    This routine may be called to parse the arguments for an implementation of
    'mpiexec'.  The 'ProcessUniverse' structure is filled in with information
    about each process.

  'MPIE_Args' processes the arguments for 'mpiexec'.  For any argument that 
  is not recognized, it calls the 'ProcessArg' routine, which returns the
  number of arguments that should be skipped.  The 'void * pointer' in
  the call to 'ProcessArg' is filled with the 'extraData' pointer.  If
  'ProcessArg' is null, then any unrecognized argument causes mpiexec to 
  print a help message and exit.  In most cases, this will invoke the
  'MPIE_ArgsCheckForEnv' routine which will check for control of the
  environment variables to be provided to the created processes.  In 
  some cases, 
  
 
  In addition the the arguments specified by the MPI standard, 
  -np is accepted as a synonym for -n and -hostfile is allowed
  to specify the available hosts.
 
  The implementation understands the ':' notation to separate out 
  different executables.  Since no ordering of these arguments is implied,
  other than that the executable comes last, we store the values until
  we see an executable.

  The routine 'mpiexec_usage' may be called to provide usage information 
  if this routine detects an erroneous argument specification.
 @*/
int MPIE_Args( int argc, char *argv[], ProcessUniverse *pUniv, 
		 int (*ProcessArg)( int, char *[], void *), void *extraData )
{
    int         i;
    int         appnum=0;
    int         np=-1;     /* These 6 values are set by command line options */
    const char *host=0;    /* These are the defaults.  When a program name */
    const char *arch=0;    /* is seen, the values in these variables are */
    const char *wdir=0;    /* used to initialize the ProcessState entries */
    const char *path=0;    /* we use np == -1 to detect both -n and -soft */
    const char *soft=0;
    const char *exename=0;
    int        indexOfFirstArg=-1;
    int        curplist = 0; /* Index of current ProcessList element */
    int        optionArgs = 0; /* Keep track of where we got 
				 the options */
    int        optionCmdline = 0;
    ProcessApp *pApp = 0, **nextAppPtr;
    EnvInfo    *appEnv = 0;

    /* FIXME: Get values from the environment first.  Command line options
       override the environment */

    /* Allocate the members of the ProcessUniverse structure */
    pUniv->worlds = (ProcessWorld*) MPIU_Malloc( sizeof(ProcessWorld) );
    pUniv->worlds->nApps     = 0;
    pUniv->worlds->nProcess  = 0;
    pUniv->worlds->nextWorld = 0;
    pUniv->worlds->worldNum  = 0;
    pUniv->worlds->genv      = 0;
    pUniv->nWorlds           = 1;
    pUniv->giveExitInfo      = 0;
    nextAppPtr = &(pUniv->worlds->apps); /* Pointer to set with the next app */

    for (i=1; i<argc; i++) {
	if ( strncmp( argv[i], "-n",  strlen( argv[i] ) ) == 0 ||
	     strncmp( argv[i], "-np", strlen( argv[i] ) ) == 0 ) {
	    np = getInt( i+1, argc, argv );
	    optionArgs = 1;
	    i++;
	}
	else if ( strncmp( argv[i], "-soft", 6 ) == 0 ) {
	    if ( i+1 < argc )
		soft = argv[++i];
	    else {
		mpiexec_usage( "Missing argument to -soft" );
	    }
	    optionArgs = 1;
	}
	else if ( strncmp( argv[i], "-host", 6 ) == 0 ) {
	    if ( i+1 < argc )
		host = argv[++i];
	    else 
		mpiexec_usage( "Missing argument to -host" );
	    optionArgs = 1;
	}
	else if ( strncmp( argv[i], "-arch", 6 ) == 0 ) {
	    if ( i+1 < argc )
		arch = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -arch" );
	    optionArgs = 1;
	}
	else if ( strncmp( argv[i], "-wdir", 6 ) == 0 ) {
	    if ( i+1 < argc )
		wdir = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -wdir" );
	    optionArgs = 1;
	}
	else if ( strncmp( argv[i], "-path", 6 ) == 0 ) {
	    if ( i+1 < argc )
		path = argv[++i];
	    else
		mpiexec_usage( "Missing argument to -path" );
	    optionArgs = 1;
	}
	else if ( strncmp( argv[i], "-configfile", 12 ) == 0) {
	    if ( i+1 < argc ) {
		/* Ignore the other command line arguments */
		ReadConfigFile( argv[++i], pUniv );
	    }
	    else
		mpiexec_usage( "Missing argument to -configfile" );
	    optionCmdline = 1;
	} 
/* Here begins the MPICH2 mpiexec extension for singleton init */	
 	else if ( strncmp( argv[i], "-pmi_args", 8 ) == 0) {
	    if (i+4 < argc ) {
		pUniv->fromSingleton   = 1;
		pUniv->portKey         = MPIU_Strdup( argv[i+3] );
		pUniv->singletonIfname = MPIU_Strdup( argv[i+2] );
		pUniv->singletonPID    = atoi(argv[i+4] );
		pUniv->singletonPort   = atoi(argv[i+1] );
		i += 4;
	    }
	    else 
		mpiexec_usage( "Missing argument to -pmi_args" );
	    optionArgs = 1;
	}
/* Here begin the MPICH2 mpiexec common extensions for 
    -usize n   - Universe size
    -l         - label stdout/err
    -maxtime n - set a timelimit of n seconds
    -exitinfo  - Provide exit code and signal info if there is an abnormal
                 exit (either non-zero, a process died on a signal, or
		 pmi was initialized but not finalized)
    -stdoutbuf=type
    -stderrbuf=type - Control the buffering on stdout and stderr.
                 By default, uses the default Unix choice.  Does *not*
		 control the application; the application may 
		 also need to control buffering.
    -channel=name - Pass a name in the environment variable MPICH_CH3CHANNEL
                 to all processes.  This is a special feature that 
		 supports the ch3 channel.  In the future, we'll allow 
		 implementations of the ADI to provide some hooks for their
		 specific mpiexec. 
*/
	else if (strcmp( argv[i], "-usize" ) == 0) {
	    pUniv->size = getInt( i+1, argc, argv );
	    optionArgs = 1;
	    i++;
	}
	else if (strcmp( argv[i], "-l" ) == 0) {
	    IOLabelSetDefault( 1 );
	    optionArgs = 1;
	}
	else if (strcmp( argv[i], "-maxtime" ) == 0) {
	    pUniv->timeout = getInt( i+1, argc, argv );
	    optionArgs = 1;
	    i++;
	}
	else if (strcmp( argv[i], "-exitinfo" ) == 0) {
	    pUniv->giveExitInfo = 1;
	    optionArgs = 1;
	}
	else if ( strncmp( argv[i], "-stdoutbuf=",  11) == 0) {
	    const char *cmd = argv[i] + 11;
	    MPIE_StdioSetMode( stdout, cmd );
	}
	else if (strncmp( argv[i], "-stderrbuf=", 11 ) == 0) {
	    const char *cmd = argv[i] + 11;
	    MPIE_StdioSetMode( stderr, cmd );
	}
	else if (strncmp( argv[i], "-channel=", 9 ) == 0) {
	    const char *channame = argv[i] + 9;
	    char envstring[256];
	    MPIU_Snprintf( envstring, sizeof(envstring), "MPICH_CH3CHANNEL=%s",
			   channame );
	    MPIE_Putenv( pUniv->worlds, envstring );
	}
/* End of the MPICH2 mpiexec common extentions */

	else if (argv[i][0] != '-') {
	    exename = argv[i];

	    /* if the executable name is relative to the current
	       directory, convert it to an absolute name.  
	       FIXME: Make this optional (MPIEXEC_EXEPATH_ABSOLUTE?) */
	    /* We may not want to do this, if the idea is that that
	       executable should be found in the PATH at the destionation */
	    /* wd = getwd( curdir ) */

	    /* Skip arguments until we hit either the end of the args
	       or a : */
	    i++;
	    indexOfFirstArg = i;
	    while (i < argc && argv[i][0] != ':') i++;
	    if (i == indexOfFirstArg) { 
		/* There really wasn't an argument */
		indexOfFirstArg = -1;
	    }
	    
	    /* Create a new app and add to the app list*/
	    pApp = (ProcessApp*) MPIU_Malloc( sizeof(ProcessApp) );
	    *nextAppPtr = pApp;
	    nextAppPtr = &(pApp->nextApp);
	    pApp->nextApp = 0;
	    pUniv->worlds[0].nApps++;
	    pApp->pWorld = &pUniv->worlds[0];
	    if (appEnv) {
		/* Initialize the env items */
		MPIE_EnvInitData( appEnv->envPairs, 0 );
		MPIE_EnvInitData( appEnv->envNames, 1 );
	    }
	    pApp->env    = appEnv;
	    appEnv       = 0;

	    pApp->pState  = 0;

	    /* Save the properties of this app */
	    pApp->exename  = exename;
	    pApp->arch     = arch;
	    pApp->path     = path;
	    pApp->wdir     = wdir;
	    pApp->hostname = host;
	    if (indexOfFirstArg > 0) {
		pApp->args  = (const char **)(argv + indexOfFirstArg);
		pApp->nArgs = i - indexOfFirstArg;
	    }
	    else {
		pApp->args  = 0;
		pApp->nArgs = 0;
	    }

	    if (soft) {
		/* Set the np to 0 to indicate valid softspec */
		pApp->nProcess = 0;
		if (np > 0) {
		    mpiexec_usage( "-n and -soft may not be used together" );
		}
		MPIE_ParseSoftspec( soft, &pApp->soft );
	    }
	    else {
		if (np == -1) np = 1;
		pApp->nProcess    = np;
		pApp->soft.nelm   = 0;
		pApp->soft.tuples = 0;
		pUniv->worlds[0].nProcess += np;
	    }
	    pApp->myAppNum = appnum++;

	    /* Now, clear all of the values for the next set */
	    host = arch = wdir = path = soft = exename = 0;
	    indexOfFirstArg = -1;
	    np              = -1;
	}
	else {
	    int incr = 0;
	    /* Unrecognized argument.  First check for environment variable 
	       controls */
	    incr = MPIE_ArgsCheckForEnv( argc-i, &argv[i], &pUniv->worlds[0], 
					 &appEnv );
	    if (incr == 0) {
		/* Use the callback routine to handle any unknown arguments
		   before the program name */
		if (ProcessArg) {
		    incr = ProcessArg( argc, argv, extraData );
		}
	    }

	    if (incr) {
		/* increment by one less because the for loop will also
		   increment i */
		i += (incr-1);
	    }
	    else {
		MPIU_Error_printf( "invalid mpiexec argument %s\n", argv[i] );
		mpiexec_usage( NULL );
		return -1;
	    }
	}
    }

    /* Initialize the genv items */
    if (pUniv->worlds->genv) {
	MPIE_EnvInitData( pUniv->worlds->genv->envPairs, 0 );
	MPIE_EnvInitData( pUniv->worlds->genv->envNames, 1 );
    }

    if (optionArgs && optionCmdline) {
	MPIU_Error_printf( "-configfile may not be used with other options\n" );
	return -1;
    }
    return curplist;
}

/*@
  MPIE_CheckEnv - Check the environment for parameters and default values

  Output Parameter:
. pUniv - Process universe structure; some fields are set (see notes)

  Notes:

  @*/
int MPIE_CheckEnv( ProcessUniverse *pUniv, 
		   int (*processEnv)( ProcessUniverse *, void * ), 
		   void *extraData )
{
    int rc = 0;
    const char *s;

    /* A negative universe size is none set */
    pUniv->size    = GetIntValue( "MPIEXEC_UNIVERSE_SIZE", -1 );
    /* A negative timeout is infinite */
    pUniv->timeout = GetIntValue( "MPIEXEC_TIMEOUT", -1 );

    if (getenv( "MPIEXEC_DEBUG" )) {
	/* Any value of MPIEXEC_DEBUG turns on debugging */
	MPIE_Debug = 1;
	PMISetDebug( 1 );
    }

    /* Check for stdio buffering controls.  Set the default to none
       as that preserves the behavior of the user's program.
    */
    s = getenv( "MPIEXEC_STDOUTBUF" );
    if (s) {
	rc = MPIE_StdioSetMode( stdout, s );
    }
    else {
	MPIE_StdioSetMode( stdout, "none" );
    }

    s = getenv( "MPIEXEC_STDERRBUF" );
    if (s) {
	rc = MPIE_StdioSetMode( stderr, s );
    }
    else {
	MPIE_StdioSetMode( stderr, "none" );
    }

    if (processEnv) {
	rc = (*processEnv)( pUniv, extraData );
    }

    return rc;
}

/*@
  MPIE_ArgDescription - Return a pointer to a description of the
  arguments handled by MPIE_Args

  This includes the handling of the env arguments
  @*/
const char *MPIE_ArgDescription( void )
{
    return "-usize <universesize> -maxtime <seconds> -exitinfo -l\\\n\
               -n <numprocs> -soft <softness> -host <hostname> \\\n\
               -wdir <working directory> -path <search path> \\\n\
               -file <filename> -configfile <filename> \\\n\
               -genvnone -genvlist <name1,name2,...> -genv name value\\\n\
               -envnone -envlist <name1,name2,...> -env name value\\\n\
               execname <args>\\\n\
               [ : -n <numprocs> ... execname <args>]\n";
}

/*@ MPIE_PrintProcessUniverse - Debugging routine used to print out the 
    results from MPIE_Args
    
    Input Parameters:
+   fp - File for output
-   pUniv - Process Univers
 @*/
void MPIE_PrintProcessUniverse( FILE *fp, ProcessUniverse *pUniv )
{
    ProcessWorld    *pWorld;
    int              nWorld = 0;

    pWorld = pUniv->worlds;
    while (pWorld) {
	fprintf(fp,"Apps for world %d\n", nWorld );
	MPIE_PrintProcessWorld( fp, pWorld );
	pWorld = pWorld->nextWorld;
	nWorld++;
    }
}

/*@ MPIE_PrintProcessWorld - Print a ProcessWorld structure
    
    Input Parameters:
+   fp - File for output
-   pWorld - Process World
 @*/
void MPIE_PrintProcessWorld( FILE *fp, ProcessWorld *pWorld )
{
    int              j;
    ProcessApp      *pApp;
    ProcessSoftSpec *sSpec;

    pApp   = pWorld->apps;
    while (pApp) {
	fprintf( fp, "App %d:\n\
    exename   = %s\n\
    hostname  = %s\n\
    arch      = %s\n\
    path      = %s\n\
    wdir      = %s\n", pApp->myAppNum, 
		      pApp->exename  ? pApp->exename : "<NULL>", 
		      pApp->hostname ? pApp->hostname : "<NULL>", 
		      pApp->arch     ? pApp->arch     : "<NULL>", 
		      pApp->path     ? pApp->path     : "<NULL>", 
		      pApp->wdir     ? pApp->wdir     : "<NULL>" );
	fprintf(fp, "    args (%d):\n", pApp->nArgs );
	for (j=0; j<pApp->nArgs; j++) {
	    fprintf(fp, "        %s\n", 
			  pApp->args[j] ? pApp->args[j] : "<NULL>" );
	}
	sSpec = &(pApp->soft);
	if (sSpec->nelm > 0) {
	    fprintf(fp, "    Soft spec with %d tuples\n", sSpec->nelm );
	    for (j=0; j<sSpec->nelm; j++) {
		fprintf(fp, "        %d:%d:%d\n", 
			      sSpec->tuples[j][0],
			      sSpec->tuples[j][1],
			      sSpec->tuples[j][2]);
	    }
	}
	else {
	    fprintf(fp, "    n         = %d\n", pApp->nProcess );
	    fprintf(fp, "    No soft spec\n");
	}
	pApp = pApp->nextApp;
    }
    fflush( fp );
}

/* ------------------------------------------------------------------------- */
/* Internal Routines                                                         */
/* ------------------------------------------------------------------------- */

/* Return the int-value of the given argument.  If there is no 
   argument, or it is not a valid int, exit with an error message */
static int getInt( int argnum, int argc, char *argv[] )
{
    char *p;
    long i;

    if (argnum < argc) {
	p = argv[argnum];
	i = strtol( argv[argnum], &p, 0 );
	if (p == argv[argnum]) {
	    MPIU_Error_printf( "Invalid parameter value %s to argument %s\n",
		     argv[argnum], argv[argnum-1] );
	    mpiexec_usage( NULL );
	    /* Does not return */
	}
	return (int)i;
    }
    else {
	MPIU_Error_printf( "Missing argument to %s\n", argv[argnum-1] );
	mpiexec_usage( NULL );
	/* Does not return */
    }
    /* Keep compiler happy */
    return 0;
}

/* FIXME: Move this routine else where; perhaps a pmutil.c? */
/* 
 * Try to get an integer value from the enviroment.  Return the default
 * if the value is not available or invalid
 */
static int GetIntValue( const char name[], int default_val )
{
    const char *env_val;
    int  val = default_val;

    env_val = getenv( name );
    if (env_val) {
#ifdef HAVE_STRTOL
	char *invalid_char; /* Used to detect invalid input */
	val = (int) strtol( env_val, &invalid_char, 0 );
	if (*invalid_char != '\0') val = default_val;
#else
	val = atoi( env_val );
#endif
    }
    return val;
}

/*
 * Process a "soft" specification.  Returns the maximum of the 
 * number of requested processes, or -1 on error
 *   Format is in pseudo BNF:
 *  soft -> element[,element]
 *  element -> number | range
 *  range   -> number:number[:number]
 */
int MPIE_ParseSoftspec( const char *str, ProcessSoftSpec *sspec )
{
    const char *p = str, *p1, *p2;
    int s, e, incr;
    int nelm;
    int maxproc = 1;
    /* First, count the number of commas to preallocate the SoftSpec 
       tuples array */
    nelm = 1;
    p1 = p;
    while ( (p1 = strchr(p1,',')) != NULL ) {
	nelm ++;
	p1++;
    }
    sspec->nelm   = nelm;
    sspec->tuples = (int (*)[3]) MPIU_Malloc( nelm * sizeof(int [3]));

    nelm = 0;
    while ( *p ) {
	p1 = strchr(p,',');
	if (!p1) {
	    /* Use the rest of the string */
	    p1 = p + strlen(p);
	}
	/* Extract the element between p and p1-1 */
	/* FIXME: handle sign, invalid input */
	s = 0; e = 0; incr = 1;
	p2 = p;
	while (p2 < p1 && *p2 != ':') {
	    s = 10 * s + (*p2 - '0');
	    p2++;
	}
	if (*p2 == ':') {
	    /* Keep going (end) */
	    p2++;
	    while (p2 < p1 && *p2 != ':') {
		e = 10 * e + (*p2 - '0');
		p2++;
	    }
	    if (*p2 == ':') {
		/* Keep going (stride) */
		p2++;
		incr = 0;
		while (p2 < p1 && *p2 != ':') {
		    incr = 10 * incr + (*p2 - '0');
		    p2++;
		}
	    }
	}
	else {
	    e = s; 
	}

	/* Save the results */
	sspec->tuples[nelm][0] = s;
	sspec->tuples[nelm][1] = e;
	sspec->tuples[nelm][2] = incr;

	/* FIXME: handle negative increments, and e not s + k incr */
	if (e > maxproc) maxproc = e;
	nelm++;

	p = p1;
	if (*p == ',') p++;
    }
    return maxproc;
}

/*
 * Read a file of mpiexec arguments, with a newline between groups.
 * Initialize the values in plist, and return the number of entries.
 * Return -1 on error.
 */
#define MAXLINEBUF 2048
#define MAXARGV    256
static int LineToArgv( char *buf, char *(argv[]), int maxargc );

static int ReadConfigFile( const char *filename, ProcessUniverse *pUniv)
{
    FILE *fp = 0;
    int curplist = 0;
    char linebuf[MAXLINEBUF];
    char *(argv[MAXARGV]);       /* A kind of dummy argv */
    int  argc, newplist;
    

    fp = fopen( filename, "r" );
    if (!fp) {
	MPIU_Error_printf( "Unable to open configfile %s\n", filename );
	return -1;
    }

    /* Read until we get eof */
    while (fgets( linebuf, MAXLINEBUF, fp )) {
	/* Convert the line into an argv array */
	argc = LineToArgv( linebuf, argv, MAXARGV );
	
	/* Process this argv.  We can use the same routine as for the
	   command line (this allows slightly more than the standard
	   requires for configfile, but the extension (allowing :)
	   is not prohibited by the standard */
	newplist = MPIE_Args( argc, argv, pUniv, 0, 0 );
	if (newplist > 0) 
	    curplist += newplist;
	else 
	    /* An error occurred */
	    break;
    }

    fclose( fp );
    return curplist;
}

/* 
   Convert a line into an array of pointers to the arguments, which are
   all null-terminated.  The argument values copy the values in linebuf 
   so that the line buffer may be reused.
*/
static int LineToArgv( char *linebuf, char *(argv[]), int maxargv )
{
    int argc = 0;
    char *p;

    p = linebuf;
    while (*p) {
	while (isspace(*p)) p++;
	if (argc >= maxargv) {
	    MPIU_Error_printf( "Too many arguments in configfile line\n" );
	    return -1;
	}
	argv[argc] = p;
	/* Skip over the arg and insert a null at end */
	while (*p && !isspace(*p)) p++;

	/* Convert the entry into a copy */
	argv[argc] = MPIU_Strdup( argv[argc] );
	argc++;
	*p++ = 0;
    }
    return 0;
}

/* Set the buffering mode for the specified FILE descriptor */
int MPIE_StdioSetMode( FILE *fp, const char *mode )
{
    int rc = 0;
    /* Set the default to none (makes the buffering mimic the 
       users program) */
    setvbuf( fp, NULL, _IONBF, 0 );
    if (strcmp( mode, "none" ) == 0 || strcmp( mode, "NONE" ) == 0) {
	DBG_PRINTF(("Setting buffer mode to unbuffered\n"));
	setvbuf( fp, NULL, _IONBF, 0 );
    }
    else if (strcmp( mode, "line" ) == 0 || strcmp( mode, "LINE" ) == 0) {
	DBG_PRINTF(("Setting buffer mode to line buffered\n"));
	setvbuf( fp, NULL, _IOLBF, 0 );
    }
    else if (strcmp( mode, "block" ) == 0 || strcmp( mode, "BLOCK" ) == 0) {
	DBG_PRINTF(("Setting buffer mode to block buffered\n"));
	setvbuf( fp, NULL, _IOFBF, 0 );
    }
    else {
	rc = 1;
    }
    return rc;
}
