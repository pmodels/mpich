/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "simple2pmi.h"
#include "simple_pmiutil.h"
#include "pmi2.h"


#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#if defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#define printf_d(x...)  /* printf(x) */

#ifdef USE_PMI_PORT
#ifndef MAXHOSTNAME
#define MAXHOSTNAME 256
#endif
#endif

#define PMII_EXIT_CODE -1

#define PMI_VERSION    2
#define PMI_SUBVERSION 0

#define MAX_INT_STR_LEN 11 /* number of digits in MAX_UINT + 1 */

typedef enum { PMI_UNINITIALIZED = 0, 
               SINGLETON_INIT_BUT_NO_PM = 1,
	       NORMAL_INIT_WITH_PM,
	       SINGLETON_INIT_WITH_PM } PMIState;
static PMIState PMI_initialized = PMI_UNINITIALIZED;

static int PMI_debug = 0;
static int PMI_fd = -1;
static int PMI_size = 1;
static int PMI_rank = 0;

static int PMI_debug_init = 0;    /* Set this to true to debug the init */

#ifdef MPICH_IS_THREADED
static MPID_Thread_mutex_t mutex;
static int blocked = FALSE;
static MPID_Thread_cond_t cond;
#endif

/* init_kv_str -- fills in keyvalpair.  val is required to be a
   null-terminated string.  isCopy is set to FALSE, so caller must
   free key and val memory, if necessary.
*/
static void init_kv_str(PMI_Keyvalpair *kv, const char key[], const char val[])
{
    kv->key = key;
    kv->value = val;
    kv->valueLen = strlen(val);
    kv->isCopy = FALSE;
}


static int getPMIFD(void);
static int PMIi_ReadCommandExp( int fd, PMI_Command *cmd, const char *exp, int* rc, const char **errmsg );
static int PMIi_ReadCommand( int fd, PMI_Command *cmd );

static int PMIi_WriteSimpleCommand( int fd, PMI_Command *resp, const char cmd[], PMI_Keyvalpair *pairs[], int npairs);
static int PMIi_WriteSimpleCommandStr( int fd, PMI_Command *resp, const char cmd[], ...);
static int PMIi_InitIfSingleton(void);

static void freepairs(PMI_Keyvalpair** pairs, int npairs);
static int getval(PMI_Keyvalpair *const pairs[], int npairs, const char *key,  const char **value, int *vallen);
static int getvalint(PMI_Keyvalpair *const pairs[], int npairs, const char *key, int *val);
static int getvalptr(PMI_Keyvalpair *const pairs[], int npairs, const char *key, void *val);
static int getvalbool(PMI_Keyvalpair *const pairs[], int npairs, const char *key, int *val);

static int accept_one_connection(int list_sock);
static int GetResponse(const char request[], const char expectedCmd[], int checkRc);

static void dump_PMI_Command(FILE *file, PMI_Command *cmd);
static void dump_PMI_Keyvalpair(FILE *file, PMI_Keyvalpair *kv);


typedef struct MPIDI_Process
{
    void * my_pg;
    int my_pg_rank;
}
MPIDI_Process_t;
extern MPIDI_Process_t MPIDI_Process;

int PRINT = 0;

typedef struct pending_item 
{
    struct pending_item *next;
    PMI_Command *cmd;
} pending_item_t;

pending_item_t *pendingq_head = NULL;
pending_item_t *pendingq_tail = NULL;

static inline void ENQUEUE(PMI_Command *cmd)
{
    pending_item_t *pi = MPIU_Malloc(sizeof(pending_item_t));

    if (PRINT)
        printf("%d ENQUEUE(%p)\n", MPIDI_Process.my_pg_rank, cmd);

    pi->next = NULL;
    pi->cmd = cmd;

    if (pendingq_head == NULL) {
        pendingq_head = pendingq_tail = pi;
    } else {
        pendingq_tail->next = pi;
        pendingq_tail = pi;
    }
}
        
static inline int SEARCH_REMOVE(PMI_Command *cmd)
{
    pending_item_t *pi, *prev;

    if (PRINT)
        printf("%d SEARCH_REMOVE(%p)\n", MPIDI_Process.my_pg_rank, cmd);

    pi = pendingq_head;
    if (pi->cmd == cmd) {
        pendingq_head = pi->next;
        if (pendingq_head == NULL)
            pendingq_tail = NULL;
        MPIU_Free(pi);
        return 1;
    }
    prev = pi;
    pi = pi->next;
    
    for ( ; pi ; pi = pi->next) {
        if (pi->cmd == cmd) {
            prev->next = pi->next;
            if (prev->next == NULL)
                pendingq_tail = prev;
            MPIU_Free(pi);
            return 1;
        }
    }
    
    printf("%d cmd %p not found in queue\n", MPIDI_Process.my_pg_rank, cmd);

    return 0;
}



