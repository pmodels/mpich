/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/mpid_buffer.c
 * \brief MPID buffer copy
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpidimpl.h>

#ifndef MPIDI_COPY_BUFFER_SZ
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
void MPIDI_Buffer_copy(
    const void * const sbuf, MPI_Aint scount, MPI_Datatype sdt,                       int * smpi_errno,
          void * const rbuf, MPI_Aint rcount, MPI_Datatype rdt, MPIDI_msg_sz_t * rsz, int * rmpi_errno)
{
    int sdt_contig;
    int rdt_contig;
    MPI_Aint sdt_true_lb, rdt_true_lb;
    MPIDI_msg_sz_t sdata_sz;
    MPIDI_msg_sz_t rdata_sz;
    MPID_Datatype * sdt_ptr;
    MPID_Datatype * rdt_ptr;

    MPI_Aint  sdt_extent;
    MPI_Aint  rdt_extent;

    *smpi_errno = MPI_SUCCESS;
    *rmpi_errno = MPI_SUCCESS;

    /* printf("bufcopy: src count=%d dt=%d\n", scount, sdt); */
    /* printf("bufcopy: dst count=%d dt=%d\n", rcount, rdt); */

    MPIDI_Datatype_get_info(scount, sdt, sdt_contig, sdata_sz, sdt_ptr, sdt_true_lb);
    MPIDI_Datatype_get_info(rcount, rdt, rdt_contig, rdata_sz, rdt_ptr, rdt_true_lb);

    /* --BEGIN ERROR HANDLING-- */
    if (sdata_sz > rdata_sz)
    {
        *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __FUNCTION__, __LINE__, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", sdata_sz, rdata_sz );
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
#if CUDA_AWARE_SUPPORT
      if(MPIDI_Process.cuda_aware_support_on && MPIDI_cuda_is_device_buf(rbuf))
      {
        cudaError_t cudaerr = CudaMemcpy(rbuf + rdt_true_lb, sbuf + sdt_true_lb, sdata_sz, cudaMemcpyHostToDevice);
      }
      else
#endif
        memcpy((char*)rbuf + rdt_true_lb, (const char *)sbuf + sdt_true_lb, sdata_sz);
        *rsz = sdata_sz;
    }
    else if (sdt_contig)
    {
#if CUDA_AWARE_SUPPORT
      // This will need to be done in two steps:
      // 1 - Allocate a temp buffer which is the same size as user buffer and unpack in it.
      // 2 - Copy unpacked data into user buffer from temp buffer.
      if(MPIDI_Process.cuda_aware_support_on && MPIDI_cuda_is_device_buf(rbuf))
      {
        MPID_Datatype_get_extent_macro(rdt, rdt_extent);
        char *buf =  MPIU_Malloc(rdt_extent * rcount);
        memset(buf, 0, rdt_extent * rcount);        
        MPID_Segment seg;
        DLOOP_Offset last;

        MPID_Segment_init(buf, rcount, rdt, &seg, 0);
        last = sdata_sz;
        MPID_Segment_unpack(&seg, 0, &last, (char*)sbuf + sdt_true_lb);
        /* --BEGIN ERROR HANDLING-- */
        if (last != sdata_sz)
        {
            *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __FUNCTION__, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
        }
        /* --END ERROR HANDLING-- */

       *rsz = last;

        
        cudaError_t cudaerr = CudaMemcpy(rbuf + rdt_true_lb, buf, rdt_extent * rcount, cudaMemcpyHostToDevice);

        MPIU_Free(buf);

        goto fn_exit;

      }
#endif

        MPID_Segment seg;
        DLOOP_Offset last;

        MPID_Segment_init(rbuf, rcount, rdt, &seg, 0);
        last = sdata_sz;
        MPID_Segment_unpack(&seg, 0, &last, (char*)sbuf + sdt_true_lb);
        /* --BEGIN ERROR HANDLING-- */
        if (last != sdata_sz)
        {
            *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __FUNCTION__, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
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
            *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __FUNCTION__, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
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
            *smpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, __FUNCTION__, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
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
                *rmpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __FUNCTION__, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
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
