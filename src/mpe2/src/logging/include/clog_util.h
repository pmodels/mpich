/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_UTIL )
#define _CLOG_UTIL

#include "clog_const.h"

void CLOG_Util_abort( int errorcode );

CLOG_BOOL_T CLOG_Util_getenvbool( const char *env_var,
                                  CLOG_BOOL_T default_value );

void CLOG_Util_set_tmpfilename( char *tmp_pathname );

void CLOG_Util_swap_bytes( void *bytes, int elem_sz, int Nelem );

char *CLOG_Util_strbuf_put(       char *buf_ptr, const char *buf_tail,
                            const char *val_str, const char *err_str );

char *CLOG_Util_strbuf_get(       char *val_ptr, const char *val_tail,
                            const char *buf_str, const char *err_str );

CLOG_BOOL_T CLOG_Util_is_MPIWtime_synchronized( void );

CLOG_BOOL_T CLOG_Util_is_runtime_bigendian( void );

#endif  /* of _CLOG_UTIL */
