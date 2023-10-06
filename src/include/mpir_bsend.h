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
 * both the user's message and information that may be needed to send that
 * message.  In addition, space within that buffer must be allocated, so
 * additional information is required to manage that space allocation.
 * In the following, the term "segment" denotes a fragment of the user buffer
 * that has been allocated either to free (unused) space or to a particular
 * user message.
 *
 * The following datastructures are used:
 *
 *  BsendBuff_t - Describes an attached user buffer.
 *
 *  BsendMsg_t  - Describes a user message, including the values of tag
 *                and datatype (*could* be used in case the data is already
 *                contiguous; see below)
 *  BsendData_t - Describes a segment of the user buffer.  This data structure
 *                contains a BsendMsg_t for segments that contain a user
 *                message.  Each BsendData_t segment belongs to one of
 *                two lists: avail (unused and free), and active (currently
 *                sending).
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

/* BsendBuffer is the structure that describes the overall Bsend buffer */
/*
 * We use separate buffer and origbuffer because we may need to align
 * the buffer (we *could* always memcopy the header to an aligned region,
 * but it is simpler to just align it internally.  This does increase the
 * BSEND_OVERHEAD, but that is already relatively large.  We could instead
 * make sure that the initial header was set at an aligned location (
 * taking advantage of the "alignpad"), but this would require more changes.
 */
typedef struct MPII_BsendBuffer {
    void *buffer;               /* Pointer to the beginning of the user-
                                 * provided buffer */
    MPI_Aint buffer_size;       /* Size of the user-provided buffer */
    void *origbuffer;           /* Pointer to the buffer provided by
                                 * the user */
    MPI_Aint origbuffer_size;   /* Size of the buffer as provided
                                 * by the user */
    MPII_Bsend_data_t *avail;   /* Pointer to the first available block
                                 * of space */
    MPII_Bsend_data_t *pending; /* Pointer to the first message that
                                 * could not be sent because of a
                                 * resource limit (e.g., no requests
                                 * available) */
    MPII_Bsend_data_t *active;  /* Pointer to the first active (sending)
                                 * message */
} MPII_BsendBuffer;

/* Function Prototypes for the bsend utility functions */
int MPIR_Process_bsend_finalize(void);
int MPIR_Comm_bsend_finalize(struct MPIR_Comm *comm_ptr);
int MPIR_Session_bsend_finalize(struct MPIR_Session *session);
int MPIR_Bsend_isend(const void *buf, int count, MPI_Datatype dtype,
                     int dest, int tag, MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIR_Bsend_isend(const void *, int, MPI_Datatype, int, int, MPIR_Comm *, MPIR_Request **);

#endif /* MPII_BSEND_H_INCLUDED */
