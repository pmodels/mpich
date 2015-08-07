/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file contains a simple implementation of the name server routines,
 * using a file written to a shared file system to communication the
 * data.  
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mpiimpl.h"
#include "namepub.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : PROCESS_MANAGER
      description : cvars that control the client-side process manager code

cvars:
    - name        : MPIR_CVAR_NAMESERV_FILE_PUBDIR
      category    : PROCESS_MANAGER
      alt-env     : MPIR_CVAR_NAMEPUB_DIR
      type        : string
      default     : NULL
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Sets the directory to use for MPI service publishing in the
        file nameserv implementation.  Allows the user to override
        where the publish and lookup information is placed for
        connect/accept based applications.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* For writing the name/service pair */
/* style: allow:fprintf:1 sig:0 */   

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/* Define the name service handle */
#define MPID_MAX_NAMEPUB 64
struct MPID_NS_Handle { 
    int nactive;                         /* Number of active files */
    int mypid;                           /* My process id */
    char dirname[MAXPATHLEN];            /* Directory for all files */
    char *(filenames[MPID_MAX_NAMEPUB]); /* All created files */
};

/* Create a structure that we will use to remember files created for
   publishing.  */
#undef FUNCNAME
#define FUNCNAME MPID_NS_Create
int MPID_NS_Create( const MPID_Info *info_ptr, MPID_NS_Handle *handle_ptr )
{
    static const char FCNAME[] = "MPID_NS_Create";
    const char *dirname;
    struct stat st;
    int        err, ret;

    *handle_ptr = (MPID_NS_Handle)MPIU_Malloc( sizeof(struct MPID_NS_Handle) );
    /* --BEGIN ERROR HANDLING-- */
    if (!*handle_ptr) {
	err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	return err;
    }
    /* --END ERROR HANDLING-- */
    (*handle_ptr)->nactive = 0;
    (*handle_ptr)->mypid   = getpid();

    /* Get the dirname.  Could use an info value of NAMEPUB_CONTACT */
    dirname = MPIR_CVAR_NAMESERV_FILE_PUBDIR;
    if (!dirname) {
        /* user did not specify a directory, try using HOME */
        ret = MPL_env2str("HOME", &dirname);
        if (!ret) {
            /* HOME not found ; use current directory */
            dirname = ".";
        }
    }

    MPIU_Strncpy( (*handle_ptr)->dirname, dirname, MAXPATHLEN );
    MPIU_Strnapp( (*handle_ptr)->dirname, "/.mpinamepub/", MAXPATHLEN );

    /* Make the directory if necessary */
    /* FIXME : Determine if the directory exists before trying to create it */
    
    if (stat( (*handle_ptr)->dirname, &st ) || !S_ISDIR(st.st_mode) ) {
	/* This mode is rwx by owner only.  */
	if (mkdir( (*handle_ptr)->dirname, 0000700 )) {
	    /* FIXME : An error.  Ignore most ? 
	     For example, ignore EEXIST?  */
	    ;
	}
    }
    
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Publish
int MPID_NS_Publish( MPID_NS_Handle handle, const MPID_Info *info_ptr, 
                     const char service_name[], const char port[] )
{
    static const char FCNAME[] = "MPID_NS_Publish";
    FILE *fp;
    char filename[MAXPATHLEN];
    int  err;

    /* Determine file and directory name.  The file name is from
       the service name */
    MPIU_Strncpy( filename, handle->dirname, MAXPATHLEN );
    MPIU_Strnapp( filename, service_name, MAXPATHLEN );

    /* Add the file name to the known files now, in case there is 
       a failure during open or writing */
    if (handle->nactive < MPID_MAX_NAMEPUB) {
	handle->filenames[handle->nactive++] = MPIU_Strdup( filename );
    }
    else {
	/* --BEGIN ERROR HANDLING-- */
	err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
				    FCNAME, __LINE__, MPI_ERR_OTHER, 
				    "**nomem", 0 );
	return err;
	/* --END ERROR HANDLING-- */
    }

    /* Now, open the file and write out the port name */
    fp = fopen( filename, "w" );
    /* --BEGIN ERROR HANDLING-- */
    if (!fp) {
	char *reason;
	/* Generate a better error message */
	/* Check for errno = 
 	     EACCES (access denied to file or a dir),
	     ENAMETOOLONG (name too long)
	     ENOENT (no such directory)
	     ENOTDIR (a name in the path that should have been a directory
	              wasn't)
	     ELOOP (too many symbolic links in path)
             ENOMEM (insufficient kernel memory available)
	   There are a few others that aren't covered here
	*/
#ifdef HAVE_STRERROR
	reason = strerror( errno );
#else
	/* FIXME : This should use internationalization calls */
	switch (errno) {
	case EACCES:
	    reason = "Access denied to some element of the path";
	    break;
	case ENAMETOOLONG:
	    reason = "File name is too long";
	    break;
	case ENOENT:
	    reason = "A directory specified in the path does not exist";
	    break;
	case ENOTDIR:
	    reason = "A name specified in the path exists, but is not a directory and is used where a directory is required";
	    break;
	case ENOMEM:
	    reason "Insufficient kernel memory available";
	default:
	    MPL_snprintf( rstr, sizeof(rstr), "errno = %d", errno );
	}
#endif
	err = MPIR_Err_create_code( 
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
	    MPI_ERR_OTHER, "**namepubfile",
	    "**namepubfile %s %s %s", service_name, filename, reason );
	return err;
    }
    /* --END ERROR HANDLING-- */
    /* Should also add date? */
    fprintf( fp, "%s\n%d\n", port, handle->mypid );
    fclose( fp );

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Lookup
int MPID_NS_Lookup( MPID_NS_Handle handle, const MPID_Info *info_ptr,
                    const char service_name[], char port[] )
{
    FILE *fp;
    char filename[MAXPATHLEN];
    int  mpi_errno = MPI_SUCCESS;
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPID_NS_Lookup";
#endif
    
    /* Determine file and directory name.  The file name is from
       the service name */
    MPIU_Strncpy( filename, handle->dirname, MAXPATHLEN );
    MPIU_Strnapp( filename, service_name, MAXPATHLEN );

    fp = fopen( filename, "r" );
    if (!fp) {
	/* --BEGIN ERROR HANDLING-- */
	port[0] = 0;
	MPIR_ERR_SET1( mpi_errno, MPI_ERR_NAME, 
		      "**namepubnotpub", "**namepubnotpub %s", service_name );
	/* --END ERROR HANDLING-- */
    }
    else {
	/* The first line is the name, the second is the
	   process that published. We just read the name */
	if (!fgets( port, MPI_MAX_PORT_NAME, fp )) {
	    /* --BEGIN ERROR HANDLING-- */
	    port[0] = 0;
	    MPIR_ERR_SET1( mpi_errno, MPI_ERR_NAME, 
			   "**namepubnotfound", "**namepubnotfound %s", 
			   service_name );
	    /* --END ERROR HANDLING-- */
	}
	else {
	    char *nl;
	    /* Remove the newline, if any.  We use fgets instead of fscanf
	       to allow port names to contain blanks */
	    nl = strchr( port, '\n' );
	    if (nl) *nl = 0;
	    /* printf( "Read %s from %s\n", port, filename ); */
	}
	fclose( fp );
    }
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Unpublish
int MPID_NS_Unpublish( MPID_NS_Handle handle, const MPID_Info *info_ptr, 
                       const char service_name[] )
{
    static const char FCNAME[] = "MPID_NS_Unpublish";
    char filename[MAXPATHLEN];
    int  err;
    int  i;

    /* Remove the file corresponding to the service name */
    /* Determine file and directory name.  The file name is from
       the service name */
    MPIU_Strncpy( filename, handle->dirname, MAXPATHLEN );
    MPIU_Strnapp( filename, service_name, MAXPATHLEN );

    /* Find the filename from the list of published files */
    for (i=0; i<handle->nactive; i++) {
	if (handle->filenames[i] &&
	    strcmp( filename, handle->filenames[i] ) == 0) {
	    /* unlink the file only if we find it */
	    unlink( filename );
	    MPIU_Free( handle->filenames[i] );
	    handle->filenames[i] = 0;
	    break;
	}
    }

    if (i == handle->nactive) {
	/* --BEGIN ERROR HANDLING-- */
	/* Error: this name was not found */
	err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
				    MPI_ERR_SERVICE, "**namepubnotpub",
				    "**namepubnotpub %s", service_name );
	return err;
	/* --END ERROR HANDLING-- */
    }

    /* Later, we can reduce the number of active and compress the list */

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Free
int MPID_NS_Free( MPID_NS_Handle *handle_ptr )
{
    /* static const char FCNAME[] = "MPID_NS_Free"; */
    int i;
    MPID_NS_Handle handle = *handle_ptr;
    
    for (i=0; i<handle->nactive; i++) {
	if (handle->filenames[i]) {
	    /* Remove the file if it still exists */
	    unlink( handle->filenames[i] );
	    MPIU_Free( handle->filenames[i] );
	}
    }
    MPIU_Free( *handle_ptr );
    *handle_ptr = 0;

    return 0;
}


