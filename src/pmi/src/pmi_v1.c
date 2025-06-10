/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*********************** PMI implementation ********************************/
/*
 * This file implements the client-side of the PMI interface.
 *
 * Note that the PMI client code must not print error messages (except
 * when an abort is required) because MPI error handling is based on
 * reporting error codes to which messages are attached.
 *
 * In v2, we should require a PMI client interface to use MPI error codes
 * to provide better integration with MPICH.
 */
/***************************************************************************/

#ifndef DISABLE_PMI1

#include "pmi_config.h"
#include "mpl.h"
#include "uthash.h"

#include "pmi_util.h"
#include "pmi.h"
#include "pmi_wire.h"
#include "pmi_msg.h"
#include "pmi_common.h"

#include <sys/socket.h>

#define USE_WIRE_VER  PMIU_WIRE_V1
static const bool no_static = false;

static int PMI_server_version = 0;
static int PMI_server_subversion = 0;

/* ALL GLOBAL VARIABLES MUST BE INITIALIZED TO AVOID POLLUTING THE
   LIBRARY WITH COMMON SYMBOLS */
static int PMI_kvsname_max = 0;
static int PMI_keylen_max = 0;
static int PMI_vallen_max = 0;

static int PMI_spawned = 0;

/* Function prototypes for internal routines */
static int PMII_init(int *server_version_p, int *server_subversion_p);
static int PMII_getmaxes(int *kvsname_max, int *keylen_max, int *vallen_max);
static int PMII_Set_from_port(int id);

#ifdef USE_PMI_PORT
static int PMII_singinit(void);
static int PMI_totalview = 0;
#endif
static int PMIi_InitIfSingleton(void);
static int accept_one_connection(int);

/* We allow a single PUT with SINGLETON_INIT_BUT_NO_PM */
static int cached_singinit_inuse = 0;
static char cached_singinit_key[PMIU_MAXLINE];
static char cached_singinit_val[PMIU_MAXLINE];

#define MAX_SINGINIT_KVSNAME 256
static char singinit_kvsname[MAX_SINGINIT_KVSNAME];

static int expect_pmi_cmd(const char *cmd, const char *key, int *val_out);

