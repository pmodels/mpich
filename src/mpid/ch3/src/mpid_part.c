/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

int MPID_Psend_init(const void *buf, int partitions, MPI_Count count, MPI_Datatype datatype,
                    int dest, int tag, MPIR_Comm *comm, MPIR_Info *info,
                    MPIR_Request **request )
{
    MPIR_Assert(0);
    return 0;
}

int MPID_Precv_init(void *buf, int partitions, MPI_Count count, MPI_Datatype datatype,
                    int source, int tag, MPIR_Comm *comm, MPIR_Info *info,
                    MPIR_Request **request )
{
    MPIR_Assert(0);
    return 0;
}

int MPID_Pready_range(int partition_low, int partition_high, MPIR_Request *sreq)
{
    MPIR_Assert(0);
    return 0;
}

int MPID_Pready_list(int length, const int array_of_partitions[], MPIR_Request *sreq)
{
    MPIR_Assert(0);
    return 0;
}

int MPID_Parrived(MPIR_Request *rreq, int partition, int *flag)
{
    MPIR_Assert(0);
    return 0;
}
