/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_CACHE )
#define _CLOG_CACHE

#include "clog_const.h"
#include "clog_preamble.h"
#include "clog_commset.h"
#include "clog_block.h"
#include "clog_record.h"

typedef struct {
    CLOG_Preamble_t       *preamble;
    CLOG_CommSet_t        *commset;
    CLOG_BlockData_t      *blockdata;
    int                    block_size;

    CLOG_BOOL_T            is_runtime_bigendian;
    int                    local_fd;
    char                   local_filename[ CLOG_PATH_STRLEN ];

    CLOG_Time_t            local_timediff; /* for local file */
} CLOG_Cache_t;

CLOG_Cache_t* CLOG_Cache_create( void );

void CLOG_Cache_free( CLOG_Cache_t **cache_handle );

void CLOG_Cache_flushlastblock( CLOG_Cache_t *cache );

void CLOG_Cache_open4read( CLOG_Cache_t *cache, const char *filename );

void CLOG_Cache_init4read( CLOG_Cache_t *cache );

void CLOG_Cache_open4write( CLOG_Cache_t *cache, const char *filename );

void CLOG_Cache_init4write( CLOG_Cache_t *cache );

void CLOG_Cache_open4readwrite( CLOG_Cache_t *cache, const char *filename );

void CLOG_Cache_close4read( CLOG_Cache_t *cache );

void CLOG_Cache_close4write( CLOG_Cache_t *cache );

void CLOG_Cache_close4readwrite( CLOG_Cache_t *cache );

CLOG_BOOL_T CLOG_Cache_has_rec( CLOG_Cache_t *cache );

CLOG_Rec_Header_t* CLOG_Cache_get_rec( CLOG_Cache_t *cache );

CLOG_Time_t CLOG_Cache_get_time( const CLOG_Cache_t *cache );

void CLOG_Cache_put_rec( CLOG_Cache_t *cache, const CLOG_Rec_Header_t *hdr );

void CLOG_Cache_flush( CLOG_Cache_t *cache );



typedef struct _CLOG_CacheLink_t {
    CLOG_Cache_t              *cache;
    struct _CLOG_CacheLink_t  *prev;
    struct _CLOG_CacheLink_t  *next;
} CLOG_CacheLink_t;

CLOG_CacheLink_t* CLOG_CacheLink_create( CLOG_Cache_t* cache );

void CLOG_CacheLink_free( CLOG_CacheLink_t **cachelink_handle );

void CLOG_CacheLink_detach( CLOG_CacheLink_t **head_handle,
                            CLOG_CacheLink_t **tail_handle,
                            CLOG_CacheLink_t  *detach );

void CLOG_CacheLink_append( CLOG_CacheLink_t **head_handle,
                            CLOG_CacheLink_t **tail_handle,
                            CLOG_CacheLink_t  *curr );

void CLOG_CacheLink_insert( CLOG_CacheLink_t **head_handle,
                            CLOG_CacheLink_t **tail_handle,
                            CLOG_CacheLink_t  *location,
                            CLOG_CacheLink_t  *insert );

#endif  /* _CLOG_CACHE */
