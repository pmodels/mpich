/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/impl/mpid_buffer.c
 * \brief MPID buffer copy
 */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#if !defined(MPIDI_COPY_BUFFER_SZ)
#define MPIDI_COPY_BUFFER_SZ 16384
#endif

/**
 * \brief MPID buffer copy
 *
 * Implements non-contiguous buffers correctly.
 *
 * \param[in]  sbuf       The address of the input buffer
 * \param[in]  scount     The number of elements in that buffer
 * \param[in]  sdt        The datatype of those elements
 * \param[out] smpi_errno Returns errors
 * \param[in]  rbuf       The address of the output buffer
 * \param[out] rcount     The number of elements in that buffer
 * \param[in]  rdt        The datatype of those elements
 * \param[out] rsz        The size of the ouput data
 * \param[out] rmpi_errno Returns errors
 */
void MPIDI_DCMF_Buffer_copy(
    const void * const sbuf, int scount, MPI_Datatype sdt, int * smpi_errno,
    void * const rbuf, int rcount, MPI_Datatype rdt, MPIDI_msg_sz_t * rsz,
    int * rmpi_errno)
{
    int sdt_contig;
    int rdt_contig;
    MPI_Aint sdt_true_lb, rdt_true_lb;
    MPIDI_msg_sz_t sdata_sz;
    MPIDI_msg_sz_t rdata_sz;
    MPID_Datatype * sdt_ptr;
    MPID_Datatype * rdt_ptr;

    *smpi_errno = MPI_SUCCESS;
    *rmpi_errno = MPI_SUCCESS;


    //    printf ("bufcopy: src count=%d dt =%d\n", scount, sdt);
    // printf ("bufcopy: dst count=%d dt=%d\n", rcount, rdt);

    MPIDI_Datatype_get_info(scount, sdt, sdt_contig, sdata_sz, sdt_ptr, sdt_true_lb);
    MPIDI_Datatype_get_info(rcount, rdt, rdt_contig, rdata_sz, rdt_ptr, rdt_true_lb);

    /* --BEGIN ERROR HANDLING-- */
    if (sdata_sz > rdata_sz)
    {
        *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPIDI_DCMF_Buffer_copy", __LINE__, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", sdata_sz, rdata_sz );
        sdata_sz = rdata_sz;
    }
    /* --END ERROR HANDLING-- */

    if (sdata_sz == 0)
    {
        *rsz = 0;
        goto fn_exit;
    }

    if (sdt_contig && rdt_contig)
    {
        memcpy((char *)rbuf + rdt_true_lb, (const char *)sbuf + sdt_true_lb, sdata_sz);
        *rsz = sdata_sz;
    }
    else if (sdt_contig)
    {
        MPID_Segment seg;
        DLOOP_Offset last;

        MPID_Segment_init(rbuf, rcount, rdt, &seg, 0);
        last = sdata_sz;
        MPID_Segment_unpack(&seg, 0, &last, (char*)sbuf + sdt_true_lb);
        /* --BEGIN ERROR HANDLING-- */
        if (last != sdata_sz)
        {
            *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPIDI_DCMF_Buffer_copy", __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
        }
        /* --END ERROR HANDLING-- */

        *rsz = last;
    }
    else if (rdt_contig)
    {
        MPID_Segment seg;
        DLOOP_Offset last;

        MPID_Segment_init(sbuf, scount, sdt, &seg, 0);
        last = sdata_sz;
        MPID_Segment_pack(&seg, 0, &last, (char*)rbuf + rdt_true_lb);
        /* --BEGIN ERROR HANDLING-- */
        if (last != sdata_sz)
        {
            *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPIDI_DCMF_Buffer_copy", __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
        }
        /* --END ERROR HANDLING-- */

        *rsz = last;
    }
    else
    {
        char * buf;
        MPIDI_msg_sz_t buf_off;
        MPID_Segment sseg;
        MPIDI_msg_sz_t sfirst;
        MPID_Segment rseg;
        MPIDI_msg_sz_t rfirst;

        buf = MPIU_Malloc(MPIDI_COPY_BUFFER_SZ);
        /* --BEGIN ERROR HANDLING-- */
        if (buf == NULL)
        {
            *smpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, "MPIDI_DCMF_Buffer_copy", __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            *rmpi_errno = *smpi_errno;
            *rsz = 0;
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        MPID_Segment_init(sbuf, scount, sdt, &sseg, 0);
        MPID_Segment_init(rbuf, rcount, rdt, &rseg, 0);

        sfirst = 0;
        rfirst = 0;
        buf_off = 0;

        for(;;)
        {
            DLOOP_Offset last;
            char * buf_end;

            if (sdata_sz - sfirst > MPIDI_COPY_BUFFER_SZ - buf_off)
            {
                last = sfirst + (MPIDI_COPY_BUFFER_SZ - buf_off);
            }
            else
            {
                last = sdata_sz;
            }

            MPID_Segment_pack(&sseg, sfirst, &last, buf + buf_off);
            /* --BEGIN ERROR HANDLING-- */
            MPID_assert(last > sfirst);
            /* --END ERROR HANDLING-- */

            buf_end = buf + buf_off + (last - sfirst);
            sfirst = last;

            MPID_Segment_unpack(&rseg, rfirst, &last, buf);
            /* --BEGIN ERROR HANDLING-- */
            MPID_assert(last > rfirst);
            /* --END ERROR HANDLING-- */

            rfirst = last;

            if (rfirst == sdata_sz)
            {
                /* successful completion */
                break;
            }

            /* --BEGIN ERROR HANDLING-- */
            if (sfirst == sdata_sz)
            {
                /* datatype mismatch -- remaining bytes could not be unpacked */
                *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPIDI_DCMF_Buffer_copy", __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
                break;
            }
            /* --END ERROR HANDLING-- */

            buf_off = sfirst - rfirst;
            if (buf_off > 0)
            {
                memmove(buf, buf_end - buf_off, buf_off);
            }
        }

        *rsz = rfirst;
        MPIU_Free(buf);
    }

  fn_exit:
    return;
}
