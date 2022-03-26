/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_nem_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===
cvars:
    - name        : MPIR_CVAR_NEMESIS_ENABLE_CKPOINT
      category    : NEMESIS
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, enables checkpointing support and returns an error if
        checkpointing library cannot be initialized.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef ENABLE_CHECKPOINTING

#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <libcr.h>
#include <stdio.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#define CHECK_ERR(cond, msg) do {                                              \
        if (cond) {                                                            \
            fprintf(stderr, "Error: %s:%d \"%s\"\n", __FILE__, __LINE__, msg); \
            fflush(stderr);                                                    \
            return -1;                                                         \
        }                                                                      \
    } while (0)

#define CHECK_ERR_ERRNO(cond, msg) do {                                        \
        if (cond) {                                                            \
            fprintf(stderr, "Error: %s:%d \"%s\", %s\n", __FILE__, __LINE__,   \
                    msg, strerror(errno));                                     \
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
static int restore_stdinouterr(int rank);

static sem_t ckpt_sem;
static sem_t cont_sem;

static int ckpt_cb(void *arg)
{
    int rc, ret;
    const struct cr_restart_info* ri;

    if (MPIDI_Process.my_pg_rank == 0) {
        MPIDI_nem_ckpt_start_checkpoint = TRUE;
        /* poke the progress engine in case we're waiting in a blocking recv */
        MPIDI_CH3_Progress_signal_completion();
    }

    do {
        ret = sem_wait(&ckpt_sem);
    } while (ret == -1 && errno == EINTR);
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
        ret = restore_stdinouterr(MPIDI_Process.my_pg_rank);
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
    
    do {
        ret = sem_post(&cont_sem);
    } while (ret == -1 && errno == EINTR);
    CHECK_ERR(ret, "sem_post");

    return 0;
}

int MPIDI_nem_ckpt_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    cr_callback_id_t cb_id;
    cr_client_id_t client_id;
    int ret;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];

    MPIR_FUNC_ENTER;

    if (!MPIR_CVAR_NEMESIS_ENABLE_CKPOINT)
        goto fn_exit;
    
    client_id = cr_init();
    MPIR_ERR_CHKANDJUMP(client_id < 0 && errno == ENOSYS, mpi_errno, MPI_ERR_OTHER, "**blcr_mod");

    cb_id = cr_register_callback(ckpt_cb, NULL, CR_THREAD_CONTEXT);
    MPIR_ERR_CHKANDJUMP1(cb_id == -1, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));
    
    checkpointing = FALSE;
    current_wave = 0;

    ret = sem_init(&ckpt_sem, 0, 0);
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_init", "**sem_init %s",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));
    ret = sem_init(&cont_sem, 0, 0);
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_init", "**sem_init %s",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));

 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int MPIDI_nem_ckpt_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];

    MPIR_FUNC_ENTER;

    ret = sem_destroy(&ckpt_sem);
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_destroy", "**sem_destroy %s",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));
    ret = sem_destroy(&cont_sem);
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_destroy", "**sem_destroy %s",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));

 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

static int reinit_pmi(void)
{
    int ret;
    
    MPIR_FUNC_ENTER;

    /* Init pmi and do some sanity checks */
    ret = MPIR_pmi_init();
    CHECK_ERR(ret, "pmi_init");

    CHECK_ERR(MPIR_Process.size != MPIDI_Process.my_pg->size, "pg size differs after restart");
    CHECK_ERR(MPIR_Process.rank != MPIDI_Process.my_pg_rank, "pg rank differs after restart");

    /* get new pg_id */
    ret = PMI_KVS_Get_name_length_max(&pg_id_sz);
    CHECK_ERR(ret, "pmi_get_id_length_max");
    
    MPL_free(MPIDI_Process.my_pg->id);
    MPIDI_Process.my_pg->id = MPL_strdup(MPIR_pmi_job_id());

    MPL_free(MPIDI_Process.my_pg->connData);
    MPIDI_Process.my_pg->connData = MPL_strdup(MPIR_pmi_job_id());
    
    MPIR_FUNC_EXIT;
    return 0;
}