PMI_API_PUBLIC int PMI_Init(int *spawned)
{
    int pmi_errno = PMI_SUCCESS;
    char *p;
    int rc;

    PMI_initialized = PMI_UNINITIALIZED;

    /* FIXME: Why is setvbuf commented out? */
    /* FIXME: What if the output should be fully buffered (directed to file)?
     * unbuffered (user explicitly set?) */
    /* setvbuf(stdout,0,_IONBF,0); */
    setbuf(stdout, NULL);
    /* PMIU_printf(1, "PMI_INIT\n"); */

    /* Get the value of PMI_DEBUG from the environment if possible, since
     * we may have set it to help debug the setup process */
    p = getenv("PMI_DEBUG");
    if (p) {
        PMIU_verbose = atoi(p);
    }

    /* enable PMIU_CS_{ENTER/EXIT} */
    PMIU_is_threaded = 1;
    PMIU_thread_init();

    /* Get the fd for PMI commands; if none, we're a singleton */
    bool do_handshake;
    rc = PMIU_get_pmi_fd(&PMI_fd, &do_handshake);
    if (rc) {
        return rc;
    }

    if (PMI_fd == -1) {
        /* Check if we are in a PMIx environment to throw an error */
        if (PMIU_detect_pmix_launcher()) {
            PMIU_printf(1, "launcher not compatible with PMI1 client\n");
            return PMI_FAIL;
        }

        /* Singleton init: Process not started with mpiexec,
         * so set size to 1, rank to 0 */
        PMI_size = 1;
        PMI_rank = 0;
        *spawned = 0;

        PMI_initialized = SINGLETON_INIT_BUT_NO_PM;
        /* 256 is picked as the minimum allowed length by the PMI servers */
        PMI_kvsname_max = 256;
        PMI_keylen_max = 256;
        PMI_vallen_max = 256;

        return PMI_SUCCESS;
    }

    if (do_handshake) {
        p = getenv("PMI_ID");
        if (p) {
            int id = atoi(p);
            /* PMII_Set_from_port sets up the values that are delivered
             * by environment variables when a separate port is not used */
            pmi_errno = PMII_Set_from_port(id);
            if (pmi_errno) {
                PMIU_printf(1, "PMI_PORT initialization failed\n");
                return pmi_errno;
            }
        }
    } else {
        /* If size, rank, and debug are not set from a communication port,
         * use the environment */
        if ((p = getenv("PMI_SIZE")))
            PMI_size = atoi(p);
        else
            PMI_size = 1;

        if ((p = getenv("PMI_RANK"))) {
            PMI_rank = atoi(p);
            /* Let the util routine know the rank of this process for
             * any messages (usually debugging or error) */
            PMIU_Set_rank(PMI_rank);
        } else
            PMI_rank = 0;

        if ((p = getenv("PMI_DEBUG"))) {
            PMIU_verbose = atoi(p);
        }

        /* Leave unchanged otherwise, which indicates that no value
         * was set */
    }

/* FIXME: Why does this depend on their being a port??? */
/* FIXME: What is this for? */
#ifdef USE_PMI_PORT
    if ((p = getenv("PMI_TOTALVIEW")))
        PMI_totalview = atoi(p);
    if (PMI_totalview) {
        pmi_errno = expect_pmi_cmd("tv_ready", NULL, NULL);
        PMIU_ERR_POP(pmi_errno);
    }
#endif

    pmi_errno = PMII_init(&PMI_server_version, &PMI_server_subversion);
    PMIU_ERR_POP(pmi_errno);

    pmi_errno = PMII_getmaxes(&PMI_kvsname_max, &PMI_keylen_max, &PMI_vallen_max);
    PMIU_ERR_POP(pmi_errno);

    /* FIXME: This is something that the PM should tell the process,
     * rather than deliver it through the environment */
    if ((p = getenv("PMI_SPAWNED")))
        PMI_spawned = atoi(p);
    else
        PMI_spawned = 0;
    if (PMI_spawned)
        *spawned = 1;
    else
        *spawned = 0;

    if (!PMI_initialized)
        PMI_initialized = NORMAL_INIT_WITH_PM;

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI_Initialized(int *initialized)
{
    /* Turn this into a logical value (1 or 0) .  This allows us
     * to use PMI_initialized to distinguish between initialized with
     * an PMI service (e.g., via mpiexec) and the singleton init,
     * which has no PMI service */
    *initialized = (PMI_initialized != 0);
    return PMI_SUCCESS;
}

PMI_API_PUBLIC int PMI_Get_size(int *size)
{
    if (PMI_initialized)
        *size = PMI_size;
    else
        *size = 1;
    return PMI_SUCCESS;
}

PMI_API_PUBLIC int PMI_Get_rank(int *rank)
{
    if (PMI_initialized)
        *rank = PMI_rank;
    else
        *rank = 0;
    return PMI_SUCCESS;
}

/*
 * Get_universe_size is one of the routines that needs to communicate
 * with the process manager.  If we started as a singleton init, then
 * we first need to connect to the process manager and acquire the
 * needed information.
 */
PMI_API_PUBLIC int PMI_Get_universe_size(int *size)
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    /* Connect to the PM if we haven't already */
    if (PMIi_InitIfSingleton() != 0)
        return PMI_FAIL;

    if (PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        PMIU_msg_set_query(&pmicmd, USE_WIRE_VER, PMIU_CMD_UNIVERSE, no_static);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);

        pmi_errno = PMIU_msg_get_response_universe(&pmicmd, size);
        PMIU_ERR_POP(pmi_errno);
    } else {
        /* FIXME: do we require PM or not? */
        *size = 1;
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI_Get_appnum(int *appnum)
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        PMIU_msg_set_query(&pmicmd, USE_WIRE_VER, PMIU_CMD_APPNUM, no_static);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);

        pmi_errno = PMIU_msg_get_response_appnum(&pmicmd, appnum);
        PMIU_ERR_POP(pmi_errno);
    } else {
        *appnum = -1;
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI_Barrier(void)
{
    return PMI_Barrier_group(PMI_GROUP_WORLD, 0, NULL);
}

PMI_API_PUBLIC int PMI_Barrier_group(const int *group, int count, const char *tag)
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized == PMI_UNINITIALIZED) {
        pmi_errno = PMI_ERR_INIT;
        goto fn_fail;
    } else if (PMI_initialized == SINGLETON_INIT_BUT_NO_PM) {
        /* NOOP for singleton barrier */
        goto fn_exit;
    } else if (PMI_server_version != 1 || PMI_server_subversion < 2) {
        /* server doesn't support PMI 1.2 */
        /* NOTE: there isn't a PMI api for checking server versions. As a workaround,
         *       user can use the return from PMI_Barrier_group(PMI_GROUP_SELF, 0)
         *       to check whether it is supported.
         */
        pmi_errno = PMI_FAIL;
        goto fn_fail;
    } else if (group == PMI_GROUP_SELF) {
        /* NOOP for self barrier */
        goto fn_exit;
    }

    int inttag = 0;
    if (tag) {
        HASH_FNV(tag, strlen(tag), inttag);
        /* make sure it is positive in the wire protocol for robustness */
        inttag = abs(inttag);
    }

    char *group_str;
    if (group == PMI_GROUP_WORLD) {
        group_str = MPL_malloc(20, MPL_MEM_OTHER);
        snprintf(group_str, 20, "WORLD:%d", inttag);
    } else if (group == PMI_GROUP_NODE) {
        group_str = MPL_malloc(20, MPL_MEM_OTHER);
        snprintf(group_str, 20, "NODE:%d", inttag);
    } else {
        /* convert the int array into a comma-separated int list */
        int size = count * 8;
        group_str = MPL_malloc(size, MPL_MEM_OTHER);
        char *s = group_str;
        for (int i = 0; i < count; i++) {
            int n = snprintf(s, 8, "%d,", group[i]);
            s += n;
            size -= n;
        }
        /* overwrite the last comma */
        s--;
        size++;
        snprintf(s, size, ":%d", inttag);
    }

    PMIU_msg_set_query_barrier(&pmicmd, USE_WIRE_VER, no_static, group_str);

    if (!PMIU_is_threaded) {
        PMIU_CS_ENTER;
        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_CS_EXIT;
        PMIU_ERR_POP(pmi_errno);
    } else {
        PMIU_CS_ENTER;
        pmi_errno = PMIU_cmd_send(PMI_fd, &pmicmd);
        PMIU_CS_EXIT;
        PMIU_ERR_POP(pmi_errno);

        while (true) {
            bool flag;
            PMIU_CS_ENTER;
            pmi_errno = PMIU_cmd_test_barrier(PMI_fd, inttag, &flag);
            PMIU_CS_EXIT;
            PMIU_ERR_POP(pmi_errno);
            if (flag) {
                break;
            }
            MPL_thread_yield();
        }
    }

    MPL_free(group_str);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/* Inform the process manager that we're in finalize */
