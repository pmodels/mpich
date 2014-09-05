/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  (C) 2012 NEC Corporation
 *      Author: Masamichi Takagi
 */

#include "ib_impl.h"
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

//#define MPID_NEM_IB_DEBUG_INIT
#ifdef dprintf  /* avoid redefinition with src/mpid/ch3/include/mpidimpl.h */
#undef dprintf
#endif
#ifdef MPID_NEM_IB_DEBUG_INIT
#define dprintf printf
#else
#define dprintf(...)
#endif

MPID_nem_netmod_funcs_t MPIDI_nem_ib_funcs = {
    MPID_nem_ib_init,
    MPID_nem_ib_finalize,
    MPID_nem_ib_poll,
    MPID_nem_ib_get_business_card,
    MPID_nem_ib_connect_to_root,
    MPID_nem_ib_vc_init,
    MPID_nem_ib_vc_destroy,
    MPID_nem_ib_vc_terminate,
    NULL,       /*MPID_nem_ib_anysource_iprobe */
    NULL,       /*MPID_nem_ib_anysource_improbe */
};

MPIDI_CH3_PktHandler_Fcn *MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_NUM_PKT_HANDLERS];

static MPIDI_Comm_ops_t comm_ops = {
    /*NULL, */ MPID_nem_ib_recv_posted,
    /* recv_posted */

    NULL,       /* send */
    NULL,       /* rsend */
    NULL,       /* ssend */
    NULL,       /* isend */
    NULL,       /* irsend */
    NULL,       /* issend */

    NULL,       /* send_init */
    NULL,       /* bsend_init */
    NULL,       /* rsend_init */
    NULL,       /* ssend_init */
    NULL,       /* startall */

    NULL,       /* cancel_send */
    NULL,       /* cancel_recv */

    NULL,       /* probe */
    NULL,       /* iprobe */
    NULL,       /* improbe */
};

void *MPID_nem_ib_fl[18];
int MPID_nem_ib_nranks;
MPID_nem_ib_conn_ud_t *MPID_nem_ib_conn_ud;
MPID_nem_ib_conn_t *MPID_nem_ib_conns;
int MPID_nem_ib_conns_ref_count;
//MPIDI_VC_t **MPID_nem_ib_pollingset;
int MPID_nem_ib_conn_ud_fd;
MPID_nem_ib_com_t *MPID_nem_ib_conn_ud_MPID_nem_ib_com;
//int MPID_nem_ib_npollingset;
int *MPID_nem_ib_scratch_pad_fds;
int MPID_nem_ib_scratch_pad_fds_ref_count;
MPID_nem_ib_com_t **MPID_nem_ib_scratch_pad_ibcoms;
//char *MPID_nem_ib_recv_buf;
int MPID_nem_ib_myrank;
uint64_t MPID_nem_ib_tsc_poll;
int MPID_nem_ib_ncqe;
uint64_t MPID_nem_ib_progress_engine_vt;
uint16_t MPID_nem_ib_remote_poll_shared;
#ifdef MPID_NEM_IB_ONDEMAND
uint16_t MPID_nem_ib_cm_ringbuf_head;
uint16_t MPID_nem_ib_cm_ringbuf_tail;
uint64_t MPID_nem_ib_cm_ringbuf_released[(MPID_NEM_IB_CM_NSEG + 63) / 64];
MPID_nem_ib_cm_sendq_t MPID_nem_ib_cm_sendq = { NULL, NULL };
MPID_nem_ib_cm_notify_sendq_t MPID_nem_ib_cm_notify_sendq = { NULL, NULL };

int MPID_nem_ib_ncqe_scratch_pad_to_drain;
#endif
MPID_nem_ib_ringbuf_sendq_t MPID_nem_ib_ringbuf_sendq = { NULL, NULL };


int MPID_nem_ib_ncqe_scratch_pad;
int MPID_nem_ib_ncqe_to_drain;
int MPID_nem_ib_ncqe_nces;
MPID_nem_ib_lmtq_t MPID_nem_ib_lmtq = { NULL, NULL };
MPID_nem_ib_lmtq_t MPID_nem_ib_lmt_orderq = { NULL, NULL };
uint8_t MPID_nem_ib_lmt_tail_addr_cbf[MPID_nem_ib_cbf_nslot * MPID_nem_ib_cbf_bitsperslot /
                                      8] = { 0 };
static uint32_t MPID_nem_ib_rand_next = 1;
MPID_nem_ib_vc_area *MPID_nem_ib_debug_current_vc_ib;
uint64_t MPID_nem_ib_ringbuf_acquired[(MPID_NEM_IB_NRINGBUF + 63) / 64];
uint64_t MPID_nem_ib_ringbuf_allocated[(MPID_NEM_IB_NRINGBUF + 63) / 64];
MPID_nem_ib_ringbuf_t *MPID_nem_ib_ringbuf;

uint8_t MPID_nem_ib_rand()
{
    //return 0xaa;
    MPID_nem_ib_rand_next = MPID_nem_ib_rand_next * 1103515245 + 12345;
    return (MPID_nem_ib_rand_next / 65536) % 256;
}

