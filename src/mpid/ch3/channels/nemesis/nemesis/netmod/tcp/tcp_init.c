/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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

#define DBG_IFNAME 0

#ifdef ENABLE_CHECKPOINTING
static int ckpt_restart(void);
#endif

MPID_nem_netmod_funcs_t MPIDI_nem_tcp_funcs = {
    MPID_nem_tcp_init,
    MPID_nem_tcp_finalize,
#ifdef ENABLE_CHECKPOINTING
    NULL, /* ckpt_precheck */
    ckpt_restart,
    NULL, /* ckpt_continue */
#endif
    MPID_nem_tcp_connpoll,
    MPID_nem_tcp_get_business_card,
    MPID_nem_tcp_connect_to_root,
    MPID_nem_tcp_vc_init,
    MPID_nem_tcp_vc_destroy,
    MPID_nem_tcp_vc_terminate,
    NULL /* anysource iprobe */
};

/* in case there are no packet types defined (e.g., they're ifdef'ed out) make sure the array is not zero length */
static MPIDI_CH3_PktHandler_Fcn *pkt_handlers[MPIDI_NEM_TCP_PKT_NUM_TYPES ? MPIDI_NEM_TCP_PKT_NUM_TYPES : 1];
    

#undef FUNCNAME
#define FUNCNAME set_up_listener
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int set_up_listener(void)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_SET_UP_LISTENER);

    MPIDI_FUNC_ENTER(MPID_STATE_SET_UP_LISTENER);

    MPID_nem_tcp_g_lstn_plfd.fd = MPID_nem_tcp_g_lstn_sc.fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    MPIU_ERR_CHKANDJUMP2(MPID_nem_tcp_g_lstn_sc.fd == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create", "**sock_create %s %d", MPIU_Strerror(errno), errno);

    mpi_errno = MPID_nem_tcp_set_sockopts(MPID_nem_tcp_g_lstn_sc.fd);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPID_nem_tcp_g_lstn_plfd.events = POLLIN;
    mpi_errno = MPID_nem_tcp_bind(MPID_nem_tcp_g_lstn_sc.fd);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    ret = listen(MPID_nem_tcp_g_lstn_sc.fd, SOMAXCONN);
    MPIU_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**listen", "**listen %s %d", MPIU_Strerror(errno), errno);  
    MPID_nem_tcp_g_lstn_sc.state.lstate = LISTEN_STATE_LISTENING;
    MPID_nem_tcp_g_lstn_sc.handler = MPID_nem_tcp_state_listening_handler;

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SET_UP_LISTENER);
    return mpi_errno;
fn_fail:

    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_init (MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_INIT);

    MPID_nem_net_module_vc_dbg_print_sendq = MPID_nem_tcp_vc_dbg_print_sendq;

    /* first make sure that our private fields in the vc fit into the area provided  */
    MPIU_Assert(sizeof(MPID_nem_tcp_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);

    /* set up listener socket */
    mpi_errno = set_up_listener();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    /* create business card */
    mpi_errno = MPID_nem_tcp_get_business_card(pg_rank, bc_val_p, val_max_sz_p);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPID_nem_tcp_sm_init();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_nem_tcp_send_init();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

#ifdef HAVE_SIGNAL
    {
        /* In order to be able to handle socket errors on our own, we need
           to ignore SIGPIPE.  This may cause problems for programs that
           intend to handle SIGPIPE or count on being killed, but I expect
           such programs are very rare, and I'm not sure what the best
           solution would be anyway. */
        void *ret;

        ret = signal(SIGPIPE, SIG_IGN);
        MPIU_ERR_CHKANDJUMP1(ret == SIG_ERR, mpi_errno, MPI_ERR_OTHER, "**signal", "**signal %s", MPIU_Strerror(errno));
        if (ret != SIG_DFL && ret != SIG_IGN) {
            /* The app has set its own signal handler.  Replace the previous handler. */
            ret = signal(SIGPIPE, ret);
            MPIU_ERR_CHKANDJUMP1(ret == SIG_ERR, mpi_errno, MPI_ERR_OTHER, "**signal", "**signal %s", MPIU_Strerror(errno));
        }
    }
#endif
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_INIT);
/*     fprintf(stdout, FCNAME " Exit\n"); fflush(stdout); */
    return mpi_errno;
 fn_fail:
