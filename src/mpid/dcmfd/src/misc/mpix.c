/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpix.c
 * \brief Blue Gene extensions to the MPI Spec
 */

#include "mpidimpl.h"
#include "mpix.h"

#pragma weak PMI_torus2rank = MPIX_torus2rank
unsigned MPIX_torus2rank (unsigned        x,
                         unsigned        y,
                         unsigned        z,
                         unsigned        t)
{
  unsigned rank;
  int rc = DCMF_Messager_torus2rank(x, y, z, t, &rank);
  if(rc == DCMF_SUCCESS) return rank;
  else return (unsigned)-1;
}

#pragma weak PMI_Comm_torus2rank = MPIX_Comm_torus2rank
unsigned MPIX_Comm_torus2rank (MPI_Comm comm,
                              unsigned x,
                              unsigned y,
                              unsigned z,
                              unsigned t)
{
  int rank, worldrank = MPIX_torus2rank(x, y, z, t);
  if (comm == MPI_COMM_WORLD) rank = worldrank;
  else
    {
      MPI_Group group_a, worldgroup;
      MPI_Comm_group (comm, &group_a);
      MPI_Comm_group (MPI_COMM_WORLD, &worldgroup);
      MPI_Group_translate_ranks (worldgroup, 1, &worldrank, group_a, &rank);
    }
  return rank;
}

#pragma weak PMI_rank2torus = MPIX_rank2torus
void MPIX_rank2torus (unsigned        rank,
                     unsigned       *x,
                     unsigned       *y,
                     unsigned       *z,
                     unsigned       *t)
{
  DCMF_Messager_rank2torus (rank, x, y, z, t);
}

#pragma weak PMI_Comm_rank2torus = MPIX_Comm_rank2torus
void MPIX_Comm_rank2torus(MPI_Comm comm,
                         unsigned rank,
                         unsigned *x,
                         unsigned *y,
                         unsigned *z,
                         unsigned *t)
{
  int worldrank;
  if (comm == MPI_COMM_WORLD) worldrank = rank;
  else
    {
      MPI_Group group_a, worldgroup;
      MPI_Comm_group (comm, &group_a);
      MPI_Comm_group (MPI_COMM_WORLD, &worldgroup);
      MPI_Group_translate_ranks (group_a, 1, (int*)&rank, worldgroup, &worldrank);
    }
  MPIX_rank2torus (worldrank, x, y, z, t);
}


/**
 * \brief Compare each elemt of two four-element arrays
 * \param[in] A The first array
 * \param[in] B The first array
 * \returns MPI_SUCCESS (does not return on failure)
 */
#define CMP_4(A,B)                              \
({                                              \
  assert(A[0] == B[0]);                         \
  assert(A[1] == B[1]);                         \
  assert(A[2] == B[2]);                         \
  assert(A[3] == B[3]);                         \
  MPI_SUCCESS;                                  \
})
#pragma weak PMI_Cart_comm_create = MPIX_Cart_comm_create
int MPIX_Cart_comm_create(MPI_Comm *cart_comm)
{
  int result;
  int rank, numprocs,
      dims[4],
      wrap[4],
      coords[4];
  int new_rank,
      new_dims[4],
      new_wrap[4],
      new_coords[4];
  DCMF_Hardware_t pers;


  *cart_comm = MPI_COMM_NULL;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  DCMF_Hardware(&pers);


  dims[3]   = pers.xSize;
  dims[2]   = pers.ySize;
  dims[1]   = pers.zSize;
  dims[0]   = pers.tSize;

/* This only works if MPI_COMM_WORLD is the full partition */
  if (dims[3] * dims[2] * dims[1] * dims[0] != numprocs)
    return MPI_ERR_TOPOLOGY;

  wrap[3]   = pers.xTorus;
  wrap[2]   = pers.yTorus;
  wrap[1]   = pers.zTorus;
  wrap[0]   = pers.tTorus;

  coords[3] = pers.xCoord;
  coords[2] = pers.yCoord;
  coords[1] = pers.zCoord;
  coords[0] = pers.tCoord;


  result = MPI_Cart_create(
    MPI_COMM_WORLD,
    4,
    dims,
    wrap,
    0,
    cart_comm
  );
  if (result != MPI_SUCCESS) return result;


  MPI_Comm_rank(*cart_comm, &new_rank);
  MPI_Cart_get (*cart_comm, 4, new_dims, new_wrap, new_coords);

  CMP_4(dims,   new_dims);
  CMP_4(wrap,   new_wrap);
  CMP_4(coords, new_coords);

  return MPI_SUCCESS;
}

#pragma weak PMI_Pset_same_comm_create = MPIX_Pset_same_comm_create
int MPIX_Pset_same_comm_create(MPI_Comm *pset_comm)
{
  int key, color;
  DCMF_Hardware_t pers;

  DCMF_Hardware(&pers);
  /*  All items of the same color are grouped in the same communicator  */
  color = pers.idOfPset;
  /*  The key implies the new rank  */
  key   = pers.rankInPset*pers.tSize + pers.tCoord;

  return MPI_Comm_split(MPI_COMM_WORLD, color, key, pset_comm);
}

#pragma weak PMI_Pset_diff_comm_create = MPIX_Pset_diff_comm_create
int MPIX_Pset_diff_comm_create(MPI_Comm *pset_comm)
{
  int key, color;
  DCMF_Hardware_t pers;

  DCMF_Hardware(&pers);
  /*  The key implies the new rank  */
  key   = pers.idOfPset;
  /*  All items of the same color are grouped in the same communicator  */
  color = pers.rankInPset*pers.tSize + pers.tCoord;

  return MPI_Comm_split(MPI_COMM_WORLD, color, key, pset_comm);
}
