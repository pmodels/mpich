/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
/* Needed for getenv */
#include <stdlib.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif

#include "mpe_log.h"
#include "mpe_log_thread.h"

#if !defined( CLOG_NOMPI )
#include "mpi.h"                /* Needed for PMPI routines */
#else
#include "mpi_null.h"
#endif /* Endof if !defined( CLOG_NOMPI ) */

#include "clog.h"
#include "clog_const.h"
#include "clog_util.h"
#include "clog_timer.h"
#include "clog_sync.h"
#include "clog_commset.h"

/* we want to use the PMPI routines instead of the MPI routines for all of 
   the logging calls internal to the mpe_log package */
#ifdef USE_PMPI
#define MPI_BUILD_PROFILING
#include "mpiprof.h"
#endif

extern               CLOG_Uuid_t     CLOG_UUID_NULL;

/* Global variables for MPE logging */
                    CLOG_Stream_t  *CLOG_Stream            = NULL;
                    CLOG_Buffer_t  *CLOG_Buffer            = NULL;
MPEU_DLL_SPEC       CLOG_CommSet_t *CLOG_CommSet           = NULL;
                    int             MPE_Log_hasBeenInit    = 0;
                    int             MPE_Log_hasBeenClosed  = 0;


/*@
    MPE_Init_log - Initialize the logging of events.

    Notes:
    This routine must be called before any of the other MPE logging routines.
    It is a collective call over 'MPI_COMM_WORLD'.
    MPE_Init_log() & MPE_Finish_log() are NOT needed when liblmpe.a is linked
    because MPI_Init() would have called MPE_Init_log() already.
    liblmpe.a will be included in the link path if either "mpicc -mpe=mpilog"
    or "mpecc -mpilog" is used in the link step.

    This function is threadsafe, but
    MPE_Init_log() is expected to be called on only one thread in each
    process.  This thread will be labelled as main thread. 

    Returns:
    Always return MPE_LOG_OK.

.seealso: MPE_Finish_log()
@*/
int MPE_Init_log( void )
{
    MPE_LOG_THREADSTM_DECL

#if !defined( CLOG_NOMPI )
    int           flag;
    PMPI_Initialized(&flag);
    if (!flag) {
        fprintf( stderr, __FILE__":MPE_Init_log() - **** WARNING ****!\n"
                 "\tMPI_Init() has not been called before MPE_Init_log()!\n"
                 "\tMPE logging will call MPI_Init() to get things going.\n"
                 "\tHowever, you are see this message because you're likely\n"
                 "\tmaking an error somewhere, e.g. link with wrong MPE\n"
                 "\tlibrary or make incorrect sequence of MPE logging calls."
                 "\tCheck MPE documentation or README for detailed info.\n" );
        PMPI_Init( NULL, NULL );
    }
#else
    /* Initialize Serial MPI implementation */
    PMPI_Init( NULL, NULL );
#endif

    MPE_LOG_THREAD_INIT
    MPE_LOG_THREADSTM_GET

    MPE_LOG_THREAD_LOCK

    if (!MPE_Log_hasBeenInit || MPE_Log_hasBeenClosed) {
        CLOG_Stream     = CLOG_Open();
        CLOG_Local_init( CLOG_Stream, NULL );
        CLOG_Buffer     = CLOG_Stream->buffer;
        CLOG_CommSet    = CLOG_Buffer->commset;
        MPE_Log_commIDs_intracomm( CLOG_CommSet->IDs4world, THREADID,
                                   CLOG_COMM_WORLD_CREATE,
                                   CLOG_CommSet->IDs4world );
        MPE_Log_commIDs_intracomm( CLOG_CommSet->IDs4world, THREADID,
                                   CLOG_COMM_SELF_CREATE,
                                   CLOG_CommSet->IDs4self );
#if !defined( CLOG_NOMPI )
        /*
           Leave these on so the clog2 file can be distinguished
           if real MPI(i.e. non serial-MPI) is used or not.
        */
        if ( CLOG_Buffer->world_rank == 0 ) {
            CLOG_Buffer_save_constdef( CLOG_Buffer,
                                       CLOG_CommSet->IDs4world, THREADID,
                                       CLOG_EVT_CONST,
                                       MPI_PROC_NULL, "MPI_PROC_NULL" );
            CLOG_Buffer_save_constdef( CLOG_Buffer,
                                       CLOG_CommSet->IDs4world, THREADID,
                                       CLOG_EVT_CONST,
                                       MPI_ANY_SOURCE, "MPI_ANY_SOURCE" );
            CLOG_Buffer_save_constdef( CLOG_Buffer,
                                       CLOG_CommSet->IDs4world, THREADID,
                                       CLOG_EVT_CONST,
                                       MPI_ANY_TAG, "MPI_ANY_TAG" );
        }
#endif
        MPE_Log_hasBeenInit = 1;        /* set MPE_Log as being initialized */
        MPE_Log_hasBeenClosed = 0;
    }

    /* Invoke non-threadsafe version of MPE_Start_log() */
    CLOG_Buffer->status = CLOG_INIT_AND_ON;

    MPE_LOG_THREAD_UNLOCK
    return MPE_LOG_OK;
}

