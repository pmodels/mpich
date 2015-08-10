/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpichconf.h"
#include <stdio.h>
/* Note that we need _XOPEN_SOURCE or some BSD source to get a prototype
   for gethostname from unistd.h */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
/* Note that we need _XOPEN_SOURCE or _SVID_SOURCE to get a prototype
   for putenv from stdlib.h */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
/* ctype is needed for isspace and isascii (isspace is only defined for 
   values on which isascii returns true). */
#include <ctype.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "pmutil.h"
#include "process.h"
#include "pmiserv.h"
#include "ioloop.h"

#if defined( NEEDS_GETHOSTNAME_DECL )
int gethostname(char *name, size_t len);
#endif

#ifndef MAX_PENDING_CONN
#define MAX_PENDING_CONN 10
#endif
#ifndef MAX_HOST_NAME
#define MAX_HOST_NAME 1024
#endif
#ifndef MAX_PORT_STRING
#define MAX_PORT_STRING (MAX_HOST_NAME+15)
#endif

static int listenfd = -1;

/* ----------------------------------------------------------------------- */
/* Get a port for the PMI interface                                        */
/* Ports can be allocated within a requested range using the runtime       */
/* parameter value MPIEXEC_PORTRANGE, which has the format low:high,       */
/* where both low and high are positive integers.  Unless this program is  */
/* privaledged, the numbers must be greater than 1023.                     */
/* Return -1 on error                                                      */
/* ----------------------------------------------------------------------- */
#include <errno.h>
/* sockaddr_in (Internet) */
#include <netinet/in.h>
/* TCP_NODELAY */
#include <netinet/tcp.h>
#include <fcntl.h>
/* This is really IP!? */
#ifndef TCP
#define TCP 0
#endif

/* Definitions for types that may be ints or may be something else */
/* socklen_t - getsockname and accept */
#ifndef HAVE_SOCKLEN_T
typedef unsigned int socklen_t;
#endif

/*
 * Input Parameters:
 *   portLen - Number of characters in portString
 * Output Parameters:
 *   fdout - An fd that is listening for connection attempts.
 *           Use PMIServAcceptFromPort to process reads from this fd
 *   portString - The name of a port that can be used to connect to 
 *           this process (using connect).
 */
int PMIServGetPort( int *fdout, char *portString, int portLen )
{
    int                fd = -1;
    struct sockaddr_in sa;
    int                optval = 1;
    int                portnum;
    char               *range_ptr;
    int                low_port=0, high_port=0;

    /* Under cygwin we may want to use 1024 as a low port number */
    /* a low and high port of zero allows the system to choose 
       the port value */
    
    /* Get the low and high portnumber range.  zero may be used to allow
       the system to choose.  There is a priority to these values, 
       we keep going until we get one (and skip if none is found) */
    
    range_ptr = getenv( "MPIEXEC_PORTRANGE" );
    if (!range_ptr) {
	range_ptr = getenv( "MPIEXEC_PORT_RANGE" );
    }
    if (!range_ptr) {
	range_ptr = getenv( "MPICH_PORT_RANGE" );
    }
    if (range_ptr) {
	char *p;
	/* Look for n:m format */
	p = range_ptr;
	while (*p && isspace(*p)) p++;
	while (*p && isdigit(*p)) low_port = 10 * low_port + (*p++ - '0');
	if (*p == ':') {
	    p++;
	    while (*p && isdigit(*p)) high_port = 10 * high_port + (*p++ - '0');
	}
	if (*p) {
	    MPL_error_printf( "Invalid character %c in MPIEXEC_PORTRANGE\n", 
			       *p );
	    return -1;
	}
    }

    for (portnum=low_port; portnum<=high_port; portnum++) {
	memset( (void *)&sa, 0, sizeof(sa) );
	sa.sin_family	   = AF_INET;
	sa.sin_port	   = htons( portnum );
	sa.sin_addr.s_addr = INADDR_ANY;
    
	fd = socket( AF_INET, SOCK_STREAM, TCP );
	if (fd < 0) {
	    /* Failure; return immediately */
	    return fd;
	}
    
	if (setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, 
		    (char *)&optval, sizeof(optval) )) {
	    MPL_internal_sys_error_printf( "setsockopt", errno, 0 );
	}
	
	if (bind( fd, (struct sockaddr *)&sa, sizeof(sa) ) < 0) {
	    close( fd );
	    fd = -1;
	    if (errno != EADDRINUSE && errno != EADDRNOTAVAIL) {
		return -1;
	    }
	}
	else {
	    /* Success! We have a port.  */
	    break;
	}
    }
    
    if (fd < 0) {
	/* We were unable to find a usable port */
	return -1;
    }

    DBG_PRINTF( ("Listening on fd %d\n", fd) );
    /* Listen is a non-blocking call that enables connections */
    listen( fd, MAX_PENDING_CONN );

    /* Make sure that this fd doesn't get sent to the children */
    fcntl( fd, F_SETFD, FD_CLOEXEC );
    
    *fdout = fd;
    if (portnum == 0) {
	socklen_t sinlen = sizeof(sa);
	/* We asked for *any* port, so we need to find which
	   port we actually got */
	getsockname( fd, (struct sockaddr *)&sa, &sinlen );
	portnum = ntohs(sa.sin_port);
    }

    /* Create the port string */
    {
	char hostname[MAX_HOST_NAME+1];
	hostname[0] = 0;
	MPIE_GetMyHostName( hostname, sizeof(hostname) );
	MPL_snprintf( portString, portLen, "%s:%d", hostname, portnum );
    }
    
    return 0;
}

