/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/coll_utils.c
 * \brief ???
 */

#include "mpido_coll.h"
#include <stdarg.h>

char * mpido_algorithms[] = {"MPIDO_USE_ABCAST_ALLGATHER",                    
                             "MPIDO_USE_ABINOM_BCAST_ALLGATHER",
                             "MPIDO_USE_ALLTOALL_ALLGATHER",
                             "MPIDO_USE_ARECT_BCAST_ALLGATHER",
                             "MPIDO_USE_ALLREDUCE_ALLGATHER",
                             "MPIDO_USE_BCAST_ALLGATHER",
                             "MPIDO_USE_BINOM_BCAST_ALLGATHER",
                             "MPIDO_USE_RECT_BCAST_ALLGATHER",
                             "MPIDO_USE_RECT_DPUT_ALLGATHER",
                             "MPIDO_USE_MPICH_ALLGATHER",
                             "MPIDO_ALLGATHER_RESERVED1",
                             "MPIDO_USE_PREALLREDUCE_ALLGATHER",
                             "MPIDO_USE_ALLREDUCE_ALLGATHERV",
                             "MPIDO_USE_BCAST_ALLGATHERV",
                             "MPIDO_USE_ALLTOALL_ALLGATHERV",
                             "MPIDO_USE_ABCAST_ALLGATHERV",
                             "MPIDO_USE_ARECT_BCAST_ALLGATHERV",
                             "MPIDO_USE_ABINOM_BCAST_ALLGATHERV",
                             "MPIDO_USE_RECT_BCAST_ALLGATHERV",
                             "MPIDO_USE_BINOM_BCAST_ALLGATHERV",
                             "MPIDO_USE_RECT_DPUT_ALLGATHERV",
                             "MPIDO_USE_MPICH_ALLGATHERV",
                             "MPIDO_ALLGATHERV_RESERVED1",
                             "MPIDO_USE_PREALLREDUCE_ALLGATHERV",
                             "MPIDO_USE_TREE_ALLREDUCE",
                             "MPIDO_USE_CCMI_TREE_ALLREDUCE",
                             "MPIDO_USE_PIPELINED_TREE_ALLREDUCE",
                             "MPIDO_USE_RECT_ALLREDUCE",
                             "MPIDO_USE_RECTRING_ALLREDUCE",
                             "MPIDO_USE_BINOM_ALLREDUCE",
                             "MPIDO_USE_ARECT_ALLREDUCE",
                             "MPIDO_USE_ARECTRING_ALLREDUCE",
                             "MPIDO_USE_ABINOM_ALLREDUCE",
                             "MPIDO_USE_SHORT_ASYNC_RECT_ALLREDUCE",
                             "MPIDO_USE_SHORT_ASYNC_BINOM_ALLREDUCE",
                             "MPIDO_USE_RRING_DPUT_SINGLETH_ALLREDUCE",
                             "MPIDO_USE_MPICH_ALLREDUCE",
                             "MPIDO_ALLREDUCE_RESERVED1",
                             "MPIDO_USE_STORAGE_ALLREDUCE",
                             "MPIDO_USE_TORUS_ALLTOALL",
                             "MPIDO_USE_MPICH_ALLTOALL",
                             "MPIDO_USE_TORUS_ALLTOALLV",
                             "MPIDO_USE_MPICH_ALLTOALLV",
                             "MPIDO_USE_TORUS_ALLTOALLW",
                             "MPIDO_USE_MPICH_ALLTOALLW",
                             "MPIDO_ALLTOALL_RESERVED1",
                             "MPIDO_USE_PREMALLOC_ALLTOALL",
                             "MPIDO_USE_BINOM_BARRIER",
                             "MPIDO_USE_GI_BARRIER",
                             "MPIDO_USE_RECT_BARRIER",
                             "MPIDO_USE_MPICH_BARRIER",
                             "MPIDO_USE_BINOM_LBARRIER",
                             "MPIDO_USE_LOCKBOX_LBARRIER",
                             "MPIDO_USE_RECT_LOCKBOX_LBARRIER",
                             "MPIDO_BARRIER_RESERVED1",
                             "MPIDO_USE_ABINOM_BCAST",
                             "MPIDO_USE_ARECT_BCAST",
                             "MPIDO_USE_BINOM_BCAST",
                             "MPIDO_USE_BINOM_SINGLETH_BCAST",
                             "MPIDO_USE_RECT_BCAST",
                             "MPIDO_USE_RECT_DPUT_BCAST",
                             "MPIDO_USE_RECT_SINGLETH_BCAST",
                             "MPIDO_USE_SCATTER_GATHER_BCAST",
                             "MPIDO_USE_TREE_BCAST",
                             "MPIDO_USE_MPICH_BCAST",
                             "MPIDO_BCAST_RESERVED1",
                             "MPIDO_BCAST_RESERVED2",
                             "MPIDO_USE_MPICH_EXSCAN",
                             "MPIDO_USE_REDUCE_GATHER",
                             "MPIDO_USE_MPICH_GATHER",
                             "MPIDO_GATHER_RESERVED1",
                             "MPIDO_USE_MPICH_GATHERV",
                             "MPIDO_GATHERV_RESERVED1",
                             "MPIDO_USE_BINOM_REDUCE",
                             "MPIDO_USE_CCMI_TREE_REDUCE",
                             "MPIDO_USE_RECT_REDUCE",
                             "MPIDO_USE_RECTRING_REDUCE",
                             "MPIDO_USE_TREE_REDUCE",
                             "MPIDO_USE_MPICH_REDUCE",
                             "MPIDO_USE_ALLREDUCE_REDUCE",
                             "MPIDO_USE_PREMALLOC_REDUCE",
                             "MPIDO_USE_STORAGE_REDUCE",
                             "MPIDO_USE_REDUCESCATTER",
                             "MPIDO_USE_MPICH_REDUCESCATTER",
                             "MPIDO_REDUCESCATTER_RESERVED1",
                             "MPIDO_USE_MPICH_SCAN",
                             "MPIDO_SCAN_RESERVED1",
                             "MPIDO_USE_BCAST_SCATTER",
                             "MPIDO_USE_MPICH_SCATTER",
                             "MPIDO_SCATTER_RESERVED1",
                             "MPIDO_USE_BCAST_SCATTERV",
                             "MPIDO_USE_ALLTOALL_SCATTERV",
                             "MPIDO_USE_ALLREDUCE_SCATTERV",
                             "MPIDO_USE_MPICH_SCATTERV",
                             "MPIDO_SCATTERV_RESERVED1",
                             "MPIDO_USE_PREALLREDUCE_SCATTERV"};

