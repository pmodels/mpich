/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_CSEL_H_INCLUDED
#define MPIR_CSEL_H_INCLUDED

#include "json.h"
#include "coll_impl.h"

typedef enum {
    MPIR_CSEL_COLL_TYPE__ALLGATHER = 0,
    MPIR_CSEL_COLL_TYPE__ALLGATHERV,
    MPIR_CSEL_COLL_TYPE__ALLREDUCE,
    MPIR_CSEL_COLL_TYPE__ALLTOALL,
    MPIR_CSEL_COLL_TYPE__ALLTOALLV,
    MPIR_CSEL_COLL_TYPE__ALLTOALLW,
    MPIR_CSEL_COLL_TYPE__BARRIER,
    MPIR_CSEL_COLL_TYPE__BCAST,
    MPIR_CSEL_COLL_TYPE__EXSCAN,
    MPIR_CSEL_COLL_TYPE__GATHER,
    MPIR_CSEL_COLL_TYPE__GATHERV,
    MPIR_CSEL_COLL_TYPE__IALLGATHER,
    MPIR_CSEL_COLL_TYPE__IALLGATHERV,
    MPIR_CSEL_COLL_TYPE__IALLREDUCE,
    MPIR_CSEL_COLL_TYPE__IALLTOALL,
    MPIR_CSEL_COLL_TYPE__IALLTOALLV,
    MPIR_CSEL_COLL_TYPE__IALLTOALLW,
    MPIR_CSEL_COLL_TYPE__IBARRIER,
    MPIR_CSEL_COLL_TYPE__IBCAST,
    MPIR_CSEL_COLL_TYPE__IEXSCAN,
    MPIR_CSEL_COLL_TYPE__IGATHER,
    MPIR_CSEL_COLL_TYPE__IGATHERV,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLGATHER,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLGATHERV,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALL,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALLV,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALLW,
    MPIR_CSEL_COLL_TYPE__IREDUCE,
    MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER,
    MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER_BLOCK,
    MPIR_CSEL_COLL_TYPE__ISCAN,
    MPIR_CSEL_COLL_TYPE__ISCATTER,
    MPIR_CSEL_COLL_TYPE__ISCATTERV,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLGATHER,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLGATHERV,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALL,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALLV,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALLW,
    MPIR_CSEL_COLL_TYPE__REDUCE,
    MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER,
    MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER_BLOCK,
    MPIR_CSEL_COLL_TYPE__SCAN,
    MPIR_CSEL_COLL_TYPE__SCATTER,
    MPIR_CSEL_COLL_TYPE__SCATTERV,
    MPIR_CSEL_COLL_TYPE__END,
} MPIR_Csel_coll_type_e;

typedef struct {
    MPIR_Csel_coll_type_e coll_type;
    MPIR_Comm *comm_ptr;

    union {
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            MPI_Aint recvcount;
            MPI_Datatype recvtype;
        } allgather, iallgather, neighbor_allgather, ineighbor_allgather;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *displs;
            MPI_Datatype recvtype;
        } allgatherv, iallgatherv, neighbor_allgatherv, ineighbor_allgatherv;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
        } allreduce, iallreduce;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            MPI_Aint recvcount;
            void *recvbuf;
            MPI_Datatype recvtype;
        } alltoall, ialltoall, neighbor_alltoall, ineighbor_alltoall;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *sdispls;
            MPI_Datatype sendtype;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *rdispls;
            MPI_Datatype recvtype;
        } alltoallv, ialltoallv, neighbor_alltoallv, ineighbor_alltoallv;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *sdispls;
            const MPI_Datatype *sendtypes;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *rdispls;
            const MPI_Datatype *recvtypes;
        } alltoallw, ialltoallw;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *sdispls;
            const MPI_Datatype *sendtypes;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *rdispls;
            const MPI_Datatype *recvtypes;
        } neighbor_alltoallw, ineighbor_alltoallw;
        struct {
            int dummy;          /* some compiler (suncc) doesn't like empty struct */
        } barrier, ibarrier;
        struct {
            void *buffer;
            MPI_Aint count;
            MPI_Datatype datatype;
            int root;
        } bcast, ibcast;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
        } exscan, iexscan;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            MPI_Aint recvcount;
            void *recvbuf;
            MPI_Datatype recvtype;
            int root;
        } gather, igather, scatter, iscatter;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *displs;
            MPI_Datatype recvtype;
            int root;
        } gatherv, igatherv;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
            int root;
        } reduce, ireduce;
        struct {
            const void *sendbuf;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            MPI_Datatype datatype;
            MPI_Op op;
        } reduce_scatter, ireduce_scatter;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint recvcount;
            MPI_Datatype datatype;
            MPI_Op op;
        } reduce_scatter_block, ireduce_scatter_block;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
        } scan, iscan;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *displs;
            MPI_Datatype sendtype;
            MPI_Aint recvcount;
            void *recvbuf;
            MPI_Datatype recvtype;
            int root;
        } scatterv, iscatterv;
    } u;
} MPIR_Csel_coll_sig_s;

int MPIR_Csel_create_from_file(const char *json_file,
                               void *(*create_container) (struct json_object *), void **csel);
int MPIR_Csel_create_from_buf(const char *json,
                              void *(*create_container) (struct json_object *), void **csel);
int MPIR_Csel_free(void *csel);
int MPIR_Csel_prune(void *root_csel, MPIR_Comm * comm_ptr, void **comm_csel);
void *MPIR_Csel_search(void *csel, MPIR_Csel_coll_sig_s coll_sig);

#endif /* MPIR_CSEL_H_INCLUDED */
