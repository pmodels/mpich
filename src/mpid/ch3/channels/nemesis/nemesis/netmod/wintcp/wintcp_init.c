/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "wintcp_impl.h"
#ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
    #include <arpa/inet.h>
#endif

/*S
  MPIDU_Sock_ifaddr_t - Structure to hold an Internet address.

+ len - Length of the address.  4 for IPv4, 16 for IPv6.
- ifaddr - Address bytes (as bytes, not characters)

S*/
typedef struct MPIDU_Sock_ifaddr_t {
    int len, type;
    unsigned char ifaddr[16];
} MPIDU_Sock_ifaddr_t;

/* FIXME: Move all externs to say socksm_globals.h */
extern MPIU_ExSetHandle_t MPID_nem_newtcp_module_ex_set_hnd;

extern sockconn_t MPID_nem_newtcp_module_g_lstn_sc;
extern struct pollfd g_lstn_plfd;

static MPID_nem_queue_t _free_queue;

static int dbg_ifname = 0;

static int get_addr_port_from_bc (const char *business_card, struct in_addr *addr, in_port_t *port);
static int GetIPInterface( MPIDU_Sock_ifaddr_t *, int * );

MPID_nem_netmod_funcs_t MPIDI_nem_newtcp_module_funcs = {
    MPID_nem_newtcp_module_init,
    MPID_nem_newtcp_module_finalize,
    MPID_nem_newtcp_module_poll,
    MPID_nem_newtcp_module_get_business_card,
    MPID_nem_newtcp_module_connect_to_root,
    MPID_nem_newtcp_module_vc_init,
    MPID_nem_newtcp_module_vc_destroy,
    MPID_nem_newtcp_module_vc_terminate
};

#define MPIDI_CH3I_PORT_KEY "port"
#define MPIDI_CH3I_HOST_DESCRIPTION_KEY "description"
#define MPIDI_CH3I_IFNAME_KEY "ifname"

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_init (MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_INIT);

    MPIU_UNREFERENCED_ARG(pg_p);

    /* first make sure that our private fields in the vc fit into the area provided  */
    MPIU_Assert(sizeof(MPID_nem_newtcp_module_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);
    
    /* Initialize sock wrapper */
    mpi_errno = MPIU_SOCKW_Init();
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    /* Initialize the Executive progress engine */
    mpi_errno = MPIU_ExInitialize();
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    /* Create an Executive set */
    /* FIXME: All MPI util funcs should return an MPI error code */
    MPID_nem_newtcp_module_ex_set_hnd = MPIU_ExCreateSet();
    MPIU_ERR_CHKANDJUMP(MPID_nem_newtcp_module_ex_set_hnd == MPIU_EX_INVALID_SET,
                            mpi_errno, MPI_ERR_OTHER, "**ex_create_set"); 

    /* Setup listen socket */
    mpi_errno = MPIU_SOCKW_Sock_open(AF_INET, SOCK_STREAM, IPPROTO_TCP, &(MPID_nem_newtcp_module_g_lstn_sc.fd));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPID_nem_newtcp_module_set_sockopts(MPID_nem_newtcp_module_g_lstn_sc.fd);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPID_nem_newtcp_module_bind (MPID_nem_newtcp_module_g_lstn_sc.fd);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    mpi_errno = MPIU_SOCKW_Listen(MPID_nem_newtcp_module_g_lstn_sc.fd, SOMAXCONN);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Associate the listen sock with the newtcp module executive set 
     * - Key 0 command processor */
    /* FIXME: No error code returned ! */
    MPIU_ExAttachHandle(MPID_nem_newtcp_module_ex_set_hnd, MPIU_EX_WIN32_COMP_PROC_KEY, (HANDLE )MPID_nem_newtcp_module_g_lstn_sc.fd);
    
    /* Register the listening handlers with executive */
    /* We also need to post the first accept here */

    MPID_nem_newtcp_module_g_lstn_sc.state = LISTEN_STATE_LISTENING;
    /* FIXME: WINTCP ASYNC
    MPID_nem_newtcp_module_g_lstn_sc.handler = MPID_nem_newtcp_module_state_listening_handler;
    */

    /* create business card */
    mpi_errno = MPID_nem_newtcp_module_get_business_card (pg_rank, bc_val_p, val_max_sz_p);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    mpi_errno = MPID_nem_newtcp_module_sm_init();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_nem_newtcp_module_send_init();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_nem_newtcp_module_poll_init();
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_INIT);
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
	if (dbg_ifname && ifname_string) {
	    fprintf( stdout, "Found interface name %s from %s\n", 
		    ifname_string, namebuf );
	    fflush( stdout );
	}
    }
    else if (dbg_ifname) {
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
	mpi_errno = GetIPInterface( ifaddr, &ifaddrFound );
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
#define FUNCNAME MPID_nem_newtcp_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPIU_STR_SUCCESS;
    MPIDU_Sock_ifaddr_t ifaddr;
    char ifname[MAX_HOST_DESCRIPTION_LEN];
    int ret;
    struct sockaddr_in sock_id;
    socklen_t len;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_GET_BUSINESS_CARD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_GET_BUSINESS_CARD);
    
    mpi_errno = GetSockInterfaceAddr(my_rank, ifname, sizeof(ifname), &ifaddr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    
    
    str_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_HOST_DESCRIPTION_KEY, ifname);
    if (str_errno) {
        MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }

    len = sizeof(sock_id);
    ret = getsockname (MPID_nem_newtcp_module_g_lstn_sc.fd, (struct sockaddr *)&sock_id, &len);
    MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**getsockname", "**getsockname %s", strerror (errno));

    str_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, sock_id.sin_port);
    if (str_errno) {
        MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }

    {
	char ifname[256];
	unsigned char *p;
	if (ifaddr.len > 0 && ifaddr.type == AF_INET)
        {
	    p = (unsigned char *)(ifaddr.ifaddr);
	    MPIU_Snprintf( ifname, sizeof(ifname), "%u.%u.%u.%u", p[0], p[1], p[2], p[3] );
	    MPIU_DBG_MSG_S(CH3_CONNECT,VERBOSE,"ifname = %s",ifname );
	    str_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_IFNAME_KEY, ifname);
	    if (str_errno) {
                MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
            }
	}
    }

    /*     printf("MPID_nem_newtcp_module_get_business_card. port=%d\n", sock_id.sin_port); */

 fn_exit:
