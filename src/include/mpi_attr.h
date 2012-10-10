/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPI_ATTR_H_INCLUDED
#define MPI_ATTR_H_INCLUDED

/* bit 0 distinguishes between pointers (0) and integers (1) */
typedef enum
  { MPIR_ATTR_PTR=0, MPIR_ATTR_AINT=1, MPIR_ATTR_INT=3 } MPIR_AttrType;

#define MPIR_ATTR_KIND(_a) (_a & 0x1)

int MPIR_CommSetAttr( MPI_Comm, int, void *, MPIR_AttrType );
int MPIR_TypeSetAttr( MPI_Datatype, int, void *, MPIR_AttrType );
int MPIR_WinSetAttr( MPI_Win, int, void *, MPIR_AttrType );
int MPIR_CommGetAttr( MPI_Comm, int, void *, int *, MPIR_AttrType );
int MPIR_TypeGetAttr( MPI_Datatype, int, void *, int *, MPIR_AttrType );
int MPIR_WinGetAttr( MPI_Win, int, void *, int *, MPIR_AttrType );

int MPIR_CommGetAttr_fort( MPI_Comm, int, void *, int *, MPIR_AttrType );

#endif /* MPI_ATTR_H_INCLUDED */
