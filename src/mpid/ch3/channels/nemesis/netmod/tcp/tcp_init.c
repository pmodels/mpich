/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "tcp_impl.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH3_INTERFACE_HOSTNAME
      category    : CH3
      alt-env     : MPIR_CVAR_INTERFACE_HOSTNAME
      type        : string
      default     : NULL
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If non-NULL, this cvar specifies the IP address that
        other processes should use when connecting to this process.
        This cvar is mutually exclusive with the
        MPIR_CVAR_CH3_NETWORK_IFACE cvar and it is an error to set them
        both.

    - name        : MPIR_CVAR_CH3_PORT_RANGE
      category    : CH3
      alt-env     : MPIR_CVAR_PORTRANGE, MPIR_CVAR_PORT_RANGE
      type        : range
      default     : "0:0"
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The MPIR_CVAR_CH3_PORT_RANGE environment variable allows you to
        specify the range of TCP ports to be used by the process
        manager and the MPICH library. The format of this variable is
        <low>:<high>.  To specify any available port, use 0:0.

    - name        : MPIR_CVAR_NEMESIS_TCP_NETWORK_IFACE
      category    : NEMESIS
      alt-env     : MPIR_CVAR_NETWORK_IFACE
      type        : string
      default     : NULL
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If non-NULL, this cvar specifies which pseudo-ethernet
        interface the tcp netmod should use (e.g., "eth1", "ib0").
        Note, this is a Linux-specific cvar.
        This cvar is mutually exclusive with the
        MPIR_CVAR_CH3_INTERFACE_HOSTNAME cvar and it is an error to set
        them both.

    - name        : MPIR_CVAR_NEMESIS_TCP_HOST_LOOKUP_RETRIES
      category    : NEMESIS
      type        : int
      default     : 10
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This cvar controls the number of times to retry the
        gethostbyname() function before giving up.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#define DBG_IFNAME 0

#ifdef ENABLE_CHECKPOINTING
static int ckpt_restart(void);
#endif

MPID_nem_netmod_funcs_t MPIDI_nem_tcp_funcs = {
    MPID_nem_tcp_init,
    MPID_nem_tcp_finalize,
#ifdef ENABLE_CHECKPOINTING
    NULL,       /* ckpt_precheck */
    ckpt_restart,
    NULL,       /* ckpt_continue */
#endif
    MPID_nem_tcp_connpoll,
    MPID_nem_tcp_get_business_card,
    MPID_nem_tcp_connect_to_root,
    MPID_nem_tcp_vc_init,
    MPID_nem_tcp_vc_destroy,
    MPID_nem_tcp_vc_terminate,
    NULL,       /* anysource iprobe */
    NULL,       /* anysource_improbe */
    MPID_nem_tcp_get_ordering
};

/* in case there are no packet types defined (e.g., they're ifdef'ed out) make sure the array is not zero length */
static MPIDI_CH3_PktHandler_Fcn *pkt_handlers[MPIDI_NEM_TCP_PKT_NUM_TYPES ?
                                              MPIDI_NEM_TCP_PKT_NUM_TYPES : 1];

MPL_dbg_class MPIDI_NEM_TCP_DBG_DET;

