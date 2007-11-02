/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: nmpi.h,v 1.38 2007/07/11 16:06:38 robl Exp $
 *
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
#define NMPI_Abort MPI_Abort
#define NMPI_Bcast MPI_Bcast
#define NMPI_Get_count MPI_Get_count
#define NMPI_Pack MPI_Pack
#define NMPI_Pack_size MPI_Pack_size
#define NMPI_Reduce MPI_Reduce
#define NMPI_Reduce_scatter MPI_Reduce_scatter
#define NMPI_Unpack MPI_Unpack
#define NMPI_Wait MPI_Wait
#define NMPI_Test MPI_Test
#define NMPI_Allreduce MPI_Allreduce
#define NMPI_Allgather MPI_Allgather
#define NMPI_Comm_get_attr MPI_Comm_get_attr
#define NMPI_Comm_set_attr MPI_Comm_set_attr
#define NMPI_Comm_delete_attr MPI_Comm_delete_attr
#define NMPI_Comm_create_keyval MPI_Comm_create_keyval 
#define NMPI_Comm_free_keyval MPI_Comm_free_keyval 
#define NMPI_Comm_group MPI_Comm_group
#define NMPI_Comm_remote_group MPI_Comm_remote_group
#define NMPI_Group_compare MPI_Group_compare
#define NMPI_Group_free MPI_Group_free
#define NMPI_Comm_split MPI_Comm_split
#define NMPI_Comm_size MPI_Comm_size 
#define NMPI_Comm_rank MPI_Comm_rank
#define NMPI_Alltoall MPI_Alltoall
#define NMPI_Isend MPI_Isend
#define NMPI_Irecv MPI_Irecv
#define NMPI_Recv MPI_Recv 
#define NMPI_Send MPI_Send
#define NMPI_Waitall MPI_Waitall
#define NMPI_Sendrecv MPI_Sendrecv
#define NMPI_Type_extent MPI_Type_extent
#define NMPI_Comm_free MPI_Comm_free
#define NMPI_Comm_dup MPI_Comm_dup
#define NMPI_Type_lb MPI_Type_lb
#define NMPI_Type_indexed MPI_Type_indexed 
#define NMPI_Type_commit MPI_Type_commit
#define NMPI_Type_free MPI_Type_free
#define NMPI_Type_size MPI_Type_size
#define NMPI_Type_get_extent MPI_Type_get_extent
#define NMPI_Cart_rank MPI_Cart_rank
#define NMPI_Cart_map  MPI_Cart_map
#define NMPI_Graph_map  MPI_Graph_map
#define NMPI_Iprobe MPI_Iprobe
#define NMPI_Barrier MPI_Barrier
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
#define NMPI_Comm_get_name MPI_Comm_get_name
#define NMPI_Comm_get_errhandler MPI_Comm_get_errhandler
#define NMPI_Comm_set_errhandler MPI_Comm_set_errhandler
#define NMPI_Comm_create_errhandler MPI_Comm_create_errhandler
#define NMPI_Comm_call_errhandler MPI_Comm_call_errhandler
#define NMPI_Open_port MPI_Open_port
#define NMPI_Comm_accept MPI_Comm_accept
#define NMPI_Comm_connect MPI_Comm_connect
#define NMPI_Grequest_start MPI_Grequest_start
#define NMPI_Grequest_complete MPI_Grequest_complete
#define NMPI_Cancel MPI_Cancel
#define NMPI_Test_cancelled MPI_Test_cancelled
#define NMPI_Ibsend MPI_Ibsend
#define NMPI_Buffer_detach MPI_Buffer_detach
#define NMPI_Gather MPI_Gather
#define NMPIX_Grequest_class_create MPIX_Grequest_class_create
#define NMPIX_Grequest_class_allocate MPIX_Grequest_class_allocate
#define NMPIX_Grequest_start MPIX_Grequest_start
#else
#define NMPI_Abort PMPI_Abort
#define NMPI_Bcast PMPI_Bcast
#define NMPI_Get_count PMPI_Get_count
#define NMPI_Pack PMPI_Pack
#define NMPI_Pack_size PMPI_Pack_size
#define NMPI_Reduce PMPI_Reduce
#define NMPI_Reduce_scatter PMPI_Reduce_scatter
#define NMPI_Unpack PMPI_Unpack
#define NMPI_Wait PMPI_Wait
#define NMPI_Test PMPI_Test
#define NMPI_Allreduce PMPI_Allreduce
#define NMPI_Allgather PMPI_Allgather
#define NMPI_Comm_get_attr PMPI_Comm_get_attr
#define NMPI_Comm_set_attr PMPI_Comm_set_attr
#define NMPI_Comm_delete_attr PMPI_Comm_delete_attr
#define NMPI_Comm_create_keyval PMPI_Comm_create_keyval 
#define NMPI_Comm_free_keyval PMPI_Comm_free_keyval 
#define NMPI_Comm_group PMPI_Comm_group
#define NMPI_Comm_remote_group PMPI_Comm_remote_group
#define NMPI_Group_compare PMPI_Group_compare
#define NMPI_Group_free PMPI_Group_free
#define NMPI_Comm_split PMPI_Comm_split
#define NMPI_Comm_size PMPI_Comm_size 
#define NMPI_Comm_rank PMPI_Comm_rank
#define NMPI_Alltoall PMPI_Alltoall
#define NMPI_Isend PMPI_Isend
#define NMPI_Irecv PMPI_Irecv
#define NMPI_Recv PMPI_Recv 
#define NMPI_Send PMPI_Send
#define NMPI_Waitall PMPI_Waitall
#define NMPI_Sendrecv PMPI_Sendrecv
#define NMPI_Type_extent PMPI_Type_extent
#define NMPI_Comm_free PMPI_Comm_free
#define NMPI_Comm_dup PMPI_Comm_dup
#define NMPI_Type_lb PMPI_Type_lb
#define NMPI_Type_indexed PMPI_Type_indexed 
#define NMPI_Type_commit PMPI_Type_commit
#define NMPI_Type_free PMPI_Type_free
#define NMPI_Type_size PMPI_Type_size
#define NMPI_Type_get_extent PMPI_Type_get_extent
#define NMPI_Cart_rank PMPI_Cart_rank
#define NMPI_Cart_map  PMPI_Cart_map
#define NMPI_Graph_map  PMPI_Graph_map
#define NMPI_Iprobe PMPI_Iprobe
#define NMPI_Barrier PMPI_Barrier
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
#define NMPI_Comm_get_name PMPI_Comm_get_name
#define NMPI_Comm_get_errhandler PMPI_Comm_get_errhandler
#define NMPI_Comm_set_errhandler PMPI_Comm_set_errhandler
#define NMPI_Comm_create_errhandler PMPI_Comm_create_errhandler
#define NMPI_Comm_call_errhandler PMPI_Comm_call_errhandler
#define NMPI_Open_port PMPI_Open_port
#define NMPI_Close_port PMPI_Close_port
#define NMPI_Comm_accept PMPI_Comm_accept
#define NMPI_Comm_connect PMPI_Comm_connect
#define NMPI_Grequest_start PMPI_Grequest_start
#define NMPI_Grequest_complete PMPI_Grequest_complete
#define NMPI_Cancel PMPI_Cancel
#define NMPI_Test_cancelled PMPI_Test_cancelled
#define NMPI_Ibsend PMPI_Ibsend
#define NMPI_Buffer_detach PMPI_Buffer_detach
#define NMPI_Gather PMPI_Gather
#define NMPIX_Grequest_class_create PMPIX_Grequest_class_create
#define NMPIX_Grequest_class_allocate PMPIX_Grequest_class_allocate
#define NMPIX_Grequest_start PMPIX_Grequest_start
#endif
#endif /* MPICH_NMPI_H_INCLUDED */
