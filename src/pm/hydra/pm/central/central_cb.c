/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "csi.h"
#include "pmcu_pmi.h"
#include "pmci.h"
#include "bsci.h"
#include "demux.h"
#include "central.h"

#define MAX_BUFFER_SIZE (64 * 1024)

int HYD_PMCD_Central_listenfd;
HYD_CSI_Handle * csi_handle;

/*
 * HYD_PMCD_Central_cb: This is the core PMI server part of the
 * code. Here's the expected protocol:
 *
 * 1. The client (MPI process) connects to us, which will result in an
 * event on the listener socket.
 *
 * 2. The client sends us a "cmd" or "mcmd" string which means that a
 * single or multi-line command is to follow.
 *
 * 3. Here are the commands that we respect:
 *     - initack [done]
 *     - init [done]
 *     - get_maxes [done]
 *     - get_appnum [done]
 *     - get_my_kvsname [done]
 *     - barrier_in [done]
 *     - put [done]
 *     - get [done]
 *     - finalize [done]
 *     - abort
 *     - create_kvs
 *     - destroy_kvs
 *     - getbyidx
 *     - spawn
 *     - get_universe_size
 */
#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCD_Central_cb"
HYD_Status HYD_PMCD_Central_cb(int fd, HYD_CSI_Event_t events)
{
    int accept_fd, linelen, i;
    char * buf, * cmd, * args[HYD_PMCU_NUM_STR];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(buf, char *, MAX_BUFFER_SIZE, status);

    if (fd == HYD_PMCD_Central_listenfd) { /* Someone is trying to connect to us */
	status = HYDU_Sock_accept(fd, &accept_fd);
	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("sock utils returned error when accepting connection\n");
	    goto fn_fail;
	}

	status = HYD_DMX_Register_fd(1, &accept_fd, HYD_CSI_OUT, HYD_PMCD_Central_cb);
	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("demux engine returned error when registering fd\n");
	    goto fn_fail;
	}
    }
    else {
	status = HYDU_Sock_readline(fd, buf, MAX_BUFFER_SIZE, &linelen);
	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("sock utils returned error when reading PMI line\n");
	    goto fn_fail;
	}

	if (linelen == 0) {
	    status = HYD_DMX_Deregister_fd(fd);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("unable to deregister fd %d\n", fd);
		goto fn_fail;
	    }
	    close(fd);
	    goto fn_exit;
	}

	/* Check what command we got and call the appropriate
	 * function */
	buf[linelen - 1] = 0;

	cmd = strtok(buf, " ");
	for (i = 0; i < HYD_PMCU_NUM_STR; i++) {
	    args[i] = strtok(NULL, " ");
	    if (args[i] == NULL)
		break;
	}

	if (cmd == NULL) {
	    status = HYD_SUCCESS;
	}
	else if (!strcmp("cmd=initack", cmd)) {
	    status = HYD_PMCU_pmi_initack(fd, args);
	}
	else if (!strcmp("cmd=init", cmd)) {
	    status = HYD_PMCU_pmi_init(fd, args);
	}
	else if (!strcmp("cmd=get_maxes", cmd)) {
	    status = HYD_PMCU_pmi_get_maxes(fd, args);
	}
	else if (!strcmp("cmd=get_appnum", cmd)) {
	    status = HYD_PMCU_pmi_get_appnum(fd, args);
	}
	else if (!strcmp("cmd=get_my_kvsname", cmd)) {
	    status = HYD_PMCU_pmi_get_my_kvsname(fd, args);
	}
	else if (!strcmp("cmd=barrier_in", cmd)) {
	    status = HYD_PMCU_pmi_barrier_in(fd, args);
	}
	else if (!strcmp("cmd=put", cmd)) {
	    status = HYD_PMCU_pmi_put(fd, args);
	}
	else if (!strcmp("cmd=get", cmd)) {
	    status = HYD_PMCU_pmi_get(fd, args);
	}
	else if (!strcmp("cmd=finalize", cmd)) {
	    status = HYD_PMCU_pmi_finalize(fd, args);

	    if (status == HYD_SUCCESS) {
		status = HYD_DMX_Deregister_fd(fd);
		if (status != HYD_SUCCESS) {
		    HYDU_Error_printf("unable to deregister fd %d\n", fd);
		    goto fn_fail;
		}
		close(fd);
	    }
	}
	else if (!strcmp("cmd=get_universe_size", cmd)) {
	    status = HYD_PMCU_pmi_get_usize(fd, args);
	}
	else {
	    /* We don't understand the command */
	    HYDU_Error_printf("Unrecognized PMI command: %s\n", cmd);
	    status = HYD_INTERNAL_ERROR;
	    goto fn_fail;
	}

	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("PMI server function returned an error\n");
	    goto fn_fail;
	}
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