int MPID_nem_tcp_listen(int sockfd);
static int set_up_listener(void)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_ERROR_CHECKING
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];
#endif

    MPIR_FUNC_ENTER;

    MPID_nem_tcp_g_lstn_plfd.fd = MPID_nem_tcp_g_lstn_sc.fd =
        socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    MPIR_ERR_CHKANDJUMP2(MPID_nem_tcp_g_lstn_sc.fd == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create",
                         "**sock_create %s %d", MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE),
                         errno);

    mpi_errno = MPID_nem_tcp_set_sockopts(MPID_nem_tcp_g_lstn_sc.fd);
    MPIR_ERR_CHECK(mpi_errno);

    MPID_nem_tcp_g_lstn_plfd.events = POLLIN;
    mpi_errno = MPID_nem_tcp_listen(MPID_nem_tcp_g_lstn_sc.fd);
    MPIR_ERR_CHKANDJUMP2(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**listen", "**listen %s %d",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
    MPID_nem_tcp_g_lstn_sc.state.lstate = LISTEN_STATE_LISTENING;
    MPID_nem_tcp_g_lstn_sc.handler = MPID_nem_tcp_state_listening_handler;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPID_nem_tcp_init(MPIDI_PG_t * pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_ERROR_CHECKING
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];
#endif

    MPIR_FUNC_ENTER;

    MPID_nem_net_module_vc_dbg_print_sendq = MPID_nem_tcp_vc_dbg_print_sendq;

    /* first make sure that our private fields in the vc fit into the area provided  */
    MPIR_Assert(sizeof(MPID_nem_tcp_vc_area) <= MPIDI_NEM_VC_NETMOD_AREA_LEN);

#if defined (MPL_USE_DBG_LOGGING)
    MPIDI_NEM_TCP_DBG_DET = MPL_dbg_class_alloc("MPIDI_NEM_TCP_DBG_DET", "nem_sock_det");
#endif /* MPL_USE_DBG_LOGGING */

    /* set up listener socket */
    mpi_errno = set_up_listener();
    MPIR_ERR_CHECK(mpi_errno);

    /* create business card */
    mpi_errno = MPID_nem_tcp_get_business_card(pg_rank, bc_val_p, val_max_sz_p);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPID_nem_tcp_sm_init();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPID_nem_tcp_send_init();
    MPIR_ERR_CHECK(mpi_errno);

#ifdef HAVE_SIGNAL
    {
        /* In order to be able to handle socket errors on our own, we need
         * to ignore SIGPIPE.  This may cause problems for programs that
         * intend to handle SIGPIPE or count on being killed, but I expect
         * such programs are very rare, and I'm not sure what the best
         * solution would be anyway. */
        /* Linux and BSD typedef to sighandler_t and sig_t respectively, which
         * means we can't use either. Declare directly instead. */
        void (*ret) (int);

        ret = signal(SIGPIPE, SIG_IGN);
        MPIR_ERR_CHKANDJUMP1(ret == SIG_ERR, mpi_errno, MPI_ERR_OTHER, "**signal", "**signal %s",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));
        if (ret != SIG_DFL && ret != SIG_IGN) {
            /* The app has set its own signal handler.  Replace the previous handler. */
            ret = signal(SIGPIPE, ret);
            MPIR_ERR_CHKANDJUMP1(ret == SIG_ERR, mpi_errno, MPI_ERR_OTHER, "**signal",
                                 "**signal %s", MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));
        }
    }
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
/*     fprintf(stdout, __func__ " Exit\n"); fflush(stdout); */
    return mpi_errno;
  fn_fail:
/*     fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno); */
    goto fn_exit;
}

#ifdef ENABLE_CHECKPOINTING