uint64_t MPID_nem_ib_rdtsc()
{
    uint64_t x;
    __asm__ __volatile__("rdtsc; shl $32, %%rdx; or %%rdx, %%rax":"=a"(x)::"%rdx", "memory");   /* rdtsc cannot be executed earlier than here */
    return x;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_kvs_put_binary
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_ib_kvs_put_binary(int from, const char *postfix, const uint8_t * buf,
                                      int length)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    char *kvs_name;
    char key[256], val[256], str[256];
    int j;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_KVS_PUT_BINARY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_KVS_PUT_BINARY);

    mpi_errno = MPIDI_PG_GetConnKVSname(&kvs_name);
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPIDI_PG_GetConnKVSname");
    //dprintf("kvs_put_binary,kvs_name=%s\n", kvs_name);

    sprintf(key, "bc/%d/%s", from, postfix);
    val[0] = 0;
    for (j = 0; j < length; j++) {
        sprintf(str, "%02x", buf[j]);
        strcat(val, str);
    }
    //dprintf("kvs_put_binary,rank=%d,from=%d,PMI_KVS_Put(%s, %s, %s)\n", MPID_nem_ib_myrank, from,
    //kvs_name, key, val);
    pmi_errno = PMI_KVS_Put(kvs_name, key, val);
    MPIU_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**PMI_KVS_Put");
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_KVS_PUT_BINARY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_kvs_get_binary
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_ib_kvs_get_binary(int from, const char *postfix, char *buf, int length)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    char *kvs_name;
    char key[256], val[256], str[256];
    int j;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_KVS_GET_BINARY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_KVS_GET_BINARY);

    mpi_errno = MPIDI_PG_GetConnKVSname(&kvs_name);
    //dprintf("kvs_get_binary,kvs_name=%s\n", kvs_name);
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPIDI_PG_GetConnKVSname");

    sprintf(key, "bc/%d/%s", from, postfix);
    pmi_errno = PMI_KVS_Get(kvs_name, key, val, 256);
    //dprintf("kvs_put_binary,rank=%d,from=%d,PMI_KVS_Get(%s, %s, %s)\n", MPID_nem_ib_myrank, from,
    //kvs_name, key, val);
    MPIU_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**PMS_KVS_Get");

    dprintf("rank=%d,obtained val=%s\n", MPID_nem_ib_myrank, val);
    char *strp = val;
    for (j = 0; j < length; j++) {
        memcpy(str, strp, 2);
        str[2] = 0;
        buf[j] = strtol(str, NULL, 16);
        strp += 2;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_KVS_GET_BINARY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#ifndef MPID_NEM_IB_ONDEMAND
static int MPID_nem_ib_announce_network_addr(int my_rank, char **bc_val_p, int *val_max_sz_p);
#endif

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_init(MPIDI_PG_t * pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno = 0, pmi_errno;
    int i, j, k;
    int ib_port = 1;

    MPIU_CHKPMEM_DECL(6);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_INIT);

    /* first make sure that our private fields in the vc fit into the area provided  */
    MPIU_Assert(sizeof(MPID_nem_ib_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);

    MPID_nem_ib_nranks = pg_p->size;
    MPID_nem_ib_myrank = pg_rank;
    MPID_nem_ib_tsc_poll = MPID_nem_ib_rdtsc();
    MPID_nem_ib_ncqe = 0;
    MPID_nem_ib_ncqe_to_drain = 0;
    MPID_nem_ib_ncqe_nces = 0;
    MPID_nem_ib_ncqe_scratch_pad = 0;
    MPID_nem_ib_ncqe_scratch_pad_to_drain = 0;
    //    MPID_nem_ib_npollingset = 0;
    MPID_nem_ib_progress_engine_vt = 0;
    MPID_nem_ib_remote_poll_shared = 0;
#ifdef MPID_NEM_IB_ONDEMAND
    MPID_nem_ib_cm_ringbuf_head = 0;
    MPID_nem_ib_cm_ringbuf_tail = -1;   /* it means slot 0 is not acquired */
    memset(MPID_nem_ib_cm_ringbuf_released, 0, (MPID_NEM_IB_CM_NSEG + 63) / 64);
#endif

    /* no need to malloc scratch-pad when the number of rank is '1' */
    if (pg_p->size == 1) {
        goto fn_exit;
    }

    /* malloc scratch-pad fd */
    MPIU_CHKPMEM_MALLOC(MPID_nem_ib_scratch_pad_fds, int *, MPID_nem_ib_nranks * sizeof(int),
                        mpi_errno, "connection table");
    memset(MPID_nem_ib_scratch_pad_fds, 0, MPID_nem_ib_nranks * sizeof(int));

    MPIU_CHKPMEM_MALLOC(MPID_nem_ib_scratch_pad_ibcoms, MPID_nem_ib_com_t **,
                        MPID_nem_ib_nranks * sizeof(MPID_nem_ib_com_t *),
                        mpi_errno, "connection table");
    memset(MPID_nem_ib_scratch_pad_ibcoms, 0, MPID_nem_ib_nranks * sizeof(MPID_nem_ib_com_t *));

    /* prepare scrath-pad QP and malloc scratch-pad */
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        if (i == MPID_nem_ib_myrank) {
            continue;
        }
        dprintf("init,MPID_nem_ib_myrank=%d,i=%d\n", MPID_nem_ib_myrank, i);
        ibcom_errno =
            MPID_nem_ib_com_open(ib_port, MPID_NEM_IB_COM_OPEN_SCRATCH_PAD,
                                 &MPID_nem_ib_scratch_pad_fds[i]);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_open");
        MPID_nem_ib_scratch_pad_fds_ref_count++;

        ibcom_errno =
            MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_scratch_pad_fds[i],
                                           &MPID_nem_ib_scratch_pad_ibcoms[i]);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                            "**MPID_nem_ib_com_obtain_pointer");


        ibcom_errno = MPID_nem_ib_com_alloc(MPID_nem_ib_scratch_pad_fds[i],
#ifdef MPID_NEM_IB_ONDEMAND
                                            MPID_NEM_IB_CM_OFF_CMD +
                                            MPID_NEM_IB_CM_NSEG * sizeof(MPID_nem_ib_cm_cmd_t) +
                                            sizeof(MPID_nem_ib_ringbuf_headtail_t)
#else
                                            MPID_nem_ib_nranks * sizeof(MPID_nem_ib_com_qp_state_t)
#endif
);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_alloc");
    }
#ifdef MPID_NEM_IB_ONDEMAND
    /* Release CAS word */
    *((uint64_t *) MPID_nem_ib_scratch_pad) = MPID_NEM_IB_CM_RELEASED;
