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
MPIDI_VC_t **MPID_nem_ib_pollingset;
int MPID_nem_ib_conn_ud_fd;
MPID_nem_ib_com_t *MPID_nem_ib_conn_ud_MPID_nem_ib_com;
int MPID_nem_ib_npollingset;
int *MPID_nem_ib_scratch_pad_fds;
//char *MPID_nem_ib_recv_buf;
int MPID_nem_ib_myrank;
uint64_t MPID_nem_ib_tsc_poll;
int MPID_nem_ib_ncqe;
int MPID_nem_ib_ncqe_lmt_put;
#ifdef MPID_NEM_IB_ONDEMAND
MPID_nem_ib_cm_map_t MPID_nem_ib_cm_state;
int MPID_nem_ib_ncqe_connect;
#endif
int MPID_nem_ib_ncqe_scratch_pad;
int MPID_nem_ib_ncqe_to_drain;
int MPID_nem_ib_ncqe_nces;
MPID_nem_ib_lmtq_t MPID_nem_ib_lmtq = { NULL, NULL };
MPID_nem_ib_lmtq_t MPID_nem_ib_lmt_orderq = { NULL, NULL };
uint8_t MPID_nem_ib_lmt_tail_addr_cbf[MPID_nem_ib_cbf_nslot * MPID_nem_ib_cbf_bitsperslot /
                                      8] = { 0 };
static uint32_t MPID_nem_ib_rand_next = 1;
MPID_nem_ib_vc_area *MPID_nem_ib_debug_current_vc_ib;
static int listen_fd;
static int listen_port;

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
    dprintf("kvs_put_binary,kvs_name=%s\n", kvs_name);

    sprintf(key, "bc/%d/%s", from, postfix);
    val[0] = 0;
    for (j = 0; j < length; j++) {
        sprintf(str, "%02x", buf[j]);
        strcat(val, str);
    }
    dprintf("kvs_put_binary,rank=%d,from=%d,PMI_KVS_Put(%s, %s, %s)\n", MPID_nem_ib_myrank, from,
            kvs_name, key, val);
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
    dprintf("kvs_get_binary,kvs_name=%s\n", kvs_name);
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPIDI_PG_GetConnKVSname");

    sprintf(key, "bc/%d/%s", from, postfix);
    pmi_errno = PMI_KVS_Get(kvs_name, key, val, 256);
    dprintf("kvs_put_binary,rank=%d,from=%d,PMI_KVS_Get(%s, %s, %s)\n", MPID_nem_ib_myrank, from,
            kvs_name, key, val);
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

