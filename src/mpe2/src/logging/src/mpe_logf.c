/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
/* mpe_log.c */
/* Custom Fortran interface file */
/* These have been edited because they require special string processing */
#ifndef DEBUG_ALL
#define DEBUG_ALL
#endif
#include "mpe_conf.h"
#include "mpe_logging_conf.h"

#if defined( HAVE_STDIO_H ) || defined( STDC_HEADERS )
#include <stdio.h>
#endif

#if defined( HAVE_STDLIB_H ) || defined( STDC_HEADERS )
#include <stdlib.h>
#else
extern char *malloc();
extern void free();
#endif

#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#endif

#include "clog_util.h"
#include "mpe_log.h"

/* This is needed to process Cray - style character data */
#if defined( WITH_CRAY_FCD_STRING )
#include <fortran.h>
#endif

#ifdef F77_NAME_UPPER
#define mpe_init_log_ MPE_INIT_LOG
#define mpe_initialized_logging_ MPE_INITIALIZED_LOGGING
#define mpe_start_log_ MPE_START_LOG
#define mpe_stop_log_ MPE_STOP_LOG
#define mpe_log_get_event_number_ MPE_LOG_GET_EVENT_NUMBER
#define mpe_log_get_state_eventids_ MPE_LOG_GET_STATE_EVENTIDS
#define mpe_log_get_solo_eventid_ MPE_LOG_GET_SOLO_EVENTID
#define mpe_describe_info_state_ MPE_DESCRIBE_INFO_STATE
#define mpe_describe_state_ MPE_DESCRIBE_STATE
#define mpe_describe_info_event_ MPE_DESCRIBE_INFO_EVENT
#define mpe_describe_event_ MPE_DESCRIBE_EVENT
#define mpe_log_pack_ MPE_LOG_PACK
#define mpe_log_event_ MPE_LOG_EVENT
#define mpe_log_bare_event_ MPE_LOG_BARE_EVENT
#define mpe_log_info_event_ MPE_LOG_INFO_EVENT
#define mpe_log_send_ MPE_LOG_SEND
#define mpe_log_receive_ MPE_LOG_RECEIVE
#define mpe_log_sync_clocks_ MPE_LOG_SYNC_CLOCKS
#define mpe_finish_log_ MPE_FINISH_LOG
#elif defined(F77_NAME_LOWER_2USCORE)
#define mpe_init_log_ mpe_init_log__
#define mpe_initialized_logging_ mpe_initialized_logging__
#define mpe_start_log_ mpe_start_log__
#define mpe_stop_log_ mpe_stop_log__
#define mpe_log_get_event_number_ mpe_log_get_event_number__
#define mpe_log_get_state_eventids_ mpe_log_get_state_eventids__
#define mpe_log_get_solo_eventid_ mpe_log_get_solo_eventid__
#define mpe_describe_info_state_ mpe_describe_info_state__
#define mpe_describe_state_ mpe_describe_state__
#define mpe_describe_info_event_ mpe_describe_info_event__
#define mpe_describe_event_ mpe_describe_event__
#define mpe_log_pack_ mpe_log_pack__
#define mpe_log_event_ mpe_log_event__
#define mpe_log_bare_event_ mpe_log_bare_event__
#define mpe_log_info_event_ mpe_log_info_event__
#define mpe_log_send_ mpe_log_send__
#define mpe_log_receive_ mpe_log_receive__
#define mpe_log_sync_clocks_ mpe_log_sync_clocks__
#define mpe_finish_log_ mpe_finish_log__
#elif defined(F77_NAME_LOWER)
#define mpe_init_log_ mpe_init_log
#define mpe_initialized_logging_ mpe_initialized_logging
#define mpe_start_log_ mpe_start_log
#define mpe_stop_log_ mpe_stop_log
#define mpe_log_get_event_number_ mpe_log_get_event_number
#define mpe_log_get_state_eventids_ mpe_log_get_state_eventids
#define mpe_log_get_solo_eventid_ mpe_log_get_solo_eventid
#define mpe_describe_info_state_ mpe_describe_info_state
#define mpe_describe_state_ mpe_describe_state
#define mpe_describe_info_event_ mpe_describe_info_event
#define mpe_describe_event_ mpe_describe_event
#define mpe_log_pack_ mpe_log_pack
#define mpe_log_event_ mpe_log_event
#define mpe_log_bare_event_ mpe_log_bare_event
#define mpe_log_info_event_ mpe_log_info_event
#define mpe_log_send_ mpe_log_send
#define mpe_log_receive_ mpe_log_receive
#define mpe_log_sync_clocks_ mpe_log_sync_clocks
#define mpe_finish_log_ mpe_finish_log
#endif

/* 
 * In order to suppress warnings about missing prototypes, we've added
 * them to this file.
 */

