/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Allow fprintf to logfile */
/* style: allow:fprintf:1 sig:0 */

/* Utility functions associated with PMI implementation, but not part of
   the PMI interface itself.  Reading and writing on pipes, signals, and parsing
   key=value messages
*/

#include "pmi_config.h"
#include "mpl.h"

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

#include "pmi.h"
#include "pmi_util.h"
#include "utlist.h"

#define MAXVALLEN 1024
#define MAXKEYLEN   32

int PMIU_is_threaded = 0;
MPL_thread_mutex_t PMIU_mutex;

int PMIU_verbose = 0;           /* Set this to true to print PMI debugging info */

/* These are not the keyvals in the keyval space that is part of the
   PMI specification.
   They are just part of this implementation's internal utilities.
*/
struct PMIU_keyval_pairs {
    char key[MAXKEYLEN];
    char value[MAXVALLEN];
};
static struct PMIU_keyval_pairs PMIU_keyval_tab[64];

static int PMIU_keyval_tab_idx = 0;

/* This is used to prepend printed output.  Set the initial value to
   "unset" */
static char PMIU_print_id[PMIU_IDSIZE] = "unset";

void PMIU_thread_init(void)
{
    int ret;
    MPL_thread_mutex_create(&PMIU_mutex, &ret);
    PMIU_Assert(ret == 0);
}

void PMIU_Set_rank(int PMI_rank)
{
    snprintf(PMIU_print_id, PMIU_IDSIZE, "cli_%d", PMI_rank);
}

void PMIU_SetServer(void)
{
    MPL_strncpy(PMIU_print_id, "server", PMIU_IDSIZE);
}

void PMIU_Set_rank_kvsname(int rank, const char *kvsname)
{
    if (strlen(kvsname) < PMIU_IDSIZE - 10) {
        snprintf(PMIU_print_id, PMIU_IDSIZE, "%s-%d", kvsname, rank);
    } else {
        snprintf(PMIU_print_id, PMIU_IDSIZE, "cli-%d", rank);
    }
}

/* Note that vfprintf is part of C89 */

/* style: allow:fprintf:1 sig:0 */
/* style: allow:vfprintf:1 sig:0 */
/* This should be combined with the message routines */
void PMIU_printf(int print_flag, const char *fmt, ...)
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
                snprintf(filename, sizeof(filename), "testclient-%s.out", p);
                logfile = fopen(filename, "w");
            } else {
                logfile = fopen("testserver.out", "w");
            }
        } else
            logfile = stderr;
    }

    if (print_flag) {
        /* MPL_error_printf("[%s]: ", PMIU_print_id); */
        /* FIXME: Decide what role PMIU_printf should have (if any) and
         * select the appropriate MPIU routine */
        fprintf(logfile, "[%s]: ", PMIU_print_id);
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
int PMIU_readline(int fd, char *buf, int maxlen)
{
    static char readbuf[MAX_READLINE];
    static char *nextChar = 0, *lastChar = 0;   /* lastChar is really one past
                                                 * last char */
    int pmi_version = 0;
    int pmi2_cmd_len = 0;

    static int lastfd = -1;
    ssize_t n;
    int curlen;
    char *p, ch;

    /* Note: On the client side, only one thread at a time should
     * be calling this, and there should only be a single fd.
     * Server side code should not use this routine (see the
     * replacement version in src/pm/util/pmiserv.c) */
    if (nextChar != lastChar && fd != lastfd) {
        MPL_internal_error_printf("Panic - buffer inconsistent\n");
        return PMI_FAIL;
    }

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
        if (curlen == 7) {
            if (strncmp(buf, "cmd=", 4) == 0) {
                pmi_version = 1;
            } else {
                pmi_version = 2;

                char len_str[7];
                memcpy(len_str, buf, 6);
                len_str[6] = '\0';
                pmi2_cmd_len = atoi(len_str);
            }
        }
        if (pmi_version == 1) {
            if (ch == '\n') {
                break;
            }
        } else if (pmi_version == 2) {
            if (curlen == pmi2_cmd_len + 6 + 1) {
                break;
            }
        }
    }

    /* We null terminate the string for convenience in printing */
    *p = 0;

    /* Return the number of characters, not counting the null */
    return curlen - 1;
}

