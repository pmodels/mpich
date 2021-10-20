/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPII_BSEND_H_INCLUDED
#define MPII_BSEND_H_INCLUDED

/*
 * Description of the Bsend data structures.
 *
 * Bsend is buffered send; a buffer, provided by the user, is used to store
 * both the user's message and information that my be needed to send that
 * message.  In addition, space within that buffer must be allocated, so
 * additional information is required to manage that space allocation.
 * In the following, the term "segment" denotes a fragment of the user buffer
 * that has been allocated either to free (unused) space or to a particular
 * user message.
 *
 * The following datastructures are used:
 *
 *  BsendMsg_t  - Describes a user message, including the values of tag
 *                and datatype (*could* be used in case the data is already
 *                contiguous; see below)
 *  BsendData_t - Describes a segment of the user buffer.  This data structure
 *                contains a BsendMsg_t for segments that contain a user
 *                message.  Each BsendData_t segment belongs to one of
 *                three lists: avail (unused and free), active (currently
 *                sending) and pending (contains a user message that has
 *                not begun sending because of some resource limit, such
 *                as no more MPID requests available).
 *  BsendBuffer - This global structure contains pointers to the user buffer
 *                and the three lists, along with the size of the user buffer.
 *
 */

struct MPIR_Request;
struct MPIR_Comm;

/* BsendMsg is used to hold all of the message particulars in case a
   request is not currently available */
typedef struct MPII_Bsend_msg {
    void *msgbuf;
    MPI_Aint count;
    MPI_Datatype dtype;
    int tag;
    struct MPIR_Comm *comm_ptr;
    int dest;
} MPII_Bsend_msg_t;

/* BsendData describes a bsend request */
/* NOTE: The size of this structure determines the value of
 * MPI_BSEND_OVERHEAD in mpi.h, which is part of the MPICH ABI. */
typedef struct MPII_Bsend_data {
    size_t size;                /* size that is available for data */
    size_t total_size;          /* total size of this segment,
                                 * including all headers */
    struct MPII_Bsend_data *next, *prev;
    int dummy;                  /* dummy var to preserve ABI compatibility */
    struct MPIR_Request *request;
    MPII_Bsend_msg_t msg;
    double alignpad;            /* make sure that the struct
                                 * shares double alignment */
} MPII_Bsend_data_t;

/* Function Prototypes for the bsend utility functions */
int MPIR_Bsend_attach(void *, MPI_Aint);
int MPIR_Bsend_detach(void *, MPI_Aint *);
int MPIR_Bsend_isend(const void *, int, MPI_Datatype, int, int, MPIR_Comm *, MPIR_Request **);
int MPIR_Bsend_free_req_seg(MPIR_Request *);

#endif /* MPII_BSEND_H_INCLUDED */
