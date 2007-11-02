/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: uni_pmi.c,v 1.1 2002/10/07 21:41:03 toonen Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*********************** PMI implementation ********************************/
/* Special UNIPROCESS implementation */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "pmi.h"
#include "uni_pmiutil.h"

int PMI_size;
int PMI_rank;

int PMI_initialized;

int PMI_kvsname_max = 32;
int PMI_keylen_max = 16;
int PMI_vallen_max = 1024;

int PMI_iter_next_idx;
int PMI_debug;

int PMII_getmaxes( int *kvsname_max, int *keylen_max, int *vallen_max );
int PMII_iter( const char *kvsname, const int idx, int *nextidx, char *key, char *val );

/******************************** Group functions *************************/

int PMI_Init( int *spawned )
{
    char *p;

    PMI_size = 1;
    PMI_rank = 0;

    if ( ( p = getenv( "PMI_DEBUG" ) ) )
	PMI_debug = atoi( p );
    else 
	PMI_debug = 0;
	
    /* for now */
    *spawned = 0;

    PMI_initialized = 1;

    return( 0 );
}

int PMI_Initialized( void )
{
    return( PMI_initialized );
}

int PMI_Get_size( int *size )
{
    if ( PMI_initialized )
	*size = PMI_size;
    else
	*size = 1;
    return( 0 );
}

int PMI_Get_rank( int *rank )
{
    if ( PMI_initialized )
	*rank = PMI_rank;
    else
	*rank = 0;
    return( 0 );
}

int PMI_Barrier( )
{
  return( 0 );
}

int PMI_Finalize( )
{
    return( 0 );
}

/**************************************** Keymap functions *************************/

int PMI_KVS_Get_my_name( char *kvsname )
{
  MPIU_Strncpy( kvsname, "uni", 4 );
  return( 0 );
}

int PMI_KVS_Get_name_length_max( )
{
  return 31;
}

int PMI_KVS_Get_key_length_max( )
{
    return( PMI_keylen_max );
}

int PMI_KVS_Get_value_length_max( )
{
    return( PMI_vallen_max );
}

int PMI_KVS_Create( char *kvsname )
{
  MPIU_Strncpy( kvsname, "uni", 4 );
  return( 0 );
}

int PMI_KVS_Destroy( const char *kvsname )
{
  return( 0 );
}

int PMI_KVS_Put( const char *kvsname, const char *key, const char *value )
{
    return( 0 );
}

int PMI_KVS_Commit( const char *kvsname )
{
    /* no-op in this implementation */
    return( 0 );
}

int PMI_KVS_Get( const char *kvsname, const char *key, char *value)
{
  return( 0 );
}

/******************************** Process Creation functions *************************/

int PMI_Spawn(const char *command, const char *argv[], 
	      const int maxprocs, char *kvsname, int kvsnamelen )
{
#ifdef FOO    
    PMIU_printf( 1, "Spawn is not implemented" );
#endif
    return( -1 );
}
int PMI_Spawn_multiple(int count, const char *cmds[], const char **argvs[], 
                       const int *maxprocs, const void *info, int *errors, 
                       int *same_domain, const void *preput_info)
{
#ifdef FOO
    PMIU_printf( 1, "Spawn_multiple is not implemented" );
#endif
    return( -1 );
}

int PMI_Args_to_info(int *argcp, char ***argvp, void *infop)
{
    return ( 0 );
}

/********************* Internal routines not part of PMI interface *****************/

/* get a keyval pair by specific index */

/* to get all maxes in one message */
int PMII_getmaxes( int *kvsname_max, int *keylen_max, int *vallen_max )
{
  *kvsname_max = 32;
  *keylen_max = 16;
  *vallen_max = 1024;
  return 0;
}