/* Read and return the next full PMI message in a buffer */
/* NOTE: no race detection. We assume PMI is only called serailly */

struct last_read {
    int fd;
    char *buf;
    int len;
    struct last_read *prev, *next;
};

struct last_read *last_read_list;

int PMIU_read_cmd(int fd, char **buf_out, int *buflen_out)
{
    int pmi_errno = PMIU_SUCCESS;

    int cmd_len = 0;
    int buflen = 0;
    int bufsize = 0;
    char *buf;

    /* NOTE: bufsize always need 1 extra to ensure '\0' termination */
    bufsize = MAX_READLINE;
    PMIU_CHK_MALLOC(buf, char *, bufsize, pmi_errno, PMIU_ERR_NOMEM, "buf");

    int wire_version, pmi2_cmd_len;
    wire_version = 0;
    pmi2_cmd_len = 0;
    while (1) {
        int n = 0;
        /* -- read more -- */
        if (last_read_list) {
            struct last_read *p;
            DL_FOREACH(last_read_list, p) {
                if (p->fd == fd) {
                    if (bufsize - buflen - 1 < p->len) {
                        bufsize += MAX_READLINE;
                        PMIU_REALLOC_ORJUMP(buf, bufsize, pmi_errno);
                    }
                    /* transfer from what is already read */
                    memcpy(buf + buflen, p->buf, p->len);
                    n += p->len;
                    DL_DELETE(last_read_list, p);
                    MPL_free(p->buf);
                    MPL_free(p);
                    break;
                }
            }
        }
        if (n == 0) {
            do {
                if (bufsize - buflen - 1 < 100) {
                    bufsize += MAX_READLINE;
                    PMIU_REALLOC_ORJUMP(buf, bufsize, pmi_errno);
                }
                n = read(fd, buf + buflen, bufsize - buflen - 1);
            } while (n == -1 && errno == EINTR);
            if (n == 0) {
                /* EOF */
                break;
            } else if (n < 0) {
                PMIU_ERR_SETANDJUMP(pmi_errno, PMI_FAIL, "Error in PMIU_read_cmd.\n");
            }
        }

        char *s = buf + buflen;
        buflen += n;
        if (wire_version == 0) {
            /* -- check wire protocol -- */
            if (buflen > 6) {
                if (strncmp(buf, "cmd=", 4) == 0) {
                    wire_version = 1;
                } else {
                    wire_version = 2;
                    char len_str[7];
                    memcpy(len_str, buf, 6);
                    len_str[6] = '\0';
                    pmi2_cmd_len = atoi(len_str) + 6;
                    PMIU_Assert(pmi2_cmd_len > 10);
                    if (bufsize < pmi2_cmd_len + 1) {
                        bufsize = pmi2_cmd_len + 1;
                        PMIU_REALLOC_ORJUMP(buf, bufsize, pmi_errno);
                    }
                }
            }
        }

        /* -- check whether we have the full message -- */
        int got_full_cmd = 0;
        if (wire_version == 1) {
            /* newline marks the end of a PMI-1 message */
            for (int i = 0; i < n; i++) {
                if (s[i] == '\n') {
                    cmd_len = (s - buf) + i + 1;
                    got_full_cmd = 1;
                    break;
                }
            }
        } else {
            if (pmi2_cmd_len > 0 && buflen >= pmi2_cmd_len) {
                cmd_len = pmi2_cmd_len;
                got_full_cmd = 1;
            }
        }
        if (got_full_cmd) {
            /* save the remainder for next read if any */
            if (buflen > cmd_len) {
                struct last_read *p;
                p = MPL_malloc(sizeof(*p), MPL_MEM_OTHER);
                PMIU_Assert(p);
                p->fd = fd;
                p->len = buflen - cmd_len;
                p->buf = MPL_malloc(p->len, MPL_MEM_OTHER);
                PMIU_Assert(p->buf);
                memcpy(p->buf, buf + cmd_len, p->len);
                DL_APPEND(last_read_list, p);
            }
            break;
        }
    }

    if (cmd_len == 0) {
        PMIU_Free(buf);
        *buf_out = NULL;
        *buflen_out = 0;
    } else {
        buf[cmd_len] = '\0';
        *buf_out = buf;
        *buflen_out = cmd_len + 1;
    }

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

int PMIU_write(int fd, char *buf, int buflen)
{
    char *p = buf;
    ssize_t rem = buflen;
    ssize_t n;

    while (rem > 0) {
        do {
            n = write(fd, p, rem);
        } while (n == -1 && errno == EINTR);

        if (n < 0) {
            PMIU_printf(1, "PMIU_write error; fd=%d buf=:%s:\n", fd, buf);
            perror("system msg for write_line failure ");
            return PMI_FAIL;
        }

        rem -= n;
        p += n;
    };

    return PMI_SUCCESS;
}

int PMIU_writeline(int fd, char *buf)
{
    ssize_t size, n;

    size = strlen(buf);
    if (size > PMIU_MAXLINE) {
        buf[PMIU_MAXLINE - 1] = '\0';
        PMIU_printf(1, "write_line: message string too big: :%s:\n", buf);
    } else if (buf[strlen(buf) - 1] != '\n')    /* error:  no newline at end */
        PMIU_printf(1, "write_line: message string doesn't end in newline: :%s:\n", buf);
    else {
        do {
            n = write(fd, buf, size);
        } while (n == -1 && errno == EINTR);

        if (n < 0) {
            PMIU_printf(1, "write_line error; fd=%d buf=:%s:\n", fd, buf);
            perror("system msg for write_line failure ");
            return PMI_FAIL;
        }
        if (n < size)
            PMIU_printf(1, "write_line failed to write entire message\n");
    }
    return PMI_SUCCESS;
}

/*
 * Given an input string st, parse it into internal storage that can be
 * queried by routines such as PMIU_getval.
 */
int PMIU_parse_keyvals(char *st)
{
    char *p, *keystart, *valstart;
    int offset;

    if (!st)
        return PMI_FAIL;

    PMIU_keyval_tab_idx = 0;
    p = st;
    while (1) {
        while (*p == ' ')
            p++;
        /* got non-blank */
        if (*p == '=') {
            PMIU_printf(1, "PMIU_parse_keyvals:  unexpected = at character %d in %s\n", p - st, st);
            return PMI_FAIL;
        }
        if (*p == '\n' || *p == '\0')
            return PMI_SUCCESS; /* normal exit */
        /* got normal character */
        keystart = p;   /* remember where key started */
        while (*p != ' ' && *p != '=' && *p != '\n' && *p != '\0')
            p++;
        if (*p == ' ' || *p == '\n' || *p == '\0') {
            PMIU_printf(1,
                        "PMIU_parse_keyvals: unexpected key delimiter at character %d in %s\n",
                        p - st, st);
            return PMI_FAIL;
        }
        /* Null terminate the key */
        *p = 0;
        /* store key */
        MPL_strncpy(PMIU_keyval_tab[PMIU_keyval_tab_idx].key, keystart, MAXKEYLEN);

        valstart = ++p; /* start of value */
        while (*p != ' ' && *p != '\n' && *p != '\0')
            p++;
        /* store value */
        MPL_strncpy(PMIU_keyval_tab[PMIU_keyval_tab_idx].value, valstart, MAXVALLEN);
        offset = (int) (p - valstart);
        /* When compiled with -fPIC, the pgcc compiler generates incorrect
         * code if "p - valstart" is used instead of using the
         * intermediate offset */
        PMIU_keyval_tab[PMIU_keyval_tab_idx].value[offset] = '\0';
        PMIU_keyval_tab_idx++;
        if (*p == ' ')
            continue;
        if (*p == '\n' || *p == '\0')
            return PMI_SUCCESS; /* value has been set to empty */
    }
}

void PMIU_dump_keyvals(void)
{
    int i;
    for (i = 0; i < PMIU_keyval_tab_idx; i++)
        PMIU_printf(1, "  %s=%s\n", PMIU_keyval_tab[i].key, PMIU_keyval_tab[i].value);
}

char *PMIU_getval(const char *keystr, char *valstr, int vallen)
{
    int i, rc;

    for (i = 0; i < PMIU_keyval_tab_idx; i++) {
        if (strcmp(keystr, PMIU_keyval_tab[i].key) == 0) {
            rc = MPL_strncpy(valstr, PMIU_keyval_tab[i].value, vallen);
            if (rc != 0) {
                PMIU_printf(1, "MPL_strncpy failed in PMIU_getval\n");
                return NULL;
            }
            return valstr;
        }
    }
    valstr[0] = '\0';
    return NULL;
}

void PMIU_chgval(const char *keystr, char *valstr)
{
    int i;

    for (i = 0; i < PMIU_keyval_tab_idx; i++) {
        if (strcmp(keystr, PMIU_keyval_tab[i].key) == 0) {
            MPL_strncpy(PMIU_keyval_tab[i].value, valstr, MAXVALLEN - 1);
            PMIU_keyval_tab[i].value[MAXVALLEN - 1] = '\0';
        }
    }
}

/* connect to a specified host/port instead of using a
   specified fd inherited from a parent process */
static int connect_to_pm(char *hostname, int portnum)
{
    int ret;
    MPL_sockaddr_t addr;
    int fd;
    int optval = 1;
    int q_wait = 1;

    ret = MPL_get_sockaddr(hostname, &addr);
    if (ret) {
        PMIU_printf(1, "Unable to get host entry for %s\n", hostname);
        return -1;
    }

    fd = MPL_socket();
    if (fd < 0) {
        PMIU_printf(1, "Unable to get AF_INET socket\n");
        return -1;
    }

    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &optval, sizeof(optval))) {
        perror("Error calling setsockopt:");
    }

    ret = MPL_connect(fd, &addr, portnum);
    /* We wait here for the connection to succeed */
    if (ret) {
        switch (errno) {
            case ECONNREFUSED:
                PMIU_printf(1, "connect failed with connection refused\n");
                /* (close socket, get new socket, try again) */
                if (q_wait)
                    close(fd);
                return -1;

            case EINPROGRESS:  /*  (nonblocking) - select for writing. */
                break;

            case EISCONN:      /*  (already connected) */
                break;

            case ETIMEDOUT:    /* timed out */
                PMIU_printf(1, "connect failed with timeout\n");
                return -1;

            default:
                PMIU_printf(1, "connect failed with errno %d\n", errno);
                return -1;
        }
    }

    return fd;
}

