/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDI_SELF_H_INCLUDED
#define MPIDI_SELF_H_INCLUDED

#define MPIDI_is_self_comm(comm) \
    (comm->remote_size == 1 && comm->comm_kind == MPIR_COMM_KIND__INTRACOMM)

int MPIDI_Self_init(void);
int MPIDI_Self_finalize(void);
int MPIDI_Self_isend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int rank, int tag,
                     MPIR_Comm * comm, int attr, MPIR_Request ** request);
int MPIDI_Self_irecv(void *buf, MPI_Aint count, MPI_Datatype datatype, int rank, int tag,
                     MPIR_Comm * comm, int attr, MPIR_Request ** request);
int MPIDI_Self_iprobe(int rank, int tag, MPIR_Comm * comm, int attr,
                      int *flag, MPI_Status * status);
int MPIDI_Self_improbe(int rank, int tag, MPIR_Comm * comm, int attr,
                       int *flag, MPIR_Request ** message, MPI_Status * status);
int MPIDI_Self_imrecv(char *buf, MPI_Aint count, MPI_Datatype datatype,
                      MPIR_Request * message, MPIR_Request ** request);
int MPIDI_Self_cancel(MPIR_Request * request);

#endif /* MPIDI_SELF_H_INCLUDED */
