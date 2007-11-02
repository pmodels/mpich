/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if defined(USE_WINDOWS_OS)

/* If windows, set the default width to the window size */
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
if (hConsole != INVALID_HANDLE_VALUE)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(hConsole, &info))
    {
	MPIR_Err_chop_width = info.dwMaximumWindowSize.X;
    }
}

#elif defined(USE_GENERIC_UNIX_OS)
/* If Unix and if ncurses is available, you can do the following: */
#include <ncurses.h>
int row, col;
initscr();
getmaxyx(stdscr,row,col);  /* This is a macro */

/* An alternative is to allow mpiexec to provide this information, since
   it is the one that is actually connected to the display in the typical 
   Unix case */

#else 
#endif
