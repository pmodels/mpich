/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/allreduce/allreduce_algorithms.c
 * \brief ???
 */

#include "mpido_coll.h"

#ifdef USE_CCMI_COLL
/**
 * **************************************************************************
 * \brief "Done" callback for collective allreduce message.
 * **************************************************************************
 */

static void allreduce_cb_done(void *clientdata, DCMF_Error_t *err)
{
  volatile unsigned *work_left = (unsigned *) clientdata;
  *work_left = 0;
  MPID_Progress_signal();
}

int MPIDO_Allreduce_global_tree(void * sendbuf,
				void * recvbuf,
				int count,
				DCMF_Dt dcmf_dt,
				DCMF_Op dcmf_op,
				MPI_Datatype mpi_type,
				MPID_Comm * comm)
{
  int rc;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
  int root = -1;

  rc = DCMF_GlobalAllreduce(&MPIDI_Protocols.globalallreduce,
                            (DCMF_Request_t *)&request,
                            callback,
                            DCMF_MATCH_CONSISTENCY,
                            root,
                            sendbuf,
                            recvbuf,
                            count,
                            dcmf_dt,
                            dcmf_op);
  MPID_PROGRESS_WAIT_WHILE(active);

  return rc;
}

int MPIDO_Allreduce_pipelined_tree(void * sendbuf,
				   void * recvbuf,
				   int count,
				   DCMF_Dt dcmf_dt,
				   DCMF_Op dcmf_op,
				   MPI_Datatype mpi_type,
				   MPID_Comm * comm)
{
  int rc;
  //  unsigned local_alignment = 0;
  //  volatile int global_alignment = 0;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  DCMF_CollectiveProtocol_t *protocol = 
    &(MPIDI_CollectiveProtocols.pipelinedtree_allreduce);
  
  /* Short messages used the unaligned optimizations */
  //  if(count < 1024)
  //    {
  rc = DCMF_Allreduce(protocol,
                      &request,
                      callback,
                      DCMF_MATCH_CONSISTENCY,
                      geometry,
                      sendbuf,
                      recvbuf,
                      count,
                      dcmf_dt,
                      dcmf_op);
  MPID_PROGRESS_WAIT_WHILE(active);
  //     }
  //   else
  //     {
  //       /* First we need to verify alignment */
  //       local_alignment = ( (((unsigned)sendbuf & 0x0f) == 0) &&
  // 			  (((unsigned)recvbuf & 0x0f) == 0) );
  //       global_alignment = 0;
  //       /* Avoid the worst case in ccmi where two different protocols
  //        * alternate on the same communicator, resulting in temporary
  //        * buffers being freed and re-allocated. The fix would be to keep
  //        * the allreducestate persistent across allreduce calls that
  //        * different protocols.  - SK 04/04/08 */
  //       MPIDO_Allreduce_global_tree((char *)&local_alignment,
  // 				  (char *)&global_alignment,
  // 				  1,
  // 				  DCMF_UNSIGNED_INT,
  // 				  DCMF_LAND,
  // 				  MPI_UNSIGNED,
  // 				  comm);
  //
  //       if (global_alignment)
  // 	{ /*src and dst buffers are globally aligned*/
  //          protocol = &MPIDI_CollectiveProtocols.pipelinedtree_dput_allreduce;
  // 	}
  //       active = 1;
  //       rc = DCMF_Allreduce(protocol,
  //                           &request,
  //                           callback,
  //                           DCMF_MATCH_CONSISTENCY,
  //                           geometry,
  //                           sendbuf,
  //                           recvbuf,
  //                           count,
  //                           dcmf_dt,
  //                           dcmf_op);
  //       MPID_PROGRESS_WAIT_WHILE(active);
  //     }
  
  return rc;
  
}

int MPIDO_Allreduce_tree(void * sendbuf,
			 void * recvbuf,
			 int count,
			 DCMF_Dt dcmf_dt,
			 DCMF_Op dcmf_op,
			 MPI_Datatype mpi_type,
			 MPID_Comm * comm)
{
  int rc;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  
  rc = DCMF_Allreduce(&MPIDI_CollectiveProtocols.tree_allreduce,
		      &request,
		      callback,
		      DCMF_MATCH_CONSISTENCY,
		      geometry,
		      sendbuf,
		      recvbuf,
		      count,
		      dcmf_dt,
		      dcmf_op);
  MPID_PROGRESS_WAIT_WHILE(active);
  
  return rc;
}


int MPIDO_Allreduce_binom(void * sendbuf,
			  void * recvbuf,
			  int count,
			  DCMF_Dt dcmf_dt,
			  DCMF_Op dcmf_op,
			  MPI_Datatype mpi_type,
			  MPID_Comm * comm)
{
  int rc;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  
  
  rc = DCMF_Allreduce(&MPIDI_CollectiveProtocols.binomial_allreduce,
                      &request,
                      callback,
                      DCMF_MATCH_CONSISTENCY,
                      geometry,
                      sendbuf,
                      recvbuf,
                      count,
                      dcmf_dt,
                      dcmf_op);

   
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}

