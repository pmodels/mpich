/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*********************** PMI implementation ********************************/
/*
 * This file implements the client-side of the PMI interface.
 *
 * Note that the PMI client code must not print error messages (except 
 * when an abort is required) because MPI error handling is based on
 * reporting error codes to which messages are attached.  
 *
 * In v2, we should require a PMI client interface to use MPI error codes
 * to provide better integration with MPICH2.  
 */
/***************************************************************************/

#include "pmiconf.h" 

#define PMI_VERSION    1
#define PMI_SUBVERSION 1

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
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef USE_PMI_PORT
#ifndef MAXHOSTNAME
#define MAXHOSTNAME 256
#endif
#endif
/* This should be moved to pmiu for shutdown */
#if defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#include "mpibase.h"            /* Get ATTRIBUTE, some base functions */
/* mpimem includes the definitions for MPIU_Snprintf, MPIU_Malloc, and 
   MPIU_Free */
#include "mpimem.h"

/* Temporary debug definitions */
/* #define DBG_PRINTF(args) printf args ; fflush(stdout) */
#define DBG_PRINTF(args)

#include "pmi.h"
#include "simple_pmiutil.h"
#include "mpi.h"		/* to get MPI_MAX_PORT_NAME */

/* 
   These are global variable used *ONLY* in this file, and are hence
   declared static.
 */


static int PMI_fd = -1;
static int PMI_size = 1;
static int PMI_rank = 0;

/* Set PMI_initialized to 1 for singleton init but no process manager
   to help.  Initialized to 2 for normal initialization.  Initialized
   to values higher than 2 when singleton_init by a process manager.
   All values higher than 1 invlove a PM in some way.
*/
typedef enum { PMI_UNINITIALIZED = 0, 
               SINGLETON_INIT_BUT_NO_PM = 1,
	       NORMAL_INIT_WITH_PM,
	       SINGLETON_INIT_WITH_PM } PMIState;
static PMIState PMI_initialized = PMI_UNINITIALIZED;

/* ALL GLOBAL VARIABLES MUST BE INITIALIZED TO AVOID POLLUTING THE 
   LIBRARY WITH COMMON SYMBOLS */
static int PMI_kvsname_max = 0;
static int PMI_keylen_max = 0;
static int PMI_vallen_max = 0;

static int PMI_debug = 0;
static int PMI_debug_init = 0;    /* Set this to true to debug the init
				     handshakes */
static int PMI_spawned = 0;

/* Function prototypes for internal routines */
static int PMII_getmaxes( int *kvsname_max, int *keylen_max, int *vallen_max );
static int PMII_Set_from_port( int, int );
static int PMII_Connect_to_pm( char *, int );

static int GetResponse( const char [], const char [], int );
static int getPMIFD( int * );

#ifdef USE_PMI_PORT
static int PMII_singinit(void);
static int PMI_totalview = 0;
#endif
static int PMIi_InitIfSingleton(void);
static int accept_one_connection(int);
static char cached_singinit_key[PMIU_MAXLINE];
static char cached_singinit_val[PMIU_MAXLINE];
static char singinit_kvsname[256];

/******************************** Group functions *************************/

int PMI_Init( int *spawned )
{
    char *p;
    int notset = 1;
    int rc;
    
    PMI_initialized = PMI_UNINITIALIZED;
    
    /* FIXME: Why is setvbuf commented out? */
    /* FIXME: What if the output should be fully buffered (directed to file)?
       unbuffered (user explicitly set?) */
    /* setvbuf(stdout,0,_IONBF,0); */
    setbuf(stdout,NULL);
    /* PMIU_printf( 1, "PMI_INIT\n" ); */

    /* Get the value of PMI_DEBUG from the environment if possible, since
       we may have set it to help debug the setup process */
    p = getenv( "PMI_DEBUG" );
    if (p) PMI_debug = atoi( p );

    /* Get the fd for PMI commands; if none, we're a singleton */
    rc = getPMIFD(&notset);
    if (rc) {
	return rc;
    }

    if ( PMI_fd == -1 ) {
	/* Singleton init: Process not started with mpiexec, 
	   so set size to 1, rank to 0 */
	PMI_size = 1;
	PMI_rank = 0;
	*spawned = 0;
	
	PMI_initialized = SINGLETON_INIT_BUT_NO_PM;
	/* 256 is picked as the minimum allowed length by the PMI servers */
	PMI_kvsname_max = 256;
	PMI_keylen_max  = 256;
	PMI_vallen_max  = 256;
	
	return( 0 );
    }

    /* If size, rank, and debug are not set from a communication port,
       use the environment */
    if (notset) {
	if ( ( p = getenv( "PMI_SIZE" ) ) )
	    PMI_size = atoi( p );
	else 
	    PMI_size = 1;
	
	if ( ( p = getenv( "PMI_RANK" ) ) ) {
	    PMI_rank = atoi( p );
	    /* Let the util routine know the rank of this process for 
	       any messages (usually debugging or error) */
	    PMIU_Set_rank( PMI_rank );
	}
	else 
	    PMI_rank = 0;
	
	if ( ( p = getenv( "PMI_DEBUG" ) ) )
	    PMI_debug = atoi( p );
	else 
	    PMI_debug = 0;

	/* Leave unchanged otherwise, which indicates that no value
	   was set */
    }

/* FIXME: Why does this depend on their being a port??? */
/* FIXME: What is this for? */
#ifdef USE_PMI_PORT
    if ( ( p = getenv( "PMI_TOTALVIEW" ) ) )
	PMI_totalview = atoi( p );
    if ( PMI_totalview ) {
	char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
	/* FIXME: This should use a cmd/response rather than a expecting the
	   server to set a value in this and only this case */
	/* FIXME: And it most ceratainly should not happen *before* the
	   initialization handshake */
	PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
	PMIU_parse_keyvals( buf );
	PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
	if ( strncmp( cmd, "tv_ready", PMIU_MAXLINE ) != 0 ) {
	    PMIU_printf( 1, "expecting cmd=tv_ready, got %s\n", buf );
	    return( PMI_FAIL );
	}
    }
#endif

    PMII_getmaxes( &PMI_kvsname_max, &PMI_keylen_max, &PMI_vallen_max );

    /* FIXME: This is something that the PM should tell the process,
       rather than deliver it through the environment */
    if ( ( p = getenv( "PMI_SPAWNED" ) ) )
	PMI_spawned = atoi( p );
    else
	PMI_spawned = 0;
    if (PMI_spawned)
	*spawned = 1;
    else
	*spawned = 0;

    if ( ! PMI_initialized )
	PMI_initialized = NORMAL_INIT_WITH_PM;

    return( 0 );
}

