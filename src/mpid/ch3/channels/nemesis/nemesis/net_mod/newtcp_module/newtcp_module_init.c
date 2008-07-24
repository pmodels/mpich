/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*S
  MPIDU_Sock_ifaddr_t - Structure to hold an Internet address.

+ len - Length of the address.  4 for IPv4, 16 for IPv6.
- ifaddr - Address bytes (as bytes, not characters)

S*/
typedef struct MPIDU_Sock_ifaddr_t {
    int len, type;
    unsigned char ifaddr[16];
} MPIDU_Sock_ifaddr_t;


MPID_nem_queue_ptr_t MPID_nem_newtcp_module_free_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_free_queue = 0;
extern sockconn_t g_lstn_sc;
extern pollfd_t g_lstn_plfd;
extern pollfd_t *MPID_nem_newtcp_module_plfd_tbl;

static MPID_nem_queue_t _free_queue;

static int dbg_ifname = 0;

#if 0
static int GetIPInterface( MPIDU_Sock_ifaddr_t *, int * );
#endif

#define MPIDI_CH3I_PORT_KEY "port"
#define MPIDI_CH3I_HOST_DESCRIPTION_KEY "description"
#define MPIDI_CH3I_IFNAME_KEY "ifname"

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_init (MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue,
                                 MPID_nem_cell_ptr_t proc_elements, int num_proc_elements, MPID_nem_cell_ptr_t module_elements,
                                 int num_module_elements, MPID_nem_queue_ptr_t *module_free_queue,
                                 int ckpt_restart, MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_INIT);

    /* first make sure that our private fields in the vc fit into the area provided  */
    MPIU_Assert(sizeof(MPID_nem_newtcp_module_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);
    
    /* set up listener socket */
/*     fprintf(stdout, FCNAME " Enter\n"); fflush(stdout); */
    g_lstn_plfd.fd = g_lstn_sc.fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    MPIU_ERR_CHKANDJUMP2 (g_lstn_sc.fd == -1, mpi_errno, MPI_ERR_OTHER, "**sock_create", "**sock_create %s %d", strerror (errno), errno);

    mpi_errno = MPID_nem_newtcp_module_set_sockopts (g_lstn_sc.fd);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    g_lstn_plfd.events = POLLIN;
    mpi_errno = MPID_nem_newtcp_module_bind (g_lstn_sc.fd);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    ret = listen (g_lstn_sc.fd, SOMAXCONN);	      
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**listen", "**listen %s %d", errno, strerror (errno));  
    g_lstn_sc.state.lstate = LISTEN_STATE_LISTENING;
    g_lstn_sc.handler = MPID_nem_newtcp_module_state_listening_handler;

    /* create business card */
    mpi_errno = MPID_nem_newtcp_module_get_business_card (pg_rank, bc_val_p, val_max_sz_p);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* save references to queues */
    MPID_nem_process_recv_queue = proc_recv_queue;
    MPID_nem_process_free_queue = proc_free_queue;

    MPID_nem_newtcp_module_free_queue = &_free_queue;

    /* set up network module queues */
    MPID_nem_queue_init (MPID_nem_newtcp_module_free_queue);

    for (i = 0; i < num_module_elements; ++i)
    {
        MPID_nem_queue_enqueue (MPID_nem_newtcp_module_free_queue, &module_elements[i]);
    }

    *module_free_queue = MPID_nem_newtcp_module_free_queue;


    MPID_nem_newtcp_module_init_sm();
    MPID_nem_newtcp_module_send_init();
    MPID_nem_newtcp_module_poll_init();


 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_INIT);
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
		memcpy( ifaddr->ifaddr, info->h_addr_list[0], ifaddr->len );
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
    MPIDU_Sock_ifaddr_t ifaddr;
    char ifname[MAX_HOST_DESCRIPTION_LEN];
    int ret;
    struct sockaddr_in sock_id;
    socklen_t len;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_GET_BUSINESS_CARD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_GET_BUSINESS_CARD);
    
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
    ret = getsockname (g_lstn_sc.fd, (struct sockaddr *)&sock_id, &len);
    MPIU_ERR_CHKANDJUMP1 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**getsockname", "**getsockname %s", strerror (errno));

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

    {
	char ifname[256];
	unsigned char *p;
	if (ifaddr.len > 0 && ifaddr.type == AF_INET)
        {
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
    int port_name_tag;
    struct in_addr addr;
    int ret;
    in_port_t port;
    
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
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_vc_init (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    struct in_addr addr;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_VC_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_VC_INIT);

    /*     fprintf(stdout, FCNAME " Enter\n"); fflush(stdout); */
    vc_ch->state = MPID_NEM_NEWTCP_MODULE_VC_STATE_DISCONNECTED;
    
    vc->sendNoncontig_fn      = MPID_nem_newtcp_SendNoncontig;
    vc_ch->iStartContigMsg    = MPID_nem_newtcp_iStartContigMsg;
    vc_ch->iSendContig        = MPID_nem_newtcp_iSendContig;
    memset(&VC_FIELD(vc, sock_id), 0, sizeof(VC_FIELD(vc, sock_id)));
    VC_FIELD(vc, sock_id).sin_family = AF_INET;

    vc_ch->next = NULL;
    vc_ch->prev = NULL;

    ASSIGN_SC_TO_VC(vc, NULL);
    VC_FIELD(vc, send_queue).head = VC_FIELD(vc, send_queue).tail = NULL;

 fn_exit:
    /*     fprintf(stdout, FCNAME " Exit\n"); fflush(stdout); */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_VC_INIT);
    return mpi_errno;
 fn_fail:
    /*     fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno); */
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;

    /* currently do nothing */
    pollfd_t *plfd;
    sockconn_t *sc;

    sc = VC_FIELD(vc, sc);
    if (sc == NULL)
        goto fn_exit;

    plfd = &MPID_nem_newtcp_module_plfd_tbl[sc->index]; 

 fn_exit:   
       return mpi_errno;
 fn_fail:
       goto fn_exit;
}