void MPIDO_MSET_INFO(MPIDO_Embedded_Info_Set * set, ...)
{
  va_list arg_ptr;
  int value = 0;

  va_start(arg_ptr, set);

  value = va_arg(arg_ptr, int);
  
  do
  {
    MPIDO_INFO_SET(set, value);
    value = va_arg(arg_ptr, int);      
  }
  while (value > MPIDO_END_ARGS);
  
  va_end(arg_ptr);
}

int MPIDO_INFO_MET(MPIDO_Embedded_Info_Set *s, MPIDO_Embedded_Info_Set *d)
{
  int i, j = sizeof(MPIDO_Embedded_Info_Set) / sizeof(MPIDO_Embedded_Info_Mask);
  for (i = 0; i < j; i++)
    if ((MPIDO_INFO_BITS (s)[i] & MPIDO_INFO_BITS (d)[i]) != 
	MPIDO_INFO_BITS (s)[i])
      return 0;
  return 1;
}

int MPIDO_AllocateAlltoallBuffers(MPID_Comm * comm)
{
  int numprocs = comm->local_size;
  if (!comm->dcmf.sndlen)
    comm->dcmf.sndlen = MPIU_Malloc(numprocs * sizeof(unsigned));
  if (!comm->dcmf.rcvlen)
    comm->dcmf.rcvlen = MPIU_Malloc(numprocs * sizeof(unsigned));
  if (!comm->dcmf.sdispls)
    comm->dcmf.sdispls = MPIU_Malloc(numprocs * sizeof(unsigned));
  if (!comm->dcmf.rdispls)
    comm->dcmf.rdispls = MPIU_Malloc(numprocs * sizeof(unsigned));
  if (!comm->dcmf.sndcounters)
    comm->dcmf.sndcounters = MPIU_Malloc(numprocs * sizeof(unsigned));
  if (!comm->dcmf.rcvcounters)
    comm->dcmf.rcvcounters = MPIU_Malloc(numprocs * sizeof(unsigned));

  if(!comm->dcmf.sndlen || !comm->dcmf.rcvlen ||
     !comm->dcmf.sdispls || !comm->dcmf.rdispls ||
     !comm->dcmf.sndcounters || !comm->dcmf.rcvcounters)
  {
    if (comm->dcmf.sndlen) MPIU_Free(comm->dcmf.sndlen);
    if (comm->dcmf.rcvlen) MPIU_Free(comm->dcmf.rcvlen);
    if (comm->dcmf.sdispls) MPIU_Free(comm->dcmf.sdispls);
    if (comm->dcmf.rdispls) MPIU_Free(comm->dcmf.rdispls);
    if (comm->dcmf.sndcounters) MPIU_Free(comm->dcmf.sndcounters);
    if (comm->dcmf.rcvcounters) MPIU_Free(comm->dcmf.rcvcounters);
    return 0;
  }

  return 1;
}