/*@
    MPE_Start_log - Start the logging of events.

    Notes:
    This function is threadsafe.
@*/
int MPE_Start_log( void )
{
    MPE_LOG_THREAD_LOCK

    if (!MPE_Log_hasBeenInit)
        return MPE_LOG_NOT_INITIALIZED;
    CLOG_Buffer->status = CLOG_INIT_AND_ON;

    MPE_LOG_THREAD_UNLOCK
    return MPE_LOG_OK;
}

/*@
    MPE_Stop_log - Stop the logging of events

    Notes:
    This function is threadsafe.
@*/
int MPE_Stop_log( void )
{
    MPE_LOG_THREAD_LOCK

    if (!MPE_Log_hasBeenInit)
        return MPE_LOG_NOT_INITIALIZED;
    CLOG_Buffer->status = CLOG_INIT_AND_OFF;

    MPE_LOG_THREAD_UNLOCK
    return MPE_LOG_OK;
}

/*@
  MPE_Initialized_logging - Indicate whether MPE_Init_log()
                            or MPE_Finish_log() have been called.

    Notes:
    This function is threadsafe.

  Returns:
  0 if MPE_Init_log() has not been called,
  1 if MPE_Init_log() has been called
    but MPE_Finish_log() has not been called,
  and 2 otherwise.
@*/
int MPE_Initialized_logging( void )
{
    return MPE_Log_hasBeenInit + MPE_Log_hasBeenClosed;
}

/*
   The is to log CLOG intracommunicator event (Internal MPE routine)

   This function is NOT threadsafe.
*/
int MPE_Log_commIDs_intracomm( const CLOG_CommIDs_t *commIDs, int local_thread,
                               int comm_etype,
                               const CLOG_CommIDs_t *intracommIDs )
{
    CLOG_Buffer_save_commevt( CLOG_Buffer, commIDs, local_thread, comm_etype,
                              intracommIDs->global_ID,
                              intracommIDs->local_ID,
                              intracommIDs->comm_rank,
                              intracommIDs->world_rank );
    return MPE_LOG_OK;
}


/*
   The is to log CLOG Null communicator event (Internal MPE routine)

   This function is NOT threadsafe.
*/
int MPE_Log_commIDs_nullcomm( const CLOG_CommIDs_t *commIDs, int local_thread,
                              int comm_etype )
{
    CLOG_Buffer_save_commevt( CLOG_Buffer, commIDs, local_thread, comm_etype,
                              CLOG_UUID_NULL, CLOG_COMM_LID_NULL,
                              CLOG_COMM_RANK_NULL, CLOG_COMM_WRANK_NULL );
    return MPE_LOG_OK;
}

/*
   The is to log CLOG intercommunicator event (Internal MPE routine)

   This function is NOT threadsafe.
*/
int MPE_Log_commIDs_intercomm( const CLOG_CommIDs_t *commIDs, int local_thread,
                               int comm_etype,
                               const CLOG_CommIDs_t *intercommIDs )
{
    CLOG_CommIDs_t *local_intracommIDs;
    CLOG_CommIDs_t *remote_intracommIDs;

    CLOG_Buffer_save_commevt( CLOG_Buffer, commIDs, local_thread, comm_etype,
                              intercommIDs->global_ID,
                              intercommIDs->local_ID,
                              intercommIDs->comm_rank,
                              intercommIDs->world_rank );
    /*
       Don't check for local_intracommIDs = NULL or remote_intracommIDs = NULL
       prefer sigfault than fail silently.
    */
    local_intracommIDs  = intercommIDs->next;
    CLOG_Buffer_save_commevt( CLOG_Buffer, commIDs, local_thread,
                              CLOG_COMM_INTRA_LOCAL,
                              local_intracommIDs->global_ID,
                              local_intracommIDs->local_ID,
                              local_intracommIDs->comm_rank,
                              local_intracommIDs->world_rank );
    remote_intracommIDs = local_intracommIDs->next;
    CLOG_Buffer_save_commevt( CLOG_Buffer, commIDs, local_thread,
                              CLOG_COMM_INTRA_REMOTE,
                              remote_intracommIDs->global_ID,
                              remote_intracommIDs->local_ID,
                              remote_intracommIDs->comm_rank,
                              remote_intracommIDs->world_rank );
    return MPE_LOG_OK;
}



