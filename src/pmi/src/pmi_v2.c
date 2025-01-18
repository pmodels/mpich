/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef DISABLE_PMI2

#include "pmi_config.h"
#include "mpl.h"

#include "pmi_util.h"
#include "pmi2.h"
#include "pmi_wire.h"
#include "pmi_msg.h"
#include "pmi_common.h"

#define PMII_EXIT_CODE -1

#define MAX_INT_STR_LEN 11      /* number of digits in MAX_UINT + 1 */

#define PMII_MAX_COMMAND_LEN (64*1024)

#define USE_WIRE_VER  PMIU_WIRE_V2

/* we will call PMIU_cmd_free_buf */
static const bool no_static = false;

static bool cached_singinit_inuse;
static char cached_singinit_key[PMI2_MAX_KEYLEN];
static char cached_singinit_val[PMI2_MAX_VALLEN];

static int PMIi_InitIfSingleton(void);

/* ------------------------------------------------------------------------- */
/* PMI API Routines */
/* ------------------------------------------------------------------------- */
PMI_API_PUBLIC int PMI2_Set_threaded(int is_threaded)
{
#ifndef HAVE_THREADS
    if (is_threaded) {
        return PMI2_FAIL;
    }
    return PMI2_SUCCESS;

#else
    PMIU_is_threaded = is_threaded;
    return PMI2_SUCCESS;

#endif
}

PMI_API_PUBLIC int PMI2_Init(int *spawned, int *size, int *rank, int *appnum)
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    PMIU_thread_init();

    /* FIXME: Why is setvbuf commented out? */
    /* FIXME: What if the output should be fully buffered (directed to file)?
     * unbuffered (user explicitly set?) */
    /* setvbuf(stdout,0,_IONBF,0); */
    setbuf(stdout, NULL);
    /* PMIU_printf(1, "PMI2_INIT\n"); */

    /* Get the value of PMI2_DEBUG from the environment if possible, since
     * we may have set it to help debug the setup process */
    char *p;
    p = getenv("PMI2_DEBUG");
    if (p) {
        PMIU_verbose = atoi(p);
    } else {
        p = getenv("PMI_DEBUG");
        if (p) {
            PMIU_verbose = atoi(p);
        }
    }

    /* Get the fd for PMI commands; if none, we're a singleton */
    bool do_handshake ATTRIBUTE((unused));
    pmi_errno = PMIU_get_pmi_fd(&PMI_fd, &do_handshake);
    PMIU_ERR_POP(pmi_errno);

    if (PMI_fd == -1) {
        /* Singleton init: Process not started with mpiexec,
         * so set size to 1, rank to 0 */
        *size = 1;
        *rank = 0;
        *spawned = 0;

        PMI_initialized = SINGLETON_INIT_BUT_NO_PM;

        goto fn_exit;
    }

    /* do initial PMI1 init */
    PMIU_msg_set_query_init(&pmicmd, PMIU_WIRE_V1, no_static, PMI_VERSION, PMI_SUBVERSION);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    int server_version, server_subversion;
    pmi_errno = PMIU_msg_get_response_init(&pmicmd, &server_version, &server_subversion);

    PMIU_cmd_free_buf(&pmicmd);

    /* do full PMI2 init */
    const char *s_pmiid;
    s_pmiid = getenv("PMI_ID");
    if (!s_pmiid) {
        s_pmiid = getenv("PMI_RANK");
    }
    int pmiid;
    if (s_pmiid) {
        pmiid = atoi(s_pmiid);
    } else {
        pmiid = -1;
    }

    PMIU_msg_set_query_fullinit(&pmicmd, USE_WIRE_VER, no_static, pmiid);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    const char *spawner_jobid;
    int verbose;                /* unused */
    PMIU_msg_get_response_fullinit(&pmicmd, rank, size, appnum, &spawner_jobid, &verbose);
    PMIU_ERR_POP(pmi_errno);

    if (spawner_jobid == NULL) {
        *spawned = 0;
    } else {
        *spawned = 1;
    }

    if (!PMI_initialized) {
        PMI_initialized = NORMAL_INIT_WITH_PM;
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Finalize(void)
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        PMIU_msg_set_query(&pmicmd, USE_WIRE_VER, PMIU_CMD_FINALIZE, no_static);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);

        shutdown(PMI_fd, SHUT_RDWR);
        close(PMI_fd);
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Initialized(void)
{
    /* Turn this into a logical value (1 or 0) .  This allows us
     * to use PMI_initialized to distinguish between initialized with
     * an PMI service (e.g., via mpiexec) and the singleton init,
     * which has no PMI service */
    return PMI_initialized != 0;
}

