/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_PREAMBLE )
#define _CLOG_PREAMBLE

#include "clog_inttypes.h"
#include "clog_const.h"

/*
   the function of the CLOG logging routines is to write log records into
   buffers, which are processed later.
*/
#define CLOG_PREAMBLE_SIZE   1024
#define CLOG_VERSION_STRLEN    12

typedef struct {
    char          version[ CLOG_VERSION_STRLEN ];
    CLOG_BOOL_T   is_big_endian; /* either CLOG_BOOL_TRUE or CLOG_BOOL_FALSE */
    CLOG_BOOL_T   is_finalized;  /* either CLOG_BOOL_TRUE or CLOG_BOOL_FALSE */
    unsigned int  block_size;
    unsigned int  num_buffered_blocks;
    unsigned int  max_comm_world_size;
    unsigned int  max_thread_count;
             int  known_eventID_start;
             int  user_eventID_start;
             int  known_solo_eventID_start;
             int  user_solo_eventID_start;
    unsigned int  known_stateID_count;
    unsigned int  user_stateID_count;
    unsigned int  known_solo_eventID_count;
    unsigned int  user_solo_eventID_count;
    CLOG_int64_t  commtable_fptr;
} CLOG_Preamble_t;

CLOG_Preamble_t *CLOG_Preamble_create( void );

void CLOG_Preamble_free( CLOG_Preamble_t **preamble );

void CLOG_Preamble_env_init( CLOG_Preamble_t *preamble );

void CLOG_Preamble_write( const CLOG_Preamble_t *preamble,
                                CLOG_BOOL_T      is_big_endian,
                                CLOG_BOOL_T      is_finalized,
                                int              fd );

void CLOG_Preamble_read( CLOG_Preamble_t *preamble, int fd );

void CLOG_Preamble_print( const CLOG_Preamble_t *preamble, FILE *stream );

void CLOG_Preamble_copy( const CLOG_Preamble_t *src, CLOG_Preamble_t *dest );

void CLOG_Preamble_sync(       CLOG_Preamble_t *parent,
                         const CLOG_Preamble_t *child );

#endif /* of _CLOG_PREAMBLE */