#endif
    /* Initialize head and tail pointer of shared ring buffer */
    MPID_nem_ib_ringbuf_headtail_t *headtail =
        (MPID_nem_ib_ringbuf_headtail_t *) ((uint8_t *) MPID_nem_ib_scratch_pad +
                                            MPID_NEM_IB_RINGBUF_OFF_HEAD);
    headtail->head = 0;
    headtail->tail = -1;

    /* put bc/me/sp/{gid,lid} put bc/me/sp/{qpn,rmem,rkey}/you */
    int nranks;

    uint32_t my_qpnum;
    uint16_t my_lid;
    union ibv_gid my_gid;
    void *my_rmem;
    int my_rkey;

    int remote_qpnum;
    uint16_t remote_lid;
    union ibv_gid remote_gid;
    void *remote_rmem;
    int remote_rkey;

    char *remote_rank_str;
    char *key_str;

    /* count maximum length of the string representation of remote_rank */
    for (i = 0, nranks = MPID_nem_ib_nranks; nranks > 0; nranks /= 10, i++) {
    }
    MPIU_CHKPMEM_MALLOC(remote_rank_str, char *, 1 + i + 1, mpi_errno, "connection table");
    MPIU_CHKPMEM_MALLOC(key_str, char *, strlen("sp/rmem") + 1 + i + 1, mpi_errno,
                        "connection table");

    for (i = 0; i < MPID_nem_ib_nranks; i++) {

        if (i == 0) {
            ibcom_errno =
                MPID_nem_ib_com_get_info_conn(MPID_nem_ib_scratch_pad_fds[i],
                                              MPID_NEM_IB_COM_INFOKEY_PORT_LID, &my_lid,
                                              sizeof(uint16_t));
            dprintf("ib_init,scratch pad,lid=%04x\n", my_lid);
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_com_get_info_conn");

            mpi_errno =
                MPID_nem_ib_kvs_put_binary(MPID_nem_ib_myrank, "sp/lid", (uint8_t *) & my_lid,
                                           sizeof(uint16_t));
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_kvs_put_binary");

            {
                dprintf("ib_init,scratch pad,put <%d/sp/lid/,%04x>\n", MPID_nem_ib_myrank,
                        (int) my_lid);
            }

            ibcom_errno =
                MPID_nem_ib_com_get_info_conn(MPID_nem_ib_scratch_pad_fds[i],
                                              MPID_NEM_IB_COM_INFOKEY_PORT_GID, &my_gid,
                                              sizeof(union ibv_gid));
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_com_get_info_conn");

            mpi_errno =
                MPID_nem_ib_kvs_put_binary(MPID_nem_ib_myrank, "sp/gid", (uint8_t *) & my_gid,
                                           sizeof(union ibv_gid));
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_kvs_put_binary");

            dprintf("ib_init,scratch pad,gid ");
            for (k = 0; k < 16; k++) {
                dprintf("%02x", (int) my_gid.raw[k]);
            }
            dprintf("\n");
        }

        /* put bc/me/sp/qpn/you  */
        strcpy(key_str, "sp/qpn");
        sprintf(remote_rank_str, "/%x", i);
        strcat(key_str, remote_rank_str);
        ibcom_errno =
            MPID_nem_ib_com_get_info_conn(MPID_nem_ib_scratch_pad_fds[i],
                                          MPID_NEM_IB_COM_INFOKEY_QP_QPN, &my_qpnum,
                                          sizeof(uint32_t));
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                            "**MPID_nem_ib_com_get_info_conn");
        dprintf("ib_init,scratch pad,qpn=%08x\n", my_qpnum);

        mpi_errno =
            MPID_nem_ib_kvs_put_binary(MPID_nem_ib_myrank, key_str, (uint8_t *) & my_qpnum,
                                       sizeof(uint32_t));
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_kvs_put_binary");
        dprintf("ib_init,scratch pad,kvs put done\n");

        strcpy(key_str, "sp/rmem");
        sprintf(remote_rank_str, "/%x", i);
        strcat(key_str, remote_rank_str);

        ibcom_errno =
            MPID_nem_ib_com_get_info_mr(MPID_nem_ib_scratch_pad_fds[i],
                                        MPID_NEM_IB_COM_SCRATCH_PAD_TO,
                                        MPID_NEM_IB_COM_INFOKEY_MR_ADDR, &my_rmem, sizeof(void *));
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_get_info_mr");

        dprintf("ib_init,scratch_pad,rmem=%p\n", my_rmem);
        mpi_errno =
            MPID_nem_ib_kvs_put_binary(MPID_nem_ib_myrank, key_str, (uint8_t *) & my_rmem,
                                       sizeof(void *));
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_kvs_put_binary");

        strcpy(key_str, "sp/rkey");
        sprintf(remote_rank_str, "/%x", i);
        strcat(key_str, remote_rank_str);

        ibcom_errno =
            MPID_nem_ib_com_get_info_mr(MPID_nem_ib_scratch_pad_fds[i],
                                        MPID_NEM_IB_COM_SCRATCH_PAD_TO,
                                        MPID_NEM_IB_COM_INFOKEY_MR_RKEY, &my_rkey, sizeof(int));
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_get_info_mr");
        dprintf("ib_init,scratch_pad,rkey=%08x\n", my_rkey);

        mpi_errno =
            MPID_nem_ib_kvs_put_binary(MPID_nem_ib_myrank, key_str, (uint8_t *) & my_rkey,
                                       sizeof(uint32_t));
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_kvs_put_binary");
    }

    /* wait until key-value propagates among all ranks */
    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**PMI_Barrier");
    dprintf("ib_init,put KVS;barrier;\n");

    /* make me-to-you scratch-pad QP RTS */
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        if (i != MPID_nem_ib_myrank) {

            mpi_errno =
                MPID_nem_ib_kvs_get_binary(i, "sp/gid", (char *) &remote_gid,
                                           sizeof(union ibv_gid));
            dprintf("ib_init,after kvs get\n");
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }

            mpi_errno =
                MPID_nem_ib_kvs_get_binary(i, "sp/lid", (char *) &remote_lid, sizeof(uint16_t));
            dprintf("ib_init,after kvs get\n");
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }

            strcpy(key_str, "sp/qpn");
            strcat(key_str, "");        /* "" or "lmt-put" */
            sprintf(remote_rank_str, "/%x", MPID_nem_ib_myrank);
            strcat(key_str, remote_rank_str);
            mpi_errno =
                MPID_nem_ib_kvs_get_binary(i, key_str, (char *) &remote_qpnum, sizeof(uint32_t));
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }
            dprintf("ib_init,get KVS,remote_qpnum=%08x\n", remote_qpnum);

            ibcom_errno =
                MPID_nem_ib_com_rts(MPID_nem_ib_scratch_pad_fds[i], remote_qpnum, remote_lid,
                                    &remote_gid);
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_rts");

            strcpy(key_str, "sp/rmem");
            sprintf(remote_rank_str, "/%x", MPID_nem_ib_myrank);
            strcat(key_str, remote_rank_str);
            mpi_errno =
                MPID_nem_ib_kvs_get_binary(i, key_str, (char *) &remote_rmem, sizeof(void *));
            dprintf("ib_init,after kvs get\n");
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }
            dprintf("ib_init,get KVS,remote_rmem=%p\n", remote_rmem);

            strcpy(key_str, "sp/rkey");
            sprintf(remote_rank_str, "/%x", MPID_nem_ib_myrank);
            strcat(key_str, remote_rank_str);
            mpi_errno =
                MPID_nem_ib_kvs_get_binary(i, key_str, (char *) &remote_rkey, sizeof(uint32_t));
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }
            dprintf("ib_init,get KVS,remote_rkey=%08x\n", remote_rkey);

            ibcom_errno =
                MPID_nem_ib_com_reg_mr_connect(MPID_nem_ib_scratch_pad_fds[i], remote_rmem,
                                               remote_rkey);
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_com_reg_mr_connect");
        }
    }

    /* wait until you-to-me scratch-pad QP becomes RTR */
    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**PMI_Barrier");

    MPIU_CHKPMEM_MALLOC(MPID_nem_ib_conns, MPID_nem_ib_conn_t *,
                        MPID_nem_ib_nranks * sizeof(MPID_nem_ib_conn_t), mpi_errno,
                        "connection table");
    memset(MPID_nem_ib_conns, 0, MPID_nem_ib_nranks * sizeof(MPID_nem_ib_conn_t));

    /* post receive request */
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        if (i != MPID_nem_ib_myrank) {
            for (j = 0; j < MPID_NEM_IB_COM_MAX_RQ_CAPACITY; j++) {
                MPID_nem_ib_com_scratch_pad_recv(MPID_nem_ib_scratch_pad_fds[i], sizeof(MPID_nem_ib_cm_notify_send_t));
            }
        }
    }