/*     fprintf(stdout, "MPID_nem_newtcp_module_get_business_card Exit, mpi_errno=%d\n", mpi_errno); fflush(stdout); */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_GET_BUSINESS_CARD);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    struct in_addr addr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONNECT_TO_ROOT);

    /* vc is already allocated before reaching this point */

    mpi_errno = MPID_nem_newtcp_module_get_addr_port_from_bc(business_card, &addr, &(VC_FIELD(new_vc, sock_id).sin_port));
    VC_FIELD(new_vc, sock_id).sin_addr.s_addr = addr.s_addr;
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIDI_GetTagFromPort(business_card, &new_vc->port_name_tag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPID_nem_newtcp_module_connect(new_vc);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONNECT_TO_ROOT);
    return mpi_errno;

 fn_fail:
    goto fn_exit;}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_vc_init (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_VC_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_VC_INIT);

    VC_FIELD(vc, state) = MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED;
    
    vc->sendNoncontig_fn      = MPID_nem_newtcp_SendNoncontig;
    vc_ch->iStartContigMsg    = MPID_nem_newtcp_iStartContigMsg;
    vc_ch->iSendContig        = MPID_nem_newtcp_iSendContig;
    memset(&VC_FIELD(vc, sock_id), 0, sizeof(VC_FIELD(vc, sock_id)));
    VC_FIELD(vc, sock_id).sin_family = AF_INET;
    
    vc_ch->next = NULL;
    vc_ch->prev = NULL;
    VC_FIELD(vc, sc) = NULL;
    VC_FIELD(vc, sc_ref_count) = 0; 
    VC_FIELD(vc, send_queue).head = VC_FIELD(vc, send_queue).tail = NULL;
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_VC_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;   

    /* free any resources associated with this VC here */
    MPIU_UNREFERENCED_ARG(vc);

       return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_get_addr_port_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_get_addr_port_from_bc (const char *business_card, struct in_addr *addr, in_port_t *port)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    /* char desc_str[256]; */
    char ifname[256];
    MPIDI_STATE_DECL(MPID_STATE_NEWTCP_MODULE_GET_ADDR_PORT_FROM_BC);

    MPIDI_FUNC_ENTER(MPID_STATE_NEWTCP_MODULE_GET_ADDR_PORT_FROM_BC);
    
    /* desc_str is only used for debugging
    ret = MPIU_Str_get_string_arg (business_card, MPIDI_CH3I_HOST_DESCRIPTION_KEY, desc_str, sizeof(desc_str));
    MPIU_ERR_CHKANDJUMP (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
    */

    ret = MPIU_Str_get_int_arg (business_card, MPIDI_CH3I_PORT_KEY, (int *)port);
    MPIU_ERR_CHKANDJUMP (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missingport");
    /*     fprintf(stdout, "get_addr_port_from_bc buscard=%s  desc=%s port=%d\n",business_card, desc_str, *port); fflush(stdout); */

    ret = MPIU_Str_get_string_arg(business_card, MPIDI_CH3I_IFNAME_KEY, ifname, sizeof(ifname));
    MPIU_ERR_CHKANDJUMP (ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missingifname");
	
    /*
    ret = inet_pton (AF_INET, (const char *)ifname, addr);
    MPIU_ERR_CHKANDJUMP(ret == 0, mpi_errno,MPI_ERR_OTHER,"**ifnameinvalid");
    MPIU_ERR_CHKANDJUMP(ret < 0, mpi_errno, MPI_ERR_OTHER, "**afinetinvalid");
    */
    mpi_errno = MPIU_SOCKW_Inet_addr(ifname, &(addr->s_addr));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_NEWTCP_MODULE_GET_ADDR_PORT_FROM_BC);
    return mpi_errno;
 fn_fail:
/*     fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno); */
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

/* MPID_nem_newtcp_module_bind -- if MPICH_PORT_RANGE is set, this
   binds the socket to an available port number in the range.
   Otherwise, it binds it to any addr and any port */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_bind
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_bind (MPIU_SOCKW_Sockfd_t sockfd)
{
    int mpi_errno = MPI_SUCCESS;
    struct sockaddr_in sin;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_BIND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_BIND);
   
    MPIU_ERR_CHKANDJUMP(MPIR_PARAM_PORT_RANGE.low < 0 || MPIR_PARAM_PORT_RANGE.low > MPIR_PARAM_PORT_RANGE.high, mpi_errno, MPI_ERR_OTHER, "**badportrange");

    memset((void *)&sin, 0, sizeof(sin));
    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    mpi_errno = MPIU_SOCKW_Bind_port_range(sockfd, &sin, MPIR_PARAM_PORT_RANGE.low, MPIR_PARAM_PORT_RANGE.high);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
/*     if (ret == 0) */
/*         fprintf(stdout, "sockfd=%d  port=%d bound\n", sockfd, port); */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_BIND);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_vc_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_vc_terminate (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_NEM_NEWTCP_MODULE_VC_TERMINATE);

    MPIDI_FUNC_ENTER(MPID_NEM_NEWTCP_MODULE_VC_TERMINATE);

    mpi_errno = MPID_nem_newtcp_module_cleanup(vc);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_NEM_NEWTCP_MODULE_VC_TERMINATE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}


