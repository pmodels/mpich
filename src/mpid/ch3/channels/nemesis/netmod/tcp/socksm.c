/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define SOCKSM_H_DEFGLOBALS_

#include "tcp_impl.h"
#include "socksm.h"
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

/* FIXME trace/log all the state transitions */

typedef struct freenode {
    int index;
    struct freenode* next;
} freenode_t;

static struct {
    freenode_t *head, *tail;
} freeq = {NULL, NULL};

static int g_tbl_size = 0;
static int g_tbl_capacity = CONN_PLFD_TBL_INIT_SIZE;

static sockconn_t *g_sc_tbl = NULL;
struct pollfd *MPID_nem_tcp_plfd_tbl = NULL;

sockconn_t MPID_nem_tcp_g_lstn_sc = {0};
struct pollfd MPID_nem_tcp_g_lstn_plfd = {0};

/* We define this in order to trick the compiler into including
   information about the MPID_nem_tcp_vc_area type.  This is
   needed to easily expand the VC_FIELD macro in a debugger.  The
   'unused' attribute keeps the compiler from complaining.  The 'used'
   attribute makes sure the symbol is added in the lib, even if it's
   unused */
static MPID_nem_tcp_vc_area *dummy_vc_area ATTRIBUTE((unused, used)) = NULL;

#define MAX_SKIP_POLLS_INACTIVE 512 /* something bigger */
#define MAX_SKIP_POLLS_ACTIVE   128 /* something smaller */
static int MPID_nem_tcp_skip_polls = MAX_SKIP_POLLS_INACTIVE;

/* Debug function to dump the sockconn table.  This is intended to be
   called from a debugger.  The 'unused' attribute keeps the compiler
   from complaining.  The 'used' attribute makes sure the function is
   added in the lib, despite the fact that it's unused. */
static void dbg_print_sc_tbl(FILE *stream, int print_free_entries) ATTRIBUTE((unused, used));

#define MPID_NEM_TCP_RECV_MAX_PKT_LEN 1024
static char *recv_buf;

static struct {
    handler_func_t sc_state_handler;
    short sc_state_plfd_events;
} sc_state_info[CONN_STATE_SIZE];

#define IS_WRITEABLE(plfd)                      \
    (plfd->revents & POLLOUT) ? 1 : 0

#define IS_READABLE(plfd)                       \
    (plfd->revents & POLLIN) ? 1 : 0

#define IS_READ_WRITEABLE(plfd)                                 \
    (plfd->revents & POLLIN && plfd->revents & POLLOUT) ? 1 : 0

#define IS_SAME_PGID(id1, id2) \
    (strcmp(id1, id2) == 0)


#define INIT_SC_ENTRY(sc, ind)                  \
    do {                                        \
        (sc)->fd = CONN_INVALID_FD;             \
        (sc)->index = ind;                      \
        (sc)->vc = NULL;                        \
        (sc)->pg_is_set = FALSE;                \
        (sc)->is_tmpvc = FALSE;                 \
        (sc)->state.cstate = CONN_STATE_UNDEFINED;      \
        CHANGE_STATE(sc, CONN_STATE_TS_CLOSED); \
    } while (0)

#define INIT_POLLFD_ENTRY(plfd)                               \
    do {                                                      \
        (plfd)->fd = CONN_INVALID_FD;                         \
        (plfd)->events = POLLIN;                              \
    } while (0)


/* --BEGIN ERROR HANDLING-- */
/* This function can be called from a debugger to dump the contents of the
   g_sc_tbl to the given stream.  If print_free_entries is true entries
   0..g_tbl_capacity will be printed.  Otherwise, only 0..g_tbl_size will be
   shown. */
static void dbg_print_sc_tbl(FILE *stream, int print_free_entries)
{
    int i;
    sockconn_t *sc;

    fprintf(stream, "========================================\n");
    for (i = 0; i < (print_free_entries ? g_tbl_capacity : g_tbl_size); ++i) {
        sc = &g_sc_tbl[i];
#define TF(_b) ((_b) ? "TRUE" : "FALSE")
        fprintf(stream, "[%d] ptr=%p idx=%d fd=%d state=%s\n",
                i, sc, sc->index, sc->fd, CONN_STATE_TO_STRING(sc->state.cstate));
        fprintf(stream, "....pg_is_set=%s is_same_pg=%s is_tmpvc=%s pg_rank=%d pg_id=%s\n",
                TF(sc->pg_is_set), TF(sc->is_same_pg), TF(sc->is_tmpvc), sc->pg_rank, sc->pg_id);
#undef TF
    }
    fprintf(stream, "========================================\n");
}
/* --END ERROR HANDLING-- */

static int find_free_entry(int *index);
static int close_cleanup_and_free_sc_plfd(sockconn_t *const sc);
static int cleanup_and_free_sc_plfd(sockconn_t *const sc);
static int error_closed(struct MPIDI_VC *const vc, int req_errno);

#undef FUNCNAME
#define FUNCNAME is_same_connection
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int is_same_connection(sockconn_t *sc1, sockconn_t *sc2)
{

    /* Returns TRUE iff sc1 and sc2 connect the same processes */

    /* if pg_is_set is TRUE, then either it's the same pg, or pg_id is not NULL */
    MPIU_Assert(!sc1->pg_is_set || sc1->is_same_pg || sc1->pg_id != NULL);
    MPIU_Assert(!sc2->pg_is_set || sc2->is_same_pg || sc2->pg_id != NULL);

    /* if it's a tmpvc, the pg should not be set */
    MPIU_Assert(!sc1->is_tmpvc || !sc1->pg_is_set);
    MPIU_Assert(!sc1->is_tmpvc || !sc1->pg_is_set);

    return !sc1->is_tmpvc && !sc2->is_tmpvc &&
        sc1->pg_is_set && sc2->pg_is_set &&
        sc1->pg_rank == sc2->pg_rank &&
        (( sc1->is_same_pg &&  sc2->is_same_pg) ||
         (!sc1->is_same_pg && !sc2->is_same_pg &&
          IS_SAME_PGID(sc1->pg_id, sc2->pg_id)));
}


