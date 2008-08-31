/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "mpidll.h"

/*
 * This file implements the CH3 Channel interface in terms of routines
 * loaded from a dynamically-loadable library.  As such, it is a 
 * complete example of the functions required in a channel.
 */

static void *dllhandle = 0;

/* Here is the home for the table of channel functions; initialized to
   keep it from being a "common" symbol */
struct MPIDI_CH3_Funcs MPIU_CALL_MPIDI_CH3 = { 0 };
int *MPIDI_CH3I_progress_completion_count_ptr = 0;

static const char * MPIDI_CH3I_VC_GetStateString( struct MPIDI_VC * );
static int MPIDI_CH3I_NotImpl(void);
static int MPIDI_CH3I_Ignore(void);

#undef FUNCNAME
#define FUNCNAME MPICH_CH3_PreLoad
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PreLoad( void )
{
    int mpi_errno = MPI_SUCCESS;
    char *channelName = 0;
    char *libraryVersion = 0;
    char dllname[256];
    char *shlibExt = MPICH_SHLIB_EXT;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CH3_PRELOAD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CH3_PRELOAD);

    /* Determine the channel to load.  There is a default (selected
       at configure time and an override (provided by an environment
       variable (we could use the PMI interface as well).

       Make sure that the selected DLLs are consistent

       Load the function table, and start by invoking the
       channel's version of this routine 
    */
    
    /* This must be getenv because we haven't initialized *any* state yet. */
    channelName = getenv( "MPICH_CH3CHANNEL" );
    if (!channelName) {
	channelName = MPICH_DEFAULT_CH3_CHANNEL;
    }
    /* Form the basic dll name as libmpich2-ch3-<name>.<dllextension> */
    MPIU_Snprintf( dllname, sizeof(dllname), "libmpich2-ch3-%s.%s", 
		   channelName, shlibExt );
    
    mpi_errno = MPIU_DLL_Open( dllname, &dllhandle );
    if (mpi_errno) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* We should check the version numbers here for consistency */
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_ABIVersion", 
				  (void **)&libraryVersion );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    if (libraryVersion == 0 || *libraryVersion == 0) {
	MPIU_ERR_SETSIMPLE(mpi_errno,MPI_ERR_OTHER,"**nodllversion");
	MPIU_ERR_POP(mpi_errno);
    }
    if (strcmp( libraryVersion, MPICH_CH3ABIVERSION ) != 0) {
	MPIU_ERR_SET2(mpi_errno,MPI_ERR_OTHER,
			  "**dllversionmismatch","**dllversionmismatch %s %s",
			  libraryVersion, MPICH_CH3ABIVERSION );
	MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Init", 
				  (void **)&MPIU_CALL_MPIDI_CH3.Init );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_InitCompleted", 
				  (void **)&MPIU_CALL_MPIDI_CH3.InitCompleted );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Finalize", 
				  (void **)&MPIU_CALL_MPIDI_CH3.Finalize );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_iSend", 
				  (void **)&MPIU_CALL_MPIDI_CH3.iSend );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_iSendv", 
				  (void **)&MPIU_CALL_MPIDI_CH3.iSendv );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_iStartMsg", 
				  (void **)&MPIU_CALL_MPIDI_CH3.iStartMsg );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_iStartMsgv", 
				  (void **)&MPIU_CALL_MPIDI_CH3.iStartMsgv );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3I_Progress", 
				  (void **)&MPIU_CALL_MPIDI_CH3.Progress );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    /*
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Progress_wait", 
				  &MPIU_CALL_MPIDI_CH3.Progress_wait );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    */
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_RMAFnsInit", 
				  (void **)&MPIU_CALL_MPIDI_CH3.RMAFnsInit );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Connection_terminate", 
				  (void **)&MPIU_CALL_MPIDI_CH3.Connection_terminate );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_VC_Init", 
				  (void **)&MPIU_CALL_MPIDI_CH3.VC_Init );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_VC_Destroy", 
				  (void **)&MPIU_CALL_MPIDI_CH3.VC_Destroy );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_PG_Init", 
				  (void **)&MPIU_CALL_MPIDI_CH3.PG_Init );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_PG_Destroy", 
				  (void **)&MPIU_CALL_MPIDI_CH3.PG_Destroy );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_VC_GetStateString", 
				  (void **)&MPIU_CALL_MPIDI_CH3.VC_GetStateString );
    /* If we don't find GetStateString, we provide a default */
    if (mpi_errno) {
	MPIU_CALL_MPIDI_CH3.VC_GetStateString = MPIDI_CH3I_VC_GetStateString;
    }

    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_PortFnsInit", 
				  (void **)&MPIU_CALL_MPIDI_CH3.PortFnsInit );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Connect_to_root", 
				  (void **)&MPIU_CALL_MPIDI_CH3.Connect_to_root );
    /* If we don't find Connect_to_root, set the entry to a routine that
       returns not implemented */
    if (mpi_errno) { 
	MPIU_CALL_MPIDI_CH3.Connect_to_root = 
	    (int (*)(const char *, MPIDI_VC_t **))MPIDI_CH3I_NotImpl;
    }

    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Get_business_card", 
				  (void **)&MPIU_CALL_MPIDI_CH3.Get_business_card );
    /* If we don't find Get_business_card, set the entry to a routine that
       returns not implemented */
    if (mpi_errno) { 
	MPIU_CALL_MPIDI_CH3.Get_business_card = 
	    (int (*)(int,char*,int))MPIDI_CH3I_NotImpl;
    }

    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3I_progress_completion_count", 
				  (void **)&MPIDI_CH3I_progress_completion_count_ptr );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CH3_PRELOAD);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int MPIDI_CH3_Finalize( void )
{
    int mpi_errno = MPIU_CALL(MPIDI_CH3,Finalize());
    MPIU_DLL_Close( dllhandle );
    dllhandle = 0;

    /* Once we close the dll, the structure of function pointers is
       no longer valid */
    /* FIXME: As a workaround for the use of PG_Destroy *after* 
       CH3_Finalize is called, this entry point is set to something
       benign, and the VC_Destroy that PG_Destroy may invoke, is also
       set to ignore. */
    MPIU_CALL_MPIDI_CH3.PG_Destroy = (int (*)(MPIDI_PG_t*))MPIDI_CH3I_Ignore;
    MPIU_CALL_MPIDI_CH3.VC_Destroy = (int (*)(struct MPIDI_VC *))MPIDI_CH3I_Ignore;

    return mpi_errno;
}

