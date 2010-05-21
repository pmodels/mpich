/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"

MPIU_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef ENABLE_CHECKPOINTING

#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <libcr.h>
#include <stdio.h>
#include "pmi.h"

#define CHECK_ERR(cond, msg) do {                                              \
        if (cond) {                                                            \
            fprintf(stderr, "Error: %s:%d \"%s\"\n", __FILE__, __LINE__, msg); \
            fflush(stderr);                                                    \
            return -1;                                                         \
        }                                                                      \
    } while (0)

#define CHECK_ERR_MPI(cond, mpi_errno, msg) do {                                \
        if (cond) {                                                             \
            char error_msg[ 4096 ];                                             \
            fprintf(stderr, "Error: %s:%d \"%s\"\n", __FILE__, __LINE__, msg);  \
            MPIR_Err_get_string(mpi_errno, error_msg, sizeof(error_msg), NULL); \
            fprintf(stderr, "%s\n", error_msg);                                 \
            fflush(stderr);                                                     \
            return -1;                                                          \
        }                                                                       \
    } while (0)

#define MAX_STR_LEN 1024

int MPIDI_nem_ckpt_start_checkpoint = FALSE;
int MPIDI_nem_ckpt_finish_checkpoint = FALSE;

static int checkpointing = FALSE;
static unsigned short current_wave;
static int marker_count;
static enum {CKPT_NULL, CKPT_CONTINUE, CKPT_RESTART, CKPT_ERROR} ckpt_result = CKPT_NULL;

static int reinit_pmi(void);
static int restore_env(pid_t parent_pid, int rank);
static int restore_stdinouterr(int requester_pid, int rank);
static int open_fifo(const char *fname_template, int rank, int restart_pid, int dupfd, int flags);

static sem_t ckpt_sem;
static sem_t cont_sem;

