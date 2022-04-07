/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "pmi_config.h"

#include "pmi_util.h"
#include "mpl.h"
#include "pmi2.h"

#define PMII_EXIT_CODE -1

#define MAX_INT_STR_LEN 11      /* number of digits in MAX_UINT + 1 */

#define PMII_COMMANDLEN_SIZE 6
#define PMII_MAX_COMMAND_LEN (64*1024)

static const char FULLINIT_CMD[] = "fullinit";
static const char FULLINITRESP_CMD[] = "fullinit-response";
static const char FINALIZE_CMD[] = "finalize";
static const char FINALIZERESP_CMD[] = "finalize-response";
static const char ABORT_CMD[] = "abort";
static const char JOBGETID_CMD[] = "job-getid";
static const char JOBGETIDRESP_CMD[] = "job-getid-response";
static const char JOBCONNECT_CMD[] = "job-connect";
static const char JOBCONNECTRESP_CMD[] = "job-connect-response";
static const char JOBDISCONNECT_CMD[] = "job-disconnect";
static const char JOBDISCONNECTRESP_CMD[] = "job-disconnect-response";
static const char KVSPUT_CMD[] = "kvs-put";
static const char KVSPUTRESP_CMD[] = "kvs-put-response";
static const char KVSFENCE_CMD[] = "kvs-fence";
static const char KVSFENCERESP_CMD[] = "kvs-fence-response";
static const char KVSGET_CMD[] = "kvs-get";
static const char KVSGETRESP_CMD[] = "kvs-get-response";
static const char GETNODEATTR_CMD[] = "info-getnodeattr";
static const char GETNODEATTRRESP_CMD[] = "info-getnodeattr-response";
static const char PUTNODEATTR_CMD[] = "info-putnodeattr";
static const char PUTNODEATTRRESP_CMD[] = "info-putnodeattr-response";
static const char GETJOBATTR_CMD[] = "info-getjobattr";
static const char GETJOBATTRRESP_CMD[] = "info-getjobattr-response";
static const char NAMEPUBLISH_CMD[] = "name-publish";
static const char NAMEPUBLISHRESP_CMD[] = "name-publish-response";
static const char NAMEUNPUBLISH_CMD[] = "name-unpublish";
static const char NAMEUNPUBLISHRESP_CMD[] = "name-unpublish-response";
static const char NAMELOOKUP_CMD[] = "name-lookup";
static const char NAMELOOKUPRESP_CMD[] = "name-lookup-response";


static const char PMIJOBID_KEY[] = "pmijobid";
static const char PMIRANK_KEY[] = "pmirank";
static const char SRCID_KEY[] = "srcid";
static const char THREADED_KEY[] = "threaded";
static const char RC_KEY[] = "rc";
static const char ERRMSG_KEY[] = "errmsg";
static const char PMIVERSION_KEY[] = "pmi-version";
static const char PMISUBVER_KEY[] = "pmi-subversion";
static const char RANK_KEY[] = "rank";
static const char SIZE_KEY[] = "size";
static const char APPNUM_KEY[] = "appnum";
static const char SPAWNERJOBID_KEY[] = "spawner-jobid";
static const char DEBUGGED_KEY[] = "debugged";
static const char PMIVERBOSE_KEY[] = "pmiverbose";
static const char ISWORLD_KEY[] = "isworld";
static const char MSG_KEY[] = "msg";
static const char JOBID_KEY[] = "jobid";
static const char KVSCOPY_KEY[] = "kvscopy";
static const char KEY_KEY[] = "key";
static const char VALUE_KEY[] = "value";
static const char FOUND_KEY[] = "found";
static const char WAIT_KEY[] = "wait";
static const char NAME_KEY[] = "name";
static const char PORT_KEY[] = "port";
static const char THRID_KEY[] = "thrid";
static const char INFOKEYCOUNT_KEY[] = "infokeycount";
static const char INFOKEY_KEY[] = "infokey%d";
static const char INFOVAL_KEY[] = "infoval%d";

static const char TRUE_VAL[] = "TRUE";
static const char FALSE_VAL[] = "FALSE";


/* Local types */

/* Parse commands are in this structure.  Fields in this structure are
   dynamically allocated as necessary */
typedef struct PMI2_Keyvalpair {
    const char *key;
    const char *value;
    int valueLen;               /* Length of a value (values may contain nulls, so
                                 * we need this) */
    int isCopy;                 /* The value is a copy (and will need to be freed)
                                 * if this is true, otherwise,
                                 * it is a null-terminated string in the original
                                 * buffer */
} PMI2_Keyvalpair;

typedef struct PMI2_Command {
    int nPairs;                 /* Number of key=value pairs */
    char *command;              /* Overall command buffer */
    PMI2_Keyvalpair **pairs;    /* Array of pointers to pairs */
    int complete;
} PMI2_Command;


typedef enum { PMI2_UNINITIALIZED = 0,
    SINGLETON_INIT_BUT_NO_PM = 1,
    NORMAL_INIT_WITH_PM,
    SINGLETON_INIT_WITH_PM
} PMI2State;
static PMI2State PMI2_initialized = PMI2_UNINITIALIZED;

static int PMI2_debug = 0;
static int PMI2_fd = -1;
static int PMI2_size = 1;
static int PMI2_rank = 0;

static int PMI2_is_threaded = 0;        /* Set this to true to require thread safety */

#ifdef HAVE_THREADS
static MPL_thread_mutex_t mutex;
static int blocked = PMIU_FALSE;
static MPL_thread_cond_t cond;
#endif

/* XXX DJG the "const"s on both of these functions and the Keyvalpair
 * struct are wrong in the isCopy==TRUE case! */
/* init_kv_str -- fills in keyvalpair.  val is required to be a
   null-terminated string.  isCopy is set to FALSE, so caller must
   free key and val memory, if necessary.
*/
static void init_kv_str(PMI2_Keyvalpair * kv, const char key[], const char val[])
{
    kv->key = key;
    kv->value = val;
    kv->valueLen = strlen(val);
    kv->isCopy = PMIU_FALSE;
}

/* same as init_kv_str, but strdup's the key and val first, and sets isCopy=TRUE */
static void init_kv_strdup(PMI2_Keyvalpair * kv, const char key[], const char val[])
{
    /* XXX DJG could be slightly more efficient */
    init_kv_str(kv, PMIU_Strdup(key), PMIU_Strdup(val));
    kv->isCopy = PMIU_TRUE;
}

/* same as init_kv_strdup, but converts val into a string first */
/* XXX DJG could be slightly more efficient */
static void init_kv_strdup_int(PMI2_Keyvalpair * kv, const char key[], int val)
{
    char tmpbuf[32] = { 0 };
    int rc = PMI2_SUCCESS;

    rc = MPL_snprintf(tmpbuf, sizeof(tmpbuf), "%d", val);
    PMIU_Assert(rc >= 0);
    init_kv_strdup(kv, key, tmpbuf);
}

/* initializes the key with ("%s%d", key_prefix, suffix), uses a string value */
/* XXX DJG could be slightly more efficient */
static void init_kv_strdup_intsuffix(PMI2_Keyvalpair * kv, const char key_prefix[], int suffix,
                                     const char val[])
{
    char tmpbuf[256 /*XXX HACK */ ] = { 0 };
    int rc = PMI2_SUCCESS;

    rc = MPL_snprintf(tmpbuf, sizeof(tmpbuf), "%s%d", key_prefix, suffix);
    PMIU_Assert(rc >= 0);
    init_kv_strdup(kv, tmpbuf, val);
}


