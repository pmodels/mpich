/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIBSEND_H_INCLUDED
#define MPIBSEND_H_INCLUDED

/* This file is separated out as it is used by the configure script to
 * find the Bsend overhead value. */

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
 *                and datatype (*could* be used incase the data is already 
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

/* Used to communication the type of bsend */
typedef enum {
    BSEND = 0,
    IBSEND = 1,
    BSEND_INIT = 2
} MPIR_Bsend_kind_t;

struct MPID_Request;
struct MPID_Comm;

/* BsendMsg is used to hold all of the message particulars in case a
   request is not currently available */
typedef struct MPIR_Bsend_msg {
    void         *msgbuf;
    MPI_Aint     count;
    MPI_Datatype dtype;
    int          tag;
    struct MPID_Comm    *comm_ptr;
    int          dest;
} MPIR_Bsend_msg_t;

/* BsendData describes a bsend request */
typedef struct MPIR_Bsend_data {
    size_t            size;            /* size that is available for data */
    size_t            total_size;      /* total size of this segment,
                                          including all headers */
    struct MPIR_Bsend_data *next, *prev;
    MPIR_Bsend_kind_t kind;
    struct MPID_Request  *request;
    MPIR_Bsend_msg_t  msg;
    double            alignpad;        /* make sure that the struct
                                          shares double alignment */
} MPIR_Bsend_data_t;

#endif /* MPIBSEND_H_INCLUDED */