#if 0
    MPIU_CHKPMEM_MALLOC(MPID_nem_ib_pollingset, MPIDI_VC_t **,
                        MPID_NEM_IB_MAX_POLLINGSET * sizeof(MPIDI_VC_t *), mpi_errno,
                        "connection table");
    memset(MPID_nem_ib_pollingset, 0, MPID_NEM_IB_MAX_POLLINGSET * sizeof(MPIDI_VC_t *));
#endif
#ifndef MPID_NEM_IB_ONDEMAND
    /* prepare eager-send QP */
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        ibcom_errno =
            MPID_nem_ib_com_open(ib_port, MPID_NEM_IB_COM_OPEN_RC, &MPID_nem_ib_conns[i].fd);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_open");
        MPID_nem_ib_conns_ref_count++;
        dprintf("init,fd=%d\n", MPID_nem_ib_conns[i].fd);
    }

    /* put bc/me/{gid,lid}, put bc/me/{qpn,rmem,rkey}/you */
    mpi_errno = MPID_nem_ib_announce_network_addr(pg_rank, bc_val_p, val_max_sz_p);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* wait until key-value propagates among all ranks */
    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**PMI_Barrier");

    /* make me-to-you eager-send QP RTS */
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        if (i != MPID_nem_ib_myrank) {

            mpi_errno =
                MPID_nem_ib_kvs_get_binary(i, MPID_NEM_IB_LID_KEY, (char *) &remote_lid,
                                           sizeof(uint16_t));
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }
            mpi_errno =
                MPID_nem_ib_kvs_get_binary(i, MPID_NEM_IB_GID_KEY, (char *) &remote_gid,
                                           sizeof(union ibv_gid));
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }

            strcpy(key_str, MPID_NEM_IB_RMEM_KEY);
            sprintf(remote_rank_str, "/%x", MPID_nem_ib_myrank);
            strcat(key_str, remote_rank_str);
            mpi_errno =
                MPID_nem_ib_kvs_get_binary(i, key_str, (char *) &remote_rmem, sizeof(void *));
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }

            strcpy(key_str, MPID_NEM_IB_RKEY_KEY);
            sprintf(remote_rank_str, "/%x", MPID_nem_ib_myrank);
            strcat(key_str, remote_rank_str);
            mpi_errno =
                MPID_nem_ib_kvs_get_binary(i, key_str, (char *) &remote_rkey, sizeof(uint32_t));
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }

            strcpy(key_str, MPID_NEM_IB_QPN_KEY);
            strcat(key_str, "");        /* "" or "lmt-put" */
            sprintf(remote_rank_str, "/%x", MPID_nem_ib_myrank);
            strcat(key_str, remote_rank_str);
            mpi_errno =
                MPID_nem_ib_kvs_get_binary(i, key_str, (char *) &remote_qpnum, sizeof(uint32_t));
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }
            dprintf("remote_qpnum obtained=%08x\n", remote_qpnum);

            ibcom_errno =
                MPID_nem_ib_com_rts(MPID_nem_ib_conns[i].fd, remote_qpnum, remote_lid, &remote_gid);
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_rts");

            /* report me-to-you eager-send QP becomes RTR */
            MPID_nem_ib_com_qp_state_t state = {.state = MPID_NEM_IB_COM_QP_STATE_RTR };
            ibcom_errno =
                MPID_nem_ib_com_put_scratch_pad(MPID_nem_ib_scratch_pad_fds[i],
                                                (uint64_t) MPID_nem_ib_scratch_pad_ibcoms[i],
                                                sizeof(MPID_nem_ib_com_qp_state_t) *
                                                MPID_nem_ib_myrank,
                                                sizeof(MPID_nem_ib_com_qp_state_t),
                                                (void *) &state);
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_com_put_scratch_pad");
            MPID_nem_ib_ncqe_scratch_pad += 1;

            ibcom_errno =
                MPID_nem_ib_com_reg_mr_connect(MPID_nem_ib_conns[i].fd, remote_rmem, remote_rkey);
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_com_reg_mr_connect");
            dprintf("ib_init,after mr_connect for me-to-you eager-send QP\n");

        }
    }

#if 0   /* debug */
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        dprintf("init,fd[%d]=%d\n", i, MPID_nem_ib_conns[i].fd);
    }
#endif
#else /* define(MPID_NEM_IB_ONDEMAND)  */
    /* We need to communicate with all other ranks in close sequence.  */
    MPID_nem_ib_conns_ref_count = MPID_nem_ib_nranks - MPID_nem_mem_region.num_local;

    if (MPID_nem_ib_conns_ref_count == 0) {
        MPIU_Free(MPID_nem_ib_conns);
    }

    for (i = 0; i < MPID_nem_mem_region.num_local; i++) {
        if (MPID_nem_mem_region.local_procs[i] != MPID_nem_ib_myrank) {
            ibcom_errno =
                MPID_nem_ib_com_close(MPID_nem_ib_scratch_pad_fds
                                      [MPID_nem_mem_region.local_procs[i]]);
            if (--MPID_nem_ib_scratch_pad_fds_ref_count == 0) {
                MPIU_Free(MPID_nem_ib_scratch_pad_fds);
                MPIU_Free(MPID_nem_ib_scratch_pad_ibcoms);
            }
        }
    }