int MPIDO_Allreduce_rring_dput_singleth(void *sendbuf,
                                        void *recvbuf,
                                        int count,
                                        DCMF_Dt dcmf_dt,
                                        DCMF_Op dcmf_op,
                                        MPI_Datatype mpi_type,
                                        MPID_Comm *comm)
{
   int rc;
   DCMF_CollectiveRequest_t request;
   volatile unsigned active = 1;
   DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
   DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
   
   rc = DCMF_Allreduce(
      &MPIDI_CollectiveProtocols.rring_dput_allreduce_singleth,
      &request,
      callback,
      DCMF_MATCH_CONSISTENCY,
      geometry,
      sendbuf,
      recvbuf,
      count,
      dcmf_dt,
      dcmf_op);

   
   MPID_PROGRESS_WAIT_WHILE(active);
   return rc;
}
int MPIDO_Allreduce_short_async_binom(void *sendbuf,
                                     void *recvbuf,
                                     int count,
                                     DCMF_Dt dcmf_dt,
                                     DCMF_Op dcmf_op,
                                     MPI_Datatype mpi_type,
                                     MPID_Comm *comm)
{
   int rc;
   DCMF_CollectiveRequest_t request;
   volatile unsigned active = 1;
   DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
   DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  
  
   rc = DCMF_Allreduce(&MPIDI_CollectiveProtocols.short_async_binom_allreduce,
                      &request,
                      callback,
                      DCMF_MATCH_CONSISTENCY,
                      geometry,
                      sendbuf,
                      recvbuf,
                      count,
                      dcmf_dt,
                      dcmf_op);

   
   MPID_PROGRESS_WAIT_WHILE(active);
   return rc;
}


int MPIDO_Allreduce_short_async_rect(void *sendbuf,
                                     void *recvbuf,
                                     int count,
                                     DCMF_Dt dcmf_dt,
                                     DCMF_Op dcmf_op,
                                     MPI_Datatype mpi_type,
                                     MPID_Comm *comm)
{
   int rc;
   DCMF_CollectiveRequest_t request;
   volatile unsigned active = 1;
   DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
   DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  
  
   rc = DCMF_Allreduce(&MPIDI_CollectiveProtocols.short_async_rect_allreduce,
                      &request,
                      callback,
                      DCMF_MATCH_CONSISTENCY,
                      geometry,
                      sendbuf,
                      recvbuf,
                      count,
                      dcmf_dt,
                      dcmf_op);

   
   MPID_PROGRESS_WAIT_WHILE(active);
   return rc;
}

int MPIDO_Allreduce_rect(void * sendbuf,
			 void * recvbuf,
			 int count,
			 DCMF_Dt dcmf_dt,
			 DCMF_Op dcmf_op,
			 MPI_Datatype mpi_type,
			 MPID_Comm * comm)
{
  int rc;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  
  rc = DCMF_Allreduce(&MPIDI_CollectiveProtocols.rectangle_allreduce,
		      &request,
		      callback,
		      DCMF_MATCH_CONSISTENCY,
		      geometry,
		      sendbuf,
		      recvbuf,
		      count,
		      dcmf_dt,
		      dcmf_op);
  
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}



int MPIDO_Allreduce_rectring(void * sendbuf,
			     void * recvbuf,
			     int count,
			     DCMF_Dt dcmf_dt,
			     DCMF_Op dcmf_op,
			     MPI_Datatype mpi_type,
			     MPID_Comm * comm)
{
  int rc;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
   
  rc = DCMF_Allreduce(&MPIDI_CollectiveProtocols.rectanglering_allreduce,
                      &request,
                      callback,
                      DCMF_MATCH_CONSISTENCY,
                      geometry,
                      sendbuf,
                      recvbuf,
                      count,
                      dcmf_dt,
                      dcmf_op);

  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}


int MPIDO_Allreduce_async_binom(void * sendbuf,
                                void * recvbuf,
                                int count,
                                DCMF_Dt dcmf_dt,
                                DCMF_Op dcmf_op,
                                MPI_Datatype mpi_type,
                                MPID_Comm * comm)
{
  int rc;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  
  
  rc = DCMF_Allreduce(&MPIDI_CollectiveProtocols.async_binomial_allreduce,
                      &request,
                      callback,
                      DCMF_MATCH_CONSISTENCY,
                      geometry,
                      sendbuf,
                      recvbuf,
                      count,
                      dcmf_dt,
                      dcmf_op);

   
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}

int MPIDO_Allreduce_async_rect(void * sendbuf,
                               void * recvbuf,
                               int count,
                               DCMF_Dt dcmf_dt,
                               DCMF_Op dcmf_op,
                               MPI_Datatype mpi_type,
                               MPID_Comm * comm)
{
  int rc;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  
  rc = DCMF_Allreduce(&MPIDI_CollectiveProtocols.async_rectangle_allreduce,
		      &request,
		      callback,
		      DCMF_MATCH_CONSISTENCY,
		      geometry,
		      sendbuf,
		      recvbuf,
		      count,
		      dcmf_dt,
		      dcmf_op);
  
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}



int MPIDO_Allreduce_async_rectring(void * sendbuf,
                                   void * recvbuf,
                                   int count,
                                   DCMF_Dt dcmf_dt,
                                   DCMF_Op dcmf_op,
                                   MPI_Datatype mpi_type,
                                   MPID_Comm * comm)
{
  int rc;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { allreduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
   
  rc =DCMF_Allreduce(&MPIDI_CollectiveProtocols.async_ringrectangle_allreduce,
                     &request,
                     callback,
                     DCMF_MATCH_CONSISTENCY,
                     geometry,
                     sendbuf,
                     recvbuf,
                     count,
                     dcmf_dt,
                     dcmf_op);

  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}

#endif /* USE_CCMI_COLL */
