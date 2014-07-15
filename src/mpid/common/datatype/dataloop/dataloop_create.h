/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DATALOOP_CREATE_H
#define DATALOOP_CREATE_H

/* Dataloop construction functions */
void PREPEND_PREFIX(Dataloop_create)(MPI_Datatype type,
				     DLOOP_Dataloop **dlp_p,
				     DLOOP_Size *dlsz_p,
				     int *dldepth_p,
				     int flag);
int PREPEND_PREFIX(Dataloop_create_contiguous)(DLOOP_Count count,
					       MPI_Datatype oldtype,
					       DLOOP_Dataloop **dlp_p,
					       DLOOP_Size *dlsz_p,
					       int *dldepth_p,
					       int flag);
int PREPEND_PREFIX(Dataloop_create_vector)(DLOOP_Count count,
					   DLOOP_Size blocklength,
					   MPI_Aint stride,
					   int strideinbytes,
					   MPI_Datatype oldtype,
					   DLOOP_Dataloop **dlp_p,
					   DLOOP_Size *dlsz_p,
					   int *dldepth_p,
					   int flag);
int PREPEND_PREFIX(Dataloop_create_blockindexed)(DLOOP_Count count,
						 DLOOP_Size blklen,
						 const void *disp_array,
						 int dispinbytes,
						 MPI_Datatype oldtype,
						 DLOOP_Dataloop **dlp_p,
						 DLOOP_Size *dlsz_p,
						 int *dldepth_p,
						 int flag);
/* we bump up the size of the blocklength array because create_struct might use
 * create_indexed in an optimization, and in course of doing so, generate a
 * request of a large blocklength. */
int PREPEND_PREFIX(Dataloop_create_indexed)(DLOOP_Count count,
					    const DLOOP_Size *blocklength_array,
					    const void *displacement_array,
					    int dispinbytes,
					    MPI_Datatype oldtype,
					    DLOOP_Dataloop **dlp_p,
					    DLOOP_Size *dlsz_p,
					    int *dldepth_p,
					    int flag);
int PREPEND_PREFIX(Dataloop_create_struct)(DLOOP_Count count,
					   const int *blklen_array,
					   const MPI_Aint *disp_array,
					   const MPI_Datatype *oldtype_array,
					   DLOOP_Dataloop **dlp_p,
					   DLOOP_Size *dlsz_p,
					   int *dldepth_p,
					   int flag);
int PREPEND_PREFIX(Dataloop_create_pairtype)(MPI_Datatype type,
					     DLOOP_Dataloop **dlp_p,
					     DLOOP_Size *dlsz_p,
					     int *dldepth_p,
					     int flag);

/* Helper functions for dataloop construction */
int PREPEND_PREFIX(Type_convert_subarray)(int ndims,
					  int *array_of_sizes, 
					  int *array_of_subsizes,
					  int *array_of_starts,
					  int order,
					  MPI_Datatype oldtype, 
					  MPI_Datatype *newtype);
int PREPEND_PREFIX(Type_convert_darray)(int size,
					int rank,
					int ndims, 
					int *array_of_gsizes,
					int *array_of_distribs, 
					int *array_of_dargs,
					int *array_of_psizes, 
					int order,
					MPI_Datatype oldtype, 
					MPI_Datatype *newtype);

DLOOP_Count PREPEND_PREFIX(Type_indexed_count_contig)(DLOOP_Count count,
                                                      const DLOOP_Count *blocklength_array,
                                                      const void *displacement_array,
                                                      int dispinbytes,
                                                      DLOOP_Offset old_extent);

DLOOP_Count PREPEND_PREFIX(Type_blockindexed_count_contig)(DLOOP_Count count,
                                                           DLOOP_Count blklen,
                                                           const void *disp_array,
                                                           int dispinbytes,
                                                           DLOOP_Offset old_extent);

#endif