#endif

    MPIU_Free(remote_rank_str);
    MPIU_Free(key_str);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_INIT);
    return mpi_errno;
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_get_business_card(int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_GET_BUSINESS_CARD);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_GET_BUSINESS_CARD);
    dprintf("MPID_nem_ib_get_business_card,enter\n");
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_GET_BUSINESS_CARD);
    return mpi_errno;
}

#ifndef MPID_NEM_IB_ONDEMAND
#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_announce_network_addr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_ib_announce_network_addr(int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;
    int i, j, nranks;

    uint32_t my_qpnum;
    uint16_t my_lid;
    union ibv_gid my_gid;
    void *my_rmem;
    int my_rkey;
    char *remote_rank_str;      /* perl -e '$key_str .= $remote_rank;' */
    char *key_str;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_ANNOUNCE_NETWORK_ADDR);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_ANNOUNCE_NETWORK_ADDR);
    MPIU_CHKLMEM_DECL(2);       /* argument is the number of alloca */

    /* count maximum length of the string representation of remote_rank */
    for (i = 0, nranks = MPID_nem_ib_nranks; nranks > 0; nranks /= 10, i++) {
    }
    MPIU_CHKLMEM_MALLOC(remote_rank_str, char *, i + 1, mpi_errno, "key_str");  /* alloca */
    MPIU_CHKLMEM_MALLOC(key_str, char *, strlen(MPID_NEM_IB_QPN_KEY) + i + 1, mpi_errno, "key_str");    /* alloca */

    /* We have one local qp and remote qp for each rank-pair,
     * so a rank should perform
     * remote_qpn = kvs_get($remote_rank . "qpnum/" . $local_rank).
     * a memory area to read from and write to HCA,
     * and a memory area to read from HCA and write to DRAM is
     * associated with each connection, so a rank should perform
     * rkey = kvs_get($remote_rank . "rkey/" . $local_rank)
     * and raddr = kvs_get($remote_rank . "raddr/" . $local_rank). */
    for (i = 0; i < MPID_nem_ib_nranks; i++) {

        /* lid and gid are common for all remote-ranks */
        if (i == 0) {
            ibcom_errno =
                MPID_nem_ib_com_get_info_conn(MPID_nem_ib_conns[i].fd,
                                              MPID_NEM_IB_COM_INFOKEY_PORT_LID, &my_lid,
                                              sizeof(uint16_t));
            dprintf("get_business_card,lid=%04x\n", my_lid);
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_com_get_info_conn");

            mpi_errno =
                MPID_nem_ib_kvs_put_binary(MPID_nem_ib_myrank, MPID_NEM_IB_LID_KEY,
                                           (uint8_t *) & my_lid, sizeof(uint16_t));
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_kvs_put_binary");

            ibcom_errno =
                MPID_nem_ib_com_get_info_conn(MPID_nem_ib_conns[i].fd,
                                              MPID_NEM_IB_COM_INFOKEY_PORT_GID, &my_gid,
                                              sizeof(union ibv_gid));

            dprintf("get_business_card,val_max_sz=%d\n", *val_max_sz_p);
            dprintf("get_business_card,sz=%ld,my_gid=", sizeof(union ibv_gid));
            for (j = 0; j < 16; j++) {
                dprintf("%02x", (int) my_gid.raw[j]);
            }
            dprintf("\n");

            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_com_get_info_conn");

            mpi_errno =
                MPID_nem_ib_kvs_put_binary(MPID_nem_ib_myrank, MPID_NEM_IB_GID_KEY,
                                           (uint8_t *) & my_gid, sizeof(union ibv_gid));
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_kvs_put_binary");
            dprintf("get_business_card,val_max_sz=%d\n", *val_max_sz_p);
        }

        /* we use different RDMA-rbuf for different senders.
         * so announce like this:
         * <"0/qpn/0", 0xa0000>
         * <"0/qpn/1", 0xb0000>
         * <"0/qpn/2", 0xc0000>
         * <"0/qpn/3", 0xd0000>
         */
        strcpy(key_str, MPID_NEM_IB_QPN_KEY);
        sprintf(remote_rank_str, "/%x", i);
        strcat(key_str, remote_rank_str);
        ibcom_errno =
            MPID_nem_ib_com_get_info_conn(MPID_nem_ib_conns[i].fd, MPID_NEM_IB_COM_INFOKEY_QP_QPN,
                                          &my_qpnum, sizeof(uint32_t));
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                            "**MPID_nem_ib_com_get_info_conn");

        mpi_errno =
            MPID_nem_ib_kvs_put_binary(MPID_nem_ib_myrank, key_str, (uint8_t *) & my_qpnum,
                                       sizeof(uint32_t));
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_kvs_put_binary");


        strcpy(key_str, MPID_NEM_IB_RMEM_KEY);
        sprintf(remote_rank_str, "/%x", i);
        strcat(key_str, remote_rank_str);

        ibcom_errno =
            MPID_nem_ib_com_get_info_mr(MPID_nem_ib_conns[i].fd, MPID_NEM_IB_COM_RDMAWR_TO,
                                        MPID_NEM_IB_COM_INFOKEY_MR_ADDR, &my_rmem, sizeof(void *));
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_get_info_mr");

        dprintf("rmem=%p\n", my_rmem);
        mpi_errno =
            MPID_nem_ib_kvs_put_binary(MPID_nem_ib_myrank, key_str, (uint8_t *) & my_rmem,
                                       sizeof(void *));
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_kvs_put_binary");

        strcpy(key_str, MPID_NEM_IB_RKEY_KEY);
        sprintf(remote_rank_str, "/%x", i);
        strcat(key_str, remote_rank_str);

        ibcom_errno =
            MPID_nem_ib_com_get_info_mr(MPID_nem_ib_conns[i].fd, MPID_NEM_IB_COM_RDMAWR_TO,
                                        MPID_NEM_IB_COM_INFOKEY_MR_RKEY, &my_rkey, sizeof(int));
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_get_info_mr");

        mpi_errno =
            MPID_nem_ib_kvs_put_binary(MPID_nem_ib_myrank, key_str, (uint8_t *) & my_rkey,
                                       sizeof(uint32_t));
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_kvs_put_binary");
    }

    MPIU_CHKLMEM_FREEALL();
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_ANNOUNCE_NETWORK_ADDR);
    return mpi_errno;
  fn_fail:
    MPIU_CHKLMEM_FREEALL();
    goto fn_exit;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_connect_to_root(const char *business_card, MPIDI_VC_t * new_vc)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_CONNECT_TO_ROOT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_CONNECT_TO_ROOT);

    dprintf("toroot,%d->%d", MPID_nem_ib_myrank, new_vc->pg_rank);
    /* not implemented */

    //fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_CONNECT_TO_ROOT);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_vc_onconnect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_vc_onconnect(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_VC_ONCONNECT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_VC_ONCONNECT);

    /* store pointer to MPID_nem_ib_com */
    ibcom_errno =
        MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_conns[vc->pg_rank].fd, &VC_FIELD(vc, ibcom));
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_obtain_pointer");

