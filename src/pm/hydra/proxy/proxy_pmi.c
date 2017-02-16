/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "mpx.h"
#include "proxy.h"

#define PMI_STASH_SIZE (16384)
#define MAX_PMI_ARGS   (1024)

HYD_status proxy_process_pmi_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    int recvd, closed, count;
    char pmi_stash[PMI_STASH_SIZE] = { 0 };
    int i;
    char *tmp_cmd;
    char *pmi_cmd;
    const char *delim = NULL;
    char *args[MAX_PMI_ARGS];
    struct proxy_pmi_handle *h;
    struct proxy_kv_hash *pmi_args = NULL;
    struct proxy_kv_hash *hash, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* read enough to see if it is a "cmd=" or "mcmd=" */
    count = 0;
    status =
        HYD_sock_read(fd, pmi_stash, strlen("mcmd="), &recvd, &closed,
                      HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error reading PMI cmd\n");
    count += recvd;

    if (closed) {
        status = HYD_dmx_deregister_fd(fd);
        close(fd);
        goto fn_exit;
    }

    /* see if more data is available */
    status =
        HYD_sock_read(fd, pmi_stash + count, PMI_STASH_SIZE - count, &recvd, &closed,
                      HYD_SOCK_COMM_TYPE__NONBLOCKING);
    HYD_ERR_POP(status, "error reading PMI cmd\n");
    HYD_ASSERT(!closed, status);
    count += recvd;

    /* keep reading till we have at least one full PMI command (in
     * PMI-2 we can get multiple commands before we respond to the
     * first command) */
    while (1) {
        /* parse the string to see if it is a PMI-1 or a PMI-2
         * command */
        if (!strncmp(pmi_stash, "cmd=", strlen("cmd="))) {
            if (pmi_stash[count - 1] != '\n') {
                /* if we reached the end of the buffer we read and did
                 * not yet get a full comamnd, wait till we get at
                 * least one more byte */
                status =
                    HYD_sock_read(fd, pmi_stash + count, 1, &recvd, &closed,
                                  HYD_SOCK_COMM_TYPE__BLOCKING);
                HYD_ERR_POP(status, "error reading PMI cmd\n");
                HYD_ASSERT(!closed, status);
                count += recvd;

                /* see if more data is available */
                status =
                    HYD_sock_read(fd, pmi_stash + count, PMI_STASH_SIZE - count, &recvd, &closed,
                                  HYD_SOCK_COMM_TYPE__NONBLOCKING);
                HYD_ERR_POP(status, "error reading PMI cmd\n");
                HYD_ASSERT(!closed, status);
                count += recvd;

                continue;
            }

            /* overwrite the newline character */
            pmi_stash[count - 1] = 0;
            delim = " ";
        }
        else if (!strncmp(pmi_stash, "mcmd=", strlen("mcmd="))) {
            if (count < strlen("endcmd") ||
                strncmp(&pmi_stash[count - strlen("endcmd")], "endcmd", strlen("endcmd"))) {
                /* if we reached the end of the buffer we read and did
                 * not yet get a full comamnd, wait till we get at
                 * least one more byte */
                status =
                    HYD_sock_read(fd, pmi_stash + count, 1, &recvd, &closed,
                                  HYD_SOCK_COMM_TYPE__BLOCKING);
                HYD_ERR_POP(status, "error reading PMI cmd\n");
                HYD_ASSERT(!closed, status);
                count += recvd;

                /* see if more data is available */
                status =
                    HYD_sock_read(fd, pmi_stash + count, PMI_STASH_SIZE - count, &recvd, &closed,
                                  HYD_SOCK_COMM_TYPE__NONBLOCKING);
                HYD_ERR_POP(status, "error reading PMI cmd\n");
                HYD_ASSERT(!closed, status);
                count += recvd;

                continue;
            }

            /* remove the last "endcmd" and the newline before that */
            pmi_stash[count - strlen("endcmd") - 1] = 0;
            delim = "\n";
        }

        if (proxy_params.debug)
            HYD_PRINT(stdout, "pmi cmd from fd %d: %s\n", fd, pmi_stash);

        tmp_cmd = strtok(pmi_stash, delim);
        for (i = 0;; i++) {
            args[i] = strtok(NULL, delim);
            if (args[i] == NULL)
                break;
        }

        pmi_cmd = strtok(tmp_cmd, "=");
        pmi_cmd = strtok(NULL, "=");

        /* split out the args into tokens */
        for (i = 0; args[i]; i++) {
            HYD_MALLOC(hash, struct proxy_kv_hash *, sizeof(struct proxy_kv_hash), status);
            hash->key = MPL_strdup(strtok(args[i], "="));
            hash->val = MPL_strdup(strtok(NULL, "="));
            MPL_HASH_ADD_STR(pmi_args, key, hash);
        }

        /* try to find the handler and run it */
        h = proxy_pmi_handlers;
        while (h->handler) {
            if (!strcmp(pmi_cmd, h->cmd)) {
                status = h->handler(fd, pmi_args);
                HYD_ERR_POP(status, "PMI handler returned error\n");
                break;
            }
            h++;
        }

        if (h->handler == NULL) {
            /* we don't understand the command; forward it upstream */
            HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "unknown PMI command: %s\n", pmi_cmd);
        }

        MPL_HASH_ITER(hh, pmi_args, hash, tmp) {
            MPL_HASH_DEL(pmi_args, hash);
            MPL_free(hash->key);
            MPL_free(hash->val);
            MPL_free(hash);
        }

        break;
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