/*     fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno); */
    goto fn_exit;
}

#ifdef ENABLE_CHECKPOINTING

#undef FUNCNAME
#define FUNCNAME ckpt_restart
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ckpt_restart(void)
{
    int mpi_errno = MPI_SUCCESS;
    char *publish_bc_orig = NULL;
    char *bc_val          = NULL;
    int val_max_sz;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_CKPT_RESTART);

    MPIDI_FUNC_ENTER(MPID_STATE_CKPT_RESTART);

    /* First, clean up.  We didn't shut anything down before the
       checkpoint, so we need to go close and free any resources */
    mpi_errno = MPID_nem_tcp_ckpt_cleanup();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_nem_tcp_send_finalize();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_nem_tcp_sm_finalize();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Initialize the new business card */
    mpi_errno = MPIDI_CH3I_BCInit(&bc_val, &val_max_sz);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    publish_bc_orig = bc_val;
    
    /* Now we can restart */
    mpi_errno = MPID_nem_tcp_init(MPIDI_Process.my_pg, MPIDI_Process.my_pg_rank, &bc_val, &val_max_sz);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    /* publish business card */
    mpi_errno = MPIDI_PG_SetConnInfo(MPIDI_Process.my_pg_rank, (const char *)publish_bc_orig);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIU_Free(publish_bc_orig);

    for (i = 0; i < MPIDI_Process.my_pg->size; ++i) {
        MPIDI_VC_t *vc;
        MPIDI_CH3I_VC *vc_ch;
        if (i == MPIDI_Process.my_pg_rank)
            continue;
        MPIDI_PG_Get_vc(MPIDI_Process.my_pg, i, &vc);
        vc_ch = VC_CH(vc);
        if (!vc_ch->is_local) {
            mpi_errno = vc_ch->ckpt_restart_vc(vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }
    

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CKPT_RESTART);
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
 *    ifname - name of the interface.  This may or may not be the same
 *             as the name returned by gethostname  (in Unix)
 *    ifaddr - This structure includes the interface IP address (as bytes),
 *             and the type (e.g., AF_INET or AF_INET6).  Only 
 *             ipv4 (AF_INET) is used so far.
 */

static int GetSockInterfaceAddr(int myRank, char *ifname, int maxIfname,
                                MPIDU_Sock_ifaddr_t *ifaddr)
{
    const char *ifname_string;
    int mpi_errno = MPI_SUCCESS;
    int ifaddrFound = 0;

    MPIU_ERR_CHKANDJUMP(MPIR_PARAM_INTERFACE_HOSTNAME && MPIR_PARAM_NETWORK_IFACE, mpi_errno, MPI_ERR_OTHER, "**ifname_and_hostname");
    
    /* Set "not found" for ifaddr */
    ifaddr->len = 0;

    /* Check if user specified ethernet interface name, e.g., ib0, eth1 */
    if (MPIR_PARAM_NETWORK_IFACE) {
        mpi_errno = MPIDI_Get_IP_for_iface(MPIR_PARAM_NETWORK_IFACE, ifaddr, &ifaddrFound);
        MPIU_ERR_CHKANDJUMP1(mpi_errno || !ifaddrFound, mpi_errno, MPI_ERR_OTHER, "**iface_notfound", "**iface_notfound %s", MPIR_PARAM_NETWORK_IFACE);
        
        MPIU_DBG_MSG_FMT(CH3_CONNECT, VERBOSE, (MPIU_DBG_FDEST,
                                                "ifaddrFound=TRUE ifaddr->type=%d ifaddr->len=%d ifaddr->ifaddr[0-3]=%#08x",
                                                ifaddr->type, ifaddr->len, *((unsigned int *)ifaddr->ifaddr)));
        goto fn_exit;
    }

    /* Check for a host name supplied through an environment variable */
    ifname_string = MPIR_PARAM_INTERFACE_HOSTNAME;
    if (!ifname_string) {
	/* See if there is a per-process name for the interfaces (e.g.,
	   the process manager only delievers the same values for the 
	   environment to each process.  There's no way to do this with
           the param interface, so we need to use getenv() here. */
	char namebuf[1024];
	MPIU_Snprintf( namebuf, sizeof(namebuf), 
		       "MPICH_INTERFACE_HOSTNAME_R%d", myRank );
	ifname_string = getenv( namebuf );

	if (DBG_IFNAME && ifname_string) {
	    fprintf( stdout, "Found interface name %s from %s\n", 
		    ifname_string, namebuf );
	    fflush( stdout );
	}
    }
    else if (DBG_IFNAME) {
	fprintf( stdout, 
		 "Found interface name %s from MPICH_INTERFACE_HOSTNAME\n", 
		 ifname_string );
	fflush( stdout );
    }
	 
    if (!ifname_string) {
	int len;

	/* User did not specify a hostname.  Look it up. */
	mpi_errno = MPID_Get_processor_name(ifname, maxIfname, &len );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

	ifname_string = ifname;

	/* If we didn't find a specific name, then try to get an IP address
	   directly from the available interfaces, if that is supported on
	   this platform.  Otherwise, we'll drop into the next step that uses 
	   the ifname */
	mpi_errno = MPIDI_GetIPInterface( ifaddr, &ifaddrFound );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
	/* Copy this name into the output name */
	MPIU_Strncpy( ifname, ifname_string, maxIfname );
    }

    /* If we don't have an IP address, try to get it from the name */
    if (!ifaddrFound) {
        int i;
	struct hostent *info = NULL;
        for (i = 0; i < MPIR_PARAM_HOST_LOOKUP_RETRIES; ++i) {
            info = gethostbyname( ifname_string );
            if (info || h_errno != TRY_AGAIN)
                break;
        }
        MPIU_ERR_CHKANDJUMP2(!info || !info->h_addr_list, mpi_errno, MPI_ERR_OTHER, "**gethostbyname", "**gethostbyname %s %d", ifname_string, h_errno);
        
        /* Use the primary address */
        ifaddr->len  = info->h_length;
        ifaddr->type = info->h_addrtype;
        if (ifaddr->len > sizeof(ifaddr->ifaddr)) {
            /* If the address won't fit in the field, reset to
               no address */
            ifaddr->len = 0;
            ifaddr->type = -1;
            MPIU_ERR_INTERNAL(mpi_errno, "Address too long to fit in field");
        } else {
            MPIU_Memcpy( ifaddr->ifaddr, info->h_addr_list[0], ifaddr->len );
	}
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPIU_STR_SUCCESS;
    MPIDU_Sock_ifaddr_t ifaddr;
    char ifname[MAX_HOST_DESCRIPTION_LEN];
    int ret;
    struct sockaddr_in sock_id;
    socklen_t len;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_GET_BUSINESS_CARD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_GET_BUSINESS_CARD);
    
    mpi_errno = GetSockInterfaceAddr(my_rank, ifname, sizeof(ifname), &ifaddr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    
    str_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_HOST_DESCRIPTION_KEY, ifname);
    if (str_errno) {
        MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }

    len = sizeof(sock_id);
    ret = getsockname (MPID_nem_tcp_g_lstn_sc.fd, (struct sockaddr *)&sock_id, &len);
    MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**getsockname", "**getsockname %s", MPIU_Strerror (errno));

    str_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, ntohs(sock_id.sin_port));
    if (str_errno) {
        MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }
    
    if (ifaddr.len > 0 && ifaddr.type == AF_INET)
    {
        unsigned char *p;
        p = (unsigned char *)(ifaddr.ifaddr);
        MPIU_Snprintf( ifname, sizeof(ifname), "%u.%u.%u.%u", p[0], p[1], p[2], p[3] );
        MPIU_DBG_MSG_S(CH3_CONNECT,VERBOSE,"ifname = %s",ifname );
        str_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_IFNAME_KEY, ifname);
        if (str_errno) {
            MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
    }
    

    /*     printf("MPID_nem_tcp_get_business_card. port=%d\n", sock_id.sin_port); */

 fn_exit:
/*     fprintf(stdout, "MPID_nem_tcp_get_business_card Exit, mpi_errno=%d\n", mpi_errno); fflush(stdout); */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_GET_BUSINESS_CARD);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    struct in_addr addr;
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(new_vc);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_CONNECT_TO_ROOT);

    /* vc is already allocated before reaching this point */

    mpi_errno = MPID_nem_tcp_get_addr_port_from_bc(business_card, &addr, &vc_tcp->sock_id.sin_port);
    vc_tcp->sock_id.sin_addr.s_addr = addr.s_addr;
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIDI_GetTagFromPort(business_card, &new_vc->port_name_tag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_nem_tcp_connect(new_vc);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_CONNECT_TO_ROOT);
    return mpi_errno;

 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_vc_init (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = VC_CH(vc);
    MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_VC_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_VC_INIT);

    vc_tcp->state = MPID_NEM_TCP_VC_STATE_DISCONNECTED;
    
    vc->sendNoncontig_fn   = MPID_nem_tcp_SendNoncontig;
    vc_ch->iStartContigMsg = MPID_nem_tcp_iStartContigMsg;
    vc_ch->iSendContig     = MPID_nem_tcp_iSendContig;
