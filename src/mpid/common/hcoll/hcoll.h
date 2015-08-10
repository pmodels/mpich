#ifndef _HCOLL_H_
#define _HCOLL_H_

#include "mpidimpl.h"
#include "hcoll/api/hcoll_api.h"
#include "hcoll/api/hcoll_constants.h"

extern int world_comm_destroying;

int hcoll_comm_create(MPID_Comm * comm, void *param);
int hcoll_comm_destroy(MPID_Comm * comm, void *param);

int hcoll_Barrier(MPID_Comm * comm_ptr, MPIR_Errflag_t *err);
int hcoll_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                MPID_Comm * comm_ptr, MPIR_Errflag_t *err);
int hcoll_Allgather(const void *sbuf, int scount, MPI_Datatype sdtype,
                    void *rbuf, int rcount, MPI_Datatype rdtype, MPID_Comm * comm_ptr, MPIR_Errflag_t *err);
int hcoll_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                    MPI_Op op, MPID_Comm * comm_ptr, MPIR_Errflag_t *err);

int hcoll_Ibarrier_req(MPID_Comm * comm_ptr, MPID_Request ** request);
int hcoll_Ibcast_req(void *buffer, int count, MPI_Datatype datatype, int root,
                     MPID_Comm * comm_ptr, MPID_Request ** request);
int hcoll_Iallgather_req(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         int recvcount, MPI_Datatype recvtype, MPID_Comm * comm_ptr,
                         MPID_Request ** request);
int hcoll_Iallreduce_req(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                         MPI_Op op, MPID_Comm * comm_ptr, MPID_Request ** request);
int hcoll_do_progress(int *made_progress);

#endif
