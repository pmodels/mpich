/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define SOCKSM_H_DEFGLOBALS_

#include "wintcp_impl.h"
#include "socksm.h"

typedef struct freenode {
    struct sockconn_t *sc;
    struct freenode* next;
} freenode_t;

/* freeq contains the list of sockconns no longer in use.
 * Use sockconns from this list before allocating a new sockconn
 */
static struct {
    freenode_t *head, *tail;
} freeq = {NULL, NULL};

#define SOCKCONN_TMP_BUF_SZ MPID_NEM_NEWTCP_MODULE_RECV_MAX_PKT_LEN

static int g_sc_tbl_size = 0;
static int g_sc_tbl_capacity = CONN_TBL_INIT_SIZE;
static int g_sc_tbl_grow_size = CONN_TBL_GROW_SIZE;

/* Current global socket conn table 
 * - This table contains the latest set of sockconn s allocated
 * - To get a free sockconn, first check the freeq and then check
 * the global sockconn table. If no sockconns available then expand
 * the global sockconn table.
 */
static sockconn_t *g_sc_tbl = NULL;

/* Each node of the global sc table list contains,
 * sc_tbl ==> sc_tbl allocated
 * sc_tbl_sz ==> size of sc_tbl
 * next ==> next sc_tbl
 */
typedef struct sc_tbl_list_node{
    sockconn_t *sc_tbl;
    int sc_tbl_sz;
    struct sc_tbl_list_node *next;
} sc_tbl_list_node_t;

/* Global sc table list containing the list of sc tables allocated 
 * Whenever an sc table is allocated, add it to this list/queue. This
 * list is used for browsing through all sockconns - for get/free
 */
struct {
    sc_tbl_list_node_t *head, *tail;
} g_sc_tbl_list = { NULL, NULL};

/* Forward iterator for the global sc table list */
typedef struct sc_tbl_fw_iterator{
    sc_tbl_list_node_t  *cur_sc_tbl_list_node;
    int g_sc_tbl_index;
} sc_tbl_fw_iterator_t;

/* Initialize the sc_tbl forward iterator */
static inline void sc_tbl_fw_iterator_init(sc_tbl_fw_iterator_t *iter)
{
    MPIU_Assert(iter != NULL);

    iter->cur_sc_tbl_list_node = g_sc_tbl_list.head;
    iter->g_sc_tbl_index = 0;
}

/* Finalize the sc_tbl iterator */
static inline void sc_tbl_fw_iterator_finalize(sc_tbl_fw_iterator_t *iter)
{
    sc_tbl_fw_iterator_init(iter);
}
/* Returns 1 is sc_tbl has more elements, 0 otherwise */
static inline int sc_tbl_has_next(sc_tbl_fw_iterator_t *iter)
{
    MPIU_Assert(iter != NULL);

    return( (iter->cur_sc_tbl_list_node != NULL) &&
            (   (iter->g_sc_tbl_index < iter->cur_sc_tbl_list_node->sc_tbl_sz) ||
                (iter->cur_sc_tbl_list_node->next != NULL) ) );
}

/* Get the next sc from sc_tbl. This iterator searches all the
 * sc_tbls registered with the global sc tbl list
 */
static inline sockconn_t * sc_tbl_get_next(sc_tbl_fw_iterator_t *iter)
{
    int cur_ind = 0;
    MPIU_Assert(iter != NULL);
    if(iter->g_sc_tbl_index >= iter->cur_sc_tbl_list_node->sc_tbl_sz){
        /* Move to the next sc tbl registered with global sc tbl list */
        iter->cur_sc_tbl_list_node = iter->cur_sc_tbl_list_node->next;
        iter->g_sc_tbl_index = 0;
    }
    MPIU_Assert(iter->cur_sc_tbl_list_node != NULL);
    cur_ind = iter->g_sc_tbl_index++;
    return (iter->cur_sc_tbl_list_node->sc_tbl + cur_ind);
}

/* Init global sc tbl list */
static inline int init_sc_tbl_list(void )
{
    Q_EMPTY(g_sc_tbl_list);
    return MPI_SUCCESS;
}

