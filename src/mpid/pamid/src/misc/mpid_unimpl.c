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

#ifndef DYNAMIC_TASKING
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
#endif

int MPID_Comm_failure_ack(MPID_Comm *comm_ptr)
{
  MPID_abort();
  return 0;
}

int MPID_Comm_failure_get_acked(MPID_Comm *comm_ptr, MPID_Group **failed_group_ptr)
{
  MPID_abort();
  return 0;
}

int MPID_Comm_get_all_failed_procs(MPID_Comm *comm_ptr, MPID_Group **failed_group, int tag)
{
  MPID_abort();
  return 0;
}

int MPID_Comm_revoke(MPID_Comm *comm_ptr, int is_remote)
{
  MPID_abort();
  return 0;
}

int MPID_Comm_AS_enabled(MPID_Comm *comm_ptr)
{
  /* This function must return 1 in the default case and should not be ignored
   * by the implementation. */
  return 1;
}

int MPID_Request_is_anysource(MPID_Request *request_ptr)
{
  /* This function must not abort in the default case since it is used in many
   * MPI functions. As long as the device does not implement FT, it doesn't
   * matter what this function returns. */
  return 0;
}