/* 
   This function makes a copy of a Fortran string into a C string.  Some
   Unix Fortran compilers add nulls at the ends of string CONSTANTS, but
   (a) not for substring expressions and (b) not all compilers do so (e.g.,
   RS6000)
 */

static char *mpe_tmp_cpy( char *, int );
static char *mpe_tmp_cpy( char *s, int d )
{
    char *p;
    p = (char *)malloc( d + 1 );
    if (!p) {
        fprintf( stderr,
                 "MPE Fort2C: malloc() failed! mpe_tmp_cpy returns NULL!..." );
        fflush( stderr );
        return NULL;
    }
    strncpy( p, s, d );
    p[d] = 0;
    return p;
}

int mpe_init_log_( void );
int mpe_init_log_( void )
{
    return MPE_Init_log();
}

int mpe_initialized_logging_( void );
int mpe_initialized_logging_( void )
{
    return MPE_Initialized_logging();
}

int mpe_start_log_( void );
int mpe_start_log_( void )
{
    return MPE_Start_log();
}

int mpe_stop_log_( void );
int mpe_stop_log_( void )
{
    return MPE_Stop_log();
}

int mpe_log_sync_clocks_( void );
int mpe_log_sync_clocks_( void )
{
    return MPE_Log_sync_clocks();
}

int mpe_log_get_event_number_( void );
int mpe_log_get_event_number_( void )
{
    return MPE_Log_get_event_number();
}

int mpe_log_get_state_eventids_( int*, int* );
int mpe_log_get_state_eventids_( int *statedef_startID, int *statedef_finalID )
{
    return MPE_Log_get_state_eventIDs( statedef_startID, statedef_finalID );
}

int mpe_log_get_solo_eventid_( int* );
int mpe_log_get_solo_eventid_( int *eventdef_eventID )
{
    return MPE_Log_get_solo_eventID( eventdef_eventID );
}

int mpe_log_send_( int*, int*, int* );
int mpe_log_send_( int *otherParty, int *tag, int *size )
{
    return MPE_Log_send( *otherParty, *tag, *size );
}

int mpe_log_receive_( int*, int*, int* );
int mpe_log_receive_( int *otherParty, int *tag, int *size )
{
    return MPE_Log_receive( *otherParty, *tag, *size );
}

#if defined( WITH_CRAY_FCD_STRING )
int  mpe_describe_info_state_( int *start_etype, int *final_etype,
                               _fcd name, _fcd color, _fcd format )
{
    char *c1, *c2, *c3;
    int  err;
    c1 = mpe_tmp_cpy( _fcdtocp( name ), _fcdlen( name ) );
    c2 = mpe_tmp_cpy( _fcdtocp( color ), _fcdlen( color ) );
    c3 = mpe_tmp_cpy( _fcdtocp( format ), _fcdlen( format ) );
    err = MPE_Describe_info_state( *start_etype, *final_etype, c1, c2, c3 );
    free( c3 );
    free( c2 );
    free( c1 );
    return err;
}
#else
int  mpe_describe_info_state_( int *, int *,
                               char *, char *, char *,
                               int, int, int );
int  mpe_describe_info_state_( int *start_etype, int *final_etype,
                               char *name, char *color, char *format,
                               int  d1, int d2, int d3 )
{
    char *c1, *c2, *c3;
    int  err;
    c1 = mpe_tmp_cpy( name, d1 );
    c2 = mpe_tmp_cpy( color, d2 );
    c3 = mpe_tmp_cpy( format, d3 );
    err = MPE_Describe_info_state( *start_etype, *final_etype, c1, c2, c3 );
    free( c3 );
    free( c2 );
    free( c1 );
    return err;
}
#endif

#if defined( WITH_CRAY_FCD_STRING )
int  mpe_describe_state_( int *start_etype, int *final_etype,
                          _fcd name, _fcd color )
{
    char *c1, *c2;
    int  err;
    c1 = mpe_tmp_cpy( _fcdtocp( name ), _fcdlen( name ) );
    c2 = mpe_tmp_cpy( _fcdtocp( color ), _fcdlen( color ) );
    err = MPE_Describe_state( *start_etype, *final_etype, c1, c2 );
    free( c2 );
    free( c1 );
    return err;
}
#else
int  mpe_describe_state_( int *, int *, char *, char *, int, int );
int  mpe_describe_state_( int *start_etype, int *final_etype,
                          char *name, char *color, int d1, int d2 )
{
    char *c1, *c2;
    int  err;
    c1 = mpe_tmp_cpy( name, d1 );
    c2 = mpe_tmp_cpy( color, d2 );
    err = MPE_Describe_state( *start_etype, *final_etype, c1, c2 );
    free( c2 );
    free( c1 );
    return err;
}
#endif