#undef FUNCNAME
#define FUNCNAME alloc_sc_plfd_tbls
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int alloc_sc_plfd_tbls (void)
{
    int i, mpi_errno = MPI_SUCCESS, idx = -1;
    MPIU_CHKPMEM_DECL (2);

    MPIU_Assert(g_sc_tbl == NULL);
    MPIU_Assert(MPID_nem_tcp_plfd_tbl == NULL);

    MPIU_CHKPMEM_MALLOC (g_sc_tbl, sockconn_t *, g_tbl_capacity * sizeof(sockconn_t), 
                         mpi_errno, "connection table");
    MPIU_CHKPMEM_MALLOC (MPID_nem_tcp_plfd_tbl, struct pollfd *, g_tbl_capacity * sizeof(struct pollfd), 
                         mpi_errno, "pollfd table");
#if defined(MPICH_DEBUG_MEMINIT)
    /* We initialize the arrays in order to eliminate spurious valgrind errors
       that occur when poll(2) returns 0.  See valgrind bugzilla#158425 and
       remove this code if the fix ever gets into a release of valgrind.
       [goodell@ 2007-02-25] */
    memset(MPID_nem_tcp_plfd_tbl, 0, g_tbl_capacity * sizeof(struct pollfd));
#endif

    for (i = 0; i < g_tbl_capacity; i++) {
        INIT_SC_ENTRY(((sockconn_t *)&g_sc_tbl[i]), i);
        INIT_POLLFD_ENTRY(((struct pollfd *)&MPID_nem_tcp_plfd_tbl[i]));
    }
    MPIU_CHKPMEM_COMMIT();

    mpi_errno = find_free_entry(&idx);
    if (mpi_errno != MPI_SUCCESS) MPIR_ERR_POP (mpi_errno);

    MPIU_Assert(0 == idx); /* assumed in other parts of this file */
    MPIU_Memcpy (&g_sc_tbl[idx], &MPID_nem_tcp_g_lstn_sc, sizeof(MPID_nem_tcp_g_lstn_sc));
    MPIU_Memcpy (&MPID_nem_tcp_plfd_tbl[idx], &MPID_nem_tcp_g_lstn_plfd, sizeof(MPID_nem_tcp_g_lstn_plfd));
    MPIU_Assert(MPID_nem_tcp_plfd_tbl[idx].fd == g_sc_tbl[idx].fd);
    MPIU_Assert(MPID_nem_tcp_plfd_tbl[idx].events == POLLIN);

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
#define FCNAME MPL_QUOTE(FUNCNAME)
static int free_sc_plfd_tbls (void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Free(g_sc_tbl);
    MPIU_Free(MPID_nem_tcp_plfd_tbl);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
static int expand_sc_plfd_tbls (void)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *new_sc_tbl = NULL;
    struct pollfd *new_plfd_tbl = NULL;
    int new_capacity = g_tbl_capacity + CONN_PLFD_TBL_GROW_SIZE, i;
    MPIU_CHKPMEM_DECL (2);

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "expand_sc_plfd_tbls Entry"));
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "expand_sc_plfd_tbls b4 g_sc_tbl[0].fd=%d", g_sc_tbl[0].fd));
    MPIU_CHKPMEM_MALLOC (new_sc_tbl, sockconn_t *, new_capacity * sizeof(sockconn_t), 
                         mpi_errno, "expanded connection table");
    MPIU_CHKPMEM_MALLOC (new_plfd_tbl, struct pollfd *, new_capacity * sizeof(struct pollfd), 
                         mpi_errno, "expanded pollfd table");

    MPIU_Memcpy (new_sc_tbl, g_sc_tbl, g_tbl_capacity * sizeof(sockconn_t));
    MPIU_Memcpy (new_plfd_tbl, MPID_nem_tcp_plfd_tbl, g_tbl_capacity * sizeof(struct pollfd));

    /* VCs have pointers to entries in the sc table.  These
       are updated here after the expand. */
    for (i = 1; i < g_tbl_capacity; i++)   /* i=0 = listening socket fd won't have a VC pointer */
    {
        sockconn_t *new_sc = &new_sc_tbl[i];
        sockconn_t *sc = &g_sc_tbl[i];
        MPIDI_VC_t *vc = sc->vc;
        MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);

        /* It's important to only make the assignment if the sc address in the
           vc matches the old sc address, otherwise we can corrupt the vc's
           state in certain head-to-head situations. */
        if (vc && vc_tcp->sc && vc_tcp->sc == sc)
        {
            ASSIGN_SC_TO_VC(vc_tcp, new_sc);
        }
    }

    MPIU_Free(g_sc_tbl);
    MPIU_Free(MPID_nem_tcp_plfd_tbl);
    g_sc_tbl = new_sc_tbl;
    MPID_nem_tcp_plfd_tbl = new_plfd_tbl;
    for (i = g_tbl_capacity; i < new_capacity; i++) {
        INIT_SC_ENTRY(((sockconn_t *)&g_sc_tbl[i]), i);
        INIT_POLLFD_ENTRY(((struct pollfd *)&MPID_nem_tcp_plfd_tbl[i]));
    }
    g_tbl_capacity = new_capacity;

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "expand_sc_plfd_tbls af g_sc_tbl[0].fd=%d", g_sc_tbl[0].fd));
    for (i = 0; i < g_tbl_capacity; ++i)
    {
        sockconn_t *sc = &g_sc_tbl[i];
        MPIDI_VC_t *vc = sc->vc;
        MPIU_AssertDeclValue(MPID_nem_tcp_vc_area *vc_tcp, VC_TCP(vc));
       /*         sockconn_t *dbg_sc = g_sc_tbl[i].vc ? VC_FIELD(g_sc_tbl[i].vc, sc) : (sockconn_t*)(-1); */

        /* The state is only valid if the FD is valid.  The VC field is only
           valid if the state is valid and COMMRDY. */
        MPIU_Assert(MPID_nem_tcp_plfd_tbl[i].fd == CONN_INVALID_FD ||
                    sc->state.cstate != CONN_STATE_TS_COMMRDY ||
                    vc_tcp->sc == sc);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
static int find_free_entry(int *idx)
{
    int mpi_errno = MPI_SUCCESS;
    freenode_t *node;

    if (!Q_EMPTY(freeq)) {
        Q_DEQUEUE(&freeq, ((freenode_t **)&node)); 
        *idx = node->index;
        MPIU_Free(node);
        goto fn_exit;
    }

    if (g_tbl_size == g_tbl_capacity) {
        mpi_errno = expand_sc_plfd_tbls();
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
    }

    MPIU_Assert(g_tbl_capacity > g_tbl_size);
    *idx = g_tbl_size;
    ++g_tbl_size;

 fn_exit:
    /* This function is the closest thing we have to a constructor, so we throw
       in a couple of initializers here in case the caller is sloppy about his
       assumptions. */
    INIT_SC_ENTRY(&g_sc_tbl[*idx], *idx);
    INIT_POLLFD_ENTRY(&MPID_nem_tcp_plfd_tbl[*idx]);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* Note:
   fnd_sc is returned only for certain states. If it is not returned for a state,
   the handler function can simply pass NULL as the second argument.
 */
#undef FUNCNAME
#define FUNCNAME found_better_sc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int found_better_sc(sockconn_t *sc, sockconn_t **fnd_sc)
{
    int i, found = FALSE;
    MPIDI_STATE_DECL(MPID_STATE_FOUND_BETTER_SC);

    MPIDI_FUNC_ENTER(MPID_STATE_FOUND_BETTER_SC);

    /* tmpvc's can never match a better sc */
    if (sc->is_tmpvc) {
        found = FALSE;
        goto fn_exit;
    }

    /* if we don't know our own pg info, how can we look for a better SC? */
    MPIU_Assert(sc->pg_is_set);

    for(i = 0; i < g_tbl_size && !found; i++)
    {
        sockconn_t *iter_sc = &g_sc_tbl[i];
        MPID_nem_tcp_Conn_State_t istate = iter_sc->state.cstate;

        if (iter_sc != sc && iter_sc->fd != CONN_INVALID_FD 
            && is_same_connection(iter_sc, sc))
        {
            switch (sc->state.cstate)
            {
            case CONN_STATE_TC_C_CNTD:
                MPIU_Assert(fnd_sc == NULL);
                if (istate == CONN_STATE_TS_COMMRDY ||
                    istate == CONN_STATE_TA_C_RANKRCVD ||
                    istate == CONN_STATE_TC_C_TMPVCSENT)
                    found = TRUE;
                break;
            case CONN_STATE_TA_C_RANKRCVD:
                MPIU_Assert(fnd_sc != NULL);
                if (istate == CONN_STATE_TS_COMMRDY || istate == CONN_STATE_TC_C_RANKSENT) {
                    found = TRUE;
                    *fnd_sc = iter_sc;
                }
                break;                
            case CONN_STATE_TA_C_TMPVCRCVD:
                MPIU_Assert(fnd_sc != NULL);
                if (istate == CONN_STATE_TS_COMMRDY || istate == CONN_STATE_TC_C_TMPVCSENT) {
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

fn_exit:
    if (found) {
        if (fnd_sc) {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE,
                             (MPIU_DBG_FDEST, "found_better_sc(sc=%p (%s), *fnd_sc=%p (%s)) found=TRUE",
                              sc, CONN_STATE_STR[sc->state.cstate],
                              *fnd_sc, (*fnd_sc ? CONN_STATE_STR[(*fnd_sc)->state.cstate] : "N/A")));
        }
        else {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE,
                             (MPIU_DBG_FDEST, "found_better_sc(sc=%p (%s), fnd_sc=(nil)) found=TRUE",
                              sc, CONN_STATE_STR[sc->state.cstate]));
        }
    }
    else {
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE,
                         (MPIU_DBG_FDEST, "found_better_sc(sc=%p (%s), *fnd_sc=N/A) found=FALSE",
                          sc, CONN_STATE_STR[sc->state.cstate]));
    }
    MPIDI_FUNC_EXIT(MPID_STATE_FOUND_BETTER_SC);
    return found;
}


#undef FUNCNAME
#define FUNCNAME vc_is_in_shutdown
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int vc_is_in_shutdown(MPIDI_VC_t *vc)
{
    int retval = FALSE;
    MPIU_Assert(vc != NULL);
    if (vc->state == MPIDI_VC_STATE_REMOTE_CLOSE ||
        vc->state == MPIDI_VC_STATE_CLOSE_ACKED ||
        vc->state == MPIDI_VC_STATE_CLOSED ||
        vc->state == MPIDI_VC_STATE_LOCAL_CLOSE ||
        vc->state == MPIDI_VC_STATE_INACTIVE ||
        vc->state == MPIDI_VC_STATE_INACTIVE_CLOSED ||
        vc->state == MPIDI_VC_STATE_MORIBUND)
    {
        retval = TRUE;
    }

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "vc_is_in_shutdown(%p)=%s", vc, (retval ? "TRUE" : "FALSE")));
    return retval;
}

#undef FUNCNAME
#define FUNCNAME send_id_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int send_id_info(const sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_nem_tcp_idinfo_t id_info;
    MPIDI_nem_tcp_header_t hdr;
    struct iovec iov[3];
    int buf_size, iov_cnt = 2;
    ssize_t offset;
    size_t pg_id_len = 0;
    MPIDI_STATE_DECL(MPID_STATE_SEND_ID_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_ID_INFO);

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "my_pg->id=%s my_pg->rank=%d, sc->pg_rank=%d sc->is_same_pg=%s",
                                             (char *)MPIDI_Process.my_pg->id, MPIDI_Process.my_pg_rank, sc->pg_rank,
                                             (sc->is_same_pg ? "TRUE" : "FALSE")));
    if (!sc->is_same_pg)
        pg_id_len = strlen(MPIDI_Process.my_pg->id) + 1; 

