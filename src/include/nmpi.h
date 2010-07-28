/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPICH_NMPI_H_INCLUDED
#define MPICH_NMPI_H_INCLUDED

/* 
 * This file provides a flexible way to map MPI routines that are used
 * within the MPICH2 implementation to either the MPI or PMPI versions.
 * In normal use, it is appropriate to use PMPI, but in some cases, 
 * using the MPI routines instead is desired.
 */

/*
 * When adding names, make sure that you add them to both branches.
 * This allows the --enable-nmpi-as-mpi switch in configure to 
 * cause the internal routines to use the MPI versions; this is good for 
 * using tools that use the MPI profiling interface to collect data (e.g.,
 * for people who what to see what happens when the collective operations 
 * are implemented over MPI point-to-point.
 */

#ifdef USE_MPI_FOR_NMPI
#define NMPI_Get_count MPI_Get_count
#define NMPI_Pack MPI_Pack
#define NMPI_Pack_size MPI_Pack_size
#define NMPI_Unpack MPI_Unpack
#define NMPI_Wait MPI_Wait
#define NMPI_Test MPI_Test
#define NMPI_Type_get_attr MPI_Type_get_attr
#define NMPI_Type_set_attr MPI_Type_set_attr
#define NMPI_Comm_split MPI_Comm_split
#define NMPI_Isend MPI_Isend
#define NMPI_Irecv MPI_Irecv
#define NMPI_Recv MPI_Recv 
#define NMPI_Send MPI_Send
#define NMPI_Waitall MPI_Waitall
#define NMPI_Sendrecv MPI_Sendrecv
#define NMPI_Type_extent MPI_Type_extent
#define NMPI_Type_lb MPI_Type_lb
#define NMPI_Type_indexed MPI_Type_indexed 
#define NMPI_Type_commit MPI_Type_commit
#define NMPI_Type_free MPI_Type_free
#define NMPI_Type_size MPI_Type_size
#define NMPI_Type_get_extent MPI_Type_get_extent
#define NMPI_Graph_map  MPI_Graph_map
#define NMPI_Iprobe MPI_Iprobe
#define NMPI_Probe MPI_Probe
#define NMPI_Type_get_true_extent MPI_Type_get_true_extent
#define NMPI_Group_translate_ranks MPI_Group_translate_ranks
#define NMPI_Type_create_indexed_block MPI_Type_create_indexed_block
#define NMPI_Type_create_struct MPI_Type_create_struct
#define NMPI_Wtime MPI_Wtime 
#define NMPI_Info_get_nkeys MPI_Info_get_nkeys
#define NMPI_Info_get_nthkey MPI_Info_get_nthkey
#define NMPI_Info_get_valuelen MPI_Info_get_valuelen
#define NMPI_Info_get MPI_Info_get
#define NMPI_Info_create MPI_Info_create
#define NMPI_Info_set MPI_Info_set
#define NMPI_Comm_call_errhandler MPI_Comm_call_errhandler
#define NMPI_Grequest_start MPI_Grequest_start
#define NMPI_Grequest_complete MPI_Grequest_complete
#define NMPI_Test_cancelled MPI_Test_cancelled
#define NMPI_Ibsend MPI_Ibsend
#define NMPI_Buffer_detach MPI_Buffer_detach
#define NMPI_Type_get_envelope MPI_Type_get_envelope
#define NMPI_Type_contiguous MPI_Type_contiguous
#define NMPI_Type_vector MPI_Type_vector
#define NMPI_Type_hvector MPI_Type_hvector
#define NMPI_Type_indexed MPI_Type_indexed
#define NMPI_Type_hindexed MPI_Type_hindexed
#define NMPI_Type_struct MPI_Type_struct
#define NMPIX_Grequest_class_create MPIX_Grequest_class_create
#define NMPIX_Grequest_class_allocate MPIX_Grequest_class_allocate
#define NMPIX_Grequest_start MPIX_Grequest_start
#else
#define NMPI_Get_count PMPI_Get_count
#define NMPI_Pack PMPI_Pack
#define NMPI_Pack_size PMPI_Pack_size
#define NMPI_Unpack PMPI_Unpack
#define NMPI_Wait PMPI_Wait
#define NMPI_Test PMPI_Test
#define NMPI_Type_get_attr PMPI_Type_get_attr
#define NMPI_Type_set_attr PMPI_Type_set_attr
#define NMPI_Comm_split PMPI_Comm_split
#define NMPI_Isend PMPI_Isend
#define NMPI_Irecv PMPI_Irecv
#define NMPI_Recv PMPI_Recv 
#define NMPI_Send PMPI_Send
#define NMPI_Waitall PMPI_Waitall
#define NMPI_Sendrecv PMPI_Sendrecv
#define NMPI_Type_extent PMPI_Type_extent
#define NMPI_Type_lb PMPI_Type_lb
#define NMPI_Type_indexed PMPI_Type_indexed 
#define NMPI_Type_commit PMPI_Type_commit
#define NMPI_Type_free PMPI_Type_free
#define NMPI_Type_size PMPI_Type_size
#define NMPI_Type_get_extent PMPI_Type_get_extent
#define NMPI_Graph_map  PMPI_Graph_map
#define NMPI_Iprobe PMPI_Iprobe
#define NMPI_Probe PMPI_Probe
#define NMPI_Type_get_true_extent PMPI_Type_get_true_extent
#define NMPI_Group_translate_ranks PMPI_Group_translate_ranks
#define NMPI_Type_create_indexed_block PMPI_Type_create_indexed_block
#define NMPI_Type_create_struct PMPI_Type_create_struct
#define NMPI_Wtime PMPI_Wtime
#define NMPI_Info_get_nkeys PMPI_Info_get_nkeys
#define NMPI_Info_get_nthkey PMPI_Info_get_nthkey
#define NMPI_Info_get_valuelen PMPI_Info_get_valuelen
#define NMPI_Info_get PMPI_Info_get
#define NMPI_Info_create PMPI_Info_create
#define NMPI_Info_set PMPI_Info_set
#define NMPI_Comm_call_errhandler PMPI_Comm_call_errhandler
#define NMPI_Grequest_start PMPI_Grequest_start
#define NMPI_Grequest_complete PMPI_Grequest_complete
#define NMPI_Test_cancelled PMPI_Test_cancelled
#define NMPI_Ibsend PMPI_Ibsend
#define NMPI_Buffer_detach PMPI_Buffer_detach
#define NMPI_Type_get_envelope PMPI_Type_get_envelope
#define NMPI_Type_contiguous PMPI_Type_contiguous
#define NMPI_Type_vector PMPI_Type_vector
#define NMPI_Type_hvector PMPI_Type_hvector
#define NMPI_Type_indexed PMPI_Type_indexed
#define NMPI_Type_hindexed PMPI_Type_hindexed
#define NMPI_Type_struct PMPI_Type_struct
#define NMPIX_Grequest_class_create PMPIX_Grequest_class_create
#define NMPIX_Grequest_class_allocate PMPIX_Grequest_class_allocate
#define NMPIX_Grequest_start PMPIX_Grequest_start
#endif
#endif /* MPICH_NMPI_H_INCLUDED */




