/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mpi.h>

#include <armci_internals.h>
#include <debug.h>

/* Set the default debugging message classes to enable.
 */
unsigned DEBUG_CATS_ENABLED = 
    DEBUG_CAT_NONE;
    // DEBUG_CAT_ALL;
    // DEBUG_CAT_ALLOC;
    // DEBUG_CAT_ALLOC | DEBUG_CAT_MEM_REGION;
    // DEBUG_CAT_MUTEX;
    // DEBUG_CAT_GROUPS;


/** Print an assertion failure message and abort the program.
  */
void ARMCII_Assert_fail(const char *expr, const char *msg, const char *file, int line, const char *func) {
  int rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (msg == NULL)
    fprintf(stderr, "[%d] ARMCI assert fail in %s() [%s:%d]: \"%s\"\n", rank, func, file, line, expr);
  else
    fprintf(stderr, "[%d] ARMCI assert fail in %s() [%s:%d]: \"%s\"\n"
                    "[%d] Message: \"%s\"\n", rank, func, file, line, expr, rank, msg);

#if HAVE_EXECINFO_H
  {
#include <execinfo.h>

    const int SIZE = 100;
    int    j, nframes;
    void  *frames[SIZE];
    char **symbols;

    nframes = backtrace(frames, SIZE);
    symbols = backtrace_symbols(frames, nframes);

    if (symbols == NULL)
      perror("Backtrace failure");

    fprintf(stderr, "[%d] Backtrace:\n", rank);
    for (j = 0; j < nframes; j++)
      fprintf(stderr, "[%d]  %2d - %s\n", rank, nframes-j-1, symbols[j]);

    free(symbols);
  }
#endif

  fflush(NULL);
  {
    double stall = MPI_Wtime();
    while (MPI_Wtime() - stall < 1) ;
  }
  MPI_Abort(MPI_COMM_WORLD, -1);
}


/** Print a debugging message.
  */
void ARMCII_Dbg_print_impl(const char *func, const char *format, ...) {
  va_list etc;
  int  disp;
  char string[500];

  disp  = 0;
  disp += snprintf(string, 500, "[%d] %s: ", ARMCI_GROUP_WORLD.rank, func);
  va_start(etc, format);
  disp += vsnprintf(string+disp, 500-disp, format, etc);
  va_end(etc);

  fprintf(stderr, "%s", string);
}


/** Print an ARMCI warning message.
  */
void ARMCII_Warning(const char *fmt, ...) {
  va_list etc;
  int  disp;
  char string[500];

  disp  = 0;
  disp += snprintf(string, 500, "[%d] ARMCI Warning: ", ARMCI_GROUP_WORLD.rank);
  va_start(etc, fmt);
  disp += vsnprintf(string+disp, 500-disp, fmt, etc);
  va_end(etc);

  fprintf(stderr, "%s", string);
  fflush(NULL);
}
