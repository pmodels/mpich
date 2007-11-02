/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: uni_pmiutil.h,v 1.2 2003/02/17 14:23:02 gropp Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* maximum sizes for arrays */
#define PMIU_MAXLINE 1024
#define PMIU_IDSIZE    32

char PMIU_print_id[PMIU_IDSIZE];

/* prototypes for PMIU routines */
void PMIU_printf( int print_flag, char *fmt, ... );
int  PMIU_parse_keyvals( char *st );
void PMIU_dump_keyvals( void );
char *PMIU_getval( const char *keystr, char *valstr );
void PMIU_chgval( const char *keystr, char *valstr );