static int MPID_nem_ib_announce_network_addr(int my_rank, char **bc_val_p, int *val_max_sz_p);

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_init(MPIDI_PG_t * pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno = 0, pmi_errno;
    int ret;
    int i, j, k;
    int ib_port = 1;

    MPIU_CHKPMEM_DECL(7);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_INIT);

    /* first make sure that our private fields in the vc fit into the area provided  */
    MPIU_Assert(sizeof(MPID_nem_ib_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);

    MPID_nem_ib_nranks = pg_p->size;
    MPID_nem_ib_myrank = pg_rank;
    MPID_nem_ib_tsc_poll = MPID_nem_ib_rdtsc();
    MPID_nem_ib_ncqe = 0;
    MPID_nem_ib_ncqe_lmt_put = 0;
#ifdef MPID_NEM_IB_ONDEMAND
    MPID_nem_ib_ncqe_connect = 0;
#endif
    MPID_nem_ib_ncqe_scratch_pad = 0;
    MPID_nem_ib_ncqe_to_drain = 0;
    MPID_nem_ib_ncqe_nces = 0;
    MPID_nem_ib_npollingset = 0;

#ifdef MPID_NEM_IB_ONDEMAND
    /* prepare UD QPN for dynamic connection */
    ibcom_errno = MPID_nem_ib_com_open(ib_port, MPID_NEM_IB_COM_OPEN_UD, &MPID_nem_ib_conn_ud_fd);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_open");
    ibcom_errno =
        MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_conn_ud_fd,
                                       &MPID_nem_ib_conn_ud_MPID_nem_ib_com);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_obtain_pointer");
    ibcom_errno = MPID_nem_ib_com_rts(MPID_nem_ib_conn_ud_fd, 0, 0, 0);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_rts");

    for (i = 0; i < MPID_NEM_IB_COM_MAX_RQ_CAPACITY; i++) {
        ibcom_errno = MPID_nem_ib_com_udrecv(MPID_nem_ib_conn_ud_fd);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_udrecv");
    }

    /* obtain gid, lid, qpn using KVS */
    MPIU_CHKPMEM_MALLOC(MPID_nem_ib_conn_ud, MPID_nem_ib_conn_ud_t *,
                        MPID_nem_ib_nranks * sizeof(MPID_nem_ib_conn_ud_t), mpi_errno,
                        "ud connection table");
    memset(MPID_nem_ib_conn_ud, 0, MPID_nem_ib_nranks * sizeof(MPID_nem_ib_conn_ud_t));

    /* put bc/<my rank>/dcs/gid:lid:qpn */
    uint32_t my_qpnum;
    uint16_t my_lid;
    union ibv_gid my_gid;
    MPID_nem_ib_com_get_info_conn(MPID_nem_ib_conn_ud_fd, MPID_NEM_IB_COM_INFOKEY_QP_QPN, &my_qpnum,
                                  sizeof(uint32_t));
    MPID_nem_ib_com_get_info_conn(MPID_nem_ib_conn_ud_fd, MPID_NEM_IB_COM_INFOKEY_PORT_LID, &my_lid,
                                  sizeof(uint16_t));
    MPID_nem_ib_com_get_info_conn(MPID_nem_ib_conn_ud_fd, MPID_NEM_IB_COM_INFOKEY_PORT_GID, &my_gid,
                                  sizeof(union ibv_gid));

    char *kvs_name;
    mpi_errno = MPIDI_PG_GetConnKVSname(&kvs_name);
    char *key_dcs, val[2 * sizeof(union ibv_gid) + 1 + 4 + 1 + 8 + 1], str[9];

    /* count maximum length of the string representation of remote_rank */
    for (i = 0, nranks = MPID_nem_ib_nranks; nranks > 0; nranks /= 10, i++) {
    }
    MPIU_CHKPMEM_MALLOC(key_dcs, char *, strlen("bc/") + i + strlen("/dcs/gid_lid_qpn") + 1,
                        mpi_errno, "connection table");

    sprintf(key, "bc/%d/dcs/gid_lid_qpn", MPID_nem_ib_myrank);
    val[0] = 0;
    for (j = 0; j < sizeof(union ibv_gid); j++) {
        sprintf(str, "%02x", my_gid.raw[j]);
        strcat(val, str);
    }
    sprintf(str, ":");
    strcat(val, str);
    sprintf(str, "%04x:", my_lid);
    strcat(val, str);
    sprintf(str, "%08x", my_qpnum);
    strcat(val, str);
    dprintf("rank=%d,PMI_KVS_Put(%s, %s, %s)\n", MPID_nem_ib_myrank, kvs_name, key_dcs, val);
    pmi_errno = PMI_KVS_Put(kvs_name, key_dcs, val);
    MPIU_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**PMI_KVS_Put");

    /* wait for key-value to propagate among all ranks */
    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**PMI_Barrier");

    /* obtain GID, LID, QP number for remote UD QP for dynamic connection */
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        if (i != MPID_nem_ib_myrank) {
            sprintf(key_dcs, "bc/%d/dcs/gid_lid_qpn", i);
            pmi_errno = PMI_KVS_Get(kvs_name, key_dcs, val, 256);
            dprintf("pmi_errno=%d\n", pmi_errno);
            MPIU_ERR_CHKANDJUMP(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**PMI_KVS_Get");
            dprintf("rank=%d,obtained val=%s\n", MPID_nem_ib_myrank, val);
            char *strp = val;
            for (j = 0; j < sizeof(union ibv_gid); j++) {
                memcpy(str, strp, 2);
                str[2] = 0;
                MPID_nem_ib_conn_ud[i].gid.raw[j] = strtol(str, NULL, 16);
                strp += 2;
            }
            sscanf(strp, ":%04x:%08x", &MPID_nem_ib_conn_ud[i].lid, &MPID_nem_ib_conn_ud[i].qpn);

            dprintf("remote rank=%d,gid=", i);
            for (j = 0; j < sizeof(union ibv_gid); j++) {
                dprintf("%02x", MPID_nem_ib_conn_ud[i].gid.raw[j]);
            }
            dprintf(",lid=%04x,qpn=%08x\n", MPID_nem_ib_conn_ud[i].lid, MPID_nem_ib_conn_ud[i].qpn);
        }
    }
