/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
   Define _ISOC99_SOURCE to get snprintf() prototype visible in <stdio.h>
   when it is compiled with --enable-stricttest.
*/
#define _ISOC99_SOURCE

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mpi.h"

#include "connectstuff.h"

static void printTimeStamp(void)
{
    time_t t;
    struct tm *ltime;
    time(&t);
    ltime = localtime(&t);
    printf("%04d-%02d-%02d %02d:%02d:%02d: ", 1900 + ltime->tm_year,
           ltime->tm_mon, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
    fflush(stdout);
}

void safeSleep(double seconds)
{
    struct timespec sleepAmt = { 0, 0 };
    int ret = 0;
    sleepAmt.tv_sec = floor(seconds);
    sleepAmt.tv_nsec = 1e9 * (seconds - floor(seconds));
    ret = nanosleep(&sleepAmt, NULL);
    if (ret == -1) {
        printf("Safesleep returned early. Sorry\n");
    }
}

void printStackTrace()
{
    static char cmd[512];
    int ierr;
    snprintf(cmd, 512, "/bin/sh -c \"/home/eellis/bin/pstack1 %d\"", getpid());
    ierr = system(cmd);
    fflush(stdout);
}

void msg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printTimeStamp();
    vprintf(fmt, ap);
    fflush(stdout);
    va_end(ap);
}

/*
 * You should free the string once you've used it
 */
char *getPortFromFile(const char *fmt, ...)
{
    char fname[PATH_MAX];
    char dirname[PATH_MAX];
    char *retPort;
    char *cerr;
    va_list ap;
    FILE *fp;
    int done = 0;
    int count = 0;              /* Just used for the NFS sync - not really a count */

    retPort = (char *) calloc(MPI_MAX_PORT_NAME + 1, sizeof(char));

    va_start(ap, fmt);
    vsnprintf(fname, PATH_MAX, fmt, ap);
    va_end(ap);

    srand(getpid());

    while (!done) {
        count += rand();
        fp = fopen(fname, "rt");
        if (fp != NULL) {
            cerr = fgets(retPort, MPI_MAX_PORT_NAME, fp);
            fclose(fp);
            /* ignore bogus tag - assume that the real tag must be longer than 8
             * characters */
            if (strlen(retPort) >= 8) {
                done = 1;
            }
        }
        if (!done) {
            int retcode;
            safeSleep(0.1);
            /* force NFS to update by creating and then deleting a subdirectory. Ouch. */
            snprintf(dirname, PATH_MAX, "%s___%d", fname, count);
            retcode = mkdir(dirname, 0777);
            if (retcode != 0) {
                perror("Calling mkdir");
                _exit(9);
            }
            else {
                rmdir(dirname);
            }
        }
    }
    return retPort;
}

/*
 * Returns the filename written to. Free this once you're done.
 */
char *writePortToFile(const char *port, const char *fmt, ...)
{
    char *fname;
    va_list ap;
    FILE *fp;

    fname = (char *) calloc(PATH_MAX, sizeof(char));

    va_start(ap, fmt);
    vsnprintf(fname, PATH_MAX, fmt, ap);
    va_end(ap);

    fp = fopen(fname, "wt");
    fprintf(fp, "%s\n", port);
    fclose(fp);

    msg("Wrote port <%s> to file <%s>\n", port, fname);
    return fname;
}
