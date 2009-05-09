/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_finalize.c
 * \brief Normal job termination code
 */
#include "mpidimpl.h"
extern void STAR_FreeMem(MPID_Comm *);

/**
 * \brief Shut down the system
 *
 * At this time, no attempt is made to free memory being used for MPI structures.
 * \return MPI_SUCCESS
*/
int MPID_Finalize()
{
  MPID_Comm * comm;
  MPID_Comm_get_ptr(MPI_COMM_WORLD, comm);

  PMPI_Barrier(MPI_COMM_WORLD);

  STAR_FreeMem(comm);

  /* ------------------------- */
  /* shutdown the statistics   */
  /* ------------------------- */
  MPIDI_Statistics_finalize();

  /* ------------------------- */
  /* shutdown request queues   */
  /* ------------------------- */
  MPIDI_Recvq_finalize();

  DCMF_Messager_finalize();

  return MPI_SUCCESS;
}
