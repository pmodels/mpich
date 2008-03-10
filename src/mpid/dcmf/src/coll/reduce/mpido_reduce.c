/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/reduce/mpido_reduce.c
 * \brief ???
 */

#include "mpido_coll.h"

#pragma weak PMPIDO_Reduce = MPIDO_Reduce

/**
 * **************************************************************************
 * \brief "Done" callback for collective allreduce message.
 * **************************************************************************
 */

static void cb_done (void *clientdata)
{
   volatile unsigned *work_left = (unsigned *) clientdata;
   *work_left = 0;
   MPID_Progress_signal();

   return;
}


static int tree_global_reduce(void * sendbuf,
                              void * recvbuf,
                              int count,
                              DCMF_Dt dcmf_dt,
                              DCMF_Op dcmf_op,
                              int root,
                              DCMF_Geometry_t * geometry)
{
   int rc;
   DCMF_CollectiveRequest_t request;
   volatile unsigned active = 1;
   DCMF_Callback_t callback = { cb_done, (void *) &active };
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

static int tree_reduce(void * sendbuf,
                       void * recvbuf,
                       int count,
                       DCMF_Dt dcmf_dt,
                       DCMF_Op dcmf_op,
                       int root,
                       DCMF_Geometry_t * geometry)
{
   int rc;
   DCMF_CollectiveRequest_t request;
   volatile unsigned active = 1;
   DCMF_Callback_t callback = { cb_done, (void *) &active };
   rc = DCMF_Reduce(&MPIDI_CollectiveProtocols.reduce.tree,
                    &request,
                    callback,
                    DCMF_MATCH_CONSISTENCY,
                    geometry,
                    root,
                    sendbuf,
                    recvbuf,
                    count,
                    dcmf_dt,
                    dcmf_op);
   MPID_PROGRESS_WAIT_WHILE(active);

   return rc;
}


static int binom_reduce(void * sendbuf,
                        void * recvbuf,
                        int count,
                        DCMF_Dt dcmf_dt,
                        DCMF_Op dcmf_op,
                        int root,
                        DCMF_Geometry_t * geometry)
{
   int rc;
   DCMF_CollectiveRequest_t request;
   volatile unsigned active = 1;
   DCMF_Callback_t callback = { cb_done, (void *) &active };

   rc = DCMF_Reduce(&MPIDI_CollectiveProtocols.reduce.binomial,
                    &request,
                    callback,
                    DCMF_MATCH_CONSISTENCY,
                    geometry,
                    root,
                    sendbuf,
                    recvbuf,
                    count,
                    dcmf_dt,
                    dcmf_op);

   MPID_PROGRESS_WAIT_WHILE(active);
   return rc;
}

static int rect_reduce(void * sendbuf,
                       void * recvbuf,
                       int count,
                       DCMF_Dt dcmf_dt,
                       DCMF_Op dcmf_op,
                       int root,
                       DCMF_Geometry_t * geometry)
{
   int rc;
   DCMF_CollectiveRequest_t request;
   volatile unsigned active = 1;
   DCMF_Callback_t callback = { cb_done, (void *) &active };

   rc = DCMF_Reduce(&MPIDI_CollectiveProtocols.reduce.rectangle,
                    &request,
                    callback,
                    DCMF_MATCH_CONSISTENCY,
                    geometry,
                    root,
                    sendbuf,
                    recvbuf,
                    count,
                    dcmf_dt,
                    dcmf_op);

   MPID_PROGRESS_WAIT_WHILE(active);
   return rc;
}



static int rectring_reduce(void * sendbuf,
                       void * recvbuf,
                       int count,
                       DCMF_Dt dcmf_dt,
                       DCMF_Op dcmf_op,
                       int root,
                       DCMF_Geometry_t * geometry)
{
   int rc;
   DCMF_CollectiveRequest_t request;
   volatile unsigned active = 1;
   DCMF_Callback_t callback = { cb_done, (void *) &active };

   rc = DCMF_Reduce(&MPIDI_CollectiveProtocols.reduce.rectanglering,
                    &request,
                    callback,
                    DCMF_MATCH_CONSISTENCY,
                    geometry,
                    root,
                    sendbuf,
                    recvbuf,
                    count,
                    dcmf_dt,
                    dcmf_op);

   MPID_PROGRESS_WAIT_WHILE(active);
   return rc;
}



int MPIDO_Reduce(void * sendbuf,
                 void * recvbuf,
                 int count,
                 MPI_Datatype datatype,
                 MPI_Op op,
                 int root,
                 MPID_Comm * comm_ptr)
{
   int dt_contig, dt_extent, rc;
   unsigned treeavail, rectavail, binomavail, rectringavail;

   MPID_Datatype *dt_ptr;
   MPI_Aint dt_true_lb;

   DCMF_Dt dcmf_dt = DCMF_UNDEFINED_DT;
   DCMF_Op dcmf_op = DCMF_UNDEFINED_OP;

   if(count == 0)
      return MPI_SUCCESS;

   treeavail = comm_ptr->dcmf.reducetree | comm_ptr->dcmf.reduceccmitree;

   rc = MPIDI_ConvertMPItoDCMF(op, &dcmf_op, datatype, &dcmf_dt);

   extern int DCMF_TREE_SMP_SHORTCUT;

   if(rc == 0 && treeavail && comm_ptr->local_size > 2)
   {
      if(sendbuf == MPI_IN_PLACE)
         sendbuf = recvbuf;
      if(DCMF_TREE_SMP_SHORTCUT && comm_ptr->dcmf.reducetree)
        rc = tree_global_reduce(sendbuf,
                                recvbuf,
                                count,
                                dcmf_dt,
                                dcmf_op,
                                comm_ptr->vcr[root]->lpid,
                                &comm_ptr->dcmf.geometry);
      else
        rc = tree_reduce(sendbuf,
                         recvbuf,
                         count,
                         dcmf_dt,
                         dcmf_op,
                         comm_ptr->vcr[root]->lpid,
                         &comm_ptr->dcmf.geometry);
      return rc;
   }

   /* quick exit conditions */
   if(comm_ptr->comm_kind != MPID_INTRACOMM)
      return MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr);

   /* check geometry for possibilities */
   rectavail = MPIDI_CollectiveProtocols.reduce.userect &&
               DCMF_Geometry_analyze(&comm_ptr->dcmf.geometry,
                     &MPIDI_CollectiveProtocols.reduce.rectangle);

   rectringavail = MPIDI_CollectiveProtocols.reduce.userectring &&
               DCMF_Geometry_analyze(&comm_ptr->dcmf.geometry,
                     &MPIDI_CollectiveProtocols.reduce.rectanglering);

   binomavail = MPIDI_CollectiveProtocols.reduce.usebinom &&
                DCMF_Geometry_analyze(&comm_ptr->dcmf.geometry,
                     &MPIDI_CollectiveProtocols.reduce.binomial);



   MPIDI_Datatype_get_info(count,
                           datatype,
                           dt_contig,
                           dt_extent,
                           dt_ptr,
                           dt_true_lb);


   rc = MPIDI_ConvertMPItoDCMF(op, &dcmf_op, datatype, &dcmf_dt);
   /* return conditions */
   if(
   // unsupported datatype or op
   rc == -1 ||
   // no optimized topologies for this geometry
   (!rectavail && !binomavail && !rectringavail) ||
   // return to mpich for 1 processor reduce
   (comm_ptr -> local_size <=2))
   {
      return MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr);
   }

   /* at this point, decide which network/algorithm we are using based on
    * benchmark data, the op, the type, etc, etc
    * until then just pick rectangle then binomial based on availability*/
   unsigned usingbinom=1 && binomavail;
   unsigned usingrect=1 && rectavail;
   unsigned usingrectring=1 && rectringavail;


   if(sendbuf != MPI_IN_PLACE)
   {
     //      int err =
     //         MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
     //      if (err) return err;
   }
   else
     sendbuf = recvbuf;

   if(usingrect)
   {
//      fprintf(stderr,"rect reduce count: %d, dt: %d, op: %d, send: 0x%x, recv: 0x%x\n", count, dcmf_dt, dcmf_op, sendbuf, recvbuf);
      rc = rect_reduce(sendbuf,
                       recvbuf,
                       count,
                       dcmf_dt,
                       dcmf_op,
                       comm_ptr->vcr[root]->lpid,
                       &comm_ptr->dcmf.geometry);
   }
   else if(usingbinom)
   {
//      fprintf(stderr,"binom reduce count: %d, dt: %d, op: %d, send: 0x%x, recv: 0x%x\n", count, dcmf_dt, dcmf_op, sendbuf, recvbuf);
      rc = binom_reduce(sendbuf,
                        recvbuf,
                        count,
                        dcmf_dt,
                        dcmf_op,
                        comm_ptr->vcr[root]->lpid,
                        &comm_ptr->dcmf.geometry);
   }
   else if(usingrectring)
   {
//      fprintf(stderr,"rectring reduce count: %d, dt: %d, op: %d, send: 0x%x, recv: 0x%x\n", count, dcmf_dt, dcmf_op, sendbuf, recvbuf);
      rc = rectring_reduce(sendbuf,
                       recvbuf,
                       count,
                       dcmf_dt,
                       dcmf_op,
                       comm_ptr->vcr[root]->lpid,
                       &comm_ptr->dcmf.geometry);
   }

   return rc;

}
