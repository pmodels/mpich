/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "hydra_utils.h"
#include "pmi_common.h"
#include "bind.h"

HYD_status HYD_pmcd_pmi_read_pmi_cmd(int fd, char **buf, int *pmi_version, int *closed)
{
    int buflen, linelen, cmdlen;
    char *bufptr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    buflen = HYD_TMPBUF_SIZE;

    HYDU_MALLOC(*buf, char *, buflen, status);
    bufptr = *buf;

    /* FIXME: We initialize to whatever PMI version we detect while
     * reading the PMI command, instead of relying on what the init
     * command gave us. This part of the code should not know anything
     * about PMI-1 vs. PMI-2. But the simple PMI client-side code is
     * so hacked up, that commands can arrive out-of-order and this is
     * necessary. */

    *closed = 0;
    do {
        status = HYDU_sock_read(fd, bufptr, 6, &linelen, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read the length of the command");
        buflen -= linelen;
        bufptr += linelen;

        /* Unexpected termination of connection */
        if (linelen == 0) {
            *closed = 1;
            break;
        }

        /* If we get "cmd=" here, we just assume that this is PMI-1
         * format (or a PMI-2 command that is backward compatible). */
        if (!strncmp(*buf, "cmd=", strlen("cmd=")) ||
            !strncmp(*buf, "mcmd=", strlen("mcmd="))) {
            /* PMI-1 format command; read the rest of it */
            *pmi_version = 1;

            while (1) {
                status = HYDU_sock_readline(fd, bufptr, buflen, &linelen);
                HYDU_ERR_POP(status, "PMI read line error\n");
                buflen -= linelen;
                bufptr += linelen;

                /* Unexpected termination of connection */
                if (!linelen) {
                    *closed = 1;
                    break;
                }

                if (!strncmp(*buf, "cmd=", strlen("cmd=")) ||
                    !strncmp(bufptr - strlen("endcmd\n"), "endcmd", strlen("endcmd")))
                    break;
                else
                    *(bufptr - 1) = ' ';
            }

            /* Unexpected termination of connection */
            if (*closed)
                break;

            *(bufptr - 1) = '\0';
        }
        else {  /* PMI-2 command */
            *pmi_version = 2;

            *bufptr = '\0';
            cmdlen = atoi(*buf);

            status = HYDU_sock_read(fd, *buf, cmdlen, &linelen, HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "PMI read line error\n");
            (*buf)[linelen] = 0;

            /* Unexpected termination of connection */
            if (!linelen) {
                *closed = 1;
                break;
            }
        }
    } while (0);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_parse_pmi_cmd(char *buf, int pmi_version, char **pmi_cmd, char *args[])
{
    char *tbuf = NULL, *seg, *str1 = NULL, *cmd;
    char *tmp[HYD_NUM_TMP_STRINGS], *targs[HYD_NUM_TMP_STRINGS];
    const char *delim;
    int i, j, k;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (pmi_version == 1) {
        if (!strncmp(buf, "cmd=", strlen("cmd=")))
            delim = " ";
        else
            delim = "\n";

        /* Here we only get PMI-1 commands or backward compatible
         * PMI-2 commands, so we always explicitly use the PMI-1
         * delimiter. This allows us to get backward-compatible PMI-2
         * commands interleaved with regular PMI-2 commands. */
        tbuf = HYDU_strdup(buf);
        cmd = strtok(tbuf, delim);
        for (i = 0; i < HYD_NUM_TMP_STRINGS; i++) {
            targs[i] = strtok(NULL, delim);
            if (targs[i] == NULL)
                break;
        }

        /* Make a pass through targs and merge space separated
         * arguments which are actually part of the same key */
        k = 0;
        for (i = 0; targs[i]; i++) {
            if (!strrchr(targs[i], ' ')) {
                /* no spaces */
                args[k++] = HYDU_strdup(targs[i]);
            }
            else {
                /* space in the argument; each segment is either a new
                 * key, or a space-separated part of the previous
                 * key */
                j = 0;
                seg = strtok(targs[i], " ");
                while (1) {
                    if (!seg || strrchr(seg, '=')) {
                        /* segment has an '='; it's a start of a new key */
                        if (j) {
                            tmp[j++] = NULL;
                            status = HYDU_str_alloc_and_join(tmp, &args[k++]);
                            HYDU_ERR_POP(status, "error while joining strings\n");
                            HYDU_free_strlist(tmp);
                        }
                        j = 0;

                        if (!seg)
                            break;
                    }
                    else {
                        /* no '='; part of the previous key */
                        tmp[j++] = HYDU_strdup(" ");
                    }
                    tmp[j++] = HYDU_strdup(seg);

                    seg = strtok(NULL, " ");
                }
            }
        }
        args[k++] = NULL;
    }
    else { /* PMI-v2 */
        delim = ";";

        tbuf = HYDU_strdup(buf);
        cmd = strtok(tbuf, delim);
        for (i = 0; i < HYD_NUM_TMP_STRINGS; i++) {
            args[i] = strtok(NULL, delim);
            args[i] = HYDU_strdup(args[i]);
            if (args[i] == NULL)
                break;
        }
    }

    /* Search for the PMI command in our table */
    status = HYDU_strsplit(cmd, &str1, pmi_cmd, '=');
    HYDU_ERR_POP(status, "string split returned error\n");

  fn_exit:
    if (tbuf)
        HYDU_FREE(tbuf);
    if (str1)
        HYDU_FREE(str1);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
