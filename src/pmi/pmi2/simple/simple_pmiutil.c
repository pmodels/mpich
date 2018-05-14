/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Allow fprintf to logfile */
/* style: allow:fprintf:1 sig:0 */

/* Utility functions associated with PMI implementation, but not part of
   the PMI interface itself.  Reading and writing on pipes, signals, and parsing
   key=value messages
*/
#include "mpichconf.h"

#include "pmi2compat.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <stdarg.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#include "pmi2.h"
#include "simple_pmiutil.h"

#define MAXVALLEN 1024
#define MAXKEYLEN   32

/* These are not the keyvals in the keyval space that is part of the
   PMI specification.
   They are just part of this implementation's internal utilities.
*/
struct PMI2U_keyval_pairs {
    char key[MAXKEYLEN];
    char value[MAXVALLEN];
};
static struct PMI2U_keyval_pairs PMI2U_keyval_tab[64] = { {{0}, {0}} };

static int PMI2U_keyval_tab_idx = 0;

/* This is used to prepend printed output.  Set the initial value to
   "unset" */
static char PMI2U_print_id[PMI2U_IDSIZE] = "unset";

void PMI2U_Set_rank(int PMI_rank)
{
    MPL_snprintf(PMI2U_print_id, PMI2U_IDSIZE, "cli_%d", PMI_rank);
}

void PMI2U_SetServer(void)
{
    MPL_strncpy(PMI2U_print_id, "server", PMI2U_IDSIZE);
}

/* Note that vfprintf is part of C89 */

/* style: allow:fprintf:1 sig:0 */
/* style: allow:vfprintf:1 sig:0 */
/* This should be combined with the message routines */
void PMI2U_printf(int print_flag, const char *fmt, ...)
{
    va_list ap;
    static FILE *logfile = 0;

    /* In some cases when we are debugging, the handling of stdout or
     * stderr may be unreliable.  In that case, we make it possible to
     * select an output file. */
    if (!logfile) {
        char *p;
        p = getenv("PMI_USE_LOGFILE");
        if (p) {
            char filename[1024];
            p = getenv("PMI_ID");
            if (p) {
                MPL_snprintf(filename, sizeof(filename), "testclient-%s.out", p);
                logfile = fopen(filename, "w");
            } else {
                logfile = fopen("testserver.out", "w");
            }
        } else
            logfile = stderr;
    }

    if (print_flag) {
        /* PMI2U_Error_printf("[%s]: ", PMI2U_print_id); */
        /* FIXME: Decide what role PMI2U_printf should have (if any) and
         * select the appropriate PMI2U routine */
        fprintf(logfile, "[%s]: ", PMI2U_print_id);
        va_start(ap, fmt);
        vfprintf(logfile, fmt, ap);
        va_end(ap);
        fflush(logfile);
    }
}

#define MAX_READLINE 1024
/*
 * Return the next newline-terminated string of maximum length maxlen.
 * This is a buffered version, and reads from fd as necessary.  A
 */
int PMI2U_readline(int fd, char *buf, int maxlen)
{
    static char readbuf[MAX_READLINE];
    static char *nextChar = 0, *lastChar = 0;   /* lastChar is really one past
                                                 * last char */
    static int lastErrno = 0;
    static int lastfd = -1;
    int curlen, n;
    char *p, ch;

    /* Note: On the client side, only one thread at a time should
     * be calling this, and there should only be a single fd.
     * Server side code should not use this routine (see the
     * replacement version in src/pm/util/pmiserv.c) */
    PMI2U_Assert(nextChar == lastChar || fd == lastfd);

    p = buf;
    curlen = 1; /* Make room for the null */
    while (curlen < maxlen) {
        if (nextChar == lastChar) {
            lastfd = fd;
            do {
                n = read(fd, readbuf, sizeof(readbuf) - 1);
            } while (n == -1 && errno == EINTR);
            if (n == 0) {
                /* EOF */
                break;
            } else if (n < 0) {
                /* Error.  Return a negative value if there is no
                 * data.  Save the errno in case we need to return it
                 * later. */
                lastErrno = errno;
                if (curlen == 1) {
                    curlen = 0;
                }
                break;
            }
            nextChar = readbuf;
            lastChar = readbuf + n;
            /* Add a null at the end just to make it easier to print
             * the read buffer */
            readbuf[n] = 0;
            /* FIXME: Make this an optional output */
            /* printf("Readline %s\n", readbuf); */
        }

        ch = *nextChar++;
        *p++ = ch;
        curlen++;
        if (ch == '\n')
            break;
    }

    /* We null terminate the string for convenience in printing */
    *p = 0;

    printf_d("PMI received: %s\n", buf);

    /* Return the number of characters, not counting the null */
    return curlen - 1;
}