PMI_API_PUBLIC int PMI2_Abort(int flag, const char msg[])
{
    PMIU_printf(1, "aborting job:\n%s\n", msg);

    struct PMIU_cmd pmicmd;
    PMIU_msg_set_query_abort(&pmicmd, USE_WIRE_VER, no_static, PMII_EXIT_CODE, msg);

    /* ignoring return code, because we're exiting anyway */
    PMIU_cmd_send(PMI_fd, &pmicmd);

    PMIU_Exit(PMII_EXIT_CODE);
    return PMI2_SUCCESS;
}

/* XXX DJG from Pavan's email:
cmd=spawn;thrid=string;ncmds=count;preputcount=n;ppkey0=name;ppval0=string;...;\
        subcmd=spawn-exe1;maxprocs=n;argc=narg;argv0=name;\
                argv1=name;...;infokeycount=n;infokey0=key;\
                infoval0=string;...;\
(... one subcmd for each executable ...)
*/
PMI_API_PUBLIC int PMI2_Job_Spawn(int count, const char *cmds[],
                                  int argcs[], const char **argvs[],
                                  const int maxprocs[],
                                  const int info_keyval_sizes[],
                                  const PMI2_keyval_t * info_keyval_vectors[],
                                  int preput_keyval_size,
                                  const PMI2_keyval_t preput_keyval_vector[],
                                  char jobId[], int jobIdSize, int errors[])
{
    int pmi_errno = PMI2_SUCCESS;
    int total_num_processes = 0;

    /* Connect to the PM if we haven't already */
    if (PMIi_InitIfSingleton() != 0)
        return -1;

    for (int i = 0; i < count; i++) {
        total_num_processes += maxprocs[i];
    }

    struct PMIU_cmd pmicmd;
    PMIU_msg_set_query_spawn(&pmicmd, USE_WIRE_VER, no_static,
                             count, cmds, maxprocs, argcs, argvs,
                             info_keyval_sizes, (const struct PMIU_token **) info_keyval_vectors,
                             preput_keyval_size, (const struct PMIU_token *) preput_keyval_vector);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    if (jobId && jobIdSize) {
        const char *jid;
        PMIU_CMD_GET_STRVAL(&pmicmd, "jobid", jid);
        MPL_strncpy(jobId, jid, jobIdSize);
    }

    /* PMI2 does not return error codes, so we'll just pretend that means
     * that it was going to send all `0's. */
    for (int i = 0; i < total_num_processes; ++i) {
        errors[i] = 0;
    }

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;

}