int PMI_Initialized( int *initialized )
{
    /* Turn this into a logical value (1 or 0) .  This allows us
       to use PMI_initialized to distinguish between initialized with
       an PMI service (e.g., via mpiexec) and the singleton init, 
       which has no PMI service */
    *initialized = (PMI_initialized != 0);
    return PMI_SUCCESS;
}

int PMI_Get_size( int *size )
{
    if ( PMI_initialized )
	*size = PMI_size;
    else
	*size = 1;
    return( 0 );
}

int PMI_Get_rank( int *rank )
{
    if ( PMI_initialized )
	*rank = PMI_rank;
    else
	*rank = 0;
    return( 0 );
}

/* 
 * Get_universe_size is one of the routines that needs to communicate
 * with the process manager.  If we started as a singleton init, then
 * we first need to connect to the process manager and acquire the 
 * needed information.
 */
int PMI_Get_universe_size( int *size)
{
    int  err;
    char size_c[PMIU_MAXLINE];

    /* Connect to the PM if we haven't already */
    if (PMIi_InitIfSingleton() != 0) return -1;

    if ( PMI_initialized > SINGLETON_INIT_BUT_NO_PM)  {
	err = GetResponse( "cmd=get_universe_size\n", "universe_size", 0 );
	if (err == PMI_SUCCESS) {
	    PMIU_getval( "size", size_c, PMIU_MAXLINE );
	    *size = atoi(size_c);
	    return( PMI_SUCCESS );
	}
	else return err;
    }
    else
	*size = 1;
    return( PMI_SUCCESS );
}

int PMI_Get_appnum( int *appnum )
{
    int  err;
    char appnum_c[PMIU_MAXLINE];

    if ( PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
	err = GetResponse( "cmd=get_appnum\n", "appnum", 0 );
	if (err == PMI_SUCCESS) {
	    PMIU_getval( "appnum", appnum_c, PMIU_MAXLINE );
	    *appnum = atoi(appnum_c);
	    return( PMI_SUCCESS );
	}
	else return err;
	
    }
    else
	*appnum = -1;

    return( PMI_SUCCESS );
}

int PMI_Barrier( void )
{
    int err = PMI_SUCCESS;

    if ( PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
	err = GetResponse( "cmd=barrier_in\n", "barrier_out", 0 );
    }

    return err;
}

/* Inform the process manager that we're in finalize */
int PMI_Finalize( void )
{
    int err = PMI_SUCCESS;

    if ( PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
	err = GetResponse( "cmd=finalize\n", "finalize_ack", 0 );
	shutdown( PMI_fd, SHUT_RDWR );
	close( PMI_fd );
    }

    return err;
}

int PMI_Abort(int exit_code, const char error_msg[])
{
    PMIU_printf(1, "aborting job:\n%s\n", error_msg);
    MPIU_Exit(exit_code);
    return -1;
}

/************************************* Keymap functions **********************/

/*FIXME: need to return an error if the value of the kvs name returned is 
  truncated because it is larger than length */
/* FIXME: My name should be cached rather than re-acquired, as it is
   unchanging (after singleton init) */
int PMI_KVS_Get_my_name( char kvsname[], int length )
{
    int err;

    if (PMI_initialized == SINGLETON_INIT_BUT_NO_PM) {
	/* Return a dummy name */
	/* FIXME: We need to support a distinct kvsname for each 
	   process group */
	MPIU_Snprintf( kvsname, length, "singinit_kvs_%d_0", (int)getpid() );
	return 0;
    }
    err = GetResponse( "cmd=get_my_kvsname\n", "my_kvsname", 0 );
    if (err == PMI_SUCCESS) {
	PMIU_getval( "kvsname", kvsname, length );
    }
    return err;
}

int PMI_KVS_Get_name_length_max( int *maxlen )
{
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_kvsname_max;
    return PMI_SUCCESS;
}