/* Get the FD to use for PMI operations.  If a port is used, rather than
   a pre-established FD (i.e., via pipe), an additional handshake is needed.
*/
int PMIU_get_pmi_fd(int *pmi_fd, bool * do_handshake)
{
    int pmi_errno = PMIU_SUCCESS;
    char *p;

    /* Set the default */
    *pmi_fd = -1;
    *do_handshake = false;

    p = getenv("PMI_FD");
    if (p) {
        *pmi_fd = atoi(p);
        goto fn_exit;
    }

    p = getenv("PMI_PORT");
    if (p) {
        int portnum;
        char hostname[MAXHOSTNAME + 1];
        char *pn, *ph;

        /* Connect to the indicated port (in format hostname:portnumber)
         * and get the fd for the socket */

        /* Split p into host and port */
        pn = p;
        ph = hostname;
        while (*pn && *pn != ':' && (ph - hostname) < MAXHOSTNAME) {
            *ph++ = *pn++;
        }
        *ph = 0;

        PMIU_ERR_CHKANDJUMP1(*pn != ':', pmi_errno, PMIU_FAIL, "**pmi2_port %s", p);

        portnum = atoi(pn + 1);
        /* FIXME: Check for valid integer after : */
        *pmi_fd = connect_to_pm(hostname, portnum);
        PMIU_ERR_CHKANDJUMP2(*pmi_fd < 0, pmi_errno, PMIU_FAIL,
                             "**connect_to_pm %s %d", hostname, portnum);
        *do_handshake = true;
    }

    /* OK to return success for singleton init */

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}