#if 0
    /* Insert into polling set */
    MPIU_ERR_CHKANDJUMP(MPID_nem_ib_npollingset + 1 > MPID_NEM_IB_MAX_POLLINGSET, mpi_errno,
                        MPI_ERR_OTHER, "**MPID_nem_ib_npollingset");
    MPID_nem_ib_pollingset[MPID_nem_ib_npollingset++] = vc;
    //printf("vc_init,%d->%d,vc=%p,npollingset=%d\n", MPID_nem_ib_myrank, vc->pg_rank, vc, MPID_nem_ib_npollingset);
#endif
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_VC_ONCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_vc_init(MPIDI_VC_t * vc)
{
    MPIDI_CH3I_VC *vc_ch = VC_CH(vc);
    int mpi_errno = MPI_SUCCESS;

    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_VC_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_VC_INIT);

    vc_ib->sc = &MPID_nem_ib_conns[vc->pg_rank];

    /* initialize sendq */
    vc_ib->sendq.head = NULL;
    vc_ib->sendq.tail = NULL;
#ifdef MPID_NEM_IB_ONDEMAND
    VC_FIELD(vc, connection_state) = MPID_NEM_IB_CM_CLOSED;
    VC_FIELD(vc, connection_guard) = 0;
#endif
    VC_FIELD(vc, vc_terminate_buf) = NULL;

    /* rank is sent as wr_id and used to obtain vc in poll */
    MPID_nem_ib_conns[vc->pg_rank].vc = vc;

#ifndef MPID_NEM_IB_ONDEMAND
    MPID_nem_ib_vc_onconnect(vc);

    /* wait until you-to-me eager-send QP becomes RTR */
    MPID_nem_ib_com_t *MPID_nem_ib_com_scratch_pad;
    ibcom_errno =
        MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_scratch_pad_fds[vc->pg_rank],
                                       &MPID_nem_ib_com_scratch_pad);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_obtain_pointer");

    int ntrial = 0;
    volatile MPID_nem_ib_com_qp_state_t *rstate =
        (MPID_nem_ib_com_qp_state_t *) ((uint8_t *)
                                        MPID_nem_ib_com_scratch_pad->icom_mem
                                        [MPID_NEM_IB_COM_SCRATCH_PAD_TO] +
                                        vc->pg_rank * sizeof(MPID_nem_ib_com_qp_state_t));
    dprintf("ib_init,rstate=%p,*rstate=%08x\n", rstate, *((uint32_t *) rstate));
    while (rstate->state != MPID_NEM_IB_COM_QP_STATE_RTR) {
        __asm__ __volatile__("pause;":::"memory");
        if (++ntrial > 1024) {
            /* detect RDMA-write failure */
            ibcom_errno = MPID_nem_ib_drain_scq_scratch_pad();
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_drain_scq_scratch_pad");
        }
    }
    dprintf("ib_init,you-to-me eager-send QP is RTR\n");

    /* post MPID_NEM_IB_COM_MAX_SQ_CAPACITY of recv commands beforehand, replenish when retiring them in ib_poll */
    int i;
    for (i = 0; i < MPID_NEM_IB_COM_MAX_RQ_CAPACITY; i++) {
        //dprintf("irecv,%d->%d\n", MPID_nem_ib_myrank, vc->pg_rank);
        ibcom_errno =
            MPID_nem_ib_com_irecv(MPID_nem_ib_conns[vc->pg_rank].fd, (uint64_t) vc->pg_rank);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_irecv");
    }

#endif
    MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

#if 0   /* dead code */
    uint32_t max_msg_sz;
    MPID_nem_ib_com_get_info_conn(MPID_nem_ib_conns[vc->pg_rank].fd,
                                  MPID_NEM_IB_COM_INFOKEY_PATTR_MAX_MSG_SZ, &max_msg_sz,
                                  sizeof(max_msg_sz));
#endif
    VC_FIELD(vc, pending_sends) = 0;

    //MPIU_Assert(sizeof(MPID_nem_ib_netmod_hdr_t) == 8);    /* assumption in ib_ibcom.h */
    MPIU_Assert(sizeof(MPID_nem_ib_netmod_trailer_t) == 1);     /* assumption in ib_ibcom.h */

    uint32_t sz;
#if 0
    /* assumption in released(), must be power of two  */
    sz = MPID_NEM_IB_COM_RDMABUF_NSEG;
    while ((sz & 1) == 0) {
        sz >>= 1;
    }
    sz >>= 1;
    if (sz) {
        MPIU_Assert(0);
    }
#endif

    /* assumption in ib_poll.c, must be power of two */
    for (sz = MPID_NEM_IB_COM_RDMABUF_SZSEG; sz > 0; sz >>= 1) {
        if (sz != 1 && (sz & 1)) {
            MPIU_Assert(0);
        }
    }

    char *val;
    val = getenv("MP2_IBA_EAGER_THRESHOLD");
    vc->eager_max_msg_sz = val ? atoi(val) : MPID_NEM_IB_EAGER_MAX_MSG_SZ;
    vc->ready_eager_max_msg_sz = val ? atoi(val) : MPID_NEM_IB_EAGER_MAX_MSG_SZ;
    dprintf("ib_vc_init,vc->eager_max_msg_sz=%d\n", vc->eager_max_msg_sz);

    /* vc->rndvSend_fn is set in MPID_nem_vc_init (in src/mpid/ch3/channels/nemesis/src/mpid_nem_init.c) */
    ;
    vc->sendNoncontig_fn = MPID_nem_ib_SendNoncontig;

    vc->comm_ops = &comm_ops;


    /* register packet handler */
    vc_ch->pkt_handler = MPID_nem_ib_pkt_handler;
    vc_ch->num_pkt_handlers = MPIDI_NEM_IB_PKT_NUM_PKT_HANDLERS;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_EAGER_SEND] = MPID_nem_ib_PktHandler_EagerSend;