/* 
   FIXME: this is the same function as in socksm.c 
   This should be removed and use only one function eventually.
*/
   
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_get_addr_port_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_get_addr_port_from_bc(const char *business_card, struct in_addr *addr, in_port_t *port)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int port_int;
    char desc_str[256];
    char ifname[256];
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_GET_ADDR_PORT_FROM_BC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_GET_ADDR_PORT_FROM_BC);
    
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
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_GET_ADDR_PORT_FROM_BC);
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
int MPID_nem_newtcp_module_bind (int sockfd)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    struct sockaddr_in sin;
    int port, low_port, high_port;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_BIND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_BIND);
   
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
        MPIU_ERR_CHKANDJUMP3 (errno != EADDRINUSE && errno != EADDRNOTAVAIL, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", port, errno, strerror (errno));
    }
    /* check if an available port was found */
    MPIU_ERR_CHKANDJUMP3 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock|poll|bind", "**sock|poll|bind %d %d %s", port, errno, strerror (errno));

 fn_exit:
/*     if (ret == 0) */
/*         fprintf(stdout, "sockfd=%d  port=%d bound\n", sockfd, port); */
/*     fprintf(stdout, FCNAME " Exit\n"); fflush(stdout); */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_BIND);
    return mpi_errno;
 fn_fail:
/*     fprintf(stdout, "failure. mpi_errno = %d\n", mpi_errno); */
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_vc_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_vc_terminate (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS, rc;
    sockconn_t *sc;

    MPIDI_FUNC_ENTER(MPID_NEM_NEWTCP_MODULE_VC_TERMINATE);

    mpi_errno = MPID_nem_newtcp_module_cleanup(vc);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_NEM_NEWTCP_MODULE_VC_TERMINATE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}


#if 0
/* These includes are here because they're used just for getting the interface
 *   names
 */

/* FIXME: This more-or-less duplicates the code in 
   ch3/util/sock/ch3u_getintefaces.c , which is already more thoroughly 
   tested and more portable than the code here.
 */
#ifdef USE_NOPOSIX_FOR_IFCONF
/* This is a very special case.  Allow the use of some extensions for 
   just the rest of this file so that we can get the ifconf structure */
#undef _POSIX_C_SOURCE
#endif

#ifdef USE_SVIDSOURCE_FOR_IFCONF
/* This is a very special case.  Allow the use of some extensions for just
   the rest of this file so that we can get the ifconf structure */
#define _SVID_SOURCE
#endif

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
		    memcpy( myifaddr.ifaddr, &addr.s_addr, 4 );
		}
	    }
	    else {
		nfound++;
		myifaddr.type = AF_INET;
		myifaddr.len  = 4;
		memcpy( myifaddr.ifaddr, &addr.s_addr, 4 );
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
    return 0;
}
#endif
#endif
