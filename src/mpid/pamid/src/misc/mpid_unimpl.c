/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/misc/mpid_unimpl.c
 * \brief These are functions that are not supported
 */
#include <mpidimpl.h>

int MPID_Close_port(const char *port_name)
{
  MPID_abort();
  return 0;
}
int MPID_Open_port(MPID_Info *info_ptr,
                   char *port_name)
{
  MPID_abort();
  return 0;
}

int MPID_Comm_accept(const char *port_name,
                     MPID_Info *info_ptr,
                     int root,
                     MPID_Comm *comm_ptr,
                     MPID_Comm **newcomm)
{
  MPID_abort();
  return 0;
}
int MPID_Comm_connect(const char *port_name,
                      MPID_Info *info_ptr,
                      int root,
                      MPID_Comm *comm_ptr,
                      MPID_Comm **newcomm)
{
  MPID_abort();
  return 0;
}
int MPID_Comm_disconnect(MPID_Comm *comm_ptr)
{
  MPID_abort();
  return 0;
}
int MPID_Comm_spawn_multiple(int count,
                             char *array_of_commands[],
                             char* *array_of_argv[],
                             const int array_of_maxprocs[],
                             MPID_Info *array_of_info[],
                             int root,
                             MPID_Comm *comm_ptr,
                             MPID_Comm **intercomm,
                             int array_of_errcodes[])
{
  MPID_abort();
  return 0;
}


int MPID_Comm_reenable_anysource(MPID_Comm *comm,
                                 MPID_Group **failed_group_ptr)
{
  MPID_abort();
  return 0;
}

int MPID_Comm_remote_group_failed(MPID_Comm *comm, MPID_Group **failed_group_ptr)
{
  MPID_abort();
  return 0;
}

int MPID_Comm_group_failed(MPID_Comm *comm_ptr, MPID_Group **failed_group_ptr)
{
  MPID_abort();
  return 0;
}

int MPID_Win_attach(MPID_Win *win, void *base, MPI_Aint size)
{
  MPID_abort();
  return 0;
}

int MPID_Win_allocate_shared(MPI_Aint size, int disp_unit, MPID_Info *info_ptr, MPID_Comm *comm_ptr,
                             void **base_ptr, MPID_Win **win_ptr)
{
  MPID_abort();
  return 0;
}

int MPID_Rput(const void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPID_Win *win,
              MPID_Request **request)
{
  MPID_abort();
  return 0;
}

int MPID_Win_flush_local(int rank, MPID_Win *win)
{
  MPID_abort();
  return 0;
}

int MPID_Win_detach(MPID_Win *win, const void *base)
{
  MPID_abort();
  return 0;
}

int MPID_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                          void *result_addr, MPI_Datatype datatype, int target_rank,
                          MPI_Aint target_disp, MPID_Win *win)
{
  MPID_abort();
  return 0;
}

int MPID_Raccumulate(const void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                     int target_count, MPI_Datatype target_datatype, MPI_Op op, MPID_Win *win,
                     MPID_Request **request)
{
  MPID_abort();
  return 0;
}

int MPID_Rget_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, int result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype, MPI_Op op, MPID_Win *win,
                         MPID_Request **request)
{
  MPID_abort();
  return 0;
}

int MPID_Fetch_and_op(const void *origin_addr, void *result_addr,
                      MPI_Datatype datatype, int target_rank, MPI_Aint target_disp,
                      MPI_Op op, MPID_Win *win)
{
  MPID_abort();
  return 0;
}

int MPID_Win_shared_query(MPID_Win *win, int rank, MPI_Aint *size, int *disp_unit,
                          void *baseptr)
{
  MPID_abort();
  return 0;
}

int MPID_Win_allocate(MPI_Aint size, int disp_unit, MPID_Info *info,
                      MPID_Comm *comm, void *baseptr, MPID_Win **win)
{
  MPID_abort();
  return 0;
}

int MPID_Win_flush(int rank, MPID_Win *win)
{
  MPID_abort();
  return 0;
}

int MPID_Win_flush_local_all(MPID_Win *win)
{
  MPID_abort();
  return 0;
}

int MPID_Win_unlock_all(MPID_Win *win)
{
  MPID_abort();
  return 0;
}

int MPID_Win_create_dynamic(MPID_Info *info, MPID_Comm *comm, MPID_Win **win)
{
  MPID_abort();
  return 0;
}

int MPID_Rget(void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPID_Win *win,
              MPID_Request **request)
{
  MPID_abort();
  return 0;
}

int MPID_Win_sync(MPID_Win *win)
{
  MPID_abort();
  return 0;
}

int MPID_Win_flush_all(MPID_Win *win)
{
  MPID_abort();
  return 0;
}

int MPID_Get_accumulate(const void *origin_addr, int origin_count,
                        MPI_Datatype origin_datatype, void *result_addr, int result_count,
                        MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                        int target_count, MPI_Datatype target_datatype, MPI_Op op, MPID_Win *win)
{
  MPID_abort();
  return 0;
}

int MPID_Win_lock_all(int assert, MPID_Win *win)
{
  MPID_abort();
  return 0;
}