/*     store ending NULL also */
/*     FIXME better keep pg_id_len itself as part of MPIDI_Process.my_pg structure to */
/*     avoid computing the length of string everytime this function is called. */
    
    hdr.pkt_type = MPIDI_NEM_TCP_SOCKSM_PKT_ID_INFO;
    hdr.datalen = sizeof(MPIDI_nem_tcp_idinfo_t) + pg_id_len;    
    id_info.pg_rank = MPIDI_Process.my_pg_rank;

    iov[0].iov_base = (MPL_IOV_BUF_CAST)&hdr;
    iov[0].iov_len = sizeof(hdr);
    iov[1].iov_base = (MPL_IOV_BUF_CAST)&id_info;
    iov[1].iov_len = sizeof(id_info);
    buf_size = sizeof(hdr) + sizeof(id_info);

    if (!sc->is_same_pg) {
        iov[2].iov_base = MPIDI_Process.my_pg->id;
        iov[2].iov_len = pg_id_len;
        buf_size += pg_id_len;
        ++iov_cnt;
    }
    
    offset = MPL_large_writev(sc->fd, iov, iov_cnt);
    MPIR_ERR_CHKANDJUMP1(offset == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", MPIU_Strerror(errno));
    MPIR_ERR_CHKANDJUMP1(offset != buf_size, mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", MPIU_Strerror(errno));
/*     FIXME log appropriate error */
/*     FIXME-Z1  socket is just connected and we are sending a few bytes. So, there should not */
/*     be a problem of partial data only being written to. If partial data written, */
/*     handle this. */

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_ID_INFO);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d, offset=%lld, errno=%d %s", mpi_errno, (long long)offset, errno, MPIU_Strerror(errno)));
    goto fn_exit;    
}


#undef FUNCNAME
#define FUNCNAME send_tmpvc_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int send_tmpvc_info(const sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_nem_tcp_portinfo_t port_info;
    MPIDI_nem_tcp_header_t hdr;
    struct iovec iov[3];
    int buf_size, iov_cnt = 2;
    ssize_t offset;
    MPIDI_STATE_DECL(MPID_STATE_SEND_TMPVC_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_TMPVC_INFO);

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "my->pg_rank=%d, sc->pg_rank=%d"
                                             , MPIDI_Process.my_pg_rank, sc->pg_rank));

/*     store ending NULL also */
/*     FIXME better keep pg_id_len itself as part of MPIDI_Process.my_pg structure to */
/*     avoid computing the length of string everytime this function is called. */
    
    hdr.pkt_type = MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_INFO;
    hdr.datalen = sizeof(MPIDI_nem_tcp_portinfo_t);
    port_info.port_name_tag = sc->vc->port_name_tag;

    iov[0].iov_base = (MPL_IOV_BUF_CAST)&hdr;
    iov[0].iov_len = sizeof(hdr);
    iov[1].iov_base = (MPL_IOV_BUF_CAST)&port_info;
    iov[1].iov_len = sizeof(port_info);
    buf_size = sizeof(hdr) + sizeof(port_info);
    
    offset = MPL_large_writev(sc->fd, iov, iov_cnt);
    MPIR_ERR_CHKANDJUMP1(offset == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", MPIU_Strerror(errno));
    MPIR_ERR_CHKANDJUMP1(offset != buf_size, mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", MPIU_Strerror(errno));
/*     FIXME log appropriate error */
/*     FIXME-Z1  socket is just connected and we are sending a few bytes. So, there should not */
/*     be a problem of partial data only being written to. If partial data written, */
/*     handle this. */

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_TMPVC_INFO);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d, offset=%lld, errno=%d %s", mpi_errno, (long long)offset, errno, MPIU_Strerror(errno)));
    goto fn_exit;    
}

