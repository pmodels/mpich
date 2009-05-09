/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file include/mpix.h
 * \brief Blue Gene extensions to the MPI Spec
 *
 * These functions generally use MPI functions and internal APIs to
 * expose extra information relating to the specific system on which
 * the job is running.  This may allow certain hardware specific
 * optimizations to be made.
 */

#ifndef MPIX_H
#define MPIX_H

#if defined(__cplusplus)
extern "C" {
#endif


#include <stdint.h>
#include "mpido_properties.h"

/**
 * \defgroup MPIX IBM Blue Gene extensions to MPICH2
 * \brief These utilities can be used to better understand & manage
 * the BG hardware currently in use.
 */

  /**
   * \ingroup MPIX
   * \brief Create a Cartesian communicator that exactly matches the partition
   *
   * \param[out] cart_comm The new Cartesian communicator
   *
   * \return MPI_SUCCESS or MPI_ERR_TOPOLOGY
   *
   * This is a collective operation on MPI_COMM_WORLD, and will only
   * run successfully on a full partition job (no -np)
   *
   * The communicator is created to match the size of each dimension,
   * the physical coords on each node, and the torus/messh link
   * status.  Because of MPICH2 dimension ordering, the associated
   * arrays (i.e. coords, sizes, and periods) are in [t, z, y, x,]
   * order so that the rank in cart_comm matches the rank in
   * MPI_COMM_WORLD
   */
  int MPIX_Cart_comm_create      (MPI_Comm *cart_comm);

  /**
   * \ingroup MPIX
   * \brief Create a communicator such that all nodes in the same
   * communicator are served by the same I/O node
   *
   * \param[out] pset_comm The new communicator
   *
   * \return MPI_SUCCESS
   *
   *  This is a collective operation on MPI_COMM_WORLD
   */
  int MPIX_Pset_same_comm_create (MPI_Comm *pset_comm);

  /**
   * \ingroup MPIX
   * \brief Create a communicator such that all nodes in the same
   * communicator are served by a different I/O node
   *
   * \param[out] pset_comm The new communicator
   *
   * \return MPI_SUCCESS
   *
   *  This is a collective operation on MPI_COMM_WORLD
   */
  int MPIX_Pset_diff_comm_create (MPI_Comm *pset_comm);


  /**
   * \ingroup MPIX
   * \brief Return the mapped rank based on the physical X, Y, Z, and
   * T coords
   *
   * \param[in]  x    The node's X coord
   * \param[in]  y    The node's Y coord
   * \param[in]  z    The node's Z coord
   * \param[in]  t    The node's T coord
   *
   * \return Mapped rank
   */
  unsigned MPIX_torus2rank       (unsigned x,
                                  unsigned y,
                                  unsigned z,
                                  unsigned t);

  /**
   * \ingroup MPIX
   * \brief Return the physical X, Y, Z, and T coords based on the
   * mapped rank
   *
   * \param[in]  rank The node's mapped rank
   * \param[out] x    The node's X coord
   * \param[out] y    The node's Y coord
   * \param[out] z    The node's Z coord
   * \param[out] t    The node's T coord
   */
  void     MPIX_rank2torus       (unsigned  rank,
                                  unsigned *x,
                                  unsigned *y,
                                  unsigned *z,
                                  unsigned *t);


   /**
   * \ingroup MPIX
   * \brief Return the communicator rank based on the physical X, Y,
   * Z, and T coords
   *
   * \param[in]  comm The communicator to use
   * \param[in]  x    The node's X coord
   * \param[in]  y    The node's Y coord
   * \param[in]  z    The node's Z coord
   * \param[in]  t    The node's T coord
   *
   * \return Communicator rank
   */
  unsigned MPIX_Comm_torus2rank  (MPI_Comm comm,
                                  unsigned x,
                                  unsigned y,
                                  unsigned z,
                                  unsigned t);

  /**
   * \ingroup MPIX
   * \brief Return the physical X, Y, Z, and T coords based on the
   * communicator rank
   *
   * \param[in]  comm The communicator to use
   * \param[in]  rank The node's communicator rank
   * \param[out] x    The node's X coord
   * \param[out] y    The node's Y coord
   * \param[out] z    The node's Z coord
   * \param[out] t    The node's T coord
   */
  void     MPIX_Comm_rank2torus  (MPI_Comm comm,
                                  unsigned rank,
                                  unsigned *x,
                                  unsigned *y,
                                  unsigned *z,
                                  unsigned *t);

  /**
   * \brief Print the current system stack
   *
   * The first frame (this function) is discarded to make the trace look nicer.
   */
  void     MPIX_Dump_stacks();

  /**
   * \brief Gets the values of the bits on the communicator
   *
   * \param[in]  prop The specific property to retrieve its value
   * \param[out] array The values of various bits are stored in this array
   *
   * \return MPI_SUCCESS
   */
  int MPIX_Get_properties(MPI_Comm comm, int * array);

  /**
   * \brief Get the value (0 or 1) of a given property
   *
   * \param[in]  comm The communicator to get the bits values off of
   * \param[in]  prop The specific property to retrieve its value
   * \param[out] result The value of this property (0 or 1)
   *
   * \return MPI_SUCCESS
   */
  int MPIX_Get_property(MPI_Comm comm, int prop, int * result);

  /**
   * \brief Set the value (0 or 1) of a given property
   *
   * \param[in]  comm The communicator to get the bits values off of
   * \param[in]  prop The specific property to retrieve its value
   * \param[out] value The value of this property that is to be set
   *
   * \return MPI_SUCCESS
   */
  int MPIX_Set_property(MPI_Comm comm, int prop, int value);
  
  /**
   * \brief Informs of the collective algorithm used in a collective operation.
   *
   * \param[in]  comm The communicator
   * \param[out]  protocol The string to copy the algorithm into.
   * \param[in] length The size of the protocol array
   *
   * \return MPI_SUCCESS
   */
  int MPIX_Get_coll_protocol(MPI_Comm comm, char * protocol, int length);

#if defined(__cplusplus)
}
#endif

#endif
