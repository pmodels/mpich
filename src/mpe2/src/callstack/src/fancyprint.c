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

#include "mpe_callstack.h"

/*@
    void MPE_CallStack_fancyprint - A decorated version of
                                    MPE_CallStack_print()

    Input Parameters:
+ cstk        - pointer to the MPE_CallStack_t data structure.
. fd          - file descriptor of where output will go.
. prefix      - prefix string for each line of output.
. printidx    - boolean variable to indicate to print stack index,
                1=true, 0=false.
- maxframes   - maximum number of frames to be printed.
                MPE_CALLSTACK_UNLIMITED means unlimited frames.
@*/
void MPE_CallStack_fancyprint( MPE_CallStack_t *cstk, int fd,
                               const char      *prefix,
                                     int        printidx,
                                     int        maxframes )
{
    char   strbuf[ MPE_CALLSTACK_MAXLINE ];
    int    printf_mode, idx;
    int    ierr;

    MPE_CallStack_iteratorInit( cstk );
#if defined( HAVE_PRINTSTACK )
    /* Take off the stackframe that contains this function call. */
    MPE_CallStack_iteratorHasMore( cstk );
#endif

    printf_mode = printidx ? 1 : 0;
    printf_mode = prefix != NULL ? printf_mode+2 : printf_mode; 

    idx = 0; 
    while ( MPE_CallStack_iteratorHasMore( cstk ) && idx < maxframes ) {
        switch( printf_mode ) {
            case 3:
                sprintf( strbuf, "%s[%d]: %s\n", prefix, idx,
                         MPE_CallStack_iteratorFetchNext( cstk ) );
                break;
            case 2:
                sprintf( strbuf, "%s%s\n", prefix,
                         MPE_CallStack_iteratorFetchNext( cstk ) );
                break;
            case 1:
                sprintf( strbuf, "[%d]: %s\n", idx,
                         MPE_CallStack_iteratorFetchNext( cstk ) );
                break;
            default:
                sprintf( strbuf, "%s\n",
                         MPE_CallStack_iteratorFetchNext( cstk ) );
        }
        ierr = write( fd, strbuf, strlen(strbuf)+1 );
        idx++ ;
    }
}
