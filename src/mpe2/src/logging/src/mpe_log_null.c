/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif

#include "mpe_log.h"

int MPE_Init_log( void )
{ return MPE_LOG_OK; }

int MPE_Start_log( void )
{ return MPE_LOG_OK; }

int MPE_Stop_log( void )
{ return MPE_LOG_OK; }

int MPE_Initialized_logging( void )
{ return MPE_LOG_OK; }

int MPE_Describe_comm_state( MPI_Comm comm, int local_thread,
                             int start_etype, int final_etype,
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


int MPE_Describe_comm_event( MPI_Comm comm, int local_thread,
                             int event, const char *name, const char *color,
                             const char *format )
{ return MPE_LOG_OK; }

int MPE_Describe_info_event( int event, const char *name, const char *color,
                             const char *format )
{ return MPE_LOG_OK; }

int MPE_Describe_event( int event, const char *name, const char *color )
{ return MPE_LOG_OK; }

int MPE_Log_get_event_number( void )
{ return -99999; }

int MPE_Log_comm_send( MPI_Comm comm, int local_thread,
                       int receiver, int tag, int size )
{ return MPE_LOG_OK; }

int MPE_Log_send( int other_party, int tag, int size )
{ return MPE_LOG_OK; }

int MPE_Log_comm_receive( MPI_Comm comm, int local_thread,
                          int sender, int tag, int size )
{ return MPE_LOG_OK; }

int MPE_Log_receive( int other_party, int tag, int size )
{ return MPE_LOG_OK; }

int MPE_Log_pack( MPE_LOG_BYTES bytebuf, int *position,
                  char tokentype, int count, const void *data )
{ return MPE_LOG_OK; }

int MPE_Log_comm_event( MPI_Comm comm, int local_thread,
                        int event, const char *bytebuf )
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
{ return MPE_LOG_OK; }

const char *MPE_Log_merged_logfilename( void )
{ return NULL; }
