/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file contains a simple PMI-based implementation of the name server routines.
 */

#include "mpiimpl.h"
#include "namepub.h"
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

/* style: allow:fprintf:1 sig:0 */   /* For writing the name/service pair */

/* Define the name service handle */
#define MPID_MAX_NAMEPUB 64
struct MPID_NS_Handle { int dummy; };    /* unused for now */

#undef FUNCNAME
#define FUNCNAME MPID_NS_Create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_NS_Create( const MPID_Info *info_ptr, MPID_NS_Handle *handle_ptr )
{
    static struct MPID_NS_Handle nsHandleWithNoData;

    MPIU_UNREFERENCED_ARG(info_ptr);
    /* MPID_NS_Create() should always create a valid handle */
    *handle_ptr = &nsHandleWithNoData;	/* The name service needs no local data */
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Publish
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_NS_Publish( MPID_NS_Handle handle, const MPID_Info *info_ptr, 
                     const char service_name[], const char port[] )
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    MPIU_UNREFERENCED_ARG(info_ptr);
    MPIU_UNREFERENCED_ARG(handle);

#ifdef USE_PMI2_API
    /* release the global CS for PMI calls */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    rc = PMI2_Nameserv_publish(service_name, info_ptr, port);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#else
    rc = PMI_Publish_name( service_name, port );
#endif
    MPIR_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_NAME, "**namepubnotpub", "**namepubnotpub %s", service_name);

fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Lookup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_NS_Lookup( MPID_NS_Handle handle, const MPID_Info *info_ptr,
                    const char service_name[], char port[] )
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    MPIU_UNREFERENCED_ARG(info_ptr);
    MPIU_UNREFERENCED_ARG(handle);

#ifdef USE_PMI2_API
    /* release the global CS for PMI calls */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    rc = PMI2_Nameserv_lookup(service_name, info_ptr, port, MPI_MAX_PORT_NAME);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#else
    rc = PMI_Lookup_name( service_name, port );
#endif
    MPIR_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_NAME, "**namepubnotfound", "**namepubnotfound %s", service_name);

fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Unpublish
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_NS_Unpublish( MPID_NS_Handle handle, const MPID_Info *info_ptr, 
                       const char service_name[] )
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    MPIU_UNREFERENCED_ARG(info_ptr);
    MPIU_UNREFERENCED_ARG(handle);

#ifdef USE_PMI2_API
    /* release the global CS for PMI calls */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    rc = PMI2_Nameserv_unpublish(service_name, info_ptr);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
#else
    rc = PMI_Unpublish_name( service_name );
#endif
    MPIR_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_SERVICE, "**namepubnotunpub", "**namepubnotunpub %s", service_name);

fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Free
int MPID_NS_Free( MPID_NS_Handle *handle_ptr )
{
    /* MPID_NS_Handle is Null */
    MPIU_UNREFERENCED_ARG(handle_ptr);
    return 0;
}

#if 0
/* PMI_Put/Get version */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file contains a simple implementation of the name server routines,
 * using the PMI interface.  
 */
#include "mpiimpl.h"
#include "namepub.h"
#include "pmi.h"

/* Define the name service handle */
struct MPID_NS_Handle
{
    char *kvsname;
};