#ifdef ENABLE_CHECKPOINTING
    vc_ch->ckpt_pause_send_vc = MPID_nem_tcp_ckpt_pause_send_vc;
    vc_ch->ckpt_continue_vc   = MPID_nem_tcp_ckpt_continue_vc;
    vc_ch->ckpt_restart_vc    = MPID_nem_tcp_ckpt_restart_vc;

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

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_VC_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;

    /* currently do nothing */

    return mpi_errno;
}


/* 
   FIXME: this is the same function as in socksm.c 
   This should be removed and use only one function eventually.
*/
   
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_get_addr_port_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_get_addr_port_from_bc(const char *business_card, struct in_addr *addr, in_port_t *port)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int port_int;
    /*char desc_str[256];*/
    char ifname[256];
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_GET_ADDR_PORT_FROM_BC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_GET_ADDR_PORT_FROM_BC);
    
    /*     fprintf(stdout, FCNAME " Enter\n"); fflush(stdout); */
    /* desc_str is only used for debugging
    ret = MPIU_Str_get_string_arg (business_card, MPIDI_CH3I_HOST_DESCRIPTION_KEY, desc_str, sizeof(desc_str));
    MPIU_ERR_CHKANDJUMP (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
    */

    /* sizeof(in_port_t) != sizeof(int) on most platforms, so we need to use
     * port_int as the arg to MPIU_Str_get_int_arg. */
    ret = MPIU_Str_get_int_arg (business_card, MPIDI_CH3I_PORT_KEY, &port_int);
    /* MPIU_STR_FAIL is not a valid MPI error code so we store the result in ret
     * instead of mpi_errno. */
    MPIU_ERR_CHKANDJUMP (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missingport");
    MPIU_Assert((port_int >> (8*sizeof(*port))) == 0); /* ensure port_int isn't too large for *port */
    *port = htons((in_port_t)port_int);

    ret = MPIU_Str_get_string_arg(business_card, MPIDI_CH3I_IFNAME_KEY, ifname, sizeof(ifname));
    MPIU_ERR_CHKANDJUMP (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missingifname");

    ret = inet_pton (AF_INET, (const char *)ifname, addr);
    MPIU_ERR_CHKANDJUMP(ret == 0, mpi_errno,MPI_ERR_OTHER,"**ifnameinvalid");
    MPIU_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**afinetinvalid");
    
 fn_exit:
/*     fprintf(stdout, FCNAME " Exit\n"); fflush(stdout); */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_GET_ADDR_PORT_FROM_BC);
    return mpi_errno;
 fn_fail:
/*     fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno); */
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* MPID_nem_tcp_bind -- if MPICH_PORT_RANGE is set, this
   binds the socket to an available port number in the range.
   Otherwise, it binds it to any addr and any port */
#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_bind
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_bind (int sockfd)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    struct sockaddr_in sin;
    int port;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_BIND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_BIND);
   
    MPIU_ERR_CHKANDJUMP(MPIR_PARAM_PORT_RANGE.low < 0 || MPIR_PARAM_PORT_RANGE.low > MPIR_PARAM_PORT_RANGE.high, mpi_errno, MPI_ERR_OTHER, "**badportrange");

    /* default MPICH_PORT_RANGE is {0,0} so bind will use any available port */
    ret = 0;
    for (port = MPIR_PARAM_PORT_RANGE.low; port <= MPIR_PARAM_PORT_RANGE.high; ++port)
    {
        memset ((void *)&sin, 0, sizeof(sin));
        sin.sin_family      = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        sin.sin_port        = htons(port);

        ret = bind (sockfd, (struct sockaddr *)&sin, sizeof(sin));
        if (ret == 0)
            break;
        
        /* check for real error */
        MPIU_ERR_CHKANDJUMP3 (errno != EADDRINUSE && errno != EADDRNOTAVAIL, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", port, errno, MPIU_Strerror (errno));
    }
    /* check if an available port was found */
    MPIU_ERR_CHKANDJUMP3 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", port, errno, MPIU_Strerror (errno));

 fn_exit:
/*     if (ret == 0) */
/*         fprintf(stdout, "sockfd=%d  port=%d bound\n", sockfd, port); */
/*     fprintf(stdout, FCNAME " Exit\n"); fflush(stdout); */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_BIND);
    return mpi_errno;
 fn_fail:
