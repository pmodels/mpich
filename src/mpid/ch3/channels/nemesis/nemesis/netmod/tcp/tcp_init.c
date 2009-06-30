/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "tcp_impl.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define DBG_IFNAME 0

MPID_nem_netmod_funcs_t MPIDI_nem_tcp_funcs = {
    MPID_nem_tcp_init,
    MPID_nem_tcp_finalize,
    MPID_nem_tcp_ckpt_shutdown,
    MPID_nem_tcp_connpoll,
    MPID_nem_tcp_send,
    MPID_nem_tcp_get_business_card,
    MPID_nem_tcp_connect_to_root,
    MPID_nem_tcp_vc_init,
    MPID_nem_tcp_vc_destroy,
    MPID_nem_tcp_vc_terminate
};

#define MPIDI_CH3I_PORT_KEY "port"
#define MPIDI_CH3I_HOST_DESCRIPTION_KEY "description"
#define MPIDI_CH3I_IFNAME_KEY "ifname"

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_init (MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue,
                                 MPID_nem_cell_ptr_t proc_elements, int num_proc_elements, MPID_nem_cell_ptr_t module_elements,
                                 int num_module_elements, MPID_nem_queue_ptr_t *module_free_queue,
                                 int ckpt_restart, MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_INIT);

    MPID_nem_net_module_vc_dbg_print_sendq = MPID_nem_tcp_vc_dbg_print_sendq;

    /* first make sure that our private fields in the vc fit into the area provided  */
    MPIU_Assert(sizeof(MPID_nem_tcp_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);

    /* set up listener socket */
/*     fprintf(stdout, FCNAME " Enter\n"); fflush(stdout); */
    MPID_nem_tcp_g_lstn_plfd.fd = MPID_nem_tcp_g_lstn_sc.fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    MPIU_ERR_CHKANDJUMP2 (MPID_nem_tcp_g_lstn_sc.fd == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create", "**sock_create %s %d", MPIU_Strerror (errno), errno);

    mpi_errno = MPID_nem_tcp_set_sockopts (MPID_nem_tcp_g_lstn_sc.fd);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    MPID_nem_tcp_g_lstn_plfd.events = POLLIN;
    mpi_errno = MPID_nem_tcp_bind (MPID_nem_tcp_g_lstn_sc.fd);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    ret = listen (MPID_nem_tcp_g_lstn_sc.fd, SOMAXCONN);	      
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**listen", "**listen %s %d", errno, MPIU_Strerror (errno));  
    MPID_nem_tcp_g_lstn_sc.state.lstate = LISTEN_STATE_LISTENING;
    MPID_nem_tcp_g_lstn_sc.handler = MPID_nem_tcp_state_listening_handler;

    /* create business card */
    mpi_errno = MPID_nem_tcp_get_business_card(pg_rank, bc_val_p, val_max_sz_p);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    *module_free_queue = NULL;

    mpi_errno = MPID_nem_tcp_sm_init();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_nem_tcp_send_init();
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);


 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_TCP_INIT);
/*     fprintf(stdout, FCNAME " Exit\n"); fflush(stdout); */
    return mpi_errno;
 fn_fail:
/*     fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno); */
    goto fn_exit;
}

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
    char *ifname_string;
    int mpi_errno = MPI_SUCCESS;
    int ifaddrFound = 0;

    /* Set "not found" for ifaddr */
    ifaddr->len = 0;

    /* Check for the name supplied through an environment variable */
    ifname_string = getenv("MPICH_INTERFACE_HOSTNAME");
    if (!ifname_string) {
	/* See if there is a per-process name for the interfaces (e.g.,
	   the process manager only delievers the same values for the 
	   environment to each process */
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

	/* If we have nothing, then use the host name */
	mpi_errno = MPID_Get_processor_name(ifname, maxIfname, &len );
	ifname_string = ifname;

	/* If we didn't find a specific name, then try to get an IP address
	   directly from the available interfaces, if that is supported on
	   this platform.  Otherwise, we'll drop into the next step that uses 
	   the ifname */
	mpi_errno = MPIDI_GetIPInterface( ifaddr, &ifaddrFound );
    }
    else {
	/* Copy this name into the output name */
	MPIU_Strncpy( ifname, ifname_string, maxIfname );
    }

    /* If we don't have an IP address, try to get it from the name */
    if (!ifaddrFound) {
	struct hostent *info;
	info = gethostbyname( ifname_string );
	if (info && info->h_addr_list) {
	    /* Use the primary address */
	    ifaddr->len  = info->h_length;
	    ifaddr->type = info->h_addrtype;
	    if (ifaddr->len > sizeof(ifaddr->ifaddr)) {
		/* If the address won't fit in the field, reset to
		   no address */
		ifaddr->len = 0;
		ifaddr->type = -1;
	    }
	    else
		MPIU_Memcpy( ifaddr->ifaddr, info->h_addr_list[0], ifaddr->len );
	}
    }

    return 0;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDU_Sock_ifaddr_t ifaddr;
    char ifname[MAX_HOST_DESCRIPTION_LEN];
    int ret;
    struct sockaddr_in sock_id;
    socklen_t len;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_GET_BUSINESS_CARD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_GET_BUSINESS_CARD);
    
    mpi_errno = GetSockInterfaceAddr(my_rank, ifname, sizeof(ifname), &ifaddr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    
    mpi_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_HOST_DESCRIPTION_KEY, ifname);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
        if (mpi_errno == MPIU_STR_NOMEM)
        {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        }
        else
        {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
    }

    len = sizeof(sock_id);
    ret = getsockname (MPID_nem_tcp_g_lstn_sc.fd, (struct sockaddr *)&sock_id, &len);
    MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**getsockname", "**getsockname %s", MPIU_Strerror (errno));

    mpi_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, ntohs(sock_id.sin_port));
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
        if (mpi_errno == MPIU_STR_NOMEM)
        {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        }
        else
        {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
    }
    
    if (ifaddr.len > 0 && ifaddr.type == AF_INET)
    {
        unsigned char *p;
        p = (unsigned char *)(ifaddr.ifaddr);
        MPIU_Snprintf( ifname, sizeof(ifname), "%u.%u.%u.%u", p[0], p[1], p[2], p[3] );
        MPIU_DBG_MSG_S(CH3_CONNECT,VERBOSE,"ifname = %s",ifname );
        mpi_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_IFNAME_KEY, ifname);
        if (mpi_errno != MPIU_STR_SUCCESS)
        {
            if (mpi_errno == MPIU_STR_NOMEM)
            {
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
            }
            else
            {
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
            }
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
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_CONNECT_TO_ROOT);

    /* vc is already allocated before reaching this point */

    mpi_errno = MPID_nem_tcp_get_addr_port_from_bc(business_card, &addr, &(VC_FIELD(new_vc, sock_id).sin_port));
    VC_FIELD(new_vc, sock_id).sin_addr.s_addr = addr.s_addr;
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIDI_GetTagFromPort(business_card, &new_vc->port_name_tag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPID_nem_tcp_connect(new_vc); 

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
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_VC_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_VC_INIT);

    vc_ch->state = MPID_NEM_TCP_VC_STATE_DISCONNECTED;
    
    vc->sendNoncontig_fn      = MPID_nem_tcp_SendNoncontig;
    vc_ch->iStartContigMsg    = MPID_nem_tcp_iStartContigMsg;
    vc_ch->iSendContig        = MPID_nem_tcp_iSendContig;
    memset(&VC_FIELD(vc, sock_id), 0, sizeof(VC_FIELD(vc, sock_id)));
    VC_FIELD(vc, sock_id).sin_family = AF_INET;

    vc_ch->next = NULL;
    vc_ch->prev = NULL;

    ASSIGN_SC_TO_VC(vc, NULL);
    VC_FIELD(vc, send_queue).head = VC_FIELD(vc, send_queue).tail = NULL;

    VC_FIELD(vc, sc_ref_count) = 0;

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
#if 0
    struct pollfd *plfd;
    sockconn_t *sc;

    sc = VC_FIELD(vc, sc);
    if (sc == NULL)
        goto fn_exit;

    plfd = &MPID_nem_tcp_plfd_tbl[sc->index]; 
#endif

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
    int port, low_port, high_port;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_TCP_BIND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_TCP_BIND);
   
    low_port = 0;
    high_port = 0;

/*     fprintf(stdout, FCNAME " Enter\n"); fflush(stdout); */
    MPIU_GetEnvRange( "MPICH_PORT_RANGE", &low_port, &high_port );
    MPIU_ERR_CHKANDJUMP (low_port < 0 || low_port > high_port, mpi_errno, MPI_ERR_OTHER, "**badportrange");

    /* if MPICH_PORT_RANGE is not set, low_port and high_port are 0 so bind will use any available port */
    ret = 0;
    for (port = low_port; port <= high_port; ++port)
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_tcp_vc_terminate (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_NEM_TCP_VC_TERMINATE);

    MPIDI_FUNC_ENTER(MPID_NEM_TCP_VC_TERMINATE);

    mpi_errno = MPID_nem_tcp_cleanup(vc);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_NEM_TCP_VC_TERMINATE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