#endif

    /* malloc scratch-pad fd */
    MPIU_CHKPMEM_MALLOC(MPID_nem_ib_scratch_pad_fds, int *, MPID_nem_ib_nranks * sizeof(int),
                        mpi_errno, "connection table");
    memset(MPID_nem_ib_scratch_pad_fds, 0, MPID_nem_ib_nranks * sizeof(int));

    /* prepare scrath-pad QP and malloc scratch-pad */
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        ibcom_errno =
            MPID_nem_ib_com_open(ib_port, MPID_NEM_IB_COM_OPEN_SCRATCH_PAD,
                                 &MPID_nem_ib_scratch_pad_fds[i]);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_open");

        ibcom_errno =
            MPID_nem_ib_com_alloc(MPID_nem_ib_scratch_pad_fds[i],
                                  MPID_nem_ib_nranks * sizeof(MPID_nem_ib_com_qp_state_t));
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_alloc");
    }

    /* put bc/me/sp/{gid,lid} put bc/me/sp/{qpn,rmem,rkey}/you */
    int nranks;

#ifndef MPID_NEM_IB_ONDEMAND
    uint32_t my_qpnum;
    uint16_t my_lid;
    union ibv_gid my_gid;
#endif
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
    MPIU_CHKPMEM_MALLOC(key_str, char *, strlen("sp/qpn") + 1 + i + 1, mpi_errno,
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

    MPIU_CHKPMEM_MALLOC(MPID_nem_ib_pollingset, MPIDI_VC_t **,
                        MPID_NEM_IB_MAX_POLLINGSET * sizeof(MPIDI_VC_t *), mpi_errno,
                        "connection table");
    memset(MPID_nem_ib_pollingset, 0, MPID_NEM_IB_MAX_POLLINGSET * sizeof(MPIDI_VC_t *));

    /* prepare eager-send QP */
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        ibcom_errno =
            MPID_nem_ib_com_open(ib_port, MPID_NEM_IB_COM_OPEN_RC, &MPID_nem_ib_conns[i].fd);
        dprintf("init,fd=%d\n", MPID_nem_ib_conns[i].fd);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_open");
    }

#if 0
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        ibcom_errno =
            MPID_nem_ib_com_open(ib_port, MPID_NEM_IB_COM_OPEN_RC_LMT_PUT,
                                 &MPID_nem_ib_conns[i].fd_lmt_put);
        dprintf("init,fd_lmt_put=%d\n", MPID_nem_ib_conns[i].fd_lmt_put);
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_open");
    }
#endif

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
            MPID_nem_ib_com_t *MPID_nem_ib_com_scratch_pad;
            ibcom_errno =
                MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_scratch_pad_fds[i],
                                               &MPID_nem_ib_com_scratch_pad);
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                                "**MPID_nem_ib_com_obtain_pointer");

            MPID_nem_ib_com_qp_state_t state = {.state = MPID_NEM_IB_COM_QP_STATE_RTR };
            ibcom_errno =
                MPID_nem_ib_com_put_scratch_pad(MPID_nem_ib_scratch_pad_fds[i],
                                                (uint64_t) MPID_nem_ib_com_scratch_pad,
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

#if 0
            /* CQ, SQ, SCQ for lmt-put */
            strcpy(key_str, MPID_NEM_IB_QPN_KEY);
            strcat(key_str, "lmt-put"); /* "" or "lmt-put" */
            sprintf(remote_rank_str, "/%x", MPID_nem_ib_myrank);
            strcat(key_str, remote_rank_str);
            mpi_errno =
                MPID_nem_ib_kvs_get_binary(i, key_str, (char *) &remote_qpnum, sizeof(uint32_t));
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }

            ibcom_errno =
                MPID_nem_ib_com_rts(MPID_nem_ib_conns[i].fd_lmt_put, remote_qpnum, remote_lid,
                                    &remote_gid);
            MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_rts");
#endif
        }
    }

#if 0   /* debug */
    for (i = 0; i < MPID_nem_ib_nranks; i++) {
        dprintf("init,fd[%d]=%d\n", i, MPID_nem_ib_conns[i].fd);
    }
