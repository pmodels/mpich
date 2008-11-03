/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "csi.h"
#include "bsci.h"
#include "bscu.h"

HYD_CSI_Handle csi_handle;

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Create_process"
HYD_Status HYD_BSCU_Create_process(char **client_arg, int *in, int *out, int *err, int *pid)
{
    int inpipe[2], outpipe[2], errpipe[2], arg, tpid;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (in != NULL) {
	if (pipe(inpipe) < 0) {
	    HYDU_Error_printf("pipe error (errno: %d)\n", errno);
	    status = HYD_SOCK_ERROR;
	    goto fn_fail;
	}
    }

    if (pipe(outpipe) < 0) {
	HYDU_Error_printf("pipe error (errno: %d)\n", errno);
	status = HYD_SOCK_ERROR;
	goto fn_fail;
    }

    if (pipe(errpipe) < 0) {
	HYDU_Error_printf("pipe error (errno: %d)\n", errno);
	status = HYD_SOCK_ERROR;
	goto fn_fail;
    }

    /* Fork off the process */
    tpid = fork();
    if (tpid == 0) {	/* Child process */
	close(outpipe[0]);
	close(1);
	if (dup2(outpipe[1], 1) < 0) {
	    HYDU_Error_printf("dup2 error (errno: %d)\n", errno);
	    status = HYD_SOCK_ERROR;
	    goto fn_fail;
	}

	close(errpipe[0]);
	close(2);
	if (dup2(errpipe[1], 2) < 0) {
	    HYDU_Error_printf("dup2 error (errno: %d)\n", errno);
	    status = HYD_SOCK_ERROR;
	    goto fn_fail;
	}

	if (in != NULL) {
	    close(inpipe[1]);
	    close(0);
	    if (dup2(inpipe[0], 0) < 0) {
		HYDU_Error_printf("dup2 error (errno: %d)\n", errno);
		status = HYD_SOCK_ERROR;
		goto fn_fail;
	    }
	}

	if (chdir(csi_handle.wdir) < 0) {
	    if (chdir(getenv("HOME")) < 0) {
		HYDU_Error_printf("unable to set working directory to %s\n", csi_handle.wdir);
		status = HYD_INTERNAL_ERROR;
		goto fn_fail;
	    }
	}

	/* execvp the process */
	if (execvp(client_arg[0], client_arg) < 0) {
	    HYDU_Error_printf("execvp error\n");
	    status = HYD_INTERNAL_ERROR;
	    goto fn_fail;
	}
    }
    else {	/* Parent process */
	close(outpipe[1]);
	close(errpipe[1]);
	if (in)
	    *in = inpipe[1];
	if (out)
	    *out = outpipe[0];
	if (err)
	    *err = errpipe[0];
    }

    if (pid)
	*pid = tpid;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
