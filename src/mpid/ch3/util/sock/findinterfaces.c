/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
   CONSISTENTLY FOR ALL FILES COMPILED AS PART OF MPICH2
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

/*
 * FIXME: This is old code that can be used to discover the available 
 * interfaces, and is left here for reference purposes only
 */
#if 0
    int32_t local_ip[MAX_NUM_NICS];
    unsigned int a, b, c, d;
    int num_nics, i;
    char *value_orig;
    struct hostent *h;
    int port;

    /*snprintf(value, length, "%s:%d", host, port);*/

    value_orig = value;
    num_nics = GetLocalIPs(local_ip, MAX_NUM_NICS);
    for (i=0; i<num_nics; i++)
    {
	a = (unsigned char)(((unsigned char *)(&local_ip[i]))[0]);
	b = (unsigned char)(((unsigned char *)(&local_ip[i]))[1]);
	c = (unsigned char)(((unsigned char *)(&local_ip[i]))[2]);
	d = (unsigned char)(((unsigned char *)(&local_ip[i]))[3]);

	if (a != 127)
	{
	    h = gethostbyaddr((const char *)&local_ip[i], sizeof(int), AF_INET);
	    if (h && h->h_name != NULL)
		value += MPIU_Snprintf(value, MPI_MAX_PORT_NAME, 
				 "%s:%u.%u.%u.%u:%d:", 
				 h->h_name, 
				 a, b, c, d,
				 port);
	    else
		value += MPIU_Snprintf(value, MPI_MAX_PORT_NAME, 
				 "%u.%u.%u.%u:%u.%u.%u.%u:%d:", 
				 a, b, c, d, 
				 a, b, c, d,
				 port);
	}
    }
#endif

/* This code was removed from ch3_progress_connect in channels/ssm */
#if 0

static unsigned int GetIP(char *pszIP)
{
    unsigned int nIP;
    unsigned int a,b,c,d;
    if (pszIP == NULL)
	return 0;
    sscanf(pszIP, "%u.%u.%u.%u", &a, &b, &c, &d);
    /*MPIU_DBG_PRINTF(("mask: %u.%u.%u.%u\n", a, b, c, d));*/
    nIP = (d << 24) | (c << 16) | (b << 8) | a;
    return nIP;
}

static unsigned int GetMask(char *pszMask)
{
    int i, nBits;
    unsigned int nMask = 0;
    unsigned int a,b,c,d;

    if (pszMask == NULL)
	return 0;

    if (strstr(pszMask, "."))
    {
	sscanf(pszMask, "%u.%u.%u.%u", &a, &b, &c, &d);
	/*MPIU_DBG_PRINTF(("mask: %u.%u.%u.%u\n", a, b, c, d));*/
	nMask = (d << 24) | (c << 16) | (b << 8) | a;
    }
    else
    {
	nBits = atoi(pszMask);
	for (i=0; i<nBits; i++)
	{
	    nMask = nMask << 1;
	    nMask = nMask | 0x1;
	}
    }
    /*
    unsigned int a, b, c, d;
    a = ((unsigned char *)(&nMask))[0];
    b = ((unsigned char *)(&nMask))[1];
    c = ((unsigned char *)(&nMask))[2];
    d = ((unsigned char *)(&nMask))[3];
    MPIU_DBG_PRINTF(("mask: %u.%u.%u.%u\n", a, b, c, d));
    */
    return nMask;
}

static int GetHostAndPort(char *host, int *port, char *business_card)
{
    char pszNetMask[50];
    char *pEnv, *token;
    unsigned int nNicNet, nNicMask;
    char *temp, *pszHost, *pszIP, *pszPort;
    unsigned int ip;

    pEnv = getenv("MPICH_NETMASK");
    if (pEnv != NULL)
    {
	MPIU_Strncpy(pszNetMask, pEnv, 50);
	token = strtok(pszNetMask, "/");
	if (token != NULL)
	{
	    token = strtok(NULL, "\n");
	    if (token != NULL)
	    {
		nNicNet = GetIP(pszNetMask);
		nNicMask = GetMask(token);

		/* parse each line of the business card and match the ip address with the network mask */
		temp = MPIU_Strdup(business_card);
		token = strtok(temp, ":\r\n");
		while (token)
		{
		    pszHost = token;
		    pszIP = strtok(NULL, ":\r\n");
		    pszPort = strtok(NULL, ":\r\n");
		    ip = GetIP(pszIP);
		    /*msg_printf("masking '%s'\n", pszIP);*/
		    if ((ip & nNicMask) == nNicNet)
		    {
			/* the current ip address matches the requested network so return these values */
			MPIU_Strncpy(host, pszIP, MAXHOSTNAMELEN); /*pszHost);*/
			*port = atoi(pszPort);
			MPIU_Free(temp);
			return MPI_SUCCESS;
		    }
		    token = strtok(NULL, ":\r\n");
		}
		if (temp)
		    MPIU_Free(temp);
	    }
	}
    }

    temp = MPIU_Strdup(business_card);
    if (temp == NULL)
    {
	/*MPIDI_err_printf("GetHostAndPort", "MPIU_Strdup failed\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MPIU_Strdup", 0);
    }
    /* move to the host part */
    token = strtok(temp, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**business_card", "**business_card %s", business_card); /*"[ch3:mm] GetHostAndPort: Invalid business card - %s", business_card);*/
    }
    /*strcpy(host, token);*/
    /* move to the ip part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**business_card", "**business_card %s", business_card); /*"[ch3:mm] GetHostAndPort: Invalid business card - %s", business_card);*/
    }
    MPIU_Strncpy(host, token, MAXHOSTNAMELEN); /* use the ip string instead of the hostname, it's more reliable */
    /* move to the port part */
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	MPIU_Free(temp);
	/*MPIDI_err_printf("GetHostAndPort", "invalid business card\n");*/
	return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**business_card", "**business_card %s", business_card); /*"[ch3:mm] GetHostAndPort: Invalid business card - %s", business_card);*/
    }
    *port = atoi(token);
    MPIU_Free(temp);

    return MPI_SUCCESS;
}
#endif