static int getPMIFD(void);
static int PMIi_ReadCommandExp(int fd, PMI2_Command * cmd, const char *exp, int *rc,
                               const char **errmsg);
static int PMIi_ReadCommand(int fd, PMI2_Command * cmd);

static int PMIi_WriteSimpleCommand(int fd, PMI2_Command * resp, const char cmd[],
                                   PMI2_Keyvalpair * pairs[], int npairs);
static int PMIi_WriteSimpleCommandStr(int fd, PMI2_Command * resp, const char cmd[], ...);
static int PMIi_InitIfSingleton(void);

static void freepairs(PMI2_Keyvalpair ** pairs, int npairs);
static int getval(PMI2_Keyvalpair * const pairs[], int npairs, const char *key, const char **value,
                  int *vallen);
static int getvalint(PMI2_Keyvalpair * const pairs[], int npairs, const char *key, int *val);
static int getvalptr(PMI2_Keyvalpair * const pairs[], int npairs, const char *key, void *val);
static int getvalbool(PMI2_Keyvalpair * const pairs[], int npairs, const char *key, int *val);

static void dump_PMI2_Keyvalpair(FILE * file, PMI2_Keyvalpair * kv);


typedef struct pending_item {
    struct pending_item *next;
    PMI2_Command *cmd;
} pending_item_t;

pending_item_t *pendingq_head = NULL;
pending_item_t *pendingq_tail = NULL;

static inline void ENQUEUE(PMI2_Command * cmd)
{
    pending_item_t *pi = PMIU_Malloc(sizeof(pending_item_t));

    pi->next = NULL;
    pi->cmd = cmd;

    if (pendingq_head == NULL) {
        pendingq_head = pendingq_tail = pi;
    } else {
        pendingq_tail->next = pi;
        pendingq_tail = pi;
    }
}

static inline int SEARCH_REMOVE(PMI2_Command * cmd)
{
    pending_item_t *pi, *prev;

    pi = pendingq_head;
    if (pi->cmd == cmd) {
        pendingq_head = pi->next;
        if (pendingq_head == NULL)
            pendingq_tail = NULL;
        PMIU_Free(pi);
        return 1;
    }
    prev = pi;
    pi = pi->next;

    for (; pi; pi = pi->next) {
        if (pi->cmd == cmd) {
            prev->next = pi->next;
            if (prev->next == NULL)
                pendingq_tail = prev;
            PMIU_Free(pi);
            return 1;
        }
    }

    return 0;
}



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
    PMI2_is_threaded = is_threaded;
    return PMI2_SUCCESS;

#endif
}

PMI_API_PUBLIC int PMI2_Init(int *spawned, int *size, int *rank, int *appnum)
{
    int pmi_errno = PMI2_SUCCESS;
    char *p;
    char buf[PMIU_MAXLINE], cmdline[PMIU_MAXLINE];
    char *jobid;
    char *pmiid;
    int ret;

#if HAVE_THREADS
    MPL_thread_mutex_create(&mutex, &ret);
    PMIU_Assert(!ret);
    MPL_thread_cond_create(&cond, &ret);
    PMIU_Assert(!ret);
#endif

    /* FIXME: Why is setvbuf commented out? */
    /* FIXME: What if the output should be fully buffered (directed to file)?
     * unbuffered (user explicitly set?) */
    /* setvbuf(stdout,0,_IONBF,0); */
    setbuf(stdout, NULL);
    /* PMIU_printf(1, "PMI2_INIT\n"); */

    /* Get the value of PMI2_DEBUG from the environment if possible, since
     * we may have set it to help debug the setup process */
    p = getenv("PMI2_DEBUG");
    if (p)
        PMI2_debug = atoi(p);

    /* Get the fd for PMI commands; if none, we're a singleton */
    pmi_errno = getPMIFD();
    PMIU_ERR_POP(pmi_errno);

    if (PMI2_fd == -1) {
        /* Singleton init: Process not started with mpiexec,
         * so set size to 1, rank to 0 */
        PMI2_size = 1;
        PMI2_rank = 0;
        *spawned = 0;

        PMI2_initialized = SINGLETON_INIT_BUT_NO_PM;

        goto fn_exit;
    }

    /* do initial PMI1 init */
    ret =
        MPL_snprintf(buf, PMIU_MAXLINE, "cmd=init pmi_version=%d pmi_subversion=%d\n", PMI_VERSION,
                     PMI_SUBVERSION);
    PMIU_ERR_CHKANDJUMP1(ret < 0, pmi_errno, PMI2_ERR_OTHER,
                         "**intern %s", "failed to generate init line");

    ret = PMIU_writeline(PMI2_fd, buf);
    PMIU_ERR_CHKANDJUMP(ret < 0, pmi_errno, PMI2_ERR_OTHER, "**pmi2_init_send");

    ret = PMIU_readline(PMI2_fd, buf, PMIU_MAXLINE);
    PMIU_ERR_CHKANDJUMP1(ret < 0, pmi_errno, PMI2_ERR_OTHER, "**pmi2_initack %s", strerror(errno));

    PMIU_parse_keyvals(buf);
    cmdline[0] = 0;
    PMIU_getval("cmd", cmdline, PMIU_MAXLINE);
    PMIU_ERR_CHKANDJUMP(strncmp(cmdline, "response_to_init", PMIU_MAXLINE) != 0, pmi_errno,
                        PMI2_ERR_OTHER, "**bad_cmd");

    PMIU_getval("rc", buf, PMIU_MAXLINE);
    if (strncmp(buf, "0", PMIU_MAXLINE) != 0) {
        char buf1[PMIU_MAXLINE];
        PMIU_getval("pmi_version", buf, PMIU_MAXLINE);
        PMIU_getval("pmi_subversion", buf1, PMIU_MAXLINE);
        PMIU_ERR_SETANDJUMP4(pmi_errno, PMI2_ERR_OTHER,
                             "**pmi2_version %s %s %d %d", buf, buf1, PMI_VERSION, PMI_SUBVERSION);
    }

    /* do full PMI2 init */
    {
        PMI2_Keyvalpair pairs[3];
        PMI2_Keyvalpair *pairs_p[] = { pairs, pairs + 1, pairs + 2 };
        int npairs = 0;
        const char *errmsg;
        int rc;
        int found;
        int version, subver;
        const char *spawner_jobid;
        int spawner_jobid_len;
        PMI2_Command cmd = { 0 };
        int debugged;


        jobid = getenv("PMI_JOBID");
        if (jobid) {
            init_kv_str(&pairs[npairs], PMIJOBID_KEY, jobid);
            ++npairs;
        }

        pmiid = getenv("PMI_ID");
        if (pmiid) {
            init_kv_str(&pairs[npairs], PMIRANK_KEY, pmiid);
            ++npairs;
        } else {
            pmiid = getenv("PMI_RANK");
            if (pmiid) {
                init_kv_str(&pairs[npairs], PMIRANK_KEY, pmiid);
                ++npairs;
            }
        }

        init_kv_str(&pairs[npairs], THREADED_KEY, PMI2_is_threaded ? "TRUE" : "FALSE");
        ++npairs;


        pmi_errno = PMIi_WriteSimpleCommand(PMI2_fd, 0, FULLINIT_CMD, pairs_p, npairs); /* don't pass in thread id for init */
        PMIU_ERR_POP(pmi_errno);

        /* Read auth-response */
        /* Send auth-response-complete */

        /* Read fullinit-response */
        pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, FULLINITRESP_CMD, &rc, &errmsg);
        PMIU_ERR_POP(pmi_errno);
        PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                             "**pmi2_fullinit %s", errmsg ? errmsg : "unknown");

        found = getvalint(cmd.pairs, cmd.nPairs, PMIVERSION_KEY, &version);
        PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

        found = getvalint(cmd.pairs, cmd.nPairs, PMISUBVER_KEY, &subver);
        PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

        found = getvalint(cmd.pairs, cmd.nPairs, RANK_KEY, rank);
        PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

        found = getvalint(cmd.pairs, cmd.nPairs, SIZE_KEY, size);
        PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

        found = getvalint(cmd.pairs, cmd.nPairs, APPNUM_KEY, appnum);
        PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

        found = getval(cmd.pairs, cmd.nPairs, SPAWNERJOBID_KEY, &spawner_jobid, &spawner_jobid_len);
        PMIU_ERR_CHKANDJUMP(found == -1, pmi_errno, PMI2_ERR_OTHER, "**intern");
        if (found)
            *spawned = PMIU_TRUE;
        else
            *spawned = PMIU_FALSE;

        debugged = 0;
        found = getvalbool(cmd.pairs, cmd.nPairs, DEBUGGED_KEY, &debugged);
        PMIU_ERR_CHKANDJUMP(found == -1, pmi_errno, PMI2_ERR_OTHER, "**intern");
        PMI2_debug |= debugged;

        found = getvalbool(cmd.pairs, cmd.nPairs, PMIVERBOSE_KEY, &PMIU_verbose);
        PMIU_ERR_CHKANDJUMP(found == -1, pmi_errno, PMI2_ERR_OTHER, "**intern");

        PMIU_Free(cmd.command);
        freepairs(cmd.pairs, cmd.nPairs);
    }

    if (!PMI2_initialized)
        PMI2_initialized = NORMAL_INIT_WITH_PM;

  fn_exit:
    return pmi_errno;
  fn_fail:

    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Finalize(void)
{
    int pmi_errno = PMI2_SUCCESS;
    int rc;
    const char *errmsg;
    PMI2_Command cmd = { 0 };

    if (PMI2_initialized > SINGLETON_INIT_BUT_NO_PM) {
        pmi_errno = PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, FINALIZE_CMD, NULL);
        PMIU_ERR_POP(pmi_errno);
        pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, FINALIZERESP_CMD, &rc, &errmsg);
        PMIU_ERR_POP(pmi_errno);
        PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                             "**pmi2_finalize %s", errmsg ? errmsg : "unknown");
        PMIU_Free(cmd.command);
        freepairs(cmd.pairs, cmd.nPairs);

        shutdown(PMI2_fd, SHUT_RDWR);
        close(PMI2_fd);
    }

  fn_exit:
    return pmi_errno;
  fn_fail:

    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Initialized(void)
{
    /* Turn this into a logical value (1 or 0) .  This allows us
     * to use PMI2_initialized to distinguish between initialized with
     * an PMI service (e.g., via mpiexec) and the singleton init,
     * which has no PMI service */
    return PMI2_initialized != 0;
}

