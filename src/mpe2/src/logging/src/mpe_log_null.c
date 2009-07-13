/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif

#include "mpe_log.h"

#define MPE_NULL_EVENTID  -99999

int MPE_Log_hasBeenInit   = 0;
int MPE_Log_hasBeenClosed = 0;

int MPE_Init_log( void )
{
  MPE_Log_hasBeenInit   = 1;
  MPE_Log_hasBeenClosed = 0;
  return MPE_LOG_OK;
}

int MPE_Start_log( void )
{ return MPE_LOG_OK; }

int MPE_Stop_log( void )
{ return MPE_LOG_OK; }

int MPE_Initialized_logging( void )
{ return MPE_Log_hasBeenInit + MPE_Log_hasBeenClosed; }

int MPE_Describe_comm_state( MPI_Comm comm, int start_etype, int final_etype,
                             const char *name, const char *color,
                             const char *format )
{ return MPE_LOG_OK; }

int MPE_Describe_info_state( int start_etype, int final_etype,
                             const char *name, const char *color,
                             const char *format )
{ return MPE_LOG_OK; }

int MPE_Describe_state( int start_etype, int final_etype,
                        const char *name, const char *color )
{ return MPE_LOG_OK; }


int MPE_Describe_comm_event( MPI_Comm comm, int event,
                             const char *name, const char *color,
                             const char *format )
{ return MPE_LOG_OK; }

int MPE_Describe_info_event( int event, const char *name, const char *color,
                             const char *format )
{ return MPE_LOG_OK; }

int MPE_Describe_event( int event, const char *name, const char *color )
{ return MPE_LOG_OK; }

int MPE_Log_get_event_number( void )
{ return MPE_NULL_EVENTID; }

int MPE_Log_get_solo_eventID( int *eventdef_eventID )
{
  if (eventdef_eventID) *eventdef_eventID = MPE_NULL_EVENTID; 
  return MPE_LOG_OK;
}

int MPE_Log_get_state_eventIDs( int *statedef_startID, int *statedef_finalID )
{
  if (statedef_startID) *statedef_startID = MPE_NULL_EVENTID;
  if (statedef_finalID) *statedef_finalID = MPE_NULL_EVENTID; 
  return MPE_LOG_OK;
}

int MPE_Log_comm_send( MPI_Comm comm, int receiver, int tag, int size )
{ return MPE_LOG_OK; }

int MPE_Log_send( int other_party, int tag, int size )
{ return MPE_LOG_OK; }

int MPE_Log_comm_receive( MPI_Comm comm, int sender, int tag, int size )
{ return MPE_LOG_OK; }

int MPE_Log_receive( int other_party, int tag, int size )
{ return MPE_LOG_OK; }

int MPE_Log_pack( MPE_LOG_BYTES bytebuf, int *position,
                  char tokentype, int count, const void *data )
{ return MPE_LOG_OK; }

int MPE_Log_comm_event( MPI_Comm comm, int event, const char *bytebuf )
{ return MPE_LOG_OK; }

int MPE_Log_event( int event, int data, const char *bytebuf )
{ return MPE_LOG_OK; }

int MPE_Log_bare_event( int event )
{ return MPE_LOG_OK; }

int MPE_Log_info_event( int event, const char *bytebuf )
{ return MPE_LOG_OK; }

int MPE_Log_sync_clocks( void )
{ return MPE_LOG_OK; }

int MPE_Finish_log( const char *filename )
{
  MPE_Log_hasBeenClosed = 1;
  return MPE_LOG_OK;
}

const char *MPE_Log_merged_logfilename( void )
{ return NULL; }
