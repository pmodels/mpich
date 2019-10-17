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
 *    addr - This structure includes the interface IP address (as bytes),
 *             and the type (e.g., AF_INET or AF_INET6).
 */

#undef FUNCNAME
#define FUNCNAME MPIDU_CH3U_GetSockInterfaceAddr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_CH3U_GetSockInterfaceAddr( int myRank, char *ifname, int maxIfname,
				     MPL_sockaddr_t *p_addr )
{
    char *ifname_string;
    int mpi_errno = MPI_SUCCESS;
    int ifaddrFound = 0;

    if (dbg_ifname < 0) {
	int rc;
	rc = MPL_env2bool( "MPICH_DBG_IFNAME", &dbg_ifname );
	if (rc != 1) dbg_ifname = 0;
    }

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
        int ret;

	/* If we have nothing, then use the host name */
	mpi_errno = MPID_Get_processor_name(ifname, maxIfname, &len );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	ifname_string = ifname;

        ret = MPL_get_sockaddr_iface(NULL, p_addr);
        ifaddrFound = (ret == 0);
    }
    else {
	/* Copy this name into the output name */
	MPL_strncpy( ifname, ifname_string, maxIfname );
    }

    /* If we don't have an IP address, try to get it from the name */
    if (!ifaddrFound) {
        int ret;
        ret = MPL_get_sockaddr( ifname_string, p_addr );
        if (ret){
            MPIR_ERR_CHKANDJUMP(ret!=0, mpi_errno, MPI_ERR_OTHER, "**sockaddrfailed");
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