PMI_API_PUBLIC int PMI_Finalize(void)
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        PMIU_msg_set_query(&pmicmd, USE_WIRE_VER, PMIU_CMD_FINALIZE, no_static);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);

        if (0) {
            /* closing PMI_fd prevents re-init. Disable for now. */
            shutdown(PMI_fd, SHUT_RDWR);
            close(PMI_fd);
        }
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI_Abort(int exit_code, const char error_msg[])
{
    int pmi_errno = PMI_SUCCESS;

    PMIU_printf(PMIU_verbose, "aborting job:\n%s\n", error_msg);

    struct PMIU_cmd pmicmd;
    PMIU_msg_set_query_abort(&pmicmd, USE_WIRE_VER, no_static, exit_code, error_msg);

    pmi_errno = PMIU_cmd_send(PMI_fd, &pmicmd);

    return pmi_errno;
}

/************************************* Keymap functions **********************/

/*FIXME: need to return an error if the value of the kvs name returned is
  truncated because it is larger than length */
/* FIXME: My name should be cached rather than re-acquired, as it is
   unchanging (after singleton init) */
PMI_API_PUBLIC int PMI_KVS_Get_my_name(char kvsname[], int length)
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized == SINGLETON_INIT_BUT_NO_PM) {
        /* Return a dummy name */
        /* Upon singinit of server, we'll check and replace "singinit" with
         * initialized singinit_kvsname */
        snprintf(kvsname, length, "singinit");
        goto fn_exit;
    }

    PMIU_msg_set_query(&pmicmd, USE_WIRE_VER, PMIU_CMD_KVSNAME, no_static);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    const char *tmp_kvsname;
    pmi_errno = PMIU_msg_get_response_kvsname(&pmicmd, &tmp_kvsname);

    MPL_strncpy(kvsname, tmp_kvsname, length);
    PMIU_Set_rank_kvsname(PMI_rank, tmp_kvsname);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI_KVS_Get_name_length_max(int *maxlen)
{
    if (maxlen == NULL)
        return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_kvsname_max;
    return PMI_SUCCESS;
}

