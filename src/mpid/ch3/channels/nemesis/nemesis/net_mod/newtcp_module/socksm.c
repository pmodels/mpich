/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define SOCKSM_H_DEFGLOBALS_

#include "newtcp_module_impl.h"
#include "socksm.h"

/* FIXME trace/log all the state transitions */


typedef struct freenode {
    int index;
    struct freenode* next;
} freenode_t;

struct {
    freenode_t *head, *tail;
} freeq = {NULL, NULL};

int g_tbl_size = 0; //DARIUS
static int g_tbl_capacity = CONN_PLFD_TBL_INIT_SIZE;
static int g_tbl_grow_size = CONN_PLFD_TBL_GROW_SIZE;

static sockconn_t *g_sc_tbl = NULL;
pollfd_t *g_plfd_tbl = NULL; //DARIUS

sockconn_t g_lstn_sc;
pollfd_t g_lstn_plfd;

static struct {
    handler_func_t sc_state_handler;
    short sc_state_plfd_events;
} sc_state_info[CONN_STATE_SIZE];

static int is_valid_state (sockconn_t *sc);

#define IS_WRITEABLE(plfd)                      \
    (plfd->revents & POLLOUT) ? 1 : 0

#define IS_READABLE(plfd)                       \
    (plfd->revents & POLLIN) ? 1 : 0

#define IS_READ_WRITEABLE(plfd)                                 \
    (plfd->revents & POLLIN && plfd->revents & POLLOUT) ? 1 : 0

#define IS_SAME_PGID(id1, id2) \
    (strcmp(id1, id2) == 0)

#define IS_SAME_CONNECTION(sc1, sc2)                                    \
    (sc1->pg_rank == sc2->pg_rank &&                                    \
     ((sc1->is_same_pg && sc2->is_same_pg) ||                           \
      (!sc1->is_same_pg && !sc2->is_same_pg &&                          \
       IS_SAME_PGID(sc1->pg_id, sc2->pg_id) == 0)))

#define INIT_SC_ENTRY(sc, ind)                  \
    {                                           \
        sc->fd = CONN_INVALID_FD;               \
        sc->index = ind;                        \
        sc->vc = NULL;                          \
    }

#define INIT_POLLFD_ENTRY(plfd)                               \
    {                                                         \
        plfd->fd = CONN_INVALID_FD;                           \
        plfd->events = POLLIN;                                \
    }

/*
#define DBG_PRINT_CSTATE_ENTER(sc)                                      \
    {                                                                   \
        printf("SM Enter:: rank=%d state=%s\n", sc->pg_rank,            \
               CONN_STATE_STR[sc->state.cstate]);                       \
    }

#define DBG_PRINT_CSTATE_EXIT(sc)                                       \
    {                                                                   \
        printf("SM Exit:: rank=%d state=%s\n", sc->pg_rank,             \
               CONN_STATE_STR[sc->state.cstate]);                       \
    }

#define DBG_PRINT_LSTATE_ENTER(sc)                                      \
    {                                                                   \
        printf("LSM Enter::  state=%s\n",                               \
               LISTEN_STATE_STR[sc->state.lstate]);                     \
    }

#define DBG_PRINT_LSTATE_EXIT(sc)                                       \
    {                                                                   \
        printf("LSM Exit:: state=%s\n",                                 \
               LISTEN_STATE_STR[sc->state.lstate]);                     \
    }
*/

static int find_free_entry(int *index);

#undef FUNCNAME
#define FUNCNAME alloc_sc_plfd_tbls
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int alloc_sc_plfd_tbls (void)
{
    int i, mpi_errno = MPI_SUCCESS, index = -1;
    MPIU_CHKPMEM_DECL (2);

    MPIU_Assert(g_sc_tbl == NULL);
    MPIU_Assert(g_plfd_tbl == NULL);

    MPIU_CHKPMEM_MALLOC (g_sc_tbl, sockconn_t *, g_tbl_capacity * sizeof(sockconn_t), 
                         mpi_errno, "connection table");
    MPIU_CHKPMEM_MALLOC (g_plfd_tbl, pollfd_t *, g_tbl_capacity * sizeof(pollfd_t), 
                         mpi_errno, "pollfd table");
    for (i = 0; i < g_tbl_capacity; i++) {
        
        INIT_SC_ENTRY(((sockconn_t *)&g_sc_tbl[i]), i);
        INIT_POLLFD_ENTRY(((pollfd_t *)&g_plfd_tbl[i]));
    }
    MPIU_CHKPMEM_COMMIT();

    mpi_errno = find_free_entry(&index);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);

    g_sc_tbl[index].fd = g_plfd_tbl[index].fd = g_lstn_plfd.fd;
    g_plfd_tbl[index].events = POLLIN;
    g_sc_tbl[index].handler = g_lstn_sc.handler;

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME free_sc_plfd_tbls
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int free_sc_plfd_tbls (void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Free(g_sc_tbl);
    MPIU_Free(g_plfd_tbl);
    return mpi_errno;
}