static int ckpt_cb(void *arg)
{
    int rc, ret;
    const struct cr_restart_info* ri;

    if (MPIDI_Process.my_pg_rank == 0)
        MPIDI_nem_ckpt_start_checkpoint = TRUE;

    ret = sem_wait(&ckpt_sem);
    CHECK_ERR(ret, "sem_wait");

    if (MPID_nem_netmod_func->ckpt_precheck) {
        int mpi_errno;
        mpi_errno = MPID_nem_netmod_func->ckpt_precheck();
        CHECK_ERR_MPI(mpi_errno, mpi_errno, "ckpt_precheck failed");
    }

    rc = cr_checkpoint(0);
    if (rc < 0) {
        ckpt_result = CKPT_ERROR;
    } else if (rc) {

        ckpt_result = CKPT_RESTART;
        ri = cr_get_restart_info();
        CHECK_ERR(!ri, "cr_get_restart_info");
        ret = restore_env(ri->requester, MPIDI_Process.my_pg_rank);
        CHECK_ERR(ret, "restore_env");
        ret = restore_stdinouterr(ri->requester, MPIDI_Process.my_pg_rank);
        CHECK_ERR(ret, "restore_stdinouterr");
        ret = reinit_pmi();
        CHECK_ERR(ret, "reinit_pmi");

        if (MPID_nem_netmod_func->ckpt_restart) {
            int mpi_errno;
            mpi_errno = MPID_nem_netmod_func->ckpt_restart();
            CHECK_ERR_MPI(mpi_errno, mpi_errno, "ckpt_restart failed");
        }
    } else {

        ckpt_result = CKPT_CONTINUE;

        if (MPID_nem_netmod_func->ckpt_continue) {
            int mpi_errno;
            mpi_errno = MPID_nem_netmod_func->ckpt_continue();
            CHECK_ERR_MPI(mpi_errno, mpi_errno, "ckpt_continue failed");
        }
    }

    ret = sem_post(&cont_sem);
    CHECK_ERR(ret, "sem_post");

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_nem_ckpt_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_nem_ckpt_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    cr_callback_id_t cb_id;
    cr_client_id_t client_id;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_NEM_CKPT_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_NEM_CKPT_INIT);

    client_id = cr_init();
    MPIU_ERR_CHKANDJUMP(client_id < 0 && errno == ENOSYS, mpi_errno, MPI_ERR_OTHER, "**blcr_mod");

    cb_id = cr_register_callback(ckpt_cb, NULL, CR_THREAD_CONTEXT);
    MPIU_ERR_CHKANDJUMP1(cb_id == -1, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", strerror(errno));
    
    checkpointing = FALSE;
    current_wave = 0;

    ret = sem_init(&ckpt_sem, 0, 0);
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_init", "**sem_init %s", strerror(errno));
    ret = sem_init(&cont_sem, 0, 0);
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_init", "**sem_init %s", strerror(errno));

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_NEM_CKPT_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_nem_ckpt_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_nem_ckpt_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_NEM_CKPT_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_NEM_CKPT_FINALIZE);

    ret = sem_destroy(&ckpt_sem);
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_destroy", "**sem_destroy %s", strerror(errno));
    ret = sem_destroy(&cont_sem);
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_destroy", "**sem_destroy %s", strerror(errno));

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_NEM_CKPT_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME reinit_pmi
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int reinit_pmi(void)
{
    int ret;
    int has_parent = 0;
    int pg_rank, pg_size;
    int kvs_name_sz, pg_id_sz;
    
    MPIDI_STATE_DECL(MPID_STATE_REINIT_PMI);

    MPIDI_FUNC_ENTER(MPID_STATE_REINIT_PMI);

    /* Init pmi and do some sanity checks */
    ret = PMI_Init(&has_parent);
    CHECK_ERR(ret, "pmi_init");

    ret = PMI_Get_rank(&pg_rank);
    CHECK_ERR(ret, "pmi_get_rank");

    ret = PMI_Get_size(&pg_size);
    CHECK_ERR(ret, "pmi_get_size");

    CHECK_ERR(pg_size != MPIDI_Process.my_pg->size, "pg size differs after restart");
    CHECK_ERR(pg_rank != MPIDI_Process.my_pg_rank, "pg rank differs after restart");

    /* get new pg_id */
    ret = PMI_Get_id_length_max(&pg_id_sz);
    CHECK_ERR(ret, "pmi_get_id_length_max");
    
    MPIU_Free(MPIDI_Process.my_pg->id);
   
    MPIDI_Process.my_pg->id = MPIU_Malloc(pg_id_sz + 1);
    CHECK_ERR(MPIDI_Process.my_pg->id == NULL, "malloc failed");

    ret = PMI_Get_id(MPIDI_Process.my_pg->id, pg_id_sz);
    CHECK_ERR(ret, "pmi_get_id");

    /* get new kvsname */
    ret = PMI_KVS_Get_name_length_max(&kvs_name_sz);
    CHECK_ERR(ret, "PMI_KVS_Get_name_length_max");
    
    MPIU_Free(MPIDI_Process.my_pg->connData);
   
    MPIDI_Process.my_pg->connData = MPIU_Malloc(kvs_name_sz + 1);
    CHECK_ERR(MPIDI_Process.my_pg->connData == NULL, "malloc failed");

    ret = PMI_KVS_Get_my_name(MPIDI_Process.my_pg->connData, kvs_name_sz);
    CHECK_ERR(ret, "PMI_Get_id");

    
    MPIDI_FUNC_EXIT(MPID_STATE_REINIT_PMI);
    return 0;
}


