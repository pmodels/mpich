/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/topo/mpid_topo.c
 * \brief Topology setup
 */

#include "mpid_topo.h"

/**
 * \brief Hook function to handle topology-specific optimization during communicator creation
 */
void MPIDI_Topo_Comm_create (MPID_Comm *comm)
{
  MPID_assert (comm!= NULL);
  if (comm->topo_fns) MPIU_Free(comm->topo_fns);
  comm->topo_fns=NULL;

  /* User may disable all topology optimizations */
  if (!MPIDI_Process.optimized.topology) return;

  /* ****************************************** */
  /* Allocate space for the topology pointers */
  /* ****************************************** */
  comm->topo_fns = (MPID_TopoOps *)MPIU_Malloc(sizeof(MPID_TopoOps));
  MPID_assert (comm->topo_fns != NULL);
  memset (comm->topo_fns, 0, sizeof(MPID_TopoOps));
  comm->topo_fns->cartMap = MPID_Cart_map;
}

/**
 * \brief Hook function to handle topology-specific optimization during communicator destruction
 * \note  We want to free the associated topo_fns buffer at this time.
 */
void MPIDI_Topo_Comm_destroy (MPID_Comm *comm)
{
  MPID_assert (comm != NULL);
  if (comm->topo_fns) MPIU_Free(comm->topo_fns);
  comm->topo_fns = NULL;
}
