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
 * \file src/onesided/mpid_win_reqops.c
 * \brief ???
 */
#include "mpidi_onesided.h"


/**
 * \brief MPI-PAMI glue for MPI_PUT function
 *
 * \param[in] origin_addr      Source buffer
 * \param[in] origin_count     Number of datatype elements
 * \param[in] origin_datatype  Source datatype
 * \param[in] target_rank      Destination rank (target)
 * \param[in] target_disp      Displacement factor in target buffer
 * \param[in] target_count     Number of target datatype elements
 * \param[in] target_datatype  Destination datatype
 * \param[in] win              Window
 * \return MPI_SUCCESS
 */
#undef FUNCNAME
#define FUNCNAME MPID_Rput
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPID_Rput(const void  *origin_addr,
         int           origin_count,
         MPI_Datatype  origin_datatype,
         int           target_rank,
         MPI_Aint      target_disp,
         int           target_count,
         MPI_Datatype  target_datatype,
         MPID_Win     *win,
         MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;

    if(win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK &&
       win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK_ALL){
      MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                          return mpi_errno, "**rmasync");
    }
  
    MPID_Request *rreq = MPIDI_Request_create1();
    *request = rreq;
    rreq->kind = MPID_WIN_REQUEST;
    win->mpid.rreq = rreq;
    win->mpid.request_based = 1;

    /* Enqueue or perform the RMA operation */
    mpi_errno = MPID_Put(origin_addr, origin_count,
                         origin_datatype, target_rank,
                         target_disp, target_count,
                         target_datatype, win);
    MPID_assert(mpi_errno == MPI_SUCCESS);
    return (mpi_errno);
}


/**
 * \brief MPI-PAMI glue for MPI_GET function
 *
 * \param[in] origin_addr      Source buffer
 * \param[in] origin_count     Number of datatype elements
 * \param[in] origin_datatype  Source datatype
 * \param[in] target_rank      Destination rank (target)
 * \param[in] target_disp      Displacement factor in target buffer
 * \param[in] target_count     Number of target datatype elements
 * \param[in] target_datatype  Destination datatype
 * \param[in] win              Window
 * \return MPI_SUCCESS
 */
#undef FUNCNAME
#define FUNCNAME MPID_Rget
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPID_Rget(void         *origin_addr,
         int           origin_count,
         MPI_Datatype  origin_datatype,
         int           target_rank,
         MPI_Aint      target_disp,
         int           target_count,
         MPI_Datatype  target_datatype,
         MPID_Win     *win,
         MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;

    if(win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK &&
       win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK_ALL){
      MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                          return mpi_errno, "**rmasync");
    }

    MPID_Request *rreq = MPIDI_Request_create1();
    rreq->kind = MPID_WIN_REQUEST;
    *request = rreq;
    win->mpid.rreq = rreq;
    win->mpid.request_based = 1;

    mpi_errno = MPID_Get(origin_addr, origin_count,
                         origin_datatype, target_rank,
                         target_disp, target_count,
                         target_datatype, win);
    MPID_assert(mpi_errno == MPI_SUCCESS);
    return(mpi_errno);
}

/**
 * \brief MPI-PAMI glue for MPI_ACCUMULATE function
 *
 * According to the MPI Specification:
 *
 *        Each datatype argument must be a predefined datatype or
 *        a derived datatype, where all basic components are of the
 *        same predefined datatype. Both datatype arguments must be
 *        constructed from the same predefined datatype.
 *
 * \param[in] origin_addr      Source buffer
 * \param[in] origin_count     Number of datatype elements
 * \param[in] origin_datatype  Source datatype
 * \param[in] target_rank      Destination rank (target)
 * \param[in] target_disp      Displacement factor in target buffer
 * \param[in] target_count     Number of target datatype elements
 * \param[in] target_datatype  Destination datatype
 * \param[in] op               Operand to perform
 * \param[in] win              Window
 * \return MPI_SUCCESS
 */
#undef FUNCNAME
#define FUNCNAME MPID_Raccumulate
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPID_Raccumulate(const void  *origin_addr,
                int           origin_count,
                MPI_Datatype  origin_datatype,
                int           target_rank,
                MPI_Aint      target_disp,
                int           target_count,
                MPI_Datatype  target_datatype,
                MPI_Op        op,
                MPID_Win     *win,
                MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;

    if(win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK &&
       win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK_ALL){
      MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                          return mpi_errno, "**rmasync");
    }

    MPID_Request *rreq = MPIDI_Request_create1();
    rreq->kind = MPID_WIN_REQUEST;
    *request = rreq;
    win->mpid.rreq = rreq;
    win->mpid.request_based = 1;

    /* Enqueue or perform the RMA operation */
    mpi_errno = MPID_Accumulate(origin_addr, origin_count,
                                origin_datatype, target_rank,
                                target_disp, target_count,
                                target_datatype, op, win);
    MPID_assert(mpi_errno == MPI_SUCCESS);
    return(mpi_errno);
}


/**
 * \brief MPI-PAMI glue for MPI_ACCUMULATE function
 *
 * According to the MPI Specification:
 *
 *        Each datatype argument must be a predefined datatype or
 *        a derived datatype, where all basic components are of the
 *        same predefined datatype. Both datatype arguments must be
 *        constructed from the same predefined datatype.
 *
 * \param[in] origin_addr      Source buffer
 * \param[in] origin_count     Number of datatype elements
 * \param[in] origin_datatype  Source datatype
 * \param[in] target_rank      Destination rank (target)
 * \param[in] target_disp      Displacement factor in target buffer
 * \param[in] target_count     Number of target datatype elements
 * \param[in] target_datatype  Destination datatype
 * \param[in] op               Operand to perform
 * \param[in] win              Window
 * \return MPI_SUCCESS
 */
#undef FUNCNAME
#define FUNCNAME MPID_Rget_accumulate
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int
MPID_Rget_accumulate(const void         *origin_addr,
                int           origin_count,
                MPI_Datatype  origin_datatype,
                void         *result_addr,
                int           result_count,
                MPI_Datatype  result_datatype,
                int           target_rank,
                MPI_Aint      target_disp,
                int           target_count,
                MPI_Datatype  target_datatype,
                MPI_Op        op,
                MPID_Win     *win,
		MPID_Request **request)
{

    int mpi_errno = MPI_SUCCESS;

    if(win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK &&
       win->mpid.sync.origin_epoch_type != MPID_EPOTYPE_LOCK_ALL){
      MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_RMA_SYNC,
                          return mpi_errno, "**rmasync");
    }

    MPID_Request *rreq = MPIDI_Request_create1();
    rreq->kind = MPID_WIN_REQUEST;
    *request = rreq;
    win->mpid.rreq = rreq;
    win->mpid.request_based = 1;

    /* Enqueue or perform the RMA operation */
    mpi_errno = MPID_Get_accumulate(origin_addr, origin_count,
                                    origin_datatype, result_addr,
                                    result_count, result_datatype,
                                    target_rank, target_disp,
                                    target_count, target_datatype,
                                    op, win);
    MPID_assert(mpi_errno == MPI_SUCCESS);
    return (mpi_errno);
}