/*
  Reason for not doing realloc for both sc and plfd tables :
  Either both the tables have to be expanded or both should remain the same size, if
  enough memory could not be allocated, as we have only one set of variables to control
  the size of the tables. Also, it is not useful to expand one table and leave the other
  at the same size, 'coz of memory allocation failures.
*/
#undef FUNCNAME
#define FUNCNAME expand_sc_plfd_tbls
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int expand_sc_plfd_tbls (void)
{
    int mpi_errno = MPI_SUCCESS; 
    sockconn_t *new_sc_tbl = NULL;
    pollfd_t *new_plfd_tbl = NULL;
    int new_capacity = g_tbl_capacity + g_tbl_grow_size, i;
    MPIU_CHKPMEM_DECL (2);

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "expand_sc_plfd_tbls Entry"));
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "expand_sc_plfd_tbls b4 g_sc_tbl[0].fd=%d", g_sc_tbl[0].fd));
    MPIU_CHKPMEM_MALLOC (new_sc_tbl, sockconn_t *, new_capacity * sizeof(sockconn_t), 
                         mpi_errno, "expanded connection table");
    MPIU_CHKPMEM_MALLOC (new_plfd_tbl, pollfd_t *, new_capacity * sizeof(pollfd_t), 
                         mpi_errno, "expanded pollfd table");

    MPID_NEM_MEMCPY (new_sc_tbl, g_sc_tbl, g_tbl_capacity * sizeof(sockconn_t));
    MPID_NEM_MEMCPY (new_plfd_tbl, g_plfd_tbl, g_tbl_capacity * sizeof(pollfd_t));

    /* VCs have pointers to entries in the sc table.  These
       are updated here after the expand. */
    for (i = 1; i < g_tbl_capacity; i++)   /* i=0 = listening socket fd won't have a VC pointer */
    {
        if (g_sc_tbl[i].vc && VC_FIELD(g_sc_tbl[i].vc, sc))
            VC_FIELD(g_sc_tbl[i].vc, sc) = &new_sc_tbl[i];
    }

    MPIU_Free(g_sc_tbl);
    MPIU_Free(g_plfd_tbl);
    g_sc_tbl = new_sc_tbl;
    g_plfd_tbl = new_plfd_tbl;
    for (i = g_tbl_capacity; i < new_capacity; i++) {
        INIT_SC_ENTRY(((sockconn_t *)&g_sc_tbl[i]), i);
        INIT_POLLFD_ENTRY(((pollfd_t *)&g_plfd_tbl[i]));
    }
    g_tbl_capacity = new_capacity;

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "expand_sc_plfd_tbls af g_sc_tbl[0].fd=%d", g_sc_tbl[0].fd));
    for (i = 0; i < g_tbl_capacity; ++i)
    {
        MPIU_Assert(g_sc_tbl[i].state.cstate != CONN_STATE_TS_COMMRDY || VC_FIELD(g_sc_tbl[i].vc, sc) == &g_sc_tbl[i])
    }
    
    
    MPIU_CHKPMEM_COMMIT();    
 fn_exit:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "expand_sc_plfd_tbls Exit"));
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/*
  Finds the first free entry in the connection table/pollfd table. Note that both the
  tables are indexed the same. i.e. each entry in one table corresponds to the
  entry of the same index in the other table. If an entry is valid in one table, then
  it is valid in the other table too.

  This function finds the first free entry in both the tables by checking the queue of
  free elements. If the free queue is empty, then it returns the next available slot
  in the tables. If the size of the slot is already full, then this expands the table
  and then returns the next available slot
*/
#undef FUNCNAME
#define FUNCNAME find_free_entry
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int find_free_entry(int *index)
{
    int mpi_errno = MPI_SUCCESS;
    freenode_t *node;

    if (!Q_EMPTY(freeq)) {
        Q_DEQUEUE(&freeq, ((freenode_t **)&node)); 
        *index = node->index;
        MPIU_Free(node);
        goto fn_exit;
    }

    if (g_tbl_size == g_tbl_capacity) {
        mpi_errno = expand_sc_plfd_tbls();
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
    }
    *index = g_tbl_size;
    ++g_tbl_size;

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}


/*
  Note: if a new state is added for validating, remeber to call this function within
  debug assert in the corresponding state handler function.
 */
#undef FUNCNAME
#define FUNCNAME is_valid_state
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int is_valid_state (sockconn_t *sc)
{
    int i, found = FALSE;

    for(i = 0; i < g_tbl_size && !found; i++)
    {
        sockconn_t *iter_sc = &g_sc_tbl[i];
        MPID_nem_newtcp_module_Conn_State_t istate = iter_sc->state.cstate;

        if (iter_sc != sc && iter_sc->fd != CONN_INVALID_FD 
            && IS_SAME_CONNECTION(iter_sc, sc))
        {
            switch (sc->state.cstate)
            {
            case CONN_STATE_TC_C_CNTD:
                if (istate == CONN_STATE_TC_C_CNTING ||
                    istate == CONN_STATE_TC_C_CNTD ||
                    istate == CONN_STATE_TC_C_RANKSENT)
                    found = TRUE;
                break;
            case CONN_STATE_TC_C_RANKSENT:
                if (istate == CONN_STATE_TC_C_CNTING ||
                    istate == CONN_STATE_TC_C_CNTD ||
                    istate == CONN_STATE_TC_C_RANKSENT ||
                    istate == CONN_STATE_TS_D_DCNTING ||
                    istate == CONN_STATE_TS_D_REQSENT)
                    found = TRUE;
                break;
            default:
                /* FIXME: need to handle error condition better */
                MPIU_Assert (0);
                break;
            }
        }      
    }

    /*     if found, then the state of the connection is not valid. A bug in the state */
    /*     machine code. */
    return !found;
}

/* Note:
   fnd_sc is returned only for certain states. If it is not returned for a state,
   the handler function can simply pass NULL as the second argument.
 */