static int restore_env(pid_t parent_pid, int rank)
{
    FILE *f;
    char env_filename[MAX_STR_LEN];
    char var_val[MAX_STR_LEN];
    int ret;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];

    MPL_snprintf(env_filename, MAX_STR_LEN, "/tmp/hydra-env-file-%d:%d", parent_pid, rank); 

    f = fopen(env_filename, "r");
    CHECK_ERR(!f, MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));

    ret = unlink(env_filename);
    CHECK_ERR(ret, MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));

    while (fgets(var_val, MAX_STR_LEN, f)) {
        size_t len = strlen(var_val);
        /* remove newline */
        if (var_val[len-1] == '\n')
            var_val[len-1] = '\0';
        ret = MPL_putenv(MPL_strdup(var_val));
        CHECK_ERR(ret != 0, MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));
    }

    ret = fclose(f);
    CHECK_ERR(ret, MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));

    return 0;
}

typedef enum { IN_SOCK, OUT_SOCK, ERR_SOCK } socktype_t;
typedef struct sock_ident {
    int rank;
    socktype_t socktype;
    int pid;
} sock_ident_t;
    
#define STDINOUTERR_PORT_NAME "CKPOINT_STDINOUTERR_PORT"

static int open_io_socket(socktype_t socktype, int rank, int dupfd)
{
    int fd;
    int ret;
    struct sockaddr_in sock_addr;
    in_addr_t addr;
    sock_ident_t id;
    int port;
    int len;
    char *id_p;

    MPIR_FUNC_ENTER;

    memset(&sock_addr, 0, sizeof(sock_addr));
    memset(&addr, 0, sizeof(addr));

    MPL_env2int(STDINOUTERR_PORT_NAME, &port);
    addr = inet_addr("127.0.0.1");
    CHECK_ERR_ERRNO(addr == INADDR_NONE, "inet_addr");

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons((in_port_t)port);
    sock_addr.sin_addr.s_addr = addr;

    do {
        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    } while (fd == -1 && errno == EINTR);
    CHECK_ERR_ERRNO(fd == -1, "socket");
    do {
        ret = connect(fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    } while (ret == -1 && errno == EINTR);
    CHECK_ERR_ERRNO(ret == -1, "connect");

    id.rank = rank;
    id.socktype = socktype;
    id.pid = getpid();
    
    len = sizeof(id);
    id_p = (char *)&id;
    do {
        do {
            ret = write(fd, id_p, len);
        } while (ret == 0 || (ret == -1 && errno == EINTR));
        CHECK_ERR_ERRNO(ret == -1, "write failed");
        len -= ret;
        id_p += ret;
    } while (len);

    ret = dup2(fd, dupfd);
    CHECK_ERR_ERRNO(ret == -1, "dup2 socket");
    ret = close(fd);
    CHECK_ERR_ERRNO(ret, "close socket");
    
    MPIR_FUNC_EXIT;
fn_exit:
    return 0;
}

static int restore_stdinouterr(int rank)
{
    int ret;

    MPIR_FUNC_ENTER;

    if (rank == 0) {
        ret = open_io_socket(IN_SOCK,  rank, 0);
        CHECK_ERR(ret, "open stdin socket");
    }
    ret = open_io_socket(OUT_SOCK, rank, 1);
    CHECK_ERR(ret, "open stdin socket");
    ret = open_io_socket(ERR_SOCK, rank, 2);
    CHECK_ERR(ret, "open stdin socket");

    MPIR_FUNC_EXIT;
    return 0;
}


int MPIDI_nem_ckpt_start(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIR_FUNC_ENTER;

    if (checkpointing)
        goto fn_exit;

    checkpointing = TRUE;

    marker_count = MPIDI_Process.my_pg->size - 1; /* We won't receive a marker from ourselves. */

    ++current_wave;

    /* send markers to all other processes */
    /* FIXME: we're only handling processes in our pg, so no dynamic connections */
    for (i = 0; i <  MPIDI_Process.my_pg->size; ++i) {
        MPIR_Request *req;
        MPIDI_VC_t *vc;
        MPIDI_CH3I_VC *vc_ch;
        MPID_PKT_DECL_CAST(upkt, MPID_nem_pkt_ckpt_marker_t, ckpt_pkt);

        /* Don't send a marker to ourselves. */
        if (i == MPIDI_Process.my_pg_rank)
            continue;
       
        MPIDI_PG_Get_vc_set_active(MPIDI_Process.my_pg, i, &vc);
        vc_ch = &vc->ch;

        MPIDI_Pkt_init(ckpt_pkt, MPIDI_NEM_PKT_CKPT_MARKER);
        ckpt_pkt->wave = current_wave;
        
        mpi_errno = MPIDI_CH3_iStartMsg(vc, ckpt_pkt, sizeof(ckpt_pkt), &req);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ckptpkt");
        if (req != NULL)
        {
            MPIR_ERR_CHKANDJUMP(req->status.MPI_ERROR, mpi_errno, MPI_ERR_OTHER, "**ckptpkt");
            MPIR_Request_free(req);
        }

        if (!vc_ch->is_local) {
            mpi_errno = vc_ch->ckpt_pause_send_vc(vc);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    

fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
fn_fail:

    goto fn_exit;
}

int MPIDI_nem_ckpt_finish(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int ret;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];

    MPIR_FUNC_ENTER;

    /* Since we're checkpointing the shared memory region (i.e., the
       channels between local procs), we don't have to flush those
       channels, just make sure no one is sending or receiving during
       the checkpoint */
    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    do {
        ret = sem_post(&ckpt_sem);
    } while (ret == -1 && errno == EINTR);
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_post", "**sem_post %s",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));

    do {
        ret = sem_wait(&cont_sem);
    } while (ret == -1 && errno == EINTR);
    MPIR_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER, "**sem_wait", "**sem_wait %s",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));

    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    if (ckpt_result == CKPT_CONTINUE) {
        for (i = 0; i < MPIDI_Process.my_pg->size; ++i) {
            MPIDI_VC_t *vc;
            MPIDI_CH3I_VC *vc_ch;
            /* We didn't send a marker to ourselves. */
            if (i == MPIDI_Process.my_pg_rank)
                continue;

            MPIDI_PG_Get_vc(MPIDI_Process.my_pg, i, &vc);
            vc_ch = &vc->ch;
            if (!vc_ch->is_local) {
                mpi_errno = vc_ch->ckpt_continue_vc(vc);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }
    
    checkpointing = FALSE;
    
fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
fn_fail:

    goto fn_exit;
}


static int pkt_ckpt_marker_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data ATTRIBUTE((unused)),
                                   intptr_t *buflen, MPIR_Request **req)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_ckpt_marker_t * const ckpt_pkt = (MPID_nem_pkt_ckpt_marker_t *)pkt;

    MPIR_FUNC_ENTER;

    if (!checkpointing) {
        mpi_errno = MPIDI_nem_ckpt_start();
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_Assert(current_wave == ckpt_pkt->wave);

    --marker_count;

    /* We're checkpointing the shared memory region, so we don't need
       to flush the channels between local processes, only remote
       processes */

    if (marker_count == 0) {
        MPIDI_nem_ckpt_finish_checkpoint = TRUE;
        /* make sure we break out of receive loop into progress loop */
        MPIDI_CH3_Progress_signal_completion();
    }
    
    *buflen = 0;
    *req = NULL;

fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
fn_fail:

    goto fn_exit;
}


int MPIDI_nem_ckpt_pkthandler_init(MPIDI_CH3_PktHandler_Fcn *pktArray[], int arraySize)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* Check that the array is large enough */
    if (arraySize <= MPIDI_CH3_PKT_END_ALL) {
	MPIR_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_INTERN, "**ch3|pktarraytoosmall");
    }

    pktArray[MPIDI_NEM_PKT_CKPT_MARKER] = pkt_ckpt_marker_handler;

 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif /* ENABLE_CHECKPOINTING */