int PMI_KVS_Get_key_length_max( int *maxlen )
{
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_keylen_max;
    return PMI_SUCCESS;
}

int PMI_KVS_Get_value_length_max( int *maxlen )
{
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_vallen_max;
    return PMI_SUCCESS;
}

int PMI_KVS_Put( const char kvsname[], const char key[], const char value[] )
{
    char buf[PMIU_MAXLINE];
    int  err = PMI_SUCCESS;
    int  rc;

    /* This is a special hack to support singleton initialization */
    if (PMI_initialized == SINGLETON_INIT_BUT_NO_PM) {
	rc = MPIU_Strncpy(cached_singinit_key,key,PMI_keylen_max);
	if (rc != 0) return PMI_FAIL;
	rc = MPIU_Strncpy(cached_singinit_val,value,PMI_vallen_max);
	if (rc != 0) return PMI_FAIL;
	return 0;
    }
    
    rc = MPIU_Snprintf( buf, PMIU_MAXLINE, 
			"cmd=put kvsname=%s key=%s value=%s\n",
			kvsname, key, value);
    if (rc < 0) return PMI_FAIL;
    err = GetResponse( buf, "put_result", 1 );
    return err;
}

int PMI_KVS_Commit( const char kvsname[] ATTRIBUTE((unused)))
{
    /* no-op in this implementation */
    return( 0 );
}

/*FIXME: need to return an error if the value returned is truncated
  because it is larger than length */
int PMI_KVS_Get( const char kvsname[], const char key[], char value[], 
		 int length)
{
    char buf[PMIU_MAXLINE];
    int err = PMI_SUCCESS;
    int  rc;

    /* Connect to the PM if we haven't already.  This is needed in case
       we're doing an MPI_Comm_join or MPI_Comm_connect/accept from
       the singleton init case.  This test is here because, in the way in 
       which MPICH2 uses PMI, this is where the test needs to be. */
    if (PMIi_InitIfSingleton() != 0) return -1;

    rc = MPIU_Snprintf( buf, PMIU_MAXLINE, "cmd=get kvsname=%s key=%s\n", 
			kvsname, key );
    if (rc < 0) return PMI_FAIL;

    err = GetResponse( buf, "get_result", 0 );
    if (err == PMI_SUCCESS) {
	PMIU_getval( "rc", buf, PMIU_MAXLINE );
	rc = atoi( buf );
	if ( rc == 0 ) {
	    PMIU_getval( "value", value, length );
	    return( 0 );
	}
	else {
	    return( -1 );
	}
    }

    return err;
}

/*************************** Name Publishing functions **********************/

int PMI_Publish_name( const char service_name[], const char port[] )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int err;

    if ( PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        MPIU_Snprintf( cmd, PMIU_MAXLINE, 
		       "cmd=publish_name service=%s port=%s\n",
		       service_name, port );
	err = GetResponse( cmd, "publish_result", 0 );
	/* FIXME: This should have used rc and msg */
	if (err == PMI_SUCCESS) {
	    PMIU_getval( "info", buf, PMIU_MAXLINE );
	    if ( strcmp(buf,"ok") != 0 ) {
	        PMIU_printf( 1, "publish failed; reason = %s\n", buf );
	        return( PMI_FAIL );
	    }
	}
    }
    else
    {
	PMIU_printf( 1, "PMI_Publish_name called before init\n" );
	return( PMI_FAIL );
    }

    return( PMI_SUCCESS );
}

int PMI_Unpublish_name( const char service_name[] )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int err = PMI_SUCCESS;

    if ( PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        MPIU_Snprintf( cmd, PMIU_MAXLINE, "cmd=unpublish_name service=%s\n", 
		       service_name );
	err = GetResponse( cmd, "unpublish_result", 0 );
	if (err == PMI_SUCCESS) {
	    PMIU_getval( "info", buf, PMIU_MAXLINE );
	    if ( strcmp(buf,"ok") != 0 ) {
		/* FIXME: Do correct error reporting */
		/*
	        PMIU_printf( 1, "unpublish failed; reason = %s\n", buf );
		*/
	        return( PMI_FAIL );
	    }
	}
    }
    else
    {
	PMIU_printf( 1, "PMI_Unpublish_name called before init\n" );
	return( PMI_FAIL );
    }

    return( PMI_SUCCESS );
}

int PMI_Lookup_name( const char service_name[], char port[] )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int err;

    if ( PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        MPIU_Snprintf( cmd, PMIU_MAXLINE, "cmd=lookup_name service=%s\n", 
		       service_name );
	err = GetResponse( cmd, "lookup_result", 0 );
	if (err == PMI_SUCCESS) {
	    PMIU_getval( "info", buf, PMIU_MAXLINE );
	    if ( strcmp(buf,"ok") != 0 ) {
		/* FIXME: Do correct error reporting */
		/****
	        PMIU_printf( 1, "lookup failed; reason = %s\n", buf );
		****/
	        return( PMI_FAIL );
	    }
	    PMIU_getval( "port", port, MPI_MAX_PORT_NAME );
	}
    }
    else
    {
	PMIU_printf( 1, "PMI_Lookup_name called before init\n" );
	return( PMI_FAIL );
    }

    return( PMI_SUCCESS );
}


/************************** Process Creation functions **********************/