#undef FUNCNAME
#define FUNCNAME found_better_sc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int found_better_sc(sockconn_t *sc, sockconn_t **fnd_sc)
{
    int i, found = FALSE;

    for(i = 0; i < g_tbl_size && !found; i++)
    {
        sockconn_t *iter_sc = &g_sc_tbl[i];
        MPID_nem_newtcp_module_Conn_State_t istate = iter_sc->state.cstate;
        if (iter_sc != sc && iter_sc->fd != CONN_INVALID_FD 
            && IS_SAME_CONNECTION(iter_sc, sc))
        {
            switch (sc->state.cstate)
            {
            case CONN_STATE_TC_C_CNTD:
                MPIU_Assert(fnd_sc == NULL);
                if (istate == CONN_STATE_TS_COMMRDY || istate == CONN_STATE_TA_C_RANKRCVD)
                    found = TRUE;
                break;
            case CONN_STATE_TA_C_RANKRCVD:
                MPIU_Assert(fnd_sc != NULL);
                if (istate == CONN_STATE_TS_COMMRDY || istate == CONN_STATE_TC_C_RANKSENT) {
                    found = TRUE;
                    *fnd_sc = iter_sc;
                }
                break;                
                /* Add code for other states here, if need be. */
            default:
                /* FIXME: need to handle error condition better */
                MPIU_Assert (0);
                break;
            }
        }
    }

    return found;
}

#undef FUNCNAME
#define FUNCNAME send_id_info
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_id_info(const sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_nem_newtcp_module_idinfo_t id_info;
    MPIDI_nem_newtcp_module_header_t hdr;
    struct iovec iov[3];
    int pg_id_len = 0, offset, buf_size, iov_cnt = 2;

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "my->pg_rank=%d, sc->pg_rank=%d"
                                             , MPIDI_Process.my_pg_rank, sc->pg_rank));
    if (!sc->is_same_pg)
        pg_id_len = strlen(MPIDI_Process.my_pg->id) + 1; 
/*     store ending NULL also */
/*     FIXME better keep pg_id_len itself as part of MPIDI_Process.my_pg structure to */
/*     avoid computing the length of string everytime this function is called. */
    
    hdr.pkt_type = MPIDI_NEM_NEWTCP_MODULE_PKT_ID_INFO;
    hdr.datalen = sizeof(MPIDI_nem_newtcp_module_idinfo_t) + pg_id_len;    
    id_info.pg_rank = MPIDI_Process.my_pg_rank;

    iov[0].iov_base = &hdr;
    iov[0].iov_len = sizeof(hdr);
    iov[1].iov_base = &id_info;
    iov[1].iov_len = sizeof(id_info);
    buf_size = sizeof(hdr) + sizeof(id_info);

    if (!sc->is_same_pg) {
        iov[2].iov_base = MPIDI_Process.my_pg->id;
        iov[2].iov_len = pg_id_len;
        buf_size += pg_id_len;
        ++iov_cnt;
    }
    
    CHECK_EINTR (offset, writev(sc->fd, iov, iov_cnt));
    MPIU_ERR_CHKANDJUMP1 (offset == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, 
                          "**write", "**write %s", strerror (errno));
    MPIU_ERR_CHKANDJUMP1 (offset != buf_size, mpi_errno, MPI_ERR_OTHER, 
                          "**write", "**write %s", strerror (errno)); 
/*     FIXME log appropriate error */
/*     FIXME-Z1  socket is just connected and we are sending a few bytes. So, there should not */
/*     be a problem of partial data only being written to. If partial data written, */
/*     handle this. */

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d, offset=%d, errno=%d %s", mpi_errno, offset, errno, strerror(errno)));
    goto fn_exit;    
}


