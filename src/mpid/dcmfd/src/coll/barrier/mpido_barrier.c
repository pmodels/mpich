/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/barrier/mpido_barrier.c
 * \brief ???
 */

#include "mpido_coll.h"
#include "mpidi_coll_prototypes.h"


#ifdef USE_CCMI_COLL

#pragma weak PMPIDO_Barrier = MPIDO_Barrier

/**
 * **************************************************************************
 * \brief General MPIDO_Barrier() implementation
 * **************************************************************************
 */

int MPIDO_Barrier(MPID_Comm * comm)
{
  int rc;
  MPIDO_Embedded_Info_Set * properties = &(comm->dcmf.properties);
  
  if(MPIDO_INFO_ISSET(properties, MPIDO_USE_MPICH_BARRIER))
  {
    comm->dcmf.last_algorithm = MPIDO_USE_MPICH_BARRIER;
    return MPIR_Barrier_intra(comm);
  }
  if (MPIDO_INFO_ISSET(properties, MPIDO_USE_GI_BARRIER))
  {
    comm->dcmf.last_algorithm = MPIDO_USE_GI_BARRIER;
    rc = MPIDO_Barrier_gi(comm);
  }
  //else if (MPIDO_INFO_ISSET(properties, MPIDO_USE_RECT_BARRIER))
  //  rc = MPIDO_Barrier_rect(comm);
  else
  {
    //    comm->dcmf.last_algorithm = MPIDO_USE_DCMF_BARRIER;
    rc = MPIDO_Barrier_dcmf(comm);
  }
  if (rc != DCMF_SUCCESS)
  {
    comm->dcmf.last_algorithm = MPIDO_USE_MPICH_BARRIER;
    rc = MPIR_Barrier_intra(comm);
  }
  return rc;
}

#else /* !USE_CCMI_COLL */

int MPIDO_Barrier(MPID_Comm *comm_ptr)
{
  MPID_abort();
}
#endif /* !USE_CCMI_COLL */