/* This routine must be exported beyond ch3 device because ch3 defines
   MPID_Progress_wait as this routine.  We may want to change how this
   is done so that the export is used only in the top-level (above ch3) 
   routines */
#ifdef MPIDI_CH3_Progress_wait
#undef MPIDI_CH3_Progress_wait
#endif
int MPIDI_CH3_Progress_wait(MPID_Progress_state *progress_state)
{
    return MPIU_CALL_MPIDI_CH3.Progress( 1, progress_state );
}
/* This routine must be exported beyond ch3 device because ch3 defines
   MPID_Progress_test as this routine.  We may want to change how this
   is done so that the export is used only in the top-level (above ch3) 
   routines */
#ifdef MPIDI_CH3_Progress_test
#undef MPIDI_CH3_Progress_test
#endif
int MPIDI_CH3_Progress_test(void)
{
    return MPIU_CALL_MPIDI_CH3.Progress( 0, NULL );
}

/*
int MPIDI_CH3_CompareABIVersion( const char expected[] )
{
    return strcmp( expected, CH3ABIVERSION );
}
*/
static const char * MPIDI_CH3I_VC_GetStateString( struct MPIDI_VC *vc )
{
    return "unknown";
}

static int MPIDI_CH3I_NotImpl(void)
{
    return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
				 "Dynanmicallyloadable", __LINE__, 
				 MPI_ERR_OTHER, "**notimpl", 0);
}

static int MPIDI_CH3I_Ignore(void)
{
    return MPI_SUCCESS;
}
