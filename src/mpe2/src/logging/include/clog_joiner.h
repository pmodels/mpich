/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_JOINER )
#define _CLOG_JOINER

#include "clog_cache.h"
#include "clog_block.h"

typedef struct {
   /* array[num_caches] of input cache pointers */
   CLOG_Cache_t     **in_caches;
   unsigned int       num_caches;

   CLOG_Cache_t      *out_cache;

   /* binary linked list of in_caches[] sorted by increasing time order */
   CLOG_CacheLink_t  *sorted_caches_head;
   CLOG_CacheLink_t  *sorted_caches_tail;
} CLOG_Joiner_t;

CLOG_Joiner_t* CLOG_Joiner_create( int num_files, char *filenames[] );

void CLOG_Joiner_free( CLOG_Joiner_t **joiner_handle );

void CLOG_Joiner_init( CLOG_Joiner_t *joiner, const char *out_filename );

void CLOG_Joiner_sort( CLOG_Joiner_t *joiner );

void CLOG_Joiner_finalize( CLOG_Joiner_t *joiner );

#endif
