/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* We need to include the conf file first so that we can use
   the _SVID_SOURCE if needed before any file includes features.h 
   on GNU systems */
#include "mpichconf.h"


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

#include "mpidi_ch3_impl.h"

#include <stdlib.h>

#ifdef HAVE_NETDB_H
 #include <netdb.h>
#endif

/* We set dbg_ifname to 1 to help debug the choice of interface name 
   used when determining which interface to advertise to other processes.
   The value is -1 if it has not yet been set.
 */
static int dbg_ifname = -1;

static int MPIDI_CH3U_GetIPInterface( MPIDU_Sock_ifaddr_t *, int * );

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

#undef FUNCNAME
#define FUNCNAME MPIDU_CH3U_GetSockInterfaceAddr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_CH3U_GetSockInterfaceAddr( int myRank, char *ifname, int maxIfname,
				     MPIDU_Sock_ifaddr_t *ifaddr )
{
    char *ifname_string;
    int mpi_errno = MPI_SUCCESS;
    int ifaddrFound = 0;

    if (dbg_ifname < 0) {
	int rc;
	rc = MPL_env2bool( "MPICH_DBG_IFNAME", &dbg_ifname );
	if (rc != 1) dbg_ifname = 0;
    }

    /* Set "not found" for ifaddr */
    ifaddr->len = 0;

    /* Check for the name supplied through an environment variable */
    ifname_string = getenv("MPICH_INTERFACE_HOSTNAME");

    if (!ifname_string) {
	/* See if there is a per-process name for the interfaces (e.g.,
	   the process manager only delievers the same values for the 
	   environment to each process */
	char namebuf[1024];
	MPL_snprintf( namebuf, sizeof(namebuf), 
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
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	ifname_string = ifname;

	/* If we didn't find a specific name, then try to get an IP address
	   directly from the available interfaces, if that is supported on
	   this platform.  Otherwise, we'll drop into the next step that uses 
	   the ifname */
	mpi_errno = MPIDI_CH3U_GetIPInterface( ifaddr, &ifaddrFound );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
	/* Copy this name into the output name */
	MPIU_Strncpy( ifname, ifname_string, maxIfname );
    }

    /* If we don't have an IP address, try to get it from the name */
    if (!ifaddrFound) {
	struct hostent *info;
	/* printf( "Name to check is %s\n", ifname_string ); fflush(stdout); */
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
	    else {
		MPIU_Memcpy( ifaddr->ifaddr, info->h_addr_list[0], ifaddr->len );
	    }
	}
    }

fn_exit:
    return mpi_errno;
fn_fail:
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

static int MPIDI_CH3U_GetIPInterface( MPIDU_Sock_ifaddr_t *ifaddr, int *found )
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

    if (dbg_ifname < 0) {
	int rc;
	rc = MPL_env2bool( "MPICH_DBG_IFNAME", &dbg_ifname );
	if (rc != 1) dbg_ifname = 0;
    }

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
	    fprintf( stdout, "%10s\t", ifreq->ifr_name ); fflush(stdout);
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
static int MPIDI_CH3U_GetIPInterface( MPIDU_Sock_ifaddr_t *ifaddr, int *found )
{
    *found = 0;
    return 0;
}
#endif
