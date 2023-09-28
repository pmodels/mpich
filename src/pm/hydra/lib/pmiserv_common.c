/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "pmiserv_common.h"
#include "topo.h"

/* There are two types of wire protocols:
 *   1. direct HDR protocols, used between mpiexec and proxies. It's
 *      a direct struct HYD_pmcd_hdr followed with a potential payload.
 *   2. PMI wire protocols, text string based. There are two versions:
 *      * PMI-1: cmd=xxx attr1=xxx ... attrn=xxx\n
 *      * PMI-2: length cmd=xxx;attr1=xxx;...;attrn=xxx;
 *               , where length is a 6-character decimal number.
 *
 * This file provides utilitis for handling these wire protocols.
 */

void HYD_pmcd_init_header(struct HYD_pmcd_hdr *hdr)
{
    memset(hdr, 0, sizeof(struct HYD_pmcd_hdr));
    hdr->cmd = CMD_INVALID;
    hdr->buflen = -1;
}

void HYD_pmcd_pmi_dump(struct PMIU_cmd *pmicmd)
{
    char *buf = NULL;
    int buflen = 0;

    PMIU_cmd_output(pmicmd, &buf, &buflen);
    if (buf[buflen - 1] == '\n') {
        HYDU_dump_noprefix(stdout, "%s", buf);
    } else {
        HYDU_dump_noprefix(stdout, "%s\n", buf);
    }
}

HYD_status HYD_pmcd_pmi_send(int fd, struct PMIU_cmd *pmi, struct HYD_pmcd_hdr *hdr, int debug)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    char *buf = NULL;
    int buflen = 0;

    int pmi_errno = PMIU_cmd_output(pmi, &buf, &buflen);
    HYDU_ASSERT(!pmi_errno, status);

    if (debug) {
        if (hdr) {
            HYDU_dump(stdout, "Sending internal PMI command (proxy:%d:%d):\n",
                      hdr->pgid, hdr->proxy_id);
        } else {
            HYDU_dump(stdout, "Sending PMI command:\n");
        }
        if (buf[buflen - 1] == '\n') {
            HYDU_dump_noprefix(stdout, "    %s", buf);
        } else {
            HYDU_dump_noprefix(stdout, "%s\n", buf);
        }
    }

    int sent = 0, closed;
    if (hdr) {
        hdr->buflen = buflen;
        hdr->u.pmi.pmi_version = pmi->version;

        status = HYDU_sock_write(fd, hdr, sizeof(*hdr), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to send hdr\n");
        HYDU_ASSERT(!closed, status);
    }

    status = HYDU_sock_write(fd, buf, buflen, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI message\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

const char *HYD_pmcd_cmd_name(int cmd)
{
    switch (cmd) {
        case CMD_INVALID:
            return "CMD_INVALID";
        case CMD_PROC_INFO:
            return "CMD_PROC_INFO";
        case CMD_PMI_RESPONSE:
            return "CMD_PMI_RESPONSE";
        case CMD_SIGNAL:
            return "CMD_SIGNAL";
        case CMD_STDIN:
            return "CMD_STDIN";
        case CMD_PID_LIST:
            return "CMD_PID_LIST";
        case CMD_EXIT_STATUS:
            return "CMD_EXIT_STATUS";
        case CMD_PMI:
            return "CMD_PMI";
        case CMD_STDOUT:
            return "CMD_STDOUT";
        case CMD_STDERR:
            return "CMD_STDERR";
        case CMD_PROCESS_TERMINATED:
            return "CMD_PROCESS_TERMINATED";
        default:
            return "UNKNOWN";
    }
}
