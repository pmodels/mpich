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
 * \file include/mpix.h
 * \brief PAMI device extensions to the MPI Spec
 *
 * These functions generally use MPI functions and internal APIs to
 * expose extra information relating to the specific system on which
 * the job is running.  This may allow certain hardware specific
 * optimizations to be made.
 */

#ifndef __include_mpix_h__
#define __include_mpix_h__

#if defined(__cplusplus)
extern "C" {
#endif


  /**
   * \brief Print the current system stack
   *
   * The first frame (this function) is discarded to make the trace look nicer.
   */
  void MPIX_Dump_stacks();

  void MPIX_Progress_poke();

  /**
   * \brief Wait for network to quiesce
   *
   * \praram[in] timeout	Maximum time, Seconds, to wait. 0 for internal default
   * \retval	MPI_SUCCESS	Network appears to be quiesced
   * \retval	MPI_ERR_PENDING	Network did not quiesce
   * \retval	MPI_ERR_OTHER	Encounter error(s), network state unknown
   */
  int MPIX_Progress_quiesce(double timeout);

#define MPIX_TORUS_MAX_DIMS 5 /* This is the maximum physical size of the torus */
  typedef struct
  {
/* These fields will be used on all platforms. */
    unsigned prank;    /**< Physical rank of the node (irrespective of mapping) */
    unsigned psize;    /**< Size of the partition (irrespective of mapping) */
    unsigned ppn;      /**< Processes per node */
    unsigned coreID;   /**< Process id; values monotonically increase from 0..63 */

    unsigned clockMHz; /**< Frequency in MegaHertz */
    unsigned memSize;  /**< Size of the core memory in MB */

/* These fields are only set on torus platforms (i.e. Blue Gene) */
    unsigned torus_dimension;              /**< Actual dimension for the torus */
    unsigned Size[MPIX_TORUS_MAX_DIMS];    /**< Max coordinates on the torus */
    unsigned Coords[MPIX_TORUS_MAX_DIMS];  /**< This node's coordinates */
    unsigned isTorus[MPIX_TORUS_MAX_DIMS]; /**< Do we have wraparound links? */

/* These fields are only set on systems using Blue Gene IO psets. */
    unsigned rankInPset;
    unsigned sizeOfPset;
    unsigned idOfPset;
  } MPIX_Hardware_t;

  /**
   * \brief Determine the rank-in-COMM_WORLD of the process associated with rank-in-comm
   *
   * Fortran interface:
   *   MPIX_COMM_RANK2GLOBAL (INTEGER comm, INTEGER crank, INTEGER grank, INTEGER ierr)
   *
   * \param[in]  comm  The communicator associated with the input rank
   * \param[in]  crank The rank-in-comm
   * \param[out] grank The rank-in-COMM_WORLD (AKA Global rank)
   * \return MPI_SUCCESS on success, an error on failure detection.
   */
  int MPIX_Comm_rank2global(MPI_Comm comm, int crank, int *grank);

  /**
   * \brief Fill in an MPIX_Hardware_t structure
   * \param[in] hw A pointer to an MPIX_Hardware_t structure to be filled in
   */
  int MPIX_Hardware(MPIX_Hardware_t *hw);


  /* These functions only exist on torus platforms (i.e. Blue Gene) */

  /**
   * \brief Determine the number of physical hardware dimensions
   * \param[out] numdimensions The number of torus dimensions
   * \note This does NOT include the core+thread ID, so if you plan on
   *       allocating an array based on this information, you'll need to
   *       add 1 to the value returned here
   */
  int MPIX_Torus_ndims(int *numdim);
  /**
   * \brief Convert an MPI rank into physical coordinates plus core ID
   * \param[in] rank The MPI Rank
   * \param[out] coords An array of size hw.torus_dimensions+1. The last
   *                   element of the returned array is the core+thread ID
   */
  int MPIX_Rank2torus(int rank, int *coords);
  /**
   * \brief Convert a set of coordinates (physical+core/thread) to an MPI rank
   * \param[in] coords An array of size hw.torus_dimensions+1. The last element
   *                   should be the core+thread ID (0..63).
   * \param[out] rank The MPI rank cooresponding to the coords array passed in
   */
  int MPIX_Torus2rank(int *coords, int *rank);

   /**
   * \brief Optimize/deoptimize a communicator by adding/stripping
   *        platform specific optimizations (i.e. class routes support
   *        for efficient bcast/reductions).
   * \param[in] comm     MPI communicator
   * \param[in] optimize Optimize(1) or deoptimize(0) the communicator
   */
  int MPIX_Comm_update(MPI_Comm comm, int optimize);

   /**
    * \brief Return the most recently used collective protocol name
    * param[in] comm The communicator that collective was issued on
    * param[out] protocol Storage space for the string name
    * param[in] length Length available for the string name.
    * Note: Max internal length is 100
    */
  int MPIX_Get_last_algorithm_name(MPI_Comm comm, char *protocol, int length);

  /**
   * \brief Create a communicator such that all nodes in the same
   *        communicator are served by the same I/O node
   *
   * Fortran interface:
   *   MPIX_PSET_SAME_COMM_CREATE(INTEGER pset_comm, INTEGER ierr)
   *
   * \note This is a collective operation on MPI_COMM_WORLD
   *
   * \param [out] pset_comm The new communicator
   *
   * \return MPI status code
   */
  int MPIX_Pset_same_comm_create (MPI_Comm *pset_comm);

  /**
   * \brief Create a communicator such that all nodes in the same
   *        communicator are served by a different I/O node
   *
   * Fortran interface:
   *   MPIX_PSET_DIFF_COMM_CREATE(INTEGER pset_comm, INTEGER ierr)
   *
   * \note This is a collective operation on MPI_COMM_WORLD
   *
   * \param [out] pset_comm The new communicator
   *
   * \return MPI status code
   */
  int MPIX_Pset_diff_comm_create (MPI_Comm *pset_comm);

  /**
   * \brief Create a communicator such that all nodes in the same
   *        communicator are served by the same I/O node
   *
   * Fortran interface:
   *   MPIX_PSET_SAME_COMM_CREATE (INTEGER parent_comm, INTEGER pset_comm, INTEGER ierr)
   *
   * \note This is a collective operation on the parent communicator.
   *
   * \param [in]  parent_comm The parent communicator
   * \param [out] pset_comm   The new communicator
   *
   * \return MPI status code
   */
  int MPIX_Pset_same_comm_create_from_parent (MPI_Comm parent_comm, MPI_Comm *pset_comm);

  /**
   * \brief Create a communicator such that all nodes in the same
   *        communicator are served by a different I/O node
   *
   * Fortran interface:
   *   MPIX_PSET_DIFF_COMM_CREATE (INTEGER parent_comm, INTEGER pset_comm, INTEGER ierr)
   *
   * \note This is a collective operation on the parent communicator
   *
   * \param [in]  parent_comm The parent communicator
   * \param [out] pset_comm   The new communicator
   *
   * \return MPI status code
   */
  int MPIX_Pset_diff_comm_create_from_parent (MPI_Comm parent_comm, MPI_Comm *pset_comm);

  /**
   * \brief Retrieve the identifier of the io node associated with the local compute node.
   *
   * The I/O node identifier is a unique number, yet it is not a monotonically
   * increasing integer; such as a rank in a communicator. Multiple ranks, and
   * multiple compute nodes, can be associated with the same I/O link.
   *
   * Fortran interface:
   *   MPIX_IO_NODE_ID (INTEGER io_node_id)
   *
   * \note On BG/Q two 'bridge' compute nodes are connected to each io node;
   *       each unique io node identifier will be associated with at most two
   *       different io link identifiers.
   *
   * \return I/O node identifier
   */
  int MPIX_IO_node_id ();

  /**
   * \brief Retrieve the identifier of the io link associated with the local compute node.
   *
   * The I/O link identifier is a unique number, yet it is not a monotonically
   * increasing integer; such as a rank in a communicator. Multiple ranks, and
   * multiple compute nodes, can be associated with the same I/O link.
   *
   * Fortran interface:
   *   MPIX_IO_LINK_ID (INTEGER io_link_id)
   *
   * \return I/O link identifier
   */
  int MPIX_IO_link_id ();

  /**
   * \brief Retrieve the number of hops to the io node associated with the local compute node.
   *
   * The distance to the I/O node is the number of hops on the torus from the
   * local compute node to the associated I/O node. The 'torus' vs 'mesh'
   * attribute of each dimention is considered when determining the distance.
   *
   * Fortran interface:
   *   MPIX_IO_DISTANCE (INTEGER io_distance)
   *
   * \note On BG/Q the 'bridge' compute nodes are those nodes that are closest
   *       to the I/O node and will have a distance of '1'.
   *
   * \return number of hops to the I/O node
   */
  int MPIX_IO_distance ();

  /**
   * \brief Retrieve information about the I/O node associated with the
   *        local compute node.
   *
   * \deprecated This function has been replaced with MPIX_IO_link_id() and
   *             MPIX_IO_distance()
   *
   * \see MPIX_IO_link_id
   * \see MPIX_IO_distance
   * \see MPIX_IO_node_id
   *
   * \param [out] io_node_route_id     The unique I/O node route identifier
   * \param [out] distance_to_io_node  The number of hops to the I/O node
   */
  void MPIX_Pset_io_node (int *io_node_route_id, int *distance_to_io_node);

  /**
   * \brief Create a Cartesian communicator that exactly matches the partition
   *
   * This is a collective operation on MPI_COMM_WORLD, and will only run
   * successfully on a full partition job (no -np)
   *
   * The communicator is created to match the size of each dimension, the
   * physical coords on each node, and the torus/mesh link status.
   *
   * Because of MPICH dimension ordering, the associated arrays (i.e. coords,
   * sizes, and periods) are in [a, b, c, d, e, t] order. Consequently, when
   * using the default ABCDET mapping, the rank in cart_comm will match the rank
   * in MPI_COMM_WORLD. However, when using a non-default mapping or a mapfile
   * the ranks will be different.
   *
   * Fortran interface:
   *   MPIX_CART_COMM_CREATE (INTEGER cart_comm, INTEGER ierr)
   *
   * \param [out] cart_comm The new Cartesian communicator
   *
   * \return MPI_SUCCESS or MPI_ERR_TOPOLOGY
   */
  int MPIX_Cart_comm_create (MPI_Comm *cart_comm);


#if defined(__cplusplus)
}
#endif

#endif
