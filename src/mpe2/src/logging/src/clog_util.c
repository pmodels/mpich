/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( HAVE_STDIO_H ) || defined( STDC_HEADERS )
#include <stdio.h>
#endif
#if defined( HAVE_STDLIB_H ) || defined( STDC_HEADERS )
#include <stdlib.h>
#endif
#if defined( HAVE_STRING_H ) || defined( STDC_HEADERS )
#include <string.h>
#endif
#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#if !defined( CLOG_NOMPI )

#include "mpi.h"
#if !defined( HAVE_PMPI_COMM_GET_ATTR )
#define PMPI_Comm_get_attr PMPI_Attr_get
#endif

#else
#include "mpi_null.h"
#endif /* Endof if !defined( CLOG_NOMPI ) */

#include "clog_const.h"
#include "clog_util.h"
#include "mpe_callstack.h"

#if defined( HAVE_MKSTEMP )
#if defined( NEEDS_MKSTEMP_DECL )
extern int mkstemp(char *t);
#endif
#endif

/*
    errorcode - errorcode to be returned to the invoking environment
*/
void CLOG_Util_abort( int errorcode )
{
    MPE_CallStack_t  cstk;
    char             msg[ CLOG_ERR_STRLEN ];
    int              world_rank;

    PMPI_Comm_rank( MPI_COMM_WORLD, &world_rank );
    sprintf( msg, "Backtrace of the callstack at rank %d:\n", world_rank );
    write( STDERR_FILENO, msg, strlen(msg)+1 );
    MPE_CallStack_init( &cstk );
    MPE_CallStack_fancyprint( &cstk, STDERR_FILENO,
                              "\tAt ", 1, MPE_CALLSTACK_UNLIMITED );
    PMPI_Abort( MPI_COMM_WORLD, errorcode );
}

/*
   tmp_pathname[] is assumed to be of size CLOG_PATH_STRLEN
*/
void CLOG_Util_set_tmpfilename( char *tmp_pathname )
{
    char   *env_tmpdir = NULL;
    char    tmpdirname_ref[ CLOG_PATH_STRLEN ] = "";
    char    tmpdirname[ CLOG_PATH_STRLEN ] = "";
    char    tmpfilename[ CLOG_PATH_STRLEN ] = "";
    int     my_rank;
    int     ierr;
#if defined( HAVE_MKSTEMP )
    int     tmp_fd;
#endif
                                                                                
    if ( tmp_pathname == NULL ) {
        fprintf( stderr, __FILE__":CLOG_Util_set_tmpfilename() - \n"
                         "\t""The input string buffer is NULL.\n" );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }

    PMPI_Comm_rank( MPI_COMM_WORLD, &my_rank );

    /* MPE_TMPDIR takes precedence over TMPDIR */
    env_tmpdir = (char *) getenv( "MPE_TMPDIR" );
    if ( env_tmpdir == NULL )
        env_tmpdir = (char *) getenv( "TMPDIR" );
    if ( env_tmpdir == NULL )
        env_tmpdir = (char *) getenv( "TMP" );
    if ( env_tmpdir == NULL )
        env_tmpdir = (char *) getenv( "TEMP" );
                                                                                
    /*  Set tmpdirname_ref to TMPDIR at Master if available  */
    if ( my_rank == 0 ) {
        if ( env_tmpdir != NULL )
            strcat( tmpdirname_ref, env_tmpdir );
        else
#ifdef HAVE_WINDOWS_H
            if ( GetTempPath( CLOG_PATH_STRLEN, tmpdirname_ref ) == 0 )
                strcat( tmpdirname_ref, "\\");
#else
            strcat( tmpdirname_ref, "/tmp" );
#endif
    }
                                                                                
    /*  Let everyone in MPI_COMM_WORLD know what root has */
    ierr = PMPI_Bcast( tmpdirname_ref, CLOG_PATH_STRLEN, MPI_CHAR,
                       0, MPI_COMM_WORLD );
    if ( ierr != MPI_SUCCESS ) {
        fprintf( stderr, __FILE__":CLOG_Util_get_tmpfilename_init() - \n"
                         "\t""PMPI_Bcast() fails\n" );
        fflush( stderr );
        PMPI_Abort( MPI_COMM_WORLD, 1 );
    }
                                                                                
    /*  Use TMPDIR if set in children processes  */
    if ( env_tmpdir != NULL )
        strcpy( tmpdirname, env_tmpdir );
    else
        strcpy( tmpdirname, tmpdirname_ref );
                                                                                
    if ( strlen( tmpdirname ) <= 0 ) {
        fprintf( stderr, __FILE__":CLOG_Util_get_tmpfilename() - \n"
                         "\t""strlen(tmpdirname) = %d\n",
                         (int)strlen( tmpdirname ) );
        fflush( stderr );
        CLOG_Util_abort( 1 );
    }
                                                                                
    /*  Set the local tmp filename then tmp_pathname */
    strcpy( tmp_pathname, tmpdirname );
    sprintf( tmpfilename, "/"CLOG_FILE_TYPE"_taskID=%04d_XXXXXX", my_rank );
    strcat( tmp_pathname, tmpfilename );
                                                                                
    /*  Make the filename unique ( optional ) */
#if defined( HAVE_MKSTEMP )
    /*
        Delete the file created by mkstemp().
        The file will be OPENed later if necessary
    */
    tmp_fd = mkstemp( tmp_pathname );
    if ( tmp_fd != -1 ) {
        close( tmp_fd );
        unlink( tmp_pathname );
    }
#else
    mktemp( tmp_pathname );
#endif
}