int PMI_Spawn_multiple(int count,
                       const char * cmds[],
                       const char ** argvs[],
                       const int maxprocs[],
                       const int info_keyval_sizes[],
                       const PMI_keyval_t * info_keyval_vectors[],
                       int preput_keyval_size,
                       const PMI_keyval_t preput_keyval_vector[],
                       int errors[])
{
    int  i,rc,argcnt,spawncnt,total_num_processes,num_errcodes_found;
    char buf[PMIU_MAXLINE], tempbuf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    char *lead, *lag;

    /* Connect to the PM if we haven't already */
    if (PMIi_InitIfSingleton() != 0) return -1;

    total_num_processes = 0;

    for (spawncnt=0; spawncnt < count; spawncnt++)
    {
        total_num_processes += maxprocs[spawncnt];

        rc = MPIU_Snprintf(buf, PMIU_MAXLINE, 
			   "mcmd=spawn\nnprocs=%d\nexecname=%s\n",
			   maxprocs[spawncnt], cmds[spawncnt] );
	if (rc < 0) {
	    return PMI_FAIL;
	}

	rc = MPIU_Snprintf(tempbuf, PMIU_MAXLINE,
			   "totspawns=%d\nspawnssofar=%d\n",
			   count, spawncnt+1);

	if (rc < 0) { 
	    return PMI_FAIL;
	}
	rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
	if (rc != 0) {
	    return PMI_FAIL;
	}

        argcnt = 0;
        if ((argvs != NULL) && (argvs[spawncnt] != NULL)) {
            for (i=0; argvs[spawncnt][i] != NULL; i++)
            {
		/* FIXME (protocol design flaw): command line arguments
		   may contain both = and <space> (and even tab!).
		*/
		/* Note that part of this fixme was really a design error -
		   because this uses the mcmd form, the data can be
		   sent in multiple writelines.  This code now takes 
		   advantage of that.  Note also that a correct parser 
		   of the commands will permit any character other than a 
		   new line in the argument, since the form is 
		   argn=<any nonnewline><newline> */
                rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"arg%d=%s\n",
				   i+1,argvs[spawncnt][i]);
		if (rc < 0) {
		    return PMI_FAIL;
		}
                rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
		if (rc != 0) {
		    return PMI_FAIL;
		}
                argcnt++;
		rc = PMIU_writeline( PMI_fd, buf );
		buf[0] = 0;

            }
        }
        rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"argcnt=%d\n",argcnt);
	if (rc < 0) {
	    return PMI_FAIL;
	}
        rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
	if (rc != 0) {
	    return PMI_FAIL;
	}
    
        rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"preput_num=%d\n", 
			   preput_keyval_size);
	if (rc < 0) {
	    return PMI_FAIL;
	}

        rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
	if (rc != 0) {
	    return PMI_FAIL;
	}
        for (i=0; i < preput_keyval_size; i++) {
	    rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"preput_key_%d=%s\n",
			       i,preput_keyval_vector[i].key);
	    if (rc < 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
	    if (rc != 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"preput_val_%d=%s\n",
			       i,preput_keyval_vector[i].val);
	    if (rc < 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
	    if (rc != 0) {
		return PMI_FAIL;
	    }
        } 
        rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"info_num=%d\n", 
			   info_keyval_sizes[spawncnt]);
	if (rc < 0) {
	    return PMI_FAIL;
	}
        rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
	if (rc != 0) {
	    return PMI_FAIL;
	}
	for (i=0; i < info_keyval_sizes[spawncnt]; i++)
	{
	    rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"info_key_%d=%s\n",
			       i,info_keyval_vectors[spawncnt][i].key);
	    if (rc < 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
	    if (rc != 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"info_val_%d=%s\n",
			       i,info_keyval_vectors[spawncnt][i].val);
	    if (rc < 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
	    if (rc != 0) {
		return PMI_FAIL;
	    }
	}

        rc = MPIU_Strnapp(buf, "endcmd\n", PMIU_MAXLINE);
	if (rc != 0) {
	    return PMI_FAIL;
	}
        PMIU_writeline( PMI_fd, buf );
    }

    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf ); 
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "spawn_result", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to spawn :%s:\n", buf );
	return( -1 );
    }
    else {
	PMIU_getval( "rc", buf, PMIU_MAXLINE );
	rc = atoi( buf );
	if ( rc != 0 ) {
	    /****
	    PMIU_getval( "status", tempbuf, PMIU_MAXLINE );
	    PMIU_printf( 1, "pmi_spawn_mult failed; status: %s\n",tempbuf);
	    ****/
	    return( -1 );
	}
    }
    
    PMIU_Assert(errors != NULL);
    if (PMIU_getval( "errcodes", tempbuf, PMIU_MAXLINE )) {
        num_errcodes_found = 0;
        lag = &tempbuf[0];
        do {
            lead = strchr(lag, ',');
            if (lead) *lead = '\0';
            errors[num_errcodes_found++] = atoi(lag);
            lag = lead + 1; /* move past the null char */
            PMIU_Assert(num_errcodes_found <= total_num_processes);
        } while (lead != NULL);
        PMIU_Assert(num_errcodes_found == total_num_processes);
    }
    else {
        /* gforker doesn't return errcodes, so we'll just pretend that means
           that it was going to send all `0's. */
        for (i = 0; i < total_num_processes; ++i) {
            errors[i] = 0;
        }
    }

    return( 0 );
}