#if defined( WITH_CRAY_FCD_STRING )
int  mpe_describe_info_event_( int *event, _fcd name, _fcd color, _fcd format )
{
    char *c1, *c2, *c3;
    int  err;
    c1 = mpe_tmp_cpy( _fcdtocp( name ), _fcdlen( name ) );
    c2 = mpe_tmp_cpy( _fcdtocp( color ), _fcdlen( color ) );
    c3 = mpe_tmp_cpy( _fcdtocp( format ), _fcdlen( format ) );
    err = MPE_Describe_info_event( *event, c1, c2, c3 );
    free( c3 );
    free( c2 );
    free( c1 );
    return err;
}
#else
int  mpe_describe_info_event_( int *, char *, char*, char *, int, int, int );
int  mpe_describe_info_event_( int *event,
                               char *name, char *color, char *format,
                               int d1, int d2, int d3 )
{
    char *c1, *c2, *c3;
    int  err;
    c1 = mpe_tmp_cpy( name, d1 );
    c2 = mpe_tmp_cpy( color, d2 );
    c3 = mpe_tmp_cpy( format, d3 );
    err = MPE_Describe_info_event( *event, c1, c2, c3 );
    free( c3 );
    free( c2 );
    free( c1 );
    return err;
}
#endif

#if defined( WITH_CRAY_FCD_STRING )
int mpe_describe_event_( int *event, _fcd name, _fcd color )
{
    char *c1, *c2;
    int  err;
    c1 = mpe_tmp_cpy( _fcdtocp( name ), _fcdlen( name ) );
    c2 = mpe_tmp_cpy( _fcdtocp( color ), _fcdlen( color ) );
    err = MPE_Describe_event( *event, c1, c2 );
    free( c2 );
    free( c1 );
    return err;
}
#else
int  mpe_describe_event_( int *, char *, char*, int, int );
int  mpe_describe_event_( int *event, char *name, char *color, int d1, int d2 )
{
    char *c1, *c2;
    int  err;
    c1 = mpe_tmp_cpy( name, d1 );
    c2 = mpe_tmp_cpy( color, d2 );
    err = MPE_Describe_event( *event, c1, c2 );
    free( c2 );
    free( c1 );
    return err;
}
#endif

int  mpe_log_pack_( void *, int *, char *, int *, void * );
int  mpe_log_pack_( void *bytebuf, int *position,
                    char *tokentype, int *count, void *data )
{
    return MPE_Log_pack( bytebuf, position, *tokentype, *count, data );
}

#if defined( WITH_CRAY_FCD_STRING )
int  mpe_log_event_( int *event, int *data, _fcd byteinfo )
{
    int  byteinfo_size, err;
    byteinfo_size = _fcdlen( byteinfo );
    if ( byteinfo_size > 0 )
        /* mpe_tmp_cpy uses strncpy() which cannot be used on byteinfo */
        err = MPE_Log_event( *event, *data, _fcdtocp( byteinfo ) );
    else
        err = MPE_Log_event( *event, *data, NULL );
    return err;
}
#else
int  mpe_log_event_( int *, int *, char *, int );
int  mpe_log_event_( int *event, int *data, char *byteinfo, int d1 )
{
    int  err;
    if ( d1 <= 0 )
        err = MPE_Log_event( *event, *data, NULL );
    else if ( d1 == 1 && strncmp( byteinfo, " ", 1 ) == 0 ) /* single blank */
        err = MPE_Log_event( *event, *data, NULL );
    else  /* if ( d1 > 1 || non single blank string ) */ {
        /* mpe_tmp_cpy uses strncpy() which cannot be used on byteinfo */
        err = MPE_Log_event( *event, *data, byteinfo );
    }
    return err;
}
#endif

#if defined( WITH_CRAY_FCD_STRING )
int  mpe_log_info_event_( int *event, _fcd byteinfo )
{
    return MPE_Log_info_event( *event, _fcdtocp( byteinfo ) );
}
#else
int  mpe_log_info_event_( int *, char *, int );
int  mpe_log_info_event_( int *event, char *byteinfo, int d1 )
{
    return MPE_Log_info_event( *event, byteinfo );
}
#endif

int  mpe_log_bare_event_( int * );
int  mpe_log_bare_event_( int *event )
{
    return MPE_Log_bare_event( *event );
}

#if defined( WITH_CRAY_FCD_STRING )
int  mpe_finish_log_( _fcd filename )
{
    char *c1;
    int  err;
    c1 = mpe_tmp_cpy( _fcdtocp( filename ), _fcdlen( filename ) );
    err =  MPE_Finish_log( c1 );
    free( c1 );
    return err;
}
#else
int  mpe_finish_log_( char *, int );
int  mpe_finish_log_( char *filename, int d1 )
{
    char *c1;
    int  err;
    c1 = mpe_tmp_cpy( filename, d1 );
    err =  MPE_Finish_log( c1 );
    free( c1 );
    return err;
}
#endif
