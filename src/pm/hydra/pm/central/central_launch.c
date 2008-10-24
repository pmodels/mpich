/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_sock.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "csi.h"
#include "pmci.h"
#include "bsci.h"
#include "demux.h"
#include "central.h"

int HYD_PMCD_Central_listenfd;
HYD_CSI_Handle * csi_handle;

#define create_and_add_static_env(str, val, status)	\
    { \
	HYD_CSI_Env_t * env; \
	\
	HYDU_MALLOC(env, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status); \
	env->env_name = MPIU_Strdup(str);				\
	HYDU_MALLOC(env->env_value, char *, strlen(val), status); \
	strcpy(env->env_value, val);				  \
	env->env_type = HYD_CSI_ENV_STATIC; \
	env->next = NULL; \
	proc_params = csi_handle->proc_params; \
	while (proc_params) { \
	    status = HYD_LCHU_Add_env_to_list(&proc_params->env_list, env); \
	    if (status != HYD_SUCCESS) { \
		HYDU_Error_printf("launcher utils returned error adding env to list\n"); \
		goto fn_fail; \
	    } \
	    proc_params = proc_params->next; \
	} \
    }

#define create_and_add_dynamic_env(str, val, status)	\
    { \
	HYD_CSI_Env_t * env; \
	\
	HYDU_MALLOC(env, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status); \
	env->env_name = MPIU_Strdup(str);				\
	env->env_value = NULL; \
	env->env_type = HYD_CSI_ENV_AUTOINC; \
	env->next = NULL; \
	proc_params = csi_handle->proc_params; \
	while (proc_params) { \
	    status = HYD_LCHU_Add_env_to_list(&proc_params->env_list, env);	\
	    if (status != HYD_SUCCESS) { \
		HYDU_Error_printf("launcher utils returned error adding env to list\n"); \
		goto fn_fail; \
	    } \
	    proc_params = proc_params->next; \
	} \
    }


/*
 * HYD_PMCI_Launch_procs: Here are the steps we follow:
 *
 * 1. Find what all ports the user wants to allow and listen on one of
 * those ports.
 *
 * 2. Create a call-back function to accept connections and register
 * the listening socket with the demux engine.
 *
 * 3. Create a port string out of this hostname and port and add it to
 * the environment list under the variable "PMI_PORT".
 *
 * 4. Create an environment variable for PMI_ID. This is an
 * auto-incrementing variable; the bootstrap server will take care of
 * adding the process ID to the start value.
 *
 * 5. Ask the bootstrap server to launch the processes.
 */
#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_PMCI_Launch_procs"
HYD_Status HYD_PMCI_Launch_procs()
{
    char * port_range, * port_str, * sport;
    int low_port, high_port, port, one = 1, i;
    int num_procs;
    char hostname[MAX_HOSTNAME_LEN];
    struct sockaddr_in sa;
    struct HYD_CSI_Proc_params * proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Check if the user wants us to use a port within a certain
     * range. */
    port_range = getenv("MPIEXEC_PORTRANGE");
    if (!port_range)
	port_range = getenv("MPIEXEC_PORT_RANGE");
    if (!port_range)
	port_range = getenv("MPICH_PORT_RANGE");

    low_port = 0; high_port = 0;
    if (port_range) {
	port_str = strtok(port_range, ":");
	if (port_str == NULL) {
	    HYDU_Error_printf("error parsing port range string\n");
	    status = HYD_INTERNAL_ERROR;
	    goto fn_fail;
	}
	low_port = atoi(port_str);

	port_str = strtok(NULL, ":");
	if (port_str == NULL) {
	    HYDU_Error_printf("error parsing port range string\n");
	    status = HYD_INTERNAL_ERROR;
	    goto fn_fail;
	}
	high_port = atoi(port_str);

	if (high_port < low_port) {
	    HYDU_Error_printf("high port is smaller than low port\n");
	    status = HYD_INTERNAL_ERROR;
	    goto fn_fail;
	}
    }

    /* Listen on a port in the port range */
    status = HYDU_Sock_listen(&HYD_PMCD_Central_listenfd, low_port, high_port, &port);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("sock utils returned listen error\n");
	goto fn_fail;
    }

    /* Register the listening socket with the demux engine */
    /* FIXME: The callback function needs to be written. Without it,
     * MPI applications will not work. Regular non-MPI applications
     * will still work though. */
    status = HYD_DMX_Register_fd(1, &HYD_PMCD_Central_listenfd,
				 HYD_CSI_OUT, HYD_PMCD_Central_cb);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("demux engine returned error for registering fd\n");
	goto fn_fail;
    }

    /* Create a port string for MPI processes to use to connect to */
    if (gethostname(hostname, MAX_HOSTNAME_LEN) < 0) {
	HYDU_Error_printf("gethostname error (hostname: %s; errno: %d)\n", hostname, errno);
	status = HYD_SOCK_ERROR;
	goto fn_fail;
    }

    HYDU_Int_to_str(port, sport, status);
    HYDU_MALLOC(port_str, char *, strlen(hostname) + 1 + strlen(sport) + 1, status);
    MPIU_Snprintf(port_str, strlen(hostname) + 1 + strlen(sport) + 1, "%s:%s", hostname, sport);

    create_and_add_static_env("PMI_PORT", port_str, status);
    create_and_add_dynamic_env("PMI_ID", 0, status);

    /* Create a process group for the MPI processes in this
     * comm_world */
    status = HYD_PMCU_Create_pg();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("pm utils returned error creating process group\n");
	goto fn_fail;
    }

    /* Initialize the bootstrap server and ask it to launch the
     * processes. */
    status = HYD_BSCI_Launch_procs();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("bootstrap server returned error launching processes\n");
	goto fn_fail;
    }

fn_fail:
/*     HYDU_FREE(port_str); */
    HYDU_FUNC_EXIT();
    return status;

fn_exit:
    goto fn_fail;
}