/***************** Internal routines not part of PMI interface ***************/

/* to get all maxes in one message */
/* FIXME: This mixes init with get maxes */
static int PMII_getmaxes( int *kvsname_max, int *keylen_max, int *vallen_max )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE], errmsg[PMIU_MAXLINE];
    int err, rc;

    rc = MPIU_Snprintf( buf, PMIU_MAXLINE, 
			"cmd=init pmi_version=%d pmi_subversion=%d\n",
			PMI_VERSION, PMI_SUBVERSION );
    if (rc < 0) {
	return PMI_FAIL;
    }

    rc = PMIU_writeline( PMI_fd, buf );
    if (rc != 0) {
	PMIU_printf( 1, "Unable to write to PMI_fd\n" );
	return PMI_FAIL;
    }
    buf[0] = 0;   /* Ensure buffer is empty if read fails */
    err = PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading initack on %d\n", PMI_fd );
	perror( "Error on readline:" );
	PMI_Abort(-1, "Above error when reading after init" );
    }
    PMIU_parse_keyvals( buf );
    cmd[0] = 0;
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "response_to_init", PMIU_MAXLINE ) != 0 ) {
	MPIU_Snprintf(errmsg, PMIU_MAXLINE, 
		      "got unexpected response to init :%s: (full line = %s)",
		      cmd, buf  );
	PMI_Abort( -1, errmsg );
    }
    else {
	char buf1[PMIU_MAXLINE];
        PMIU_getval( "rc", buf, PMIU_MAXLINE );
        if ( strncmp( buf, "0", PMIU_MAXLINE ) != 0 ) {
            PMIU_getval( "pmi_version", buf, PMIU_MAXLINE );
            PMIU_getval( "pmi_subversion", buf1, PMIU_MAXLINE );
	    MPIU_Snprintf(errmsg, PMIU_MAXLINE, 
			  "pmi_version mismatch; client=%d.%d mgr=%s.%s",
			  PMI_VERSION, PMI_SUBVERSION, buf, buf1 );
	    PMI_Abort( -1, errmsg );
        }
    }
    err = GetResponse( "cmd=get_maxes\n", "maxes", 0 );
    if (err == PMI_SUCCESS) {
	PMIU_getval( "kvsname_max", buf, PMIU_MAXLINE );
	*kvsname_max = atoi( buf );
	PMIU_getval( "keylen_max", buf, PMIU_MAXLINE );
	*keylen_max = atoi( buf );
	PMIU_getval( "vallen_max", buf, PMIU_MAXLINE );
	*vallen_max = atoi( buf );
    }
    return err;
}

/* ----------------------------------------------------------------------- */
/* 
 * This function is used to request information from the server and check
 * that the response uses the expected command name.  On a successful
 * return from this routine, additional PMIU_getval calls may be used
 * to access information about the returned value.
 *
 * If checkRc is true, this routine also checks that the rc value returned
 * was 0.  If not, it uses the "msg" value to report on the reason for
 * the failure.
 */
static int GetResponse( const char request[], const char expectedCmd[],
			int checkRc )
{
    int err, n;
    char *p;
    char recvbuf[PMIU_MAXLINE];
    char cmdName[PMIU_MAXLINE];

    /* FIXME: This is an example of an incorrect fix - writeline can change
       the second argument in some cases, and that will break the const'ness
       of request.  Instead, writeline should take a const item and return
       an error in the case in which it currently truncates the data. */
    err = PMIU_writeline( PMI_fd, (char *)request );
    if (err) {
	return err;
    }
    n = PMIU_readline( PMI_fd, recvbuf, sizeof(recvbuf) );
    if (n <= 0) {
	PMIU_printf( 1, "readline failed\n" );
	return PMI_FAIL;
    }
    err = PMIU_parse_keyvals( recvbuf );
    if (err) {
	PMIU_printf( 1, "parse_kevals failed %d\n", err );
	return err;
    }
    p = PMIU_getval( "cmd", cmdName, sizeof(cmdName) );
    if (!p) {
	PMIU_printf( 1, "getval cmd failed\n" );
	return PMI_FAIL;
    }
    if (strcmp( expectedCmd, cmdName ) != 0) {
	PMIU_printf( 1, "expecting cmd=%s, got %s\n", expectedCmd, cmdName );
	return PMI_FAIL;
    }
    if (checkRc) {
	p = PMIU_getval( "rc", cmdName, PMIU_MAXLINE );
	if ( p && strcmp(cmdName,"0") != 0 ) {
	    PMIU_getval( "msg", cmdName, PMIU_MAXLINE );
	    PMIU_printf( 1, "Command %s failed, reason='%s'\n", 
			 request, cmdName );
	    return PMI_FAIL;
	}
    }

    return err;
}
/* ----------------------------------------------------------------------- */


#ifdef USE_PMI_PORT
/*
 * This code allows a program to contact a host/port for the PMI socket.
 */
#include <errno.h>
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#include <sys/param.h>
#include <sys/socket.h>

/* sockaddr_in (Internet) */
#include <netinet/in.h>
/* TCP_NODELAY */
#include <netinet/tcp.h>

/* sockaddr_un (Unix) */
#include <sys/un.h>

/* defs of gethostbyname */
#include <netdb.h>