/* ------------------------------------------------------------------------- */
/* PMI API Routines */
/* ------------------------------------------------------------------------- */
#undef FUNCNAME
#define FUNCNAME PMI_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Init(int *spawned, int *size, int *rank, int *appnum)
{
    int mpi_errno = MPI_SUCCESS;
    char *p;
    char buf[PMIU_MAXLINE], cmdline[PMIU_MAXLINE];
    char *jobid;
    char *pmiid;
    int ret;

    /* FIXME: Why is setvbuf commented out? */
    /* FIXME: What if the output should be fully buffered (directed to file)?
       unbuffered (user explicitly set?) */
    /* setvbuf(stdout,0,_IONBF,0); */
    setbuf(stdout, NULL);
    /* PMIU_printf( 1, "PMI_INIT\n" ); */

    /* Get the value of PMI_DEBUG from the environment if possible, since
       we may have set it to help debug the setup process */
    p = getenv( "PMI_DEBUG" );
    if (p)
        PMI_debug = atoi( p );

    /* Get the fd for PMI commands; if none, we're a singleton */
    mpi_errno = getPMIFD();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if ( PMI_fd == -1 ) {
	/* Singleton init: Process not started with mpiexec, 
	   so set size to 1, rank to 0 */
	PMI_size = 1;
	PMI_rank = 0;
	*spawned = 0;
	
	PMI_initialized = SINGLETON_INIT_BUT_NO_PM;
	
        goto fn_exit;
    }

    /* do initial PMI1 init */
    ret = MPIU_Snprintf( buf, PMIU_MAXLINE, "cmd=init pmi_version=%d pmi_subversion=%d\n", PMI_VERSION, PMI_SUBVERSION );
    MPIU_ERR_CHKANDJUMP1(ret < 0, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "failed to generate init line");

    ret = PMIU_writeline(PMI_fd, buf);
    MPIU_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**pmi_init_send");

    ret = PMIU_readline(PMI_fd, buf, PMIU_MAXLINE);
    MPIU_ERR_CHKANDJUMP1(ret < 0, mpi_errno, MPI_ERR_OTHER, "**pmi_initack", "**pmi_initack %s", strerror(errno));

    PMIU_parse_keyvals( buf );
    cmdline[0] = 0;
    PMIU_getval( "cmd", cmdline, PMIU_MAXLINE );
    MPIU_ERR_CHKANDJUMP(strncmp( cmdline, "response_to_init", PMIU_MAXLINE ) != 0,  mpi_errno, MPI_ERR_OTHER, "**bad_cmd");

    PMIU_getval( "rc", buf, PMIU_MAXLINE );
    if ( strncmp( buf, "0", PMIU_MAXLINE ) != 0 ) {
	char buf1[PMIU_MAXLINE];
        PMIU_getval( "pmi_version", buf, PMIU_MAXLINE );
        PMIU_getval( "pmi_subversion", buf1, PMIU_MAXLINE );
        MPIU_ERR_SETANDJUMP4(mpi_errno, MPI_ERR_OTHER, "**pmi_version", "**pmi_version %s %s %s %s", buf, buf1, PMI_VERSION, PMI_SUBVERSION);
    }

    /* do full PMI2 init */
    {
        PMI_Keyvalpair pairs[3];
        PMI_Keyvalpair *pairs_p[] = { pairs, pairs+1, pairs+2 };
        int npairs = 0;
        int isThreaded = 0;
        const char *errmsg;
        int rc;
        int found;
        int version, subver;
        int spawner_jobid;
        PMI_Command cmd = {0};
        int debugged, pmiverbose;
        

        jobid = getenv("PMI_JOBID");
        if (jobid) {
            init_kv_str(&pairs[npairs], PMIJOBID_KEY, jobid);
            ++npairs;
        }
        
        pmiid = getenv("PMI_ID");
        if (pmiid) {
            init_kv_str(&pairs[npairs], PMIRANK_KEY, pmiid);
            ++npairs;
        }

#ifdef MPICH_IS_THREADED
        MPIU_THREAD_CHECK_BEGIN;
        {
            isThreaded = 1;
        }
        MPIU_THREAD_CHECK_END;
#endif
        init_kv_str(&pairs[npairs], THREADED_KEY, isThreaded ? "TRUE" : "FALSE");
        ++npairs;
        
 
        mpi_errno = PMIi_WriteSimpleCommand(PMI_fd, 0, FULLINIT_CMD, pairs_p, npairs); /* don't pass in thread id for init */
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* Read auth-response */
        /* Send auth-response-complete */
    
        /* Read fullinit-response */
        mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, FULLINITRESP_CMD, &rc, &errmsg);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_fullinit", "**pmi_fullinit %s", errmsg ? errmsg : "unknown");

        found = getvalint(cmd.pairs, cmd.nPairs, PMIVERSION_KEY, &version);
        MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

        found = getvalint(cmd.pairs, cmd.nPairs, PMISUBVER_KEY, &subver);
        MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

        found = getvalint(cmd.pairs, cmd.nPairs, RANK_KEY, rank);
        MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

        found = getvalint(cmd.pairs, cmd.nPairs, SIZE_KEY, size);
        MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

        found = getvalint(cmd.pairs, cmd.nPairs, APPNUM_KEY, appnum);
        MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

        found = getvalint(cmd.pairs, cmd.nPairs, SPAWNERJOBID_KEY, &spawner_jobid);
        MPIU_ERR_CHKANDJUMP(found == -1, mpi_errno, MPI_ERR_OTHER, "**intern");
        if (found)
            *spawned = TRUE;
        else
            *spawned = FALSE;

        debugged = 0;
        found = getvalbool(cmd.pairs, cmd.nPairs, DEBUGGED_KEY, &debugged);
        MPIU_ERR_CHKANDJUMP(found == -1, mpi_errno, MPI_ERR_OTHER, "**intern");
        PMI_debug |= debugged;
        
        pmiverbose = 0;
        found = getvalint(cmd.pairs, cmd.nPairs, PMIVERBOSE_KEY, &pmiverbose);
        MPIU_ERR_CHKANDJUMP(found == -1, mpi_errno, MPI_ERR_OTHER, "**intern");
        
        freepairs(cmd.pairs, cmd.nPairs);
    }
    
    if ( ! PMI_initialized )
	PMI_initialized = NORMAL_INIT_WITH_PM;    
        
fn_exit:
    return mpi_errno;
fn_fail:

    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMI_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    const char *errmsg;
    PMI_Command cmd = {0};
   
    if ( PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, FINALIZE_CMD, NULL);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, FINALIZERESP_CMD, &rc, &errmsg);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_finalize", "**pmi_finalize %s", errmsg ? errmsg : "unknown");
        freepairs(cmd.pairs, cmd.nPairs);
        
	shutdown( PMI_fd, SHUT_RDWR );
	close( PMI_fd );
    }

fn_exit:
    return mpi_errno;
fn_fail:

    goto fn_exit;
}

int PMI_Initialized(void)
{
    /* Turn this into a logical value (1 or 0) .  This allows us
       to use PMI_initialized to distinguish between initialized with
       an PMI service (e.g., via mpiexec) and the singleton init, 
       which has no PMI service */
    return PMI_initialized != 0;
}

int PMI_Abort( int flag, const char msg[] )
{
    PMIU_printf(1, "aborting job:\n%s\n", msg);

    /* ignoring return code, because we're exiting anyway */
    PMIi_WriteSimpleCommandStr(PMI_fd, NULL, ABORT_CMD, ISWORLD_KEY, flag ? TRUE_VAL : FALSE_VAL, MSG_KEY, msg, NULL);
    
    MPIU_Exit(PMII_EXIT_CODE);
    return MPI_SUCCESS;
}

int PMI_Job_Spawn( int count, const char * cmds[], const char ** argvs[],
                   const int maxprocs[], 
                   const int info_keyval_sizes[],
                   const MPID_Info *info_keyval_vectors[],
                   int preput_keyval_size,
                   const MPID_Info *preput_keyval_vector[],
                   char jobId[], int jobIdSize,      
                   int errors[])
{
#if 0
    int  i,rc,argcnt,spawncnt,total_num_processes,num_errcodes_found;
    char buf[PMIU_MAXLINE], tempbuf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    char *lead, *lag;

    /* Connect to the PM if we haven't already */
    if (PMIi_InitIfSingleton() != 0) return -1;

    total_num_processes = 0;

    for (spawncnt=0; spawncnt < count; spawncnt++)
    {
        total_num_processes += maxprocs[spawncnt];

        rc = MPIU_Snprintf(buf, PMIU_MAXLINE, 
			   "mcmd=spawn\nnprocs=%d\nexecname=%s\n",
			   maxprocs[spawncnt], cmds[spawncnt] );
	if (rc < 0) {
	    return PMI_FAIL;
	}

	rc = MPIU_Snprintf(tempbuf, PMIU_MAXLINE,
			   "totspawns=%d\nspawnssofar=%d\n",
			   count, spawncnt+1);

	if (rc < 0) { 
	    return PMI_FAIL;
	}
	rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
	if (rc != 0) {
	    return PMI_FAIL;
	}

        argcnt = 0;
        if ((argvs != NULL) && (argvs[spawncnt] != NULL)) {
            for (i=0; argvs[spawncnt][i] != NULL; i++)
            {
		/* FIXME (protocol design flaw): command line arguments
		   may contain both = and <space> (and even tab!).
		*/
		/* Note that part of this fixme was really a design error -
		   because this uses the mcmd form, the data can be
		   sent in multiple writelines.  This code now takes 
		   advantage of that.  Note also that a correct parser 
		   of the commands will permit any character other than a 
		   new line in the argument, since the form is 
		   argn=<any nonnewline><newline> */
                rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"arg%d=%s\n",
				   i+1,argvs[spawncnt][i]);
		if (rc < 0) {
		    return PMI_FAIL;
		}
                rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
		if (rc != 0) {
		    return PMI_FAIL;
		}
                argcnt++;
		rc = PMIU_writeline( PMI_fd, buf );
		buf[0] = 0;

            }
        }
        rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"argcnt=%d\n",argcnt);
	if (rc < 0) {
	    return PMI_FAIL;
	}
        rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
	if (rc != 0) {
	    return PMI_FAIL;
	}
    
        rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"preput_num=%d\n", 
			   preput_keyval_size);
	if (rc < 0) {
	    return PMI_FAIL;
	}

        rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
	if (rc != 0) {
	    return PMI_FAIL;
	}
        for (i=0; i < preput_keyval_size; i++) {
	    rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"preput_key_%d=%s\n",
			       i,preput_keyval_vector[i].key);
	    if (rc < 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
	    if (rc != 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"preput_val_%d=%s\n",
			       i,preput_keyval_vector[i].val);
	    if (rc < 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
	    if (rc != 0) {
		return PMI_FAIL;
	    }
        } 
        rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"info_num=%d\n", 
			   info_keyval_sizes[spawncnt]);
	if (rc < 0) {
	    return PMI_FAIL;
	}
        rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
	if (rc != 0) {
	    return PMI_FAIL;
	}
	for (i=0; i < info_keyval_sizes[spawncnt]; i++)
	{
	    rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"info_key_%d=%s\n",
			       i,info_keyval_vectors[spawncnt][i].key);
	    if (rc < 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
	    if (rc != 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"info_val_%d=%s\n",
			       i,info_keyval_vectors[spawncnt][i].val);
	    if (rc < 0) {
		return PMI_FAIL;
	    }
	    rc = MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
	    if (rc != 0) {
		return PMI_FAIL;
	    }
	}

        rc = MPIU_Strnapp(buf, "endcmd\n", PMIU_MAXLINE);
	if (rc != 0) {
	    return PMI_FAIL;
	}
        PMIU_writeline( PMI_fd, buf );
    }

    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf ); 
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "spawn_result", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to spawn :%s:\n", buf );
	return( -1 );
    }
    else {
	PMIU_getval( "rc", buf, PMIU_MAXLINE );
	rc = atoi( buf );
	if ( rc != 0 ) {
	    /****
	    PMIU_getval( "status", tempbuf, PMIU_MAXLINE );
	    PMIU_printf( 1, "pmi_spawn_mult failed; status: %s\n",tempbuf);
	    ****/
	    return( -1 );
	}
    }
    
    PMIU_Assert(errors != NULL);
    if (PMIU_getval( "errcodes", tempbuf, PMIU_MAXLINE )) {
        num_errcodes_found = 0;
        lag = &tempbuf[0];
        do {
            lead = strchr(lag, ',');
            if (lead) *lead = '\0';
            errors[num_errcodes_found++] = atoi(lag);
            lag = lead + 1; /* move past the null char */
            PMIU_Assert(num_errcodes_found <= total_num_processes);
        } while (lead != NULL);
        PMIU_Assert(num_errcodes_found == total_num_processes);
    }
    else {
        /* gforker doesn't return errcodes, so we'll just pretend that means
           that it was going to send all `0's. */
        for (i = 0; i < total_num_processes; ++i) {
            errors[i] = 0;
        }
    }

