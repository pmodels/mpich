/*
   (C) 2007 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_callstack_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif
#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif
#if defined( HAVE_FCNTL_H )
#include <fcntl.h>
#endif

#include "mpe_callstack.h"

#if defined( NEEDS_FDOPEN_DECL )
extern FILE* fdopen( int, const char * );
#endif

/*@
    MPE_CallStack_iteratorInit() -- Initialize the iterator for the
                                    back trace of the CallStack.

    Input Parameters:
.   cstk   - Pointer to MPE_CallStack_t.
@*/
void MPE_CallStack_iteratorInit( MPE_CallStack_t *cstk )
{
    int    pipefds[2];
    int    ierr;
    /* Connect a pipe to the fd used in backtrace_symbols_fd(). */
#ifdef HAVE_WINDOWS_H
    /*FIXME: CreatePipe() --- windows */
#else
    ierr = pipe( pipefds );
#endif
    /* Set the writing end of the pipe non-blocking */
#ifdef HAVE_WINDOWS_H
    /*FIXME: Set write fd non-blocking */
#else
    fcntl( pipefds[1], F_SETFL, O_NONBLOCK );
#endif
    MPE_CallStack_print( cstk, pipefds[1] );
    /* Close the pipe so EOF is sent to the reading end of the pipe */
    close( pipefds[1] );
    cstk->pipefile = fdopen( pipefds[0], "r" );
    memset( cstk->line_buf, 0, MPE_CALLSTACK_MAXLINE );
#if defined( HAVE_PRINTSTACK )
    /* Take off the stackframe that contains this function call. */
    MPE_CallStack_iteratorHasMore( cstk );
#endif
}

/*@
    MPE_CallStack_iteratorHasMore - Check if the iterator contains any
                                    more of CallStack frame.

    Input Parameters:
.   cstk   - Pointer to MPE_CallStack_t.

    Returns:
       true, 1, if a complete record (i.e. with '\n' ) of callstack is found
      false, 0, if the end of stack output is reached or error in reading.
@*/
int MPE_CallStack_iteratorHasMore( MPE_CallStack_t *cstk )
{
    char *line_end;
    if ( fgets( cstk->line_buf, MPE_CALLSTACK_MAXLINE, cstk->pipefile ) ) {
        line_end = strchr( cstk->line_buf, '\n' );
        if ( line_end != NULL ) {
            *line_end = '\0';
            return 1;
        }
    }
    /* Since no more callstack record, close the reading end of the pipe */
    fclose( cstk->pipefile );
    return 0;
}

/*@
    MPE_CallStack_iteratorFetchNext - Returns next frame in the
                                      CallStack iterator.

    Input Parameters:
.   cstk   - Pointer to MPE_CallStack_t.

    Returns:
    the next callstack string without the ending '\n'.
@*/
const char* MPE_CallStack_iteratorFetchNext( MPE_CallStack_t *cstk )
{
    return (const char *) (cstk->line_buf);
}
