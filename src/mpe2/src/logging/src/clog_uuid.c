/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif
#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if !defined( CLOG_NOMPI )
#include "mpi.h"
#else
#include "mpi_null.h"
#endif /* Endof if !defined( CLOG_NOMPI ) */

/*
   Definitions for CLOG's Universal Unique ID implementation
*/
#include "clog_const.h"
#include "clog_uuid.h"
#include "clog_util.h"


#if defined( NEEDS_SRAND48_DECL )
void srand48(long seedval);
#endif
#if defined( NEEDS_LRAND48_DECL )
long lrand48(void);
#endif



static char CLOG_UUID_NULL_NAME[ CLOG_UUID_NAME_SIZE ]  = {0};
/* const  char CLOG_UUID_NULL[ CLOG_UUID_SIZE ]            = {0}; */
const  CLOG_Uuid_t CLOG_UUID_NULL                       = {0};

void CLOG_Uuid_init( void )
{
#ifdef HAVE_WINDOWS_H
    srand(getpid());
#else
    pid_t  proc_pid;

    proc_pid = getpid();
    srand48( (long) proc_pid );
#endif
}

void CLOG_Uuid_finalize( void )
{}

void CLOG_Uuid_generate( CLOG_Uuid_t uuid )
{
    CLOG_int32_t  random_number;
    double        time;
    int           namelen;
    char         *ptr;
    char          processor_name[ MPI_MAX_PROCESSOR_NAME ] = {0};

#ifdef HAVE_WINDOWS_H
    random_number  = rand();
#else
    random_number  = (CLOG_int32_t) lrand48();
#endif

    /* Can't use CLOG_Timer_get() as CLOG_Timer_start() has been called yet */
    time  = PMPI_Wtime();

    PMPI_Get_processor_name( processor_name, &namelen );

    ptr  = &uuid[0];
    memcpy( ptr, &random_number, sizeof(CLOG_int32_t) );
    ptr += sizeof(CLOG_int32_t);
    memcpy( ptr, &time, sizeof(double) );
    ptr += sizeof(double);
    if ( namelen < CLOG_UUID_NAME_SIZE ) {
        memcpy( ptr, processor_name, namelen );
        /* pad the rest of uuid with 0 */
        ptr += namelen;
        memcpy( ptr, CLOG_UUID_NULL_NAME, CLOG_UUID_NAME_SIZE-namelen );
    }
    else /* if ( namelen >= CLOG_UUID_NAME_SIZE ) */
        memcpy( ptr, processor_name, CLOG_UUID_NAME_SIZE );
}

/*
   Assume the output string array, str, is big enough to hold
   the string representation of the CLOG_Uuid_t
*/
void CLOG_Uuid_sprint( CLOG_Uuid_t uuid, char *str )
{
    CLOG_int32_t  random_number;
    double        time;
    char          name[ CLOG_UUID_NAME_SIZE+1 ] = {0};
    char         *ptr;

    ptr  = &uuid[0];
    memcpy( &random_number, ptr, sizeof(CLOG_int32_t) );
    ptr += sizeof(CLOG_int32_t);
    memcpy( &time, ptr, sizeof(double) );
    ptr += sizeof(double);
    memcpy( &name, ptr, CLOG_UUID_NAME_SIZE );
    sprintf( str, i32fmt"-%f-%s", random_number, time, name );
}

int  CLOG_Uuid_is_equal( const CLOG_Uuid_t uuid1, const CLOG_Uuid_t uuid2 )
{
     if ( memcmp( uuid1, uuid2, CLOG_UUID_SIZE ) == 0 )
         return CLOG_BOOL_TRUE;
     else
         return CLOG_BOOL_FALSE;
}

int  CLOG_Uuid_compare( const void *obj1, const void *obj2 )
{
    return memcmp( obj1, obj2, CLOG_UUID_SIZE );
}

void CLOG_Uuid_copy( const CLOG_Uuid_t src_uuid, CLOG_Uuid_t dest_uuid )
{
    memcpy( dest_uuid, src_uuid, CLOG_UUID_SIZE );
}

void CLOG_Uuid_swap_bytes( CLOG_Uuid_t uuid )
{
    char   *ptr;

    ptr  = &uuid[0];
    CLOG_Util_swap_bytes( ptr, sizeof(CLOG_int32_t), 1 );
    ptr += sizeof(CLOG_int32_t);
    CLOG_Util_swap_bytes( ptr, sizeof(double), 1 );
}