#if 0   /* modification of mpid_nem_lmt.c is required */
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_LMT_RTS] = MPID_nem_ib_pkt_RTS_handler;
#endif
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_PUT] = MPID_nem_ib_PktHandler_Put;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_GET] = MPID_nem_ib_PktHandler_Get;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_GET_RESP] = MPID_nem_ib_PktHandler_GetResp;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_ACCUMULATE] = MPID_nem_ib_PktHandler_Accumulate;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_LMT_GET_DONE] = MPID_nem_ib_pkt_GET_DONE_handler;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_LMT_RTS] = MPID_nem_ib_pkt_RTS_handler;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_REQ_SEQ_NUM] = MPID_nem_ib_PktHandler_req_seq_num;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM] = MPID_nem_ib_PktHandler_reply_seq_num;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_CHG_RDMABUF_OCC_NOTIFY_STATE] =
        MPID_nem_ib_PktHandler_change_rdmabuf_occupancy_notify_state;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_RMA_LMT_GET_DONE] = MPID_nem_ib_pkt_rma_lmt_getdone;

    /* register CH3 send/recv functions */
    vc_ch->iStartContigMsg = MPID_nem_ib_iStartContigMsg;
    vc_ch->iSendContig = MPID_nem_ib_iSendContig;

    /* register CH3--lmt send/recv functions */
    vc_ch->lmt_initiate_lmt = MPID_nem_ib_lmt_initiate_lmt;
    vc_ch->lmt_start_recv = MPID_nem_ib_lmt_start_recv;
    vc_ch->lmt_handle_cookie = MPID_nem_ib_lmt_handle_cookie;
    vc_ch->lmt_done_send = MPID_nem_ib_lmt_done_send;
    vc_ch->lmt_done_recv = MPID_nem_ib_lmt_done_recv;
    vc_ch->lmt_vc_terminated = MPID_nem_ib_lmt_vc_terminated;
    vc_ch->next = NULL;
    vc_ch->prev = NULL;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_VC_INIT);
    return mpi_errno;
    //fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_vc_destroy(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_VC_DESTROY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_VC_DESTROY);
    /* currently do nothing */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_VC_DESTROY);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_vc_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_vc_terminate(MPIDI_VC_t * vc)
{
    dprintf("ib_vc_terminate,pg_rank=%d\n", vc->pg_rank);
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_VC_TERMINATE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_VC_TERMINATE);

    /* Check to make sure that it's OK to terminate the
     * connection without making sure that all sends have been sent */
    /* it is safe to only check command queue because
     * data transactions always proceed after confirming send by MPI_Wait
     * and control transactions always proceed after receiveing reply */
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);

    /* store address of ringbuffer to clear in poll_eager */
    uint16_t remote_poll = 0;
    MPID_nem_ib_ringbuf_t *ringbuf;
    ringbuf = VC_FIELD(vc, ibcom->remote_ringbuf);

    switch (ringbuf->type) {
    case MPID_NEM_IB_RINGBUF_EXCLUSIVE:
        remote_poll = VC_FIELD(vc, ibcom->rsr_seq_num_poll);
        break;
    case MPID_NEM_IB_RINGBUF_SHARED:
        remote_poll = MPID_nem_ib_remote_poll_shared;
        break;
    default:   /* FIXME */
        printf("unknown ringbuf->type\n");
        break;
    }

    /* Decrement because we increment this value in eager_poll. */
    remote_poll--;

    VC_FIELD(vc, vc_terminate_buf) =
        (uint8_t *) ringbuf->start +
        MPID_NEM_IB_COM_RDMABUF_SZSEG * ((uint16_t) (remote_poll % ringbuf->nslot));

    dprintf
        ("vc_terminate,before,%d->%d,diff-rsr=%d,l diff-lsr=%d,sendq_empty=%d,ncqe=%d,pending_sends=%d\n",
         MPID_nem_ib_myrank, vc->pg_rank, MPID_nem_ib_diff16(vc_ib->ibcom->rsr_seq_num_tail,
                                                             vc_ib->ibcom->
                                                             rsr_seq_num_tail_last_sent),
         MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail),
         MPID_nem_ib_sendq_empty(vc_ib->sendq), MPID_nem_ib_ncqe, VC_FIELD(vc, pending_sends));

    /* update remote RDMA-write-to buffer occupancy */
#if 0   /* we can't send it when the other party has closed QP */
    while (MPID_nem_ib_diff16
           (vc_ib->ibcom->rsr_seq_num_tail, vc_ib->ibcom->rsr_seq_num_tail_last_sent) > 0) {
        MPID_nem_ib_send_reply_seq_num(vc);
    }
#endif

    /* update local RDMA-write-to buffer occupancy */
#if 0
    while (MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail) > 0) {
        MPID_nem_ib_poll_eager(vc);
    }
#endif

#ifdef MPID_NEM_IB_ONDEMAND
    MPID_nem_ib_cm_notify_send_req_t *req = MPIU_Malloc(sizeof(MPID_nem_ib_cm_notify_send_req_t));
    req->ibcom = MPID_nem_ib_scratch_pad_ibcoms[vc->pg_rank];
    req->my_rank = MPID_nem_ib_myrank;
    req->pg_rank = vc->pg_rank;
    MPID_nem_ib_cm_notify_sendq_enqueue(&MPID_nem_ib_cm_notify_sendq, req);