#undef FUNCNAME
#define FUNCNAME restore_env
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int restore_env(pid_t parent_pid, int rank)
{
    FILE *f;
    char env_filename[MAX_STR_LEN];
    char var_val[MAX_STR_LEN];
    int ret;
    

    MPIU_Snprintf(env_filename, MAX_STR_LEN, "/tmp/hydra-env-file-%d:%d", parent_pid, rank); 

    f = fopen(env_filename, "r");
    CHECK_ERR(!f, strerror (errno));

    /* ret = unlink(env_filename); */
    /* CHECK_ERR(ret, strerror (errno)); */

    while (fgets(var_val, MAX_STR_LEN, f)) {
        ret = MPL_putenv(MPIU_Strdup(var_val));
        CHECK_ERR(ret != 0, strerror (errno));
    }

    ret = fclose(f);
    CHECK_ERR(ret, strerror (errno));

    return 0;
}

#undef FUNCNAME
#define FUNCNAME open_fifo
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int open_fifo(const char *fname_template, int rank, int restart_pid, int dupfd, int flags)
{
    char filename[256];
    int fd;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_OPEN_FIFO);

    MPIDI_FUNC_ENTER(MPID_STATE_OPEN_FIFO);

    ret = MPIU_Snprintf(filename, sizeof(filename), fname_template, restart_pid, rank);
    CHECK_ERR(ret >= sizeof(filename), "filename too long");


    fd = open(filename, flags);
    CHECK_ERR(fd == -1 && errno != ENOENT, "open fifo");
    if (fd == -1) goto fn_exit; /* if the file doesn't exist, skip this */
    ret = unlink(filename);
    CHECK_ERR(ret, "unlink fifo");
    ret = dup2(fd, dupfd);
    CHECK_ERR(ret == -1, "dup2 fifo");
    ret = close(fd);
    CHECK_ERR(ret, "close fifo");

    MPIDI_FUNC_EXIT(MPID_STATE_OPEN_FIFO);
fn_exit:
    return 0;
}


#undef FUNCNAME
#define FUNCNAME restore_stdinouterr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int restore_stdinouterr(int restart_pid, int rank)
{
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_RESTORE_STDINOUTERR);

    MPIDI_FUNC_ENTER(MPID_STATE_RESTORE_STDINOUTERR);

    ret = open_fifo("/tmp/hydra-in-%d:%d",  rank, restart_pid, 0, O_RDONLY);
    CHECK_ERR(ret, "open stdin fifo");
    ret = open_fifo("/tmp/hydra-out-%d:%d", rank, restart_pid, 1, O_WRONLY);
    CHECK_ERR(ret, "open stdout fifo");
    ret = open_fifo("/tmp/hydra-err-%d:%d", rank, restart_pid, 2, O_WRONLY);
    CHECK_ERR(ret, "open stderr fifo");

    MPIDI_FUNC_EXIT(MPID_STATE_RESTORE_STDINOUTERR);
    return 0;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_nem_ckpt_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_nem_ckpt_start(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_NEM_CKPT_START);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_NEM_CKPT_START);

    if (checkpointing)
        goto fn_exit;

    checkpointing = TRUE;

    marker_count = MPIDI_Process.my_pg->size - 1; /* We won't receive a marker from ourselves. */

    ++current_wave;

    /* send markers to all other processes */
    /* FIXME: we're only handling processes in our pg, so no dynamic connections */
    for (i = 0; i <  MPIDI_Process.my_pg->size; ++i) {
        MPID_Request *req;
        MPIDI_VC_t *vc;
        MPIDI_CH3I_VC *vc_ch;
        MPID_PKT_DECL_CAST(upkt, MPID_nem_pkt_ckpt_marker_t, ckpt_pkt);

        /* Don't send a marker to ourselves. */
        if (i == MPIDI_Process.my_pg_rank)
            continue;
       
        MPIDI_PG_Get_vc_set_active(MPIDI_Process.my_pg, i, &vc);
        vc_ch = ((MPIDI_CH3I_VC *)vc->channel_private);

        MPIDI_Pkt_init(ckpt_pkt, MPIDI_NEM_PKT_CKPT_MARKER);
        ckpt_pkt->wave = current_wave;
        
        mpi_errno = MPIDI_CH3_iStartMsg(vc, ckpt_pkt, sizeof(ckpt_pkt), &req);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ckptpkt");
        if (req != NULL)
        {
            MPIU_ERR_CHKANDJUMP(req->status.MPI_ERROR, mpi_errno, MPI_ERR_OTHER, "**ckptpkt");
            MPID_Request_release(req);
        }

        if (!vc_ch->is_local) {
            mpi_errno = vc_ch->ckpt_pause_send_vc(vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }
    

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_NEM_CKPT_START);
    return mpi_errno;
fn_fail:

    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_nem_ckpt_finish
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_nem_ckpt_finish(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_NEM_CKPT_FINISH);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_NEM_CKPT_FINISH);

    /* Since we're checkpointing the shared memory region (i.e., the
       channels between local procs), we don't have to flush those
       channels, just make sure no one is sending or receiving during
       the checkpoint */
    mpi_errno = MPID_nem_barrier();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    ret = sem_post(&ckpt_sem);
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_post", "**sem_post %s", strerror(errno));

    ret = sem_wait(&cont_sem);
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_wait", "**sem_wait %s", strerror(errno));

    mpi_errno = MPID_nem_barrier();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    for (i = 0; i < MPIDI_Process.my_pg->size; ++i) {
        MPIDI_VC_t *vc;
        MPIDI_CH3I_VC *vc_ch;
        /* We didn't send a marker to ourselves. */
        if (i == MPIDI_Process.my_pg_rank)
            continue;

        if (ckpt_result == CKPT_CONTINUE) {
            MPIDI_PG_Get_vc_set_active(MPIDI_Process.my_pg, i, &vc);
            vc_ch = ((MPIDI_CH3I_VC *)vc->channel_private);
            if (!vc_ch->is_local) {
                mpi_errno = vc_ch->ckpt_continue_vc(vc);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
        }
    }
    
    checkpointing = FALSE;
    
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_NEM_CKPT_FINISH);
    return mpi_errno;
