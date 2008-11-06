/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file contains a simple mpd-based implementation of the name server 
 * routines, using the mpd daemons to hold the data, but accessing it through 
 * the PMI interface. 
 * Publishing will go to the local mpd; lookup will search the ring, looking 
 * for the data.
 */

#include "mpiimpl.h"
#include "namepub.h"
#include "pmi.h"

/* Define the name service handle.
   Note that the interface requires that we return a non-null handle to an 
   object of this type, even though we don't use it. We use a single,
   statically allocated instance for simplicity. */
#define MPID_MAX_NAMEPUB 64
struct MPID_NS_Handle {
    int foo;			/* unused for now; some compilers object
				   to empty structs */
};    

/* Create a structure that we will use to remember files created for
   publishing.  */
#undef FUNCNAME
#define FUNCNAME MPID_NS_Create
int MPID_NS_Create( const MPID_Info *info_ptr, MPID_NS_Handle *handle_ptr )
{
    static struct MPID_NS_Handle myNShandle;
    *handle_ptr = &myNShandle;	/* The mpd name service needs no local data */
                                /* All will be handled at the mpd through the 
				   PMI interface */
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Publish
int MPID_NS_Publish( MPID_NS_Handle handle, const MPID_Info *info_ptr, 
                     const char service_name[], const char port[] )
{
    int rc;
    static const char FCNAME[] = "MPID_NS_Publish";

    rc = PMI_Publish_name( service_name, port );
    if (rc != PMI_SUCCESS) {
	/* --BEGIN ERROR HANDLING-- */
	/* printf( "PMI_Publish_name failed for %s\n", service_name ); */
	return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_NAME, "**namepubnotpub", "**namepubnotpub %s", service_name );
	/* --END ERROR HANDLING-- */
    }

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Lookup
int MPID_NS_Lookup( MPID_NS_Handle handle, const MPID_Info *info_ptr,
                    const char service_name[], char port[] )
{
    int rc;
    static const char FCNAME[] = "MPID_NS_Lookup";

    rc = PMI_Lookup_name( service_name, port );
    if (rc != PMI_SUCCESS) {
	/* --BEGIN ERROR HANDLING-- */
	/* printf( "PMI_Lookup_name failed for %s\n", service_name ); */
	return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_NAME, "**namepubnotfound", "**namepubnotfound %s", service_name );
	/* --END ERROR HANDLING-- */
    }

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Unpublish
int MPID_NS_Unpublish( MPID_NS_Handle handle, const MPID_Info *info_ptr, 
                       const char service_name[] )
{
    int rc;
    static const char FCNAME[] = "MPID_NS_Unpublish";

    rc = PMI_Unpublish_name( service_name );
    if (rc != PMI_SUCCESS) {
	/* --BEGIN ERROR HANDLING-- */
	/* printf( "PMI_Unpublish_name failed for %s\n", service_name ); */
	return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_NAME, "**namepubnotunpub", "**namepubnotunpub %s", service_name );
	/* --END ERROR HANDLING-- */
    }

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Free
int MPID_NS_Free( MPID_NS_Handle *handle_ptr )
{
    /* MPID_NS_Handle is points to statically allocated storage
       (see MPID_NS_Create) */

    return 0;
}