#endif

    /* Empty sendq */
    while (!MPID_nem_ib_sendq_empty(vc_ib->sendq) ||
           VC_FIELD(vc, pending_sends) > 0 ||
           (MPID_nem_ib_scratch_pad_ibcoms[vc->pg_rank]->notify_outstanding_tx_empty !=
            NOTIFY_OUTSTANDING_TX_COMP)) {
#ifdef MPID_NEM_IB_ONDEMAND
        MPID_nem_ib_cm_notify_progress();       /* progress cm_notify_sendq */
        MPID_nem_ib_cm_drain_rcq();
#endif
        /* mimic ib_poll because vc_terminate might be called from ib_poll_eager */
        mpi_errno = MPID_nem_ib_send_progress(vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_send_progress");
        ibcom_errno = MPID_nem_ib_drain_scq(0);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_drain_scq");
#ifdef MPID_NEM_IB_ONDEMAND
        ibcom_errno = MPID_nem_ib_cm_poll_syn();
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_cm_poll_syn");
        ibcom_errno = MPID_nem_ib_cm_poll();
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_cm_poll");
        ibcom_errno = MPID_nem_ib_cm_progress();
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_cm_progress");
        ibcom_errno = MPID_nem_ib_cm_drain_scq();
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_cm_drain_scq");
#endif
        ibcom_errno = MPID_nem_ib_ringbuf_progress();
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                            "**MPID_nem_ib_ringbuf_progress");

        MPID_nem_ib_progress_engine_vt += 1;    /* Progress virtual time */
    }

    dprintf("init,middle,%d->%d,r rdmaocc=%d,l rdmaocc=%d,sendq=%d,ncqe=%d,pending_sends=%d\n",
            MPID_nem_ib_myrank, vc->pg_rank,
            MPID_nem_ib_diff16(vc_ib->ibcom->rsr_seq_num_tail,
                               vc_ib->ibcom->rsr_seq_num_tail_last_sent),
            MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail),
            MPID_nem_ib_sendq_empty(vc_ib->sendq), MPID_nem_ib_ncqe, VC_FIELD(vc, pending_sends));

#if 0
    if (MPID_nem_ib_ncqe > 0 || VC_FIELD(vc, pending_sends) > 0) {
        usleep(1000);
        MPID_nem_ib_drain_scq(0);
    }
#endif

    dprintf("init,middle2,%d->%d,r rdmaocc=%d,l rdmaocc=%d,sendq=%d,ncqe=%d,pending_sends=%d\n",
            MPID_nem_ib_myrank, vc->pg_rank,
            MPID_nem_ib_diff16(vc_ib->ibcom->rsr_seq_num_tail,
                               vc_ib->ibcom->rsr_seq_num_tail_last_sent),
            MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail),
            MPID_nem_ib_sendq_empty(vc_ib->sendq), MPID_nem_ib_ncqe, VC_FIELD(vc, pending_sends));

    if (MPID_nem_ib_ncqe > 0 || VC_FIELD(vc, pending_sends) > 0) {
        usleep(1000);
        MPID_nem_ib_drain_scq(0);
    }
#if 0
    /* drain scq */
    while (MPID_nem_ib_ncqe > 0 || VC_FIELD(vc, pending_sends) > 0) {
        usleep(1000);
        MPID_nem_ib_drain_scq(0);
        //printf("%d\n", VC_FIELD(vc, pending_sends));
        //printf("%d\n", MPID_nem_ib_ncqe);
    }
#endif

    dprintf("init,after ,%d->%d,r rdmaocc=%d,l rdmaocc=%d,sendq=%d,ncqe=%d,pending_sends=%d\n",
            MPID_nem_ib_myrank, vc->pg_rank,
            MPID_nem_ib_diff16(vc_ib->ibcom->rsr_seq_num_tail,
                               vc_ib->ibcom->rsr_seq_num_tail_last_sent),
            MPID_nem_ib_diff16(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail),
            MPID_nem_ib_sendq_empty(vc_ib->sendq), MPID_nem_ib_ncqe, VC_FIELD(vc, pending_sends));

    /* drain scratch-pad scq */
#ifdef MPID_NEM_IB_ONDEMAND
    ibcom_errno = MPID_nem_ib_cm_drain_scq();
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_cm_drain_scq");
    dprintf("init,scratch_pad,ncqe=%d,to_drain=%d\n", MPID_nem_ib_ncqe_scratch_pad,
            MPID_nem_ib_ncqe_scratch_pad_to_drain);
    dprintf("init,scratch_pad,ncom_scratch_pad=%d\n",
            MPID_nem_ib_scratch_pad_ibcoms[vc->pg_rank]->ncom_scratch_pad);
#else
    ibcom_errno = MPID_nem_ib_drain_scq_scratch_pad();
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                        "**MPID_nem_ib_drain_scq_scratch_pad");
#endif

    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

#if 0   /* We move this code to the end of poll_eager. */
    /* Destroy VC QP */

    /* Destroy ring-buffer */
    ibcom_errno = MPID_nem_ib_ringbuf_free(vc);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_ringbuf_free");

    /* Check connection status stored in VC when on-demand connection is used */
    dprintf("vc_terminate,%d->%d,close\n", MPID_nem_ib_myrank, vc->pg_rank);
    ibcom_errno = MPID_nem_ib_com_close(vc_ib->sc->fd);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_close");

    /* Destroy array of scratch-pad QPs */
    MPIU_Assert(MPID_nem_ib_conns_ref_count > 0);
    if (--MPID_nem_ib_conns_ref_count == 0) {
        MPIU_Free(MPID_nem_ib_conns);
    }

    /* TODO don't create them for shared memory vc */

    /* Destroy scratch-pad */
    ibcom_errno = MPID_nem_ib_com_free(MPID_nem_ib_scratch_pad_fds[vc->pg_rank],
#ifdef MPID_NEM_IB_ONDEMAND
                                       MPID_NEM_IB_CM_OFF_CMD +
                                       MPID_NEM_IB_CM_NSEG * sizeof(MPID_nem_ib_cm_cmd_t) +
                                       sizeof(MPID_nem_ib_ringbuf_headtail_t)
#else
                                       MPID_nem_ib_nranks * sizeof(MPID_nem_ib_com_qp_state_t)
#endif
);

    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_free");

    /* Destroy scratch-pad QP */
    ibcom_errno = MPID_nem_ib_com_close(MPID_nem_ib_scratch_pad_fds[vc->pg_rank]);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_close");

    /* Destroy array of scratch-pad QPs */
    MPIU_Assert(MPID_nem_ib_scratch_pad_fds_ref_count > 0);
    if (--MPID_nem_ib_scratch_pad_fds_ref_count == 0) {
        MPIU_Free(MPID_nem_ib_scratch_pad_fds);
        MPIU_Free(MPID_nem_ib_scratch_pad_ibcoms);
    }
#endif
    dprintf("vc_terminate,exit\n");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_VC_TERMINATE);
    return mpi_errno;
  fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;

}
