/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "wintcp_impl.h"
extern int h_errno;

static int create_s_cookie (int data_sz, char **cookie, int *len);
static int read_s_cookie (MPID_IOV cookie, int *data_sz);
static int create_r_cookie (char *hostname, int port, int data_sz, char **cookie, int *len);
static int read_r_cookie (MPID_IOV cookie, char **hostname, int *port, int *data_sz);
static void free_cookie (void *c);
static int set_sockopts (int fd);

//#define TESTING_CHUNKING
#ifdef TESTING_CHUNKING
#define CHUNK 6299651//(32*1024)
#endif

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_lmt_pre_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_module_lmt_pre_send (MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV *cookie)
{
    int mpi_errno = MPI_SUCCESS;
    int len;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_PRE_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_PRE_SEND);

    MPIDI_Datatype_get_info (req->dev.user_count, req->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    mpi_errno = create_s_cookie (data_sz, &vc_ch->net.tcp.lmt_cookie, &len);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    cookie->MPID_IOV_BUF = vc_ch->net.tcp.lmt_cookie;
    cookie->MPID_IOV_LEN = len;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_PRE_SEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_lmt_pre_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_module_lmt_pre_recv (MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV s_cookie, MPID_IOV *r_cookie, int *send_cts)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    unsigned int len;
    struct sockaddr_in saddr;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_PRE_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_PRE_RECV);

    mpi_errno = read_s_cookie (s_cookie, &vc_ch->net.tcp.lmt_s_len);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    memset (&saddr, sizeof(saddr), 0);

    if (!vc_ch->net.tcp.lmt_connected)
    {
        vc_ch->net.tcp.lmt_desc = socket (AF_INET, SOCK_STREAM, 0);
        MPIU_ERR_CHKANDJUMP2 (vc_ch->net.tcp.lmt_desc == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create", "**sock_create %s %d", strerror (errno), errno);

        //        ret = fcntl (vc_ch->net.tcp.lmt_desc, F_SETFL, O_NONBLOCK);
        //        MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);

        saddr.sin_family      = AF_INET;
        saddr.sin_addr.s_addr = htonl (INADDR_ANY);
        saddr.sin_port        = htons (0);

        ret = bind (vc_ch->net.tcp.lmt_desc, (struct sockaddr *)&saddr, sizeof (saddr));
        MPIU_ERR_CHKANDJUMP3 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", ntohs (saddr.sin_port), errno, strerror (errno));

        len = sizeof (saddr);
        ret = getsockname (vc_ch->net.tcp.lmt_desc, (struct sockaddr *)&saddr, &len);
        MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);

        set_sockopts (vc_ch->net.tcp.lmt_desc);

        ret = listen (vc_ch->net.tcp.lmt_desc, SOMAXCONN);
        MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**listen", "**listen %s %d", errno, strerror (errno));
    }

    MPIDI_Datatype_get_info (req->dev.user_count, req->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    mpi_errno = create_r_cookie (MPID_nem_hostname, ntohs (saddr.sin_port), data_sz, &vc_ch->net.tcp.lmt_cookie, &len);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    r_cookie->MPID_IOV_BUF = vc_ch->net.tcp.lmt_cookie;
    r_cookie->MPID_IOV_LEN = len;

    *send_cts = 1;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_PRE_RECV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_lmt_start_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_module_lmt_start_send (MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV r_cookie)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPIDI_msg_sz_t last;
    int nb;
    int s_len = 0;
    int r_len;
    int r_port;
    char *r_hostname;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_START_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_START_SEND);

    mpi_errno = read_r_cookie (r_cookie, &r_hostname, &r_port, &r_len);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    free_cookie (vc_ch->net.tcp.lmt_cookie);

    if (!vc_ch->net.tcp.lmt_connected)
    {
        struct sockaddr_in saddr;
        struct hostent *hp;

        vc_ch->net.tcp.lmt_desc = socket (AF_INET, SOCK_STREAM, 0);
        MPIU_ERR_CHKANDJUMP2 (vc_ch->net.tcp.lmt_desc == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create", "**sock_create %s %d", strerror (errno), errno);

        //        ret = fcntl (vc_ch->net.tcp.lmt_desc, F_SETFL, O_NONBLOCK);
        //        MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);

        hp = gethostbyname (r_hostname);
        MPIU_ERR_CHKANDJUMP2 (hp == NULL, mpi_errno, MPI_ERR_OTHER, "**gethostbyname", "**gethostbyname %s %d", hstrerror (h_errno), h_errno);

        memset (&saddr, sizeof(saddr), 0);
        saddr.sin_family = AF_INET;
        saddr.sin_port   = htons (r_port);
        MPIU_Memcpy (&saddr.sin_addr, hp->h_addr, hp->h_length);

        set_sockopts (vc_ch->net.tcp.lmt_desc);

        ret = connect (vc_ch->net.tcp.lmt_desc, (struct sockaddr *)&saddr, sizeof(saddr));
        MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);

        vc_ch->net.tcp.lmt_connected = 1;
    }

    MPIDI_Datatype_get_info (req->dev.user_count, req->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (r_len < data_sz)
    {
        /* message will be truncated */
        s_len = data_sz;
        data_sz = r_len;
 	req->status.MPI_ERROR = MPIU_ERR_SET2 (mpi_errno, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", s_len, r_len);
    }

    MPID_Segment_init (req->dev.user_buf, req->dev.user_count, req->dev.datatype, &req->dev.segment, 0);
    req->dev.segment_first = 0;
    req->dev.segment_size = data_sz;
    req->dev.iov_count = MPID_IOV_LIMIT;
    req->dev.iov_offset = 0;
    last = data_sz;

    do
    {
        int iov_offset;
        int left_to_send;
        MPID_Segment_pack_vector (&req->dev.segment, req->dev.segment_first, &last, req->dev.iov, &req->dev.iov_count);

        left_to_send = last - req->dev.segment_first;
        iov_offset = 0;

#ifdef TESTING_CHUNKING
        {
            char *buf = req->dev.iov[0].MPID_IOV_BUF;
            int l;
            while (left_to_send)
            {
                if (left_to_send > CHUNK)
                    l = CHUNK;
                else
                    l = left_to_send;

                do
                    nb = write (vc_ch->net.tcp.lmt_desc, buf, l);
                while (nb == -1 && errno == EINTR);
                MPIU_ERR_CHKANDJUMP (nb == -1, mpi_errno, MPI_ERR_OTHER, "**sock_writev");

                left_to_send -= nb;
                buf += nb;
            }

            MPIDI_CH3U_Request_complete (req);
            goto fn_exit;
        }
#endif

        do
            nb = writev (vc_ch->net.tcp.lmt_desc, &req->dev.iov[iov_offset], req->dev.iov_count - iov_offset);
        while (nb == -1 && errno == EINTR);
        MPIU_ERR_CHKANDJUMP (nb == -1, mpi_errno, MPI_ERR_OTHER, "**sock_writev");

        left_to_send -= nb;
        while (left_to_send)
        { /* send rest of iov */
            while (nb >= req->dev.iov[iov_offset].MPID_IOV_LEN)
            { /* update iov to reflect sent bytes */
                nb -= req->dev.iov[iov_offset].MPID_IOV_LEN;
                ++iov_offset;
            }
            req->dev.iov[iov_offset].MPID_IOV_BUF = (char *)req->dev.iov[iov_offset].MPID_IOV_BUF + nb;
            req->dev.iov[iov_offset].MPID_IOV_LEN -= nb;

            do
                nb = writev (vc_ch->net.tcp.lmt_desc, &req->dev.iov[iov_offset], req->dev.iov_count - iov_offset);
            while (nb == -1 && errno == EINTR);
            MPIU_ERR_CHKANDJUMP (nb == -1, mpi_errno, MPI_ERR_OTHER, "**sock_writev");
            left_to_send -= nb;
        }
    }
    while (last < data_sz);

    MPIDI_CH3U_Request_complete (req);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_START_SEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_lmt_start_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_module_lmt_start_recv (MPIDI_VC_t *vc, MPID_Request *req)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPIDI_msg_sz_t last;
    int nb;
    int r_len;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_START_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_START_RECV);

    free_cookie (vc_ch->net.tcp.lmt_cookie);

    if (!vc_ch->net.tcp.lmt_connected)
    {
        int len;
        struct sockaddr_in saddr;
        int connfd;

        len = sizeof (saddr);
        connfd = accept (vc_ch->net.tcp.lmt_desc, (struct sockaddr *)&saddr, &len);
        MPIU_ERR_CHKANDJUMP2 (connfd == -1, mpi_errno, MPI_ERR_OTHER, "**sock|poll|accept", "**sock|poll|accept %d %s", errno, strerror (errno));

        /* close listen fd */
        do
            ret = close (vc_ch->net.tcp.lmt_desc);
        while (ret == -1 && errno == EINTR);
        MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**closesocket", "**closesocket %s %d", strerror (errno), errno);

        /* set lmt_desc to new connected fd */
        vc_ch->net.tcp.lmt_desc = connfd;
        vc_ch->net.tcp.lmt_connected = 1;

        //        ret = fcntl (vc_ch->net.tcp.lmt_desc, F_SETFL, O_NONBLOCK);
        //        MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);
    }

    MPIDI_Datatype_get_info (req->dev.user_count, req->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (data_sz > vc_ch->net.tcp.lmt_s_len)
    {
        data_sz = vc_ch->net.tcp.lmt_s_len;
    }
    else if (data_sz < vc_ch->net.tcp.lmt_s_len)
    {
        /* message will be truncated */
        r_len = data_sz;
 	req->status.MPI_ERROR = MPIU_ERR_SET2 (mpi_errno, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", vc_ch->net.tcp.lmt_s_len, r_len);
    }

    MPID_Segment_init (req->dev.user_buf, req->dev.user_count, req->dev.datatype, &req->dev.segment, 0);
    req->dev.segment_first = 0;
    req->dev.segment_size = data_sz;
    req->dev.iov_count = MPID_IOV_LIMIT;
    req->dev.iov_offset = 0;
    last = data_sz;

    do
    {
        int iov_offset;
        int left_to_recv;

        MPID_Segment_unpack_vector (&req->dev.segment, req->dev.segment_first, &last, req->dev.iov, &req->dev.iov_count);

        left_to_recv = last - req->dev.segment_first;
        iov_offset = 0;

#ifdef TESTING_CHUNKING
        {
            char *buf = req->dev.iov[0].MPID_IOV_BUF;
            int l;
            while (left_to_recv)
            {
                if (left_to_recv > CHUNK)
                    l = CHUNK;
                else
                    l = left_to_recv;

                do
                    nb = read (vc_ch->net.tcp.lmt_desc, buf, l);
                while (nb == -1 && errno == EINTR);
                MPIU_ERR_CHKANDJUMP (nb == -1, mpi_errno, MPI_ERR_OTHER, "**sock_writev");

                left_to_recv -= nb;
                buf += nb;
            }
            MPIDI_CH3U_Request_complete (req);
            goto fn_exit;
        }
#endif

        do
            nb = readv (vc_ch->net.tcp.lmt_desc, &req->dev.iov[iov_offset], req->dev.iov_count - iov_offset);
        while (nb == -1 && errno == EINTR);
        MPIU_ERR_CHKANDJUMP2 (nb == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);
        MPIU_ERR_CHKANDJUMP (nb == 0, mpi_errno, MPI_ERR_OTHER, "**fail");

        left_to_recv -= nb;
        while (left_to_recv)
        { /* recv rest of iov */
            while (nb >= req->dev.iov[iov_offset].MPID_IOV_LEN)
            { /* update iov to reflect sent bytes */
                nb -= req->dev.iov[iov_offset].MPID_IOV_LEN;
                ++iov_offset;
            }
            req->dev.iov[iov_offset].MPID_IOV_BUF = (char *)req->dev.iov[iov_offset].MPID_IOV_BUF + nb;
            req->dev.iov[iov_offset].MPID_IOV_LEN -= nb;

            do
                nb = readv (vc_ch->net.tcp.lmt_desc, &req->dev.iov[iov_offset], req->dev.iov_count - iov_offset);
            while (nb == -1 && errno == EINTR);
            MPIU_ERR_CHKANDJUMP2 (nb == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);
            MPIU_ERR_CHKANDJUMP (nb == 0, mpi_errno, MPI_ERR_OTHER, "**fail");
            left_to_recv -= nb;
        }
    }
    while (last < data_sz);

    MPIDI_CH3U_Request_complete (req);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_START_RECV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_lmt_post_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_module_lmt_post_send (MPIDI_VC_t *vc, MPID_Request *req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_POST_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_POST_SEND);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_POST_SEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_lmt_post_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_module_lmt_post_recv (MPIDI_VC_t *vc, MPID_Request *req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_POST_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_POST_RECV);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_MODULE_LMT_POST_RECV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME create_s_cookie
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int create_s_cookie (int data_sz, char **cookie, int *len)
{
    int mpi_errno = MPI_SUCCESS;
    int *int_cookie;

    int_cookie = MPIU_Malloc (sizeof (data_sz));
    MPIU_ERR_CHKANDJUMP (int_cookie == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
    *int_cookie = data_sz;

    *cookie = (char *)int_cookie;
    *len = sizeof (data_sz);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME read_s_cookie
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int read_s_cookie (MPID_IOV cookie, int *data_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_ERR_CHKANDJUMP (cookie.MPID_IOV_LEN != sizeof (data_sz), mpi_errno, MPI_ERR_OTHER, "**fail");

    *data_sz = *(int *)cookie.MPID_IOV_BUF;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

typedef struct r_cookie
{
    int port;
    int data_sz;
    char hostname[1];
} r_cookie_t;

#undef FUNCNAME
#define FUNCNAME create_r_cookie
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int create_r_cookie (char *hostname, int port, int data_sz, char **cookie, int *len)
{
    int mpi_errno = MPI_SUCCESS;
    int hostname_len;
    int cookie_len;
    r_cookie_t *c;

    hostname_len = strnlen (hostname, MAX_HOSTNAME_LEN) + 1;

    cookie_len = sizeof (r_cookie_t) - 1 + hostname_len;

    c = MPIU_Malloc (cookie_len);
    MPIU_ERR_CHKANDJUMP (c == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");

    c->port = port;
    c->data_sz = data_sz;
    MPIU_Strncpy (c->hostname, hostname, hostname_len);

    *cookie = (char *)c;
    *len = sizeof (r_cookie_t) - 1 + hostname_len;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* read_r_cookie - extracts hostname, port and data size from a recv cookie
   data pointed to by hostname is valid only as long as the packet containing the cookie is valid
 */
#undef FUNCNAME
#define FUNCNAME read_r_cookie
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int read_r_cookie (MPID_IOV cookie, char **hostname, int *port, int *data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    r_cookie_t *c;

    MPIU_ERR_CHKANDJUMP (cookie.MPID_IOV_LEN < sizeof (r_cookie_t), mpi_errno, MPI_ERR_OTHER, "**fail");

    c = (r_cookie_t *)cookie.MPID_IOV_BUF;

    *hostname = c->hostname;
    *port = c->port;
    *data_sz = c->data_sz;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

static void free_cookie (void *c)
{
    MPIU_Free (c);
}

#undef FUNCNAME
#define FUNCNAME set_sockopts
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int set_sockopts (int fd)
{
    int mpi_errno = MPI_SUCCESS;
    int option;
    int ret;

    option = 0;
    ret = setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(int));
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);

    option = 128*1024;
    setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &option, sizeof(int));
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);
    setsockopt (fd, SOL_SOCKET, SO_SNDBUF, &option, sizeof(int));
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s %d", strerror (errno), errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