/* fcntl, F_GET/SETFL */
#include <fcntl.h>

/* This is really IP!? */
#ifndef TCP
#define TCP 0
#endif

/* stub for connecting to a specified host/port instead of using a 
   specified fd inherited from a parent process */
static int PMII_Connect_to_pm( char *hostname, int portnum )
{
    struct hostent     *hp;
    struct sockaddr_in sa;
    int                fd;
    int                optval = 1;
    int                q_wait = 1;
    
    hp = gethostbyname( hostname );
    if (!hp) {
	PMIU_printf( 1, "Unable to get host entry for %s\n", hostname );
	return -1;
    }
    
    memset( (void *)&sa, 0, sizeof(sa) );
    /* POSIX might define h_addr_list only and node define h_addr */
#ifdef HAVE_H_ADDR_LIST
    memcpy( (void *)&sa.sin_addr, (void *)hp->h_addr_list[0], hp->h_length);
#else
    memcpy( (void *)&sa.sin_addr, (void *)hp->h_addr, hp->h_length);
#endif
    sa.sin_family = hp->h_addrtype;
    sa.sin_port   = htons( (unsigned short) portnum );
    
    fd = socket( AF_INET, SOCK_STREAM, TCP );
    if (fd < 0) {
	PMIU_printf( 1, "Unable to get AF_INET socket\n" );
	return -1;
    }
    
    if (setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, 
		    (char *)&optval, sizeof(optval) )) {
	perror( "Error calling setsockopt:" );
    }

    /* We wait here for the connection to succeed */
    if (connect( fd, (struct sockaddr *)&sa, sizeof(sa) ) < 0) {
	switch (errno) {
	case ECONNREFUSED:
	    PMIU_printf( 1, "connect failed with connection refused\n" );
	    /* (close socket, get new socket, try again) */
	    if (q_wait)
		close(fd);
	    return -1;
	    
	case EINPROGRESS: /*  (nonblocking) - select for writing. */
	    break;
	    
	case EISCONN: /*  (already connected) */
	    break;
	    
	case ETIMEDOUT: /* timed out */
	    PMIU_printf( 1, "connect failed with timeout\n" );
	    return -1;

	default:
	    PMIU_printf( 1, "connect failed with errno %d\n", errno );
	    return -1;
	}
    }

    return fd;
}