#endif


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
    int str_errno = MPIU_STR_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_GET_BUSINESS_CARD);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_GET_BUSINESS_CARD);
    dprintf("MPID_nem_ib_get_business_card,enter\n");
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_GET_BUSINESS_CARD);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_announce_network_addr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_ib_announce_network_addr(int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPIU_STR_SUCCESS;
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

#if 0
        /* lmt-put */
        strcpy(key_str, MPID_NEM_IB_QPN_KEY);
        strcat(key_str, "lmt-put");
        sprintf(remote_rank_str, "/%x", i);
        strcat(key_str, remote_rank_str);
        ibcom_errno =
            MPID_nem_ib_com_get_info_conn(MPID_nem_ib_conns[i].fd_lmt_put,
                                          MPID_NEM_IB_COM_INFOKEY_QP_QPN, &my_qpnum,
                                          sizeof(uint32_t));
        MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                            "**MPID_nem_ib_com_get_info_conn");

        mpi_errno =
            MPID_nem_ib_kvs_put_binary(MPID_nem_ib_myrank, key_str, (uint8_t *) & my_qpnum,
                                       sizeof(uint32_t));
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_kvs_put_binary");
#endif

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

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_ANNOUNCE_NETWORK_ADDR);
    return mpi_errno;
  fn_fail:
    MPIU_CHKLMEM_FREEALL();
    goto fn_exit;
}

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
#define FUNCNAME MPID_nem_ib_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_vc_init(MPIDI_VC_t * vc)
{
    MPIDI_CH3I_VC *vc_ch = VC_CH(vc);
    int mpi_errno = MPI_SUCCESS;

    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);
    int ibcom_errno;
    size_t s;
    MPID_nem_ib_conn_t *sc;
    off_t offset;

    int remote_qpnum;
    uint16_t remote_lid;
    union ibv_gid remote_gid;
    void *remote_rmem;
    int remote_rkey;

    char key_str[256], remote_rank_str[256];

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_VC_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_VC_INIT);

    vc_ib->sc = &MPID_nem_ib_conns[vc->pg_rank];

    /* store pointer to MPID_nem_ib_com */
    ibcom_errno = MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_conns[vc->pg_rank].fd, &vc_ib->ibcom);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_obtain_pointer");

    ibcom_errno =
        MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_conns[vc->pg_rank].fd_lmt_put,
                                       &vc_ib->ibcom_lmt_put);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_obtain_pointer");

    //dprintf("vc_init,open,fd=%d,ptr=%p,rsr_seq_num_poll=%d\n", MPID_nem_ib_conns[vc->pg_rank].fd, vc_ib->MPID_nem_ib_com, vc_ib->ibcom->rsr_seq_num_poll);

    /* initialize sendq */
    vc_ib->sendq.head = NULL;
    vc_ib->sendq.tail = NULL;
    vc_ib->sendq_lmt_put.head = NULL;
    vc_ib->sendq_lmt_put.tail = NULL;

    /* rank is sent as wr_id and used to obtain vc in poll */
    MPID_nem_ib_conns[vc->pg_rank].vc = vc;
    MPIU_ERR_CHKANDJUMP(MPID_nem_ib_npollingset + 1 > MPID_NEM_IB_MAX_POLLINGSET, mpi_errno,
                        MPI_ERR_OTHER, "**MPID_nem_ib_npollingset");
    MPID_nem_ib_pollingset[MPID_nem_ib_npollingset++] = vc;
    //printf("vc_init,%d->%d,vc=%p,npollingset=%d\n", MPID_nem_ib_myrank, vc->pg_rank, vc, MPID_nem_ib_npollingset);

    /* wait until you-to-me eager-send QP becomes RTR */
    MPID_nem_ib_com_t *MPID_nem_ib_com_scratch_pad;
    ibcom_errno =
        MPID_nem_ib_com_obtain_pointer(MPID_nem_ib_scratch_pad_fds[vc->pg_rank],
                                       &MPID_nem_ib_com_scratch_pad);
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER, "**MPID_nem_ib_com_obtain_pointer");

    int ntrial = 0;
    volatile MPID_nem_ib_com_qp_state_t *rstate =
        (MPID_nem_ib_com_qp_state_t *) ((uint8_t *) MPID_nem_ib_com_scratch_pad->
                                        icom_mem[MPID_NEM_IB_COM_SCRATCH_PAD_TO] +
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

    MPIDI_CHANGE_VC_STATE(vc, ACTIVE);


    uint32_t max_msg_sz;
    MPID_nem_ib_com_get_info_conn(MPID_nem_ib_conns[vc->pg_rank].fd,
                                  MPID_NEM_IB_COM_INFOKEY_PATTR_MAX_MSG_SZ, &max_msg_sz,
                                  sizeof(max_msg_sz));
    VC_FIELD(vc, pending_sends) = 0;
#ifdef MPID_NEM_IB_ONDEMAND
    VC_FIELD(vc, is_connected) = 0;
#endif

    MPIU_Assert(sizeof(MPID_nem_ib_sz_hdrmagic_t) == 8);    /* assumption in ib_ibcom.h */
    MPIU_Assert(sizeof(MPID_nem_ib_tailmagic_t) == 1);      /* assumption in ib_ibcom.h */

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
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_PUT] = MPID_nem_ib_PktHandler_Put;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_GET] = MPID_nem_ib_PktHandler_Get;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_GET_RESP] = MPID_nem_ib_PktHandler_GetResp;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_ACCUMULATE] = MPID_nem_ib_PktHandler_Accumulate;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_LMT_GET_DONE] = MPID_nem_ib_pkt_GET_DONE_handler;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_REQ_SEQ_NUM] = MPID_nem_ib_PktHandler_req_seq_num;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_REPLY_SEQ_NUM] = MPID_nem_ib_PktHandler_reply_seq_num;
    MPID_nem_ib_pkt_handler[MPIDI_NEM_IB_PKT_CHG_RDMABUF_OCC_NOTIFY_STATE] =
        MPID_nem_ib_PktHandler_change_rdmabuf_occupancy_notify_state;

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
  fn_fail:
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
    dprintf("ib_vc_terminate,enter\n");
    int mpi_errno = MPI_SUCCESS;
    int ibcom_errno;
    int req_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_IB_VC_TERMINATE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_IB_VC_TERMINATE);

    /* Check to make sure that it's OK to terminate the
     * connection without making sure that all sends have been sent */
    /* it is safe to only check command queue because
     * data transactions always proceed after confirming send by MPI_Wait
     * and control transactions always proceed after receiveing reply */
    MPID_nem_ib_vc_area *vc_ib = VC_IB(vc);

    dprintf("init,before,%d->%d,r rdmaocc=%d,l rdmaocc=%d,sendq=%d,ncqe=%d,pending_sends=%d\n",
            MPID_nem_ib_myrank, vc->pg_rank,
            MPID_nem_ib_diff32(vc_ib->ibcom->rsr_seq_num_tail,
                               vc_ib->ibcom->rsr_seq_num_tail_last_sent),
            MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail),
            MPID_nem_ib_sendq_empty(vc_ib->sendq), MPID_nem_ib_ncqe, VC_FIELD(vc, pending_sends));

    /* update remote RDMA-write-to buffer occupancy */
