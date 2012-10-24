/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define MAX_NUM_NICS 16

/*
 * FIXME: This is an archived file that captures how to find available 
 * interfaces on a node in a cluster.
 */
#ifdef HAVE_WINDOWS_H

static int GetLocalIPs(int32_t *pIP, int max)
{
    char hostname[100], **hlist;
    struct hostent *h = NULL;
    int n = 0;

    MPID_Get_processor_name( hostname, sizeof(hostname), 0 );

    h = gethostbyname(hostname);
    if (h == NULL)
    {
	return 0;
    }
    
    hlist = h->h_addr_list;
    while (*hlist != NULL && n<max)
    {
	pIP[n] = *(int32_t*)(*hlist);

	/*{	
	unsigned int a, b, c, d;
	a = ((unsigned char *)(&pIP[n]))[0];
	b = ((unsigned char *)(&pIP[n]))[1];
	c = ((unsigned char *)(&pIP[n]))[2];
	d = ((unsigned char *)(&pIP[n]))[3];
	MPIU_DBG_PRINTF(("ip: %u.%u.%u.%u\n", a, b, c, d));
	}*/

	hlist++;
	n++;
    }
    return n;
}

#else /* HAVE_WINDOWS_H */

#define NUM_IFREQS 10
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_NET_IF_H
#ifdef __STRICT_ANSI__
/* FIXME: THIS IS WRONG.  INSTEAD, SPECIFY THE SPECIFIC FEATURE LEVEL
   NEEDED, AND THEN ONLY IF A CONFIGURE TEST SHOWS THAT IT IS REQUIRED
   THESE NEED TO BE SET FOR ALL COMPILATIONS TO AVOID HAVING DIFFERENT
   FILES COMPILED WITH DIFFERENT AND INCOMPATIBLE HEADER FILES.

   WHAT IS APPARENTLY NEEDED (SEE /usr/include/features.h ON A LINUX 
   SYSTEM) IS EITHER _BSD_SOURCE OR _SVID_SOURCE; THIS MUST BE SET
   CONSISTENTLY FOR ALL FILES COMPILED AS PART OF MPICH
 */
#define __USE_MISC /* This must be defined to get struct ifreq defined */
#endif
#include <net/if.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

static int GetLocalIPs(int32_t *pIP, int max)
{
    int					fd;
    char *				buf_ptr;
    int					buf_len;
    int					buf_len_prev;
    char *				ptr;
    int n = 0;
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
	return 0;

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
	if (buf_ptr == NULL)
	    return 0;
	
	ifconf.ifc_buf = buf_ptr;
	ifconf.ifc_len = buf_len;

	rc = ioctl(fd, SIOCGIFCONF, &ifconf);
	if (rc < 0)
	{
	    if (errno != EINVAL || buf_len_prev != 0)
		return 0;
	}
        else
	{
	    if (ifconf.ifc_len == buf_len_prev)
	    {
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
     * the interfaces and save the ip addresses
     */
    ptr = buf_ptr;

    while(ptr < buf_ptr + buf_len)
    {
	struct ifreq *			ifreq;

	ifreq = (struct ifreq *) ptr;
	
	if (ifreq->ifr_addr.sa_family == AF_INET)
	{
	    struct in_addr		addr;

	    addr = ((struct sockaddr_in *) &(ifreq->ifr_addr))->sin_addr;
/*	    
	    if ((addr.s_addr & net_mask_p->s_addr) ==
		(net_addr_p->s_addr & net_mask_p->s_addr))
	    {
		*if_addr_p = addr;
		break;
	    }
*/
	    pIP[n] = addr.s_addr;
	    n++;
	}

	/*
	 *  Increment pointer to the next ifreq; some adjustment may be
	 *  required if the address is an IPv6 address
	 */
	ptr += sizeof(struct ifreq);
	
#	if defined(AF_INET6)
	{
	    if (ifreq->ifr_addr.sa_family == AF_INET6)
	    {
		ptr += sizeof(struct sockaddr_in6) - sizeof(struct sockaddr);
	    }
	}
#	endif
    }

    MPIU_Free(buf_ptr);
    close(fd);

    return n;
}

#endif /* HAVE_WINDOWS_H */