/*     fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno); */
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_vc_terminate
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_tcp_vc_terminate(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    int req_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_VC_TERMINATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_VC_TERMINATE);

    if (vc->state != MPIDI_VC_STATE_CLOSED) {
        /* VC is terminated as a result of a fault.  Complete
           outstanding sends with an error and terminate
           connection immediately. */
        MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
        mpi_errno = MPID_nem_tcp_error_out_send_queue(vc, req_errno);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        mpi_errno = MPID_nem_tcp_vc_terminated(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    } else {
        MPID_nem_tcp_vc_area *vc_tcp = VC_TCP(vc);
        /* VC is terminated as a result of the close protocol.
           Wait for sends to complete, then terminate. */

        if (MPIDI_CH3I_Sendq_empty(vc_tcp->send_queue)) {
            /* The sendq is empty, so we can immediately terminate
               the connection. */
            mpi_errno = MPID_nem_tcp_vc_terminated(vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        /* else: just return.  We'll call vc_terminated() from the
           commrdy_handler once the sendq is empty. */
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_VC_TERMINATE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_vc_terminated
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_vc_terminated(MPIDI_VC_t *vc)
{
    /* This is called when the VC is to be terminated once all queued
       sends have been sent. */
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_NEM_TCP_VC_TERMINATED);

    MPIDI_FUNC_ENTER(MPID_NEM_TCP_VC_TERMINATED);

    mpi_errno = MPID_nem_tcp_cleanup(vc);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    if(mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_NEM_TCP_VC_TERMINATED);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