PMI_API_PUBLIC int PMI2_Job_GetId(char jobid[], int jobid_size)
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized == SINGLETON_INIT_BUT_NO_PM) {
        /* Return a dummy name */
        snprintf(jobid, jobid_size, "singinit_kvs_%d_0", (int) getpid());
        goto fn_exit;
    }

    PMIU_msg_set_query(&pmicmd, USE_WIRE_VER, PMIU_CMD_KVSNAME, no_static);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    const char *jid;
    pmi_errno = PMIU_msg_get_response_kvsname(&pmicmd, &jid);
    PMIU_ERR_POP(pmi_errno);

    MPL_strncpy(jobid, jid, jobid_size);
    PMIU_Set_rank_kvsname(PMI_rank, jid);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Job_Connect(const char jobid[], PMI2_Connect_comm_t * conn)
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_msg_set_query_connect(&pmicmd, USE_WIRE_VER, no_static, jobid);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    int kvscopy;
    pmi_errno = PMIU_msg_get_response_connect(&pmicmd, &kvscopy);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP(kvscopy, pmi_errno, PMI2_ERR_OTHER, "**notimpl");

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Job_Disconnect(const char jobid[])
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_msg_set_query_disconnect(&pmicmd, USE_WIRE_VER, no_static, jobid);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_KVS_Put(const char key[], const char value[])
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    /* This is a special hack to support singleton initialization */
    if (PMI_initialized == SINGLETON_INIT_BUT_NO_PM) {
        int rc;
        if (cached_singinit_inuse)
            return PMI2_FAIL;
        rc = MPL_strncpy(cached_singinit_key, key, PMI2_MAX_KEYLEN);
        if (rc != 0)
            return PMI2_FAIL;
        rc = MPL_strncpy(cached_singinit_val, value, PMI2_MAX_VALLEN);
        if (rc != 0)
            return PMI2_FAIL;
        cached_singinit_inuse = true;
        return PMI2_SUCCESS;
    }

    PMIU_msg_set_query_kvsput(&pmicmd, USE_WIRE_VER, no_static, key, value);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_KVS_Fence(void)
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        PMIU_msg_set_query(&pmicmd, USE_WIRE_VER, PMIU_CMD_KVSFENCE, no_static);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC
    int PMI2_KVS_Get(const char *jobid, int src_pmi_id, const char key[], char value[],
                     int maxValue, int *valLen)
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    /* singletons can skip the nonexistent key used for barrier */
    if (PMI_initialized == SINGLETON_INIT_BUT_NO_PM && strcmp(key, "-NONEXIST-KEY") == 0) {
        goto fn_exit;
    }

    PMIU_msg_set_query_kvsget(&pmicmd, USE_WIRE_VER, no_static, jobid, src_pmi_id, key);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    const char *tmp_val;
    bool found;
    pmi_errno = PMIU_msg_get_response_kvsget(&pmicmd, &found, &tmp_val);
    PMIU_ERR_POP(pmi_errno || !found);

    int ret;
    ret = MPL_strncpy(value, tmp_val, maxValue);

    int kvsvallen;
    kvsvallen = strlen(tmp_val);
    *valLen = ret ? -kvsvallen : kvsvallen;

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC
    int PMI2_Info_GetNodeAttr(const char name[], char value[], int valuelen, int *flag, int waitfor)
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_msg_set_query_getnodeattr(&pmicmd, USE_WIRE_VER, no_static, name, waitfor);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);

    const char *tmp_val;
    bool found;
    if (!pmi_errno) {
        pmi_errno = PMIU_msg_get_response_getnodeattr(&pmicmd, &found, &tmp_val);
    }

    if (!pmi_errno && found) {
        *flag = 1;
        MPL_strncpy(value, tmp_val, valuelen);
    } else {
        *flag = 0;
        pmi_errno = PMIU_SUCCESS;
    }

    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
}