static int ckpt_restart(void)
{
    int mpi_errno = MPI_SUCCESS;
    char *publish_bc_orig = NULL;
    char *bc_val = NULL;
    int val_max_sz;
    int i;

    MPIR_FUNC_ENTER;

    /* First, clean up.  We didn't shut anything down before the
     * checkpoint, so we need to go close and free any resources */
    mpi_errno = MPID_nem_tcp_ckpt_cleanup();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPID_nem_tcp_send_finalize();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPID_nem_tcp_sm_finalize();
    MPIR_ERR_CHECK(mpi_errno);

    /* Initialize the new business card */
    mpi_errno = MPIDI_CH3I_BCInit(&bc_val, &val_max_sz);
    MPIR_ERR_CHECK(mpi_errno);
    publish_bc_orig = bc_val;

    /* Now we can restart */
    mpi_errno =
        MPID_nem_tcp_init(MPIDI_Process.my_pg, MPIDI_Process.my_pg_rank, &bc_val, &val_max_sz);
    MPIR_ERR_CHECK(mpi_errno);

    /* publish business card */
    mpi_errno = MPIDI_PG_SetConnInfo(MPIDI_Process.my_pg_rank, (const char *) publish_bc_orig);
    MPIR_ERR_CHECK(mpi_errno);
    MPL_free(publish_bc_orig);

    for (i = 0; i < MPIDI_Process.my_pg->size; ++i) {
        MPIDI_VC_t *vc;
        MPIDI_CH3I_VC *vc_ch;
        if (i == MPIDI_Process.my_pg_rank)
            continue;
        MPIDI_PG_Get_vc(MPIDI_Process.my_pg, i, &vc);
        vc_ch = &vc->ch;
        if (!vc_ch->is_local) {
            mpi_errno = vc_ch->ckpt_restart_vc(vc);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }


  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}
#endif

/*
 * Get a description of the network interface to use for socket communication
 *
 * Here are the steps.  This order of checks is used to provide the
 * user control over the choice of interface and to avoid, where possible,
 * the use of non-scalable services, such as centeralized name servers.
 *
 * MPICH_INTERFACE_HOSTNAME
 * MPICH_INTERFACE_HOSTNAME_R%d
 * a single (non-localhost) available IP address, if possible
 * gethostbyname(gethostname())
 *
 * We return the following items:
 *
 *    ifname - hostname of the interface.  This may or may not be the same
 *             as the name returned by gethostname  (in Unix)
 *    ifaddr - This structure includes the interface IP address (as bytes),
 *             and the type (e.g., AF_INET or AF_INET6).  Only
 *             ipv4 (AF_INET) is used so far.
 */

static int GetSockInterfaceAddr(int myRank, char *ifname, int maxIfname, MPL_sockaddr_t * p_addr)
{
    const char *ifname_string;
    int mpi_errno = MPI_SUCCESS;
    int ifaddrFound = 0;

    MPIR_Assert(maxIfname);
    ifname[0] = '\0';

    MPIR_ERR_CHKANDJUMP(MPIR_CVAR_CH3_INTERFACE_HOSTNAME &&
                        MPIR_CVAR_NEMESIS_TCP_NETWORK_IFACE, mpi_errno, MPI_ERR_OTHER,
                        "**ifname_and_hostname");

    /* Check if user specified ethernet interface name, e.g., ib0, eth1 */
    if (MPIR_CVAR_NEMESIS_TCP_NETWORK_IFACE) {
        char s[100];
        int len;
        int ret = MPL_get_sockaddr_iface(MPIR_CVAR_NEMESIS_TCP_NETWORK_IFACE, p_addr);
        MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**iface_notfound",
                             "**iface_notfound %s", MPIR_CVAR_NEMESIS_TCP_NETWORK_IFACE);

        MPL_sockaddr_to_str(p_addr, s, 100);
        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CONNECT, VERBOSE, (MPL_DBG_FDEST, "ifaddrFound: %s", s));

        /* In this case, ifname is only used for debugging purposes */
        mpi_errno = MPID_Get_processor_name(ifname, maxIfname, &len);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* Check for a host name supplied through an environment variable */
    ifname_string = MPIR_CVAR_CH3_INTERFACE_HOSTNAME;
    if (!ifname_string) {
        /* See if there is a per-process name for the interfaces (e.g.,
         * the process manager only delievers the same values for the
         * environment to each process.  There's no way to do this with
         * the param interface, so we need to use getenv() here. */
        char namebuf[1024];
        MPL_snprintf(namebuf, sizeof(namebuf), "MPICH_INTERFACE_HOSTNAME_R%d", myRank);
        ifname_string = getenv(namebuf);

        if (DBG_IFNAME && ifname_string) {
            fprintf(stdout, "Found interface name %s from %s\n", ifname_string, namebuf);
            fflush(stdout);
        }
    } else if (DBG_IFNAME) {
        fprintf(stdout, "Found interface name %s from MPICH_INTERFACE_HOSTNAME\n", ifname_string);
        fflush(stdout);
    }

    if (!ifname_string) {
        int len;

        /* User did not specify a hostname.  Look it up. */
        mpi_errno = MPID_Get_processor_name(ifname, maxIfname, &len);
        MPIR_ERR_CHECK(mpi_errno);

        ifname_string = ifname;

        /* If we didn't find a specific name, then try to get an IP address
         * directly from the available interfaces, if that is supported on
         * this platform.  Otherwise, we'll drop into the next step that uses
         * the ifname */
        int ret = MPL_get_sockaddr_iface(NULL, p_addr);
        MPIR_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, "**iface_notfound",
                             "**iface_notfound %s", NULL);
        ifaddrFound = 1;
    } else {
        /* Copy this name into the output name */
        MPL_strncpy(ifname, ifname_string, maxIfname);
    }

    /* If we don't have an IP address, try to get it from the name */
    if (!ifaddrFound) {
        int ret = MPL_get_sockaddr(ifname_string, p_addr);
        MPIR_ERR_CHKANDJUMP2(ret != 0, mpi_errno, MPI_ERR_OTHER, "**gethostbyname",
                             "**gethostbyname %s %d", ifname_string, h_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPID_nem_tcp_get_business_card(int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPL_SUCCESS;
    MPL_sockaddr_t addr;
    char ifname[MAX_HOST_DESCRIPTION_LEN];
    int ret;
    MPL_sockaddr_t sock_id;
    socklen_t len;
#ifdef HAVE_ERROR_CHECKING
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];
#endif

    MPIR_FUNC_ENTER;

    mpi_errno = GetSockInterfaceAddr(my_rank, ifname, sizeof(ifname), &addr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);


    str_errno =
        MPL_str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_HOST_DESCRIPTION_KEY, ifname);
    if (str_errno) {
        MPIR_ERR_CHKANDJUMP(str_errno == MPL_ERR_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }

    len = sizeof(sock_id);
    ret = getsockname(MPID_nem_tcp_g_lstn_sc.fd, (struct sockaddr *) &sock_id, &len);
    MPIR_ERR_CHKANDJUMP1(ret == -1, mpi_errno, MPI_ERR_OTHER, "**getsockname", "**getsockname %s",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));

    str_errno =
        MPL_str_add_int_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY,
                            MPL_sockaddr_port(&sock_id));
    if (str_errno) {
        MPIR_ERR_CHKANDJUMP(str_errno == MPL_ERR_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }

    if (addr.ss_family == AF_INET) {
        MPL_sockaddr_to_str(&addr, ifname, MAX_HOST_DESCRIPTION_LEN);
        MPL_DBG_MSG_S(MPIDI_CH3_DBG_CONNECT, VERBOSE, "ifname = %s", ifname);
        str_errno = MPL_str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_IFNAME_KEY, ifname);
        if (str_errno) {
            MPIR_ERR_CHKANDJUMP(str_errno == MPL_ERR_STR_NOMEM, mpi_errno, MPI_ERR_OTHER,
                                "**buscard_len");
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
    }


    /*     printf("MPID_nem_tcp_get_business_card. port=%d\n", sock_id.sin_port); */

  fn_exit:
/*     fprintf(stdout, "MPID_nem_tcp_get_business_card Exit, mpi_errno=%d\n", mpi_errno); fflush(stdout); */
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_tcp_connect_to_root(const char *business_card, MPIDI_VC_t * new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    struct in_addr addr;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(new_vc);

    MPIR_FUNC_ENTER;

    /* vc is already allocated before reaching this point */

    mpi_errno = MPID_nem_tcp_get_addr_port_from_bc(business_card, &addr, &vc_tcp->sock_id.sin_port);
    vc_tcp->sock_id.sin_addr.s_addr = addr.s_addr;
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_GetTagFromPort(business_card, &new_vc->port_name_tag);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPID_nem_tcp_connect(new_vc);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPID_nem_tcp_vc_init(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);

    MPIR_FUNC_ENTER;

    vc_tcp->state = MPID_NEM_TCP_VC_STATE_DISCONNECTED;

    vc->sendNoncontig_fn = MPID_nem_tcp_SendNoncontig;
    vc_ch->iStartContigMsg = MPID_nem_tcp_iStartContigMsg;
    vc_ch->iSendContig = MPID_nem_tcp_iSendContig;
    vc_ch->iSendIov = MPID_nem_tcp_iSendIov;
#ifdef ENABLE_CHECKPOINTING
    vc_ch->ckpt_pause_send_vc = MPID_nem_tcp_ckpt_pause_send_vc;
    vc_ch->ckpt_continue_vc = MPID_nem_tcp_ckpt_continue_vc;
    vc_ch->ckpt_restart_vc = MPID_nem_tcp_ckpt_restart_vc;

    pkt_handlers[MPIDI_NEM_TCP_PKT_UNPAUSE] = MPID_nem_tcp_pkt_unpause_handler;
#endif

    vc_ch->pkt_handler = pkt_handlers;
    vc_ch->num_pkt_handlers = MPIDI_NEM_TCP_PKT_NUM_TYPES;

    memset(&vc_tcp->sock_id, 0, sizeof(vc_tcp->sock_id));
    vc_tcp->sock_id.sin_family = AF_INET;

    vc_ch->next = NULL;
    vc_ch->prev = NULL;

    ASSIGN_SC_TO_VC(vc_tcp, NULL);
    vc_tcp->send_queue.head = vc_tcp->send_queue.tail = NULL;

    vc_tcp->send_paused = FALSE;
    vc_tcp->paused_send_queue.head = vc_tcp->paused_send_queue.tail = NULL;

    vc_tcp->sc_ref_count = 0;

    vc_tcp->connect_retry_count = 0;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPID_nem_tcp_vc_destroy(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;

    /* currently do nothing */

    return mpi_errno;
}


/*
   FIXME: this is the same function as in socksm.c
   This should be removed and use only one function eventually.
*/

int MPID_nem_tcp_get_addr_port_from_bc(const char *business_card, struct in_addr *addr,
                                       in_port_t * port)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int port_int;
    /*char desc_str[256]; */
    char ifname[256];

    MPIR_FUNC_ENTER;

    /*     fprintf(stdout, __func__ " Enter\n"); fflush(stdout); */
    /* desc_str is only used for debugging
     * ret = MPL_str_get_string_arg (business_card, MPIDI_CH3I_HOST_DESCRIPTION_KEY, desc_str, sizeof(desc_str));
     * MPIR_ERR_CHKANDJUMP (ret != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
     */

    /* sizeof(in_port_t) != sizeof(int) on most platforms, so we need to use
     * port_int as the arg to MPL_str_get_int_arg. */
    ret = MPL_str_get_int_arg(business_card, MPIDI_CH3I_PORT_KEY, &port_int);
    /* MPL_ERR_STR_FAIL is not a valid MPI error code so we store the result in ret
     * instead of mpi_errno. */
    MPIR_ERR_CHKANDJUMP(ret != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missingport");
    MPIR_Assert((port_int >> (8 * sizeof(*port))) == 0);        /* ensure port_int isn't too large for *port */
    *port = htons((in_port_t) port_int);

    ret = MPL_str_get_string_arg(business_card, MPIDI_CH3I_IFNAME_KEY, ifname, sizeof(ifname));
    MPIR_ERR_CHKANDJUMP(ret != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missingifname");

    ret = inet_pton(AF_INET, (const char *) ifname, addr);
    MPIR_ERR_CHKANDJUMP(ret == 0, mpi_errno, MPI_ERR_OTHER, "**ifnameinvalid");
    MPIR_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**afinetinvalid");

  fn_exit:
/*     fprintf(stdout, __func__ " Exit\n"); fflush(stdout); */
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
/*     fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno); */
    MPL_DBG_MSG_FMT(MPIDI_NEM_TCP_DBG_DET, VERBOSE,
                    (MPL_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* MPID_nem_tcp_listen -- if MPICH_PORT_RANGE is set, this
   binds the socket to an available port number in the range.
   Otherwise, it binds it to any addr and any port */
int MPID_nem_tcp_listen(int sockfd)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    unsigned short port;
#ifdef HAVE_ERROR_CHECKING
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];
#endif

    MPIR_FUNC_ENTER;

    MPIR_ERR_CHKANDJUMP(MPIR_CVAR_CH3_PORT_RANGE.low < 0 ||
                        MPIR_CVAR_CH3_PORT_RANGE.low > MPIR_CVAR_CH3_PORT_RANGE.high, mpi_errno,
                        MPI_ERR_OTHER, "**badportrange");

    /* default MPICH_PORT_RANGE is {0,0} so bind will use any available port */
    ret = 0;
    if (MPIR_CVAR_CH3_PORT_RANGE.low == 0) {
        ret = MPL_listen_anyport(sockfd, &port);
    } else {
        ret =
            MPL_listen_portrange(sockfd, &port, MPIR_CVAR_CH3_PORT_RANGE.low,
                                 MPIR_CVAR_CH3_PORT_RANGE.high);
    }
    if (ret == -2) {
        /* check if an available port was found */
        MPIR_ERR_CHKANDJUMP3(1, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind",
                             "**sock|poll|bind %d %d %s", port - 1, errno,
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));
    } else if (ret) {
        /* check for real error */
        MPIR_ERR_CHKANDJUMP3(errno != EADDRINUSE &&
                             errno != EADDRNOTAVAIL, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind",
                             "**sock|poll|bind %d %d %s", port, errno,
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE));
    }

  fn_exit:
/*     if (ret == 0) */
/*         fprintf(stdout, "sockfd=%d  port=%d bound\n", sockfd, port); */
/*     fprintf(stdout, __func__ " Exit\n"); fflush(stdout); */
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
/*     fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno); */
    MPL_DBG_MSG_FMT(MPIDI_NEM_TCP_DBG_DET, VERBOSE,
                    (MPL_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

int MPID_nem_tcp_vc_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    int req_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (vc->state != MPIDI_VC_STATE_CLOSED) {
        /* VC is terminated as a result of a fault.  Complete
         * outstanding sends with an error and terminate
         * connection immediately. */
        MPIR_ERR_SET1(req_errno, MPIX_ERR_PROC_FAILED, "**comm_fail", "**comm_fail %d",
                      vc->pg_rank);
        mpi_errno = MPID_nem_tcp_error_out_send_queue(vc, req_errno);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPID_nem_tcp_vc_terminated(vc);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
        /* VC is terminated as a result of the close protocol.
         * Wait for sends to complete, then terminate. */

        if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue)) {
            /* The sendq is empty, so we can immediately terminate
             * the connection. */
            mpi_errno = MPID_nem_tcp_vc_terminated(vc);
            MPIR_ERR_CHECK(mpi_errno);
        }
        /* else: just return.  We'll call vc_terminated() from the
         * commrdy_handler once the sendq is empty. */
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPID_nem_tcp_vc_terminated(MPIDI_VC_t * vc)
{
    /* This is called when the VC is to be terminated once all queued
     * sends have been sent. */
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPID_nem_tcp_cleanup(vc);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    MPL_DBG_MSG_FMT(MPIDI_NEM_TCP_DBG_DET, VERBOSE,
                    (MPL_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

int MPID_nem_tcp_get_ordering(int *ordering)
{
    (*ordering) = 1;
    return MPI_SUCCESS;
}