/* These includes are here because they're used just for getting the interface
 *   names
 */


#include <sys/types.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
/* Needed for SIOCGIFCONF */
#include <sys/sockio.h>
#endif

#if defined(SIOCGIFCONF) && defined(HAVE_STRUCT_IFCONF)
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>

/* We can only access the interfaces if we have a number of features.
   Test for these, otherwise define this routine to return false in the
   "found" variable */

#define NUM_IFREQS 10

static int GetIPInterface( MPIDU_Sock_ifaddr_t *ifaddr, int *found )
{
    char *buf_ptr, *ptr;
    int buf_len, buf_len_prev;
    int fd;
    MPIDU_Sock_ifaddr_t myifaddr;
    int nfound = 0, foundLocalhost = 0;
    /* We predefine the LSB and MSB localhost addresses */
    unsigned int localhost = 0x0100007f;
#ifdef WORDS_BIGENDIAN
    unsigned int MSBlocalhost = 0x7f000001;
#endif

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
	fprintf( stderr, "Unable to open an AF_INET socket\n" );
	return 1;
    }

    /* Use MSB localhost if necessary */
#ifdef WORDS_BIGENDIAN
    localhost = MSBlocalhost;
#endif
    

    /*
     * Obtain the interface information from the operating system
     *
     * Note: much of this code is borrowed from W. Richard Stevens' book
     * entitled "UNIX Network Programming", Volume 1, Second Edition.  See
     * section 16.6 for details.
     */
    buf_len = NUM_IFREQS * sizeof(struct ifreq);
    buf_len_prev = 0;

    for(;;)
    {
	struct ifconf			ifconf;
	int				rc;

	buf_ptr = (char *) MPIU_Malloc(buf_len);
	if (buf_ptr == NULL) {
	    fprintf( stderr, "Unable to allocate %d bytes\n", buf_len );
	    return 1;
	}
	
	ifconf.ifc_buf = buf_ptr;
	ifconf.ifc_len = buf_len;

	rc = ioctl(fd, SIOCGIFCONF, &ifconf);
	if (rc < 0) {
	    if (errno != EINVAL || buf_len_prev != 0) {
		fprintf( stderr, "Error from ioctl = %d\n", errno );
		perror(" Error is: ");
		return 1;
	    }
	}
        else {
	    if (ifconf.ifc_len == buf_len_prev) {
		buf_len = ifconf.ifc_len;
		break;
	    }

	    buf_len_prev = ifconf.ifc_len;
	}
	
	MPIU_Free(buf_ptr);
	buf_len += NUM_IFREQS * sizeof(struct ifreq);
    }
	
    /*
     * Now that we've got the interface information, we need to run through
     * the interfaces and check out the ip addresses.  If we find a
     * unique, non-lcoal host (127.0.0.1) address, return that, otherwise
     * return nothing.
     */
    ptr = buf_ptr;

    while(ptr < buf_ptr + buf_len) {
	struct ifreq *			ifreq;

	ifreq = (struct ifreq *) ptr;

	if (dbg_ifname) {
	    fprintf( stdout, "%10s\t", ifreq->ifr_name );
	}
	
	if (ifreq->ifr_addr.sa_family == AF_INET) {
	    struct in_addr		addr;

	    addr = ((struct sockaddr_in *) &(ifreq->ifr_addr))->sin_addr;
	    if (dbg_ifname) {
		fprintf( stdout, "IPv4 address = %08x (%s)\n", addr.s_addr, 
			 inet_ntoa( addr ) );
	    }

	    if (addr.s_addr == localhost && dbg_ifname) {
		fprintf( stdout, "Found local host\n" );
	    }
	    /* Save localhost if we find it.  Let any new interface 
	       overwrite localhost.  However, if we find more than 
	       one non-localhost interface, then we'll choose none for the 
	       interfaces */
	    if (addr.s_addr == localhost) {
		foundLocalhost = 1;
		if (nfound == 0) {
		    myifaddr.type = AF_INET;
		    myifaddr.len  = 4;
		    MPIU_Memcpy( myifaddr.ifaddr, &addr.s_addr, 4 );
		}
	    }
	    else {
		nfound++;
		myifaddr.type = AF_INET;
		myifaddr.len  = 4;
		MPIU_Memcpy( myifaddr.ifaddr, &addr.s_addr, 4 );
	    }
	}
	else {
	    if (dbg_ifname) {
		fprintf( stdout, "\n" );
	    }
	}

	/*
	 *  Increment pointer to the next ifreq; some adjustment may be
	 *  required if the address is an IPv6 address
	 */
	/* This is needed for MAX OSX */
#ifdef _SIZEOF_ADDR_IFREQ
	ptr += _SIZEOF_ADDR_IFREQ(*ifreq);
#else
	ptr += sizeof(struct ifreq);
	
#	if defined(AF_INET6)
	{
	    if (ifreq->ifr_addr.sa_family == AF_INET6)
	    {
		ptr += sizeof(struct sockaddr_in6) - sizeof(struct sockaddr);
	    }
	}
#	endif
#endif
    }

    MPIU_Free(buf_ptr);
    close(fd);
    
    /* If we found a unique address, use that */
    if (nfound == 1 || (nfound == 0 && foundLocalhost == 1)) {
	*ifaddr = myifaddr;
	*found  = 1;
    }
    else {
	*found  = 0;
    }

    return 0;
}

#else /* things needed to find the interfaces */

/* In this case, just return false for interfaces found */
static int GetIPInterface( MPIDU_Sock_ifaddr_t *ifaddr, int *found )
{
    *found = 0;
    MPIU_UNREFERENCED_ARG(ifaddr);
    return 0;
}
#endif