#endif
    return 0;
}

#undef FUNCNAME
#define FUNCNAME PMI_Job_GetId
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Job_GetId(char jobid[], int jobid_size)
{
    int mpi_errno = MPI_SUCCESS;
    int found;
    const char *jid;
    int jidlen;
    int rc;
    const char *errmsg;
    PMI_Command cmd = {0};
    
    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, JOBGETID_CMD, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, JOBGETIDRESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_jobgetid", "**pmi_jobgetid %s", errmsg ? errmsg : "unknown");

    found = getval(cmd.pairs, cmd.nPairs, JOBID_KEY, &jid, &jidlen);
    MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

    MPIU_Strncpy(jobid, jid, jobid_size);

fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    return mpi_errno;
fn_fail:

    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMI_Job_Connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Job_Connect(const char jobid[], PMI_Connect_comm_t *conn)
{
    int mpi_errno = MPI_SUCCESS;
    PMI_Command cmd = {0};
    int found;
    int kvscopy;
    int rc;
    const char *errmsg;

    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, JOBCONNECT_CMD, JOBID_KEY, jobid, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, JOBCONNECTRESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_jobconnect", "**pmi_jobconnect %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, KVSCOPY_KEY, &kvscopy);
    MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

    MPIU_ERR_CHKANDJUMP(kvscopy, mpi_errno, MPI_ERR_OTHER, "**notimpl");

    
 fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    return mpi_errno;
 fn_fail:

    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMI_Job_Disconnect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Job_Disconnect(const char jobid[])
{
    int mpi_errno = MPI_SUCCESS;
    PMI_Command cmd = {0};
    int rc;
    const char *errmsg;

    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, JOBDISCONNECT_CMD, JOBID_KEY, jobid, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, JOBDISCONNECTRESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_jobdisconnect", "**pmi_jobdisconnect %s", errmsg ? errmsg : "unknown");
        
fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMI_KVS_Put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_KVS_Put(const char key[], const char value[])
{
    int mpi_errno = MPI_SUCCESS;
    PMI_Command cmd = {0};
    int rc;
    const char *errmsg;

    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, KVSPUT_CMD, KEY_KEY, key, VALUE_KEY, value, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, KVSPUTRESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvsput", "**pmi_kvsput %s", errmsg ? errmsg : "unknown");        
        
fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
#undef FUNCNAME
#define FUNCNAME PMI_KVS_Fence
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_KVS_Fence(void)
{
    int mpi_errno = MPI_SUCCESS;
    PMI_Command cmd = {0};
    int rc;
    const char *errmsg;

    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, KVSFENCE_CMD, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, KVSFENCERESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvsfence", "**pmi_kvsfence %s", errmsg ? errmsg : "unknown");

fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMI_KVS_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_KVS_Get(const char *jobid, const char key[], char value [], int maxValue, int *valLen)
{
    int mpi_errno = MPI_SUCCESS;
    int found, keyfound;
    const char *kvsvalue;
    int kvsvallen;
    int ret;
    PMI_Command cmd = {0};
    int rc;
    const char *errmsg;

    mpi_errno = PMIi_InitIfSingleton();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, KVSGET_CMD, JOBID_KEY, jobid, KEY_KEY, key, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, KVSGETRESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvsget", "**pmi_kvsget %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, FOUND_KEY, &keyfound);
    MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");
    MPIU_ERR_CHKANDJUMP(!keyfound, mpi_errno, MPI_ERR_OTHER, "**pmi_kvsget_notfound");

    found = getval(cmd.pairs, cmd.nPairs, VALUE_KEY, &kvsvalue, &kvsvallen);
    MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

    ret = MPIU_Strncpy(value, kvsvalue, maxValue);
    *valLen = ret ? -kvsvallen : kvsvallen;
    

 fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    return mpi_errno;
 fn_fail:

    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMI_Info_GetNodeAttr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Info_GetNodeAttr(const char name[], char value[], int valuelen, int *flag, int waitfor)
{
    int mpi_errno = MPI_SUCCESS;
    int found;
    const char *kvsvalue;
    int kvsvallen;
    PMI_Command cmd = {0};
    int rc;
    const char *errmsg;

    mpi_errno = PMIi_InitIfSingleton();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, GETNODEATTR_CMD, KEY_KEY, name, WAIT_KEY, waitfor ? "TRUE" : "FALSE", NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, GETNODEATTRRESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_getnodeattr", "**pmi_getnodeattr %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, FOUND_KEY, flag);
    MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");
    if (*flag) {
        found = getval(cmd.pairs, cmd.nPairs, VALUE_KEY, &kvsvalue, &kvsvallen);
        MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

        MPIU_Strncpy(value, kvsvalue, valuelen);
    }
    
fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMI_Info_GetNodeAttr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Info_GetNodeAttrIntArray(const char name[], int array[], int arraylen, int *outlen, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    int found;
    const char *kvsvalue;
    int kvsvallen;
    PMI_Command cmd = {0};
    int rc;
    const char *errmsg;
    int i;
    const char *valptr;
    
    mpi_errno = PMIi_InitIfSingleton();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, GETNODEATTR_CMD, KEY_KEY, name, WAIT_KEY, "FALSE", NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, GETNODEATTRRESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_getnodeattr", "**pmi_getnodeattr %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, FOUND_KEY, flag);
    MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");
    if (*flag) {
        found = getval(cmd.pairs, cmd.nPairs, VALUE_KEY, &kvsvalue, &kvsvallen);
        MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

        valptr = kvsvalue;
        i = 0;
        rc = sscanf(valptr, "%d", &array[i]);
        MPIU_ERR_CHKANDJUMP1(rc != 1, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "unable to parse intarray");
        ++i;
        while ((valptr = strchr(valptr, ',')) && i < arraylen) {
            ++valptr; /* skip over the ',' */
            rc = sscanf(valptr, "%d", &array[i]);
            MPIU_ERR_CHKANDJUMP1(rc != 1, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "unable to parse intarray");
            ++i;
        }
        
        *outlen = i;
    }
    
fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMI_Info_PutNodeAttr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Info_PutNodeAttr(const char name[], const char value[])
{
    int mpi_errno = MPI_SUCCESS;
    PMI_Command cmd = {0};
    int rc;
    const char *errmsg;

    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, PUTNODEATTR_CMD, KEY_KEY, name, VALUE_KEY, value, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, PUTNODEATTRRESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_putnodeattr", "**pmi_putnodeattr %s", errmsg ? errmsg : "unknown");
        
fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMI_Info_GetJobAttr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Info_GetJobAttr(const char name[], char value[], int valuelen, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    int found;
    const char *kvsvalue;
    int kvsvallen;
    PMI_Command cmd = {0};
    int rc;
    const char *errmsg;

    mpi_errno = PMIi_InitIfSingleton();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, GETJOBATTR_CMD, KEY_KEY, name, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, GETJOBATTRRESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_getjobattr", "**pmi_getjobattr %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, FOUND_KEY, flag);
    MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

    if (*flag) {
        found = getval(cmd.pairs, cmd.nPairs, VALUE_KEY, &kvsvalue, &kvsvallen);
        MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");
        
        MPIU_Strncpy(value, kvsvalue, valuelen);
    }
    
fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
#undef FUNCNAME
#define FUNCNAME PMI_Info_GetJobAttrIntArray
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Info_GetJobAttrIntArray(const char name[], int array[], int arraylen, int *outlen, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    int found;
    const char *kvsvalue;
    int kvsvallen;
    PMI_Command cmd = {0};
    int rc;
    const char *errmsg;
    int i;
    const char *valptr;
    
    mpi_errno = PMIi_InitIfSingleton();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, GETJOBATTR_CMD, KEY_KEY, name, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, GETJOBATTRRESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_getjobattr", "**pmi_getjobattr %s", errmsg ? errmsg : "unknown");

    found = getvalbool(cmd.pairs, cmd.nPairs, FOUND_KEY, flag);
    MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");
    if (*flag) {
        found = getval(cmd.pairs, cmd.nPairs, VALUE_KEY, &kvsvalue, &kvsvallen);
        MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

        valptr = kvsvalue;
        i = 0;
        rc = sscanf(valptr, "%d", &array[i]);
        MPIU_ERR_CHKANDJUMP1(rc != 1, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "unable to parse intarray");
        ++i;
        while ((valptr = strchr(valptr, ',')) && i < arraylen) {
            ++valptr; /* skip over the ',' */
            rc = sscanf(valptr, "%d", &array[i]);
            MPIU_ERR_CHKANDJUMP1(rc != 1, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "unable to parse intarray");
            ++i;
        }
        
        *outlen = i;
    }
    
fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMI_Nameserv_publish
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Nameserv_publish(const char service_name[], const MPID_Info *info_ptr, const char port[])
{
#if 0
    int mpi_errno = MPI_SUCCESS;
    PMI_Keyvalpair *pairs;
    PMI_Keyvalpair **pair_p;
    int ninfokeys = 0; 
    const MPID_Info *ip = info_ptr;
    int i;
    PMI_Command cmd = {0};
    int rc;
    const char *errmsg;
    char intbuf[MAX_INT_STR_LEN];
    MPIU_CHKLMEM_DECL(2);

    MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**notimpl");
        
    
    while (ip) {
        ++ninfokeys;
        ip = ip->next;
    }
    
    /* MPIU_CHKLMEM_MALLOC(pairs, PMI_Keyvalpair *, (sizeof(PMI_Keyvalpair) * 2 * ninfokeys + 1), mpi_errno, "pairs"); */
    /* MPIU_CHKLMEM_MALLOC(pair_p, PMI_Keyvalpair **, (sizeof(PMI_Keyvalpair*) * 2 * ninfokeys + 1), mpi_errno, "pair_p"); */

    MPIU_Snprintf(intbuf, sizeof(intbuf), "%u", ninfokeys);
    init_kv_str(&pairs[0], INFOKEYCOUNT_KEY, intbuf);        

    mpi_errno = PMIi_WriteSimpleCommandStr(PMI_fd, &cmd, NAMEPUBLISH_CMD, NAME_KEY, service_name, PORT_KEY, port, NULL);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = PMIi_ReadCommandExp(PMI_fd, &cmd, NAMEPUBLISHRESP_CMD, &rc, &errmsg);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP1(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_nameservpublish", "**pmi_nameservpublish %s", errmsg ? errmsg : "unknown");

        
        
fn_exit:
    freepairs(cmd.pairs, cmd.nPairs);
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:

    goto fn_exit;
#endif
    return 0;
}


#undef FUNCNAME
#define FUNCNAME PMI_Nameserv_lookup
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Nameserv_lookup(const char service_name[], const MPID_Info *info_ptr,
                        char port[], int portLen)
{
    int mpi_errno = MPI_SUCCESS;
        
        
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMI_Nameserv_unpublish
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMI_Nameserv_unpublish(const char service_name[], 
                           const MPID_Info *info_ptr)
{
    int mpi_errno = MPI_SUCCESS;
        
        
fn_exit:
    return mpi_errno;
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


/* frees all of the keyvals pointed to by a keyvalpair* array and the array iteself*/
static void freepairs(PMI_Keyvalpair** pairs, int npairs)
{
    int i;

    if (!pairs)
        return;

    for (i = 0; i < npairs; ++i)
        if (pairs[i]->isCopy)
            MPIU_Free(pairs[i]);
    MPIU_Free(pairs);
}


/* getval & friends -- these functions search the pairs list for a
 * matching key, set val appropriately and return 1.  If no matching
 * key is found, 0 is returned.  If the value is invalid, -1 is returned */

#undef FUNCNAME
#define FUNCNAME getval
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int getval(PMI_Keyvalpair *const pairs[], int npairs, const char *key,  const char **value, int *vallen)
{
    int i;
    
    for (i = 0; i < npairs; ++i) 
        if (strncmp(key, pairs[i]->key, PMI_MAX_KEYLEN) == 0) {
            *value = pairs[i]->value;
            *vallen = pairs[i]->valueLen;
            return 1;
        }
    return 0;
}

#undef FUNCNAME
#define FUNCNAME getvalint
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int getvalint(PMI_Keyvalpair *const pairs[], int npairs, const char *key, int *val)
{
    int found;
    const char *value;
    int vallen;
    char *endptr;
    
    found = getval(pairs, npairs, key, &value, &vallen);
    if (found != 1)
        return found;

    if (vallen == 0)
        return -1;

    *val = strtoll(value, &endptr, 0);
    if (endptr - value != vallen)
        return -1;
    
    return 1;
}

#undef FUNCNAME
#define FUNCNAME getvalptr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int getvalptr(PMI_Keyvalpair *const pairs[], int npairs, const char *key, void *val)
{
    int found;
    const char *value;
    int vallen;
    char *endptr;
    void **val_ = val;
    
    found = getval(pairs, npairs, key, &value, &vallen);
    if (found != 1)
        return found;

    if (vallen == 0)
        return -1;

    *val_ = (void *)(unsigned int)strtoll(value, &endptr, 0);
    if (endptr - value != vallen)
        return -1;
    
    return 1;
}


#undef FUNCNAME
#define FUNCNAME getvalbool
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int getvalbool(PMI_Keyvalpair *const pairs[], int npairs, const char *key, int *val)
{
    int found;
    const char *value;
    int vallen;
    
    
    found = getval(pairs, npairs, key, &value, &vallen);
    if (found != 1)
        return found;

    if (strlen("TRUE") == vallen && !strncmp(value, "TRUE", vallen))
        *val = TRUE;
    else if (strlen("FALSE") == vallen && !strncmp(value, "FALSE", vallen))
        *val = FALSE;
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
#undef FUNCNAME
#define FUNCNAME parse_keyval
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int parse_keyval(char **cmdptr, int *len, char **key, char **val, int *vallen)
{
    int mpi_errno = MPI_SUCCESS;
    char *c = *cmdptr;
    char *d;

    
    /* find key */
    *key = c; /* key is at the start of the buffer */
    while (*len && *c != '=') {
        --*len;
        ++c;
    }
    MPIU_ERR_CHKANDJUMP(*len == 0, mpi_errno, MPI_ERR_OTHER, "**bad_keyval");
    MPIU_ERR_CHKANDJUMP(c - *key > PMI_MAX_KEYLEN, mpi_errno, MPI_ERR_OTHER, "**bad_keyval");
    *c = '\0'; /* terminate the key string */

    /* skip over the '=' */
    --*len;
    ++c;

    /* find val */
    *val = d = c; /* val is next */
    while (*len) {
        if (*c == ';') { /* handle escaped ';' */
            if (*(c+1) != ';') 
                break;
            else
            {
                --*len;
                ++c;
            }
        }
        --*len;
        *(d++) = *(c++);
    }
    MPIU_ERR_CHKANDJUMP(*len == 0, mpi_errno, MPI_ERR_OTHER, "**bad_keyval");
    MPIU_ERR_CHKANDJUMP(d - *val > PMI_MAX_VALLEN, mpi_errno, MPI_ERR_OTHER, "**bad_keyval");
    *vallen = d - *val;

    *cmdptr = c+1; /* skip over the ';' */
    --*len;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME create_keyval
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int create_keyval(PMI_Keyvalpair **kv, const char *key, const char *val, int vallen)
{
    int mpi_errno = MPI_SUCCESS;
    char *key_p;
    char *value_p;
    MPIU_CHKPMEM_DECL(3);

    MPIU_CHKPMEM_MALLOC(*kv, PMI_Keyvalpair *, sizeof(PMI_Keyvalpair), mpi_errno, "pair");
        
    MPIU_CHKPMEM_MALLOC(key_p, char *, strlen(key)+1, mpi_errno, "key");
    MPIU_Strncpy(key_p, key, PMI_MAX_KEYLEN+1);
    
    MPIU_CHKPMEM_MALLOC(value_p, char *, vallen+1, mpi_errno, "value");
    memcpy(value_p, val, vallen);
    value_p[vallen] = '\0';
    
    (*kv)->key = key_p;
    (*kv)->value = value_p;
    (*kv)->valueLen = vallen;
    (*kv)->isCopy = TRUE;

fn_exit:
    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


/* Note that we fill in the fields in a command that is provided.
   We may want to share these routines with the PMI version 2 server */
#undef FUNCNAME
#define FUNCNAME PMIi_ReadCommand
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMIi_ReadCommand( int fd, PMI_Command *cmd )
{
    int mpi_errno = MPI_SUCCESS;
    char cmd_len_str[PMII_COMMANDLEN_SIZE+1];
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
    PMI_Keyvalpair **pairs = NULL;
    PMI_Command *target_cmd;
    MPIU_CHKPMEM_DECL(2);

    memset(cmd_len_str, 0, sizeof(cmd_len_str));

#ifdef MPICH_IS_THREADED
    MPIU_THREAD_CHECK_BEGIN;
    {
        MPID_Thread_mutex_lock(&mutex);

        while (blocked && !cmd->complete)
            MPID_Thread_cond_wait(&cond, &mutex);

        if (cmd->complete) {
            MPID_Thread_mutex_unlock(&mutex);
            goto fn_exit;
        }

        blocked = TRUE;
        MPID_Thread_mutex_unlock(&mutex);
    }
    MPIU_THREAD_CHECK_END;
#endif

    do {

        /* get length of cmd */
        offset = 0;
        do 
        {
            do {
                nbytes = read(fd, &cmd_len_str[offset], PMII_COMMANDLEN_SIZE - offset);
            } while (nbytes == -1 && errno == EINTR);

            MPIU_ERR_CHKANDJUMP1(nbytes <= 0, mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror(errno));

            offset += nbytes;
        }
        while (offset < PMII_COMMANDLEN_SIZE);
    
        cmd_len = atoi(cmd_len_str);

        cmd_buf = MPIU_Malloc(cmd_len+1);
        if (!cmd_buf) { MPIU_CHKMEM_SETERR(mpi_errno, cmd_len+1, "cmd_buf"); goto fn_exit; }        

        memset(cmd_buf, 0, cmd_len+1);

        /* get command */
        offset = 0;
        do 
        {
            do {
                nbytes = read(fd, &cmd_buf[offset], cmd_len - offset);
            } while (nbytes == -1 && errno == EINTR);

            MPIU_ERR_CHKANDJUMP1(nbytes <= 0, mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror(errno));

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
            if (*c == ';' && *(c+1) == ';') {
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
        mpi_errno = parse_keyval(&c, &remaining_len, &key, &val, &vallen);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        MPIU_ERR_CHKANDJUMP(strncmp(key, "cmd", PMI_MAX_KEYLEN) != 0, mpi_errno, MPI_ERR_OTHER, "**bad_cmd");
    
        command = MPIU_Malloc(vallen+1);
        if (!command) { MPIU_CHKMEM_SETERR(mpi_errno, vallen+1, "command"); goto fn_exit; }
        memcpy(command, val, vallen);
        val[vallen] = '\0';

        nPairs = num_pairs-1;  /* num_pairs-1 because the first pair is the command */

        pairs = MPIU_Malloc(sizeof(PMI_Keyvalpair *) * nPairs);
        if (!pairs) { MPIU_CHKMEM_SETERR(mpi_errno, sizeof(PMI_Keyvalpair *) * nPairs, "pairs"); goto fn_exit; }
    
        pair_index = 0;
        while (remaining_len)
        {
            PMI_Keyvalpair *pair;
        
            mpi_errno = parse_keyval(&c, &remaining_len, &key, &val, &vallen);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            mpi_errno = create_keyval(&pair, key, val, vallen);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        
            pairs[pair_index] = pair;
            ++pair_index;
        }

        found = getvalptr(pairs, nPairs, THRID_KEY, &target_cmd);
        if (!found) /* if there's no thrid specified, assume it's for you */
            target_cmd = cmd;
        else
            if (PMI_debug && SEARCH_REMOVE(target_cmd) == 0) {
                int i;
                
                printf("command=%s\n", command);
                for (i = 0; i < nPairs; ++i)
                    dump_PMI_Keyvalpair(stdout, pairs[i]);
            }
        
        target_cmd->command = command;
        target_cmd->nPairs = nPairs;
        target_cmd->pairs = pairs;
#ifdef MPICH_IS_THREADED
        target_cmd->complete = TRUE;
#endif

        MPIU_Free(cmd_buf);
    } while (!cmd->complete);
    
#ifdef MPICH_IS_THREADED
    MPIU_THREAD_CHECK_BEGIN;
    {
        MPID_Thread_mutex_lock(&mutex);
        blocked = FALSE;
        MPID_Thread_cond_broadcast(&cond);
        MPID_Thread_mutex_unlock(&mutex);
    }
    MPIU_THREAD_CHECK_END;
#endif


    

fn_exit:
    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
fn_fail:
    MPIU_CHKPMEM_REAP();
    if (cmd_buf)
        MPIU_Free(cmd_buf);
    goto fn_exit;
}

/* PMIi_ReadCommandExp -- reads a command checks that it matches the
 * expected command string exp, and parses the return code */
#undef FUNCNAME
#define FUNCNAME PMIi_ReadCommandExp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMIi_ReadCommandExp( int fd, PMI_Command *cmd, const char *exp, int* rc, const char **errmsg )
{
    int mpi_errno = MPI_SUCCESS;
    int found;
    int msglen;
    
    mpi_errno = PMIi_ReadCommand(fd, cmd);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_ERR_CHKANDJUMP(strncmp(cmd->command, exp, strlen(exp)) != 0,  mpi_errno, MPI_ERR_OTHER, "**bad_cmd");

    found = getvalint(cmd->pairs, cmd->nPairs, RC_KEY, rc);
    MPIU_ERR_CHKANDJUMP(found != 1, mpi_errno, MPI_ERR_OTHER, "**intern");

    found = getval(cmd->pairs, cmd->nPairs, ERRMSG_KEY, errmsg, &msglen);
    MPIU_ERR_CHKANDJUMP(found == -1, mpi_errno, MPI_ERR_OTHER, "**intern");

    if (!found)
        *errmsg = NULL;

    
fn_exit:
    return mpi_errno;
fn_fail:

    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME PMIi_WriteSimpleCommand
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMIi_WriteSimpleCommand( int fd, PMI_Command *resp, const char cmd[], PMI_Keyvalpair *pairs[], int npairs)
{
    int mpi_errno = MPI_SUCCESS;
    char cmdbuf[PMII_MAX_COMMAND_LEN];
    char cmdlenbuf[PMII_COMMANDLEN_SIZE+1];
    char *c = cmdbuf;
    int ret;
    int remaining_len = PMII_MAX_COMMAND_LEN;
    int cmdlen;
    int i;
    ssize_t nbytes;
    ssize_t offset;
    int pair_index;

    /* leave space for length field */
    memset(c, ' ', PMII_COMMANDLEN_SIZE);
    c += PMII_COMMANDLEN_SIZE;

    MPIU_ERR_CHKANDJUMP(strlen(cmd) > PMI_MAX_VALLEN, mpi_errno, MPI_ERR_OTHER, "**cmd_too_long");

    ret = MPIU_Snprintf(c, remaining_len, "cmd=%s;", cmd);
    MPIU_ERR_CHKANDJUMP1(ret >= remaining_len, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "Ran out of room for command");
    c += ret;
    remaining_len -= ret;

#ifdef MPICH_IS_THREADED
    MPIU_THREAD_CHECK_BEGIN;
    if (resp) {
        ret = MPIU_Snprintf(c, remaining_len, "thrid=%p;", resp);
        MPIU_ERR_CHKANDJUMP1(ret >= remaining_len, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "Ran out of room for command");
        c += ret;
        remaining_len -= ret;
    }
    MPIU_THREAD_CHECK_END;
#endif
    
    for (pair_index = 0; pair_index < npairs; ++pair_index) {
        /* write key= */
        MPIU_ERR_CHKANDJUMP(strlen(pairs[pair_index]->key) > PMI_MAX_KEYLEN, mpi_errno, MPI_ERR_OTHER, "**key_too_long");
        ret = MPIU_Snprintf(c, remaining_len, "%s=", pairs[pair_index]->key);
        MPIU_ERR_CHKANDJUMP1(ret >= remaining_len, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "Ran out of room for command");
        c += ret;
        remaining_len -= ret;

        /* write value and escape ;'s as ;; */
        MPIU_ERR_CHKANDJUMP(pairs[pair_index]->valueLen > PMI_MAX_VALLEN, mpi_errno, MPI_ERR_OTHER, "**val_too_long");
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
    ret = MPIU_Snprintf(cmdlenbuf, sizeof(cmdlenbuf), "%d", cmdlen);
    MPIU_ERR_CHKANDJUMP1(ret >= PMII_COMMANDLEN_SIZE, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "Command length won't fit in length buffer");

    memcpy(cmdbuf, cmdlenbuf, ret);

    printf_d("PMI sending: %s\n", cmdbuf);
    
    
 #ifdef MPICH_IS_THREADED
    MPIU_THREAD_CHECK_BEGIN;
    {
        MPID_Thread_mutex_lock(&mutex);

        while (blocked)
            MPID_Thread_cond_wait(&cond, &mutex);

        blocked = TRUE;
        MPID_Thread_mutex_unlock(&mutex);
    }
    MPIU_THREAD_CHECK_END;
#endif

    if (PMI_debug)
        ENQUEUE(resp);

    offset = 0;
    do {
        do {
            nbytes = write(fd, &cmdbuf[offset], cmdlen + PMII_COMMANDLEN_SIZE - offset);
        } while (nbytes == -1 && errno == EINTR);

        MPIU_ERR_CHKANDJUMP1(nbytes <= 0, mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", strerror(errno));

        offset += nbytes;
    } while (offset < cmdlen + PMII_COMMANDLEN_SIZE);
#ifdef MPICH_IS_THREADED
    MPIU_THREAD_CHECK_BEGIN;
    {
        MPID_Thread_mutex_lock(&mutex);
        blocked = FALSE;
        MPID_Thread_cond_broadcast(&cond);
        MPID_Thread_mutex_unlock(&mutex);
    }
    MPIU_THREAD_CHECK_END;
#endif
    
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME PMIi_WriteSimpleCommandStr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int PMIi_WriteSimpleCommandStr(int fd, PMI_Command *resp, const char cmd[], ...)
{
    int mpi_errno = MPI_SUCCESS;
    va_list ap;
    PMI_Keyvalpair *pairs;
    PMI_Keyvalpair **pairs_p;
    int npairs;
    int i;
    const char *key;
    const char *val;
    int vallen;
    MPIU_CHKLMEM_DECL(2);

    npairs = 0;
    va_start(ap, cmd);
    while ((key = va_arg(ap, const char *))) {
        val = va_arg(ap, const char *);

        ++npairs;
    }
    va_end(ap);

    MPIU_CHKLMEM_MALLOC(pairs, PMI_Keyvalpair *, sizeof(PMI_Keyvalpair) * npairs, mpi_errno, "pairs");
    MPIU_CHKLMEM_MALLOC(pairs_p, PMI_Keyvalpair **, sizeof(PMI_Keyvalpair *) * npairs, mpi_errno, "pairs_p");

    i = 0;
    va_start(ap, cmd);
    while ((key = va_arg(ap, const char *))) {
        val = va_arg(ap, const char *);
        pairs_p[i] = &pairs[i];
        pairs[i].key = key;
        pairs[i].value = val;
        pairs[i].valueLen = strlen(val);
        pairs[i].isCopy = FALSE;
        ++i;
    }
    va_end(ap);

    mpi_errno = PMIi_WriteSimpleCommand(fd, resp, cmd, pairs_p, npairs);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
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
#undef FUNCNAME
#define FUNCNAME PMII_Connect_to_pm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int PMII_Connect_to_pm( char *hostname, int portnum )
{
    struct hostent     *hp;
    struct sockaddr_in sa;
    int                fd;
    int                optval = 1;
    int                q_wait = 1;
    
    hp = gethostbyname( hostname );
    if (!hp) {
	PMIU_printf( 1, "Unable to get host entry for %s\n", hostname );
	return -1;
    }
    
    memset( (void *)&sa, 0, sizeof(sa) );
    /* POSIX might define h_addr_list only and node define h_addr */
#ifdef HAVE_H_ADDR_LIST
    memcpy( (void *)&sa.sin_addr, (void *)hp->h_addr_list[0], hp->h_length);
#else
    memcpy( (void *)&sa.sin_addr, (void *)hp->h_addr, hp->h_length);
#endif
    sa.sin_family = hp->h_addrtype;
    sa.sin_port   = htons( (unsigned short) portnum );
    
    fd = socket( AF_INET, SOCK_STREAM, TCP );
    if (fd < 0) {
	PMIU_printf( 1, "Unable to get AF_INET socket\n" );
	return -1;
    }
    
    if (setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, 
		    (char *)&optval, sizeof(optval) )) {
	perror( "Error calling setsockopt:" );
    }

    /* We wait here for the connection to succeed */
    if (connect( fd, (struct sockaddr *)&sa, sizeof(sa) ) < 0) {
	switch (errno) {
	case ECONNREFUSED:
	    PMIU_printf( 1, "connect failed with connection refused\n" );
	    /* (close socket, get new socket, try again) */
	    if (q_wait)
		close(fd);
	    return -1;
	    
	case EINPROGRESS: /*  (nonblocking) - select for writing. */
	    break;
	    
	case EISCONN: /*  (already connected) */
	    break;
	    
	case ETIMEDOUT: /* timed out */
	    PMIU_printf( 1, "connect failed with timeout\n" );
	    return -1;

	default:
	    PMIU_printf( 1, "connect failed with errno %d\n", errno );
	    return -1;
	}
    }

    return fd;
}

#if 0
#undef FUNCNAME
#define FUNCNAME PMII_Set_from_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int PMII_Set_from_port( int fd, int id )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int err, rc;

    /* We start by sending a startup message to the server */

    if (PMI_debug) {
	PMIU_printf( 1, "Writing initack to destination fd %d\n", fd );
    }
    /* Handshake and initialize from a port */

    rc = MPIU_Snprintf( buf, PMIU_MAXLINE, "cmd=initack pmiid=%d\n", id );
    if (rc < 0) {
	return PMI_FAIL;
    }
    PMIU_printf( PMI_debug, "writing on fd %d line :%s:\n", fd, buf );
    err = PMIU_writeline( fd, buf );
    if (err) {
	PMIU_printf( 1, "Error in writeline initack\n" );
	return -1;
    }

    /* cmd=initack */
    buf[0] = 0;
    PMIU_printf( PMI_debug, "reading initack\n" );
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading initack on %d\n", fd );
	perror( "Error on readline:" );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp( cmd, "initack" ) ) {
	PMIU_printf( 1, "got unexpected input %s\n", buf );
	return -1;
    }
    
    /* Read, in order, size, rank, and debug.  Eventually, we'll want 
       the handshake to include a version number */

    /* size */
    PMIU_printf( PMI_debug, "reading size\n" );
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading size on %d\n", fd );
	perror( "Error on readline:" );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp(cmd,"set")) {
	PMIU_printf( 1, "got unexpected command %s in %s\n", cmd, buf );
	return -1;
    }
    /* cmd=set size=n */
    PMIU_getval( "size", cmd, PMIU_MAXLINE );
    PMI_size = atoi(cmd);

    /* rank */
    PMIU_printf( PMI_debug, "reading rank\n" );
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading rank on %d\n", fd );
	perror( "Error on readline:" );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp(cmd,"set")) {
	PMIU_printf( 1, "got unexpected command %s in %s\n", cmd, buf );
	return -1;
    }
    /* cmd=set rank=n */
    PMIU_getval( "rank", cmd, PMIU_MAXLINE );
    PMI_rank = atoi(cmd);
    PMIU_Set_rank( PMI_rank );

    /* debug flag */
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading debug on %d\n", fd );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp(cmd,"set")) {
	PMIU_printf( 1, "got unexpected command %s in %s\n", cmd, buf );
	return -1;
    }
    /* cmd=set debug=n */
    PMIU_getval( "debug", cmd, PMIU_MAXLINE );
    PMI_debug = atoi(cmd);

    if (PMI_debug) {
	DBG_PRINTF( ("end of handshake, rank = %d, size = %d\n", 
		    PMI_rank, PMI_size )); 
	DBG_PRINTF( ("Completed init\n" ) );
    }

    return 0;
}
#endif

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
   call PMIi_InitIfSingleton(), which both connects to the process mangager
   and sets up the initial KVS connection entry.
*/

#undef FUNCNAME
#define FUNCNAME PMII_singinit
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int PMII_singinit(void)
{
#if 0
    int pid, rc;
    int singinit_listen_sock, stdin_sock, stdout_sock, stderr_sock;
    char *newargv[8], charpid[8], port_c[8];
    struct sockaddr_in sin;
    socklen_t len;

    /* Create a socket on which to allow an mpiexec to connect back to
       us */
    sin.sin_family	= AF_INET;
    sin.sin_addr.s_addr	= INADDR_ANY;
    sin.sin_port	= htons(0);    /* anonymous port */
    singinit_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    rc = bind(singinit_listen_sock, (struct sockaddr *)&sin ,sizeof(sin));
    len = sizeof(struct sockaddr_in);
    rc = getsockname( singinit_listen_sock, (struct sockaddr *) &sin, &len ); 
    MPIU_Snprintf(port_c, sizeof(port_c), "%d",ntohs(sin.sin_port));
    rc = listen(singinit_listen_sock, 5);

    PMIU_printf( PMI_debug_init, "Starting mpiexec with %s\n", port_c );

    /* Launch the mpiexec process with the name of this port */
    pid = fork();
    if (pid < 0) {
	perror("PMII_singinit: fork failed");
	exit(-1);
    }
    else if (pid == 0) {
	newargv[0] = "mpiexec";
	newargv[1] = "-pmi_args";
	newargv[2] = port_c;
	/* FIXME: Use a valid hostname */
	newargv[3] = "default_interface";  /* default interface name, for now */
	newargv[4] = "default_key";   /* default authentication key, for now */
	MPIU_Snprintf(charpid, sizeof(charpid), "%d",getpid());
	newargv[5] = charpid;
	newargv[6] = NULL;
	rc = execvp(newargv[0],newargv);
	perror("PMII_singinit: execv failed");
	PMIU_printf(1, "  This singleton init program attempted to access some feature\n");
	PMIU_printf(1, "  for which process manager support was required, e.g. spawn or universe_size.\n");
	PMIU_printf(1, "  But the necessary mpiexec is not in your path.\n");
	return(-1);
    }
    else
    {
	char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
	char *p;
	int connectStdio = 0;

	/* Allow one connection back from the created mpiexec program */
	PMI_fd =  accept_one_connection(singinit_listen_sock);
	if (PMI_fd < 0) {
	    PMIU_printf( 1, "Failed to establish singleton init connection\n" );
	    return PMI_FAIL;
	}
	/* Execute the singleton init protocol */
	rc = PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
	PMIU_printf( PMI_debug_init, "Singinit: read %s\n", buf );

	PMIU_parse_keyvals( buf );
	PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
	if (strcmp( cmd, "singinit" ) != 0) {
	    PMIU_printf( 1, "unexpected command from PM: %s\n", cmd );
	    return PMI_FAIL;
	}
	p = PMIU_getval( "authtype", cmd, PMIU_MAXLINE );
	if (p && strcmp( cmd, "none" ) != 0) {
	    PMIU_printf( 1, "unsupported authentication method %s\n", cmd );
	    return PMI_FAIL;
	}
	/* p = PMIU_getval( "authstring", cmd, PMIU_MAXLINE ); */
	
	/* If we're successful, send back our own singinit */
	rc = MPIU_Snprintf( buf, PMIU_MAXLINE, 
     "cmd=singinit pmi_version=%d pmi_subversion=%d stdio=yes authtype=none\n",
			PMI_VERSION, PMI_SUBVERSION );
	if (rc < 0) {
	    return PMI_FAIL;
	}
	PMIU_printf( PMI_debug_init, "GetResponse with %s\n", buf );

	rc = GetResponse( buf, "singinit_info", 0 );
	if (rc != 0) {
	    PMIU_printf( 1, "GetResponse failed\n" );
	    return PMI_FAIL;
	}
	p = PMIU_getval( "versionok", cmd, PMIU_MAXLINE );
	if (p && strcmp( cmd, "yes" ) != 0) {
	    PMIU_printf( 1, "Process manager needs a different PMI version\n" );
	    return PMI_FAIL;
	}
	p = PMIU_getval( "stdio", cmd, PMIU_MAXLINE );
	if (p && strcmp( cmd, "yes" ) == 0) {
	    PMIU_printf( PMI_debug_init, "PM agreed to connect stdio\n" );
	    connectStdio = 1;
	}
	p = PMIU_getval( "kvsname", singinit_kvsname, sizeof(singinit_kvsname) );
	PMIU_printf( PMI_debug_init, "kvsname to use is %s\n", 
		     singinit_kvsname );
	
	if (connectStdio) {
	    PMIU_printf( PMI_debug_init, 
			 "Accepting three connections for stdin, out, err\n" );
	    stdin_sock  = accept_one_connection(singinit_listen_sock);
	    dup2(stdin_sock, 0);
	    stdout_sock = accept_one_connection(singinit_listen_sock);
	    dup2(stdout_sock,1);
	    stderr_sock = accept_one_connection(singinit_listen_sock);
	    dup2(stderr_sock,2);
	}
	PMIU_printf( PMI_debug_init, "Done with singinit handshake\n" );
    }
    return 0;
#endif
}

/* Promote PMI to a fully initialized version if it was started as
   a singleton init */
#undef FUNCNAME
#define FUNCNAME PMII_InitIfSingleton
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int PMIi_InitIfSingleton(void)
{
#if 0
    int rc;
    static int firstcall = 1;

    if (PMI_initialized != SINGLETON_INIT_BUT_NO_PM || !firstcall) return 0;

    /* We only try to init as a singleton the first time */
    firstcall = 0;

    /* First, start (if necessary) an mpiexec, connect to it, 
       and start the singleton init handshake */
    rc = PMII_singinit();

    if (rc < 0)
	return(-1);
    PMI_initialized = SINGLETON_INIT_WITH_PM;    /* do this right away */
    PMI_size	    = 1;
    PMI_rank	    = 0;
    PMI_debug	    = 0;
    PMI_spawned	    = 0;

    PMII_getmaxes( &PMI_kvsname_max, &PMI_keylen_max, &PMI_vallen_max );

    /* FIXME: We need to support a distinct kvsname for each 
       process group */
    PMI_KVS_Put( singinit_kvsname, cached_singinit_key, cached_singinit_val );

#endif
    return 0;
}

#undef FUNCNAME
#define FUNCNAME accept_one_connection
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int accept_one_connection(int list_sock)
{
    int gotit, new_sock;
    struct sockaddr_in from;
    socklen_t len;

    len = sizeof(from);
    gotit = 0;
    while ( ! gotit )
    {
	new_sock = accept(list_sock, (struct sockaddr *)&from, &len);
	if (new_sock == -1)
	{
	    if (errno == EINTR)    /* interrupted? If so, try again */
		continue;
	    else
	    {
		PMIU_printf(1, "accept failed in accept_one_connection\n");
		exit(-1);
	    }
	}
	else
	    gotit = 1;
    }
    return(new_sock);
}


/* Get the FD to use for PMI operations.  If a port is used, rather than 
   a pre-established FD (i.e., via pipe), this routine will handle the 
   initial handshake.  
*/
#undef FUNCNAME
#define FUNCNAME getPMIFD
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int getPMIFD(void)
{
    int mpi_errno = MPI_SUCCESS;
    char *p;

    /* Set the default */
    PMI_fd = -1;    

    p = getenv( "PMI_PORT" );
    if (p) {
	int portnum;
	char hostname[MAXHOSTNAME+1];
	char *pn, *ph;

	/* Connect to the indicated port (in format hostname:portnumber) 
	   and get the fd for the socket */
	
	/* Split p into host and port */
	pn = p;
	ph = hostname;
	while (*pn && *pn != ':' && (ph - hostname) < MAXHOSTNAME) {
	    *ph++ = *pn++;
	}
	*ph = 0;

        MPIU_ERR_CHKANDJUMP1(*pn != ':', mpi_errno, MPI_ERR_OTHER, "**pmi_port", "**pmi_port %s", p);
        
        portnum = atoi( pn+1 );
        /* FIXME: Check for valid integer after : */
        /* This routine only gets the fd to use to talk to 
           the process manager. The handshake below is used
           to setup the initial values */
        PMI_fd = PMII_Connect_to_pm( hostname, portnum );
        MPIU_ERR_CHKANDJUMP2(PMI_fd < 0, mpi_errno, MPI_ERR_OTHER, "**connect_to_pm", "**connect_to_pm %s %d", hostname, portnum);
    }

    /* OK to return success for singleton init */

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* ----------------------------------------------------------------------- */
/* 
 * This function is used to request information from the server and check
 * that the response uses the expected command name.  On a successful
 * return from this routine, additional PMIU_getval calls may be used
 * to access information about the returned value.
 *
 * If checkRc is true, this routine also checks that the rc value returned
 * was 0.  If not, it uses the "msg" value to report on the reason for
 * the failure.
 */
#undef FUNCNAME
#define FUNCNAME GetResponse
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int GetResponse( const char request[], const char expectedCmd[],
			int checkRc )
{
    int err, n;
    char *p;
    char recvbuf[PMIU_MAXLINE];
    char cmdName[PMIU_MAXLINE];
#if 0

    /* FIXME: This is an example of an incorrect fix - writeline can change
       the second argument in some cases, and that will break the const'ness
       of request.  Instead, writeline should take a const item and return
       an error in the case in which it currently truncates the data. */
    err = PMIU_writeline( PMI_fd, (char *)request );
    if (err) {
	return err;
    }
    n = PMIU_readline( PMI_fd, recvbuf, sizeof(recvbuf) );
    if (n <= 0) {
	PMIU_printf( 1, "readline failed\n" );
	return PMI_FAIL;
    }
    err = PMIU_parse_keyvals( recvbuf );
    if (err) {
	PMIU_printf( 1, "parse_kevals failed %d\n", err );
	return err;
    }
    p = PMIU_getval( "cmd", cmdName, sizeof(cmdName) );
    if (!p) {
	PMIU_printf( 1, "getval cmd failed\n" );
	return PMI_FAIL;
    }
    if (strcmp( expectedCmd, cmdName ) != 0) {
	PMIU_printf( 1, "expecting cmd=%s, got %s\n", expectedCmd, cmdName );
	return PMI_FAIL;
    }
    if (checkRc) {
	p = PMIU_getval( "rc", cmdName, PMIU_MAXLINE );
	if ( p && strcmp(cmdName,"0") != 0 ) {
	    PMIU_getval( "msg", cmdName, PMIU_MAXLINE );
	    PMIU_printf( 1, "Command %s failed, reason='%s'\n", 
			 request, cmdName );
	    return PMI_FAIL;
	}
    }

#endif
    return err;
}


static void dump_PMI_Keyvalpair(FILE *file, PMI_Keyvalpair *kv)
{
    fprintf(file, "  key      = %s\n", kv->key);
    fprintf(file, "  value    = %s\n", kv->value);
    fprintf(file, "  valueLen = %d\n", kv->valueLen);
    fprintf(file, "  isCopy   = %s\n", kv->isCopy ? "TRUE" : "FALSE");
}

static void dump_PMI_Command(FILE *file, PMI_Command *cmd)
{
    int i;
    
    fprintf(file, "cmd    = %s\n", cmd->command);
    fprintf(file, "nPairs = %d\n", cmd->nPairs);

    for (i = 0; i < cmd->nPairs; ++i)
        dump_PMI_Keyvalpair(file, cmd->pairs[i]);
}
