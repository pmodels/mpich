/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSCI_H_INCLUDED
#define BSCI_H_INCLUDED

/** @file bsci.h.in */

/*! \addtogroup bootstrap Bootstrap Control Interface
 * @{
 */

/**
 * \brief BSCI internal structure to maintain persistent information.
 */
struct HYDT_bsci_info {
    /** \brief RMK to use */
    const char *rmk;

    /** \brief Launcher to use */
    const char *launcher;

    /** \brief Launcher executable to use */
    const char *launcher_exec;

    /** \brief Enable/disable X-forwarding */
    int enablex;

    /** \brief Enable/disable debugging */
    int debug;
};

/**
 * \brief Function pointers for device specific implementations of
 * different BSCI functions.
 */
struct HYDT_bsci_fns {
    /* RMK functions */
    /** \brief Query if the RMK integrates natively with the RM */
    HYD_status(*query_native_int) (int *ret);

    /** \brief Query for node list information */
    HYD_status(*query_node_list) (struct HYD_node ** node_list);

    /** \brief Query for job ID information */
    HYD_status(*query_jobid) (char **jobid);

    /** \brief Finalize the RMK */
    HYD_status(*rmk_finalize) (void);


    /* Launcher functions */
    /** \brief Launch processes */
    HYD_status(*launch_procs) (char **args, struct HYD_proxy * proxy_list, int *control_fd);

    /** \brief Finalize the bootstrap control device */
    HYD_status(*launcher_finalize) (void);

    /** \brief Wait for launched processes to complete */
    HYD_status(*wait_for_completion) (int timeout);

    /** \brief Query the ID of a proxy */
    HYD_status(*query_proxy_id) (int *proxy_id);

    /** \brief Query if an environment variable should be inherited */
    HYD_status(*query_env_inherit) (const char *env_name, int *ret);
};

/** \cond */
extern struct HYDT_bsci_fns HYDT_bsci_fns;
extern struct HYDT_bsci_info HYDT_bsci_info;
/** \endcond */

/**
 * \brief HYDT_bsci_init - Initialize the bootstrap control device
 *
 * \param[in]   rmk             Resource management kernel to use
 * \param[in]   launcher        Launcher to use
 * \param[in]   launcher_exec   Launcher executable to use (optional)
 * \param[in]   enablex         Enable/disable X-forwarding (hint only)
 * \param[in]   debug           Enable/disable debugging
 *
 * This function initializes the bootstrap control device. This needs
 * to be called before any other BSCI function. Implementors are
 * expected to set any bootstrap implementation specific function
 * pointers in this function to be used by later BSCI calls.
 */
HYD_status HYDT_bsci_init(const char *rmk, const char *launcher,
                          const char *launcher_exec, int enablex, int debug);


/**
 * \brief HYDT_bsci_launch_procs - Launch processes
 *
 * \param[in]   args            Arguments to be used for the launched processes
 * \param[in]   proxy_list      List of proxies to launch
 * \param[out]  control_fd      Control socket to communicate with the launched process
 *
 * This function appends a proxy ID to the end of the args list and
 * uses this combined list as the executable and its arguments to
 * launch. Upper layers will need to account for this automatic
 * addition of the proxy ID.
 *
 * Launchers that perform sequential launches (one process at a time),
 * should set the proxy ID string in sequential order. Launchers that
 * perform parallel launches should set the proxy ID string to "-1",
 * but allow proxies to query their ID information on each node using
 * the HYDT_bsci_query_proxy_id function.
 */
HYD_status HYDT_bsci_launch_procs(char **args, struct HYD_proxy *proxy_list, int *control_fd);


/**
 * \brief HYDT_bsci_finalize - Finalize the bootstrap control device
 *
 * This function cleans up any relevant state that the bootstrap
 * device maintained.
 */
HYD_status HYDT_bsci_finalize(void);


/**
 * \brief HYDT_bsci_wait_for_completion - Wait for launched processes to complete
 *
 * \param[in]  timeout        Time to wait for
 *
 * \param[ret] status         HYD_TIMED_OUT if the timer expired
 *
 * This function waits for all processes it launched to finish. The
 * launcher should keep track of the processes it is launching and
 * wait for their completion.
 */
HYD_status HYDT_bsci_wait_for_completion(int timeout);


/**
 * \brief HYDT_bsci_query_node_list - Query for node list information
 *
 * \param[out] node_list       Lists of nodes available
 *
 * This function allows the upper layers to query the available
 * nodes.
 */
HYD_status HYDT_bsci_query_node_list(struct HYD_node **node_list);


/**
 * \brief HYDT_bsci_query_jobid - Query for Job ID information
 *
 * \param[out] jobid       Job ID
 *
 * This function allows the upper layers to query the job ID.
 */
HYD_status HYDT_bsci_query_jobid(char **jobid);


/**
 * \brief HYDT_bsci_query_proxy_id - Query the ID of a proxy
 *
 * \param[out]  proxy_id    My proxy ID
 *
 * This function is called by each proxy if the proxy_str_id is
 * specified as "-1" during launch.
 */
HYD_status HYDT_bsci_query_proxy_id(int *proxy_id);

/**
 * \brief HYDT_bsci_query_env_inherit - Query if an environment
 * variable is safe to be inherited
 *
 * \param[in]  env_name    Name of the environment variable
 * \param[out] ret         Boolean for true (1) or false (0)
 *
 * This function is used to check if an environment variable inherited
 * from the user's environment is safe to be propagated to the remote
 * processes.
 */
HYD_status HYDT_bsci_query_env_inherit(const char *env_name, int *ret);

/**
 * \brief HYDT_bsci_query_native_int - Query if the RMK integrates
 * natively with the RM
 *
 * \param[out] ret                    Boolean for true (1) or false (0)
 *
 * This function is used to check if an environment variable inherited
 * from the user's environment is safe to be propagated to the remote
 * processes.
 */
HYD_status HYDT_bsci_query_native_int(int *ret);

/*! @} */

/* Each launcher has to expose an initialization function */
#if defined HAVE_BSS_EXTERNAL
HYD_status HYDT_bsci_launcher_ssh_init(void);
HYD_status HYDT_bsci_launcher_rsh_init(void);
HYD_status HYDT_bsci_launcher_fork_init(void);
HYD_status HYDT_bsci_launcher_slurm_init(void);
HYD_status HYDT_bsci_launcher_ll_init(void);
HYD_status HYDT_bsci_launcher_lsf_init(void);
HYD_status HYDT_bsci_launcher_sge_init(void);
#if defined HAVE_TM_H
HYD_status HYDT_bsci_launcher_pbs_init(void);
#endif /* HAVE_TM_H */
HYD_status HYDT_bsci_launcher_manual_init(void);

HYD_status HYDT_bsci_rmk_slurm_init(void);
HYD_status HYDT_bsci_rmk_ll_init(void);
HYD_status HYDT_bsci_rmk_lsf_init(void);
HYD_status HYDT_bsci_rmk_sge_init(void);
HYD_status HYDT_bsci_rmk_pbs_init(void);
HYD_status HYDT_bsci_rmk_user_init(void);
#endif /* HAVE_BSS_EXTERNAL */

#if defined HAVE_BSS_PERSIST
HYD_status HYDT_bsci_launcher_persist_init(void);
#endif /* HAVE_BSS_PERSIST */

#endif /* BSCI_H_INCLUDED */