/*N MPE_LOG_BYTE_FORMAT
  Notes on storage format control support:
    The format control string is printf like, e.g. "Comment = %s".
    All the MPE %-token storage support is provided by SLOG-2.  That is
    whatever supported by SLOG-2 will be supported by MPE.  Currently,
    the following is supported.

    %s : variable length string, byte buffer size is length of string + 2.

    %h : 2-byte integer, printed as decimal integer, byte buffer size is 2.

    %d : 4-byte integer, printed as decimal integer, byte buffer size is 4.

    %l : 8-byte integer, printed as decimal integer, byte buffer size is 8.

    %x : 4-byte integer, printed as hexadecimal integer, byte buffer size is 4.

    %X : 8-byte integer, printed as hexadecimal integer, byte buffer size is 8.

    %e : 4-byte float, printed as decimal float, byte buffer size is 4.

    %E : 8-byte float, printed as decimal float, byte buffer size is 8.
.n
N*/



/*@
    MPE_Describe_comm_state - Describe attributes of a state
                              with byte informational data
                              in a specified MPI_Comm and on
                              the thread that the function is invoked.

    Input Parameters:
+ comm          - MPI_Comm where this process is part of.
. state_startID - event number for the starting of the state.
. state_finalID - event number for the ending of the state.
. name          - name of the state,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_DESC), 32.
. color         - color of the state,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_COLOR), 24.
- format        - printf style %-token format control string for the state,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_FORMAT), 40.  If format is NULL, it is
                  equivalent to calling MPE_Describe_state().  The fortran
                  interface of this routine considers the zero-length string,
                  "", and single-blank string, " ", as NULL.

    Notes:
    Adds a state definition to the logfile.
    States are added to a logfile by calling 'MPE_Log_comm_event()'
    for the start and end event numbers.

    This function is threadsafe.

.N MPE_LOG_BYTE_FORMAT

.seealso: MPE_Log_get_state_eventIDs()
@*/
int MPE_Describe_comm_state( MPI_Comm comm,
                             int state_startID, int state_finalID,
                             const char *name, const char *color,
                             const char *format )
{
    const CLOG_CommIDs_t *commIDs;
          int             stateID;

    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK

    if (!MPE_Log_hasBeenInit)
        return MPE_LOG_NOT_INITIALIZED;

    commIDs  = CLOG_CommSet_get_IDs( CLOG_CommSet, comm );
    stateID  = CLOG_Get_user_stateID( CLOG_Stream );
    CLOG_Buffer_save_statedef( CLOG_Buffer, commIDs, THREADID,
                               stateID, state_startID, state_finalID,
                               color, name, format );

    MPE_LOG_THREAD_UNLOCK

    return MPE_LOG_OK;
}

/*@
    MPE_Describe_info_state - Describe attributes of a state
                              with byte informational data
                              in MPI_COMM_WORLD.

    Input Parameters:
+ state_startID - event number for the beginning of the state.
. state_finalID - event number for the ending of the state.
. name          - name of the state,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_DESC), 32.
. color         - color of the state,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_COLOR), 24.
- format        - printf style %-token format control string for the state,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_FORMAT), 40.  If format is NULL, it is
                  equivalent to calling MPE_Describe_state().  The fortran
                  interface of this routine considers the zero-length string,
                  "", and single-blank string, " ", as NULL.

    Notes:
    Adds a state definition to the logfile.
    States are added to a logfile by calling 'MPE_Log_event()'
    for the start and end event numbers.  The function is provided
    for backward compatibility purpose.  Users are urged to
    use 'MPE_Describe_comm_state' and 'MPE_Log_comm_event()' instead.

    This function is threadsafe.

.N MPE_LOG_BYTE_FORMAT

.seealso: MPE_Log_get_state_eventIDs().
@*/
int MPE_Describe_info_state( int state_startID, int state_finalID,
                             const char *name, const char *color,
                             const char *format )
{
    return MPE_Describe_comm_state( MPI_COMM_WORLD,
                                    state_startID, state_finalID,
                                    name, color, format );
}

/*
    This is a MPE internal function to describe MPI states.
    It is not meant for user application.
    stateID should be fetched from CLOG_Get_known_stateID()
    i.e, MPE_Log_get_known_stateID()

    This function is NOT threadsafe.
*/
int MPE_Describe_known_state( const CLOG_CommIDs_t *commIDs, int local_thread,
                              int stateID, int state_startID, int state_finalID,
                              const char *name, const char *color,
                              const char *format )
{
    if (!MPE_Log_hasBeenInit)
        return MPE_LOG_NOT_INITIALIZED;

    if ( CLOG_Check_known_stateID( CLOG_Stream, stateID ) != CLOG_BOOL_TRUE ) {
        fprintf( stderr, __FILE__":MPE_Describe_known_state() - \n"
                         "\t""The input stateID, %d, for state %s "
                         "is out of known range [%d..%d].\n"
                         "\t""Use user-space stateID instead.\n",
                         stateID, name, CLOG_KNOWN_STATEID_START,
                         CLOG_USER_STATEID_START-1 );
        fflush( stderr );
        stateID = CLOG_Get_user_stateID( CLOG_Stream );
    }

    CLOG_Buffer_save_statedef( CLOG_Buffer, commIDs, local_thread,
                               stateID, state_startID, state_finalID,
                               color, name, format );

    return MPE_LOG_OK;
}