#undef FUNCNAME
#define FUNCNAME recv_id_info
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int recv_id_info(sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_nem_newtcp_module_header_t hdr;
    int pg_id_len = 0, nread, iov_cnt = 1;
    int hdr_len = sizeof(MPIDI_nem_newtcp_module_header_t);
    struct iovec iov[2];
    char *pg_id = NULL;
    MPIU_CHKPMEM_DECL (1);

    CHECK_EINTR (nread, read(sc->fd, &hdr, hdr_len));
    MPIU_ERR_CHKANDJUMP1 (nread == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER,
                          "**read", "**read %s", strerror (errno));
    MPIU_ERR_CHKANDJUMP1 (nread != hdr_len, mpi_errno, MPI_ERR_OTHER,
                          "**read", "**read %s", strerror (errno));  /* FIXME-Z1 */
    MPIU_Assert(hdr.pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_INFO);
    MPIU_Assert(hdr.datalen != 0);
    
    iov[0].iov_base = (void *) &(sc->pg_rank);
    iov[0].iov_len = sizeof(sc->pg_rank);
    pg_id_len = hdr.datalen - sizeof(MPIDI_nem_newtcp_module_idinfo_t);
    if (pg_id_len != 0) {
        MPIU_CHKPMEM_MALLOC (pg_id, char *, pg_id_len, mpi_errno, "sockconn pg_id");
        iov[1].iov_base = (void *)pg_id;
        iov[1].iov_len = pg_id_len;
        ++iov_cnt;
    } 
    CHECK_EINTR (nread, readv(sc->fd, iov, iov_cnt));
    MPIU_ERR_CHKANDJUMP1 (nread == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER,
                          "**read", "**read %s", strerror (errno));
    MPIU_ERR_CHKANDJUMP1 (nread != hdr.datalen, mpi_errno, MPI_ERR_OTHER,
                          "**read", "**read %s", strerror (errno)); /* FIXME-Z1 */
    if (pg_id_len == 0) {
        sc->is_same_pg = TRUE;
        MPID_nem_newtcp_module_get_vc_from_conninfo (MPIDI_Process.my_pg->id, 
                                                     sc->pg_rank,&sc->vc);
        sc->pg_id = NULL;
    }
    else {
        sc->is_same_pg = FALSE;
        MPID_nem_newtcp_module_get_vc_from_conninfo (pg_id, sc->pg_rank, &sc->vc);
        sc->pg_id = sc->vc->pg->id;
    }

 fn_exit:
    MPIU_CHKPMEM_REAP();
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/*
  This function is used to send commands that don't have data but just only
  the header.
 */
#undef FUNCNAME
#define FUNCNAME send_cmd_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_cmd_pkt(int fd, MPIDI_nem_newtcp_module_pkt_type_t pkt_type)
{
    int mpi_errno = MPI_SUCCESS, offset;
    MPIDI_nem_newtcp_module_header_t pkt;
    int pkt_len = sizeof(MPIDI_nem_newtcp_module_header_t);

    MPIU_Assert(pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_ACK ||
                pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_NAK ||
                pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_REQ ||
                pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_ACK ||
                pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_NAK);

    pkt.pkt_type = pkt_type;
    pkt.datalen = 0;

    CHECK_EINTR (offset, write(fd, &pkt, pkt_len));
    MPIU_ERR_CHKANDJUMP1 (offset == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER,
                          "**write", "**write %s", strerror (errno));
    MPIU_ERR_CHKANDJUMP1 (offset != pkt_len, mpi_errno, MPI_ERR_OTHER,
                          "**write", "**write %s", strerror (errno)); /* FIXME-Z1 */
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}


/*
  This function is used to recv commands that don't have data but just only
  the header.
 */
#undef FUNCNAME
#define FUNCNAME recv_cmd_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int recv_cmd_pkt(int fd, MPIDI_nem_newtcp_module_pkt_type_t *pkt_type)
{
    int mpi_errno = MPI_SUCCESS, nread;
    MPIDI_nem_newtcp_module_header_t pkt;
    int pkt_len = sizeof(MPIDI_nem_newtcp_module_header_t);

    CHECK_EINTR (nread, read(fd, &pkt, pkt_len));
    MPIU_ERR_CHKANDJUMP1 (nread == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER,
                          "**read", "**read %s", strerror (errno));
    MPIU_ERR_CHKANDJUMP1 (nread != pkt_len, mpi_errno, MPI_ERR_OTHER,
                          "**read", "**read %s", strerror (errno)); /* FIXME-Z1 */
    MPIU_Assert(pkt.datalen == 0);
    MPIU_Assert(pkt.pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_ACK ||
                pkt.pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_NAK ||
                pkt.pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_REQ ||
                pkt.pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_ACK ||
                pkt.pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_NAK);
    *pkt_type = pkt.pkt_type;
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_connect (struct MPIDI_VC *const vc) 
{
    sockconn_t *sc = NULL;
    pollfd_t *plfd = NULL;
    int index = -1;
    int mpi_errno = MPI_SUCCESS;
    freenode_t *node;

    /* MPIU_DBG_MSG(NEM_SOCK_DET, TYPICAL, FCNAME "checking DBG_MSG Enter"); */
    if (((MPIDI_CH3I_VC *)vc->channel_private)->state == MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED) {
        struct sockaddr_in *sock_addr;
        int rc = 0;

        MPIU_Assert(VC_FIELD(vc, sc) == NULL);
        mpi_errno = find_free_entry(&index);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);
        
        sc = &g_sc_tbl[index];
        plfd = &g_plfd_tbl[index];        

        sock_addr = &(VC_FIELD(vc, sock_id));

        CHECK_EINTR(sc->fd, socket(AF_INET, SOCK_STREAM, 0));
        MPIU_ERR_CHKANDJUMP2(sc->fd == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create", 
                             "**sock_create %s %d", strerror(errno), errno);
        plfd->fd = sc->fd;
        mpi_errno = MPID_nem_newtcp_module_set_sockopts(sc->fd);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);

        sc->g_sc_tbl = g_sc_tbl;
        sc->g_plfd_tbl = g_plfd_tbl;

        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "connecting to 0x%08X:%d", sock_addr->sin_addr.s_addr, sock_addr->sin_port));
        rc = connect(sc->fd, (SA*)sock_addr, sizeof(*sock_addr)); 
        /* connect should not be called with CHECK_EINTR macro */
        MPIU_ERR_CHKANDJUMP2 (rc < 0 && errno != EINPROGRESS, mpi_errno, MPI_ERR_OTHER,
                              "**sock_connect", "**sock_connect %d %s", errno, strerror (errno));
        
        if (rc == 0) {
            CHANGE_STATE(sc, CONN_STATE_TC_C_CNTD);
        }
        else {
            CHANGE_STATE(sc, CONN_STATE_TC_C_CNTING);
        }
        
