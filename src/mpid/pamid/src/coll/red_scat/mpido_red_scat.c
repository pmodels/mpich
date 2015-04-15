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
 * \file src/coll/red_scat/mpido_red_scat.c
 * \brief ???
 */

/* #define TRACE_ON */
#include <mpidimpl.h>


int MPIDO_Reduce_scatter(const void *sendbuf, 
                 void *recvbuf, 
                 int *recvcounts, 
                 MPI_Datatype datatype,
                 MPI_Op op,
                 MPID_Comm *comm_ptr, 
                 int *mpierrno)

{
    const int rank = comm_ptr->rank;
    const int size = comm_ptr->local_size;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif

    if(unlikely(verbose))
       fprintf(stderr,"Using MPICH reduce_scatter algorithm\n");
#if CUDA_AWARE_SUPPORT
    if(MPIDI_Process.cuda_aware_support_on)
    {
       MPI_Aint dt_extent;
       MPID_Datatype_get_extent_macro(datatype, dt_extent);
       char *scbuf = NULL;
       char *rcbuf = NULL;
       int is_send_dev_buf = MPIDI_cuda_is_device_buf(sendbuf);
       int is_recv_dev_buf = MPIDI_cuda_is_device_buf(recvbuf);
       int i;
       size_t total_buf = 0;
       for(i = 0; i < size; i++)
       {
         total_buf += recvcounts[i];
       }

       if(is_send_dev_buf)
       {
         scbuf = MPIU_Malloc(dt_extent * total_buf);
         cudaError_t cudaerr = CudaMemcpy(scbuf, sendbuf, dt_extent * total_buf, cudaMemcpyDeviceToHost);
         if (cudaSuccess != cudaerr) 
           fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
       }
       else
         scbuf = sendbuf;

       if(is_recv_dev_buf)
       {
         rcbuf = MPIU_Malloc(total_buf * dt_extent);
         if(sendbuf == MPI_IN_PLACE)
         {
           cudaError_t cudaerr = CudaMemcpy(rcbuf, recvbuf, dt_extent * total_buf, cudaMemcpyDeviceToHost);
           if (cudaSuccess != cudaerr)
             fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
         }
         else
           memset(rcbuf, 0, total_buf * dt_extent);
       }
       else
         rcbuf = recvbuf;

       int cuda_res =  MPIR_Reduce_scatter(scbuf, rcbuf, recvcounts, datatype, op, comm_ptr, mpierrno);
       if(is_send_dev_buf)MPIU_Free(scbuf);
       if(is_recv_dev_buf)
       {
         cudaError_t cudaerr = CudaMemcpy(recvbuf, rcbuf, dt_extent * total_buf, cudaMemcpyHostToDevice);
         if (cudaSuccess != cudaerr)
           fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
         MPIU_Free(rcbuf);
       }
       return cuda_res;
    }
    else
#endif
    return MPIR_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, mpierrno);

}



int MPIDO_Reduce_scatter_block(const void *sendbuf, 
                 void *recvbuf, 
                 int recvcount, 
                 MPI_Datatype datatype,
                 MPI_Op op,
                 MPID_Comm *comm_ptr, 
                 int *mpierrno)

{
    const int rank = comm_ptr->rank;
    const int size = comm_ptr->local_size;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif
    if(unlikely(verbose))
       fprintf(stderr,"Using MPICH reduce_scatter algorithm\n");
#if CUDA_AWARE_SUPPORT
    if(MPIDI_Process.cuda_aware_support_on)
    {
       MPI_Aint dt_extent;
       MPID_Datatype_get_extent_macro(datatype, dt_extent);
       char *scbuf = NULL;
       char *rcbuf = NULL;
       int is_send_dev_buf = MPIDI_cuda_is_device_buf(sendbuf);
       int is_recv_dev_buf = MPIDI_cuda_is_device_buf(recvbuf);
       int i;
       if(is_send_dev_buf)
       {
         scbuf = MPIU_Malloc(dt_extent * recvcount * size);
         cudaError_t cudaerr = CudaMemcpy(scbuf, sendbuf, dt_extent * recvcount * size, cudaMemcpyDeviceToHost);
         if (cudaSuccess != cudaerr) 
           fprintf(stderr, "cudaMemcpy failed: %s recvbuf: %p scbuf: %p is_send_dev_buf: %d is_recv_dev_buf: %p sendbuf: %p\n", CudaGetErrorString(cudaerr), recvbuf, scbuf, is_send_dev_buf,is_recv_dev_buf, sendbuf );
       }
       else
         scbuf = sendbuf;

       if(is_recv_dev_buf)
       {
         rcbuf = MPIU_Malloc(dt_extent * recvcount * size);
         if(sendbuf == MPI_IN_PLACE)
         {
           cudaError_t cudaerr = CudaMemcpy(rcbuf, recvbuf, dt_extent * recvcount * size, cudaMemcpyDeviceToHost);
           if (cudaSuccess != cudaerr)
             fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
         }
         else
           memset(rcbuf, 0, recvcount * size * dt_extent);
       }
       else
         rcbuf = recvbuf;

       int cuda_res;
       if(comm_ptr->comm_kind == MPID_INTRACOMM)
         cuda_res =  MPIR_Reduce_scatter_block_intra(scbuf, rcbuf, recvcount, datatype, op, comm_ptr, mpierrno);
       else 
         cuda_res =  MPIR_Reduce_scatter_block_inter(scbuf, rcbuf, recvcount, datatype, op, comm_ptr, mpierrno);
       if(is_send_dev_buf)MPIU_Free(scbuf);
       if(is_recv_dev_buf)
       {
         cudaError_t cudaerr = CudaMemcpy(recvbuf, rcbuf, dt_extent * recvcount * size, cudaMemcpyHostToDevice);
         if (cudaSuccess != cudaerr)
           fprintf(stderr, "cudaMemcpy failed: %s recvbuf: %p rcbuf: %p is_send_dev_buf: %d is_recv_dev_buf: %p sendbuf: %p\n", CudaGetErrorString(cudaerr), recvbuf, rcbuf, is_send_dev_buf,is_recv_dev_buf, sendbuf );
         MPIU_Free(rcbuf);
       }
       return cuda_res;
    }
    else
#endif
       if(comm_ptr->comm_kind == MPID_INTRACOMM)
         return MPIR_Reduce_scatter_block_intra(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, mpierrno);
       else 
         return MPIR_Reduce_scatter_block_inter(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, mpierrno);

}