/*@
    MPE_Describe_state - Describe the attributes of a state
                         without byte informational data in
                         MPI_COMM_WORLD.

    Input Parameters:
+ state_startID - event number for the beginning of the state.
. state_finalID - event number for the ending of the state.
. name          - name of the state,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_DESC), 32.
- color         - color of the state,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_COLOR), 24.

    Notes:
    Adds a state definition to the logfile.
    States are added to a log file by calling 'MPE_Log_event()'
    for the start and end event numbers.  The function is provided
    for backward compatibility purpose.  Users are urged to
    use 'MPE_Describe_comm_state()' and 'MPE_Log_comm_event()' instead.

    This function is threadsafe.

.seealso: MPE_Log_get_state_eventIDs(), MPE_Describe_comm_state()
@*/
int MPE_Describe_state( int state_startID, int state_finalID,
                        const char *name, const char *color )
{
    return MPE_Describe_comm_state( MPI_COMM_WORLD,
                                    state_startID, state_finalID,
                                    name, color, NULL );
}


/*@
    MPE_Describe_comm_event - Describe the attributes of an event
                              with byte informational data in
                              a specified MPI_Comm and on
                              the thread that the function is invoked.

    Input Parameters:
+ comm         - MPI_Comm where this process is part of.
. eventID      - event number for the event.
. name         - name of the event,
                 the maximum length of the NULL-terminated string is,
                 sizeof(CLOG_DESC), 32.
. color        - color of the event,
                 the maximum length of the NULL-terminated string is,
                 sizeof(CLOG_COLOR), 24.
- format       - printf style %-token format control string for the event,
                 the maximum length of the NULL-terminated string is,
                 sizeof(CLOG_FORMAT), 40.  If format is NULL, it is
                 equivalent to calling MPE_Describe_event(). The fortran
                 interface of this routine considers the zero-length string,
                 "", and single-blank string, " ", as NULL.

    Notes:
    Adds a event definition to the logfile.

    This function is threadsafe.

.N MPE_LOG_BYTE_FORMAT

.seealso: MPE_Log_get_solo_eventID()
@*/
int MPE_Describe_comm_event( MPI_Comm comm, int eventID,
                             const char *name, const char *color,
                             const char *format )
{
    const CLOG_CommIDs_t *commIDs;

    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK

    if (!MPE_Log_hasBeenInit)
        return MPE_LOG_NOT_INITIALIZED;

    commIDs  = CLOG_CommSet_get_IDs( CLOG_CommSet, comm );
    CLOG_Buffer_save_eventdef( CLOG_Buffer, commIDs, THREADID,
                               eventID, color, name, format );

    MPE_LOG_THREAD_UNLOCK

    return MPE_LOG_OK;
}

/*@
    MPE_Describe_info_event - Describe the attributes of an event
                              with byte informational data in
                              MPI_COMM_WORLD.
    
    Input Parameters:
+ eventID       - event number for the event.
. name          - name of the event,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_DESC), 32.
. color         - color of the event,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_COLOR), 24.
- format        - printf style %-token format control string for the event,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_FORMAT), 40.  If format is NULL, it is
                  equivalent to calling MPE_Describe_event(). The fortran
                  interface of this routine considers the zero-length string,
                  "", and single-blank string, " ", as NULL.

    Notes:
    Adds a event definition to the logfile. The function is provided
    for backward compatibility purpose.  Users are urged to
    use 'MPE_Describe_comm_event' instead.

    This function is threadsafe.

.N MPE_LOG_BYTE_FORMAT

.seealso: MPE_Log_get_solo_eventID(), MPE_Describe_comm_event() 
@*/
int MPE_Describe_info_event( int eventID,
                             const char *name, const char *color,
                             const char *format )
{
    return MPE_Describe_comm_event( MPI_COMM_WORLD,
                                    eventID, name, color, format );
}

/*
    This is a MPE internal function to describe MPI events.
    It is not meant for user application.
    eventID should be fetched from CLOG_Get_known_solo_eventID()
    i.e, MPE_Log_get_known_solo_eventID()

    This function is NOT threadsafe.
*/
int MPE_Describe_known_event( const CLOG_CommIDs_t *commIDs, int local_thread,
                              int eventID,
                              const char *name, const char *color,
                              const char *format )
{
    if (!MPE_Log_hasBeenInit)
        return MPE_LOG_NOT_INITIALIZED;

    CLOG_Buffer_save_eventdef( CLOG_Buffer, commIDs, local_thread,
                               eventID, color, name, format );

    return MPE_LOG_OK;
}

