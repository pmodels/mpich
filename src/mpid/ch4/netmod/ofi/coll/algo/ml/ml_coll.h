/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifndef COLL_NAMESPACE
#error "The collectives template must be namespaced with COLL_NAMESPACE"
#endif


static inline int COLL_init()
{
    return 0;
}

static inline int COLL_comm_init(COLL_comm_t *comm)
{
    return 0;
}

static inline int COLL_allgather(const void  *sendbuf,
                                 int          sendcount,
                                 COLL_dt_t   *sendtype,
                                 void        *recvbuf,
                                 int          recvcount,
                                 COLL_dt_t   *recvtype,
                                 COLL_comm_t *comm,
                                 int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_allgatherv(const void  *sendbuf,
                                  int          sendcount,
                                  COLL_dt_t   *sendtype,
                                  void        *recvbuf,
                                  const int   *recvcounts,
                                  const int   *displs,
                                  COLL_dt_t   *recvtype,
                                  COLL_comm_t *comm,
                                  int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_allreduce(const void  *sendbuf,
                                 void        *recvbuf,
                                 int          count,
                                 COLL_dt_t   *datatype,
                                 COLL_op_t   *op,
                                 COLL_comm_t *comm,
                                 int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_alltoall(const void  *sendbuf,
                                int          sendcount,
                                COLL_dt_t   *sendtype,
                                void        *recvbuf,
                                int          recvcount,
                                COLL_dt_t   *recvtype,
                                COLL_comm_t *comm,
                                int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_alltoallv(const void  *sendbuf,
                                 const int   *sendcnts,
                                 const int   *sdispls,
                                 COLL_dt_t   *sendtype,
                                 void        *recvbuf,
                                 const int   *recvcnts,
                                 const int   *rdispls,
                                 COLL_dt_t   *recvtype,
                                 COLL_comm_t *comm,
                                 int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_alltoallw(const void       *sendbuf,
                                 const int        *sendcnts,
                                 const int        *sdispls,
                                 const COLL_dt_t **sendtypes,
                                 void             *recvbuf,
                                 const int        *recvcnts,
                                 const int        *rdispls,
                                 const COLL_dt_t **recvtypes,
                                 COLL_comm_t      *comm,
                                 int              *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_bcast(void        *buffer,
                             int          count,
                             COLL_dt_t   *datatype,
                             int          root,
                             COLL_comm_t *comm,
                             int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_exscan(const void  *sendbuf,
                              void        *recvbuf,
                              int          count,
                              COLL_dt_t   *datatype,
                              COLL_op_t   *op,
                              COLL_comm_t *comm,
                              int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_gather(const void  *sendbuf,
                              int          sendcnt,
                              COLL_dt_t    sendtype,
                              void        *recvbuf,
                              int          recvcnt,
                              COLL_dt_t   *recvtype,
                              int          root,
                              COLL_comm_t *comm,
                              int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_gatherv(const void  *sendbuf,
                               int          sendcnt,
                               COLL_dt_t   *sendtype,
                               void        *recvbuf,
                               const int   *recvcnts,
                               const int   *displs,
                               COLL_dt_t   *recvtype,
                               int          root,
                               COLL_comm_t *comm,
                               int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_reduce_scatter(const void  *sendbuf,
                                      void        *recvbuf,
                                      const int   *recvcnts,
                                      COLL_dt_t    datatype,
                                      COLL_op_t    op,
                                      COLL_comm_t  comm,
                                      int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_reduce_scatter_block(const void  *sendbuf,
                                            void        *recvbuf,
                                            int          recvcount,
                                            COLL_dt_t   *datatype,
                                            COLL_op_t   *op,
                                            COLL_comm_t *comm,
                                            int         *errflag)
{
    assert(0);
    return 0;
}


static inline int COLL_reduce(const void  *sendbuf,
                              void        *recvbuf,
                              int          count,
                              COLL_dt_t   *datatype,
                              COLL_op_t   *op,
                              int          root,
                              COLL_comm_t *comm,
                              int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_scan(const void  *sendbuf,
                            void        *recvbuf,
                            int         count,
                            COLL_dt_t   *datatype,
                            COLL_op_t   *op,
                            COLL_comm_t *comm,
                            int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_scatter(const void  *sendbuf,
                               int          sendcnt,
                               COLL_dt_t   *sendtype,
                               void        *recvbuf,
                               int          recvcnt,
                               COLL_dt_t   *recvtype,
                               int          root,
                               COLL_comm_t *comm,
                               int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_scatterv(const void  *sendbuf,
                                const int   *sendcnts,
                                const int   *displs,
                                COLL_dt_t   *sendtype,
                                void        *recvbuf,
                                int          recvcnt,
                                COLL_dt_t   *recvtype,
                                int          root,
                                COLL_comm_t *comm,
                                int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_barrier(COLL_comm_t *comm,
                               int         *errflag)
{
    // set tree radix to -1 for default value
    COLL_NAMESPACE_LOCAL(barrier)(comm->comm.local, errflag, -1);
    COLL_NAMESPACE_REMOTE(barrier)(comm->comm.remote, errflag, -1);
    return 0;
}

static inline int COLL_iallgather(const void  *sendbuf,
                                  int          sendcount,
                                  COLL_dt_t   *sendtype,
                                  void        *recvbuf,
                                  int          recvcount,
                                  COLL_dt_t   *recvtype,
                                  COLL_comm_t *comm,
                                  COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_iallgatherv(const void  *sendbuf,
                                   int          sendcount,
                                   COLL_dt_t   *sendtype,
                                   void        *recvbuf,
                                   const int   *recvcounts,
                                   const int   *displs,
                                   COLL_dt_t   *recvtype,
                                   COLL_comm_t *comm,
                                   COLL_req_t  *request)
{
    assert(0);
    return 0;
}


static inline int COLL_kick(COLL_queue_elem_t *elem)
{
    assert(0);
    return 0;
}

static inline int COLL_iallreduce(const void  *sendbuf,
                                  void        *recvbuf,
                                  int          count,
                                  COLL_dt_t   *datatype,
                                  COLL_op_t   *op,
                                  COLL_comm_t *comm,
                                  COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ialltoall(const void  *sendbuf,
                                 int          sendcount,
                                 COLL_dt_t   *sendtype,
                                 void        *recvbuf,
                                 int          recvcount,
                                 COLL_dt_t   *recvtype,
                                 COLL_comm_t *comm,
                                 COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ialltoallv(const void  *sendbuf,
                                  const int   *sendcnts,
                                  const int   *sdispls,
                                  COLL_dt_t    sendtype,
                                  void        *recvbuf,
                                  const int   *recvcnts,
                                  const int   *rdispls,
                                  COLL_dt_t    recvtype,
                                  COLL_comm_t  comm,
                                  COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ialltoallw(const void       *sendbuf,
                                  const int        *sendcnts,
                                  const int        *sdispls,
                                  const COLL_dt_t **sendtypes,
                                  void             *recvbuf,
                                  const int        *recvcnts,
                                  const int        *rdispls,
                                  const COLL_dt_t **recvtypes,
                                  COLL_comm_t      *comm,
                                  COLL_req_t       *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ibcast(void        *buffer,
                              int          count,
                              COLL_dt_t   *datatype,
                              int          root,
                              COLL_comm_t *comm,
                              COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_iexscan(const void  *sendbuf,
                               void        *recvbuf,
                               int          count,
                               COLL_dt_t   *datatype,
                               COLL_op_t   *op,
                               COLL_comm_t *comm,
                               COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_igather(const void  *sendbuf,
                               int          sendcnt,
                               COLL_dt_t   *sendtype,
                               void        *recvbuf,
                               int          recvcnt,
                               COLL_dt_t   *recvtype,
                               int          root,
                               COLL_comm_t *comm,
                               COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_igatherv(const void  *sendbuf,
                                int          sendcnt,
                                COLL_dt_t   *sendtype,
                                void        *recvbuf,
                                const int   *recvcnts,
                                const int   *displs,
                                COLL_dt_t   *recvtype,
                                int          root,
                                COLL_comm_t *comm,
                                COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ireduce_scatter(const void  *sendbuf,
                                       void        *recvbuf,
                                       const int   *recvcnts,
                                       COLL_dt_t   *datatype,
                                       COLL_op_t   *op,
                                       COLL_comm_t *comm,
                                       COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ireduce_scatter_block(const void  *sendbuf,
                                             void        *recvbuf,
                                             int          recvcount,
                                             COLL_dt_t   *datatype,
                                             COLL_op_t   *op,
                                             COLL_comm_t *comm,
                                             COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ireduce(const void  *sendbuf,
                               void        *recvbuf,
                               int          count,
                               COLL_dt_t   *datatype,
                               COLL_op_t   *op,
                               int          root,
                               COLL_comm_t *comm,
                               COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_iscan(const void  *sendbuf,
                             void        *recvbuf,
                             int          count,
                             COLL_dt_t   *datatype,
                             COLL_op_t   *op,
                             COLL_comm_t *comm,
                             COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_iscatter(const void  *sendbuf,
                                int          sendcnt,
                                COLL_dt_t   *sendtype,
                                void        *recvbuf,
                                int          recvcnt,
                                COLL_dt_t   *recvtype,
                                int          root,
                                COLL_comm_t *comm,
                                COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_iscatterv(const void  *sendbuf,
                                 const int   *sendcnts,
                                 const int   *displs,
                                 COLL_dt_t   *sendtype,
                                 void        *recvbuf,
                                 int          recvcnt,
                                 COLL_dt_t   *recvtype,
                                 int          root,
                                 COLL_comm_t *comm,
                                 COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ibarrier(COLL_comm_t *comm,
                                COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_neighbor_allgather(const void  *sendbuf,
                                          int          sendcount,
                                          COLL_dt_t   *sendtype,
                                          void        *recvbuf,
                                          int          recvcount,
                                          COLL_dt_t   *recvtype,
                                          COLL_comm_t *comm)
{
    assert(0);
    return 0;
}

static inline int COLL_neighbor_allgatherv(const void  *sendbuf,
                                           int          sendcount,
                                           COLL_dt_t   *sendtype,
                                           void        *recvbuf,
                                           const int    recvcounts[],
                                           const int    displs[],
                                           COLL_dt_t   *recvtype,
                                           COLL_comm_t *comm)
{
    assert(0);
    return 0;
}

static inline int COLL_neighbor_alltoall(const void  *sendbuf,
                                         int          sendcount,
                                         COLL_dt_t   *sendtype,
                                         void        *recvbuf,
                                         int          recvcount,
                                         COLL_dt_t   *recvtype,
                                         COLL_comm_t *comm)
{
    assert(0);
    return 0;
}

static inline int COLL_neighbor_alltoallv(const void  *sendbuf,
                                          const int    sendcounts[],
                                          const int    sdispls[],
                                          COLL_dt_t   *sendtype,
                                          void        *recvbuf,
                                          const int    recvcounts[],
                                          const int    rdispls[],
                                          COLL_dt_t   *recvtype,
                                          COLL_comm_t *comm)
{
    assert(0);
    return 0;
}

static inline int COLL_neighbor_alltoallw(const void        *sendbuf,
                                          const int          sendcounts[],
                                          const COLL_aint_t *sdispls[],
                                          const COLL_dt_t   *sendtypes[],
                                          void              *recvbuf,
                                          const int          recvcounts[],
                                          const COLL_aint_t *rdispls[],
                                          const COLL_dt_t   *recvtypes[],
                                          COLL_comm_t       *comm)
{
    assert(0);
    return 0;
}

static inline int COLL_ineighbor_allgather(const void   *sendbuf,
                                           int           sendcount,
                                           COLL_dt_t    *sendtype,
                                           void         *recvbuf,
                                           int           recvcount,
                                           COLL_dt_t    *recvtype,
                                           COLL_comm_t  *comm,
                                           COLL_sched_t *s)
{
    assert(0);
    return 0;
}

static inline int COLL_ineighbor_allgatherv(const void   *sendbuf,
                                            int           sendcount,
                                            COLL_dt_t    *sendtype,
                                            void         *recvbuf,
                                            const int     recvcounts[],
                                            const int     displs[],
                                            COLL_dt_t    *recvtype,
                                            COLL_comm_t  *comm,
                                            COLL_sched_t *s)
{
    assert(0);
    return 0;
}

static inline int COLL_ineighbor_alltoall(const void   *sendbuf,
                                          int           sendcount,
                                          COLL_dt_t    *sendtype,
                                          void         *recvbuf,
                                          int           recvcount,
                                          COLL_dt_t    *recvtype,
                                          COLL_comm_t  *comm,
                                          COLL_sched_t *s)
{
    assert(0);
    return 0;
}

static inline int COLL_ineighbor_alltoallv(const void   *sendbuf,
                                           const int     sendcounts[],
                                           const int     sdispls[],
                                           COLL_dt_t    *sendtype,
                                           void         *recvbuf,
                                           const int     recvcounts[],
                                           const int     rdispls[],
                                           COLL_dt_t    *recvtype,
                                           COLL_comm_t  *comm,
                                           COLL_sched_t *s)
{
    assert(0);
    return 0;
}

static inline int COLL_ineighbor_alltoallw(const void        *sendbuf,
                                           const int          sendcounts[],
                                           const COLL_aint_t *sdispls[],
                                           const COLL_dt_t   *sendtypes[],
                                           void              *recvbuf,
                                           const int          recvcounts[],
                                           const COLL_aint_t *rdispls[],
                                           const COLL_dt_t   *recvtypes[],
                                           COLL_comm_t       *comm,
                                           COLL_sched_t      *s)
{
    assert(0);
    return 0;
}