#undef FUNCNAME
#define FUNCNAME recv_id_or_tmpvc_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int recv_id_or_tmpvc_info(sockconn_t *const sc, int *got_sc_eof)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_nem_tcp_header_t hdr;
    int iov_cnt = 1;
    size_t pg_id_len = 0;
    ssize_t nread;
    int hdr_len = sizeof(MPIDI_nem_tcp_header_t);
    struct iovec iov[2];
    char *pg_id = NULL;

    MPIU_CHKPMEM_DECL (1);
    MPIU_CHKLMEM_DECL (1);
    MPIDI_STATE_DECL(MPID_STATE_RECV_ID_OR_TMPVC_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_RECV_ID_OR_TMPVC_INFO);

    *got_sc_eof = 0;

    CHECK_EINTR (nread, read(sc->fd, &hdr, hdr_len));

    /* The other side closed this connection (hopefully as part of a
       head-to-head resolution. */
    if (0 == nread) {
        *got_sc_eof = 1;
        goto fn_exit;
    }
    MPIR_ERR_CHKANDJUMP1(nread == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", MPIU_Strerror(errno));
    MPIR_ERR_CHKANDJUMP(nread != hdr_len, mpi_errno, MPI_ERR_OTHER, "**read");  /* FIXME-Z1 */
    MPIU_Assert(hdr.pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_ID_INFO ||
		hdr.pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_INFO);
    MPIU_Assert(hdr.datalen != 0);
    
    if (hdr.pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_ID_INFO) {
	iov[0].iov_base = (void *) &(sc->pg_rank);
	iov[0].iov_len = sizeof(sc->pg_rank);
	pg_id_len = hdr.datalen - sizeof(MPIDI_nem_tcp_idinfo_t);
	if (pg_id_len != 0) {
	    MPIU_CHKLMEM_MALLOC (pg_id, char *, pg_id_len, mpi_errno, "sockconn pg_id");
	    iov[1].iov_base = (void *)pg_id;
	    iov[1].iov_len = pg_id_len;
	    ++iov_cnt;
	} 
	nread = MPL_large_readv(sc->fd, iov, iov_cnt);
        MPIR_ERR_CHKANDJUMP1(nread == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", MPIU_Strerror(errno));
	MPIR_ERR_CHKANDJUMP(nread != hdr.datalen, mpi_errno, MPI_ERR_OTHER, "**read"); /* FIXME-Z1 */
	if (pg_id_len == 0) {
	    sc->is_same_pg = TRUE;
            mpi_errno = MPID_nem_tcp_get_vc_from_conninfo (MPIDI_Process.my_pg->id,
                                                                     sc->pg_rank, &sc->vc);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	    sc->pg_id = NULL;
	}
	else {
	    sc->is_same_pg = FALSE;
            mpi_errno = MPID_nem_tcp_get_vc_from_conninfo (pg_id, sc->pg_rank, &sc->vc);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	    sc->pg_id = sc->vc->pg->id;
	}

        {   /* Added this block sp we can declare pointers to tcp private area of vc */
            MPIDI_VC_t *sc_vc = sc->vc;
            MPID_nem_tcp_vc_area *sc_vc_tcp = VC_TCP(sc_vc);

            MPIU_Assert(sc_vc != NULL);
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "about to incr sc_ref_count sc=%p sc->vc=%p sc_ref_count=%d", sc, sc_vc, sc_vc_tcp->sc_ref_count));
            ++sc_vc_tcp->sc_ref_count;
        }
        
        /* very important, without this is_same_connection() will always fail */
        sc->pg_is_set = TRUE;
        MPIU_Assert(!sc->is_tmpvc);
        
	MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PKT_ID_INFO: sc->pg_rank=%d sc->is_same_pg=%d sc->fd=%d, sc->vc=%p, sc=%p", sc->pg_rank, sc->is_same_pg, sc->fd, sc->vc, sc));
    }
    else if (hdr.pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_INFO) {
        MPIDI_VC_t *vc;
        MPID_nem_tcp_vc_area *vc_tcp;

        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "PKT_TMPVC_INFO: sc->fd=%d", sc->fd));
        /* create a new VC */
        MPIU_CHKPMEM_MALLOC (vc, MPIDI_VC_t *, sizeof(MPIDI_VC_t), mpi_errno, "real vc from tmp vc");
        /* --BEGIN ERROR HANDLING-- */
        if (vc == NULL) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
            goto fn_fail;
        }
        /* --END ERROR HANDLING-- */

        vc_tcp = VC_TCP(vc);
        
        MPIDI_VC_Init(vc, NULL, 0);
        vc_tcp->state = MPID_NEM_TCP_VC_STATE_CONNECTED; /* FIXME: is it needed ? */
        sc->vc = vc;
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "about to incr sc_ref_count sc=%p sc->vc=%p sc_ref_count=%d", sc, sc->vc, vc_tcp->sc_ref_count));
        ++vc_tcp->sc_ref_count;

        ASSIGN_SC_TO_VC(vc_tcp, sc);

        /* get the port's tag from the packet and stash it in the VC */
        iov[0].iov_base = (void *) &(sc->vc->port_name_tag);
        iov[0].iov_len = sizeof(sc->vc->port_name_tag);

        nread = MPL_large_readv(sc->fd, iov, iov_cnt);
        MPIR_ERR_CHKANDJUMP1(nread == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", MPIU_Strerror(errno));
        MPIR_ERR_CHKANDJUMP(nread != hdr.datalen, mpi_errno, MPI_ERR_OTHER, "**read"); /* FIXME-Z1 */
        sc->is_same_pg = FALSE;
        sc->pg_id = NULL;
        sc->is_tmpvc = TRUE;

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "enqueuing on acceptq vc=%p, sc->fd=%d, tag=%d", vc, sc->fd, sc->vc->port_name_tag));
        MPIDI_CH3I_Acceptq_enqueue(vc, sc->vc->port_name_tag);

        MPIU_Assert(!sc->pg_is_set);
    }

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_RECV_ID_OR_TMPVC_INFO);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
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
#define FCNAME MPL_QUOTE(FUNCNAME)
static int send_cmd_pkt(int fd, MPIDI_nem_tcp_socksm_pkt_type_t pkt_type)
{
    int mpi_errno = MPI_SUCCESS;
    ssize_t offset;
    MPIDI_nem_tcp_header_t pkt;
    int pkt_len = sizeof(MPIDI_nem_tcp_header_t);

    MPIU_Assert(pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_ID_ACK ||
                pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_ID_NAK ||
		pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_ACK ||
		pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_NAK ||
                pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_CLOSED);

    pkt.pkt_type = pkt_type;
    pkt.datalen = 0;

    CHECK_EINTR (offset, write(fd, &pkt, pkt_len));
    MPIR_ERR_CHKANDJUMP1(offset == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", MPIU_Strerror(errno));
    MPIR_ERR_CHKANDJUMP1(offset != pkt_len, mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", MPIU_Strerror(errno)); /* FIXME-Z1 */
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
#define FCNAME MPL_QUOTE(FUNCNAME)
static int recv_cmd_pkt(int fd, MPIDI_nem_tcp_socksm_pkt_type_t *pkt_type)
{
    int mpi_errno = MPI_SUCCESS;
    ssize_t nread;
    MPIDI_nem_tcp_header_t pkt;
    int pkt_len = sizeof(MPIDI_nem_tcp_header_t);
    MPIDI_STATE_DECL(MPID_STATE_RECV_CMD_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_RECV_CMD_PKT);

    CHECK_EINTR (nread, read(fd, &pkt, pkt_len));
    MPIR_ERR_CHKANDJUMP1(nread == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", MPIU_Strerror(errno));
    MPIR_ERR_CHKANDJUMP2(nread != pkt_len, mpi_errno, MPI_ERR_OTHER, "**read", "**read %d %s", nread, MPIU_Strerror(errno)); /* FIXME-Z1 */
    MPIU_Assert(pkt.datalen == 0);
    MPIU_Assert(pkt.pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_ID_ACK ||
                pkt.pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_ID_NAK ||
                pkt.pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_ACK ||
                pkt.pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_NAK ||
                pkt.pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_CLOSED);
    
    *pkt_type = pkt.pkt_type;
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_RECV_CMD_PKT);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}


/* Initiates connection protocol to remote process.  It's OK to call
   this function with the VC in an error state; an appropriate error
   code will be returned. */
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_connect
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_connect(struct MPIDI_VC *const vc)
{
    MPID_nem_tcp_vc_area *const vc_tcp = VC_TCP(vc);
    sockconn_t *sc = NULL;
    struct pollfd *plfd = NULL;
    int idx = -1;
    int mpi_errno = MPI_SUCCESS;

    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_CONNECT);

    MPIU_Assert(vc != NULL);

    /* Handle error case */
    if (vc_tcp->state == MPID_NEM_TCP_VC_STATE_ERROR ||
        vc->state == MPIDI_VC_STATE_MORIBUND) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**vc_in_error_state");
    }

    /* We have an active connection, start polling more often */
    MPID_nem_tcp_skip_polls = MAX_SKIP_POLLS_ACTIVE;

    MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

    if (vc_tcp->state == MPID_NEM_TCP_VC_STATE_DISCONNECTED) {
        struct sockaddr_in *sock_addr;
	struct in_addr addr;
        int rc = 0;

        if (vc_tcp->connect_retry_count > MPIDI_NEM_TCP_MAX_CONNECT_RETRIES) {
            MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "exceeded retries, closing sc");
            MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**exceeded_connect_tries", "**exceeded_connect_tries %d", vc->pg_rank);
            mpi_errno = error_closed(vc, mpi_errno);
            MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**tcp_cleanup_fail");
            goto fn_fail;
        }
        
        ++vc_tcp->connect_retry_count;

        MPIU_Assert(vc_tcp->sc == NULL);
        mpi_errno = find_free_entry(&idx);
        if (mpi_errno != MPI_SUCCESS) MPIR_ERR_POP (mpi_errno);

        sc = &g_sc_tbl[idx];
        plfd = &MPID_nem_tcp_plfd_tbl[idx];

        /* FIXME:
           We need to set addr and port using bc.
           If a process is dynamically spawned, vc->pg is NULL.
           In that case, same procedure is done
           in MPID_nem_tcp_connect_to_root()
        */
        if (vc->pg != NULL) { /* VC is not a temporary one */
            char *bc;
            int pmi_errno;
            int val_max_sz;

#ifdef USE_PMI2_API
            val_max_sz = PMI2_MAX_VALLEN;
#else
            pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
            MPIR_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
#endif
            MPIU_CHKLMEM_MALLOC(bc, char *, val_max_sz, mpi_errno, "bc");
            
            sc->is_tmpvc = FALSE;
            
            mpi_errno = vc->pg->getConnInfo(vc->pg_rank, bc, val_max_sz, vc->pg);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            mpi_errno = MPID_nem_tcp_get_addr_port_from_bc(bc, &addr, &(vc_tcp->sock_id.sin_port));
            vc_tcp->sock_id.sin_addr.s_addr = addr.s_addr;
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else {
            sc->is_tmpvc = TRUE;
            MPIU_Assert(!sc->pg_is_set);
        }

        sock_addr = &(vc_tcp->sock_id);

        CHECK_EINTR(sc->fd, socket(AF_INET, SOCK_STREAM, 0));
        MPIR_ERR_CHKANDJUMP2(sc->fd == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create", "**sock_create %s %d", MPIU_Strerror(errno), errno);

        plfd->fd = sc->fd;
	MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "sc->fd=%d, plfd->events=%d, plfd->revents=%d, vc=%p, sc=%p", sc->fd, plfd->events, plfd->revents, vc, sc));
        mpi_errno = MPID_nem_tcp_set_sockopts(sc->fd);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);

        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "connecting to 0x%08X:%d", sock_addr->sin_addr.s_addr, sock_addr->sin_port));
        rc = connect(sc->fd, (SA*)sock_addr, sizeof(*sock_addr));
        /* connect should not be called with CHECK_EINTR macro */
        MPIR_ERR_CHKANDJUMP2(rc < 0 && errno != EINPROGRESS, mpi_errno, MPI_ERR_OTHER, "**sock_connect", "**sock_connect %d %s", errno, MPIU_Strerror(errno));
        
        if (rc == 0) {
            CHANGE_STATE(sc, CONN_STATE_TC_C_CNTD);
        }
        else {
            CHANGE_STATE(sc, CONN_STATE_TC_C_CNTING);
        }
        
        vc_tcp->state = MPID_NEM_TCP_VC_STATE_CONNECTED;
        sc->pg_rank = vc->pg_rank;

        if (vc->pg != NULL) { /* normal (non-dynamic) connection */
            if (IS_SAME_PGID(vc->pg->id, MPIDI_Process.my_pg->id)) {
                sc->is_same_pg = TRUE;
                sc->pg_id = NULL;
            }
            else {
                sc->is_same_pg = FALSE;
                sc->pg_id = vc->pg->id;
            }
            /* very important, without this is_same_connection() will always fail */
            sc->pg_is_set = TRUE;
            MPIU_Assert(!sc->is_tmpvc);
        }
        else { /* (vc->pg == NULL), dynamic proc connection - temp vc */
            MPIU_Assert(sc->is_tmpvc);
        }

        ASSIGN_SC_TO_VC(vc_tcp, sc);
        sc->vc = vc;
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "about to incr sc_ref_count sc=%p sc->vc=%p sc_ref_count=%d", sc, sc->vc, vc_tcp->sc_ref_count));
        ++vc_tcp->sc_ref_count;
    }
    else if (vc_tcp->state == MPID_NEM_TCP_VC_STATE_CONNECTED) {
        sc = vc_tcp->sc;
        MPIU_Assert(sc != NULL);
        /* Do nothing here, the caller just needs to wait for the connection
           state machine to work its way through the states.  Doing something at
           this point will almost always just mess up any head-to-head
           resolution. */
    }
    else {
        /* We already handled the error case at the top of the routine. */
        MPIU_Assertp(0);
    }

 fn_exit:
    /* MPID_nem_tcp_connpoll(); FIXME-Imp should be called? */
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_CONNECT);
    return mpi_errno;
 fn_fail:
    if (idx != -1)
        close_cleanup_and_free_sc_plfd(&g_sc_tbl[idx]); /* ignore error return */
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cleanup_and_free_sc_plfd
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int cleanup_and_free_sc_plfd(sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *const sc_vc = sc->vc;
    MPID_nem_tcp_vc_area *const sc_vc_tcp = VC_TCP(sc_vc);
    const int idx = sc->index;
    struct pollfd *const plfd = &MPID_nem_tcp_plfd_tbl[sc->index];
    freenode_t *node;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_CLEANUP_AND_FREE_SC_PLFD);

    MPIDI_FUNC_ENTER(MPID_STATE_CLEANUP_AND_FREE_SC_PLFD);

    if (sc == NULL)
        goto fn_exit;

    if (sc_vc) {
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "about to decr sc_ref_count sc=%p sc->vc=%p sc_ref_count=%d", sc, sc_vc, sc_vc_tcp->sc_ref_count));
        MPIU_Assert(sc_vc_tcp->sc_ref_count > 0);
        --sc_vc_tcp->sc_ref_count;
    }
    
    if (sc_vc && sc_vc_tcp->sc == sc) /* this vc may be connecting/accepting with another sc e.g., this sc lost the tie-breaker */
    {
        sc_vc_tcp->state = MPID_NEM_TCP_VC_STATE_DISCONNECTED;
        ASSIGN_SC_TO_VC(sc_vc_tcp, NULL);
    }

    CHANGE_STATE(sc, CONN_STATE_TS_CLOSED);

    INIT_SC_ENTRY(sc, idx);
    INIT_POLLFD_ENTRY(plfd);

    MPIU_CHKPMEM_MALLOC(node, freenode_t *, sizeof(freenode_t), mpi_errno, "free node");
    node->index = idx;
    Q_ENQUEUE(&freeq, node);

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CLEANUP_AND_FREE_SC_PLFD);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* Called to transition an sc to CLOSED, and free associated
   resources.  This might be done as part of a ch3 close protocol,
   because the sc lost in a head-to-head connection, or because there
   was an error associated with the connection. */