/*@
   MPE_Describe_event - Describe the attributes of an event
                        without byte informational data in
                        MPI_COMM_WORLD.

    Input Parameters:
+ eventID       - event number for the event.
. name          - name of the event,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_DESC), 32.
- color         - color of the event,
                  the maximum length of the NULL-terminated string is,
                  sizeof(CLOG_COLOR), 24.

    Notes:
    Adds a event definition to the logfile.  The function is provided
    for backward compatibility purpose.  Users are urged to
    use 'MPE_Describe_comm_event' instead.

    This function is threadsafe.

.seealso: MPE_Log_get_solo_eventID(), MPE_Describe_comm_event()
@*/
int MPE_Describe_event( int eventID,
                        const char *name, const char *color )
{
    return MPE_Describe_comm_event( MPI_COMM_WORLD,
                                    eventID, name, color, NULL );
}

/*@
  MPE_Log_get_event_number - Get an unused event number.

  Returns:
  A value that can be used in MPE_Describe_info_event() and
  MPE_Describe_info_state() which define an event or state not used before.  

  Notes: 
  This routine is provided to ensure that users are 
  using unique event numbers.  It relies on all packages using this
  routine.
  *** The function is deprecated, its use is strongly discouraged.
      The function has been replaced by
   MPE_Log_get_state_eventIDs() and MPE_Log_get_solo_eventID(). ***

  This function is threadsafe.
@*/
int MPE_Log_get_event_number( void )
{
    int  evtID;

    MPE_LOG_THREAD_LOCK

    evtID = CLOG_Get_user_eventID( CLOG_Stream );

    MPE_LOG_THREAD_UNLOCK
    return evtID;
}

/*@
    MPE_Log_get_state_eventIDs - Get a pair of event numbers to be used to
                                 define STATE drawable through
                                 MPE_Describe_state().

    Input/Output Parameters:
+ statedef_startID  - starting eventID for the definition of state drawable.
- statedef_finalID  - ending eventID for the definition of state drawable.

  Notes: 
  This routine is provided to ensure that users are 
  using unique event numbers.  It relies on all packages using this
  routine.

  This function is threadsafe.
@*/
int MPE_Log_get_state_eventIDs( int *statedef_startID,
                                int *statedef_finalID )
{
    MPE_LOG_THREAD_LOCK

    *statedef_startID = CLOG_Get_user_eventID( CLOG_Stream );
    *statedef_finalID = CLOG_Get_user_eventID( CLOG_Stream );

    MPE_LOG_THREAD_UNLOCK
    return MPE_LOG_OK;
}

/*@
    MPE_Log_get_solo_eventID - Get a single event number to be used to
                               define EVENT drawable through
                               MPE_Describe_event().

    Input/Output Parameters:
. eventdef_eventID  - eventID for the definition of event drawable.

  Notes:
  This routine is provided to ensure that users are 
  using unique event numbers.  It relies on all packages using this
  routine.

  This function is threadsafe.
@*/
int MPE_Log_get_solo_eventID( int *eventdef_eventID )
{
    MPE_LOG_THREAD_LOCK

    *eventdef_eventID = CLOG_Get_user_solo_eventID( CLOG_Stream );

    MPE_LOG_THREAD_UNLOCK
    return MPE_LOG_OK;
}

/*
    This is a MPE internal function in defining MPE_state[] evtID components.
    It is not meant for user application.

    This function is NOT threadsafe.
*/
int MPE_Log_get_known_eventID( void )
{
    return CLOG_Get_known_eventID( CLOG_Stream );
}

/*
    This is a MPE internal function in defining MPE_Event[] evtID components.
    It is not meant for user application.

    This function is NOT threadsafe.
*/
int MPE_Log_get_known_solo_eventID( void )
{
    return CLOG_Get_known_solo_eventID( CLOG_Stream );
}

/*
    This is a MPE internal function in defining MPE_state[] stateID component.
    It is not meant for user application.
*/
int MPE_Log_get_known_stateID( void )
{
    return CLOG_Get_known_stateID( CLOG_Stream );
}

/*
    This is a MPE internal function in defining MPI logging wrappers.
    It is not meant for user application.

    This function is NOT threadsafe.
*/
int MPE_Log_commIDs_send( const CLOG_CommIDs_t *commIDs, int local_thread,
                          int other_party, int tag, int size )
{
    if (other_party != MPI_PROC_NULL)
        CLOG_Buffer_save_msgevt( CLOG_Buffer, commIDs, local_thread,
                                 CLOG_EVT_SENDMSG, tag, other_party, size );
    return MPE_LOG_OK;
}

/*@
    MPE_Log_comm_send - Log the send event of a message within
                        a specified MPI_Comm (on the calling thread
                        where the send event takes place).

    Input Parameters:
+ comm          - MPI_Comm where this process is part of.
. other_party   - the rank of the other party, i.e. receive event's rank.
. tag           - message tag ID.
- size          - message size in byte.

    Notes:
    This function is threadsafe.
@*/
int MPE_Log_comm_send( MPI_Comm comm, int other_party, int tag, int size )
{
    const CLOG_CommIDs_t *commIDs;
          int             ierr;

    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK

    commIDs  = CLOG_CommSet_get_IDs( CLOG_CommSet, comm );
    ierr = MPE_Log_commIDs_send( commIDs, THREADID,
                                 other_party, tag, size );

    MPE_LOG_THREAD_UNLOCK
    return ierr;
}