PMI_API_PUBLIC int PMI_KVS_Get_key_length_max(int *maxlen)
{
    if (maxlen == NULL)
        return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_keylen_max;
    return PMI_SUCCESS;
}

PMI_API_PUBLIC int PMI_KVS_Get_value_length_max(int *maxlen)
{
    if (maxlen == NULL)
        return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_vallen_max;
    return PMI_SUCCESS;
}

PMI_API_PUBLIC int PMI_KVS_Put(const char kvsname[], const char key[], const char value[])
{
    int pmi_errno = PMI_SUCCESS;
    const char *use_kvsname = kvsname;

    /* This is a special hack to support singleton initialization */
    if (PMI_initialized == SINGLETON_INIT_BUT_NO_PM) {
        int rc;
        if (cached_singinit_inuse)
            return PMI_FAIL;
        rc = MPL_strncpy(cached_singinit_key, key, PMI_keylen_max);
        if (rc != 0)
            return PMI_FAIL;
        rc = MPL_strncpy(cached_singinit_val, value, PMI_vallen_max);
        if (rc != 0)
            return PMI_FAIL;
        cached_singinit_inuse = 1;
        return PMI_SUCCESS;
    }

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    PMIU_CS_ENTER;

    if (strcmp(kvsname, "singinit") == 0) {
        use_kvsname = singinit_kvsname;
    }

    PMIU_msg_set_query_put(&pmicmd, USE_WIRE_VER, no_static, use_kvsname, key, value);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    PMIU_CS_EXIT;
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI_KVS_Commit(const char kvsname[]ATTRIBUTE((unused)))
{
    /* no-op in this implementation */
    return PMI_SUCCESS;
}

/*FIXME: need to return an error if the value returned is truncated
  because it is larger than length */
PMI_API_PUBLIC int PMI_KVS_Get(const char kvsname[], const char key[], char value[], int length)
{
    int pmi_errno = PMI_SUCCESS;
    const char *use_kvsname = kvsname;

    /* singleton can skip PMI builtin keys */
    if (PMI_initialized == SINGLETON_INIT_BUT_NO_PM && strncmp(key, "PMI_", 4) == 0) {
        return PMI_FAIL;
    }

    /* Connect to the PM if we haven't already.  This is needed in case
     * we're doing an MPI_Comm_join or MPI_Comm_connect/accept from
     * the singleton init case.  This test is here because, in the way in
     * which MPICH uses PMI, this is where the test needs to be. */
    if (PMIi_InitIfSingleton() != 0)
        return PMI_FAIL;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    PMIU_CS_ENTER;

    if (strcmp(kvsname, "singinit") == 0) {
        use_kvsname = singinit_kvsname;
    }

    PMIU_msg_set_query_get(&pmicmd, USE_WIRE_VER, no_static, use_kvsname, key);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    const char *tmp_val;
    bool found;
    pmi_errno = PMIU_msg_get_response_get(&pmicmd, &found, &tmp_val);
    PMIU_ERR_POP(pmi_errno);

    MPL_strncpy(value, tmp_val, length);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    PMIU_CS_EXIT;
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/*************************** Name Publishing functions **********************/

PMI_API_PUBLIC int PMI_Publish_name(const char service_name[], const char port[])
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        PMIU_msg_set_query_publish(&pmicmd, USE_WIRE_VER, no_static, service_name, port);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);
    } else {
        PMIU_ERR_SETANDJUMP(pmi_errno, PMI_FAIL, "PMI_Publish_name called before init\n");
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI_Unpublish_name(const char service_name[])
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        PMIU_msg_set_query_unpublish(&pmicmd, USE_WIRE_VER, no_static, service_name);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);
    } else {
        PMIU_ERR_SETANDJUMP(pmi_errno, PMI_FAIL, "PMI_Unpublish_name called before init\n");
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI_Lookup_name(const char service_name[], char port[])
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        PMIU_msg_set_query_lookup(&pmicmd, USE_WIRE_VER, no_static, service_name);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);

        const char *tmp_port;
        PMIU_msg_get_response_lookup(&pmicmd, &tmp_port);

        MPL_strncpy(port, tmp_port, PMI_MAX_PORT_NAME);

    } else {
        PMIU_ERR_SETANDJUMP(pmi_errno, PMI_FAIL, "PMI_Lookup_name called before init\n");
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}