/* Add sc table to the global sc table list */
 static inline int add_sc_tbl_to_list(sockconn_t *sc_tbl, int sc_tbl_sz)
{
    sc_tbl_list_node_t *node = NULL;
    int mpi_errno = MPI_SUCCESS;

    node = MPIU_Malloc(sizeof(sc_tbl_list_node_t));      
    MPIU_ERR_CHKANDJUMP(node == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");

    node->sc_tbl = sc_tbl;
    node->sc_tbl_sz = sc_tbl_sz;
    node->next = NULL;

    Q_ENQUEUE(&g_sc_tbl_list, node);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Free sc tables in the global sc table list */
static inline int free_sc_tbls_in_list(void )
{
    while(!Q_EMPTY(g_sc_tbl_list)){
        sc_tbl_list_node_t *node;
        Q_DEQUEUE(&g_sc_tbl_list, &node);
        MPIU_Free(node->sc_tbl);
    }
    return MPI_SUCCESS;
}

/* Free sc tbl list */
static inline int free_sc_tbl_list(void )
{
    while(!Q_EMPTY(g_sc_tbl_list)){
        sc_tbl_list_node_t *node;
        Q_DEQUEUE(&g_sc_tbl_list, &node);
        MPIU_Free(node);
    }
    /* Sanity reset of the queue */
    Q_EMPTY(g_sc_tbl_list);
    return MPI_SUCCESS;
}

/* FIXME: Why should the listen sock conn be global ? */
sockconn_t MPID_nem_newtcp_module_g_lstn_sc;

/* newtcp module executive set handle */
MPIU_ExSetHandle_t MPID_nem_newtcp_module_ex_set_hnd = MPIU_EX_INVALID_SET;

/* We define this in order to trick the compiler into including
   information about the MPID_nem_newtcp_module_vc_area type.  This is
   needed to easily expand the VC_FIELD macro in a debugger.  The
   'unused' attribute keeps the compiler from complaining.  The 'used'
   attribute makes sure the symbol is added in the lib, even if it's
   unused */
static MPID_nem_newtcp_module_vc_area *dummy_vc_area ATTRIBUTE((unused, used)) = NULL;

/* FIXME: Tune the skip poll values since we now have a 
 * fast executive (instead of select)
 */
#define MIN_SKIP_POLLS_INACTIVE (512)
#define SKIP_POLLS_INC_CNT  (512)
#define MAX_SKIP_POLLS_INACTIVE (1024)
#define SKIP_POLLS_INC(cnt) (                               \
    (cnt >= MAX_SKIP_POLLS_INACTIVE)                        \
    ? (cnt = MIN_SKIP_POLLS_INACTIVE)                       \
    : (cnt += SKIP_POLLS_INC_CNT)                           \
)

/* There is very less overhead in handling req using iocp. Do we
 * need to skip at all ?
 */
#define MAX_SKIP_POLLS_ACTIVE (16)
static int MPID_nem_tcp_skip_polls = MIN_SKIP_POLLS_INACTIVE;

/* Debug function to dump the sockconn table.  This is intended to be
   called from a debugger.  The 'unused' attribute keeps the compiler
   from complaining.  The 'used' attribute makes sure the function is
   added in the lib, despite the fact that it's unused. */
static void dbg_print_sc_tbl(FILE *stream, int print_free_entries) ATTRIBUTE((unused, used));

typedef struct sc_state_info{
    /* FIXME: We won't need a common place holder for these handlers. Remove it eventually. */
    handler_func_t sc_state_rd_success_handler;
    handler_func_t sc_state_rd_fail_handler;
    handler_func_t sc_state_wr_success_handler;
    handler_func_t sc_state_wr_fail_handler;
} sc_state_info_t;

sc_state_info_t sc_state_info[SOCK_STATE_SIZE];

#define IS_SAME_PGID(id1, id2) \
    (strcmp(id1, id2) == 0)

/* Will evaluate to false if either one of these sc's does not have valid pg data */
#define IS_SAME_CONNECTION(sc1, sc2)                                    \
    (sc1->pg_is_set && sc2->pg_is_set &&                                \
     sc1->pg_rank == sc2->pg_rank &&                                    \
     ((sc1->is_same_pg && sc2->is_same_pg) ||                           \
      (!sc1->is_same_pg && !sc2->is_same_pg &&                          \
       IS_SAME_PGID(sc1->pg_id, sc2->pg_id))))

/* Initialize an sc entry - Initialize sc before using it for the
 * first time, or before reusing it
 */
#define INIT_SC_ENTRY(sc, ind)                                      \
    do {                                                            \
        (sc)->fd = MPIU_SOCKW_SOCKFD_INVALID;                       \
        MPIU_ExInitOverlapped(&((sc)->rd_ov), NULL, NULL);          \
        MPIU_ExInitOverlapped(&((sc)->wr_ov), NULL, NULL);          \
        memset(&((sc)->read), 0x0, sizeof(sock_read_context_t));    \
        memset(&((sc)->write), 0x0, sizeof(sock_write_context_t));  \
        memset(&((sc)->close), 0x0, sizeof(sock_close_context_t));  \
        (sc)->tmp_buf = (char *)MPIU_Malloc(SOCKCONN_TMP_BUF_SZ);   \
        (sc)->tmp_buf_len = SOCKCONN_TMP_BUF_SZ;                    \
        (sc)->index = ind;                                          \
        (sc)->vc = NULL;                                            \
        (sc)->pg_is_set = FALSE;                                    \
        (sc)->is_tmpvc = FALSE;                                     \
        (sc)->state = CONN_STATE_TS_CLOSED;                         \
    } while (0)

/* Finalize an sc */
#define FINALIZE_SC_ENTRY(sc, ind)                                  \
    do {                                                            \
        (sc)->fd = MPIU_SOCKW_SOCKFD_INVALID;                       \
        if((sc)->tmp_buf) MPIU_Free((sc)->tmp_buf);                 \
        (sc)->tmp_buf_len = 0;                                      \
        (sc)->index = -1;                                           \
        (sc)->vc = NULL;                                            \
        (sc)->pg_is_set = FALSE;                                    \
        (sc)->is_tmpvc = FALSE;                                     \
        (sc)->state = CONN_STATE_TS_CLOSED;                         \
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
    for (i = 0; i < (print_free_entries ? g_sc_tbl_capacity : g_sc_tbl_size); ++i) {
        sc = &g_sc_tbl[i];
#define TF(_b) ((_b) ? "TRUE" : "FALSE")
        fprintf(stream, "[%d] ptr=%p idx=%d fd=%d state=%s\n",
                i, sc, sc->index, sc->fd, SOCK_STATE_TO_STRING(sc->state));
        fprintf(stream, "....pg_is_set=%s is_same_pg=%s is_tmpvc=%s pg_rank=%d pg_id=%s\n",
                TF(sc->pg_is_set), TF(sc->is_same_pg), TF(sc->is_tmpvc), sc->pg_rank, sc->pg_id);
#undef TF
    }
    fprintf(stream, "========================================\n");
}
/* --END ERROR HANDLING-- */

/* Reset the rd context on a sock conn */
static inline int MPID_nem_newtcp_module_reset_readv_ex(sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(sc != NULL);
    sc->read.n_iov = -1;

    return mpi_errno;
}

/* Reset the wr context on a sock conn */
static inline int MPID_nem_newtcp_module_reset_writev_ex(sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(sc != NULL);
    sc->write.n_iov = -1;

    return mpi_errno;
}

/* Post a read on a sock conn with an iov
 * This function assumes that the read handlers are set prior to 
 * calling this func. Note: Don't post multiple simultaneous reads on the same sc
 * Input:
 * sc - ptr to sock connection
 * iov - ptr to the device IOV to read data.
 * n_iov - num of iovs
 */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_post_readv_ex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_post_readv_ex(sockconn_t *sc, MPID_IOV *iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_NEWTCP_POST_READV_EX);

    MPIDI_FUNC_ENTER(MPID_STATE_NEWTCP_POST_READV_EX);

    MPIU_Assert(sc != NULL);

    if(iov != NULL){
        /* A fresh post on sc */
        MPIU_Assert(n_iov > 0);
        sc->read.iov = iov;
        sc->read.n_iov = n_iov;
        sc->read.nb = 0;
    }
    else{
        /* We are continuing a read on the iov */
        MPIU_Assert(sc->read.iov != NULL);
        /* Make sure that we have some space left in the iov */
        MPIU_Assert((sc->read.iov)->MPID_IOV_LEN > 0);
    }

    mpi_errno = MPIU_SOCKW_Readv_ex(sc->fd, sc->read.iov, sc->read.n_iov, NULL, &(sc->rd_ov));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_NEWTCP_POST_READV_EX);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Post a read on a sock conn without an iov (eg: a char buf)
 * This function assumes that the read handlers are set prior to 
 * calling this func.
 * Note: Don't post multiple simultaneous reads on the same sc
 * Input:
 * sc   - ptr to sock conn
 * buf  - buffer to read data
 * nb   - size of buffer, buf
 */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_post_read_ex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_post_read_ex(sockconn_t *sc, void *buf, int nb)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_NEWTCP_POST_READ_EX);

    MPIDI_FUNC_ENTER(MPID_STATE_NEWTCP_POST_READ_EX);

    MPIU_Assert(sc != NULL);
    MPIU_Assert(buf != NULL);
    MPIU_Assert(nb > 0);

    sc->read.tmp_iov.MPID_IOV_BUF = buf;
    sc->read.tmp_iov.MPID_IOV_LEN = nb;

    sc->read.iov = &(sc->read.tmp_iov);
    sc->read.n_iov = 1;
    sc->read.nb = 0;

    mpi_errno = MPIU_SOCKW_Readv_ex(sc->fd, sc->read.iov, sc->read.n_iov, NULL, &(sc->rd_ov));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_NEWTCP_POST_READ_EX);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Post a dummy read on a sock conn */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_post_dummy_read_ex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_post_dummy_read_ex(sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_NEWTCP_POST_DUMMY_READ_EX);

    MPIDI_FUNC_ENTER(MPID_STATE_NEWTCP_POST_DUMMY_READ_EX);

    MPIU_Assert(sc != NULL);

    mpi_errno = MPID_nem_newtcp_module_reset_readv_ex(sc);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPIU_ExPostOverlapped(MPID_nem_newtcp_module_ex_set_hnd, MPIU_EX_WIN32_COMP_PROC_KEY, &(sc->rd_ov));

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_NEWTCP_POST_DUMMY_READ_EX);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Post a write on socket with iov
 * This function assumes that the write handlers are set prior to 
 * calling this func. Note: Don't post multiple simultaneous writes on the same sc
 * Input:
 * sc - ptr to sock connection
 * iov - ptr to the device IOV containing the data to write.
 * n_iov - num of iovs
 */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_post_writev_ex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_post_writev_ex(sockconn_t *sc, MPID_IOV *iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_NEWTCP_POST_WRITEV_EX);

    MPIDI_FUNC_ENTER(MPID_STATE_NEWTCP_POST_WRITEV_EX);

    MPIU_Assert(sc != NULL);

    if(iov != NULL){
        MPIU_Assert(n_iov > 0);
        sc->write.iov = iov;
        sc->write.n_iov = n_iov;
        sc->write.nb = 0;
    }
    else{
        /* Continue writing on the write iov */
        MPIU_Assert(sc->write.iov != NULL);
        /* Sanity check that there is something to write */
        MPIU_Assert(sc->write.iov->MPID_IOV_LEN > 0);
        MPIU_Assert(sc->write.n_iov > 0);
    }

    mpi_errno = MPIU_SOCKW_Writev_ex(sc->fd, sc->write.iov, sc->write.n_iov, NULL, &(sc->wr_ov));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_NEWTCP_POST_WRITEV_EX);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Post a write on socket without iov
 * This function assumes that the write handlers are set prior to 
 * calling this func.
 * Note: Don't post multiple simultaneous writes on the same sc
 * Input:
 * sc   - ptr to sock conn
 * buf  - buffer to write data from
 * nb   - size of buffer, buf
 */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_post_write_ex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_post_write_ex(sockconn_t *sc, void *buf, int nb)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_NEWTCP_POST_WRITE_EX);

    MPIDI_FUNC_ENTER(MPID_STATE_NEWTCP_POST_WRITE_EX);
    MPIU_Assert(sc != NULL);
    MPIU_Assert(buf != NULL);
    MPIU_Assert(nb > 0);

    sc->write.tmp_iov.MPID_IOV_BUF = buf;
    sc->write.tmp_iov.MPID_IOV_LEN = nb;

    sc->write.iov = &(sc->write.tmp_iov);
    sc->write.n_iov = 1;
    sc->write.nb = 0;

    mpi_errno = MPIU_SOCKW_Writev_ex(sc->fd, sc->write.iov, sc->write.n_iov, NULL, &(sc->wr_ov));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_NEWTCP_POST_WRITE_EX);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Post a connect on sc */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_post_connect_ex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPID_nem_newtcp_module_post_connect_ex(sockconn_t *sc, struct sockaddr_in *sin)
{
    int mpi_errno = MPI_SUCCESS;
    /* ConnectEx() requires the connect sock to be bound */
    struct sockaddr_in sockaddr_bindany;
    MPIDI_STATE_DECL(MPID_STATE_NEWTCP_POST_CONNECT_EX);

    MPIDI_FUNC_ENTER(MPID_STATE_NEWTCP_POST_CONNECT_EX);

    MPIU_Assert(sc != NULL);
    MPIU_Assert(sin != NULL);

    memset(&(sc->connect), 0x0, sizeof(sock_connect_context_t));
    /* Required when we retry connect */
    memcpy(&(sc->connect.sin), sin, sizeof(struct sockaddr_in));

    /* Bind the connect socket */
    memset(&sockaddr_bindany, 0x0, sizeof(struct sockaddr_in));
    sockaddr_bindany.sin_family = AF_INET;
    sockaddr_bindany.sin_addr.s_addr = INADDR_ANY;
    sockaddr_bindany.sin_port = htons(0);

    mpi_errno = MPIU_SOCKW_Bind(sc->fd, &sockaddr_bindany, sizeof(struct sockaddr_in));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIU_SOCKW_Connect_ex(sc->fd, sin, sizeof(struct sockaddr_in), &(sc->wr_ov));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_NEWTCP_POST_CONNECT_EX);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Post an accept on sc */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_post_accept_ex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPID_nem_newtcp_module_post_accept_ex(sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_NEWTCP_POST_ACCEPT_EX);

    MPIDI_FUNC_ENTER(MPID_STATE_NEWTCP_POST_ACCEPT_EX);
    MPIU_Assert(sc != NULL);

    memset(&(sc->accept), 0x0, sizeof(sock_listen_context_t));
    mpi_errno = MPIU_SOCKW_Sock_open(AF_INET, SOCK_STREAM, IPPROTO_TCP, &(sc->accept.connfd));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIU_SOCKW_Accept_ex(sc->fd, sc->accept.accept_buffer,
        sizeof(sc->accept.accept_buffer), &(sc->accept.connfd), &(sc->rd_ov), &(sc->accept.nb));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_NEWTCP_POST_ACCEPT_EX);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Post an close on sc */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_post_close_ex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPID_nem_newtcp_module_post_close_ex(sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_NEWTCP_POST_CLOSE_EX);

    MPIDI_FUNC_ENTER(MPID_STATE_NEWTCP_POST_CLOSE_EX);
    MPIU_Assert(sc != NULL);

    /* Flush write sock buffers - do we need to check the retval ?*/
    FlushFileBuffers((HANDLE )sc->fd);

    sc->close.conn_closing = 1;

    mpi_errno = MPIU_SOCKW_Sock_close(sc->fd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPIU_ExPostOverlapped(MPID_nem_newtcp_module_ex_set_hnd, MPIU_EX_WIN32_COMP_PROC_KEY, &(sc->wr_ov));

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_NEWTCP_POST_CLOSE_EX);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* This function trims the iov array, *iov_p, of size *n_iov_p
 * assuming nb bytes are transferred
 * Side-effect : *iov_p, *n_iov_p, buf & len of (*iov_p)
 *  could be modified by this function.
 */
static void trim_iov(MPID_IOV **iov_p, int *n_iov_p, unsigned int nb)
{
    MPID_IOV *cur_iov;
    int cur_n_iov;

    MPIU_Assert(iov_p);
    MPIU_Assert(*iov_p);
    MPIU_Assert(n_iov_p);
    MPIU_Assert((*n_iov_p) > 0);

    cur_iov = *iov_p;
    cur_n_iov = *n_iov_p;

    while(nb > 0){
        if(nb < cur_iov->MPID_IOV_LEN){
            cur_iov->MPID_IOV_BUF += nb;
            cur_iov->MPID_IOV_LEN -= nb;
            nb = 0;
        }
        else{
            nb -= cur_iov->MPID_IOV_LEN;
            cur_iov->MPID_IOV_LEN = 0;
            cur_n_iov--;
            if(cur_n_iov > 0){
                cur_iov++;
            }
            else{
                /* We should never have read more data
                 * than available in the IOV
                 */
                MPIU_Assert(nb == 0);
                break;
            }
        }
    }

    *iov_p = cur_iov;
    *n_iov_p = cur_n_iov;
}

/* Returns the total number of bytes read on sc for the posted read
 * Output:
 *  complete - (0/1) indicates whether the current read is complete
 * Note: (complete == 1) if sock is closed or when using EX bypass
 * Note: On a timeout (complete == 0)
 */
static int ex_read_progress_update(sockconn_t *sc, int *complete)
{
    int nb = 0;

    MPIU_Assert(sc != NULL);
    MPIU_Assert(complete != NULL);

    *complete = 0;
    if(sc->read.n_iov < 0){
        /* Bail out if no iov to read from - Useful for handlers
         * called explicitly by the user - EX bypass
         */
        sc->read.nb = 0;
        *complete = 1;
        goto fn_exit;
    }

    nb = MPIU_ExGetBytesTransferred(&(sc->rd_ov));
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "EX Read (sc = %p) = %d", sc, nb));

    if(nb <= 0){
        *complete = 1;
        goto fn_exit;
    }
    else{
        /* Update IOV */
        trim_iov(&(sc->read.iov), &(sc->read.n_iov), nb);
        sc->read.nb += nb;
        if(sc->read.n_iov == 0) *complete = 1;
    }

 fn_exit:
    return sc->read.nb;
}