/*@
    MPE_Log_send - Log the send event of a message within MPI_COMM_WORLD.
                   (on the calling thread where send event takes place)

    Input Parameters:
+ other_party   - the rank of the other party, i.e. receive event's rank.
. tag           - message tag ID.
- size          - message size in byte.

    Notes:
    This function is threadsafe.
@*/
int MPE_Log_send( int other_party, int tag, int size )
{
    int   ierr;

    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK

    ierr = MPE_Log_commIDs_send( CLOG_CommSet->IDs4world, THREADID,
                                 other_party, tag, size );

    MPE_LOG_THREAD_UNLOCK
    return ierr;
}

/*
    This is a MPE internal function in defining MPI logging wrappers.
    It is not meant for user application.

    This function is NOT threadsafe.
*/
int MPE_Log_commIDs_receive( const CLOG_CommIDs_t *commIDs, int local_thread,
                             int other_party, int tag, int size )
{
    if (other_party != MPI_PROC_NULL)
        CLOG_Buffer_save_msgevt( CLOG_Buffer, commIDs, local_thread,
                                 CLOG_EVT_RECVMSG, tag, other_party, size );
    return MPE_LOG_OK;
}

/*@
    MPE_Log_comm_receive - log the receive event of a message within
                           a specified MPI_Comm (on the calling thread
                           where receive event takes place)

    Input Parameters:
+ comm          - MPI_Comm where this process is part of.
. other_party   - the rank of the other party, i.e. send event's rank.
. tag           - message tag ID.
- size          - message size in byte.

    Notes:
    This function is threadsafe.
@*/
int MPE_Log_comm_receive( MPI_Comm comm, int other_party, int tag, int size )
{
    const CLOG_CommIDs_t *commIDs;
          int             ierr;

    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK

    commIDs  = CLOG_CommSet_get_IDs( CLOG_CommSet, comm );
    ierr = MPE_Log_commIDs_receive( commIDs, THREADID,
                                    other_party, tag, size );

    MPE_LOG_THREAD_UNLOCK
    return ierr;
}

/*@
    MPE_Log_receive - log the receive event of a message within MPI_COMM_WORLD.
                      (on the calling thread where send event takes place)

    Input Parameters:
+ other_party   -  the rank of the other party, i.e. send event's rank.
. tag           -  message tag ID.
- size          -  message size in byte.

    Notes:
    This function is threadsafe.
@*/
int MPE_Log_receive( int other_party, int tag, int size )
{
    int   ierr;

    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK

    ierr = MPE_Log_commIDs_receive( CLOG_CommSet->IDs4world, THREADID,
                                    other_party, tag, size );

    MPE_LOG_THREAD_UNLOCK
    return ierr;
}

/*@
    MPE_Log_pack - pack the informational data into the byte buffer to
                   be stored in a infomational event.  The routine will
                   byteswap the data if it is invoked on a small endian
                   machine.

    Output Parameters:
+ bytebuf    - output byte buffer which is of sizeof(MPE_LOG_BYTES),
               i.e. 32 bytes.  For C, bytebuf could be of type
               MPE_LOG_BYTES.  For Fortran, bytebuf could be of
               type 'character*32'.
- position   - an offset measured from the beginning of the bytebuf.
               On input, data will be written to the offset position.
               On Output, position will be updated to reflect the next
               available position in the byte buffer.
             
   
    Input Parameters:
+ tokentype  - a character token type indicator, currently supported tokens
               are 's', 'h', 'd', 'l', 'x', 'X', 'e' and 'E'.
. count      - the number of continuous storage units as indicated by
               tokentype.
- data       - pointer to the beginning of the storage units being copied.

.N MPE_LOG_BYTE_FORMAT

@*/
int MPE_Log_pack( MPE_LOG_BYTES bytebuf, int *position,
                  char tokentype, int count, const void *data )
{
    void *vptr;
    int   tot_sz;
    vptr = ( (char *) bytebuf + *position );
    /* 
     * Do the byte swapping here for now.
     * Moving byteswapping to clog2TOslog2 convertor should speed up logging.
     */
    switch (tokentype) {
        case 's':  /* STR */
            tot_sz = sizeof( CLOG_int16_t ) + count;
            if ( *position + tot_sz <= sizeof( MPE_LOG_BYTES ) ) {
                *((CLOG_int16_t *) vptr) = (CLOG_int16_t) count;
#if !defined( WORDS_BIGENDIAN )
                CLOG_Util_swap_bytes( vptr, sizeof( CLOG_int16_t ) , 1 );
#endif
                vptr = (void *)( (char *) vptr + sizeof( CLOG_int16_t ) );
                memcpy( vptr, data, count );
                *position += tot_sz;
                return MPE_LOG_OK;
            }
            break;
        case 'h':  /* INT2 */
            tot_sz = count * 2;
            if ( *position + tot_sz <= sizeof( MPE_LOG_BYTES ) ) {
                memcpy( vptr, data, tot_sz );
#if !defined( WORDS_BIGENDIAN ) 
                CLOG_Util_swap_bytes( vptr, 2 , count );
#endif
                *position += tot_sz;
                return MPE_LOG_OK;
            }
            break;
        case 'd': /* INT4 */
        case 'x': /* BYTE4 */
        case 'e': /* FLT4 */
            tot_sz = count * 4;
            if ( *position + tot_sz <= sizeof( MPE_LOG_BYTES ) ) {
                memcpy( vptr, data, tot_sz );
#if !defined( WORDS_BIGENDIAN )
                CLOG_Util_swap_bytes( vptr, 4, count );
#endif
                *position += tot_sz;
                return MPE_LOG_OK;
            }
            break;
        case 'l': /* INT8 */
        case 'X': /* BYTE8 */
        case 'E': /* FLT8 */
            tot_sz = count * 8;
            if ( *position + tot_sz <= sizeof( MPE_LOG_BYTES ) ) {
                memcpy( vptr, data, tot_sz );
#if !defined( WORDS_BIGENDIAN )
                CLOG_Util_swap_bytes( vptr, 8, count );
#endif
                *position += tot_sz;
                return MPE_LOG_OK;
            }
            break;
        default:
            fprintf( stderr, "MPE_Log_pack(): Unknown tokentype %c\n",
                             tokentype );
    }
    return MPE_LOG_PACK_FAIL;
}

