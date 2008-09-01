/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Support for dynamically loaded libraries */

#include "mpiimpl.h"
#include "mpidll.h"

#ifdef USE_DYNAMIC_LIBRARIES

#if defined(HAVE_DLOPEN) && defined(HAVE_DLFCN_H) && defined(HAVE_DLSYM)
#include <dlfcn.h>

#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#undef FCNAME
#define FCNAME "MPIU_DLL_Open"
int MPIU_DLL_Open( const char libname[], void **handle )
{
    int mpi_errno = MPI_SUCCESS;

    *handle = dlopen( libname, RTLD_LAZY | RTLD_GLOBAL );
    if (!*handle) {
	char fullpath[MAXPATHLEN];
	/* Try again, but prefixing the libname with LIBDIR */
	MPIU_Strncpy( fullpath, MPICH2_LIBDIR, sizeof(fullpath) );
	MPIU_Strnapp( fullpath, "/", sizeof(fullpath));
	MPIU_Strnapp( fullpath, libname, sizeof(fullpath) );
	*handle = dlopen( fullpath, RTLD_LAZY | RTLD_GLOBAL );
    }
    if (!*handle) {
	MPIU_ERR_SET2(mpi_errno,MPI_ERR_OTHER,"**unableToLoadDLL",
		      "**unableToLoadDLL %s %s", libname, dlerror() );
    }
    return mpi_errno;
}

#undef FCNAME
#define FCNAME "MPIU_DLL_FindSym"
int MPIU_DLL_FindSym( void *handle, const char symbol[], void **value )
{
    int mpi_errno = MPI_SUCCESS;
    
    *value = dlsym( handle, symbol );
    if (!*value) {
	MPIU_ERR_SET2(mpi_errno,MPI_ERR_OTHER,"**unableToLoadDLLsym",
		      "**unableToLoadDLLsym %s %s", symbol, dlerror() );
    }
    /* printf( "Symbol %s at location %p\n", symbol, *value ); */
    MPIU_DBG_MSG_FMT(SYSCALL,VERBOSE,
		     (MPIU_DBG_FDEST,"Symbol %s loaded at %p",symbol,*value));

    return mpi_errno;
}
int MPIU_DLL_Close( void *handle )
{
    MPIU_DBG_MSG(SYSCALL,VERBOSE,"Closing DLL");
    dlclose( handle );
    return 0;
}
#endif /* HAVE_FUNC_DLOPEN */

#else /* not USE_DYNAMIC_LIBRARIES */

#undef FCNAME
#define FCNAME "MPIU_DLL_Open"
int MPIU_DLL_Open( const char libname[], void **handle )
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**notimpl" );
    return mpi_errno;
}

#undef FCNAME
#define FCNAME "MPIU_DLL_FindSym"
int MPIU_DLL_FindSym( void *handle, const char symbol[], void **value )
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**notimpl" );
    return mpi_errno;
}
int MPIU_DLL_Close( void *handle )
{
    return 0;
}

#endif /* USE_DYNAMIC_LIBRARIES */