/************************** Process Creation functions **********************/

PMI_API_PUBLIC
    int PMI_Spawn_multiple(int count,
                           const char *cmds[],
                           const char **argvs[],
                           const int maxprocs[],
                           const int info_keyval_sizes[],
                           const PMI_keyval_t * info_keyval_vectors[],
                           int preput_keyval_size,
                           const PMI_keyval_t preput_keyval_vector[], int errors[])
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    /* Connect to the PM if we haven't already */
    if (PMIi_InitIfSingleton() != 0)
        return PMI_FAIL;

    /* split argvs */
    int *argcs = NULL;
    argcs = MPL_malloc(count * sizeof(int), MPL_MEM_OTHER);
    PMIU_Assert(argcs);

    for (int i = 0; i < count; i++) {
        int j = 0;
        if (argvs && argvs[i]) {
            while (argvs[i][j]) {
                j++;
            }
        }
        argcs[i] = j;
    }

    PMIU_msg_set_query_spawn(&pmicmd, PMIU_WIRE_V1, no_static,
                             count, cmds, maxprocs, argcs, argvs,
                             info_keyval_sizes, (const struct PMIU_token **) info_keyval_vectors,
                             preput_keyval_size, (const struct PMIU_token *) preput_keyval_vector);
    MPL_free(argcs);


    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);
    PMIU_CMD_EXPECT_INTVAL(&pmicmd, "rc", 0);

    int total_num_processes;
    total_num_processes = 0;
    for (int spawncnt = 0; spawncnt < count; spawncnt++) {
        total_num_processes += maxprocs[spawncnt];
    }

    PMIU_Assert(errors != NULL);
    const char *errcodes_str;
    errcodes_str = PMIU_cmd_find_keyval(&pmicmd, "errcodes");
    if (errcodes_str) {
        int num_errcodes_found = 0;
        const char *lag = errcodes_str;
        const char *lead;
        do {
            lead = strchr(lag, ',');
            /* NOTE: atoi converts the initial portion of the string, thus we don't need
             *       terminate the string. We can't anyway since errcodes_str is const char *.
             */
            errors[num_errcodes_found++] = atoi(lag);
            lag = lead + 1;     /* move past the null char */
            PMIU_Assert(num_errcodes_found <= total_num_processes);
        } while (lead != NULL);
        PMIU_Assert(num_errcodes_found == total_num_processes);
    } else {
        /* gforker doesn't return errcodes, so we'll just pretend that means
         * that it was going to send all `0's. */
        for (int i = 0; i < total_num_processes; ++i) {
            errors[i] = 0;
        }
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/***************** Internal routines not part of PMI interface ***************/

static int PMII_init(int *server_version_p, int *server_subversion_p)
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    /* use version 1.1 for backward compatibility. Server should reply higher subversion
     * if it supports extensions. Client should check server versions for graceful fallback
     * on 1.2 and later features.
     */
    PMIU_msg_set_query_init(&pmicmd, USE_WIRE_VER, no_static, 1, 1);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    pmi_errno = PMIU_msg_get_response_init(&pmicmd, server_version_p, server_subversion_p);
    PMIU_ERR_POP(pmi_errno);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    /* FIXME: is abort the right behavior? */
    PMI_Abort(-1, "PMI_Init failed");
    goto fn_exit;
}