/* Returns the total number of bytes written on sc for the posted wr
 * Output:
 *  complete - (0/1) indicates whether the current write is complete
 * Note: (complete == 1) if sock is closed or when using EX bypass
 * Note: On a timeout (complete == 0)
 */
static int ex_write_progress_update(sockconn_t *sc, int *complete)
{
    int nb = 0;

    MPIU_Assert(sc != NULL);
    MPIU_Assert(complete != NULL);

    *complete = 0;
    if(sc->write.n_iov < 0){
        sc->write.nb = 0;
        *complete = 1;
        goto fn_exit;
    }

    nb = MPIU_ExGetBytesTransferred(&(sc->wr_ov));
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "EX Write (sc = %p) = %d", sc, nb));

    if(nb <= 0){
        *complete = 1;
        goto fn_exit;
    }
    else{
        /* Update IOV */
        trim_iov(&(sc->write.iov), &(sc->write.n_iov), nb);
        sc->write.nb += nb;
        if(sc->write.n_iov == 0) *complete = 1;
    }

 fn_exit:
    return sc->write.nb;
}

static int find_free_sc(sockconn_t **sc_p);

/* Allocate a global sc table */
#undef FUNCNAME
#define FUNCNAME alloc_sc_tbl
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int alloc_sc_tbl(void)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL (2);

    MPIU_Assert(g_sc_tbl == NULL);
    MPIU_CHKPMEM_MALLOC (g_sc_tbl, sockconn_t *, g_sc_tbl_capacity * sizeof(sockconn_t), 
                         mpi_errno, "connection table");

    for (i = 0; i < g_sc_tbl_capacity; i++) {
        INIT_SC_ENTRY(((sockconn_t *)&g_sc_tbl[i]), i);
    }
    MPIU_CHKPMEM_COMMIT();

    /* Add sc tbl to global list of sc tbls */
    mpi_errno = add_sc_tbl_to_list(g_sc_tbl, g_sc_tbl_capacity);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* Free all sc tables allocated */
#undef FUNCNAME
#define FUNCNAME free_sc_tbls
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int free_sc_tbls (void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = free_sc_tbls_in_list();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/*
  Reason for not doing realloc for both sc and plfd tables :
  Either both the tables have to be expanded or both should remain the same size, if
  enough memory could not be allocated, as we have only one set of variables to control
  the size of the tables. Also, it is not useful to expand one table and leave the other
  at the same size, 'coz of memory allocation failures.
*/
/*
    Since we no longer use pollfd tables (hence no more same index for pollfd and sc
    tables) when expanding sc table we should just allocate a new sc table and add
    the sc table to the global list of sc tables.
*/
#undef FUNCNAME
#define FUNCNAME expand_sc_tbl
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int expand_sc_tbl (void)
{
    int mpi_errno = MPI_SUCCESS; 
    sockconn_t *new_sc_tbl = NULL;
    /* FIXME: We need to check whether a non-linear growth of the sc_tbl
     * will be better for performance for large jobs
     */
    int new_capacity = g_sc_tbl_capacity + g_sc_tbl_grow_size;
    MPIU_CHKPMEM_DECL (2);

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "expand_sc_tbl Entry"));
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "expand_sc_tbl b4 g_sc_tbl[0].fd=%d", g_sc_tbl[0].fd));

    MPIU_CHKPMEM_MALLOC (new_sc_tbl, sockconn_t *, new_capacity * sizeof(sockconn_t), 
                         mpi_errno, "expanded connection table");

    g_sc_tbl = new_sc_tbl;
    g_sc_tbl_capacity = new_capacity;
    g_sc_tbl_size = 0;

    /* Add the new sc table to global sc table list */
    mpi_errno = add_sc_tbl_to_list(g_sc_tbl, new_capacity);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

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
  ==== OLD ====
  Finds the first free entry in the connection table/pollfd table. Note that both the
  tables are indexed the same. i.e. each entry in one table corresponds to the
  entry of the same index in the other table. If an entry is valid in one table, then
  it is valid in the other table too.
  ==== OLD ====

  This function finds the first free entry in both the tables by checking the queue of
  free elements. If the free queue is empty, then it returns the next available slot
  in the tables. If the size of the slot is already full, then this expands the table
  and then returns the next available slot
*/
#undef FUNCNAME
#define FUNCNAME find_free_sc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int find_free_sc(sockconn_t **sc_p)
{
    int mpi_errno = MPI_SUCCESS;
    freenode_t *node;

    MPIU_Assert(sc_p != NULL);
    /* FIXME: Use if-else and get rid of goto */
    if (!Q_EMPTY(freeq)) {
        Q_DEQUEUE(&freeq, ((freenode_t **)&node)); 
        *sc_p = node->sc;
        MPIU_Free(node);
        INIT_SC_ENTRY(*sc_p, -1);
        goto fn_exit;
    }

    /* Look into the latest global sc table allocated for an entry */
    if (g_sc_tbl_size == g_sc_tbl_capacity) {        
        mpi_errno = expand_sc_tbl();
        if (mpi_errno != MPI_SUCCESS){ MPIU_ERR_POP(mpi_errno); }
    }

    MPIU_Assert(g_sc_tbl_capacity > g_sc_tbl_size);

    *sc_p = &g_sc_tbl[g_sc_tbl_size];
    INIT_SC_ENTRY(*sc_p, g_sc_tbl_size);
    ++g_sc_tbl_size;

 fn_exit:
    /* This function is the closest thing we have to a constructor, so we throw
       in a couple of initializers here in case the caller is sloppy about his
       assumptions. */
    /* FIXME: The index could be invalid in the case of an error */
    /* INIT_SC_ENTRY(&g_sc_tbl[*index], *index); */
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int found_better_sc(sockconn_t *sc, sockconn_t **fnd_sc)
{
    int found = FALSE;
    sc_tbl_fw_iterator_t iter;
    MPIDI_STATE_DECL(MPID_STATE_FOUND_BETTER_SC);

    MPIDI_FUNC_ENTER(MPID_STATE_FOUND_BETTER_SC);

    /* tmpvc's can never match a better sc */
    if (sc->is_tmpvc) {
        found = FALSE;
        goto fn_exit;
    }

    /* if we don't know our own pg info, how can we look for a better SC? */
    MPIU_Assert(sc->pg_is_set);

    sc_tbl_fw_iterator_init(&iter);

    while(sc_tbl_has_next(&iter)){
        sockconn_t *iter_sc = sc_tbl_get_next(&iter);
        MPID_nem_newtcp_module_sock_state_t istate = iter_sc->state;
        if (iter_sc != sc && iter_sc->fd != MPIU_SOCKW_SOCKFD_INVALID 
            && IS_SAME_CONNECTION(iter_sc, sc)){
            switch (sc->state){
            case CONN_STATE_TC_C_CNTD:
                MPIU_Assert(fnd_sc == NULL);
                if (istate == CONN_STATE_TS_COMMRDY ||
                    istate == CONN_STATE_TA_C_RANKRCVD ||
                    istate == CONN_STATE_TC_C_TMPVCSENT){
                    found = TRUE;
                }
                break;
            case CONN_STATE_TA_C_RANKRCVD:
                MPIU_Assert(fnd_sc != NULL);
                if (istate == CONN_STATE_TS_COMMRDY ||
                    istate == CONN_STATE_TC_C_RANKSENT) {
                    found = TRUE;
                    *fnd_sc = iter_sc;
                }
                break; 
            case CONN_STATE_TA_C_TMPVCRCVD:
                MPIU_Assert(fnd_sc != NULL);
                if (istate == CONN_STATE_TS_COMMRDY ||
                    istate == CONN_STATE_TC_C_TMPVCSENT) {
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
    sc_tbl_fw_iterator_finalize(&iter);

fn_exit:
    if (found) {
        if (fnd_sc) {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE,
                             (MPIU_DBG_FDEST, "found_better_sc(sc=%p (%s), *fnd_sc=%p (%s)) found=TRUE",
                              sc, SOCK_STATE_STR[sc->state],
                              *fnd_sc, (*fnd_sc ? SOCK_STATE_STR[(*fnd_sc)->state] : "N/A")));
        }
        else {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE,
                             (MPIU_DBG_FDEST, "found_better_sc(sc=%p (%s), fnd_sc=(nil)) found=TRUE",
                              sc, SOCK_STATE_STR[sc->state]));
        }
    }
    else {
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE,
                         (MPIU_DBG_FDEST, "found_better_sc(sc=%p (%s), *fnd_sc=N/A) found=FALSE",
                          sc, SOCK_STATE_STR[sc->state]));
    }
    MPIDI_FUNC_EXIT(MPID_STATE_FOUND_BETTER_SC);
    return found;
}

#undef FUNCNAME
#define FUNCNAME vc_is_in_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_id_info(const sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_nem_newtcp_module_idinfo_t id_info;
    MPIDI_nem_newtcp_module_header_t hdr;
    MPID_IOV iov[3];
    int pg_id_len = 0, offset, buf_size, iov_cnt = 2;
    MPIDI_STATE_DECL(MPID_STATE_SEND_ID_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_ID_INFO);

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

    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )&hdr;
    iov[0].MPID_IOV_LEN = sizeof(hdr);
    iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )&id_info;
    iov[1].MPID_IOV_LEN = sizeof(id_info);
    buf_size = sizeof(hdr) + sizeof(id_info);

    if (!sc->is_same_pg) {
        iov[2].MPID_IOV_BUF = MPIDI_Process.my_pg->id;
        iov[2].MPID_IOV_LEN = pg_id_len;
        buf_size += pg_id_len;
        ++iov_cnt;
    }

    /* FIXME: Post a write instead of assuming a write would succeed 
     * Looks like we should be *usually* ok here since the conn is just
     * established and socket send buffer > sizeof iov 
     */
    MPIU_OSW_RETRYON_INTR((offset == -1), 
        (mpi_errno = MPIU_SOCKW_Writev(sc->fd, iov, iov_cnt, &offset)));
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    MPIU_ERR_CHKANDJUMP1 (offset != buf_size, mpi_errno, MPI_ERR_OTHER, 
                          "**write", "**write %s", MPIU_OSW_Strerror(MPIU_OSW_Get_errno())); 

/*     FIXME log appropriate error */
/*     FIXME-Z1  socket is just connected and we are sending a few bytes. So, there should not */
/*     be a problem of partial data only being written to. If partial data written, */
/*     handle this. */

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_ID_INFO);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d, offset=%d, errno=%d %s", 
            mpi_errno, offset, MPIU_OSW_Get_errno(), MPIU_OSW_Strerror(MPIU_OSW_Get_errno())));
    goto fn_exit;    
}