static int PMII_Set_from_port( int fd, int id )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int err, rc;

    /* We start by sending a startup message to the server */

    if (PMI_debug) {
	PMIU_printf( 1, "Writing initack to destination fd %d\n", fd );
    }
    /* Handshake and initialize from a port */

    rc = MPIU_Snprintf( buf, PMIU_MAXLINE, "cmd=initack pmiid=%d\n", id );
    if (rc < 0) {
	return PMI_FAIL;
    }
    PMIU_printf( PMI_debug, "writing on fd %d line :%s:\n", fd, buf );
    err = PMIU_writeline( fd, buf );
    if (err) {
	PMIU_printf( 1, "Error in writeline initack\n" );
	return -1;
    }

    /* cmd=initack */
    buf[0] = 0;
    PMIU_printf( PMI_debug, "reading initack\n" );
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading initack on %d\n", fd );
	perror( "Error on readline:" );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp( cmd, "initack" ) ) {
	PMIU_printf( 1, "got unexpected input %s\n", buf );
	return -1;
    }
    
    /* Read, in order, size, rank, and debug.  Eventually, we'll want 
       the handshake to include a version number */

    /* size */
    PMIU_printf( PMI_debug, "reading size\n" );
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading size on %d\n", fd );
	perror( "Error on readline:" );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp(cmd,"set")) {
	PMIU_printf( 1, "got unexpected command %s in %s\n", cmd, buf );
	return -1;
    }
    /* cmd=set size=n */
    PMIU_getval( "size", cmd, PMIU_MAXLINE );
    PMI_size = atoi(cmd);

    /* rank */
    PMIU_printf( PMI_debug, "reading rank\n" );
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading rank on %d\n", fd );
	perror( "Error on readline:" );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp(cmd,"set")) {
	PMIU_printf( 1, "got unexpected command %s in %s\n", cmd, buf );
	return -1;
    }
    /* cmd=set rank=n */
    PMIU_getval( "rank", cmd, PMIU_MAXLINE );
    PMI_rank = atoi(cmd);
    PMIU_Set_rank( PMI_rank );

    /* debug flag */
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading debug on %d\n", fd );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp(cmd,"set")) {
	PMIU_printf( 1, "got unexpected command %s in %s\n", cmd, buf );
	return -1;
    }
    /* cmd=set debug=n */
    PMIU_getval( "debug", cmd, PMIU_MAXLINE );
    PMI_debug = atoi(cmd);

    if (PMI_debug) {
	DBG_PRINTF( ("end of handshake, rank = %d, size = %d\n", 
		    PMI_rank, PMI_size )); 
	DBG_PRINTF( ("Completed init\n" ) );
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
/* 
 * Singleton Init.
 * 
 * MPI-2 allows processes to become MPI processes and then make MPI calls,
 * such as MPI_Comm_spawn, that require a process manager (this is different 
 * than the much simpler case of allowing MPI programs to run with an 
 * MPI_COMM_WORLD of size 1 without an mpiexec or process manager).
 *
 * The process starts when either the client or the process manager contacts
 * the other.  If the client starts, it sends a singinit command and
 * waits for the server to respond with its own singinit command.
 * If the server start, it send a singinit command and waits for the 
 * client to respond with its own singinit command
 *
 * client sends singinit with these required values
 *   pmi_version=<value of PMI_VERSION>
 *   pmi_subversion=<value of PMI_SUBVERSION>
 *
 * and these optional values
 *   stdio=[yes|no]
 *   authtype=[none|shared|<other-to-be-defined>]
 *   authstring=<string>
 *
 * server sends singinit with the same required and optional values as
 * above.
 *
 * At this point, the protocol is now the same in both cases, and has the
 * following components:
 *
 * server sends singinit_info with these required fields
 *   versionok=[yes|no]
 *   stdio=[yes|no]
 *   kvsname=<string>
 *
 * The client then issues the init command (see PMII_getmaxes)
 *
 * cmd=init pmi_version=<val> pmi_subversion=<val>
 *
 * and expects to receive a 
 *
 * cmd=response_to_init rc=0 pmi_version=<val> pmi_subversion=<val> 
 *
 * (This is the usual init sequence).
 *
 */
/* ------------------------------------------------------------------------- */
/* This is a special routine used to re-initialize PMI when it is in 
   the singleton init case.  That is, the executable was started without 
   mpiexec, and PMI_Init returned as if there was only one process.

   Note that PMI routines should not call PMII_singinit; they should
   call PMIi_InitIfSingleton(), which both connects to the process mangager
   and sets up the initial KVS connection entry.
*/

static int PMII_singinit(void)
{
    int pid, rc;
    int singinit_listen_sock, stdin_sock, stdout_sock, stderr_sock;
    const char *newargv[8];
    char charpid[8], port_c[8];
    struct sockaddr_in sin;
    socklen_t len;

    /* Create a socket on which to allow an mpiexec to connect back to
       us */
    sin.sin_family	= AF_INET;
    sin.sin_addr.s_addr	= INADDR_ANY;
    sin.sin_port	= htons(0);    /* anonymous port */
    singinit_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    rc = bind(singinit_listen_sock, (struct sockaddr *)&sin ,sizeof(sin));
    len = sizeof(struct sockaddr_in);
    rc = getsockname( singinit_listen_sock, (struct sockaddr *) &sin, &len ); 
    MPIU_Snprintf(port_c, sizeof(port_c), "%d",ntohs(sin.sin_port));
    rc = listen(singinit_listen_sock, 5);

    PMIU_printf( PMI_debug_init, "Starting mpiexec with %s\n", port_c );

    /* Launch the mpiexec process with the name of this port */
    pid = fork();
    if (pid < 0) {
	perror("PMII_singinit: fork failed");
	exit(-1);
    }
    else if (pid == 0) {
	newargv[0] = "mpiexec";
	newargv[1] = "-pmi_args";
	newargv[2] = port_c;
	/* FIXME: Use a valid hostname */
	newargv[3] = "default_interface";  /* default interface name, for now */
	newargv[4] = "default_key";   /* default authentication key, for now */
	MPIU_Snprintf(charpid, sizeof(charpid), "%d",getpid());
	newargv[5] = charpid;
	newargv[6] = NULL;
	rc = execvp(newargv[0], (char **)newargv);
	perror("PMII_singinit: execv failed");
	PMIU_printf(1, "  This singleton init program attempted to access some feature\n");
	PMIU_printf(1, "  for which process manager support was required, e.g. spawn or universe_size.\n");
	PMIU_printf(1, "  But the necessary mpiexec is not in your path.\n");
	return(-1);
    }
    else
    {
	char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
	char *p;
	int connectStdio = 0;

	/* Allow one connection back from the created mpiexec program */
	PMI_fd =  accept_one_connection(singinit_listen_sock);
	if (PMI_fd < 0) {
	    PMIU_printf( 1, "Failed to establish singleton init connection\n" );
	    return PMI_FAIL;
	}
	/* Execute the singleton init protocol */
	rc = PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
	PMIU_printf( PMI_debug_init, "Singinit: read %s\n", buf );

	PMIU_parse_keyvals( buf );
	PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
	if (strcmp( cmd, "singinit" ) != 0) {
	    PMIU_printf( 1, "unexpected command from PM: %s\n", cmd );
	    return PMI_FAIL;
	}
	p = PMIU_getval( "authtype", cmd, PMIU_MAXLINE );
	if (p && strcmp( cmd, "none" ) != 0) {
	    PMIU_printf( 1, "unsupported authentication method %s\n", cmd );
	    return PMI_FAIL;
	}
	/* p = PMIU_getval( "authstring", cmd, PMIU_MAXLINE ); */
	
	/* If we're successful, send back our own singinit */
	rc = MPIU_Snprintf( buf, PMIU_MAXLINE, 
     "cmd=singinit pmi_version=%d pmi_subversion=%d stdio=yes authtype=none\n",
			PMI_VERSION, PMI_SUBVERSION );
	if (rc < 0) {
	    return PMI_FAIL;
	}
	PMIU_printf( PMI_debug_init, "GetResponse with %s\n", buf );

	rc = GetResponse( buf, "singinit_info", 0 );
	if (rc != 0) {
	    PMIU_printf( 1, "GetResponse failed\n" );
	    return PMI_FAIL;
	}
	p = PMIU_getval( "versionok", cmd, PMIU_MAXLINE );
	if (p && strcmp( cmd, "yes" ) != 0) {
	    PMIU_printf( 1, "Process manager needs a different PMI version\n" );
	    return PMI_FAIL;
	}
	p = PMIU_getval( "stdio", cmd, PMIU_MAXLINE );
	if (p && strcmp( cmd, "yes" ) == 0) {
	    PMIU_printf( PMI_debug_init, "PM agreed to connect stdio\n" );
	    connectStdio = 1;
	}
	p = PMIU_getval( "kvsname", singinit_kvsname, sizeof(singinit_kvsname) );
	PMIU_printf( PMI_debug_init, "kvsname to use is %s\n", 
		     singinit_kvsname );
	
	if (connectStdio) {
	    PMIU_printf( PMI_debug_init, 
			 "Accepting three connections for stdin, out, err\n" );
	    stdin_sock  = accept_one_connection(singinit_listen_sock);
	    dup2(stdin_sock, 0);
	    stdout_sock = accept_one_connection(singinit_listen_sock);
	    dup2(stdout_sock,1);
	    stderr_sock = accept_one_connection(singinit_listen_sock);
	    dup2(stderr_sock,2);
	}
	PMIU_printf( PMI_debug_init, "Done with singinit handshake\n" );
    }
    return 0;
}

/* Promote PMI to a fully initialized version if it was started as
   a singleton init */
static int PMIi_InitIfSingleton(void)
{
    int rc;
    static int firstcall = 1;

    if (PMI_initialized != SINGLETON_INIT_BUT_NO_PM || !firstcall) return 0;

    /* We only try to init as a singleton the first time */
    firstcall = 0;

    /* First, start (if necessary) an mpiexec, connect to it, 
       and start the singleton init handshake */
    rc = PMII_singinit();

    if (rc < 0)
	return(-1);
    PMI_initialized = SINGLETON_INIT_WITH_PM;    /* do this right away */
    PMI_size	    = 1;
    PMI_rank	    = 0;
    PMI_debug	    = 0;
    PMI_spawned	    = 0;

    PMII_getmaxes( &PMI_kvsname_max, &PMI_keylen_max, &PMI_vallen_max );

    /* FIXME: We need to support a distinct kvsname for each 
       process group */
    PMI_KVS_Put( singinit_kvsname, cached_singinit_key, cached_singinit_val );

    return 0;
}

static int accept_one_connection(int list_sock)
{
    int gotit, new_sock;
    struct sockaddr_in from;
    socklen_t len;

    len = sizeof(from);
    gotit = 0;
    while ( ! gotit )
    {
	new_sock = accept(list_sock, (struct sockaddr *)&from, &len);
	if (new_sock == -1)
	{
	    if (errno == EINTR)    /* interrupted? If so, try again */
		continue;
	    else
	    {
		PMIU_printf(1, "accept failed in accept_one_connection\n");
		exit(-1);
	    }
	}
	else
	    gotit = 1;
    }
    return(new_sock);
}

#endif
/* end USE_PMI_PORT */

/* Get the FD to use for PMI operations.  If a port is used, rather than 
   a pre-established FD (i.e., via pipe), this routine will handle the 
   initial handshake.  
*/
static int getPMIFD( int *notset )
{
    char *p;

    /* Set the default */
    PMI_fd = -1;
    
    p = getenv( "PMI_FD" );

    if (p) {
	PMI_fd = atoi( p );
	return 0;
    }

#ifdef USE_PMI_PORT
    p = getenv( "PMI_PORT" );
    if (p) {
	int portnum;
	char hostname[MAXHOSTNAME+1];
	char *pn, *ph;
	int id = 0;

	/* Connect to the indicated port (in format hostname:portnumber) 
	   and get the fd for the socket */
	
	/* Split p into host and port */
	pn = p;
	ph = hostname;
	while (*pn && *pn != ':' && (ph - hostname) < MAXHOSTNAME) {
	    *ph++ = *pn++;
	}
	*ph = 0;

	if (PMI_debug) {
	    DBG_PRINTF( ("Connecting to %s\n", p) );
	}
	if (*pn == ':') {
	    portnum = atoi( pn+1 );
	    /* FIXME: Check for valid integer after : */
	    /* This routine only gets the fd to use to talk to 
	       the process manager. The handshake below is used
	       to setup the initial values */
	    PMI_fd = PMII_Connect_to_pm( hostname, portnum );
	    if (PMI_fd < 0) {
		PMIU_printf( 1, "Unable to connect to %s on %d\n", 
			     hostname, portnum );
		return -1;
	    }
	}
	else {
	    PMIU_printf( 1, "unable to decode hostport from %s\n", p );
	    return PMI_FAIL;
	}

	/* We should first handshake to get size, rank, debug. */
	p = getenv( "PMI_ID" );
	if (p) {
	    id = atoi( p );
	    /* PMII_Set_from_port sets up the values that are delivered
	       by enviroment variables when a separate port is not used */
 	    PMII_Set_from_port( PMI_fd, id );
	    *notset = 0;
	}
	return 0;
    }
#endif

    /* Singleton init case - its ok to return success with no fd set */
    return 0;
}