/*         sc->handler = sc_state_info[sc->state.cstate].sc_state_handler; */
        ((MPIDI_CH3I_VC *)vc->channel_private)->state = MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTED;
        sc->pg_rank = vc->pg_rank;
        if (IS_SAME_PGID(vc->pg->id, MPIDI_Process.my_pg->id)) {
            sc->is_same_pg = TRUE;
            sc->pg_id = NULL;
        }
        else {
            sc->is_same_pg = FALSE;
            sc->pg_id = vc->pg->id;
        }
        sc->vc = vc;
        VC_FIELD(vc, sc) = sc;
        sc->pending_event = 0; /* clear pending events */
    }
    else if (((MPIDI_CH3I_VC *)vc->channel_private)->state == MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTED) {
        sc = VC_FIELD(vc, sc);
        switch(sc->state.cstate) {
        case CONN_STATE_TS_D_DCNTING:
            CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
            sc->pending_event = 0;
            VC_FIELD(sc->vc, sc) = sc;
            MPID_nem_newtcp_module_conn_est (vc);
            break;
        case CONN_STATE_TS_D_REQRCVD:
            if (IS_WRITEABLE(plfd)) {
                if (send_cmd_pkt(sc->fd, MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_NAK) 
                    == MPI_SUCCESS) {
                    CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
                    sc->pending_event = 0;
                    VC_FIELD(sc->vc, sc) = sc;
                    MPID_nem_newtcp_module_conn_est (vc);
                }
            }
            break;
        case CONN_STATE_TS_D_REQSENT:
        case CONN_STATE_TS_D_QUIESCENT:
            sc->pending_event = EVENT_CONNECT;
            break;
        default:
            break;
        }
    }
    else 
        MPIU_Assert(0);

 fn_exit:
    /* MPID_nem_newtcp_module_connpoll(); FIXME-Imp should be called? */
    return mpi_errno;
 fn_fail:
    if (index != -1) {
        if (sc->fd != CONN_INVALID_FD) {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "MPID_nem_newtcp_module_connect(). closing fd = %d", sc->fd));
            close(sc->fd);
            sc->fd = CONN_INVALID_FD;
            plfd->fd = CONN_INVALID_FD;
        }
        node = MPIU_Malloc(sizeof(freenode_t));      
        MPIU_ERR_CHKANDSTMT(node == NULL, mpi_errno, MPI_ERR_OTHER, goto fn_exit, "**nomem");
        node->index = index;
/*         Note: MPIU_ERR_CHKANDJUMP should not be used here as it will be recursive  */
/*         within fn_fail */ 
        Q_ENQUEUE(&freeq, node);
    }
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/*
  N1: If the tcp connect negotiation is in progress, closing the socket descriptor
  will result in an error. So, it is a good to wait until the socket is connected and hence
  the disconnect event is queued here. This is the only state a disconnect event is 
  queued. This event is handled in the CONN_STATE_TC_C_CNTD state handler function.
*/
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_disconnect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_disconnect (struct MPIDI_VC *const vc)
{
    sockconn_t *sc = NULL;
    int mpi_errno = MPI_SUCCESS;


/*     FIXME check whether a (different/new) error has to be reported stating the VC is  */
/*      already disconnected. */
    if (((MPIDI_CH3I_VC *)vc->channel_private)->state == MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED)
        goto fn_exit;
    else if (((MPIDI_CH3I_VC *)vc->channel_private)->state == MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTED) {
        switch(sc->state.cstate) {
        case CONN_STATE_TC_C_CNTING:
            sc->pending_event = EVENT_DISCONNECT; /*  (N1) */
            break;
        case CONN_STATE_TC_C_CNTD:
        case CONN_STATE_TC_C_RANKSENT:
        case CONN_STATE_TA_C_RANKRCVD:
/*              No need to finish negotiations to move to CONN_STATE_TS_COMMRDY state. */
/*              Just close the connection from the current state ignoring the outstanding */
/*              negotiation messages. */
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "MPID_nem_newtcp_module_disconnect(): changing to "
                  "quiescent from %s", CONN_STATE_STR[sc->state.cstate]));
            CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
            break;
        case CONN_STATE_TS_COMMRDY:
            CHANGE_STATE(sc, CONN_STATE_TS_D_DCNTING);
            break;
        case CONN_STATE_TS_D_DCNTING:
        case CONN_STATE_TS_D_REQSENT:
        case CONN_STATE_TS_D_REQRCVD:
        case CONN_STATE_TS_D_QUIESCENT: /*  already disconnecting. nothing more to do */
        default:
            break;
        }
    }
    else
        /* FIXME: handle error condition */
        MPIU_Assert(0);
 fn_exit:
/*     MPID_nem_newtcp_module_connpoll(); FIXME-Imp should be called? */
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME state_tc_c_cnting_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_tc_c_cnting_handler(pollfd_t *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_NEM_NEWTCP_MODULE_SOCK_STATUS_t stat;
   
    MPIDI_FUNC_ENTER(FUNCNAME);
    stat = MPID_nem_newtcp_module_check_sock_status(plfd);

    if (stat == MPID_NEM_NEWTCP_MODULE_SOCK_CONNECTED) {
        CHANGE_STATE(sc, CONN_STATE_TC_C_CNTD);
    }
    else if (stat == MPID_NEM_NEWTCP_MODULE_SOCK_ERROR_EOF) {
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_tc_c_cnting_handler(): changing to "
              "quiescent"));
        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
        /* FIXME: retry 'n' number of retries before signalling an error to VC layer. */
    }
    else { /* stat == MPID_NEM_NEWTCP_MODULE_SOCK_NOEVENT */
        /*
          Still connecting... let it. While still connecting, even if
          a duplicate connection exists and this connection can be closed, it can get
          tricky. close/shutdown on a unconnected fd will fail anyways. So, let it either
          connect or let the connect itself fail on the fd before making a transition
          from this state. However, we are relying on the time taken by connect to 
          report an error. If we want control over that time, fix the code to poll for
          that amount of time or change the socket option to control the time-out of
          connect.
        */
    }
 fn_exit:
    MPIDI_FUNC_EXIT(FUNCNAME);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_tc_c_cntd_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_tc_c_cntd_handler(pollfd_t *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_FUNC_ENTER(FUNCNAME);
    MPIU_Assert(is_valid_state(sc));

    if (sc->pending_event == EVENT_DISCONNECT || found_better_sc(sc, NULL)) {
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_tc_c_cntd_handler(): changing to "
              "quiescent"));
        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
        goto fn_exit;
    }
    
    if (IS_WRITEABLE(plfd)) {
        MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "inside if (IS_WRITEABLE(plfd))");
        /* plfd->events = POLLIN | POLLOUT; */
        if (send_id_info(sc) == MPI_SUCCESS) {
            CHANGE_STATE(sc, CONN_STATE_TC_C_RANKSENT);
        }
        else {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME,
                                             __LINE__,  MPI_ERR_OTHER, 
                                             "**fail", 0);
            /* FIXME-Danger add error string  actual string : "**cannot send idinfo" */
            goto fn_fail;
        }
    }
    else {
        /* Remain in the same state */
    }
 fn_exit:
    MPIDI_FUNC_EXIT(FUNCNAME);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME state_c_ranksent_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_c_ranksent_handler(pollfd_t *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_nem_newtcp_module_pkt_type_t pkt_type;

    MPIU_Assert(is_valid_state(sc));
    if (IS_READABLE(plfd)) {
        mpi_errno = recv_cmd_pkt(sc->fd, &pkt_type);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_c_ranksent_handler() 1: changing to "
              "quiescent.. "));
            CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
        }
        else {
            MPIU_Assert(pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_ACK ||
                        pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_NAK);

            if (pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_ACK) {
                CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
                VC_FIELD(sc->vc, sc) = sc;
                MPID_nem_newtcp_module_conn_est (sc->vc);
            }
            else { /* pkt_type must be MPIDI_NEM_NEWTCP_MODULE_PKT_ID_NAK */
                MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_c_ranksent_handler() 2: changing to "
                   "quiescent"));
                CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
            }
        }    
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_l_cntd_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_l_cntd_handler(pollfd_t *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_NEM_NEWTCP_MODULE_SOCK_STATUS_t stat;

    MPIDI_FUNC_ENTER(FUNCNAME);
    stat = MPID_nem_newtcp_module_check_sock_status(plfd);
    if (stat == MPID_NEM_NEWTCP_MODULE_SOCK_ERROR_EOF) {
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_l_cntd_handler() 1: changing to "
            "quiescent"));
        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
        goto fn_exit;
    }

    if (IS_READABLE(plfd)) {
        if (recv_id_info(sc) == MPI_SUCCESS) {
            CHANGE_STATE(sc, CONN_STATE_TA_C_RANKRCVD);
        }
        else {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_l_cntd_handler() 2: changing to "
               "quiescent"));
            CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
        }
    }
    else {
        /* remain in same state */
    }

 fn_exit:
    MPIDI_FUNC_EXIT(FUNCNAME);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;

}

/*
  Returns TRUE, if the process(self) wins against the remote process
  FALSE, otherwise
 */
#undef FUNCNAME
#define FUNCNAME do_i_win
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_i_win(sockconn_t *rmt_sc)
{
    int win = FALSE;
    if (rmt_sc->is_same_pg) {
        if (MPIDI_Process.my_pg_rank > rmt_sc->pg_rank)
            win = TRUE;
    }
    else {
        if (strcmp(MPIDI_Process.my_pg->id, rmt_sc->pg_id) > 0)
            win = TRUE;
    }
    return win;
}