#undef FUNCNAME
#define FUNCNAME close_cleanup_and_free_sc_plfd
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int close_cleanup_and_free_sc_plfd(sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno2 = MPI_SUCCESS;
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_CLOSE_CLEANUP_AND_FREE_SC_PLFD);

    MPIDI_FUNC_ENTER(MPID_STATE_CLOSE_CLEANUP_AND_FREE_SC_PLFD);

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "vc=%p, sc=%p, closing fd=%d", sc->vc, sc, sc->fd));

    if (sc == NULL)
        goto fn_exit;

    CHECK_EINTR(rc, close(sc->fd));
    if (rc == -1 && errno != EAGAIN && errno != EBADF)
        MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**close", "**close %s", MPIU_Strerror(errno));

    mpi_errno2 = cleanup_and_free_sc_plfd(sc);
    if (mpi_errno2) MPIR_ERR_ADD(mpi_errno, mpi_errno2);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CLOSE_CLEANUP_AND_FREE_SC_PLFD);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* this function is called when vc->state becomes CLOSED or when we need
   to clean up after we restart from a checkpoint */
/* FIXME XXX DJG do we need to do anything here to ensure that the final
   close(TRUE) packet has made it into a writev call?  The code might have a
   race for queued messages. */
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_cleanup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_cleanup (struct MPIDI_VC *const vc)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPID_nem_tcp_vc_area *const vc_tcp = VC_TCP(vc);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CLEANUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_CLEANUP);

    if (vc_tcp->sc != NULL) {
        mpi_errno = close_cleanup_and_free_sc_plfd(vc_tcp->sc);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    i = 0;
    while (vc_tcp->sc_ref_count > 0 && i < g_tbl_size) {
        if (g_sc_tbl[i].vc == vc) {
            /* We've found a proto-connection that doesn't yet have enough
               information to resolve the head-to-head situation.  If we don't
               clean him up he'll end up accessing the about-to-be-freed vc. */
            mpi_errno = close_cleanup_and_free_sc_plfd(&g_sc_tbl[i]);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIU_Assert(g_sc_tbl[i].vc == NULL);
        }
        ++i;
    }

    /* close_cleanup_and_free_sc_plfd can technically cause a reconnect on a per-sc basis, but I
       don't think that it can happen when cleanup is called.  Let's
       assert this for now and remove it if we prove that it can happen. */
    MPIU_Assert(vc_tcp->sc_ref_count == 0);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_CLEANUP);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* This cleans up and frees connection resources for all VCs.  Note
   that we didn't free or close any connections before taking the
   checkpoint, so we need to do that on restart (except we don't close
   the sockets because they should already be closed). */
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_ckpt_cleanup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_ckpt_cleanup(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CKPT_CLEANUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_CKPT_CLEANUP);

    for (i = 0; i < MPIDI_Process.my_pg->size; ++i) {
        MPIDI_VC_t *vc;
        if (i == MPIDI_Process.my_pg_rank)
            continue;
        MPIDI_PG_Get_vc(MPIDI_Process.my_pg, i, &vc);
        {
            MPIDI_CH3I_VC *vc_ch = &vc->ch;
            MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
            if (vc_ch->is_local || vc_tcp->sc == NULL)
                continue;
            
            /* cleanup vc */
            mpi_errno = cleanup_and_free_sc_plfd(vc_tcp->sc);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_CKPT_CLEANUP);
    return mpi_errno;
 fn_fail:

    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME state_tc_c_cnting_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int state_tc_c_cnting_handler(struct pollfd *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_NEM_TCP_SOCK_STATUS_t status;
    MPIDI_STATE_DECL(MPID_STATE_STATE_TC_C_CNTING_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_TC_C_CNTING_HANDLER);
   
    status = MPID_nem_tcp_check_sock_status(plfd);

    if (status == MPID_NEM_TCP_SOCK_CONNECTED) {
        CHANGE_STATE(sc, CONN_STATE_TC_C_CNTD);
    }
    else if (status == MPID_NEM_TCP_SOCK_ERROR_EOF) {
        MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "state_tc_c_cnting_handler(): closing sc");
        mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
    }
    else { /* status == MPID_NEM_TCP_SOCK_NOEVENT */
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

    MPIDI_FUNC_EXIT(MPID_STATE_STATE_TC_C_CNTING_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME state_tc_c_cntd_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int state_tc_c_cntd_handler(struct pollfd *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_STATE_TC_C_CNTD_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_TC_C_CNTD_HANDLER);

    if (found_better_sc(sc, NULL)) {
        MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "state_tc_c_cntd_handler(): closing sc");
        mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
        goto fn_exit;
    }
    
    if (IS_WRITEABLE(plfd)) {
        MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "inside if (IS_WRITEABLE(plfd))");
        if (!sc->is_tmpvc) { /* normal connection */
            mpi_errno = send_id_info(sc);
            if (mpi_errno) {
                MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "state_tc_c_cntd_handler(): closing sc");
                mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
                mpi_errno = MPI_SUCCESS; /* don't return an error if we fail in the connect protocol, just retry. */
                goto fn_exit;
            }

            CHANGE_STATE(sc, CONN_STATE_TC_C_RANKSENT);
        }
        else { /* temp VC */
            mpi_errno = send_tmpvc_info(sc);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            
            CHANGE_STATE(sc, CONN_STATE_TC_C_TMPVCSENT);
        }
    }
    else {
        /* Remain in the same state */
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_TC_C_CNTD_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME state_c_ranksent_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int state_c_ranksent_handler(struct pollfd *const plfd, sockconn_t *const sc)
{
    MPIDI_VC_t *const sc_vc = sc->vc;
    MPID_nem_tcp_vc_area *const sc_vc_tcp = VC_TCP(sc_vc);
    int mpi_errno = MPI_SUCCESS;
    MPIDI_nem_tcp_socksm_pkt_type_t pkt_type;
    MPIDI_STATE_DECL(MPID_STATE_STATE_C_RANKSENT_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_C_RANKSENT_HANDLER);

    if (IS_READABLE(plfd)) {
        mpi_errno = recv_cmd_pkt(sc->fd, &pkt_type);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        
        MPIU_Assert(pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_ID_ACK ||
                    pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_ID_NAK ||
                    pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_CLOSED);

        switch (pkt_type) {
        case MPIDI_NEM_TCP_SOCKSM_PKT_ID_ACK:
            CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
            ASSIGN_SC_TO_VC(sc_vc_tcp, sc);

            MPID_nem_tcp_conn_est (sc_vc);
            sc_vc_tcp->connect_retry_count = 0; /* successfully connected, reset connection retry count */
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "c_ranksent_handler(): connection established (sc=%p, sc->vc=%p, fd=%d)", sc, sc->vc, sc->fd));
            break;
        case MPIDI_NEM_TCP_SOCKSM_PKT_ID_NAK:
            MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "received NAK, closing sc");
            mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
            break;
        case MPIDI_NEM_TCP_SOCKSM_PKT_CLOSED:
            MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "received CLOSED, closing sc");
            mpi_errno = MPIDI_CH3U_Handle_connection(sc_vc, MPIDI_VC_EVENT_TERMINATED);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
            break;
        default:
            MPIU_Assert(0);
        }
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_C_RANKSENT_HANDLER);
    return mpi_errno;
fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
    mpi_errno = MPI_SUCCESS; /* don't return an error if we fail in the connect protocol, just retry. */
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME state_c_tmpvcsent_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int state_c_tmpvcsent_handler(struct pollfd *const plfd, sockconn_t *const sc)
{
    MPIDI_VC_t *const sc_vc = sc->vc;
    MPID_nem_tcp_vc_area *const sc_vc_tcp = VC_TCP(sc_vc);
    int mpi_errno = MPI_SUCCESS;
    MPIDI_nem_tcp_socksm_pkt_type_t pkt_type;
    MPIDI_STATE_DECL(MPID_STATE_STATE_C_TMPVCSENT_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_C_TMPVCSENT_HANDLER);

    if (IS_READABLE(plfd)) {
        mpi_errno = recv_cmd_pkt(sc->fd, &pkt_type);
        if (mpi_errno) {
            int mpi_errno2 = MPI_SUCCESS;
            MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "error sending cmd pkt, closing sc");
            mpi_errno2 = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
            if (mpi_errno2) {
                MPIR_ERR_SET(mpi_errno2, MPI_ERR_OTHER, "**tcp_cleanup_fail");
                if (mpi_errno2) MPIR_ERR_ADD(mpi_errno, mpi_errno2);
            }
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**tmpvc_connect_fail");
            /* no head-to-head issues to deal with, if we failed to recv the
               packet then there really was a problem */
        }
        else {
            MPIU_Assert(pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_ACK ||
                        pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_NAK);

            if (pkt_type == MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_ACK) {
                CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
                ASSIGN_SC_TO_VC(sc_vc_tcp, sc);
                MPID_nem_tcp_conn_est (sc_vc);
                sc_vc_tcp->connect_retry_count = 0; /* successfully connected, reset connection retry count */
                MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "c_tmpvcsent_handler(): connection established (fd=%d, sc=%p, sc->vc=%p)", sc->fd, sc, sc_vc));
            }
            else { /* pkt_type must be MPIDI_NEM_TCP_SOCKSM_PKT_ID_NAK */
                MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_c_tmpvcsent_handler() 2: closing sc"));
                MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "received NAK, closing sc");
                mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
            }
        }
    }

    MPIDI_FUNC_EXIT(MPID_STATE_STATE_C_TMPVCSENT_HANDLER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME state_l_cntd_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int state_l_cntd_handler(struct pollfd *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_NEM_TCP_SOCK_STATUS_t status;
    int got_sc_eof = 0;
    MPIDI_STATE_DECL(MPID_STATE_STATE_L_CNTD_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_L_CNTD_HANDLER);

    status = MPID_nem_tcp_check_sock_status(plfd);
    if (status == MPID_NEM_TCP_SOCK_ERROR_EOF) {
        MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "Received EOF, closing sc");
        mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
        goto fn_exit;
    }

    /* We have an active connection, start polling more often */
    MPID_nem_tcp_skip_polls = MAX_SKIP_POLLS_ACTIVE;

    if (IS_READABLE(plfd)) {
        mpi_errno = recv_id_or_tmpvc_info(sc, &got_sc_eof);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        if (got_sc_eof) {
            MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "got eof, closing sc");
            mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
            goto fn_exit;
        }
        
        if (!sc->is_tmpvc) {
            CHANGE_STATE(sc, CONN_STATE_TA_C_RANKRCVD);
        }
        else {
            CHANGE_STATE(sc, CONN_STATE_TA_C_TMPVCRCVD);
        }
    }
    else {
        /* remain in same state */
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "!IS_READABLE(plfd) fd=%d events=%#x revents=%#x", plfd->fd, plfd->events, plfd->revents));
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_L_CNTD_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
    mpi_errno = MPI_SUCCESS; /* don't return an error if we fail in the connect protocol, just retry. */
    goto fn_exit;
}

/*
  Returns TRUE, if the process(self) wins against the remote process
  FALSE, otherwise
 */
#undef FUNCNAME
#define FUNCNAME do_i_win
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int do_i_win(sockconn_t *rmt_sc)
{
    int win = FALSE;
    MPIDI_STATE_DECL(MPID_STATE_DO_I_WIN);

    MPIDI_FUNC_ENTER(MPID_STATE_DO_I_WIN);

    MPIU_Assert(rmt_sc->pg_is_set);

    if (rmt_sc->is_same_pg) {
        if (MPIDI_Process.my_pg_rank > rmt_sc->pg_rank)
            win = TRUE;
    }
    else {
        if (strcmp(MPIDI_Process.my_pg->id, rmt_sc->pg_id) > 0)
            win = TRUE;
    }

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE,
                     (MPIU_DBG_FDEST, "do_i_win(rmt_sc=%p (%s)) win=%s is_same_pg=%s my_pg_rank=%d rmt_pg_rank=%d",
                      rmt_sc, CONN_STATE_STR[rmt_sc->state.cstate],
                      (win ? "TRUE" : "FALSE"),(rmt_sc->is_same_pg ? "TRUE" : "FALSE"), MPIDI_Process.my_pg_rank,
                      rmt_sc->pg_rank));
    MPIDI_FUNC_EXIT(MPID_STATE_DO_I_WIN);
    return win;
}

#undef FUNCNAME
#define FUNCNAME state_l_rankrcvd_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int state_l_rankrcvd_handler(struct pollfd *const plfd, sockconn_t *const sc)
{
    MPIDI_VC_t *const sc_vc = sc->vc;
    MPID_nem_tcp_vc_area *const sc_vc_tcp = VC_TCP(sc_vc);
    int mpi_errno = MPI_SUCCESS;
    MPID_NEM_TCP_SOCK_STATUS_t status;
    sockconn_t *fnd_sc = NULL;
    int snd_nak = FALSE;
    MPIDI_STATE_DECL(MPID_STATE_STATE_L_RANKRCVD_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_L_RANKRCVD_HANDLER);

    status = MPID_nem_tcp_check_sock_status(plfd);
    if (status == MPID_NEM_TCP_SOCK_ERROR_EOF)
        goto fn_fail;

    if (IS_WRITEABLE(plfd)) {

        if (sc_vc->state == MPIDI_VC_STATE_INACTIVE_CLOSED) {
            mpi_errno = send_cmd_pkt(sc->fd, MPIDI_NEM_TCP_SOCKSM_PKT_CLOSED);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
        }
        
        if (found_better_sc(sc, &fnd_sc)) {
            if (fnd_sc->state.cstate == CONN_STATE_TS_COMMRDY)
                snd_nak = TRUE;
            else if (fnd_sc->state.cstate == CONN_STATE_TC_C_RANKSENT)
                snd_nak = do_i_win(sc);
        }
    
        if (snd_nak) {
            mpi_errno = send_cmd_pkt(sc->fd, MPIDI_NEM_TCP_SOCKSM_PKT_ID_NAK);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "lost head-to-head, closing sc");
            mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
        } else {
            /* The following line is _crucial_ to correct operation.  We need to
             * ensure that all head-to-head resolution has completed before we
             * move to COMMRDY and send any pending messages.  If we don't this
             * connection could shut down before the other connection has a
             * chance to finish the connect protocol.  That can lead to all
             * kinds of badness, including zombie connections, segfaults, and
             * accessing PG/VC info that is no longer present. */
            if (sc_vc_tcp->sc_ref_count > 1) goto fn_exit;

            mpi_errno = send_cmd_pkt(sc->fd, MPIDI_NEM_TCP_SOCKSM_PKT_ID_ACK);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            
            CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
            ASSIGN_SC_TO_VC(sc_vc_tcp, sc);
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "connection established: sc=%p, sc->vc=%p, sc->fd=%d, is_same_pg=%s, pg_rank=%d", sc, sc_vc, sc->fd, (sc->is_same_pg ? "TRUE" : "FALSE"), sc->pg_rank));
            MPID_nem_tcp_conn_est (sc_vc);
            sc_vc_tcp->connect_retry_count = 0; /* successfully connected, reset connection retry count */
        }
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_L_RANKRCVD_HANDLER);
    return mpi_errno;
fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
    mpi_errno = MPI_SUCCESS; /* don't return an error if we fail in the connect protocol, just retry. */
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME state_l_tmpvcrcvd_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int state_l_tmpvcrcvd_handler(struct pollfd *const plfd, sockconn_t *const sc)
{
    MPIDI_VC_t *const sc_vc = sc->vc;
    MPID_nem_tcp_vc_area *const sc_vc_tcp = VC_TCP(sc_vc);
    int mpi_errno = MPI_SUCCESS;
    MPID_NEM_TCP_SOCK_STATUS_t status;
    MPIDI_STATE_DECL(MPID_STATE_STATE_L_TMPVCRCVD_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_L_TMPVCRCVD_HANDLER);

    status = MPID_nem_tcp_check_sock_status(plfd);
    if (status == MPID_NEM_TCP_SOCK_ERROR_EOF) {
        MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "Received EOF, closing sc");
        mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
        goto fn_exit;
    }
    /* we don't want to perform any h2h resolution for temp vcs */
    if (IS_WRITEABLE(plfd)) {
        mpi_errno = send_cmd_pkt(sc->fd, MPIDI_NEM_TCP_SOCKSM_PKT_TMPVC_ACK);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
        ASSIGN_SC_TO_VC(sc_vc_tcp, sc);
        MPID_nem_tcp_conn_est(sc_vc);
        sc_vc_tcp->connect_retry_count = 0; /* successfully connected, reset connection retry count */
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "fd=%d: TMPVC_ACK sent, connection established!", sc->fd));
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_L_TMPVCRCVD_HANDLER);
    return mpi_errno;
 fn_fail:
    {
        int cleanup_errno = MPI_SUCCESS;
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
        mpi_errno = close_cleanup_and_free_sc_plfd(sc); /* QUIESCENT */
        if (cleanup_errno) {
            MPIR_ERR_SET(cleanup_errno, MPI_ERR_OTHER, "**tcp_cleanup_fail");
            MPIR_ERR_ADD(mpi_errno, cleanup_errno);
        }
        goto fn_exit;
    }
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_recv_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPID_nem_tcp_recv_handler(sockconn_t *const sc)
{
    /* as a result of handling a message, the sc table might get
       expanded making the sc pointer invalid, so we save all the info
       we need from sc here, and don't reference sc directly after
       that. */
    const int sc_fd = sc->fd;
    MPIDI_VC_t *const sc_vc = sc->vc;
    MPIU_AssertDeclValue(MPID_nem_tcp_vc_area *const sc_vc_tcp, VC_TCP(sc_vc));
    int mpi_errno = MPI_SUCCESS;
    ssize_t bytes_recvd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_RECV_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_RECV_HANDLER);

    if (sc_vc->ch.recv_active == NULL)
    {
        /* receive a new message */
        CHECK_EINTR(bytes_recvd, recv(sc_fd, recv_buf, MPID_NEM_TCP_RECV_MAX_PKT_LEN, 0));
        if (bytes_recvd <= 0)
        {
            if (bytes_recvd == -1 && errno == EAGAIN) /* handle this fast */
                goto fn_exit;

            if (bytes_recvd == 0)
            {
                MPIU_Assert(sc != NULL);
                MPIU_Assert(sc_vc != NULL);
                /* sc->vc->sc will be NULL if sc->vc->state == _INACTIVE */
                MPIU_Assert(sc_vc_tcp->sc == NULL || sc_vc_tcp->sc == sc);

                if (vc_is_in_shutdown(sc_vc))
                {
                    /* there's currently no hook for CH3 to tell nemesis/tcp
                       that we are in the middle of a disconnection dance.  So
                       if we don't check to see if we are currently
                       disconnecting, then we end up with a potential race where
                       the other side performs a tcp close() before we do and we
                       blow up here. */
                    MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "other side closed, but we're shutting down, closing sc");
                    /* it's really not an error, but we're calling
                       cleanup_on_error because it does what we want it to */
                    mpi_errno = MPID_nem_tcp_cleanup_on_error(sc_vc, MPI_SUCCESS);
                    goto fn_exit;
                }
                else
                {
                    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "ERROR: sock (fd=%d) is closed: bytes_recvd == 0", sc->fd );
                    MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sock_closed");
                }
            }
            else
            {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", MPIU_Strerror(errno));
            }
        }
    
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "New recv " MPIDI_MSG_SZ_FMT " (fd=%d, vc=%p, sc=%p)", bytes_recvd, sc_fd, sc_vc, sc));

        mpi_errno = MPID_nem_handle_pkt(sc_vc, recv_buf, bytes_recvd);
        if (mpi_errno) MPIR_ERR_POP_LABEL(mpi_errno, fn_noncomm_fail);
    }
    else
    {
        /* there is a pending receive, receive it directly into the user buffer */
        MPIDI_CH3I_VC *const sc_vc_ch = &sc_vc->ch;
        MPID_Request *const rreq = sc_vc_ch->recv_active;
        MPL_IOV *iov = &rreq->dev.iov[rreq->dev.iov_offset];
        int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

        MPIU_Assert(rreq->dev.iov_count > 0);
        MPIU_Assert(rreq->dev.iov_count + rreq->dev.iov_offset <= MPL_IOV_LIMIT);

        bytes_recvd = MPL_large_readv(sc_fd, iov, rreq->dev.iov_count);
        if (bytes_recvd <= 0)
        {
            if (bytes_recvd == -1 && errno == EAGAIN) /* handle this fast */
                goto fn_exit;
            
            if (bytes_recvd == 0) {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sock_closed");
            } else {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", MPIU_Strerror(errno));
            }
        }

        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "Cont recv %ld", (long int)bytes_recvd);

        /* update the iov */
        for (iov = &rreq->dev.iov[rreq->dev.iov_offset]; iov < &rreq->dev.iov[rreq->dev.iov_offset + rreq->dev.iov_count]; ++iov)
        {
            if (bytes_recvd < iov->MPL_IOV_LEN)
            {
                iov->MPL_IOV_BUF = (char *)iov->MPL_IOV_BUF + bytes_recvd;
                iov->MPL_IOV_LEN -= bytes_recvd;
                rreq->dev.iov_count = (int)(&rreq->dev.iov[rreq->dev.iov_offset + rreq->dev.iov_count] - iov);
                rreq->dev.iov_offset = iov - rreq->dev.iov;
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "bytes_recvd = %ld", (long int)bytes_recvd);
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "iov len = %ld", (long int)iov->MPL_IOV_LEN);
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "iov_offset = %lld", (long long)rreq->dev.iov_offset);
                goto fn_exit;
            }
            bytes_recvd -= iov->MPL_IOV_LEN;
        }
        
        /* the whole iov has been received */

        reqFn = rreq->dev.OnDataAvail;
        if (!reqFn)
        {
            MPIU_Assert(MPIDI_Request_get_type(rreq) != MPIDI_REQUEST_TYPE_GET_RESP);
            mpi_errno = MPID_Request_complete(rreq);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...complete");
            sc_vc_ch->recv_active = NULL;
        }
        else
        {
            int complete = 0;
                
            mpi_errno = reqFn(sc_vc, rreq, &complete);
            if (mpi_errno) MPIR_ERR_POP_LABEL(mpi_errno, fn_noncomm_fail);

            if (complete)
            {
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...complete");
                sc_vc_ch->recv_active = NULL;
            }
            else
            {
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...not complete");
            }
        }
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_RECV_HANDLER);
    return mpi_errno;
fn_fail: /* comm related failures jump here */
    {

        MPIR_ERR_SET1(mpi_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d", sc_vc->pg_rank);
        mpi_errno = MPID_nem_tcp_cleanup_on_error(sc_vc, mpi_errno);
        if (mpi_errno) {
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**tcp_cleanup_fail");
        }
    }
fn_noncomm_fail: /* NON-comm related failures jump here */
    goto fn_exit;
    
}

