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
#define NMPI_Pack MPI_Pack
#define NMPI_Pack_size MPI_Pack_size
#define NMPI_Unpack MPI_Unpack
#define NMPI_Wait MPI_Wait
#define NMPI_Test MPI_Test
#define NMPI_Type_get_attr MPI_Type_get_attr
#define NMPI_Type_set_attr MPI_Type_set_attr
#define NMPI_Isend MPI_Isend
#define NMPI_Irecv MPI_Irecv
#define NMPI_Recv MPI_Recv 
#define NMPI_Send MPI_Send
#define NMPI_Waitall MPI_Waitall
#define NMPI_Sendrecv MPI_Sendrecv
#define NMPI_Type_lb MPI_Type_lb
#define NMPI_Iprobe MPI_Iprobe
#define NMPI_Probe MPI_Probe
#define NMPI_Group_translate_ranks MPI_Group_translate_ranks
#define NMPI_Wtime MPI_Wtime 
#define NMPI_Info_create MPI_Info_create
#define NMPI_Info_set MPI_Info_set
#define NMPI_Comm_call_errhandler MPI_Comm_call_errhandler
#define NMPI_Test_cancelled MPI_Test_cancelled
#define NMPI_Ibsend MPI_Ibsend
#define NMPI_Buffer_detach MPI_Buffer_detach
#define NMPI_Type_hindexed MPI_Type_hindexed
#define NMPIX_Grequest_class_create MPIX_Grequest_class_create
#define NMPIX_Grequest_class_allocate MPIX_Grequest_class_allocate
#define NMPIX_Grequest_start MPIX_Grequest_start
#else
#define NMPI_Pack PMPI_Pack
#define NMPI_Pack_size PMPI_Pack_size
#define NMPI_Unpack PMPI_Unpack
#define NMPI_Wait PMPI_Wait
#define NMPI_Test PMPI_Test
#define NMPI_Type_get_attr PMPI_Type_get_attr
#define NMPI_Type_set_attr PMPI_Type_set_attr
#define NMPI_Isend PMPI_Isend
#define NMPI_Irecv PMPI_Irecv
#define NMPI_Recv PMPI_Recv 
#define NMPI_Send PMPI_Send
#define NMPI_Waitall PMPI_Waitall
#define NMPI_Sendrecv PMPI_Sendrecv
#define NMPI_Type_lb PMPI_Type_lb
#define NMPI_Iprobe PMPI_Iprobe
#define NMPI_Probe PMPI_Probe
#define NMPI_Group_translate_ranks PMPI_Group_translate_ranks
#define NMPI_Wtime PMPI_Wtime
#define NMPI_Info_create PMPI_Info_create
#define NMPI_Info_set PMPI_Info_set
#define NMPI_Comm_call_errhandler PMPI_Comm_call_errhandler
#define NMPI_Test_cancelled PMPI_Test_cancelled
#define NMPI_Ibsend PMPI_Ibsend
#define NMPI_Buffer_detach PMPI_Buffer_detach
#define NMPI_Type_hindexed PMPI_Type_hindexed
#define NMPIX_Grequest_class_create PMPIX_Grequest_class_create
#define NMPIX_Grequest_class_allocate PMPIX_Grequest_class_allocate
#define NMPIX_Grequest_start PMPIX_Grequest_start
#endif
#endif /* MPICH_NMPI_H_INCLUDED */