int PMI2U_writeline(int fd, char *buf)
{
    int size, n;

    size = strlen(buf);
    /*
     * if (size > PMI2U_MAXLINE) {
     * buf[PMI2U_MAXLINE-1] = '\0';
     * PMI2U_printf(1, "write_line: message string too big: :%s:\n", buf);
     * }
     * else */ if (buf[strlen(buf) - 1] != '\n')
        /* error:  no newline at end */
        PMI2U_printf(1, "write_line: message string doesn't end in newline: :%s:\n", buf);
    else {
        printf_d("PMI sending: %s\n", buf);

        do {
            n = write(fd, buf, size);
        } while (n == -1 && errno == EINTR);

        if (n < 0) {
            PMI2U_printf(1, "write_line error; fd=%d buf=:%s:\n", fd, buf);
            perror("system msg for write_line failure ");
            return PMI2_FAIL;
        }
        if (n < size)
            PMI2U_printf(1, "write_line failed to write entire message\n");
    }
    return PMI2_SUCCESS;
}

/*
 * Given an input string st, parse it into internal storage that can be
 * queried by routines such as PMI2U_getval.
 */
int PMI2U_parse_keyvals(char *st)
{
    char *p, *keystart, *valstart;
    int offset;

    if (!st)
        return PMI2_FAIL;

    PMI2U_keyval_tab_idx = 0;
    p = st;
    while (1) {
        while (*p == ' ')
            p++;
        /* got non-blank */
        if (*p == '=') {
            PMI2U_printf(1, "PMI2U_parse_keyvals:  unexpected = at character %d in %s\n",
                         p - st, st);
            return PMI2_FAIL;
        }
        if (*p == '\n' || *p == '\0')
            return PMI2_SUCCESS;        /* normal exit */
        /* got normal character */
        keystart = p;   /* remember where key started */
        while (*p != ' ' && *p != '=' && *p != '\n' && *p != '\0')
            p++;
        if (*p == ' ' || *p == '\n' || *p == '\0') {
            PMI2U_printf(1,
                         "PMI2U_parse_keyvals: unexpected key delimiter at character %d in %s\n",
                         p - st, st);
            return PMI2_FAIL;
        }
        /* Null terminate the key */
        *p = 0;
        /* store key */
        MPL_strncpy(PMI2U_keyval_tab[PMI2U_keyval_tab_idx].key, keystart, MAXKEYLEN);

        valstart = ++p; /* start of value */
        while (*p != ' ' && *p != '\n' && *p != '\0')
            p++;
        /* store value */
        MPL_strncpy(PMI2U_keyval_tab[PMI2U_keyval_tab_idx].value, valstart, MAXVALLEN);
        offset = p - valstart;
        /* When compiled with -fPIC, the pgcc compiler generates incorrect
         * code if "p - valstart" is used instead of using the
         * intermediate offset */
        PMI2U_keyval_tab[PMI2U_keyval_tab_idx].value[offset] = '\0';
        PMI2U_keyval_tab_idx++;
        if (*p == ' ')
            continue;
        if (*p == '\n' || *p == '\0')
            return PMI2_FAIL;   /* value has been set to empty */
    }
}

void PMI2U_dump_keyvals(void)
{
    int i;
    for (i = 0; i < PMI2U_keyval_tab_idx; i++)
        PMI2U_printf(1, "  %s=%s\n", PMI2U_keyval_tab[i].key, PMI2U_keyval_tab[i].value);
}

char *PMI2U_getval(const char *keystr, char *valstr, int vallen)
{
    int i, rc;

    for (i = 0; i < PMI2U_keyval_tab_idx; i++) {
        if (strcmp(keystr, PMI2U_keyval_tab[i].key) == 0) {
            rc = MPL_strncpy(valstr, PMI2U_keyval_tab[i].value, vallen);
            if (rc != 0) {
                PMI2U_printf(1, "MPL_strncpy failed in PMI2U_getval\n");
                return NULL;
            }
            return valstr;
        }
    }
    valstr[0] = '\0';
    return NULL;
}

void PMI2U_chgval(const char *keystr, char *valstr)
{
    int i;

    for (i = 0; i < PMI2U_keyval_tab_idx; i++) {
        if (strcmp(keystr, PMI2U_keyval_tab[i].key) == 0) {
            MPL_strncpy(PMI2U_keyval_tab[i].value, valstr, MAXVALLEN - 1);
            PMI2U_keyval_tab[i].value[MAXVALLEN - 1] = '\0';
        }
    }
}