#undef FUNCNAME
#define FUNCNAME send_tmpvc_info
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_tmpvc_info(const sockconn_t *const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_nem_newtcp_module_portinfo_t port_info;
    MPIDI_nem_newtcp_module_header_t hdr;
    MPID_IOV iov[3];
    int offset, buf_size, iov_cnt = 2;
    MPIDI_STATE_DECL(MPID_STATE_SEND_TMPVC_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_TMPVC_INFO);

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "my->pg_rank=%d, sc->pg_rank=%d"
                                             , MPIDI_Process.my_pg_rank, sc->pg_rank));

/*     store ending NULL also */
/*     FIXME better keep pg_id_len itself as part of MPIDI_Process.my_pg structure to */
/*     avoid computing the length of string everytime this function is called. */

    hdr.pkt_type = MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_INFO;
    hdr.datalen = sizeof(MPIDI_nem_newtcp_module_portinfo_t);
    port_info.port_name_tag = sc->vc->port_name_tag;

    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )&hdr;
    iov[0].MPID_IOV_LEN = sizeof(hdr);
    iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )&port_info;
    iov[1].MPID_IOV_LEN = sizeof(port_info);
    buf_size = sizeof(hdr) + sizeof(port_info);

    /* FIXME: Post a write instead of assuming a write would succeed
     * Looks like we should be *usually* ok here since the conn is just
     * established and socket send buffer > sizeof iov 
     */
    MPIU_OSW_RETRYON_INTR((offset == -1), 
        (mpi_errno = MPIU_SOCKW_Writev(sc->fd, iov, iov_cnt, &offset)));
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    MPIU_ERR_CHKANDJUMP1 (offset != buf_size, mpi_errno, MPI_ERR_OTHER, 
                          "**write", "**write %s", MPIU_OSW_Strerror(MPIU_OSW_Get_errno())); 
/*     FIXME log appropriate error */
/*     FIXME-Z1  socket is just connected and we are sending a few bytes. So, there should not */
/*     be a problem of partial data only being written to. If partial data written, */
/*     handle this. */

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_TMPVC_INFO);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d, offset=%d, errno=%d %s", mpi_errno, offset, errno, strerror(errno)));
    goto fn_exit;
}

static int cleanup_sc_vc(sockconn_t *sc);

#undef FUNCNAME
#define FUNCNAME gen_read_fail_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int gen_read_fail_handler(MPIU_EXOVERLAPPED *rd_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    MPIU_Ex_status_t ex_status;
    MPIDI_STATE_DECL(MPID_STATE_STATE_GEN_RD_FAIL_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_GEN_RD_FAIL_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_RD_OV(rd_ov);
    MPIU_Assert(sc != NULL);

    ex_status = MPIU_ExGetStatus(rd_ov);
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "Read failed (ex_status = %x)", ex_status));
    if((ex_status == MPIU_EX_STATUS_IO_ABORT) && !(IS_SOCKCONN_CLOSING(sc))){
        /* Thread aborted */
        int nb, complete;
        
        nb = ex_read_progress_update(sc, &complete);

        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "THREAD ABORT: Reposting read (already read %d bytes) on sc=%p", nb, sc));
        /* Try to repost the read */
        mpi_errno = MPID_nem_newtcp_module_post_readv_ex(sc, NULL, 0);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else{
        mpi_errno = cleanup_sc_vc(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);

        /* FIXME: We should ideally do a shutdown() not close () */
        mpi_errno = MPID_nem_newtcp_module_post_close_ex(sc);
    
        MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**read",
            "**read %s", MPIU_OSW_Strerror(MPIU_EX_STATUS_TO_ERRNO(MPIU_ExGetStatus(rd_ov))));
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_GEN_RD_FAIL_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME gen_write_fail_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int gen_write_fail_handler(MPIU_EXOVERLAPPED *wr_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    MPIU_Ex_status_t ex_status;
    MPIDI_STATE_DECL(MPID_STATE_STATE_GEN_WR_FAIL_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_GEN_WR_FAIL_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_WR_OV(wr_ov);
    MPIU_Assert(sc != NULL);

    ex_status = MPIU_ExGetStatus(wr_ov);
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "Write failed (ex_status = %x)", ex_status));
    if((ex_status == MPIU_EX_STATUS_IO_ABORT) && !(IS_SOCKCONN_CLOSING(sc))){
        /* Thread abort */
        int nb, complete;

        nb = ex_write_progress_update(sc, &complete);
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "THREAD ABORT: Reposting write (already written %d bytes) on sc=%p", nb, sc));

        /* Try reposting write */
        mpi_errno = MPID_nem_newtcp_module_post_writev_ex(sc, NULL, 0);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else{
        mpi_errno = cleanup_sc_vc(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);

        mpi_errno = MPIDI_CH3U_Handle_connection(sc->vc, MPIDI_VC_EVENT_TERMINATED);
        /* FIXME: We should ideally do a shutdown() not close () */
        mpi_errno = MPID_nem_newtcp_module_post_close_ex(sc);

        MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**write",
            "**write %s", MPIU_OSW_Strerror(MPIU_EX_STATUS_TO_ERRNO(MPIU_ExGetStatus(wr_ov))));
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_GEN_WR_FAIL_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#define recv_id_or_tmpvc_info_fail_handler gen_read_fail_handler

#undef FUNCNAME
#define FUNCNAME recv_id_or_tmpvc_info_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int recv_id_or_tmpvc_info_success_handler(MPIU_EXOVERLAPPED *rd_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    int nb = 0, complete=0;
    MPIDI_nem_newtcp_module_header_t *hdr = NULL;
    int hdr_len = sizeof(MPIDI_nem_newtcp_module_header_t);
    MPID_IOV iov[2];
    int pg_id_len = 0, iov_cnt = 1;
    char *pg_id = NULL;

    MPIU_CHKPMEM_DECL (1);
    MPIU_CHKLMEM_DECL (1);
    MPIDI_STATE_DECL(MPID_STATE_RECV_ID_OR_TMPVC_INFO_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_RECV_ID_OR_TMPVC_INFO_SUCCESS_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_RD_OV(rd_ov);
    MPIU_Assert(sc != NULL);

    nb = ex_read_progress_update(sc, &complete);

    /* FIXME: Remove the assumption that we will be able to read
     * the complete id/tmp_vc info in a single read...
     */
    MPIU_Assert(complete);

    if(nb == 0){
        /* Sock conn closed - possibly because of a head-to-head conn resoln */
        mpi_errno = cleanup_sc_vc(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);

        mpi_errno = MPID_nem_newtcp_module_post_close_ex(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        goto fn_exit;
    }

    hdr = (MPIDI_nem_newtcp_module_header_t *) (sc->tmp_buf);
    /* FIXME: Just retry if we don't read the complete header */
    MPIU_ERR_CHKANDJUMP1 (nb != hdr_len, mpi_errno, MPI_ERR_OTHER,
                          "**read", "**read %s", strerror (errno));
    MPIU_Assert(hdr->pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_INFO ||
        hdr->pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_INFO);
    MPIU_Assert(hdr->datalen != 0);
    
    if (hdr->pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_INFO) {
        iov[0].MPID_IOV_BUF = (void *) &(sc->pg_rank);
        iov[0].MPID_IOV_LEN = sizeof(sc->pg_rank);
        pg_id_len = hdr->datalen - sizeof(MPIDI_nem_newtcp_module_idinfo_t);
        if (pg_id_len != 0) {
            /* FIXME: We might want to use sc->tmp_buf - after hdr for
             * reading the pg_id
             */
            MPIU_CHKLMEM_MALLOC (pg_id, char *, pg_id_len, mpi_errno, "sockconn pg_id");
            iov[1].MPID_IOV_BUF = (void *)pg_id;
            iov[1].MPID_IOV_LEN = pg_id_len;
            ++iov_cnt;
        } 
        MPIU_OSW_RETRYON_INTR((nb == -1), 
            (mpi_errno = MPIU_SOCKW_Readv(sc->fd, iov, iov_cnt, &nb)));
        if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

        /* FIXME: Post a read if read fails - instead of aborting */
        /* Also make sure that you have only one read posted at a time */
        MPIU_ERR_CHKANDJUMP1 (nb != hdr->datalen, mpi_errno, MPI_ERR_OTHER,
                  "**read", "**read %s", strerror (errno)); /* FIXME-Z1 */
        if (pg_id_len == 0) {
            sc->is_same_pg = TRUE;
            mpi_errno = MPID_nem_newtcp_module_get_vc_from_conninfo (MPIDI_Process.my_pg->id, 
                             sc->pg_rank, &sc->vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            sc->pg_id = NULL;
        }
		else {
			sc->is_same_pg = FALSE;
			mpi_errno = MPID_nem_newtcp_module_get_vc_from_conninfo (pg_id, sc->pg_rank, &sc->vc);
			if (mpi_errno) MPIU_ERR_POP(mpi_errno);
			sc->pg_id = sc->vc->pg->id;
		}

        MPIU_Assert(sc->vc != NULL);
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "about to incr sc_ref_count sc=%p sc->vc=%p sc_ref_count=%d", sc, sc->vc, VC_FIELD(sc->vc, sc_ref_count)));
        ++VC_FIELD(sc->vc, sc_ref_count);

        /* very important, without this IS_SAME_CONNECTION will always fail */
        sc->pg_is_set = TRUE;
        
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PKT_ID_INFO (pg_rank = %d, pg_id_len = %d): sc->fd=%d, sc->vc=%p, sc=%p", sc->pg_rank, pg_id_len, sc->fd, sc->vc, sc));
    }
    else if (hdr->pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_INFO) {
        MPIDI_VC_t *vc;

        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "PKT_TMPVC_INFO: sc->fd=%d", sc->fd));
        /* create a new VC */
        MPIU_CHKPMEM_MALLOC (vc, MPIDI_VC_t *, sizeof(MPIDI_VC_t), mpi_errno, "real vc from tmp vc");
        /* --BEGIN ERROR HANDLING-- */
        if (vc == NULL) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
            goto fn_fail;
        }
        /* --END ERROR HANDLING-- */
        MPIDI_VC_Init(vc, NULL, 0);     
        VC_FIELD(vc, state) = MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTED;  /* FIXME: is it needed ? */
        sc->vc = vc; 
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "about to incr sc_ref_count sc=%p sc->vc=%p sc_ref_count=%d", sc, sc->vc, VC_FIELD(sc->vc, sc_ref_count)));
        ++VC_FIELD(vc, sc_ref_count);

        ASSIGN_SC_TO_VC(vc, sc);

        /* get the port's tag from the packet and stash it in the VC */
        iov[0].MPID_IOV_BUF = (void *) &(sc->vc->port_name_tag);
        iov[0].MPID_IOV_LEN = sizeof(sc->vc->port_name_tag);

        MPIU_OSW_RETRYON_INTR((nb == -1), 
            (mpi_errno = MPIU_SOCKW_Readv(sc->fd, iov, iov_cnt, &nb)));
        if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

        /* FIXME: Don't assume read will read the whole pg_id/tmpvc */
        MPIU_ERR_CHKANDJUMP1 (nb != hdr->datalen, mpi_errno, MPI_ERR_OTHER,
                              "**read", "**read %s", strerror (errno)); /* FIXME-Z1 */
        sc->is_same_pg = FALSE;
        sc->pg_id = NULL;
        sc->is_tmpvc = TRUE;

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "enqueuing on acceptq vc=%p, sc->fd=%d, tag=%d", vc, sc->fd, sc->vc->port_name_tag));
        MPIDI_CH3I_Acceptq_enqueue(vc, sc->vc->port_name_tag);
    }

	if(!sc->is_tmpvc){
        CHANGE_STATE(sc, CONN_STATE_TA_C_RANKRCVD);
    }
	else{
        CHANGE_STATE(sc, CONN_STATE_TA_C_TMPVCRCVD);
    }

    MPIU_CHKPMEM_COMMIT();

    /* Explicitly call the read handlers */
    mpi_errno = MPID_nem_newtcp_module_reset_readv_ex(sc);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = CALL_RD_HANDLER(sc);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIU_CHKLMEM_FREEALL();

    MPIDI_FUNC_EXIT(MPID_STATE_RECV_ID_OR_TMPVC_INFO_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#define send_cmd_pkt(sc_, pkt_type_) (  \
    send_cmd_pkt_func(sc_, pkt_type_)   \
)

