/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/reduce/reduce_algorithms.c
 * \brief ???
 */

#include "mpido_coll.h"

#ifdef USE_CCMI_COLL
/**
 * **************************************************************************
 * \brief "Done" callback for collective allreduce message.
 * **************************************************************************
 */

void
reduce_cb_done(void *clientdata, DCMF_Error_t *err)
{
  volatile unsigned *work_left = (unsigned *) clientdata;
  *work_left = 0;
  MPID_Progress_signal();
}

int
MPIDO_Reduce_global_tree(void * sendbuf,
			 void * recvbuf,
			 int count,
			 DCMF_Dt dcmf_dt,
			 DCMF_Op dcmf_op,
			 MPI_Datatype data_type,
			 int root,
			 MPID_Comm * comm)
{
  int rc, hw_root = comm->vcr[root];
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { reduce_cb_done, (void *) &active };
  rc = DCMF_GlobalAllreduce(&MPIDI_Protocols.globalallreduce,
                            (DCMF_Request_t *)&request,
                            callback,
                            DCMF_MATCH_CONSISTENCY,
                            hw_root,
                            sendbuf,
                            recvbuf,
                            count,
                            dcmf_dt,
                            dcmf_op);
  MPID_PROGRESS_WAIT_WHILE(active);

  return rc;
}

int
MPIDO_Reduce_tree(void * sendbuf,
		  void * recvbuf,
		  int count,
		  DCMF_Dt dcmf_dt,
		  DCMF_Op dcmf_op,
		  MPI_Datatype data_type,
		  int root,
		  MPID_Comm * comm)
{
  int rc, hw_root = comm->vcr[root];
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { reduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  rc = DCMF_Reduce(&MPIDI_CollectiveProtocols.tree_reduce,
                   &request,
                   callback,
                   DCMF_MATCH_CONSISTENCY,
                   geometry,
                   hw_root,
                   sendbuf,
                   recvbuf,
                   count,
                   dcmf_dt,
                   dcmf_op);
  MPID_PROGRESS_WAIT_WHILE(active);

  return rc;
}


int
MPIDO_Reduce_binom(void * sendbuf,
		   void * recvbuf,
		   int count,
		   DCMF_Dt dcmf_dt,
		   DCMF_Op dcmf_op,
		   MPI_Datatype data_type,
		   int root,
		   MPID_Comm * comm)
{
  int rc, hw_root = comm->vcr[root];
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { reduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);

  rc = DCMF_Reduce(&MPIDI_CollectiveProtocols.binomial_reduce,
                   &request,
                   callback,
                   DCMF_MATCH_CONSISTENCY,
                   geometry,
                   hw_root,
                   sendbuf,
                   recvbuf,
                   count,
                   dcmf_dt,
                   dcmf_op);

  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}

int
MPIDO_Reduce_rect(void * sendbuf,
		  void * recvbuf,
		  int count,
		  DCMF_Dt dcmf_dt,
		  DCMF_Op dcmf_op,
		  MPI_Datatype data_type,
		  int root,
		  MPID_Comm * comm)
{
  int rc, hw_root = comm->vcr[root];
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { reduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);

  rc = DCMF_Reduce(&MPIDI_CollectiveProtocols.rectangle_reduce,
                   &request,
                   callback,
                   DCMF_MATCH_CONSISTENCY,
                   geometry,
                   hw_root,
                   sendbuf,
                   recvbuf,
                   count,
                   dcmf_dt,
                   dcmf_op);

  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}



int
MPIDO_Reduce_rectring(void * sendbuf,
		      void * recvbuf,
		      int count,
		      DCMF_Dt dcmf_dt,
		      DCMF_Op dcmf_op,
		      MPI_Datatype data_type,
		      int root,
		      MPID_Comm * comm)
{
  int rc, hw_root = comm->vcr[root];
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { reduce_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);

  rc = DCMF_Reduce(&MPIDI_CollectiveProtocols.rectanglering_reduce,
                   &request,
                   callback,
                   DCMF_MATCH_CONSISTENCY,
                   geometry,
                   hw_root,
                   sendbuf,
                   recvbuf,
                   count,
                   dcmf_dt,
                   dcmf_op);

  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}

#endif /* USE_CCMI_COLL */