static int parse_int_array(const char *str, int array[], int arraylen, int *outlen)
{
    int pmi_errno = PMI2_SUCCESS;

    int rc;
    int i = 0;
    rc = sscanf(str, "%d", &array[i]);
    PMIU_ERR_CHKANDJUMP1(rc != 1, pmi_errno, PMI2_ERR_OTHER,
                         "**intern %s", "unable to parse intarray");
    ++i;
    while ((str = strchr(str, ',')) && i < arraylen) {
        ++str;  /* skip over the ',' */
        rc = sscanf(str, "%d", &array[i]);
        PMIU_ERR_CHKANDJUMP1(rc != 1, pmi_errno, PMI2_ERR_OTHER,
                             "**intern %s", "unable to parse intarray");
        ++i;
    }

    *outlen = i;

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Info_GetNodeAttrIntArray(const char name[], int array[], int arraylen,
                                                 int *outlen, int *flag)
{
    int pmi_errno = PMI2_SUCCESS;

    pmi_errno = PMIi_InitIfSingleton();
    PMIU_ERR_POP(pmi_errno);

    struct PMIU_cmd pmicmd;
    /* assume always wait=false? */
    PMIU_msg_set_query_getnodeattr(&pmicmd, USE_WIRE_VER, no_static, name, false);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);

    const char *tmp_val;
    bool found;
    if (!pmi_errno) {
        pmi_errno = PMIU_msg_get_response_getnodeattr(&pmicmd, &found, &tmp_val);
    }

    if (!pmi_errno && found) {
        pmi_errno = parse_int_array(tmp_val, array, arraylen, outlen);
        PMIU_ERR_POP(pmi_errno);
        *flag = 1;
    } else {
        *flag = 0;
        pmi_errno = PMIU_SUCCESS;
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Info_PutNodeAttr(const char name[], const char value[])
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_msg_set_query_putnodeattr(&pmicmd, USE_WIRE_VER, no_static, name, value);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Info_GetJobAttr(const char name[], char value[], int valuelen, int *flag)
{
    int pmi_errno = PMI2_SUCCESS;

    if (PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        struct PMIU_cmd pmicmd;
        PMIU_msg_set_query_get(&pmicmd, USE_WIRE_VER, no_static, NULL, name);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);

        bool found;
        const char *tmp_val;
        if (pmi_errno == PMIU_SUCCESS) {
            pmi_errno = PMIU_msg_get_response_get(&pmicmd, &found, &tmp_val);
        }

        if (!pmi_errno && found) {
            MPL_strncpy(value, tmp_val, valuelen);
            *flag = 1;
        } else {
            *flag = 0;
            pmi_errno = PMIU_SUCCESS;
        }

        PMIU_cmd_free_buf(&pmicmd);
    } else {
        *flag = 0;
    }

    return pmi_errno;
}

PMI_API_PUBLIC int PMI2_Info_GetJobAttrIntArray(const char name[], int array[], int arraylen,
                                                int *outlen, int *flag)
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_msg_set_query_get(&pmicmd, USE_WIRE_VER, no_static, NULL, name);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);

    bool found;
    const char *tmp_val;
    if (pmi_errno == PMIU_SUCCESS) {
        pmi_errno = PMIU_msg_get_response_get(&pmicmd, &found, &tmp_val);
    }

    if (!pmi_errno && found) {
        pmi_errno = parse_int_array(tmp_val, array, arraylen, outlen);
        PMIU_ERR_POP(pmi_errno);
        *flag = 1;
    } else {
        *flag = 0;
        pmi_errno = PMIU_SUCCESS;
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Nameserv_publish(const char service_name[], const PMI2_keyval_t * info_ptr,
                                         const char port[])
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    /* ignoring infokey functionality for now */
    PMIU_msg_set_query_publish(&pmicmd, USE_WIRE_VER, no_static, service_name, port);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}


PMI_API_PUBLIC int PMI2_Nameserv_lookup(const char service_name[], const PMI2_keyval_t * info_ptr,
                                        char port[], int portLen)
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    /* ignoring infokey functionality for now */
    PMIU_msg_set_query_lookup(&pmicmd, USE_WIRE_VER, no_static, service_name);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    const char *tmp_port;
    pmi_errno = PMIU_msg_get_response_lookup(&pmicmd, &tmp_port);

    MPL_strncpy(port, tmp_port, portLen);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Nameserv_unpublish(const char service_name[],
                                           const PMI2_keyval_t * info_ptr)
{
    int pmi_errno = PMI2_SUCCESS;

    struct PMIU_cmd pmicmd;
    /* ignoring infokey functionality for now */
    PMIU_msg_set_query_unpublish(&pmicmd, USE_WIRE_VER, no_static, service_name);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
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

/* Promote PMI to a fully initialized version if it was started as
   a singleton init */
static int PMIi_InitIfSingleton(void)
{
    return 0;
}

#endif /* DISABLE_PMI2 */