PMI_API_PUBLIC int PMI2_Abort(int flag, const char msg[])
{
    PMIU_printf(1, "aborting job:\n%s\n", msg);

    /* ignoring return code, because we're exiting anyway */
    PMIi_WriteSimpleCommandStr(PMI2_fd, NULL, ABORT_CMD, ISWORLD_KEY, flag ? TRUE_VAL : FALSE_VAL,
                               MSG_KEY, msg, NULL);

    PMIU_Exit(PMII_EXIT_CODE);
    return PMI2_SUCCESS;
}

PMI_API_PUBLIC
    int PMI2_Job_Spawn(int count, const char *cmds[],
                       int argcs[], const char **argvs[],
                       const int maxprocs[],
                       const int info_keyval_sizes[],
                       const PMI2_keyval_t * info_keyval_vectors[],
                       int preput_keyval_size,
                       const PMI2_keyval_t preput_keyval_vector[],
                       char jobId[], int jobIdSize, int errors[])
{
    int i, rc, spawncnt, total_num_processes;
    int found;
    const char *jid;
    int jidlen;
    int spawn_rc;
    const char *errmsg = NULL;
    PMI2_Command resp_cmd = { 0 };
    int pmi_errno = 0;
    PMI2_Keyvalpair **pairs_p = NULL;
    int npairs = 0;
    int total_pairs = 0;

    /* Connect to the PM if we haven't already */
    if (PMIi_InitIfSingleton() != 0)
        return -1;

    total_num_processes = 0;

/* XXX DJG from Pavan's email:
cmd=spawn;thrid=string;ncmds=count;preputcount=n;ppkey0=name;ppval0=string;...;\
        subcmd=spawn-exe1;maxprocs=n;argc=narg;argv0=name;\
                argv1=name;...;infokeycount=n;infokey0=key;\
                infoval0=string;...;\
(... one subcmd for each executable ...)
*/

    /* FIXME overall need a better interface for building commands!
     * Need to be able to append commands, and to easily accept integer
     * valued arguments.  Memory management should stay completely out
     * of mind when writing a new PMI command impl like this! */

    /* Calculate the total number of keyval pairs that we need.
     *
     * The command writing utility adds "cmd" and "thrid" fields for us,
     * don't include them in our count. */
    total_pairs = 2;    /* ncmds,preputcount */
    total_pairs += (3 * count); /* subcmd,maxprocs,argc */
    total_pairs += (2 * preput_keyval_size);    /* ppkeyN,ppvalN */
    for (spawncnt = 0; spawncnt < count; ++spawncnt) {
        total_pairs += argcs[spawncnt]; /* argvN */
        if (info_keyval_sizes) {
            total_pairs += 1;   /* infokeycount */
            total_pairs += 2 * info_keyval_sizes[spawncnt];     /* infokeyN,infovalN */
        }
    }

    pairs_p = PMIU_Malloc(total_pairs * sizeof(PMI2_Keyvalpair *));
    /* individiually allocating instead of batch alloc b/c freepairs assumes it */
    for (i = 0; i < total_pairs; ++i) {
        /* FIXME we are somehow still leaking some of this memory */
        pairs_p[i] = PMIU_Malloc(sizeof(PMI2_Keyvalpair));
        PMIU_Assert(pairs_p[i]);
    }

    init_kv_strdup_int(pairs_p[npairs++], "ncmds", count);

    init_kv_strdup_int(pairs_p[npairs++], "preputcount", preput_keyval_size);
    for (i = 0; i < preput_keyval_size; ++i) {
        init_kv_strdup_intsuffix(pairs_p[npairs++], "ppkey", i, preput_keyval_vector[i].key);
        init_kv_strdup_intsuffix(pairs_p[npairs++], "ppval", i, preput_keyval_vector[i].val);
    }

    for (spawncnt = 0; spawncnt < count; ++spawncnt) {
        total_num_processes += maxprocs[spawncnt];

        init_kv_strdup(pairs_p[npairs++], "subcmd", cmds[spawncnt]);
        init_kv_strdup_int(pairs_p[npairs++], "maxprocs", maxprocs[spawncnt]);

        init_kv_strdup_int(pairs_p[npairs++], "argc", argcs[spawncnt]);
        for (i = 0; i < argcs[spawncnt]; ++i) {
            init_kv_strdup_intsuffix(pairs_p[npairs++], "argv", i, argvs[spawncnt][i]);
        }

        if (info_keyval_sizes) {
            init_kv_strdup_int(pairs_p[npairs++], "infokeycount", info_keyval_sizes[spawncnt]);
            for (i = 0; i < info_keyval_sizes[spawncnt]; ++i) {
                init_kv_strdup_intsuffix(pairs_p[npairs++], "infokey", i,
                                         info_keyval_vectors[spawncnt][i].key);
                init_kv_strdup_intsuffix(pairs_p[npairs++], "infoval", i,
                                         info_keyval_vectors[spawncnt][i].val);
            }
        }
    }

    if (npairs < total_pairs) {
        printf_d("about to fail assertion, npairs=%d total_pairs=%d\n", npairs, total_pairs);
    }
    PMIU_Assert(npairs == total_pairs);

    pmi_errno = PMIi_WriteSimpleCommand(PMI2_fd, &resp_cmd, "spawn", pairs_p, npairs);
    PMIU_ERR_POP(pmi_errno);

    freepairs(pairs_p, npairs);
    pairs_p = NULL;

    /* XXX DJG TODO release any upper level MPICH critical sections */
    rc = PMIi_ReadCommandExp(PMI2_fd, &resp_cmd, "spawn-response", &spawn_rc, &errmsg);
    if (rc != 0) {
        return PMI2_FAIL;
    }

    /* XXX DJG TODO deal with the response */
    PMIU_Assert(errors != NULL);

    if (jobId && jobIdSize) {
        found = getval(resp_cmd.pairs, resp_cmd.nPairs, JOBID_KEY, &jid, &jidlen);
        PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");
        MPL_strncpy(jobId, jid, jobIdSize);
    }

    /* PMI2 does not return error codes, so we'll just pretend that means
     * that it was going to send all `0's. */
    for (i = 0; i < total_num_processes; ++i) {
        errors[i] = 0;
    }

  fn_fail:
    PMIU_Free(resp_cmd.command);
    freepairs(resp_cmd.pairs, resp_cmd.nPairs);
    if (pairs_p)
        freepairs(pairs_p, npairs);

    return pmi_errno;
}

