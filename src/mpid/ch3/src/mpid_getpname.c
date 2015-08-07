/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static char processorName[MPI_MAX_PROCESSOR_NAME];
static int  setProcessorName = 0;
static int  processorNameLen = -1;

static inline void setupProcessorName( void );

/*
 * MPID_Get_processor_name()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Get_processor_name
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Get_processor_name(char * name, int namelen, int * resultlen)
{
    int mpi_errno = MPI_SUCCESS;
    /* FIXME: Make thread safe */
    if (!setProcessorName) {
	setupProcessorName( );
	setProcessorName = 1;
    }
    MPIR_ERR_CHKANDJUMP(processorNameLen <= 0, mpi_errno, MPI_ERR_OTHER, "**procnamefailed");

    /* MPIU_Strncpy only copies until (and including) the null,
       unlike strncpy, it does not blank pad.  This is a good thing
       here, because users don't always allocated MPI_MAX_PROCESSOR_NAME
       characters */
    MPIU_Strncpy(name, processorName, namelen );
    if (resultlen)
        *resultlen = processorNameLen;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Here we define an internal routine to get the processor name, based on 
   which system or facilities are available to us */

/* Additional and alternative implmentations of these routines may be
   found in mpich/mpid/ch2/chnodename.c */

/* If we are using sysinfo, we need to make sure that both the 
   routine and headerfile are available */
#if defined(HAVE_SYSINFO)
#if defined(HAVE_SYS_SYSTEMINFO_H)
#include <sys/systeminfo.h>
#else
#ifdef HAVE_SYSINFO
#undef HAVE_SYSINFO
#endif
#endif
#endif

#if defined(HAVE_WINDOWS_H)
/* We prefer this function to gethostname because gethostname requires 
   additional libraries.  */
static inline void setupProcessorName( void )
{
    DWORD size = MPI_MAX_PROCESSOR_NAME;

    /* Use the fully qualified name instead of the short name because the 
       SSPI security functions require the full name */
    if (GetComputerNameEx(ComputerNameDnsFullyQualified, processorName, 
			  &size))
    {
	processorNameLen = (int)strlen(processorName);
    }
}

#elif defined(HAVE_GETHOSTNAME)
static inline void setupProcessorName( void )
{
    if (gethostname(processorName, MPI_MAX_PROCESSOR_NAME) == 0) {
	processorNameLen = (int)strlen( processorName );
    }
}

#elif defined(HAVE_SYSINFO)
static inline void setupProcessorName( void );
{
    if (sysinfo(SI_HOSTNAME, processorName, MPI_MAX_PROCESSOR_NAME) == 0) {
	processorNameLen = (int)strlen( processorName );
    }
}

#else
static inline void setupProcessorName( void );
{
    /* Set the name as the rank of the process */
    MPL_snprintf( processorName, MPI_MAX_PROCESSOR_NAME, "%d", 
		   MPIDI_Process.my_pg_rank );
    processorNameLen = (int)strlen( processorName );
}
#endif