/* to get all maxes in one message */
static int PMII_getmaxes(int *kvsname_max, int *keylen_max, int *vallen_max)
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_msg_set_query(&pmicmd, USE_WIRE_VER, PMIU_CMD_MAXES, no_static);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    pmi_errno = PMIU_msg_get_response_maxes(&pmicmd, kvsname_max, keylen_max, vallen_max);
    PMIU_ERR_POP(pmi_errno);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/* ----------------------------------------------------------------------- */


#ifndef USE_PMI_PORT
static int PMIi_InitIfSingleton(void)
{
    return PMI_FAIL;
}

#else
static int PMII_Set_from_port(int id)
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_msg_set_query_fullinit(&pmicmd, USE_WIRE_VER, no_static, id);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    /* weird 3 additional set commands */
    pmi_errno = expect_pmi_cmd("set", "size", &PMI_size);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = expect_pmi_cmd("set", "rank", &PMI_rank);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = expect_pmi_cmd("set", "debug", &PMIU_verbose);
    PMIU_ERR_POP(pmi_errno);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/* ------------------------------------------------------------------------- */
/*
 * Singleton Init.
 *
 * MPI-2 allows processes to become MPI processes and then make MPI calls,
 * such as MPI_Comm_spawn, that require a process manager (this is different
 * than the much simpler case of allowing MPI programs to run with an
 * MPI_COMM_WORLD of size 1 without an mpiexec or process manager).
 *
 * The process starts when either the client or the process manager contacts
 * the other.  If the client starts, it sends a singinit command and
 * waits for the server to respond with its own singinit command.
 * If the server start, it send a singinit command and waits for the
 * client to respond with its own singinit command
 *
 * client sends singinit with these required values
 *   pmi_version=<value of PMI_VERSION>
 *   pmi_subversion=<value of PMI_SUBVERSION>
 *
 * and these optional values
 *   stdio=[yes|no]
 *   authtype=[none|shared|<other-to-be-defined>]
 *   authstring=<string>
 *
 * server sends singinit with the same required and optional values as
 * above.
 *
 * At this point, the protocol is now the same in both cases, and has the
 * following components:
 *
 * server sends singinit_info with these required fields
 *   versionok=[yes|no]
 *   stdio=[yes|no]
 *   kvsname=<string>
 *
 * The client then issues the init command (see PMII_getmaxes)
 *
 * cmd=init pmi_version=<val> pmi_subversion=<val>
 *
 * and expects to receive a
 *
 * cmd=response_to_init rc=0 pmi_version=<val> pmi_subversion=<val>
 *
 * (This is the usual init sequence).
 *
 */
