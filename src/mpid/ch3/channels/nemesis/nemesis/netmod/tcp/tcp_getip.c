/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* These includes are here because they're used just for getting the interface
 *   names
 */

#include "../../../include/mpidi_ch3i_nemesis_conf.h"

/* FIXME: This should use the standard debug/logging routines and macros */
static int dbg_ifname = 0;

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

#ifdef USE_ALL_SOURCE_FOR_IFCONF
/* This is a very special case.  Allow the use of some extensions for just
   the rest of this file so that we can get the ifconf structure 
   This is needed for AIX.
 */
#define _ALL_SOURCE
#endif

#include "tcp_impl.h"
#include <sys/types.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
/* Needed for SIOCGIFCONF on various flavors of unix */
#include <sys/sockio.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
/* Needed for SIOCGIFCONF on linux */
#include <sys/ioctl.h>
#endif
/* needs sys/types.h first */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

/* We can only access the interfaces if we have a number of features.
   Test for these, otherwise define this routine to return false in the
   "found" variable */
#if defined(SIOCGIFCONF) && defined(HAVE_STRUCT_IFCONF)
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define NUM_IFREQS 10

#undef FUNCNAME
#define FUNCNAME MPIDI_GetIPInterface
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_GetIPInterface( MPIDU_Sock_ifaddr_t *ifaddr, int *found )
{
    int mpi_errno = MPI_SUCCESS;
    char *buf_ptr = NULL, *ptr;
    int buf_len, buf_len_prev;
    int fd;
    MPIDU_Sock_ifaddr_t myifaddr;
    int nfound = 0, foundLocalhost = 0;
    /* We predefine the LSB and MSB localhost addresses */
    unsigned int localhost = 0x0100007f;
#ifdef WORDS_BIGENDIAN
    unsigned int MSBlocalhost = 0x7f000001;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_GETIPINTERFACE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_GETIPINTERFACE);
    *found = 0;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
	MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sock_create");
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
	    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d", buf_len);
	}
	
	ifconf.ifc_buf = buf_ptr;
	ifconf.ifc_len = buf_len;

	rc = ioctl(fd, SIOCGIFCONF, &ifconf);
	if (rc < 0) {
	    if (errno != EINVAL || buf_len_prev != 0) {
		MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**ioctl", "**ioctl %d %s", errno, MPIU_Strerror(errno));
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

    /* If we found a unique address, use that */
    if (nfound == 1 || (nfound == 0 && foundLocalhost == 1)) {
	*ifaddr = myifaddr;
	*found  = 1;
    }

fn_exit:
    if (NULL != buf_ptr)
        MPIU_Free(buf_ptr);
    if (fd >= 0)
        close(fd);
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_GETIPINTERFACE);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#else /* things needed to find the interfaces */

/* In this case, just return false for interfaces found */
int MPIDI_GetIPInterface( MPIDU_Sock_ifaddr_t *ifaddr, int *found )
{
    *found = 0;
    return MPI_SUCCESS;
}
#endif


#if defined(SIOCGIFADDR)

/* 'ifname' is a string that might be the name of an interface (e.g., "eth0",
 * "en1", "ib0", etc), not an "interface hostname" ("node1", "192.168.1.38",
 * etc).  If that name is detected as a valid interface name then 'ifaddr' will
 * be populated with the corresponding address and '*found' will be set to TRUE.
 * Otherwise '*found' will be set to FALSE. */
#undef FUNCNAME
#define FUNCNAME MPIDI_Get_IP_for_iface
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_Get_IP_for_iface(const char *ifname, MPIDU_Sock_ifaddr_t *ifaddr, int *found)
{
    int mpi_errno = MPI_SUCCESS;
    int fd, ret;
    struct ifreq ifr;

    if (found != NULL)
        *found = FALSE;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    MPIU_ERR_CHKANDJUMP2(fd < 0, mpi_errno, MPI_ERR_OTHER, "**sock_create", "**sock_create %s %d", MPIU_Strerror(errno), errno);
    ifr.ifr_addr.sa_family = AF_INET; /* just IPv4 for now */
    MPIU_Strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);
    ret = ioctl(fd, SIOCGIFADDR, &ifr);
    MPIU_ERR_CHKANDJUMP2(ret < 0, mpi_errno, MPI_ERR_OTHER, "**ioctl", "**ioctl %d %s", errno, MPIU_Strerror(errno));

    *found = TRUE;
    MPIU_Memcpy(ifaddr->ifaddr, &((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr, 4);
    ifaddr->len = 4;
    ifaddr->type = AF_INET;

fn_exit:
    if (fd != -1) {
        ret = close(fd);
        if (ret < 0) {
            MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**sock_close", "**sock_close %s %d", MPIU_Strerror(errno), errno);
        }
    }
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#else /* things needed to determine address for a given interface name */

#undef FUNCNAME
#define FUNCNAME MPIDI_Get_IP_for_iface
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_Get_IP_for_iface(const char *ifname, MPIDU_Sock_ifaddr_t *ifaddr, int *found)
{
    if (found != NULL)
        *found = FALSE;
}

#endif