/*
    This is a MPE internal function in defining MPI logging wrappers.
    It is not meant for user application.

    This function is NOT threadsafe.
*/
int MPE_Log_commIDs_event( const CLOG_CommIDs_t *commIDs, int local_thread,
                           int event, const char *bytebuf )
{
    if ( bytebuf )
        CLOG_Buffer_save_cargoevt( CLOG_Buffer, commIDs, local_thread,
                                   event, bytebuf );
    else
        CLOG_Buffer_save_bareevt( CLOG_Buffer, commIDs, local_thread,
                                  event );
    return MPE_LOG_OK;
}

/*@
    MPE_Log_comm_event - Log an event in a specified MPI_Comm.
                         (on the calling thread where the event takes place)

    Input Parameters:
+ comm          - MPI_Comm where this process is part of.
. event         - event number.
- bytebuf       - optional byte informational array.  In C, bytebuf should be
                  set to NULL when no extra byte informational data.  In
                  Fortran, an zero-length string "", or a single blank string
                  " ", is equivalent to NULL in C.

    Notes:
    This function is threadsafe.

    Returns:
    alway returns MPE_LOG_OK
@*/
int MPE_Log_comm_event( MPI_Comm comm, int event, const char *bytebuf )
{
    const CLOG_CommIDs_t *commIDs;
          int             ierr;

    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK

    commIDs  = CLOG_CommSet_get_IDs( CLOG_CommSet, comm );
    ierr = MPE_Log_commIDs_event( commIDs, THREADID, event, bytebuf );

    MPE_LOG_THREAD_UNLOCK
    return ierr;
}


/*@
    MPE_Log_event - Log an event in MPI_COMM_WORLD.

    Input Parameters:
+   event   - event number.
.   data    - integer data value
              (not used, provided for backward compatibility purpose).
-   bytebuf - optional byte informational array.  In C, bytebuf should be
              set to NULL when no extra byte informational data.  In Fortran,
              an zero-length string "", or a single blank string " ",
              is equivalent to NULL in C.

    Notes:
    This function is threadsafe.

    Returns:
    alway returns MPE_LOG_OK

@*/
int MPE_Log_event( int event, int data, const char *bytebuf )
{
    int ierr;

    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK

    ierr = MPE_Log_commIDs_event( CLOG_CommSet->IDs4world, THREADID, 
                                  event, bytebuf );

    MPE_LOG_THREAD_UNLOCK
    return ierr;
}

/*@
    MPE_Log_bare_event - Logs a bare event in MPI_COMM_WORLD.

    Input Parameters:
.   event   - event number.

    Notes:
    This function is threadsafe.

    Returns:
    alway returns MPE_LOG_OK
@*/
int MPE_Log_bare_event( int event )
{
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK

    CLOG_Buffer_save_bareevt( CLOG_Buffer, CLOG_CommSet->IDs4world, THREADID,
                              event );

    MPE_LOG_THREAD_UNLOCK
    return MPE_LOG_OK;
}