/*
    Replace CLOGByteSwapDouble() and CLOGByteSwapInt() by the more general
    and more efficient CLOG_byteswap().
    (The measurement was done by ts_byteswap.c on Linux P4 2.4Ghz with
     gcc 2.96 or winXP P4 1.4Ghz with gcc 3.3.1.  A 15%-70% improvement
     depends on optimization level.)

    input/output  bytes    - pointer to the buffer
    input         elem_sz  - e.g. sizeof(short), sizeof(int) or sizeof(double)
    input         Nelem    - number of elements, each is of size elem_sz
 */
void CLOG_Util_swap_bytes( void  *bytes,
                           int    elem_sz,
                           int    Nelem )
{
    char *bptr;
    char  btmp;
    int end_ii;
    int ii, jj;

    bptr = (char *) bytes;
    for ( jj = 0; jj < Nelem; jj++ ) {
         for ( ii = 0; ii < elem_sz/2; ii++ ) {
             end_ii          = elem_sz - 1 - ii;
             btmp            = bptr[ ii ];
             bptr[ ii ]      = bptr[ end_ii ];
             bptr[ end_ii ]  = btmp;
         }
         bptr += elem_sz;
    }
}

/*
    CLOG_Util_strbuf_put: internal function to CLOG_Preamble_write
    It copies string val_str to buf_ptr.
    It returns the next available buffer pointer that has already
    been accounted for the number of characters copied including
    the terminating \0.
*/
char *CLOG_Util_strbuf_put(       char *buf_ptr, const char *buf_tail,
                            const char *val_str, const char *err_str )
{
    int size;
    size = (int)strlen( val_str ) + 1;
    if ( buf_ptr + size - 1 <= buf_tail )
        strcpy( buf_ptr, val_str );
    else {
        fprintf( stderr, __FILE__":CLOG_Util_strbuf_put() - \n"
                         "\t""strcpy of %s fails, lack of space in buffer.\n",
                         err_str );
        fflush( stderr );
        CLOG_Util_abort( 1 );
        return NULL;
    }
    return buf_ptr + size;
}

/*
    CLOG_Util_strbuf_get: internal function to CLOG_Preamble_read
    It copies string buf_str to val_ptr.
    It returns the next available buffer pointer that has already
    been accounted for the number of characters copied including
    the terminating \0.
*/
char *CLOG_Util_strbuf_get(       char *val_ptr, const char *val_tail,
                            const char *buf_str, const char *err_str )
{
    int size;
    size = (int)strlen( buf_str ) + 1;
    if ( val_ptr + size - 1 <= val_tail )
        strcpy( val_ptr, buf_str );
    else {
        fprintf( stderr, __FILE__":CLOG_Util_strbuf_get() - \n"
                         "\t""strcpy of %s fails, lack of space in value.\n",
                         err_str );
        fflush( stderr );
        CLOG_Util_abort( 1 );
        return 0;
    }
    return (char *) buf_str + size;
}

/*
    The function returns CLOG_BOOL_TRUE if the MPI implementation sychronized.
*/
CLOG_BOOL_T CLOG_Util_is_MPIWtime_synchronized( void )
{
    int           flag, *is_globalp;
    /* int           my_rank; */

    /* PMPI_Comm_rank( MPI_COMM_WORLD, &my_rank ); */
    PMPI_Comm_get_attr( MPI_COMM_WORLD, MPI_WTIME_IS_GLOBAL,
                        &is_globalp, &flag );
    /*
    printf( "TaskID = %d : flag = %d, is_globalp = %d, *is_globalp = %d\n",
            my_rank, flag, is_globalp, *is_globalp);
    */
    if ( !flag || (is_globalp && !*is_globalp) )
        return CLOG_BOOL_FALSE;  /* MPI Clocks are NOT synchronized */
    else
        return CLOG_BOOL_TRUE;   /* MPI Clocks are synchronized */
}

CLOG_BOOL_T CLOG_Util_is_runtime_bigendian( void )
{
    union {
        long ll;
        char cc[sizeof(long)];
    } uu;
    uu.ll = 1;
    if ( uu.cc[sizeof(long) - 1] == 1 )
        return CLOG_BOOL_TRUE;
    else
        return CLOG_BOOL_FALSE;
}
