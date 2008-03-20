/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_finalize.c
 * \brief Normal job termination code
 */
#include "mpidimpl.h"
#include "pmi.h"

/**
 * \brief Shut down the system
 *
 * At this time, no attempt is made to free memory being used for MPI structures.
 * \return MPI_SUCCESS
*/
int MPID_Finalize()
{
  PMPI_Barrier(MPI_COMM_WORLD);

  /* ------------------------- */
  /* shutdown the statistics   */
  /* ------------------------- */
  MPIDI_Statistics_finalize();

  /* ------------------------- */
  /* shutdown request queues   */
  /* ------------------------- */
  MPIDI_Recvq_finalize();

  return MPI_SUCCESS;
}