/*@
    MPE_Log_info_event - Logs an infomational event in MPI_COMM_WORLD.

    Input Parameters:
+   event   - event number.
-   bytebuf - byte informational array.  If no byte inforamtional array,
              use MPE_Log_bare_event() instead.

    Notes:
    This function is threadsafe.

    Returns:
    alway returns MPE_LOG_OK
@*/
int MPE_Log_info_event( int event, const char *bytebuf )
{
    MPE_LOG_THREADSTM_DECL

    MPE_LOG_THREADSTM_GET
    MPE_LOG_THREAD_LOCK

    CLOG_Buffer_save_cargoevt( CLOG_Buffer, CLOG_CommSet->IDs4world, THREADID,
                               event, bytebuf );

    MPE_LOG_THREAD_UNLOCK
    return MPE_LOG_OK;
}

/*@
    MPE_Log_sync_clocks - synchronize or recalibrate all MPI clocks to
                          minimize the effect of time drift.  It is like a 
                          longer version of MPI_Comm_barrier( MPI_COMM_WORLD );

    Notes:
    This function is threadsafe.

    Returns:
    alway returns MPE_LOG_OK
@*/
int MPE_Log_sync_clocks( void )
{
    CLOG_Sync_t  *clog_syncer;
    CLOG_Time_t   local_timediff;

    MPE_LOG_THREAD_LOCK

    clog_syncer = CLOG_Stream->syncer;
    if ( clog_syncer->is_ok_to_sync == CLOG_BOOL_TRUE ) {
        local_timediff = CLOG_Sync_run( clog_syncer );
        CLOG_Buffer_set_timeshift( CLOG_Buffer, local_timediff,
                                   CLOG_BOOL_TRUE );
    }

    MPE_LOG_THREAD_UNLOCK
    return MPE_LOG_OK;
}


/*
    This is a MPE internal function.
    It is not meant for user application.

    This function is NOT threadsafe.
*/
void MPE_Log_thread_sync( int local_thread_count )
{
     int max_thread_count;
     PMPI_Allreduce( &local_thread_count, &max_thread_count, 1, MPI_INT,
                     MPI_MAX, MPI_COMM_WORLD );
     CLOG_Stream->buffer->preamble->max_thread_count = max_thread_count;
}

/* Declare clog_merged_filename same as CLOG_Merger_t.out_filename */
static char clog_merged_filename[ CLOG_PATH_STRLEN ] = {0};

/*@
    MPE_Finish_log - Send log to master, who writes it out

    Notes:
    MPE_Finish_log() & MPE_Init_log() are NOT needed when liblmpe.a is linked
    because MPI_Finalize() would have called MPE_Finish_log() already.
    liblmpe.a will be included in the final executable if it is linked with
    either "mpicc -mpe=mpilog" or "mpecc -mpilog"

    This routine outputs the logfile in CLOG2 format, i.e.
    a collective call over 'MPI_COMM_WORLD'.

    This function is threadsafe, but
    MPE_Finish_log() is expected to be called only on the main thread
    which initializes MPE logging through MPE_Init_log().

    Returns:
    Always return MPE_LOG_OK.

.seealso: MPE_Init_log()
@*/
int MPE_Finish_log( const char *filename )
{
/*
   The environment variable MPE_LOG_FORMAT is NOT read
*/
    char         *env_logfile_prefix;

    MPE_LOG_THREAD_LOCK
    MPE_LOG_THREAD_FINALIZE

    if ( MPE_Log_hasBeenClosed == 0 ) {
        CLOG_Local_finalize( CLOG_Stream );
        /*
           Invoke non-threadsafe version of MPE_Stop_log()
           before CLOG_Close() which nullifies CLOG_Stream.
        */
        CLOG_Buffer->status = CLOG_INIT_AND_OFF;

        /* Even every process reads MPE_LOGFILE_PREFIX, only rank=0 needs it */
        env_logfile_prefix = (char *) getenv( "MPE_LOGFILE_PREFIX" );
        if ( env_logfile_prefix != NULL )
            CLOG_Converge_init( CLOG_Stream, env_logfile_prefix );
        else
            CLOG_Converge_init( CLOG_Stream, filename );

        /*
           Save the merged filename in the local memory so
           CLOG_Stream_t can be freed to be destroyed
        */
        strncpy( clog_merged_filename, CLOG_Stream->merger->out_filename,
                 CLOG_PATH_STRLEN );
        CLOG_Converge_sort( CLOG_Stream );
        CLOG_Converge_finalize( CLOG_Stream );

        /*
           Finalize the CLOG_Stream_t and nullify CLOG_Buffer so calling
           other MPE routines after MPE_Finish_log will cause seg. fault.
        */
        CLOG_Close( &CLOG_Stream );
        CLOG_Buffer = NULL;

        MPE_Log_hasBeenClosed = 1;
    }
    MPE_LOG_THREAD_UNLOCK

#if defined( CLOG_NOMPI )
    /* Finalize the serial-MPI implementation */
    PMPI_Finalize();
#endif
    return MPE_LOG_OK;
}

/*@
    MPE_Log_merged_logfilename - return the immutable name of
                                 the merged final logfile.

    Notes:
    This function has no fortran interface.
@*/
const char *MPE_Log_merged_logfilename( void )
{
    return clog_merged_filename;
}
