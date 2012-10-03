


/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"


#undef FUNCNAME
#define FUNCNAME MPI_nem_ptl_pack_byte
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void MPI_nem_ptl_pack_byte(MPID_Segment *segment, MPI_Aint first, MPI_Aint last, void *buf,
                           MPID_nem_ptl_pack_overflow_t *overflow)
{
    MPI_Aint my_last;
    MPI_Aint bytes;
    char *end;
    MPIDI_STATE_DECL(MPID_STATE_MPI_NEM_PTL_PACK_BYTE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPI_NEM_PTL_PACK_BYTE);

    /* first copy out of overflow buffer */
    if (overflow->len) {
        if (overflow->len <= last-first) {
            MPIU_Memcpy(buf, &overflow->buf[overflow->offset], overflow->len);
            first += overflow->len;
            buf = (char *)buf + overflow->len;
            overflow->len = 0;
            if (last == first)
                goto fn_exit;
        } else {
            MPIU_Memcpy(buf, &overflow->buf[overflow->offset], last-first);
            overflow->offset += overflow->len - (last-first);
            overflow->len -= last-first;
            goto fn_exit;
        }
    }

    /* unpack as much as we can into buf */
    my_last = last;
    MPID_Segment_pack(segment, first, &my_last, buf);
    
    if (my_last == last)
        /* buf is completely filled */
        goto fn_exit;

    /* remember where the unfilled section starts and how large it is */
    end = &((char *)buf)[my_last-first];
    bytes = last - my_last;

    /* unpack some into the overflow */
    first = my_last;
    my_last += sizeof(overflow->buf);
    MPID_Segment_pack(segment, first, &my_last, overflow->buf);
    MPIU_Assert(my_last - first);
    
    /* fill in the rest of buf */
    MPIU_Memcpy(end, overflow->buf, bytes);

    /* save the beginning of the offset buffer and its length */
    overflow->offset = bytes;
    overflow->len = my_last-first - bytes;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPI_NEM_PTL_PACK_BYTE);
    return;
}

/* MPID_nem_ptl_unpack_byte -- this function unpacks a contig buffer handling
   the case where there are bytes left in the buffer that are less than a basic
   datatype.  Those bytes are copied into an overflow buffer so that the next
   time unpack_byte is called they can be "prepended" to that buffer before
   doing a segment_unpack.  Note, that means that after calling unpack_byte,
   the last few bytes may not have been copied into the target buffer. */
#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_unpack_byte
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_unpack_byte(MPID_Segment *segment, MPI_Aint first, MPI_Aint last, void *buf,
                             MPID_nem_ptl_pack_overflow_t *overflow)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint my_last;
    MPI_Aint bytes;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_UNPACK_BYTE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_UNPACK_BYTE);

    if (overflow->len) {
        /* if there is data in the overflow buffer, then we already know that
           it's too small to unpack with this datatype, so we return an error */
        MPIU_ERR_CHKANDJUMP(overflow->len >= last-first, mpi_errno, MPI_ERR_OTHER, "**dtypemismatch");

        /* bytes is the number of additional bytes to copy into the overflow buffer */
        bytes = sizeof(overflow->buf) - overflow->len;
        if (bytes > last-first)
            bytes = last-first;

        MPIU_Memcpy(overflow->buf + overflow->len, buf, bytes);

        /* Now there are (overflow->len + bytes) bytes in the overflow buffer.
           We try to unpack as many as we can */
        my_last = first + overflow->len + bytes;
        MPID_Segment_unpack(segment, first, &my_last, buf);

        /* we update the buf pointer to the  */
        buf = ((char *)buf) + my_last-first - overflow->len;
        first = my_last;

        overflow->len = 0;
    }

    if (first == last)
        goto fn_exit;

    my_last = last;
    MPID_Segment_unpack(segment, first, &my_last, buf);

    if (my_last == last)
        /* buf has been completely unpacked */
        goto fn_exit;

    bytes = last - my_last;
    
    /* copy remaining bytes int overflow buffer */
    MPIU_Memcpy(overflow->buf, &((char *)buf)[my_last-first], bytes);

    overflow->offset = 0;
    overflow->len = bytes;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_UNPACK_BYTE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