/*
  This function is used to send commands that don't have data but just only
  the header.
 */
#undef FUNCNAME
#define FUNCNAME send_cmd_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_cmd_pkt_func(sockconn_t *sc, MPIDI_nem_newtcp_module_pkt_type_t pkt_type)
{
    int mpi_errno = MPI_SUCCESS, offset;
    MPIDI_nem_newtcp_module_header_t *pkt;
    int pkt_len = sizeof(MPIDI_nem_newtcp_module_header_t);

    MPIU_Assert(sc != NULL);
    MPIU_Assert(pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_ACK ||
                pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_NAK ||
                pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_REQ ||
                pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_ACK ||
                pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_DISC_NAK ||
                pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_ACK ||
                pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_NAK);

    /* FIXME: We are assuming the command packets are only sent during
     * connection phase - enough sock buffer for the calls to succeed
     */
    pkt = (MPIDI_nem_newtcp_module_header_t *) (sc->tmp_buf);
    pkt->pkt_type = pkt_type;
    pkt->datalen = 0;

    MPIU_OSW_RETRYON_INTR((offset == -1),
                            (mpi_errno = MPIU_SOCKW_Write(sc->fd, (char *)pkt, pkt_len, &offset)));
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    
    MPIU_ERR_CHKANDJUMP1 (offset != pkt_len, mpi_errno, MPI_ERR_OTHER,
                    "**write", "**write %s", strerror (errno)); /* FIXME-Z1 */
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
    int mpi_errno = MPI_SUCCESS;
    freenode_t *node;
    MPIU_CHKLMEM_DECL(1);

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONNECT);

    MPIU_Assert(vc != NULL);

    /* We have an active connection, start polling more often */
    MPID_nem_tcp_skip_polls = MAX_SKIP_POLLS_ACTIVE;    

    MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

    if(VC_FIELD(vc, state) == MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED){
        struct sockaddr_in *sock_addr;
        struct in_addr addr;

        MPIU_Assert(VC_FIELD(vc, sc) == NULL);

        mpi_errno = find_free_sc(&sc);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);

        /* FIXME:
           We need to set addr and port using bc.
           If a process is dynamically spawned, vc->pg is NULL.
           In that case, same procedure is done
           in MPID_nem_newtcp_module_connect_to_root()
        */
        if (vc->pg != NULL) { /* VC is not a temporary one */
            char *bc;
            int pmi_errno;
            int val_max_sz;
            
            pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
            MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
            MPIU_CHKLMEM_MALLOC(bc, char *, val_max_sz, mpi_errno, "bc");

            sc->is_tmpvc = FALSE;

            mpi_errno = vc->pg->getConnInfo(vc->pg_rank, bc, val_max_sz, vc->pg);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            mpi_errno = MPID_nem_newtcp_module_get_addr_port_from_bc(bc, &addr, &(VC_FIELD(vc, sock_id).sin_port));
            VC_FIELD(vc, sock_id).sin_addr.s_addr = addr.s_addr;
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        else {
            sc->is_tmpvc = TRUE;
        }

        sock_addr = &(VC_FIELD(vc, sock_id));

        MPIU_OSW_RETRYON_INTR((sc->fd == -1), 
            (mpi_errno = MPIU_SOCKW_Sock_open(AF_INET, SOCK_STREAM, 0, &(sc->fd))));
        if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

        mpi_errno = MPID_nem_newtcp_module_set_sockopts(sc->fd);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);

        MPIU_ExAttachHandle(MPID_nem_newtcp_module_ex_set_hnd, MPIU_EX_WIN32_COMP_PROC_KEY, (HANDLE )(sc->fd));

        CHANGE_STATE(sc, CONN_STATE_TC_C_CNTING);

        mpi_errno = MPID_nem_newtcp_module_post_connect_ex(sc, sock_addr);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
      
        VC_FIELD(vc, state) = MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTED;
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
        }
        else { /* (vc->pg == NULL), dynamic proc connection - temp vc */
            sc->is_same_pg = FALSE;
            sc->pg_id = NULL;
        }
        /* very important, without this IS_SAME_CONNECTION will always fail */
        sc->pg_is_set = TRUE;

        ASSIGN_SC_TO_VC(vc, sc);
        sc->vc = vc;
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "about to incr sc_ref_count sc=%p sc->vc=%p sc_ref_count=%d", sc, sc->vc, VC_FIELD(sc->vc, sc_ref_count)));
        ++VC_FIELD(vc, sc_ref_count);
    }
    else if(VC_FIELD(vc, state) == MPID_NEM_NEWTCP_MODULE_VC_STATE_CONNECTED){
        sc = VC_FIELD(vc, sc);
        MPIU_Assert(sc != NULL);
        /* Do nothing here, the caller just needs to wait for the connection
           state machine to work its way through the states.  Doing something at
           this point will almost always just mess up any head-to-head
           resolution. */
    }
    else{ 
        MPIU_Assert(0);
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    /* MPID_nem_newtcp_module_connpoll(); FIXME-Imp should be called? */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONNECT);
    return mpi_errno;
 fn_fail:
    if (sc != NULL) {
        if (sc->fd != MPIU_SOCKW_SOCKFD_INVALID) {
            MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "MPID_nem_newtcp_module_connect(). closing fd = %d", sc->fd));

            /* FIXME: Post a close and let the quiescent handler take care of adding sc to freeq */
            mpi_errno = MPIU_SOCKW_Sock_close(sc->fd);
        }
        FINALIZE_SC_ENTRY(sc, -1);
        node = MPIU_Malloc(sizeof(freenode_t));      
        MPIU_ERR_CHKANDSTMT(node == NULL, mpi_errno, MPI_ERR_OTHER, goto fn_exit, "**nomem");
        node->sc = sc; 
/*         Note: MPIU_ERR_CHKANDJUMP should not be used here as it will be recursive  */
/*         within fn_fail */ 
        Q_ENQUEUE(&freeq, node);
    }
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* Cleanup vc related stuff in sc 
 * When vc->state becomes CLOSED, MPID_nem_newtcp_module_cleanup() calls this function
 * to cleanup vc related fields in sc
 * We cleanup sc only after the sock close succeeds
 */
static int cleanup_sc_vc(sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;

    if (sc == NULL){
        goto fn_exit;
    }
    if (sc->vc) {
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "about to decr sc_ref_count sc=%p sc->vc=%p sc_ref_count=%d", sc, sc->vc, VC_FIELD(sc->vc, sc_ref_count)));
        MPIU_Assert(VC_FIELD(sc->vc, sc_ref_count) > 0);
        --VC_FIELD(sc->vc, sc_ref_count);
    }
    if (sc->vc && VC_FIELD(sc->vc, sc) == sc) /* this vc may be connecting/accepting with another sc e.g., this sc lost
the tie-breaker */
    {
        VC_FIELD(sc->vc, state) = MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED;
        ASSIGN_SC_TO_VC(sc->vc, NULL);
    }
    sc->vc = NULL;

 fn_exit:
    return mpi_errno;
}

/* Called to transition an sc to CLOSED.  This might be done as part of a ch3
   close protocol or it might be done because the sc is in a quiescent state. */