#undef FUNCNAME
#define FUNCNAME state_l_rankrcvd_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_l_rankrcvd_handler(pollfd_t *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_NEM_NEWTCP_MODULE_SOCK_STATUS_t stat;
    sockconn_t *fnd_sc;
    int snd_nak = FALSE;

    MPIDI_FUNC_ENTER(FUNCNAME);
    stat = MPID_nem_newtcp_module_check_sock_status(plfd);
    if (stat == MPID_NEM_NEWTCP_MODULE_SOCK_ERROR_EOF) {
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_l_rankrcvd_handler() 1: changing to "
            "quiescent"));
        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
        goto fn_exit;
    }
    if (found_better_sc(sc, &fnd_sc)) {
        if (fnd_sc->state.cstate == CONN_STATE_TS_COMMRDY)
            snd_nak = TRUE;
        else if (fnd_sc->state.cstate == CONN_STATE_TC_C_RANKSENT)
            snd_nak = do_i_win(sc);
    }
    if (IS_WRITEABLE(plfd)) {
        if (snd_nak) {
            if (send_cmd_pkt(sc->fd, MPIDI_NEM_NEWTCP_MODULE_PKT_ID_NAK) == MPI_SUCCESS) {
                MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_l_rankrcvd_handler() 2: changing to "
                  "quiescent"));
                CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
            }
        }
        else {
            if (send_cmd_pkt(sc->fd, MPIDI_NEM_NEWTCP_MODULE_PKT_ID_ACK) == MPI_SUCCESS) {
                CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
                VC_FIELD(sc->vc, sc) = sc;
                MPID_nem_newtcp_module_conn_est (sc->vc);
            }
        }
    }
 fn_exit:
    MPIDI_FUNC_EXIT(FUNCNAME);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME state_commrdy_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_commrdy_handler(pollfd_t *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;

    if (IS_READABLE(plfd))
    {
        mpi_errno = MPID_nem_newtcp_module_recv_handler(plfd, sc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }
    if (IS_WRITEABLE(plfd))
    {
        mpi_errno = send_queued(sc->vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_d_dcnting_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_d_dcnting_handler(pollfd_t *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;

    if (IS_WRITEABLE(plfd)) {
        if (send_cmd_pkt(sc->fd, MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_REQ) == MPI_SUCCESS) {
            CHANGE_STATE(sc, CONN_STATE_TS_D_REQSENT);
        }
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_d_reqsent_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_d_reqsent_handler(pollfd_t *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_nem_newtcp_module_pkt_type_t pkt_type;

    if (IS_READABLE(plfd)) {
        mpi_errno = recv_cmd_pkt(sc->fd, &pkt_type);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_d_reqsent_handler() 1: changing to "
                "quiescent"));
            CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
        }
        else {
            MPIU_Assert(pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_REQ ||
                        pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_ACK);
            if (pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_REQ || 
                pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_ACK) {
                MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_d_reqsent_handler() 2: changing to "
                    "quiescent"));
                CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
            }
        }
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME state_d_reqrcvd_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_d_reqrcvd_handler(pollfd_t *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;

    if (IS_WRITEABLE(plfd)) {
        if (send_cmd_pkt(sc->fd, MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_ACK) == MPI_SUCCESS) {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_d_reqrcvd_handler(): changing to "
                "quiescent"));
            CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
        }
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_d_quiescent_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_d_quiescent_handler(pollfd_t *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS, rc;
    freenode_t *node;

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_d_quiescent_handler(). closing fd = %d", sc->fd));
    CHECK_EINTR(rc, close(sc->fd));
    MPIU_ERR_CHKANDJUMP1 (rc == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, 
                          "**close", "**close %s", strerror (errno));
    sc->fd = plfd->fd = CONN_INVALID_FD;
    if (sc->vc && VC_FIELD(sc->vc, sc) == sc) /* this vc may be connecting/accepting with another sc e.g., this sc lost the tie-breaker */
    {
        ((MPIDI_CH3I_VC *)sc->vc->channel_private)->state = MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED;
        if (sc->pending_event != EVENT_CONNECT)
            VC_FIELD(sc->vc, sc) = NULL;
    }
    if (sc->vc && VC_FIELD(sc->vc, sc) == sc && sc->pending_event == EVENT_CONNECT)
        MPID_nem_newtcp_module_connect(sc->vc);
    else {
        node = MPIU_Malloc(sizeof(freenode_t));      
        MPIU_ERR_CHKANDSTMT(node == NULL, mpi_errno, MPI_ERR_OTHER, goto fn_fail, "**nomem");
        node->index = sc->index;
        Q_ENQUEUE(&freeq, node);
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_init_sm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_init_sm()
{
    /* Set the appropriate handlers */
    sc_state_info[CONN_STATE_TC_C_CNTING].sc_state_handler = state_tc_c_cnting_handler;
    sc_state_info[CONN_STATE_TC_C_CNTD].sc_state_handler = state_tc_c_cntd_handler;
    sc_state_info[CONN_STATE_TC_C_RANKSENT].sc_state_handler = state_c_ranksent_handler;
    sc_state_info[CONN_STATE_TA_C_CNTD].sc_state_handler = state_l_cntd_handler;
    sc_state_info[CONN_STATE_TA_C_RANKRCVD].sc_state_handler = state_l_rankrcvd_handler;
    sc_state_info[CONN_STATE_TS_COMMRDY].sc_state_handler = state_commrdy_handler;

    sc_state_info[CONN_STATE_TS_D_DCNTING].sc_state_handler = state_d_dcnting_handler;

    sc_state_info[CONN_STATE_TS_D_REQSENT].sc_state_handler = state_d_reqsent_handler;
    sc_state_info[CONN_STATE_TS_D_REQRCVD].sc_state_handler = state_d_reqrcvd_handler;
    sc_state_info[CONN_STATE_TS_D_QUIESCENT].sc_state_handler = state_d_quiescent_handler;

    /* Set the appropriate states */
    sc_state_info[CONN_STATE_TC_C_CNTING].sc_state_plfd_events = POLLOUT | POLLIN;
    sc_state_info[CONN_STATE_TC_C_CNTD].sc_state_plfd_events = POLLOUT | POLLIN;
    sc_state_info[CONN_STATE_TC_C_RANKSENT].sc_state_plfd_events = POLLIN;
    sc_state_info[CONN_STATE_TA_C_CNTD].sc_state_plfd_events = POLLOUT | POLLIN;
    sc_state_info[CONN_STATE_TA_C_RANKRCVD].sc_state_plfd_events = POLLOUT | POLLIN;
    sc_state_info[CONN_STATE_TS_COMMRDY].sc_state_plfd_events = POLLIN;

    sc_state_info[CONN_STATE_TS_D_DCNTING].sc_state_plfd_events = POLLOUT | POLLIN;

    sc_state_info[CONN_STATE_TS_D_REQSENT].sc_state_plfd_events = POLLOUT | POLLIN;
    sc_state_info[CONN_STATE_TS_D_REQRCVD].sc_state_plfd_events = POLLOUT | POLLIN;
    sc_state_info[CONN_STATE_TS_D_QUIESCENT].sc_state_plfd_events = POLLOUT | POLLIN;

    /* Allocate the PLFD table */
    alloc_sc_plfd_tbls();
    return 0;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_connection_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_connection_progress (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS, n;
    pollfd_t *plfd;
    sockconn_t *sc;
    
    sc = VC_FIELD(vc, sc);
    if (sc == NULL)
        goto fn_exit;

    plfd = &g_plfd_tbl[sc->index];

    CHECK_EINTR(n, poll(plfd, 1, 0));
    MPIU_ERR_CHKANDJUMP1 (n == -1, mpi_errno, MPI_ERR_OTHER, 
                          "**poll", "**poll %s", strerror (errno));
    if (n == 1)
    {
        mpi_errno = sc->handler(plfd, sc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}


/*
 N1: create a new listener fd?? While doing so, if we bind it to the same port used befor,
then it is ok. Else,the new port number(and thus the business card) has to be communicated 
to the other processes (in same and different pg's), which is not quite simple to do. 
Evaluate the need for it by testing and then do it, if needed.

*/
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_connpoll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_connpoll()
{
    int mpi_errno = MPI_SUCCESS, n, i;

    CHECK_EINTR(n, poll(g_plfd_tbl, g_tbl_size, 0));
    MPIU_ERR_CHKANDJUMP1 (n == -1, mpi_errno, MPI_ERR_OTHER, 
                          "**poll", "**poll %s", strerror (errno));
    /* MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "some sc fd poll event")); */
    for(i = 0; i < g_tbl_size; i++)
    {
        pollfd_t *it_plfd = &g_plfd_tbl[i];
        sockconn_t *it_sc = &g_sc_tbl[i];

        if (it_plfd->fd != CONN_INVALID_FD && it_plfd->revents != 0)
        {
          /* FIXME@san  Uncomment and test */
            MPIU_Assert ((it_plfd->revents & POLLHUP) == 0);
            MPIU_Assert ((it_plfd->revents & POLLERR) == 0);
            MPIU_Assert ((it_plfd->revents & POLLNVAL) == 0);
            

            mpi_errno = it_sc->handler(it_plfd, it_sc);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno); 
            /* @san The above line results in error propagated to above layers that causes
             * MPI process to abort */
        }
    }
    
 fn_exit:
    return mpi_errno;   /* @san Always return success ok?? */
    /* return MPI_SUCCESS; */
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/*
  FIXME
  1.Check for accept error.
  2.If listening socket dies, create a new listening socket so that future connects
  can use that. In that case, how to tell other processes about the new listening port. It's
  probably a design issue at MPI level.

  N1: There might be a timing window where poll on listen_fd was successful and there was
  a connection in the queue. Assume there was only one connection in the listen queue and
  before we called accept, the peer had reset the connection. On receiving a TCP RST, 
  some implementations remove the connection from the listen queue. So, accept on 
  a non-blocking fd would return error with EWOULDBLOCK.

  N2: The peer might have reset or closed the connection. In some implementations,
  even if the connection is reset by the peer, accept is successful. By POSIX standard,
  if the connection is closed by the peer, accept will still be successful. So, soon 
  after accept, check whether the new fd is really connected (i.e neither reset nor
  EOF received from peer).
  Now, it is decided not to check for this condition at this point. After all, in the next
  state, anyhow we can close the socket, if we receive an EOF.

  N3:  find_free_entry is called within the while loop. It may cause the table to expand. So, 
  the arguments passed for this callback function may get invalidated. So, it is imperative
  that we obtain sc pointer and plfd pointer everytime within the while loop.
  Accordingly, the parameters are named unused1 and unused2 for clarity.
*/
#undef FUNCNAME
#define FUNCNAME state_listening_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_state_listening_handler(pollfd_t *const unused_1, sockconn_t *const unused_2)
        /*  listener fd poll struct and sockconn structure */
{
    int mpi_errno = MPI_SUCCESS;
    int connfd;
    socklen_t len;
    SA_IN rmt_addr;
    pollfd_t *l_plfd;
    sockconn_t *l_sc;

    while (1) {
        l_sc = &g_sc_tbl[0];  /* N3 Important */
        l_plfd = &g_plfd_tbl[0];
        len = sizeof(SA_IN);
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "before accept"));
        if ((connfd = accept(l_sc->fd, (SA *) &rmt_addr, &len)) < 0) {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "after accept, l_sc=%08X lstnfd=%d connfd=%d, errno=%d:%s ", l_sc, l_sc->fd, connfd, errno, strerror(errno)));
            if (errno == EINTR) 
                continue;
            else if (errno == EWOULDBLOCK)
                break; /*  no connection in the listen queue. get out of here.(N1) */
            MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER,
                                  "**sock_accept", "**sock_accept %s", strerror(errno));
        }
        else {
            int index;
            pollfd_t *plfd;
            sockconn_t *sc;

            MPID_nem_newtcp_module_set_sockopts(connfd); /* (N2) */
            mpi_errno = find_free_entry(&index);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);        
            sc = &g_sc_tbl[index];
            plfd = &g_plfd_tbl[index];
            
            sc->fd = plfd->fd = connfd;
            sc->pg_rank = CONN_INVALID_RANK;
            CHANGE_STATE(sc, CONN_STATE_TA_C_CNTD);

            sc->g_sc_tbl = g_sc_tbl;
            sc->g_plfd_tbl = g_plfd_tbl;

            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "accept success, added to table, connfd=%d", connfd));        
        }
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* FIXME-Danger Delete later. */
#undef FUNCNAME
#define FUNCNAME f
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int f (void)
{
    int mpi_errno = MPI_SUCCESS;

 fn_exit:
    return mpi_errno;
 fn_fail:    
    goto fn_exit;
}


/*** General FIXME and "ThinkAboutIt" Questions 

1. To check whether a socket connection is connected or not in rank_rcvd_handler
and possibly other handler functions, see whether you can use VC_FIELD(sc->vc, state)
and VC_FIELD(sc->vc, sc). 
Check the set/use of VC_FIELD(sc->vc, sc) in connect side and accept side sequence of
events properly. This may be helpful in optimizing the code.
2. Make sure all system calls are called within CHECK_EINTR macro

***/


/* FIXME Some debug logging code for logging, delete after NEM_SOCK is stable

        
        {                   
            char dbg_ipaddr_str[INET_ADDRSTRLEN], *dbg_p;
            dbg_p = inet_ntop (AF_INET, (struct in_addr *)&sock_addr->sin_addr, dbg_ipaddr_str, 
                               sizeof(dbg_ipaddr_str));
            if (dbg_p) {
                MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "connecting to %s:%d", dbg_ipaddr_str, sock_addr->sin_port));
            }
            else {
                MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "error in inet_ntop"));
            }
        }
*/
