/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "wintcp_impl.h"

/* MPID_nem_newtcp_module_get_conninfo -- This function takes a VC
   pointer as input and outputs the sockaddr, pg_id, and pg_rank of
   the remote process associated with this VC.  [NOTE: I'm not sure
   yet, if the pg_id parameters will be char* or char**.  I'd like to
   avoid a copy on this.] */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_get_conninfo
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_get_conninfo (struct MPIDI_VC *vc, struct sockaddr_in *addr, char **pg_id, int *pg_rank)
{
    int mpi_errno = MPI_SUCCESS;

    *addr = VC_FIELD(vc, sock_id);
    *pg_id = (char *)vc->pg->id;
    *pg_rank = vc->pg_rank;
    
    return mpi_errno;
}

/* MPID_nem_newtcp_module_get_vc_from_conninfo -- This function takes
   the pg_id and pg_rank and returns the corresponding VC. */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_get_vc_from_conninfo
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_get_vc_from_conninfo (char *pg_id, int pg_rank, struct MPIDI_VC **vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_PG_t *pg;
    
    mpi_errno = MPIDI_PG_Find (pg_id, &pg);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    MPIU_ERR_CHKANDJUMP1 (pg == NULL, mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "invalid PG");
    MPIU_ERR_CHKANDJUMP1 (pg_rank < 0 || pg_rank > MPIDI_PG_Get_size (pg), mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "invalid pg_rank");
        
    MPIDI_PG_Get_vc_set_active (pg, pg_rank, vc);
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME set_sockopts
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_set_sockopts (MPIU_SOCKW_Sockfd_t fd)
{
    int mpi_errno = MPI_SUCCESS;
    int option;
    socklen_t len;

    /* I heard you have to read the options after setting them in some implementations */

    option = 1;
    len = sizeof(int);

    mpi_errno = MPIU_SOCKW_Sock_setopt(fd, IPPROTO_TCP, TCP_NODELAY, &option, len);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    option = MPID_NEM_NEWTCP_MODULE_RCVBUF_SZ;
    len = sizeof(int);

    mpi_errno = MPIU_SOCKW_Sock_setopt(fd, SOL_SOCKET, SO_RCVBUF, &option, len);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    option = MPID_NEM_NEWTCP_MODULE_SNDBUF_SZ;
    len = sizeof(int);

    mpi_errno = MPIU_SOCKW_Sock_setopt(fd, SOL_SOCKET, SO_SNDBUF, &option, len);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    
    option = 1;
    mpi_errno = MPIU_SOCKW_Sock_cntrl_nb(fd, option);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/*
  MPID_NEM_NEWTCP_MODULE_SOCK_ERROR_EOF : connection failed
  MPID_NEM_NEWTCP_MODULE_SOCK_CONNECTED : socket connected (connection success)
  MPID_NEM_NEWTCP_MODULE_SOCK_NOEVENT   : No event on socket

N1: some implementations do not set POLLERR when there is a pending error on socket.
So, solution is to check for readability/writeablility and then call getsockopt.
Again, getsockopt behaves differently in different implementations which  is handled
safely here (per Stevens-Unix Network Programming)

N2: As far as the socket code is concerned, it doesn't really differentiate whether
there is an error in the socket or whether the peer has closed it (i.e we have received
EOF and hence recv returns 0). Either way, we deccide the socket fd is not usable any
more. So, same return code is used.
A design decision is not to write also, if the peer has closed the socket. Please note that
write will still be succesful, even if the peer has sent us FIN. Only the subsequent 
write will fail. So, this function is made tight enough and this should be called
before doing any read/write at least in the connection establishment state machine code.

N3: return code MPID_NEM_NEWTCP_MODULE_SOCK_NOEVENT is used only by the code that wants to
know whether the connect is still not complete after a non-blocking connect is issued.

TODO: Make this a macro for performance, if needed based on the usage.
FIXME : Above comments are inconsistent now with the changes. No check for EOF is 
actually done now in this function.
*/


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_check_sock_status
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_NEM_NEWTCP_MODULE_SOCK_STATUS_t 
MPID_nem_newtcp_module_check_sock_status(MPIU_SOCKW_Waitset_sock_hnd_t fd_ws_hnd)
{
    int rc = MPID_NEM_NEWTCP_MODULE_SOCK_NOEVENT;

    /* FIXME: At least on windows there is no guarantee that a successful socket call
     * resets the socket error code
     * We will assume that sock is connected for now and let the state machine handle
     * errors
     */
    if(MPIU_SOCKW_Waitset_is_sock_readable(fd_ws_hnd) ||
        MPIU_SOCKW_Waitset_is_sock_writeable(fd_ws_hnd)){
        rc = MPID_NEM_NEWTCP_MODULE_SOCK_CONNECTED;
    }
    else{
        /* Should we return no event for OOB data - EX ? */
        rc = MPID_NEM_NEWTCP_MODULE_SOCK_NOEVENT;
    }

    return rc;
}
