/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* maximum sizes for arrays */
#define PMIU_MAXLINE 1024
#define PMIU_IDSIZE    32

/* we don't have access to MPIU_Assert and friends here in the PMI code */
#if defined(HAVE_ASSERT_H)
#  include <assert.h>
#  define PMIU_Assert(expr) assert(expr)
#else
#  define PMIU_Assert(expr)
#endif

#if defined HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */


/* prototypes for PMIU routines */
void PMIU_Set_rank( int PMI_rank );
void PMIU_SetServer( void );
void PMIU_printf( int print_flag, const char *fmt, ... );
int  PMIU_readline( int fd, char *buf, int max );
int  PMIU_writeline( int fd, char *buf );
int  PMIU_parse_keyvals( char *st );
void PMIU_dump_keyvals( void );
char *PMIU_getval( const char *keystr, char *valstr, int vallen );
void PMIU_chgval( const char *keystr, char *valstr );