#undef FUNCNAME
#define FUNCNAME state_commrdy_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int state_commrdy_handler(struct pollfd *const plfd, sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    /* save references to sc->vc etc because if the sc table is
       expanded inside recv_handler, this pointer to sc will be
       invalid. */
    MPIDI_VC_t *sc_vc = sc->vc;
    MPID_nem_tcp_vc_area *sc_vc_tcp = VC_TCP(sc_vc);
    MPIDI_STATE_DECL(MPID_STATE_STATE_COMMRDY_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_COMMRDY_HANDLER);

    MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "vc = %p", sc->vc);
    if (IS_READABLE(plfd))
    {
        mpi_errno = MPID_nem_tcp_recv_handler(sc);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
    }
    if (IS_WRITEABLE(plfd) && !MPIDI_CH3I_Sendq_empty(sc_vc_tcp->send_queue))
    {
        mpi_errno = MPID_nem_tcp_send_queued(sc_vc, &sc_vc_tcp->send_queue);
        if (mpi_errno) MPIR_ERR_POP (mpi_errno);
        /* check to see if this VC is waiting for outstanding sends to
           finish in order to terminate */
        if (sc_vc->state == MPIDI_VC_STATE_CLOSED && MPIDI_CH3I_Sendq_empty(sc_vc_tcp->send_queue)) {
            mpi_errno = MPID_nem_tcp_vc_terminated(sc_vc);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_COMMRDY_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_sm_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_sm_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);
    /* Set the appropriate handlers */
    sc_state_info[CONN_STATE_TS_CLOSED].sc_state_handler = NULL;
    sc_state_info[CONN_STATE_TC_C_CNTING].sc_state_handler = state_tc_c_cnting_handler;
    sc_state_info[CONN_STATE_TC_C_CNTD].sc_state_handler = state_tc_c_cntd_handler;
    sc_state_info[CONN_STATE_TC_C_RANKSENT].sc_state_handler = state_c_ranksent_handler;
    sc_state_info[CONN_STATE_TC_C_TMPVCSENT].sc_state_handler = state_c_tmpvcsent_handler;
    sc_state_info[CONN_STATE_TA_C_CNTD].sc_state_handler = state_l_cntd_handler;
    sc_state_info[CONN_STATE_TA_C_RANKRCVD].sc_state_handler = state_l_rankrcvd_handler;
    sc_state_info[CONN_STATE_TA_C_TMPVCRCVD].sc_state_handler = state_l_tmpvcrcvd_handler;
    sc_state_info[CONN_STATE_TS_COMMRDY].sc_state_handler = state_commrdy_handler;

    /* Set the appropriate states */
    sc_state_info[CONN_STATE_TS_CLOSED].sc_state_plfd_events = 0;
    sc_state_info[CONN_STATE_TC_C_CNTING].sc_state_plfd_events = POLLOUT | POLLIN;
    sc_state_info[CONN_STATE_TC_C_CNTD].sc_state_plfd_events = POLLOUT | POLLIN;
    sc_state_info[CONN_STATE_TC_C_RANKSENT].sc_state_plfd_events = POLLIN;
    sc_state_info[CONN_STATE_TC_C_TMPVCSENT].sc_state_plfd_events = POLLIN;
    sc_state_info[CONN_STATE_TA_C_CNTD].sc_state_plfd_events = POLLIN;
    sc_state_info[CONN_STATE_TA_C_RANKRCVD].sc_state_plfd_events = POLLOUT | POLLIN;
    sc_state_info[CONN_STATE_TA_C_TMPVCRCVD].sc_state_plfd_events = POLLOUT | POLLIN;
    sc_state_info[CONN_STATE_TS_COMMRDY].sc_state_plfd_events = POLLIN;

    /* Allocate the PLFD table */
    g_tbl_size = 0;
    g_tbl_capacity = CONN_PLFD_TBL_INIT_SIZE;
    g_sc_tbl = NULL;
    MPID_nem_tcp_plfd_tbl = NULL;
    alloc_sc_plfd_tbls();
    
    MPIU_CHKPMEM_MALLOC(recv_buf, char*, MPID_NEM_TCP_RECV_MAX_PKT_LEN, mpi_errno, "TCP temporary buffer");
    MPIU_CHKPMEM_COMMIT();

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_sm_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_sm_finalize(void)
{
    freenode_t *node;

    /* walk the freeq and free all the elements */
    while (!Q_EMPTY(freeq)) {
        Q_DEQUEUE(&freeq, ((freenode_t **)&node)); 
        MPIU_Free(node);
    }

    free_sc_plfd_tbls();

    MPIU_Free(recv_buf);

    return MPI_SUCCESS;
}

/*
 N1: create a new listener fd?? While doing so, if we bind it to the same port used befor,
then it is ok. Else,the new port number(and thus the business card) has to be communicated 
to the other processes (in same and different pg's), which is not quite simple to do. 
Evaluate the need for it by testing and then do it, if needed.

*/
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_connpoll
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_connpoll(int in_blocking_poll)
{
    int mpi_errno = MPI_SUCCESS, n, i;
    static int num_skipped_polls = 0;

    /* num_polled is needed b/c the call to it_sc->handler() can change the
       size of the table, which leads to iterating over invalid revents. */
    int num_polled = g_tbl_size;

    if (num_polled) {
        MPIU_Assert(MPID_nem_tcp_plfd_tbl != NULL);
        MPIU_Assert(g_sc_tbl != NULL);
    }

    /* To improve shared memory performance, we don't call the poll()
     * system call every time. The MPID_nem_tcp_skip_polls value is
     * changed depending on whether we have any active connections.
     * We only skip polls when we're in a blocking progress loop in
     * order to avoid poor performance if the user does a "MPI_Test();
     * compute();" loop waiting for a req to complete. */
    if (in_blocking_poll && num_skipped_polls++ < MPID_nem_tcp_skip_polls)
        goto fn_exit;
    num_skipped_polls = 0;

    CHECK_EINTR(n, poll(MPID_nem_tcp_plfd_tbl, num_polled, 0));
    MPIR_ERR_CHKANDJUMP1(n == -1, mpi_errno, MPI_ERR_OTHER, "**poll", "**poll %s", MPIU_Strerror(errno));
    /* MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "some sc fd poll event")); */
    for(i = 0; i < num_polled; i++)
    {
        struct pollfd *it_plfd = &MPID_nem_tcp_plfd_tbl[i];
        sockconn_t *it_sc = &g_sc_tbl[i];

        if (it_plfd->fd != CONN_INVALID_FD && it_plfd->revents != 0)
        {
            /* We could check for POLLHUP here, but HUP/HUP+EOF is not erroneous
             * on many platforms, including modern Linux. */
            if (it_plfd->revents & POLLERR || it_plfd->revents & POLLNVAL) {
                int req_errno = MPI_SUCCESS;
                ssize_t rc;
                char dummy;
                const char *err_str ATTRIBUTE((unused)) = "UNKNOWN";

                /* See if we can get a specific error for this fd
                   (Stevens Network Programming Vol 1, pg 184) */
                rc = read(it_plfd->fd, &dummy, 1);
                if (rc < 0)
                    err_str = MPIU_Strerror(errno);
                
                MPIU_DBG_MSG(NEM_SOCK_DET, VERBOSE, "error polling fd, closing sc");
                if (it_sc->vc) {
                    MPIR_ERR_SET2(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d %s", it_sc->vc->pg_rank, err_str);
                    mpi_errno = MPID_nem_tcp_cleanup_on_error(it_sc->vc, req_errno);
                    MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**tcp_cleanup_fail");
                } else {
                    MPIR_ERR_SET2(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail_conn", "**comm_fail_conn %s %s", CONN_STATE_STR[it_sc->state.cstate], err_str);
                    mpi_errno = close_cleanup_and_free_sc_plfd(it_sc);
                    MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**tcp_cleanup_fail");
                }
                continue;
            }
            
            mpi_errno = it_sc->handler(it_plfd, it_sc);
            if (mpi_errno) MPIR_ERR_POP (mpi_errno);
            /* note that g_sc_tbl might have been expanded while
               inside handler, so the it_plfd and it_sc pointers may
               be invalid at this point. */
        }
    }
    
 fn_exit:
    return mpi_errno;
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_state_listening_handler(struct pollfd *const unused_1, sockconn_t *const unused_2)
        /*  listener fd poll struct and sockconn structure */
{
    int mpi_errno = MPI_SUCCESS;
    int connfd;
    socklen_t len;
    SA_IN rmt_addr;
    sockconn_t *l_sc;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_STATE_LISTENING_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_STATE_LISTENING_HANDLER);

    while (1) {
        l_sc = &g_sc_tbl[0];  /* N3 Important */
        len = sizeof(SA_IN);
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "before accept"));
        if ((connfd = accept(l_sc->fd, (SA *) &rmt_addr, &len)) < 0) {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "after accept, l_sc=%p lstnfd=%d connfd=%d, errno=%d:%s ", l_sc, l_sc->fd, connfd, errno, MPIU_Strerror(errno)));
            if (errno == EINTR) 
                continue;
            else if (errno == EWOULDBLOCK || errno == EAGAIN)
                break; /*  no connection in the listen queue. get out of here.(N1) */

            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**sock_accept", "**sock_accept %s", MPIU_Strerror(errno));
        }
        else {
            int idx = -1;
            struct pollfd *plfd;
            sockconn_t *sc;

            MPID_nem_tcp_set_sockopts(connfd); /* (N2) */
            mpi_errno = find_free_entry(&idx);
            if (mpi_errno != MPI_SUCCESS) MPIR_ERR_POP (mpi_errno);
            sc = &g_sc_tbl[idx];
            plfd = &MPID_nem_tcp_plfd_tbl[idx];
            
            sc->fd = plfd->fd = connfd;
            sc->pg_rank = CONN_INVALID_RANK;
            sc->pg_is_set = FALSE;
            sc->is_tmpvc = 0;

            CHANGE_STATE(sc, CONN_STATE_TA_C_CNTD);

            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "accept success, added to table, connfd=%d, sc->vc=%p, sc=%p plfd->events=%#x", connfd, sc->vc, sc, plfd->events)); /* sc->vc should be NULL at this point */
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_STATE_LISTENING_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* This is called whenever a vc has been closed as a result of an
   error.  The VC is put into an error state and cannot be opened
   again. */
#undef FUNCNAME
#define FUNCNAME error_closed
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int error_closed(struct MPIDI_VC *const vc, int req_errno)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_tcp_vc_area * const vc_tcp = VC_TCP(vc);
    MPIDI_STATE_DECL(MPID_STATE_ERROR_CLOSED);

    MPIDI_FUNC_ENTER(MPID_STATE_ERROR_CLOSED);

    vc_tcp->state = MPID_NEM_TCP_VC_STATE_ERROR;

    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPID_nem_tcp_error_out_send_queue(vc, req_errno);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_ERROR_CLOSED);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* This is called when an communication error has occurred on a VC to
   close the VC and release associated resources.
   Any outstanding requests will have MPI_ERROR set to req_errno */
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_cleanup_on_error
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_tcp_cleanup_on_error(MPIDI_VC_t *const vc, int req_errno)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno2 = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CLEANUP_ON_ERROR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_CLEANUP_ON_ERROR);

    mpi_errno = MPID_nem_tcp_cleanup(vc);
    /* not jumping on error, keep going */
    
    mpi_errno2 = error_closed(vc, req_errno);
    if (mpi_errno2) MPIR_ERR_ADD(mpi_errno, mpi_errno2);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_CLEANUP_ON_ERROR);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