#undef FUNCNAME
#define FUNCNAME MPID_NS_Create
int MPID_NS_Create( const MPID_Info *info_ptr, MPID_NS_Handle *handle_ptr )
{
    static const char FCNAME[] = "MPID_NS_Create";
    int err;
    int length;
    char *pmi_namepub_kvs;

    *handle_ptr = (MPID_NS_Handle)MPIU_Malloc( sizeof(struct MPID_NS_Handle) );
    /* --BEGIN ERROR HANDLING-- */
    if (!*handle_ptr)
    {
	err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return err;
    }
    /* --END ERROR HANDLING-- */

    err = PMI_KVS_Get_name_length_max(&length);
    /* --BEGIN ERROR HANDLING-- */
    if (err != PMI_SUCCESS)
    {
	err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }
    /* --END ERROR HANDLING-- */

    (*handle_ptr)->kvsname = (char*)MPIU_Malloc(length);
    /* --BEGIN ERROR HANDLING-- */
    if (!(*handle_ptr)->kvsname)
    {
	err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return err;
    }
    /* --END ERROR HANDLING-- */

    pmi_namepub_kvs = getenv("PMI_NAMEPUB_KVS");
    if (pmi_namepub_kvs)
    {
	MPIU_Strncpy((*handle_ptr)->kvsname, pmi_namepub_kvs, length);
    }
    else
    {
	err = PMI_KVS_Get_my_name((*handle_ptr)->kvsname, length);
	/* --BEGIN ERROR HANDLING-- */
	if (err != PMI_SUCCESS)
	{
	    err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	}
	/* --END ERROR HANDLING-- */
    }

    /*printf("namepub kvs: <%s>\n", (*handle_ptr)->kvsname);fflush(stdout);*/
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Publish
int MPID_NS_Publish( MPID_NS_Handle handle, const MPID_Info *info_ptr, 
                     const char service_name[], const char port[] )
{
    static const char FCNAME[] = "MPID_NS_Publish";
    int  err;

    /*printf("publish kvs: <%s>\n", handle->kvsname);fflush(stdout);*/
    err = PMI_KVS_Put(handle->kvsname, service_name, port);
    /* --BEGIN ERROR HANDLING-- */
    if (err != PMI_SUCCESS)
    {
	err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", 0 );
	return err;
    }
    /* --END ERROR HANDLING-- */
    err = PMI_KVS_Commit(handle->kvsname);
    /* --BEGIN ERROR HANDLING-- */
    if (err != PMI_SUCCESS)
    {
	err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", 0 );
	return err;
    }
    /* --END ERROR HANDLING-- */

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Lookup
int MPID_NS_Lookup( MPID_NS_Handle handle, const MPID_Info *info_ptr,
                    const char service_name[], char port[] )
{
    static const char FCNAME[] = "MPID_NS_Lookup";
    int err;

    /*printf("lookup kvs: <%s>\n", handle->kvsname);fflush(stdout);*/
    err = PMI_KVS_Get(handle->kvsname, service_name, port, MPI_MAX_PORT_NAME);
    /* --BEGIN ERROR HANDLING-- */
    if (err != PMI_SUCCESS)
    {
	err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_NAME, "**pmi_kvs_get", 0 );
	return err;
    }
    /* --END ERROR HANDLING-- */

    if (port[0] == '\0')
    {
	return MPI_ERR_NAME;
    }
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Unpublish
int MPID_NS_Unpublish( MPID_NS_Handle handle, const MPID_Info *info_ptr, 
                       const char service_name[] )
{
    static const char FCNAME[] = "MPID_NS_Unpublish";
    int  err;

    /*printf("unpublish kvs: <%s>\n", handle->kvsname);fflush(stdout);*/
    /* This assumes you can put the same key more than once which breaks the PMI specification */
    err = PMI_KVS_Put(handle->kvsname, service_name, "");
    /* --BEGIN ERROR HANDLING-- */
    if (err != PMI_SUCCESS)
    {
	err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put", 0 );
	return err;
    }
    /* --END ERROR HANDLING-- */
    err = PMI_KVS_Commit(handle->kvsname);
    /* --BEGIN ERROR HANDLING-- */
    if (err != PMI_SUCCESS)
    {
	err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_commit", 0 );
	return err;
    }
    /* --END ERROR HANDLING-- */

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Free
int MPID_NS_Free( MPID_NS_Handle *handle_ptr )
{
    static const char FCNAME[] = "MPID_NS_Free";
    int err;

    MPIU_Free( (*handle_ptr)->kvsname );
    MPIU_Free( *handle_ptr );
    *handle_ptr = 0;

    return 0;
}
#endif