int MPIDI_IsTreeOp(MPI_Op op, MPI_Datatype datatype)
{
  int rc=0;
  switch(op)
  {
  case MPI_SUM:
  case MPI_MAX:
  case MPI_MIN:
  case MPI_BAND:
  case MPI_BOR:
  case MPI_BXOR:
  case MPI_MAXLOC:
  case MPI_MINLOC:
    rc = 1;
    break;
  default:
    return 0;
  }
  switch(datatype)
  {
  case MPI_FLOAT:
    if(op==MPI_MAX || op == MPI_MIN)
      return 1;
    else 
      return 0;
    break;
  case MPI_LONG_DOUBLE:
  case MPI_UNSIGNED_CHAR:
  case MPI_BYTE:
  case MPI_CHAR:
  case MPI_SIGNED_CHAR:
  case MPI_CHARACTER:
  case MPI_DOUBLE_COMPLEX:
  case MPI_COMPLEX:
    return 0;
  }
  return rc;
}


int MPIDI_ConvertMPItoDCMF(MPI_Op op, DCMF_Op *dcmf_op,
                           MPI_Datatype datatype, DCMF_Dt *dcmf_dt)
{
  int rc = MPIDO_TREE_SUPPORT;

  switch(op)
  {
  case MPI_SUM:
    *dcmf_op = DCMF_SUM;
    break;
  case MPI_PROD:
    if(datatype == MPI_COMPLEX || datatype == MPI_DOUBLE_COMPLEX)
      return MPIDO_NOT_SUPPORTED;
    *dcmf_op = DCMF_PROD;
    rc = MPIDO_TORUS_SUPPORT;
    break;
  case MPI_MAX:
    *dcmf_op = DCMF_MAX;
    break;
  case MPI_MIN:
    *dcmf_op = DCMF_MIN;
    break;
  case MPI_LAND: /* orange book, page 231 */
    if(datatype == MPI_LOGICAL || datatype == MPI_INT ||
       datatype == MPI_INTEGER || datatype == MPI_LONG ||
       datatype == MPI_UNSIGNED|| datatype == MPI_UNSIGNED_LONG)
    {
      *dcmf_op = DCMF_LAND;
      rc = MPIDO_TORUS_SUPPORT;
    }
    else return MPIDO_NOT_SUPPORTED;
    break;
  case MPI_LOR:
    if(datatype == MPI_LOGICAL || datatype == MPI_INT ||
       datatype == MPI_INTEGER || datatype == MPI_LONG ||
       datatype == MPI_UNSIGNED|| datatype == MPI_UNSIGNED_LONG)
    {
      *dcmf_op = DCMF_LOR;
      rc = MPIDO_TORUS_SUPPORT;
    }
    else return MPIDO_NOT_SUPPORTED;
    break;
  case MPI_LXOR:
    if(datatype == MPI_LOGICAL || datatype == MPI_INT ||
       datatype == MPI_INTEGER || datatype == MPI_LONG ||
       datatype == MPI_UNSIGNED|| datatype == MPI_UNSIGNED_LONG)
    {
      *dcmf_op = DCMF_LXOR;
      rc = MPIDO_TORUS_SUPPORT;
    }
    else return MPIDO_NOT_SUPPORTED;
    break;
  case MPI_BAND:
    if(datatype == MPI_LONG || datatype == MPI_INTEGER ||
       datatype == MPI_BYTE || datatype == MPI_UNSIGNED ||
       datatype == MPI_INT  || datatype == MPI_UNSIGNED_LONG)
    {
      *dcmf_op = DCMF_BAND;
    }
    else return MPIDO_NOT_SUPPORTED;
    break;
  case MPI_BOR:
    if(datatype == MPI_LONG || datatype == MPI_INTEGER ||
       datatype == MPI_BYTE || datatype == MPI_UNSIGNED ||
       datatype == MPI_INT  || datatype == MPI_UNSIGNED_LONG)
    {
      *dcmf_op = DCMF_BOR;
    }
    else return MPIDO_NOT_SUPPORTED;
    break;
  case MPI_BXOR:
    if(datatype == MPI_LONG || datatype == MPI_INTEGER ||
       datatype == MPI_BYTE || datatype == MPI_UNSIGNED ||
       datatype == MPI_INT  || datatype == MPI_UNSIGNED_LONG)
    {
      *dcmf_op = DCMF_BXOR;
    }
    else return MPIDO_NOT_SUPPORTED;
    break;
  case MPI_MAXLOC:
    *dcmf_op = DCMF_MAXLOC;
    break;
  case MPI_MINLOC:
    *dcmf_op = DCMF_MINLOC;
    break;
  default:
    *dcmf_dt = DCMF_UNDEFINED_DT;
    *dcmf_op = DCMF_UNDEFINED_OP;
    return MPIDO_NOT_SUPPORTED;
  }
  
  int rc_tmp = rc;
  switch(datatype)
  {
  case MPI_CHAR:
  case MPI_SIGNED_CHAR:
  case MPI_CHARACTER:
    *dcmf_dt = DCMF_SIGNED_CHAR;
    return MPIDO_TORUS_SUPPORT;
    break;

  case MPI_UNSIGNED_CHAR:
  case MPI_BYTE:
    *dcmf_dt = DCMF_UNSIGNED_CHAR;
    return MPIDO_TORUS_SUPPORT;
    break;

  case MPI_INT:
  case MPI_INTEGER:
  case MPI_LONG:
    *dcmf_dt = DCMF_SIGNED_INT;
    break;

  case MPI_UNSIGNED:
  case MPI_UNSIGNED_LONG:
    *dcmf_dt = DCMF_UNSIGNED_INT;
    break;

  case MPI_SHORT:
    *dcmf_dt = DCMF_SIGNED_SHORT;
    break;

  case MPI_UNSIGNED_SHORT:
    *dcmf_dt = DCMF_UNSIGNED_SHORT;
    break;


  case MPI_FLOAT:
  case MPI_REAL:
    *dcmf_dt = DCMF_FLOAT;
    if(op != MPI_MAX || op != MPI_MIN )
      return MPIDO_TORUS_SUPPORT;
    break;

  case MPI_DOUBLE:
  case MPI_DOUBLE_PRECISION:
    *dcmf_dt = DCMF_DOUBLE;
    if(rc == MPIDO_TORUS_SUPPORT)
      rc_tmp = MPIDO_TORUS_SUPPORT;
    else
      rc_tmp = MPIDO_TREE_MIN_SUPPORT;
    break;

  case MPI_LONG_DOUBLE:
    *dcmf_dt = DCMF_LONG_DOUBLE;
    if(op == MPI_LAND || op == MPI_LOR || op == MPI_LXOR)
      return MPIDO_NOT_SUPPORTED;
    return MPIDO_TORUS_SUPPORT;
    break;

  case MPI_LONG_LONG:
    *dcmf_dt = DCMF_UNSIGNED_LONG_LONG;
    break;

  case MPI_DOUBLE_COMPLEX:
    *dcmf_dt = DCMF_DOUBLE_COMPLEX;
    return MPIDO_TORUS_SUPPORT;
    break;

  case MPI_COMPLEX:
    *dcmf_dt = DCMF_SINGLE_COMPLEX;
    return MPIDO_TORUS_SUPPORT;
    break;

  case MPI_LOGICAL:
    *dcmf_dt = DCMF_LOGICAL;
    break;

  case MPI_FLOAT_INT:
    *dcmf_dt = DCMF_LOC_FLOAT_INT;
    break;

  case MPI_DOUBLE_INT:
    *dcmf_dt = DCMF_LOC_DOUBLE_INT;
    break;

  case MPI_LONG_INT:
  case MPI_2INT:
  case MPI_2INTEGER:
    *dcmf_dt = DCMF_LOC_2INT;
    break;

  case MPI_SHORT_INT:
    *dcmf_dt = DCMF_LOC_SHORT_INT;
    break;

  case MPI_2REAL:
    *dcmf_dt = DCMF_LOC_2FLOAT;
    break;

  case MPI_2DOUBLE_PRECISION:
    *dcmf_dt = DCMF_LOC_2DOUBLE;
    break;

  default:
    *dcmf_dt = DCMF_UNDEFINED_DT;
    *dcmf_op = DCMF_UNDEFINED_OP;
    return MPIDO_NOT_SUPPORTED;
  }
  if(rc_tmp == MPIDO_TORUS_SUPPORT || rc_tmp == MPIDO_TREE_MIN_SUPPORT)
    return rc_tmp;
  return rc;
}