/* ------------------------------------------------------------------------- */
/* This is a special routine used to re-initialize PMI when it is in
   the singleton init case.  That is, the executable was started without
   mpiexec, and PMI_Init returned as if there was only one process.

   Note that PMI routines should not call PMII_singinit; they should
   call PMIi_InitIfSingleton(), which both connects to the process manager
   and sets up the initial KVS connection entry.
*/

static int PMII_singinit(void)
{
    int pmi_errno = PMI_SUCCESS;

    int singinit_listen_sock;
    char port_c[8];
    unsigned short port;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    /* Create a socket on which to allow an mpiexec to connect back to
     * us */
    singinit_listen_sock = MPL_socket();
    PMIU_ERR_CHKANDJUMP(singinit_listen_sock == -1, pmi_errno, PMI_FAIL,
                        "PMII_singinit: socket creation failed");

    MPL_LISTEN_PUSH(0, 5);      /* use_loopback=0, max_conn=5 */
    int rc;
    rc = MPL_listen_anyport(singinit_listen_sock, &port);
    MPL_LISTEN_POP;     /* back to default: use_loopback=0, max_conn=SOMAXCONN */
    PMIU_ERR_CHKANDJUMP(rc, pmi_errno, PMI_FAIL, "PMII_singinit: listen failed");

    snprintf(port_c, sizeof(port_c), "%d", port);

    PMIU_printf(PMIU_verbose, "Starting mpiexec with %s\n", port_c);

    /* Launch the mpiexec process with the name of this port */
    int pid;
    pid = fork();
    PMIU_ERR_CHKANDJUMP(pid < 0, pmi_errno, PMI_FAIL, "PMII_singinit: fork failed");

    if (pid == 0) {
        const char *newargv[8];
        int i = 0;
        newargv[i++] = "mpiexec";
        if (PMIU_verbose) {
            newargv[i++] = "-v";
        }
        newargv[i++] = "-pmi_args";
        newargv[i++] = port_c;
        /* FIXME: Use a valid hostname */
        newargv[i++] = "default_interface";     /* default interface name, for now */
        newargv[i++] = "default_key";   /* default authentication key, for now */
        char charpid[8];
        snprintf(charpid, 8, "%d", getpid());
        newargv[i++] = charpid;
        newargv[i++] = NULL;
        PMIU_Assert(i <= 8);
        rc = execvp(newargv[0], (char **) newargv);

        /* never should return unless failed */
        perror("PMII_singinit: execv failed");
        PMIU_printf(1, "  This singleton init program attempted to access some feature\n");
        PMIU_printf(1,
                    "  for which process manager support was required, e.g. spawn or universe_size.\n");
        PMIU_printf(1, "  But the necessary mpiexec is not in your path.\n");
        return PMI_FAIL;
    } else {
        int connectStdio = 0;

        /* Allow one connection back from the created mpiexec program */
        PMI_fd = accept_one_connection(singinit_listen_sock);
        PMIU_ERR_CHKANDJUMP(PMI_fd < 0, pmi_errno, PMI_FAIL,
                            "Failed to establish singleton init connection\n");

        /* Execute the singleton init protocol */
        PMIU_cmd_read(PMI_fd, &pmicmd);
        PMIU_ERR_CHKANDJUMP1(strcmp(pmicmd.cmd, "singinit") != 0,
                             pmi_errno, PMI_FAIL, "unexpected command from PM: %s\n", pmicmd.cmd);

        PMIU_CMD_EXPECT_STRVAL(&pmicmd, "authtype", "none");
        PMIU_cmd_free_buf(&pmicmd);

        /* If we're successful, send back our own singinit */
        PMIU_msg_set_query_singinit(&pmicmd, USE_WIRE_VER, no_static,
                                    PMI_VERSION, PMI_SUBVERSION, "yes", "none");

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);

        PMIU_CMD_EXPECT_STRVAL(&pmicmd, "versionok", "yes");

        const char *p;
        PMIU_CMD_GET_STRVAL(&pmicmd, "stdio", p);
        if (p && strcmp(p, "yes") == 0) {
            PMIU_printf(PMIU_verbose, "PM agreed to connect stdio\n");
            connectStdio = 1;
        }

        PMIU_CMD_GET_STRVAL(&pmicmd, "kvsname", p);
        MPL_strncpy(singinit_kvsname, p, MAX_SINGINIT_KVSNAME);
        PMIU_printf(PMIU_verbose, "kvsname to use is %s\n", singinit_kvsname);

        if (connectStdio) {
            int stdin_sock, stdout_sock, stderr_sock;
            PMIU_printf(PMIU_verbose, "Accepting three connections for stdin, out, err\n");
            stdin_sock = accept_one_connection(singinit_listen_sock);
            dup2(stdin_sock, 0);
            stdout_sock = accept_one_connection(singinit_listen_sock);
            dup2(stdout_sock, 1);
            stderr_sock = accept_one_connection(singinit_listen_sock);
            dup2(stderr_sock, 2);
        }
        PMIU_printf(PMIU_verbose, "Done with singinit handshake\n");
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/* Promote PMI to a fully initialized version if it was started as
   a singleton init */
static int PMIi_InitIfSingleton(void)
{
    int pmi_errno = PMI_SUCCESS;
    int rc;
    static int firstcall = 1;

    if (PMI_initialized != SINGLETON_INIT_BUT_NO_PM || !firstcall) {
        goto fn_exit;
    }

    /* We only try to init as a singleton the first time */
    firstcall = 0;

    /* First, start (if necessary) an mpiexec, connect to it,
     * and start the singleton init handshake */
    rc = PMII_singinit();
    if (rc < 0) {
        pmi_errno = PMI_FAIL;
        goto fn_fail;
    }

    PMI_initialized = SINGLETON_INIT_WITH_PM;   /* do this right away */
    PMI_size = 1;
    PMI_rank = 0;
    PMI_spawned = 0;

    pmi_errno = PMII_init(&PMI_server_version, &PMI_server_subversion);
    PMIU_ERR_POP(pmi_errno);

    pmi_errno = PMII_getmaxes(&PMI_kvsname_max, &PMI_keylen_max, &PMI_vallen_max);
    PMIU_ERR_POP(pmi_errno);

    if (cached_singinit_inuse) {
        /* if we cached a key-value put, push it up to the server */
        PMI_KVS_Put(singinit_kvsname, cached_singinit_key, cached_singinit_val);
        /* make the key-value visible */
        PMI_Barrier();
    }

  fn_exit:
    return PMI_SUCCESS;
  fn_fail:
    goto fn_exit;
}

static int accept_one_connection(int list_sock)
{
    int gotit, new_sock;
    MPL_sockaddr_t addr;
    socklen_t len;

    len = sizeof(addr);
    gotit = 0;
    while (!gotit) {
        new_sock = accept(list_sock, (struct sockaddr *) &addr, &len);
        if (new_sock == -1) {
            if (errno == EINTR) /* interrupted? If so, try again */
                continue;
            else {
                PMIU_printf(1, "accept failed in accept_one_connection\n");
                exit(-1);
            }
        } else
            gotit = 1;
    }
    return (new_sock);
}

#endif
/* end USE_PMI_PORT */

static int expect_pmi_cmd(const char *cmd, const char *key, int *val_out)
{
    int pmi_errno = PMI_SUCCESS;

    struct PMIU_cmd pmicmd;
    pmi_errno = PMIU_cmd_read(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP2(strcmp(pmicmd.cmd, cmd) != 0,
                         pmi_errno, PMI_FAIL, "expecting cmd=%s, got %s\n", cmd, pmicmd.cmd);

    if (val_out != NULL) {
        PMIU_CMD_GET_INTVAL(&pmicmd, key, *val_out);
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* DISABLE_PMI1 */
