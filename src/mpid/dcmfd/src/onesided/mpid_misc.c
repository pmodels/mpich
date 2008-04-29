/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/onesided/mpid_misc.c
 * \brief MPI-DCMF Left-over stuff not part of RMA
 */

#include "mpidimpl.h"

/// \cond NOT_DOCUMENTED
/**
 * \brief Obsolete hack for intercomm support
 *
 * In MPICH 1.0.3, this call was added as a temporary hack for
 * intercomm support.
 *
 * In the bgl device, we don't use gpids
 * so this is a "do nothing function"
 */
int MPID_PG_ForwardPGInfo( MPID_Comm *peer_ptr, MPID_Comm *comm_ptr,
        int nPGids, int gpids[], int root) {

        return 0;
}

/* ---- these are new in MPICH2 0.94b1 ------- */

#undef FUNCNAME
#define FUNCNAME MPID_Open_port
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/**
 * \brief MPI-DCMF glue for MPI_OPEN_PORT function
 */
int MPID_Open_port(MPID_Info *info_ptr, char *port_name)
{
        MPID_abort();
}

#undef FUNCNAME
#define FUNCNAME MPID_Close_port
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/**
 * \brief MPI-DCMF glue for MPI_CLOSE_PORT function
 */
int MPID_Close_port(const char *port_name)
{
        MPID_abort();
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_connect
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/**
 * \brief MPI-DCMF glue for MPI_COMM_CONNECT function
 */
int MPID_Comm_connect(const char *port_name, MPID_Info *info_ptr,
        int root, MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
        MPID_abort();
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/**
 * \brief MPI-DCMF glue for MPI_COMM_SPAWN_MULTIPLE function
 */
int MPID_Comm_spawn_multiple(int count, char *array_of_commands[],
        char* *array_of_argv[], int array_of_maxprocs[],
        MPID_Info *array_of_info[], int root, MPID_Comm *comm_ptr,
        MPID_Comm **intercomm, int array_of_errcodes[])
{
        MPID_abort();
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_accept
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/**
 * \brief MPI-DCMF glue for MPI_COMM_ACCEPT function
 */
int MPID_Comm_accept(char *port_name, MPID_Info *info_ptr, int root,
        MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
        MPID_abort();
}

#undef FUNCNAME
#define FUNCNAME MPID_Comm_disconnect
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/**
 * \brief MPI-DCMF glue for MPI_COMM_DISCONNECT function
 */
int MPID_Comm_disconnect(MPID_Comm *comm_ptr)
{
        MPID_abort();
}
/// \endcond