/* defs of gethostbyname */
#include <netdb.h>
int MPIE_GetMyHostName( char myname[MAX_HOST_NAME+1], int namelen )
{
    struct hostent     *hp;
    char *hostname = 0;
    /*
     * First, get network name if necessary
     */
    hostname = myname;
    if (!myname[0]) {
	gethostname( myname, namelen );
    }
    /* ??? */
    hp = gethostbyname( hostname );
    if (!hp) {
	return -1;
    }
    return 0;
}

/* IO Handler for the listen socket
   Respond to a connection request by creating a new socket, which is
   then registered.
   Initialize the startup handshake.
 */
int PMIServAcceptFromPort( int fd, int rdwr, void *data )
{
    int             newfd;
    struct sockaddr sock;
    socklen_t       addrlen = sizeof(sock);
    int             id;
    ProcessUniverse *univ = (ProcessUniverse *)data;
    ProcessWorld    *pWorld = univ->worlds;
    ProcessApp      *app;

    /* Get the new socket */
    MPIE_SYSCALL(newfd,accept,( fd, &sock, &addrlen ));
    DBG_PRINTF(("Acquired new socket in accept (fd = %d)\n", newfd ));
    if (newfd < 0) {
	DBG(perror("Error on accept: " ));
	return newfd;
    }

#ifdef FOO
    /* Mark this fd as non-blocking */
    flags = fcntl( newfd, F_GETFL, 0 );
    if (flags >= 0) {
	flags |= O_NDELAY;
	fcntl( newfd, F_SETFL, flags );
    }
#endif
    /* Make sure that exec'd processes don't get this fd */
    fcntl( newfd, F_SETFD, FD_CLOEXEC );

    /* Find the matching process.  Do this by reading from the socket and 
       getting the id value with which process was created. */
    id = PMI_Init_port_connection( newfd );
    if (id >= 0) {
	/* find the matching entry */
	ProcessState *pState = 0;
	int           nSoFar = 0;
	PMIProcess   *pmiprocess;

	/* This code assigns processes to the states in a pWorld
	   by using the id as the rank, and finding the corresponding
	   process among the ranks */
	while (pWorld) {
	    app = pWorld->apps;
	    while (app) {
		if (app->nProcess > id - nSoFar) {
		    /* Found the matching app */
		    pState = app->pState + (id - nSoFar);
		    break;
		}
		else {
		    nSoFar += app->nProcess;
		}
		app = app->nextApp;
	    }
	    pWorld = pWorld->nextWorld;
	}
	if (!pState) {
	    /* We have a problem */
	    MPL_error_printf( "Unable to find process with PMI_ID = %d in the universe", id );
	    return -1;
	}

	/* Now, initialize the connection */
	/* Create the new process structure (see PMISetupFinishInServer
	   for this step when a pre-existing FD is used */
	DBG_PRINTF( ("Server connection to id = %d on fd %d\n", id, newfd ));
	pmiprocess = PMISetupNewProcess( newfd, pState );
	PMI_Init_remote_proc( newfd, pmiprocess );
	MPIE_IORegister( newfd, IO_READ, PMIServHandleInput, 
			 pmiprocess );
    }
    else {
	/* Error, the id should never be less than zero or unset */
	/* An alternative would be to dynamically assign the ranks
	   as processes come in (but we'd still need to use the 
	   PMI_ID to identify the ProcessApp) */
	DBG_PRINTF(("Found an invalid id\n" ));
	return -1;
    }

    /* Return success. */
    return 0;
}
/*
 * Setup a port and register the handler on which to listen.
 * Input Parameters:
 *   mypUniv - Pointer to a process universe.  This is passed to the
 *             routine that is called to handle connection requests to the port
 *
 * Output Parameters:
 *   portString - Name of the port (of maximum size portLen)
 *
 * Return Value:
 *   0 on success.
 *
 * Notes:
 * The listenfd is global to simplify closing it once all processes have 
 * exited.
 */