static int cleanup_sc(sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;
    freenode_t *node;
    MPIU_CHKPMEM_DECL(1);
    if (sc == NULL)
        goto fn_exit;

    CHANGE_STATE(sc, CONN_STATE_TS_CLOSED);

    /* FIXME: How can the sc->fd be invalid here ? */
    if(sc->fd != MPIU_SOCKW_SOCKFD_INVALID){
        /* There is no way to detach a handle from the Executive */
        sc->fd = MPIU_SOCKW_SOCKFD_INVALID;
    }

    FINALIZE_SC_ENTRY(sc, -1);

    MPIU_CHKPMEM_MALLOC (node, freenode_t *, sizeof(freenode_t), mpi_errno, "free node");
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    node->sc = sc;
    Q_ENQUEUE(&freeq, node);

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* this function is called when vc->state becomes CLOSED */
/* FIXME XXX DJG do we need to do anything here to ensure that the final
   close(TRUE) packet has made it into a writev call?  The code might have a
   race for queued messages. */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_cleanup
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_cleanup (struct MPIDI_VC *const vc)
{
    int mpi_errno = MPI_SUCCESS;
    sc_tbl_fw_iterator_t iter;
    sockconn_t *sc = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CLEANUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CLEANUP);

    MPIU_Assert(vc->state == MPIDI_VC_STATE_CLOSED);
    sc = VC_FIELD(vc, sc);

    if (sc != NULL) {
        mpi_errno = cleanup_sc_vc(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);

        mpi_errno = MPID_nem_newtcp_module_post_close_ex(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    sc_tbl_fw_iterator_init(&iter);
    while(sc_tbl_has_next(&iter) && (VC_FIELD(vc, sc_ref_count) > 0)){
        sockconn_t *iter_sc = sc_tbl_get_next(&iter);
        if((iter_sc->fd != sc->fd) && (iter_sc->vc == vc)){
            mpi_errno = cleanup_sc_vc(iter_sc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            CHANGE_STATE(iter_sc, CONN_STATE_TS_D_QUIESCENT);

            mpi_errno = MPID_nem_newtcp_module_post_close_ex(iter_sc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }    
    }
    sc_tbl_fw_iterator_finalize(&iter);

    /* cleanup_sc can technically cause a reconnect on a per-sc basis, but I
       don't think that it can happen when _module_cleanup is called.  Let's
       assert this for now and remove it if we prove that it can happen. */
    /* FIXME: WINTCP ASYNC MPIU_Assert(VC_FIELD(vc, sc_ref_count) == 0); */

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CLEANUP);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME state_tc_c_cnting_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_tc_c_cnting_success_handler(MPIU_EXOVERLAPPED *wr_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc;
    MPIDI_STATE_DECL(MPID_STATE_STATE_TC_C_CNTING_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_TC_C_CNTING_SUCCESS_HANDLER);
   
    sc = GET_SOCKCONN_FROM_EX_WR_OV(wr_ov);
    MPIU_Assert(sc != NULL);

    CHANGE_STATE(sc, CONN_STATE_TC_C_CNTD);

    /* FIXME: We are calling the cntd success handler explicitly here.
     * Get rid of one of these states
     */
	mpi_errno = MPID_nem_newtcp_module_reset_writev_ex(sc);
	if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    CALL_WR_HANDLER(sc);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_TC_C_CNTING_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME gen_cnting_fail_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int gen_cnting_fail_handler(MPIU_EXOVERLAPPED *wr_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc;
    MPIU_Ex_status_t ex_status;
    MPIDI_STATE_DECL(MPID_STATE_GEN_CNTING_FAIL_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_GEN_CNTING_FAIL_HANDLER);
   
    sc = GET_SOCKCONN_FROM_EX_WR_OV(wr_ov);
    MPIU_Assert(sc != NULL);

    ex_status = MPIU_ExGetStatus(wr_ov);
    if((ex_status == MPIU_EX_STATUS_IO_ABORT) && !(IS_SOCKCONN_CLOSING(sc))){
        /* Thread aborted - repost connect */
        mpi_errno = MPID_nem_newtcp_module_post_connect_ex(sc, &(sc->connect.sin));
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else{
        mpi_errno = cleanup_sc_vc(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);

        mpi_errno = MPID_nem_newtcp_module_post_close_ex(sc);

        MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**sock_connect",
            "**sock_connect %s %d", MPIU_OSW_Strerror(MPIU_EX_STATUS_TO_ERRNO(MPIU_ExGetStatus(wr_ov))),
            MPIU_EX_STATUS_TO_ERRNO(MPIU_ExGetStatus(wr_ov)));
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_GEN_CNTING_FAIL_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#define state_tc_c_cnting_fail_handler gen_cnting_fail_handler

#undef FUNCNAME
#define FUNCNAME state_tc_c_cntd_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_tc_c_cntd_success_handler(MPIU_EXOVERLAPPED *wr_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;

    MPIDI_STATE_DECL(MPID_STATE_STATE_TC_C_CNTD_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_TC_C_CNTD_SUCCESS_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_WR_OV(wr_ov);
    MPIU_Assert(sc != NULL);

    if (found_better_sc(sc, NULL)) {
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_tc_c_cntd_handler(): changing to "
              "quiescent"));
        mpi_errno = cleanup_sc_vc(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);

        mpi_errno = MPID_nem_newtcp_module_post_close_ex(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        goto fn_exit;
    }

    if (!sc->is_tmpvc) { /* normal connection */
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
    else { /* temp VC */
        if (send_tmpvc_info(sc) == MPI_SUCCESS) {
            CHANGE_STATE(sc, CONN_STATE_TC_C_TMPVCSENT);
        }
        else {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME,
                                             __LINE__, MPI_ERR_OTHER,
                                             "**fail", 0);
            goto fn_fail;
         }
    } 
	/* Call the handlers explicitly since we don't post a write
	 * for id and tmp_vc
	 * Since we are calling the EX handlers explicitly we need
	 * to reset the write context
	 */
	mpi_errno = MPID_nem_newtcp_module_reset_writev_ex(sc);
	if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = CALL_WR_HANDLER(sc);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_TC_C_CNTD_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    /* Explicitly call the fail handlers */
    mpi_errno = MPID_nem_newtcp_module_reset_writev_ex(sc);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    CALL_WR_FAIL_HANDLER(sc);

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#define state_tc_c_cntd_fail_handler gen_cnting_fail_handler

#undef FUNCNAME
#define FUNCNAME state_c_ack_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_c_ack_success_handler(MPIU_EXOVERLAPPED *rd_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    MPIDI_nem_newtcp_module_header_t *pkt;
    int nb, complete=0;
    MPIDI_STATE_DECL(MPID_STATE_STATE_C_ACKRECV_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_C_ACKRECV_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_RD_OV(rd_ov);
    MPIU_Assert(sc != NULL);

    nb = ex_read_progress_update(sc, &complete);
    /* FIXME: Don't assume that the whole hdr is read */
    MPIU_Assert(complete);
    /* Sanity check that the conn is not closed */
    MPIU_Assert(nb > 0);

    pkt = (MPIDI_nem_newtcp_module_header_t *) (sc->tmp_buf);

    /* We are just expecting ACK/NACK command packets - no data */
    MPIU_Assert(pkt->datalen == 0);
    MPIU_Assert(pkt->pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_ACK ||
                pkt->pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_NAK);

    if (pkt->pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_ID_ACK) {
        CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
        ASSIGN_SC_TO_VC(sc->vc, sc);
        MPID_nem_newtcp_module_conn_est(sc->vc);
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_c_ack_success_handler(): connection established: sc=%p, sc->vc=%p, sc->fd=%d, is_same_pg=%s, pg_rank=%d", sc, sc->vc, sc->fd, (sc->is_same_pg ? "TRUE" : "FALSE"), sc->pg_rank));
    }
    else { 
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_c_ack_success_handler() 2: changing to "
                   "quiescent"));
        mpi_errno = cleanup_sc_vc(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);

        mpi_errno = MPID_nem_newtcp_module_post_close_ex(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_C_ACKRECV_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#define state_c_ack_fail_handler    gen_read_fail_handler

#undef FUNCNAME
#define FUNCNAME state_c_ranksent_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_c_ranksent_success_handler(MPIU_EXOVERLAPPED *wr_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    int nb, complete;
    MPIDI_nem_newtcp_module_header_t *pkt;
    int pkt_len;
    MPIDI_STATE_DECL(MPID_STATE_STATE_C_RANKSENT_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_C_RANKSENT_SUCCESS_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_WR_OV(wr_ov);
    MPIU_Assert(sc != NULL);

    nb = ex_write_progress_update(sc, &complete);
    MPIU_Assert(complete);

    SOCKCONN_EX_RD_HANDLERS_SET(sc, state_c_ack_success_handler, state_c_ack_fail_handler);

    pkt = (MPIDI_nem_newtcp_module_header_t *) (sc->tmp_buf);
    pkt_len = sizeof(MPIDI_nem_newtcp_module_header_t);
    MPIU_Assert(sc->tmp_buf_len >= pkt_len);
    mpi_errno = MPID_nem_newtcp_module_post_read_ex(sc, pkt, pkt_len);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_C_RANKSENT_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#define state_c_ranksent_fail_handler gen_write_fail_handler

#undef FUNCNAME
#define FUNCNAME state_c_tmpvcack_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_c_tmpvcack_success_handler(MPIU_EXOVERLAPPED *rd_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    MPIDI_nem_newtcp_module_header_t *pkt;
    int nb, complete;
    MPIDI_STATE_DECL(MPID_STATE_STATE_C_TMPVCACK_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_C_TMPVCACK_SUCCESS_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_RD_OV(rd_ov);
    MPIU_Assert(sc != NULL);

    nb = ex_read_progress_update(sc, &complete);
    /* FIXME : Remove this restriction */
    MPIU_Assert(complete);
    /* Make sure that conn is not closed */
    MPIU_Assert(nb > 0);

    pkt = (MPIDI_nem_newtcp_module_header_t *) (sc->tmp_buf);
    MPIU_Assert(pkt->datalen == 0);

    MPIU_Assert(pkt->pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_ACK ||
                pkt->pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_NAK);

    if (pkt->pkt_type == MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_ACK) {
        CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
        ASSIGN_SC_TO_VC(sc->vc, sc);
        MPID_nem_newtcp_module_conn_est (sc->vc);
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "c_tmpvcsent_handler(): connection established (fd=%d, sc=%p, sc->vc=%p)", sc->fd, sc, sc->vc));
    }
    else {
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_c_tmpvcsent_handler() 2: changing to quiescent"));
        mpi_errno = cleanup_sc_vc(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);

        mpi_errno = MPID_nem_newtcp_module_post_close_ex(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_ENTER(MPID_STATE_STATE_C_TMPVCACK_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#define state_c_tmpvcack_fail_handler   gen_read_fail_handler

#undef FUNCNAME
#define FUNCNAME state_c_tmpvcsent_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_c_tmpvcsent_success_handler(MPIU_EXOVERLAPPED *wr_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    MPIDI_nem_newtcp_module_header_t *pkt;
    int pkt_len, nb, complete;
    MPIDI_STATE_DECL(MPID_STATE_STATE_C_TMPVCSENT_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_C_TMPVCSENT_SUCCESS_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_WR_OV(wr_ov);
    MPIU_Assert(sc != NULL);

    nb = ex_write_progress_update(sc, &complete);
    MPIU_Assert(complete);

    SOCKCONN_EX_RD_HANDLERS_SET(sc, state_c_tmpvcack_success_handler, state_c_tmpvcack_fail_handler);

    pkt = (MPIDI_nem_newtcp_module_header_t *) (sc->tmp_buf);
    pkt_len = sizeof(MPIDI_nem_newtcp_module_header_t);

    /* Post a read for TMPVC ACK/NACK */
    mpi_errno = MPID_nem_newtcp_module_post_read_ex(sc, pkt, pkt_len);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_C_TMPVCSENT_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#define state_c_tmpvcsent_fail_handler gen_write_fail_handler

#undef FUNCNAME
#define FUNCNAME state_l_cntd_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_l_cntd_success_handler(MPIU_EXOVERLAPPED *rd_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    MPIDI_nem_newtcp_module_header_t *pkt;
    int pkt_len;
    MPIDI_STATE_DECL(MPID_STATE_STATE_L_CNTD_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_L_CNTD_SUCCESS_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_RD_OV(rd_ov);
    MPIU_Assert(sc != NULL);

    /* We have an active connection, start polling more often */
    MPID_nem_tcp_skip_polls = MAX_SKIP_POLLS_ACTIVE;

    pkt = (MPIDI_nem_newtcp_module_header_t *) (sc->tmp_buf);
    pkt_len = sizeof(MPIDI_nem_newtcp_module_header_t);

    /* FIXME: Add more states instead of setting handlers explicitly..
     * OR get rid of all states
     */
    SOCKCONN_EX_RD_HANDLERS_SET(sc, recv_id_or_tmpvc_info_success_handler, recv_id_or_tmpvc_info_fail_handler);
    mpi_errno = MPID_nem_newtcp_module_post_read_ex(sc, pkt, pkt_len);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_L_CNTD_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* FIXME: Is this reqd ? - there is a *accept_fail_handler*/
#undef FUNCNAME
#define FUNCNAME state_l_cntd_fail_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_l_cntd_fail_handler(MPIU_EXOVERLAPPED *rd_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    MPIDI_STATE_DECL(MPID_STATE_STATE_L_CNTD_FAIL_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_L_CNTD_FAIL_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_RD_OV(rd_ov);
    MPIU_Assert(sc != NULL);

	MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_l_cntd_handler() 1: changing to "
		"quiescent"));

    /* FIXME: Implement connect retry ...*/
    /* FIXME : Is this needed ? On accept side the sc won't have a vc right now */
    mpi_errno = cleanup_sc_vc(sc);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

	CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);
    
    MPID_nem_newtcp_module_post_close_ex(sc);
    MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**sock_accept",
        "**sock_accept %s %d", MPIU_OSW_Strerror(MPIU_EX_STATUS_TO_ERRNO(MPIU_ExGetStatus(rd_ov))),
        MPIU_EX_STATUS_TO_ERRNO(MPIU_ExGetStatus(rd_ov)));

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_L_CNTD_FAIL_HANDLER);
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
                      rmt_sc, SOCK_STATE_STR[rmt_sc->state],
                      (win ? "TRUE" : "FALSE"),(rmt_sc->is_same_pg ? "TRUE" : "FALSE"), MPIDI_Process.my_pg_rank,
                      rmt_sc->pg_rank));
    MPIDI_FUNC_EXIT(MPID_STATE_DO_I_WIN);
    return win;
}

#undef FUNCNAME
#define FUNCNAME state_l_rankrcvd_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_l_rankrcvd_success_handler(MPIU_EXOVERLAPPED *rd_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    sockconn_t *fnd_sc;
    int snd_nak = FALSE;
    int nb, complete;
    MPIDI_STATE_DECL(MPID_STATE_STATE_L_RANKRCVD_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_L_RANKRCVD_SUCCESS_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_RD_OV(rd_ov);
    MPIU_Assert(sc != NULL);

    nb = ex_read_progress_update(sc, &complete);
    MPIU_Assert(complete);
    /* FIXME: Re-introduce after we get rid of explicit calls to handlers
      MPIU_Assert(nb > 0);
     */

    /* FIXME: Get rid of found_better_sc() in the C_CNTD state.
     * Always have the connection ACK/NACK by the listen side
     * There are 2 cases here:
     * 1) A connect() is in progress - Send a NACK - Let the other
     * connection succeed
     * 2) A connection already exists - Send a NACK
     */
    if (found_better_sc(sc, &fnd_sc)) {
        if (fnd_sc->state == CONN_STATE_TS_COMMRDY)
            snd_nak = TRUE;
        else if (fnd_sc->state == CONN_STATE_TC_C_RANKSENT)
            snd_nak = do_i_win(sc);
    }

	if (snd_nak) {
		if (send_cmd_pkt(sc, MPIDI_NEM_NEWTCP_MODULE_PKT_ID_NAK) == MPI_SUCCESS) {
			MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "state_l_rankrcvd_handler() 2: changing to "
			  "quiescent"));
            mpi_errno = cleanup_sc_vc(sc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

			CHANGE_STATE(sc,  CONN_STATE_TS_D_QUIESCENT);

            mpi_errno = MPID_nem_newtcp_module_post_close_ex(sc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
		}
	}
	else {
		/* The following line is _crucial_ to correct operation.  We need to
		 * ensure that all head-to-head resolution has completed before we
		 * move to COMMRDY and send any pending messages.  If we don't this
		 * connection could shut down before the other connection has a
		 * chance to finish the connect protocol.  That can lead to all
		 * kinds of badness, including zombie connections, segfaults, and
		 * accessing PG/VC info that is no longer present. */
        if (VC_FIELD(sc->vc, sc_ref_count) > 1){
            /* Post a dummy read so that EX does not lose track of this sock conn */
            mpi_errno = MPID_nem_newtcp_module_post_dummy_read_ex(sc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            goto fn_exit;
        }

		if (send_cmd_pkt(sc, MPIDI_NEM_NEWTCP_MODULE_PKT_ID_ACK) == MPI_SUCCESS) {
			CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
			ASSIGN_SC_TO_VC(sc->vc, sc);
			MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "connection established: sc=%p, sc->vc=%p, sc->fd=%d, is_same_pg=%s, pg_rank=%d", sc, sc->vc, sc->fd, (sc->is_same_pg ? "TRUE" : "FALSE"), sc->pg_rank));
			MPID_nem_newtcp_module_conn_est(sc->vc);
		}
	}

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_L_RANKRCVD_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#define state_l_rankrcvd_fail_handler   gen_read_fail_handler

#undef FUNCNAME
#define FUNCNAME state_l_tmpvcrcvd_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_l_tmpvcrcvd_success_handler(MPIU_EXOVERLAPPED *rd_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    int snd_nak = FALSE;
    int nb, complete;
    MPIDI_STATE_DECL(MPID_STATE_STATE_L_TMPVCRCVD_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_L_TMPVCRCVD_SUCCESS_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_RD_OV(rd_ov);
    MPIU_Assert(sc != NULL);

    nb = ex_read_progress_update(sc, &complete);
    MPIU_Assert(complete);
    MPIU_Assert(nb > 0);

    /* FIXME: When is snd_nak true ? */
	if (snd_nak) {
		if (send_cmd_pkt(sc, MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_NAK) == MPI_SUCCESS) {
            mpi_errno = cleanup_sc_vc(sc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

			CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);

            mpi_errno = MPID_nem_newtcp_module_post_dummy_read_ex(sc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
		}
	}
	else {
		if (send_cmd_pkt(sc, MPIDI_NEM_NEWTCP_MODULE_PKT_TMPVC_ACK) == MPI_SUCCESS) {
			CHANGE_STATE(sc, CONN_STATE_TS_COMMRDY);
			ASSIGN_SC_TO_VC(sc->vc, sc);
			MPID_nem_newtcp_module_conn_est (sc->vc);
			MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "fd=%d: TMPVC_ACK sent, connection established!", sc->fd));
        }
	}

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_L_TMPVCRCVD_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#define state_l_tmpvcrcvd_fail_handler  gen_read_fail_handler

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_recv_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_newtcp_module_recv_success_handler(MPIU_EXOVERLAPPED *rd_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    int complete;
    ssize_t bytes_recvd;
    MPID_Request *rreq = NULL;
    MPID_IOV *iov = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_RECV_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_RECV_SUCCESS_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_RD_OV(rd_ov);
    MPIU_Assert(sc != NULL);

    bytes_recvd = ex_read_progress_update(sc, &complete);
    rreq = ((MPIDI_CH3I_VC *)sc->vc->channel_private)->recv_active;

    if (rreq == NULL){
        /* Received a new message on scratch pad, sc->tmp_buf */
        if (bytes_recvd == 0){
            MPIU_Assert(sc != NULL);
            MPIU_Assert(sc->vc != NULL);
            /* sc->vc->sc will be NULL if sc->vc->state == _INACTIVE */
            MPIU_Assert(VC_FIELD(sc->vc, sc) == NULL || VC_FIELD(sc->vc, sc) == sc);
            if (vc_is_in_shutdown(sc->vc)){
                /* there's currently no hook for CH3 to tell nemesis/newtcp
                   that we are in the middle of a disconnection dance.  So
                   if we don't check to see if we are currently
                   disconnecting, then we end up with a potential race where
                   the other side performs a tcp close() before we do and we
                   blow up here. */
                mpi_errno = cleanup_sc_vc(sc);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                CHANGE_STATE(sc, CONN_STATE_TS_D_QUIESCENT);

                mpi_errno = MPID_nem_newtcp_module_post_close_ex(sc);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                goto fn_exit;
            }
            else{
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "ERROR: sock (fd=%d) is closed: bytes_recvd == 0", sc->fd );
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sock_closed");
            }
        }
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "New recv %d (fd=%d, vc=%p, sc=%p)", bytes_recvd, sc->fd, sc->vc, sc));

        mpi_errno = MPID_nem_handle_pkt(sc->vc, sc->tmp_buf, bytes_recvd);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* The pkt handlers can sever the conn btw sc and vc - eg: when vc is terminated*/
        if(sc->vc != NULL){
            rreq = ((MPIDI_CH3I_VC *)sc->vc->channel_private)->recv_active;
            if (rreq == NULL){
                /* The packets were completely consumed - post read on scratch pad recv buf */
                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Packets (bytes = %d) on (fd=%d, vc=%p, sc=%p) completely consumed", bytes_recvd, sc->fd, sc->vc, sc));
                mpi_errno = MPID_nem_newtcp_module_post_read_ex(sc, sc->tmp_buf, sc->tmp_buf_len);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
            else{
                /* The packet was not completely consumed - post read for remaining data */
                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Packets (bytes = %d) on (fd=%d, vc=%p, sc=%p) NOT completely consumed", bytes_recvd, sc->fd, sc->vc, sc));
                iov = &rreq->dev.iov[rreq->dev.iov_offset];
                mpi_errno = MPID_nem_newtcp_module_post_readv_ex(sc, iov, rreq->dev.iov_count);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
        }
    }
    else{
        /* Continuing recv of an old message */
        /* There is a pending receive, and we are receiving
         * data directly into the user buffer
         */
        int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

        MPIU_Assert(rreq->dev.iov_count > 0);
        MPIU_Assert(rreq->dev.iov_offset >= 0);
        MPIU_Assert(rreq->dev.iov_offset < MPID_IOV_LIMIT);
        MPIU_Assert(rreq->dev.iov_count <= MPID_IOV_LIMIT);

        iov = &rreq->dev.iov[rreq->dev.iov_offset];
        if (bytes_recvd == 0){
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sock_closed");
        }
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "Cont recv, total bytes = %d", bytes_recvd);

        if(!complete){
            /* IOV is not filled completely. Continue read on the prev posted IOV. */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Cont recv, IOV not filled completely - cont recv on same iov");
            mpi_errno = MPID_nem_newtcp_module_post_readv_ex(sc, NULL, 0);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
        else{
            /* the whole iov has been received */
            int req_complete=0;
            
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Cont recv, IOV filled completely...");
            reqFn = rreq->dev.OnDataAvail;
            if (!reqFn){
                MPIU_Assert(MPIDI_Request_get_type(rreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                MPIDI_CH3U_Request_complete(rreq);
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Req complete...");
                ((MPIDI_CH3I_VC *)sc->vc->channel_private)->recv_active = NULL;
                req_complete = 1;
            }
            else{
                req_complete = 0;

                mpi_errno = reqFn(sc->vc, rreq, &req_complete);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                if (req_complete){
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Req complete...");
                    ((MPIDI_CH3I_VC *)sc->vc->channel_private)->recv_active = NULL;
                }
                else{
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Req not complete...");
                }
            }
            if(req_complete){
                mpi_errno = MPID_nem_newtcp_module_post_read_ex(sc, sc->tmp_buf, sc->tmp_buf_len);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
            else{
                iov = &rreq->dev.iov[rreq->dev.iov_offset];

                mpi_errno = MPID_nem_newtcp_module_post_readv_ex(sc, iov, rreq->dev.iov_count);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
        } /* the whole iov has been received */
    } /* processing recv on an old message */

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_RECV_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#define MPID_nem_newtcp_module_recv_fail_handler gen_read_fail_handler

#define state_commrdy_rd_success_handler MPID_nem_newtcp_module_recv_success_handler

#undef FUNCNAME
#define FUNCNAME state_commrdy_wr_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_commrdy_wr_success_handler(MPIU_EXOVERLAPPED *wr_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    int nb, complete = 0;

    MPIDI_STATE_DECL(MPID_STATE_STATE_COMMRDY_WR_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_COMMRDY_WR_SUCCESS_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_WR_OV(wr_ov);
    MPIU_Assert(sc != NULL);

    nb = ex_write_progress_update(sc, &complete);

    if(complete){
        /* Call req handler */
        mpi_errno = MPID_nem_newtcp_module_handle_sendq_head_req(sc->vc, &complete);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        /* Send queued data, if any */
	    mpi_errno = MPID_nem_newtcp_module_send_queued(sc->vc);
	    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP (mpi_errno);
    }
    else{
        /* Continue writing from the posted iov */
        mpi_errno = MPID_nem_newtcp_module_post_writev_ex(sc, NULL, 0);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_COMMRDY_WR_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#define state_commrdy_rd_fail_handler gen_read_fail_handler
#define state_commrdy_wr_fail_handler gen_write_fail_handler

#undef FUNCNAME
#define FUNCNAME state_d_quiescent_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_d_quiescent_handler(MPIU_EXOVERLAPPED *wr_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *sc = NULL;
    MPIDI_STATE_DECL(MPID_STATE_STATE_D_QUIESCENT_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_D_QUIESCENT_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_WR_OV(wr_ov);
    MPIU_Assert(sc != NULL);

    mpi_errno = cleanup_sc_vc(sc);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = cleanup_sc(sc);
    if(mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_D_QUIESCENT_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#define state_d_quiescent_success_handler state_d_quiescent_handler
#define state_d_quiescent_fail_handler state_d_quiescent_handler

static inline int sc_state_init_handler(sc_state_info_t *sc_state_info,
    handler_func_t rd_success_fn, handler_func_t rd_fail_fn,
    handler_func_t wr_success_fn, handler_func_t wr_fail_fn)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(sc_state_info != NULL);
    sc_state_info->sc_state_rd_success_handler = rd_success_fn;
    sc_state_info->sc_state_rd_fail_handler = rd_fail_fn;
    sc_state_info->sc_state_wr_success_handler = wr_success_fn;
    sc_state_info->sc_state_wr_fail_handler = wr_fail_fn;

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_sm_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_sm_init()
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t  *sc = NULL;

#ifdef MPICH_IS_THREADED
    MPIU_THREAD_CHECK_BEGIN;
    /* FIXME: Create post thread here */
    MPIU_THREAD_CHECK_END;
#endif
    /* Set the appropriate handlers */
    sc_state_init_handler(&sc_state_info[CONN_STATE_TS_CLOSED],
                            state_d_quiescent_success_handler,
                            state_d_quiescent_fail_handler,
                            state_d_quiescent_success_handler,
                            state_d_quiescent_fail_handler);

    sc_state_init_handler(&sc_state_info[CONN_STATE_TC_C_CNTING],
                            NULL, NULL,
                            state_tc_c_cnting_success_handler,
                            state_tc_c_cnting_fail_handler);
    sc_state_init_handler(&sc_state_info[CONN_STATE_TC_C_CNTD],
                            NULL, NULL,
                            state_tc_c_cntd_success_handler,
                            state_tc_c_cntd_fail_handler);
    sc_state_init_handler(&sc_state_info[CONN_STATE_TC_C_RANKSENT],
                            NULL, NULL,
                            state_c_ranksent_success_handler,
                            state_c_ranksent_fail_handler);
    sc_state_init_handler(&sc_state_info[CONN_STATE_TC_C_TMPVCSENT],
                            NULL, NULL,
                            state_c_tmpvcsent_success_handler,
                            state_c_tmpvcsent_fail_handler);
    sc_state_init_handler(&sc_state_info[CONN_STATE_TA_C_CNTD],
                            state_l_cntd_success_handler,
                            state_l_cntd_fail_handler,
                            NULL, NULL);
    sc_state_init_handler(&sc_state_info[CONN_STATE_TA_C_RANKRCVD],
                            state_l_rankrcvd_success_handler,
                            state_l_rankrcvd_fail_handler,
                            NULL, NULL);
    sc_state_init_handler(&sc_state_info[CONN_STATE_TA_C_TMPVCRCVD],
                            state_l_tmpvcrcvd_success_handler,
                            state_l_tmpvcrcvd_fail_handler,
                            NULL, NULL);
    sc_state_init_handler(&sc_state_info[CONN_STATE_TS_COMMRDY],
                            state_commrdy_rd_success_handler,
                            state_commrdy_rd_fail_handler,
                            state_commrdy_wr_success_handler,
                            state_commrdy_wr_fail_handler);
    sc_state_init_handler(&sc_state_info[CONN_STATE_TS_D_QUIESCENT],
                            NULL, NULL,
                            state_d_quiescent_success_handler,
                            state_d_quiescent_fail_handler);

    /* Init the sc tbls list */
    mpi_errno = init_sc_tbl_list();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Create & init the sock conn table */
    mpi_errno = alloc_sc_tbl();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Post the first accept on the listening sock */
    sc = &MPID_nem_newtcp_module_g_lstn_sc; 
    SOCKCONN_EX_RD_HANDLERS_SET(sc, MPID_nem_newtcp_module_state_accept_success_handler, MPID_nem_newtcp_module_state_accept_fail_handler);
    SOCKCONN_EX_WR_HANDLERS_SET(sc, NULL, NULL);

    mpi_errno = MPID_nem_newtcp_module_post_accept_ex(sc);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_sm_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_sm_finalize()
{
    freenode_t *node;
    int mpi_errno = MPI_SUCCESS;

#ifdef MPICH_IS_THREADED
    MPIU_THREAD_CHECK_BEGIN;
    /* FIXME: Kill post thread here */
    MPIU_THREAD_CHECK_END;
#endif

    /* walk the freeq and free all the elements */
    while (!Q_EMPTY(freeq)) {
        Q_DEQUEUE(&freeq, ((freenode_t **)&node));
        MPIU_Free(node);
    }

    mpi_errno = free_sc_tbls_in_list();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = free_sc_tbl_list();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

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
#define SOCK_PROGRESS_EVENTS_MAX 100
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_connpoll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_connpoll(int in_blocking_poll)
{
    int mpi_errno = MPI_SUCCESS;
    static int num_skipped_polls = 0;
    int nevents = 0;
    /* BOOL wait_for_event_and_status = (in_blocking_poll) ? TRUE : FALSE; */
    BOOL wait_for_event_and_status = FALSE;

    /* To improve shared memory performance, we don't call the poll()
     * systemcall every time. The MPID_nem_tcp_skip_polls value is
     * changed depending on whether we have any active connections. */
    if (in_blocking_poll && num_skipped_polls++ < MPID_nem_tcp_skip_polls){
        goto fn_exit;
    }
    SKIP_POLLS_INC(MPID_nem_tcp_skip_polls);
    num_skipped_polls = 0;

    mpi_errno = MPIU_ExProcessCompletions(MPID_nem_newtcp_module_ex_set_hnd, &wait_for_event_and_status);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* FIXME: We also need to tune the number of times we need to
     * wait for an event. 
     * eg: SOCK_PROGRESS_EVENTS_MAX = 2 x No_of_fds_in_ex_set
     */
    while((wait_for_event_and_status == TRUE) &&
            (++nevents < SOCK_PROGRESS_EVENTS_MAX)){
        /* Don't block for an event to complete */
        wait_for_event_and_status = FALSE;
        /* On return, if (wait_for_event_and_status == FALSE) then
         * there are no more events in EX queue at the moment
         */
        mpi_errno = MPIU_ExProcessCompletions(MPID_nem_newtcp_module_ex_set_hnd, &wait_for_event_and_status);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

static int complete_connection(sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(sc != NULL);

    MPID_nem_newtcp_module_set_sockopts(sc->fd); /* (N2) */

	sc->pg_rank = CONN_INVALID_RANK;
	sc->pg_is_set = FALSE;
	sc->is_tmpvc = 0;

	MPIU_ExAttachHandle(MPID_nem_newtcp_module_ex_set_hnd, MPIU_EX_WIN32_COMP_PROC_KEY, (HANDLE )sc->fd);

	CHANGE_STATE(sc, CONN_STATE_TA_C_CNTD);

    /* FIXME: Try to avoid calling the rd handler explicitly */
    /* Explicitly call the read handlers */
    mpi_errno = MPID_nem_newtcp_module_reset_readv_ex(sc);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    CALL_RD_HANDLER(sc);

    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "accept success, added to table, connfd=%d", sc->accept.connfd));        

 fn_exit:
    return mpi_errno;
 fn_fail:
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

  N3:  find_free_sc is called within the while loop. It may cause the table to expand. So, 
  the arguments passed for this callback function may get invalidated. So, it is imperative
  that we obtain sc pointer and plfd pointer everytime within the while loop.
  Accordingly, the parameters are named unused1 and unused2 for clarity.
*/
#undef FUNCNAME
#define FUNCNAME state_listening_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_state_accept_success_handler(MPIU_EXOVERLAPPED *rd_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t *l_sc = NULL;
    int len;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_STATE_LISTENING_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_STATE_LISTENING_SUCCESS_HANDLER);

    /* Accept succeeded... */
    l_sc = GET_SOCKCONN_FROM_EX_RD_OV(rd_ov);
    MPIU_Assert(l_sc != NULL);

    /* Process all connections waiting to be accepted & post the next accept */
    while (1) {
        if(l_sc->accept.connfd == MPIU_SOCKW_SOCKFD_INVALID){
            /* Post next accept and quit */
            mpi_errno = MPID_nem_newtcp_module_post_accept_ex(l_sc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            break;
        }
        else {
            sockconn_t *sc;
            mpi_errno = find_free_sc(&sc);
            if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP (mpi_errno); }

            sc->fd = l_sc->accept.connfd;

            mpi_errno = complete_connection(sc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
        len = sizeof(SA_IN);
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "before accept"));
        MPIU_OSW_RETRYON_INTR((mpi_errno != MPI_SUCCESS),
            (mpi_errno = MPIU_SOCKW_Accept(l_sc->fd, (SA *) &l_sc->accept.accept_buffer, &len, &(l_sc->accept.connfd))));
        if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_STATE_LISTENING_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_state_accept_fail_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_state_accept_fail_handler(MPIU_EXOVERLAPPED *rd_ov)
{
    int mpi_errno = MPI_SUCCESS;
    sockconn_t  *sc = NULL;
    MPIU_Ex_status_t ex_status;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_STATE_LISTENING_FAIL_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_STATE_LISTENING_FAIL_HANDLER);

    sc = GET_SOCKCONN_FROM_EX_RD_OV(rd_ov);

    MPIU_Assert(sc != NULL);

    ex_status = MPIU_ExGetStatus(rd_ov);
    if((ex_status == MPIU_EX_STATUS_IO_ABORT) && !(IS_SOCKCONN_CLOSING(sc))){
        /* Thread aborted - try to repost accept */
        MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "THREAD ABORT: Reposting accept on sc=%p", sc));
        mpi_errno = MPID_nem_newtcp_module_post_accept_ex(sc);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else{
        /* FIXME: Get rid of static allocn of listen sock and post a close */
        mpi_errno = MPIU_SOCKW_Sock_close(sc->fd);
        MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**sock_accept",
            "**sock_accept %s %d", MPIU_OSW_Strerror(MPIU_EX_STATUS_TO_ERRNO(MPIU_ExGetStatus(rd_ov))),
            MPIU_EX_STATUS_TO_ERRNO(MPIU_ExGetStatus(rd_ov)));
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_STATE_LISTENING_FAIL_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
