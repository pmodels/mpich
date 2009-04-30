/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Comm_spawn_multiple(int count, char **commands,
                                  char ***argvs, int *maxprocs,
                                  MPID_Info **info_ptrs, int root,
                                  MPID_Comm *comm_ptr, MPID_Comm
                                  **intercomm, int *errcodes)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				     __LINE__, MPI_ERR_OTHER,
				     "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_SPAWN_MULTIPLE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Open_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Open_port(char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_OPEN_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_OPEN_PORT);
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				     __LINE__, MPI_ERR_OTHER,
				     "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_OPEN_PORT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Comm_accept
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Comm_accept(char *port_name, int root, MPID_Comm
                          *comm_ptr, MPID_Comm **newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_ACCEPT);
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				     __LINE__, MPI_ERR_OTHER,
				     "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_ACCEPT);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Comm_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Comm_connect(char *port_name, int root, MPID_Comm
                           *comm_ptr, MPID_Comm **newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_COMM_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_COMM_CONNECT);
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
				     __LINE__, MPI_ERR_OTHER,
				     "**notimpl", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_COMM_CONNECT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Get_business_card (int myRank, char *value, int length)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_STATE_DECL(MPIDI_STATE_MPIDI_CH3_GET_BUSINESS_CARD);

    MPIDI_FUNC_ENTER(MPIDI_STATE_MPIDI_CH3_GET_BUSINESS_CARD);

    ret = MPID_nem_get_business_card (myRank, value, length);
    if (ret != 0)
	mpi_errno =  MPIR_Err_create_code (MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**intern", 0);

    MPIDI_FUNC_EXIT(MPIDI_STATE_MPIDI_CH3_GET_BUSINESS_CARD);
    return mpi_errno;
}