PMI_API_PUBLIC int PMI2_Job_GetId(char jobid[], int jobid_size)
{
    int pmi_errno = PMI2_SUCCESS;
    int found;
    const char *jid;
    int jidlen;
    int rc;
    const char *errmsg;
    PMI2_Command cmd = { 0 };

    pmi_errno = PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, JOBGETID_CMD, NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, JOBGETIDRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER, "**pmi2_jobgetid %s",
                         errmsg ? errmsg : "unknown");

    found = getval(cmd.pairs, cmd.nPairs, JOBID_KEY, &jid, &jidlen);
    PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

    MPL_strncpy(jobid, jid, jobid_size);

  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:

    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Job_Connect(const char jobid[], PMI2_Connect_comm_t * conn)
{
    int pmi_errno = PMI2_SUCCESS;
    PMI2_Command cmd = { 0 };
    int found;
    int kvscopy;
    int rc;
    const char *errmsg;

    pmi_errno = PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, JOBCONNECT_CMD, JOBID_KEY, jobid, NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, JOBCONNECTRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_jobconnect %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, KVSCOPY_KEY, &kvscopy);
    PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

    PMIU_ERR_CHKANDJUMP(kvscopy, pmi_errno, PMI2_ERR_OTHER, "**notimpl");


  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:

    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Job_Disconnect(const char jobid[])
{
    int pmi_errno = PMI2_SUCCESS;
    PMI2_Command cmd = { 0 };
    int rc;
    const char *errmsg;

    pmi_errno =
        PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, JOBDISCONNECT_CMD, JOBID_KEY, jobid, NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, JOBDISCONNECTRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_jobdisconnect %s", errmsg ? errmsg : "unknown");

  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_KVS_Put(const char key[], const char value[])
{
    int pmi_errno = PMI2_SUCCESS;
    PMI2_Command cmd = { 0 };
    int rc;
    const char *errmsg;

    pmi_errno =
        PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, KVSPUT_CMD, KEY_KEY, key, VALUE_KEY, value, NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, KVSPUTRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_kvsput %s", errmsg ? errmsg : "unknown");

  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_KVS_Fence(void)
{
    int pmi_errno = PMI2_SUCCESS;
    PMI2_Command cmd = { 0 };
    int rc;
    const char *errmsg;

    pmi_errno = PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, KVSFENCE_CMD, NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, KVSFENCERESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_kvsfence %s", errmsg ? errmsg : "unknown");

  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC
    int PMI2_KVS_Get(const char *jobid, int src_pmi_id, const char key[], char value[],
                     int maxValue, int *valLen)
{
    int pmi_errno = PMI2_SUCCESS;
    int found, keyfound;
    const char *kvsvalue;
    int kvsvallen;
    int ret;
    PMI2_Command cmd = { 0 };
    int rc;
    char src_pmi_id_str[256];
    const char *errmsg;

    MPL_snprintf(src_pmi_id_str, sizeof(src_pmi_id_str), "%d", src_pmi_id);

    pmi_errno = PMIi_InitIfSingleton();
    PMIU_ERR_POP(pmi_errno);

    pmi_errno =
        PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, KVSGET_CMD, JOBID_KEY, jobid, SRCID_KEY,
                                   src_pmi_id_str, KEY_KEY, key, NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, KVSGETRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_kvsget %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, FOUND_KEY, &keyfound);
    PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");
    PMIU_ERR_CHKANDJUMP(!keyfound, pmi_errno, PMI2_ERR_OTHER, "**pmi2_kvsget_notfound");

    found = getval(cmd.pairs, cmd.nPairs, VALUE_KEY, &kvsvalue, &kvsvallen);
    PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

    ret = MPL_strncpy(value, kvsvalue, maxValue);
    *valLen = ret ? -kvsvallen : kvsvallen;


  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:

    goto fn_exit;
}

PMI_API_PUBLIC
    int PMI2_Info_GetNodeAttr(const char name[], char value[], int valuelen, int *flag, int waitfor)
{
    int pmi_errno = PMI2_SUCCESS;
    int found;
    const char *kvsvalue;
    int kvsvallen;
    PMI2_Command cmd = { 0 };
    int rc;
    const char *errmsg;

    pmi_errno = PMIi_InitIfSingleton();
    PMIU_ERR_POP(pmi_errno);

    pmi_errno =
        PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, GETNODEATTR_CMD, KEY_KEY, name, WAIT_KEY,
                                   waitfor ? "TRUE" : "FALSE", NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, GETNODEATTRRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_getnodeattr %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, FOUND_KEY, flag);
    PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");
    if (*flag) {
        found = getval(cmd.pairs, cmd.nPairs, VALUE_KEY, &kvsvalue, &kvsvallen);
        PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

        MPL_strncpy(value, kvsvalue, valuelen);
    }

  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC
    int PMI2_Info_GetNodeAttrIntArray(const char name[], int array[], int arraylen, int *outlen,
                                      int *flag)
{
    int pmi_errno = PMI2_SUCCESS;
    int found;
    const char *kvsvalue;
    int kvsvallen;
    PMI2_Command cmd = { 0 };
    int rc;
    const char *errmsg;
    int i;
    const char *valptr;

    pmi_errno = PMIi_InitIfSingleton();
    PMIU_ERR_POP(pmi_errno);

    pmi_errno =
        PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, GETNODEATTR_CMD, KEY_KEY, name, WAIT_KEY, "FALSE",
                                   NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, GETNODEATTRRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_getnodeattr %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, FOUND_KEY, flag);
    PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");
    if (*flag) {
        found = getval(cmd.pairs, cmd.nPairs, VALUE_KEY, &kvsvalue, &kvsvallen);
        PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

        valptr = kvsvalue;
        i = 0;
        rc = sscanf(valptr, "%d", &array[i]);
        PMIU_ERR_CHKANDJUMP1(rc != 1, pmi_errno, PMI2_ERR_OTHER,
                             "**intern %s", "unable to parse intarray");
        ++i;
        while ((valptr = strchr(valptr, ',')) && i < arraylen) {
            ++valptr;   /* skip over the ',' */
            rc = sscanf(valptr, "%d", &array[i]);
            PMIU_ERR_CHKANDJUMP1(rc != 1, pmi_errno, PMI2_ERR_OTHER,
                                 "**intern %s", "unable to parse intarray");
            ++i;
        }

        *outlen = i;
    }

  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Info_PutNodeAttr(const char name[], const char value[])
{
    int pmi_errno = PMI2_SUCCESS;
    PMI2_Command cmd = { 0 };
    int rc;
    const char *errmsg;

    pmi_errno =
        PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, PUTNODEATTR_CMD, KEY_KEY, name, VALUE_KEY, value,
                                   NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, PUTNODEATTRRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_putnodeattr %s", errmsg ? errmsg : "unknown");

  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC int PMI2_Info_GetJobAttr(const char name[], char value[], int valuelen, int *flag)
{
    int pmi_errno = PMI2_SUCCESS;
    int found;
    const char *kvsvalue;
    int kvsvallen;
    PMI2_Command cmd = { 0 };
    int rc;
    const char *errmsg;

    pmi_errno = PMIi_InitIfSingleton();
    PMIU_ERR_POP(pmi_errno);

    pmi_errno = PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, GETJOBATTR_CMD, KEY_KEY, name, NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, GETJOBATTRRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_getjobattr %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, FOUND_KEY, flag);
    PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

    if (*flag) {
        found = getval(cmd.pairs, cmd.nPairs, VALUE_KEY, &kvsvalue, &kvsvallen);
        PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

        MPL_strncpy(value, kvsvalue, valuelen);
    }

  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC
    int PMI2_Info_GetJobAttrIntArray(const char name[], int array[], int arraylen, int *outlen,
                                     int *flag)
{
    int pmi_errno = PMI2_SUCCESS;
    int found;
    const char *kvsvalue;
    int kvsvallen;
    PMI2_Command cmd = { 0 };
    int rc;
    const char *errmsg;
    int i;
    const char *valptr;

    pmi_errno = PMIi_InitIfSingleton();
    PMIU_ERR_POP(pmi_errno);

    pmi_errno = PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, GETJOBATTR_CMD, KEY_KEY, name, NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, GETJOBATTRRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_getjobattr %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, FOUND_KEY, flag);
    PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");
    if (*flag) {
        found = getval(cmd.pairs, cmd.nPairs, VALUE_KEY, &kvsvalue, &kvsvallen);
        PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

        valptr = kvsvalue;
        i = 0;
        rc = sscanf(valptr, "%d", &array[i]);
        PMIU_ERR_CHKANDJUMP1(rc != 1, pmi_errno, PMI2_ERR_OTHER,
                             "**intern %s", "unable to parse intarray");
        ++i;
        while ((valptr = strchr(valptr, ',')) && i < arraylen) {
            ++valptr;   /* skip over the ',' */
            rc = sscanf(valptr, "%d", &array[i]);
            PMIU_ERR_CHKANDJUMP1(rc != 1, pmi_errno, PMI2_ERR_OTHER,
                                 "**intern %s", "unable to parse intarray");
            ++i;
        }

        *outlen = i;
    }

  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC
    int PMI2_Nameserv_publish(const char service_name[], const PMI2_keyval_t * info_ptr,
                              const char port[])
{
    int pmi_errno = PMI2_SUCCESS;
    PMI2_Command cmd = { 0 };
    int rc;
    const char *errmsg;

    /* ignoring infokey functionality for now */
    pmi_errno = PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, NAMEPUBLISH_CMD,
                                           NAME_KEY, service_name, PORT_KEY, port,
                                           INFOKEYCOUNT_KEY, "0", NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, NAMEPUBLISHRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_nameservpublish %s", errmsg ? errmsg : "unknown");


  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}


PMI_API_PUBLIC
    int PMI2_Nameserv_lookup(const char service_name[], const PMI2_keyval_t * info_ptr,
                             char port[], int portLen)
{
    int pmi_errno = PMI2_SUCCESS;
    int found;
    int rc;
    PMI2_Command cmd = { 0 };
    int plen;
    const char *errmsg;
    const char *found_port;

    /* ignoring infos for now */
    pmi_errno = PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, NAMELOOKUP_CMD,
                                           NAME_KEY, service_name, INFOKEYCOUNT_KEY, "0", NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, NAMELOOKUPRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_nameservlookup %s", errmsg ? errmsg : "unknown");

    found = getval(cmd.pairs, cmd.nPairs, PORT_KEY, &found_port, &plen);
    PMIU_ERR_CHKANDJUMP1(!found, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_nameservlookup %s", "not found");
    MPL_strncpy(port, found_port, portLen);

  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

PMI_API_PUBLIC
    int PMI2_Nameserv_unpublish(const char service_name[], const PMI2_keyval_t * info_ptr)
{
    int pmi_errno = PMI2_SUCCESS;
    int rc;
    PMI2_Command cmd = { 0 };
    const char *errmsg;

    pmi_errno = PMIi_WriteSimpleCommandStr(PMI2_fd, &cmd, NAMEUNPUBLISH_CMD,
                                           NAME_KEY, service_name, INFOKEYCOUNT_KEY, "0", NULL);
    PMIU_ERR_POP(pmi_errno);
    pmi_errno = PMIi_ReadCommandExp(PMI2_fd, &cmd, NAMEUNPUBLISHRESP_CMD, &rc, &errmsg);
    PMIU_ERR_POP(pmi_errno);
    PMIU_ERR_CHKANDJUMP1(rc, pmi_errno, PMI2_ERR_OTHER,
                         "**pmi2_nameservunpublish %s", errmsg ? errmsg : "unknown");

  fn_exit:
    PMIU_Free(cmd.command);
    freepairs(cmd.pairs, cmd.nPairs);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}


/* ------------------------------------------------------------------------- */
/* Service routines */
/*
 * PMIi_ReadCommand - Reads an entire command from the PMI socket.  This
 * routine blocks the thread until the command is read.
 *
 * PMIi_WriteSimpleCommand - Write a simple command to the PMI socket; this
 * allows printf - style arguments.  This blocks the thread until the buffer
 * has been written (for fault-tolerance, we may want to keep it around
 * in case of PMI failure).
 *
 * PMIi_WaitFor - Wait for a particular PMI command request to complete.
 * In a multithreaded environment, this may
 */
/* ------------------------------------------------------------------------- */


/* frees all of the keyvals pointed to by a keyvalpair* array and the array itself*/
static void freepairs(PMI2_Keyvalpair ** pairs, int npairs)
{
    int i;

    if (!pairs)
        return;

    for (i = 0; i < npairs; ++i)
        if (pairs[i]->isCopy) {
            /* FIXME casts are here to suppress legitimate constness warnings */
            PMIU_Free((void *) pairs[i]->key);
            PMIU_Free((void *) pairs[i]->value);
            PMIU_Free(pairs[i]);
        }
    PMIU_Free(pairs);
}

/* getval & friends -- these functions search the pairs list for a
 * matching key, set val appropriately and return 1.  If no matching
 * key is found, 0 is returned.  If the value is invalid, -1 is returned */

static int getval(PMI2_Keyvalpair * const pairs[], int npairs, const char *key, const char **value,
                  int *vallen)
{
    int i;

    for (i = 0; i < npairs; ++i)
        if (strncmp(key, pairs[i]->key, PMI2_MAX_KEYLEN) == 0) {
            *value = pairs[i]->value;
            *vallen = pairs[i]->valueLen;
            return 1;
        }
    return 0;
}

static int getvalint(PMI2_Keyvalpair * const pairs[], int npairs, const char *key, int *val)
{
    int found;
    const char *value;
    int vallen;
    int ret;
    /* char *endptr; */

    found = getval(pairs, npairs, key, &value, &vallen);
    if (found != 1)
        return found;

    if (vallen == 0)
        return -1;

    ret = sscanf(value, "%d", val);
    if (ret != 1)
        return -1;

    /* *val = strtoll(value, &endptr, 0); */
    /* if (endptr - value != vallen) */
    /*     return -1; */

    return 1;
}

static int getvalptr(PMI2_Keyvalpair * const pairs[], int npairs, const char *key, void *val)
{
    int found;
    const char *value;
    int vallen;
    int ret;
    void **val_ = val;
    /* char *endptr; */

    found = getval(pairs, npairs, key, &value, &vallen);
    if (found != 1)
        return found;

    if (vallen == 0)
        return -1;

    ret = sscanf(value, "%p", val_);
    if (ret != 1)
        return -1;

    /* *val_ = (void *)(PMI2R_Upint)strtoll(value, &endptr, 0); */
    /* if (endptr - value != vallen) */
    /*     return -1; */

    return 1;
}


static int getvalbool(PMI2_Keyvalpair * const pairs[], int npairs, const char *key, int *val)
{
    int found;
    const char *value;
    int vallen;


    found = getval(pairs, npairs, key, &value, &vallen);
    if (found != 1)
        return found;

    if (strlen("TRUE") == vallen && !strncmp(value, "TRUE", vallen))
        *val = PMIU_TRUE;
    else if (strlen("FALSE") == vallen && !strncmp(value, "FALSE", vallen))
        *val = PMIU_FALSE;
    else
        return -1;

    return 1;
}



/* parse_keyval(cmdptr, len, key, val, vallen)
   Scans through buffer specified by cmdptr looking for the first key and value.
     IN/OUT cmdptr - IN: pointer to buffer; OUT: pointer to byte after the ';' terminating the value
     IN/OUT len    - IN: length of buffer; OUT: length of buffer not read
     OUT    key    - pointer to null-terminated string containing the key
     OUT    val    - pointer to string containing the value
     OUT    vallen - length of the value string

   This function will modify the buffer passed through cmdptr to
   insert '\0' following the key, and to replace escaped ';;' with
   ';'.
 */
static int parse_keyval(char **cmdptr, int *len, char **key, char **val, int *vallen)
{
    int pmi_errno = PMI2_SUCCESS;
    char *c = *cmdptr;
    char *d;


    /* find key */
    *key = c;   /* key is at the start of the buffer */
    while (*len && *c != '=') {
        --*len;
        ++c;
    }
    PMIU_ERR_CHKANDJUMP(*len == 0, pmi_errno, PMI2_ERR_OTHER, "**bad_keyval");
    PMIU_ERR_CHKANDJUMP(c - *key > PMI2_MAX_KEYLEN, pmi_errno, PMI2_ERR_OTHER, "**bad_keyval");
    *c = '\0';  /* terminate the key string */

    /* skip over the '=' */
    --*len;
    ++c;

    /* find val */
    *val = d = c;       /* val is next */
    while (*len) {
        if (*c == ';') {        /* handle escaped ';' */
            if (*(c + 1) != ';')
                break;
            else {
                --*len;
                ++c;
            }
        }
        --*len;
        *(d++) = *(c++);
    }
    PMIU_ERR_CHKANDJUMP(*len == 0, pmi_errno, PMI2_ERR_OTHER, "**bad_keyval");
    PMIU_ERR_CHKANDJUMP(d - *val > PMI2_MAX_VALLEN, pmi_errno, PMI2_ERR_OTHER, "**bad_keyval");
    *vallen = d - *val;

    *cmdptr = c + 1;    /* skip over the ';' */
    --*len;

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

static int create_keyval(PMI2_Keyvalpair ** kv, const char *key, const char *val, int vallen)
{
    int pmi_errno = PMI2_SUCCESS;
    char *key_p = NULL;
    char *value_p = NULL;

    PMIU_CHK_MALLOC(*kv, PMI2_Keyvalpair *, sizeof(PMI2_Keyvalpair),
                    pmi_errno, PMI2_ERR_NOMEM, "pair");

    PMIU_CHK_MALLOC(key_p, char *, strlen(key) + 1, pmi_errno, PMI2_ERR_NOMEM, "key");
    MPL_strncpy(key_p, key, PMI2_MAX_KEYLEN + 1);

    PMIU_CHK_MALLOC(value_p, char *, vallen + 1, pmi_errno, PMI2_ERR_NOMEM, "value");
    PMIU_Memcpy(value_p, val, vallen);
    value_p[vallen] = '\0';

    (*kv)->key = key_p;
    (*kv)->value = value_p;
    (*kv)->valueLen = vallen;
    (*kv)->isCopy = PMIU_TRUE;

  fn_exit:
    return pmi_errno;
  fn_fail:
    PMIU_Free(*kv);
    PMIU_Free(key_p);
    PMIU_Free(value_p);
    goto fn_exit;
}


/* Note that we fill in the fields in a command that is provided.
   We may want to share these routines with the PMI version 2 server */
int PMIi_ReadCommand(int fd, PMI2_Command * cmd)
{
    int pmi_errno = PMI2_SUCCESS, err;
    char cmd_len_str[PMII_COMMANDLEN_SIZE + 1];
    int cmd_len, remaining_len, vallen = 0;
    char *c, *cmd_buf = NULL;
    char *key, *val = NULL;
    ssize_t nbytes;
    ssize_t offset;
    int num_pairs;
    int pair_index;
    char *command = NULL;
    int nPairs;
    int found;
    PMI2_Keyvalpair **pairs = NULL;
    PMI2_Command *target_cmd;

    memset(cmd_len_str, 0, sizeof(cmd_len_str));

    if (PMI2_is_threaded) {
#ifdef HAVE_THREADS
        MPL_thread_mutex_lock(&mutex, &err, MPL_THREAD_PRIO_HIGH);

        while (blocked && !cmd->complete)
            MPL_thread_cond_wait(&cond, &mutex, &err);

        if (cmd->complete) {
            MPL_thread_mutex_unlock(&mutex, &err);
            goto fn_exit;
        }

        blocked = PMIU_TRUE;
        MPL_thread_mutex_unlock(&mutex, &err);
#else
        PMIU_Assert(0);
#endif
    }

    do {

        /* get length of cmd */
        offset = 0;
        do {
            do {
                nbytes = read(fd, &cmd_len_str[offset], PMII_COMMANDLEN_SIZE - offset);
            } while (nbytes == -1 && errno == EINTR);

            PMIU_ERR_CHKANDJUMP1(nbytes <= 0, pmi_errno, PMI2_ERR_OTHER,
                                 "**read %s", strerror(errno));

            offset += nbytes;
        }
        while (offset < PMII_COMMANDLEN_SIZE);

        cmd_len = atoi(cmd_len_str);

        cmd_buf = PMIU_Malloc(cmd_len + 1);
        if (!cmd_buf) {
            PMIU_CHKMEM_SETERR(pmi_errno, PMI2_ERR_NOMEM, cmd_len + 1, "cmd_buf");
            goto fn_exit;
        }

        memset(cmd_buf, 0, cmd_len + 1);

        /* get command */
        offset = 0;
        do {
            do {
                nbytes = read(fd, &cmd_buf[offset], cmd_len - offset);
            } while (nbytes == -1 && errno == EINTR);

            PMIU_ERR_CHKANDJUMP1(nbytes <= 0, pmi_errno, PMI2_ERR_OTHER,
                                 "**read %s", strerror(errno));

            offset += nbytes;
        }
        while (offset < cmd_len);

        printf_d("PMI received (cmdlen %d):  %s\n", cmd_len, cmd_buf);

        /* count number of "key=val;" */
        c = cmd_buf;
        remaining_len = cmd_len;
        num_pairs = 0;

        while (remaining_len) {
            while (remaining_len && *c != ';') {
                --remaining_len;
                ++c;
            }
            if (*c == ';' && *(c + 1) == ';') {
                remaining_len -= 2;
                c += 2;
            } else {
                ++num_pairs;
                --remaining_len;
                ++c;
            }
        }

        c = cmd_buf;
        remaining_len = cmd_len;
        pmi_errno = parse_keyval(&c, &remaining_len, &key, &val, &vallen);
        PMIU_ERR_POP(pmi_errno);

        PMIU_ERR_CHKANDJUMP(strncmp(key, "cmd", PMI2_MAX_KEYLEN) != 0, pmi_errno, PMI2_ERR_OTHER,
                            "**bad_cmd");

        command = PMIU_Malloc(vallen + 1);
        if (!command) {
            PMIU_CHKMEM_SETERR(pmi_errno, PMI2_ERR_NOMEM, vallen + 1, "command");
            goto fn_exit;
        }
        PMIU_Memcpy(command, val, vallen);
        val[vallen] = '\0';

        nPairs = num_pairs - 1; /* num_pairs-1 because the first pair is the command */

        pairs = PMIU_Malloc(sizeof(PMI2_Keyvalpair *) * nPairs);
        if (!pairs) {
            PMIU_CHKMEM_SETERR(pmi_errno, PMI2_ERR_NOMEM, sizeof(PMI2_Keyvalpair *) * nPairs,
                               "pairs");
            goto fn_exit;
        }

        pair_index = 0;
        while (remaining_len) {
            PMI2_Keyvalpair *pair;

            pmi_errno = parse_keyval(&c, &remaining_len, &key, &val, &vallen);
            PMIU_ERR_POP(pmi_errno);

            pmi_errno = create_keyval(&pair, key, val, vallen);
            PMIU_ERR_POP(pmi_errno);

            pairs[pair_index] = pair;
            ++pair_index;
        }

        found = getvalptr(pairs, nPairs, THRID_KEY, &target_cmd);
        if (!found)     /* if there's no thrid specified, assume it's for you */
            target_cmd = cmd;
        else if (PMI2_debug && SEARCH_REMOVE(target_cmd) == 0) {
            int i;

            printf("command=%s\n", command);
            for (i = 0; i < nPairs; ++i)
                dump_PMI2_Keyvalpair(stdout, pairs[i]);
        }

        target_cmd->command = command;
        target_cmd->nPairs = nPairs;
        target_cmd->pairs = pairs;
        target_cmd->complete = PMIU_TRUE;

        PMIU_Free(cmd_buf);
    } while (!cmd->complete);

    if (PMI2_is_threaded) {
#ifdef HAVE_THREADS
        MPL_thread_mutex_lock(&mutex, &err, MPL_THREAD_PRIO_HIGH);
        blocked = PMIU_FALSE;
        MPL_thread_cond_broadcast(&cond, &err);
        MPL_thread_mutex_unlock(&mutex, &err);
#else
        PMIU_Assert(0);
#endif
    }

  fn_exit:
    return pmi_errno;
  fn_fail:
    PMIU_Free(cmd_buf);
    goto fn_exit;
}

/* PMIi_ReadCommandExp -- reads a command checks that it matches the
 * expected command string exp, and parses the return code */
int PMIi_ReadCommandExp(int fd, PMI2_Command * cmd, const char *exp, int *rc, const char **errmsg)
{
    int pmi_errno = PMI2_SUCCESS;
    int found;
    int msglen;

    pmi_errno = PMIi_ReadCommand(fd, cmd);
    PMIU_ERR_POP(pmi_errno);

    PMIU_ERR_CHKANDJUMP(strncmp(cmd->command, exp, strlen(exp)) != 0, pmi_errno, PMI2_ERR_OTHER,
                        "**bad_cmd");

    found = getvalint(cmd->pairs, cmd->nPairs, RC_KEY, rc);
    PMIU_ERR_CHKANDJUMP(found != 1, pmi_errno, PMI2_ERR_OTHER, "**intern");

    found = getval(cmd->pairs, cmd->nPairs, ERRMSG_KEY, errmsg, &msglen);
    PMIU_ERR_CHKANDJUMP(found == -1, pmi_errno, PMI2_ERR_OTHER, "**intern");

    if (!found)
        *errmsg = NULL;


  fn_exit:
    return pmi_errno;
  fn_fail:

    goto fn_exit;
}


int PMIi_WriteSimpleCommand(int fd, PMI2_Command * resp, const char cmd[],
                            PMI2_Keyvalpair * pairs[], int npairs)
{
    int pmi_errno = PMI2_SUCCESS, err;
    char cmdbuf[PMII_MAX_COMMAND_LEN];
    char cmdlenbuf[PMII_COMMANDLEN_SIZE + 1];
    char *c = cmdbuf;
    int ret;
    int remaining_len = PMII_MAX_COMMAND_LEN - PMII_COMMANDLEN_SIZE;
    int cmdlen;
    int i;
    ssize_t nbytes;
    ssize_t offset;
    int pair_index;

    /* leave space for length field */
    memset(c, ' ', PMII_COMMANDLEN_SIZE);
    c += PMII_COMMANDLEN_SIZE;

    PMIU_ERR_CHKANDJUMP(strlen(cmd) > PMI2_MAX_VALLEN, pmi_errno, PMI2_ERR_OTHER, "**cmd_too_long");

    ret = MPL_snprintf(c, remaining_len, "cmd=%s;", cmd);
    PMIU_ERR_CHKANDJUMP1(ret >= remaining_len, pmi_errno, PMI2_ERR_OTHER,
                         "**intern %s", "Ran out of room for command");
    c += ret;
    remaining_len -= ret;

    if (PMI2_is_threaded && resp) {
        ret = MPL_snprintf(c, remaining_len, "thrid=%p;", resp);
        PMIU_ERR_CHKANDJUMP1(ret >= remaining_len, pmi_errno, PMI2_ERR_OTHER,
                             "**intern %s", "Ran out of room for command");
        c += ret;
        remaining_len -= ret;
    }

    for (pair_index = 0; pair_index < npairs; ++pair_index) {
        /* write key= */
        PMIU_ERR_CHKANDJUMP(strlen(pairs[pair_index]->key) > PMI2_MAX_KEYLEN, pmi_errno,
                            PMI2_ERR_OTHER, "**key_too_long");
        ret = MPL_snprintf(c, remaining_len, "%s=", pairs[pair_index]->key);
        PMIU_ERR_CHKANDJUMP1(ret >= remaining_len, pmi_errno, PMI2_ERR_OTHER,
                             "**intern %s", "Ran out of room for command");
        c += ret;
        remaining_len -= ret;

        /* write value and escape ;'s as ;; */
        PMIU_ERR_CHKANDJUMP(pairs[pair_index]->valueLen > PMI2_MAX_VALLEN, pmi_errno,
                            PMI2_ERR_OTHER, "**val_too_long");
        for (i = 0; i < pairs[pair_index]->valueLen; ++i) {
            if (pairs[pair_index]->value[i] == ';') {
                *c = ';';
                ++c;
                --remaining_len;
            }
            *c = pairs[pair_index]->value[i];
            ++c;
            --remaining_len;
        }

        /* append ; */
        *c = ';';
        ++c;
        --remaining_len;
    }

    /* prepend the buffer length stripping off the trailing '\0' */
    cmdlen = PMII_MAX_COMMAND_LEN - remaining_len;
    ret = MPL_snprintf(cmdlenbuf, sizeof(cmdlenbuf), "%d", cmdlen);
    PMIU_ERR_CHKANDJUMP1(ret >= PMII_COMMANDLEN_SIZE, pmi_errno, PMI2_ERR_OTHER,
                         "**intern %s", "Command length won't fit in length buffer");

    PMIU_Memcpy(cmdbuf, cmdlenbuf, ret);

    cmdbuf[cmdlen + PMII_COMMANDLEN_SIZE] = '\0';       /* silence valgrind warnings in printf_d */
    printf_d("PMI sending: %s\n", cmdbuf);


    if (PMI2_is_threaded) {
#ifdef HAVE_THREADS
        MPL_thread_mutex_lock(&mutex, &err, MPL_THREAD_PRIO_HIGH);

        while (blocked)
            MPL_thread_cond_wait(&cond, &mutex, &err);

        blocked = PMIU_TRUE;
        MPL_thread_mutex_unlock(&mutex, &err);
#else
        PMIU_Assert(0);
#endif
    }

    if (PMI2_debug)
        ENQUEUE(resp);

    offset = 0;
    do {
        do {
            nbytes = write(fd, &cmdbuf[offset], cmdlen + PMII_COMMANDLEN_SIZE - offset);
        } while (nbytes == -1 && errno == EINTR);

        PMIU_ERR_CHKANDJUMP1(nbytes <= 0, pmi_errno, PMI2_ERR_OTHER, "**write %s", strerror(errno));

        offset += nbytes;
    } while (offset < cmdlen + PMII_COMMANDLEN_SIZE);

    if (PMI2_is_threaded) {
#ifdef HAVE_THREADS
        MPL_thread_mutex_lock(&mutex, &err, MPL_THREAD_PRIO_HIGH);
        blocked = PMIU_FALSE;
        MPL_thread_cond_broadcast(&cond, &err);
        MPL_thread_mutex_unlock(&mutex, &err);
#else
        PMIU_Assert(0);
#endif
    }

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

int PMIi_WriteSimpleCommandStr(int fd, PMI2_Command * resp, const char cmd[], ...)
{
    int pmi_errno = PMI2_SUCCESS;
    va_list ap;
    PMI2_Keyvalpair *pairs = NULL;
    PMI2_Keyvalpair **pairs_p = NULL;
    int npairs;
    int i;
    const char *key;
    const char *val;

    npairs = 0;
    va_start(ap, cmd);
    while ((key = va_arg(ap, const char *))) {
        val = va_arg(ap, const char *);

        ++npairs;
    }
    va_end(ap);

    PMIU_CHK_MALLOC(pairs, PMI2_Keyvalpair *, sizeof(PMI2_Keyvalpair) * npairs,
                    pmi_errno, PMI2_ERR_NOMEM, "pairs");
    PMIU_CHK_MALLOC(pairs_p, PMI2_Keyvalpair **, sizeof(PMI2_Keyvalpair *) * npairs,
                    pmi_errno, PMI2_ERR_NOMEM, "pairs_p");

    i = 0;
    va_start(ap, cmd);
    while ((key = va_arg(ap, const char *))) {
        val = va_arg(ap, const char *);
        pairs_p[i] = &pairs[i];
        pairs[i].key = key;
        pairs[i].value = val;
        if (val)
            pairs[i].valueLen = strlen(val);
        else
            pairs[i].valueLen = 0;
        pairs[i].isCopy = PMIU_FALSE;
        ++i;
    }
    va_end(ap);

    pmi_errno = PMIi_WriteSimpleCommand(fd, resp, cmd, pairs_p, npairs);
    PMIU_ERR_POP(pmi_errno);

  fn_exit:
    PMIU_Free(pairs);
    PMIU_Free(pairs_p);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}


/*
 * This code allows a program to contact a host/port for the PMI socket.
 */
#include <errno.h>
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#include <sys/param.h>
#include <sys/socket.h>

/* sockaddr_in (Internet) */
#include <netinet/in.h>
/* TCP_NODELAY */
#include <netinet/tcp.h>

/* sockaddr_un (Unix) */
#include <sys/un.h>

/* defs of gethostbyname */
#include <netdb.h>

/* fcntl, F_GET/SETFL */
#include <fcntl.h>

/* This is really IP!? */
#ifndef TCP
#define TCP 0
#endif

/* stub for connecting to a specified host/port instead of using a
   specified fd inherited from a parent process */
static int PMII_Connect_to_pm(char *hostname, int portnum)
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

/* Get the FD to use for PMI operations.  If a port is used, rather than
   a pre-established FD (i.e., via pipe), this routine will handle the
   initial handshake.
*/
static int getPMIFD(void)
{
    int pmi_errno = PMI2_SUCCESS;
    char *p;

    /* Set the default */
    PMI2_fd = -1;

    p = getenv("PMI_FD");
    if (p) {
        PMI2_fd = atoi(p);
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

        PMIU_ERR_CHKANDJUMP1(*pn != ':', pmi_errno, PMI2_ERR_OTHER, "**pmi2_port %s", p);

        portnum = atoi(pn + 1);
        /* FIXME: Check for valid integer after : */
        /* This routine only gets the fd to use to talk to
         * the process manager. The handshake below is used
         * to setup the initial values */
        PMI2_fd = PMII_Connect_to_pm(hostname, portnum);
        PMIU_ERR_CHKANDJUMP2(PMI2_fd < 0, pmi_errno, PMI2_ERR_OTHER,
                             "**connect_to_pm %s %d", hostname, portnum);
    }

    /* OK to return success for singleton init */

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

static void dump_PMI2_Keyvalpair(FILE * file, PMI2_Keyvalpair * kv)
{
    fprintf(file, "  key      = %s\n", kv->key);
    fprintf(file, "  value    = %s\n", kv->value);
    fprintf(file, "  valueLen = %d\n", kv->valueLen);
    fprintf(file, "  isCopy   = %s\n", kv->isCopy ? "TRUE" : "FALSE");
}