int PMIServSetupPort( ProcessUniverse *mypUniv, char *portString, int portLen )
{
    int rc = 0;
    
    rc = PMIServGetPort( &listenfd, portString, portLen );
    if (rc) return rc;
    rc = MPIE_IORegister( listenfd, IO_READ, PMIServAcceptFromPort, mypUniv );
    if (mypUniv->OnNone) {
	MPL_internal_error_printf( "pUniv.OnNone already set; cannot set to PMIServEndPort\n" );
	return -1;
    }
    else {
	mypUniv->OnNone = PMIServEndPort;
    }
    return rc;
}
/* This is a signal-safe routine, used to terminate the use of the port
   within the ioloop */
int PMIServEndPort( void )
{
    DBG_PRINTF(("deregistering listenerfd %d\n", listenfd ));
    MPIE_IODeregister( listenfd );
    return 0;
}
/* ------------------------------------------------------------------------- */
/*
 * This code allows mpiexec to connect back to a program in the
 * singleton init case.  This routine essentially identical to 
 * PMII_Connect_to_pm( char *hostname, int portnum ) in simple_pmi.c 
 */
#include <sys/param.h>

/* sockaddr_un (Unix) */
#include <sys/un.h>

/* This routine blocks until connected to the indicated host (by
   interface name) and port.  It returns the fd, or -1 on failure */
int MPIE_ConnectToPort( char *hostname, int portnum )
{
    struct hostent     *hp;
    struct sockaddr_in sa;
    int                fd;
    int                optval = 1;
    int                q_wait = 1;
    char defaultHostname[MAX_HOST_NAME+1];
    
    
    DBG_PRINTF( ("Connecting to %s:%d\n", hostname, portnum ) );
    /* FIXME: simple_pmi should *never* start mpiexec with a bogus
       interface name */
    if (strcmp(hostname,"default_interface") == 0) {
	defaultHostname[0] = 0;
	MPIE_GetMyHostName( defaultHostname, sizeof(defaultHostname) );
	hostname = defaultHostname;
	DBG_PRINTF( ( "Connecting to %s:%d\n", hostname, portnum ) );
    }
    hp = gethostbyname( hostname );
    if (!hp) {
	return -1;
    }
    
    memset( (void *)&sa, 0, sizeof(sa) );
    /* POSIX might define h_addr_list only and not define h_addr */
#ifdef HAVE_H_ADDR_LIST
    memcpy( (void *)&sa.sin_addr, (void *)hp->h_addr_list[0], hp->h_length);
#else
    memcpy( (void *)&sa.sin_addr, (void *)hp->h_addr, hp->h_length);
#endif
    sa.sin_family = hp->h_addrtype;
    sa.sin_port   = htons( (unsigned short) portnum );
    
    fd = socket( AF_INET, SOCK_STREAM, TCP );
    if (fd < 0) {
	return -1;
    }
    
    if (setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, 
		    (char *)&optval, sizeof(optval) )) {
	perror( "Error calling setsockopt:" );
    }

    /* We wait here for the connection to succeed */
    if (connect( fd, (struct sockaddr *)&sa, sizeof(sa) ) < 0) {
	switch (errno) {
	case ECONNREFUSED:
	    /* (close socket, get new socket, try again) */
	    if (q_wait)
		close(fd);
	    return -1;
	    
	case EINPROGRESS: /*  (nonblocking) - select for writing. */
	    break;
	    
	case EISCONN: /*  (already connected) */
	    break;
	    
	case ETIMEDOUT: /* timed out */
	    return -1;

	default:
	    return -1;
	}
    }

    /* The first message must also be received: cmd=initack */
    
    return fd;
}