fn_fail:

    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME pkt_ckpt_marker_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int pkt_ckpt_marker_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIDI_msg_sz_t *buflen, MPID_Request **req)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_ckpt_marker_t * const ckpt_pkt = (MPID_nem_pkt_ckpt_marker_t *)pkt;
    MPIDI_CH3I_VC *vc_ch = ((MPIDI_CH3I_VC *)vc->channel_private);
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_PKT_CKPT_MARKER_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_PKT_CKPT_MARKER_HANDLER);

    if (!checkpointing) {
        mpi_errno = MPIDI_nem_ckpt_start();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    MPIU_Assert(current_wave == ckpt_pkt->wave);

    --marker_count;

    /* We're checkpointing the shared memory region, so we don't need
       to flush the channels between local processes, only remote
       processes */
    
    /* make sure netmods don't receive any packets from this vc after the marker */
    if (!vc_ch->is_local) {
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        /* There should be nothing in the channel following this. */
        MPIU_Assert(*buflen == sizeof(MPIDI_CH3_Pkt_t));
    }

    if (marker_count == 0) {
        MPIDI_nem_ckpt_finish_checkpoint = TRUE;
        /* make sure we break out of receive loop into progress loop */
        MPIDI_CH3_Progress_signal_completion();
    }
    
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *req = NULL;

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_PKT_CKPT_MARKER_HANDLER);
    return mpi_errno;
fn_fail:

    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ckpt_pkthandler_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_nem_ckpt_pkthandler_init(MPIDI_CH3_PktHandler_Fcn *pktArray[], int arraySize)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CKPT_PKTHANDLER_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CKPT_PKTHANDLER_INIT);

    /* Check that the array is large enough */
    if (arraySize <= MPIDI_NEM_PKT_END) {
	MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_INTERN, "**ch3|pktarraytoosmall");
    }

    pktArray[MPIDI_NEM_PKT_CKPT_MARKER] = pkt_ckpt_marker_handler;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CKPT_PKTHANDLER_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif /* ENABLE_CHECKPOINTING */