#if 0   /* we can't send it when the other party has closed QP */
    while (MPID_nem_ib_diff32
           (vc_ib->ibcom->rsr_seq_num_tail, vc_ib->ibcom->rsr_seq_num_tail_last_sent) > 0) {
        MPID_nem_ib_send_reply_seq_num(vc);
    }
#endif

    /* update local RDMA-write-to buffer occupancy */
#if 0
    while (MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail) > 0) {
        MPID_nem_ib_poll_eager(vc);
    }
#endif

    /* drain sendq */
    while (!MPID_nem_ib_sendq_empty(vc_ib->sendq)) {
        MPID_nem_ib_send_progress(vc_ib);
    }

    dprintf("init,middle,%d->%d,r rdmaocc=%d,l rdmaocc=%d,sendq=%d,ncqe=%d,pending_sends=%d\n",
            MPID_nem_ib_myrank, vc->pg_rank,
            MPID_nem_ib_diff32(vc_ib->ibcom->rsr_seq_num_tail,
                               vc_ib->ibcom->rsr_seq_num_tail_last_sent),
            MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail),
            MPID_nem_ib_sendq_empty(vc_ib->sendq), MPID_nem_ib_ncqe, VC_FIELD(vc, pending_sends));

    if (MPID_nem_ib_ncqe > 0 || VC_FIELD(vc, pending_sends) > 0) {
        usleep(1000);
        MPID_nem_ib_drain_scq(0);
    }
    dprintf("init,middle2,%d->%d,r rdmaocc=%d,l rdmaocc=%d,sendq=%d,ncqe=%d,pending_sends=%d\n",
            MPID_nem_ib_myrank, vc->pg_rank,
            MPID_nem_ib_diff32(vc_ib->ibcom->rsr_seq_num_tail,
                               vc_ib->ibcom->rsr_seq_num_tail_last_sent),
            MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail),
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
            MPID_nem_ib_diff32(vc_ib->ibcom->rsr_seq_num_tail,
                               vc_ib->ibcom->rsr_seq_num_tail_last_sent),
            MPID_nem_ib_diff32(vc_ib->ibcom->sseq_num, vc_ib->ibcom->lsr_seq_num_tail),
            MPID_nem_ib_sendq_empty(vc_ib->sendq), MPID_nem_ib_ncqe, VC_FIELD(vc, pending_sends));

    /* drain scratch-pad scq */
    ibcom_errno = MPID_nem_ib_drain_scq_scratch_pad();
    MPIU_ERR_CHKANDJUMP(ibcom_errno, mpi_errno, MPI_ERR_OTHER,
                        "**MPID_nem_ib_drain_scq_scratch_pad");

    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_IB_VC_TERMINATE);
    return mpi_errno;
  fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;

}
