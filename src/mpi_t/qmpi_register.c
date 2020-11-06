
/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : TOOLS
      description : cvars that control tools connected to MPICH

cvars:
    - name        : MPIR_CVAR_QMPI_TOOL_LIST
      alias       : QMPI_TOOL_LIST
      category    : TOOLS
      type        : string
      default     : NULL
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Set the number and order of QMPI tools to be loaded by the MPI library when it is
        initialized.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

typedef void (*generic_mpi_func) (void);

int MPII_qmpi_register_internal_functions(void)
{

    MPIR_QMPI_pointers[MPI_ABORT_T] = (void (*)(void)) &QMPI_Abort;
    MPIR_QMPI_pointers[MPI_ACCUMULATE_T] = (void (*)(void)) &QMPI_Accumulate;
    MPIR_QMPI_pointers[MPI_ADD_ERROR_CLASS_T] = (void (*)(void)) &QMPI_Add_error_class;
    MPIR_QMPI_pointers[MPI_ADD_ERROR_CODE_T] = (void (*)(void)) &QMPI_Add_error_code;
    MPIR_QMPI_pointers[MPI_ADD_ERROR_STRING_T] = (void (*)(void)) &QMPI_Add_error_string;
    MPIR_QMPI_pointers[MPI_AINT_ADD_T] = (void (*)(void)) &QMPI_Aint_add;
    MPIR_QMPI_pointers[MPI_AINT_DIFF_T] = (void (*)(void)) &QMPI_Aint_diff;
    MPIR_QMPI_pointers[MPI_ALLGATHER_T] = (void (*)(void)) &QMPI_Allgather;
    MPIR_QMPI_pointers[MPI_ALLGATHERV_T] = (void (*)(void)) &QMPI_Allgatherv;
    MPIR_QMPI_pointers[MPI_ALLOC_MEM_T] = (void (*)(void)) &QMPI_Alloc_mem;
    MPIR_QMPI_pointers[MPI_ALLREDUCE_T] = (void (*)(void)) &QMPI_Allreduce;
    MPIR_QMPI_pointers[MPI_ALLTOALL_T] = (void (*)(void)) &QMPI_Alltoall;
    MPIR_QMPI_pointers[MPI_ALLTOALLV_T] = (void (*)(void)) &QMPI_Alltoallv;
    MPIR_QMPI_pointers[MPI_ALLTOALLW_T] = (void (*)(void)) &QMPI_Alltoallw;
    MPIR_QMPI_pointers[MPI_ATTR_DELETE_T] = (void (*)(void)) &QMPI_Attr_delete;
    MPIR_QMPI_pointers[MPI_ATTR_GET_T] = (void (*)(void)) &QMPI_Attr_get;
    MPIR_QMPI_pointers[MPI_ATTR_PUT_T] = (void (*)(void)) &QMPI_Attr_put;
    MPIR_QMPI_pointers[MPI_BARRIER_T] = (void (*)(void)) &QMPI_Barrier;
    MPIR_QMPI_pointers[MPI_BCAST_T] = (void (*)(void)) &QMPI_Bcast;
    MPIR_QMPI_pointers[MPI_BSEND_T] = (void (*)(void)) &QMPI_Bsend;
    MPIR_QMPI_pointers[MPI_BSEND_INIT_T] = (void (*)(void)) &QMPI_Bsend_init;
    MPIR_QMPI_pointers[MPI_BUFFER_ATTACH_T] = (void (*)(void)) &QMPI_Buffer_attach;
    MPIR_QMPI_pointers[MPI_BUFFER_DETACH_T] = (void (*)(void)) &QMPI_Buffer_detach;
    MPIR_QMPI_pointers[MPI_CANCEL_T] = (void (*)(void)) &QMPI_Cancel;
    MPIR_QMPI_pointers[MPI_CART_COORDS_T] = (void (*)(void)) &QMPI_Cart_coords;
    MPIR_QMPI_pointers[MPI_CART_CREATE_T] = (void (*)(void)) &QMPI_Cart_create;
    MPIR_QMPI_pointers[MPI_CART_GET_T] = (void (*)(void)) &QMPI_Cart_get;
    MPIR_QMPI_pointers[MPI_CART_MAP_T] = (void (*)(void)) &QMPI_Cart_map;
    MPIR_QMPI_pointers[MPI_CART_RANK_T] = (void (*)(void)) &QMPI_Cart_rank;
    MPIR_QMPI_pointers[MPI_CART_SHIFT_T] = (void (*)(void)) &QMPI_Cart_shift;
    MPIR_QMPI_pointers[MPI_CART_SUB_T] = (void (*)(void)) &QMPI_Cart_sub;
    MPIR_QMPI_pointers[MPI_CARTDIM_GET_T] = (void (*)(void)) &QMPI_Cartdim_get;
    MPIR_QMPI_pointers[MPI_CLOSE_PORT_T] = (void (*)(void)) &QMPI_Close_port;
    MPIR_QMPI_pointers[MPI_COMM_ACCEPT_T] = (void (*)(void)) &QMPI_Comm_accept;
    MPIR_QMPI_pointers[MPI_COMM_CALL_ERRHANDLER_T] = (void (*)(void)) &QMPI_Comm_call_errhandler;
    MPIR_QMPI_pointers[MPI_COMM_COMPARE_T] = (void (*)(void)) &QMPI_Comm_compare;
    MPIR_QMPI_pointers[MPI_COMM_CONNECT_T] = (void (*)(void)) &QMPI_Comm_connect;
    MPIR_QMPI_pointers[MPI_COMM_CREATE_T] = (void (*)(void)) &QMPI_Comm_create;
    MPIR_QMPI_pointers[MPI_COMM_CREATE_ERRHANDLER_T] =
        (void (*)(void)) &QMPI_Comm_create_errhandler;
    MPIR_QMPI_pointers[MPI_COMM_CREATE_GROUP_T] = (void (*)(void)) &QMPI_Comm_create_group;
    MPIR_QMPI_pointers[MPI_COMM_CREATE_KEYVAL_T] = (void (*)(void)) &QMPI_Comm_create_keyval;
    MPIR_QMPI_pointers[MPI_COMM_DELETE_ATTR_T] = (void (*)(void)) &QMPI_Comm_delete_attr;
    MPIR_QMPI_pointers[MPI_COMM_DISCONNECT_T] = (void (*)(void)) &QMPI_Comm_disconnect;
    MPIR_QMPI_pointers[MPI_COMM_DUP_T] = (void (*)(void)) &QMPI_Comm_dup;
    MPIR_QMPI_pointers[MPI_COMM_DUP_WITH_INFO_T] = (void (*)(void)) &QMPI_Comm_dup_with_info;
    MPIR_QMPI_pointers[MPI_COMM_FREE_T] = (void (*)(void)) &QMPI_Comm_free;
    MPIR_QMPI_pointers[MPI_COMM_FREE_KEYVAL_T] = (void (*)(void)) &QMPI_Comm_free_keyval;
    MPIR_QMPI_pointers[MPI_COMM_GET_ATTR_T] = (void (*)(void)) &QMPI_Comm_get_attr;
    MPIR_QMPI_pointers[MPI_COMM_GET_ERRHANDLER_T] = (void (*)(void)) &QMPI_Comm_get_errhandler;
    MPIR_QMPI_pointers[MPI_COMM_GET_INFO_T] = (void (*)(void)) &QMPI_Comm_get_info;
    MPIR_QMPI_pointers[MPI_COMM_GET_NAME_T] = (void (*)(void)) &QMPI_Comm_get_name;
    MPIR_QMPI_pointers[MPI_COMM_GET_PARENT_T] = (void (*)(void)) &QMPI_Comm_get_parent;
    MPIR_QMPI_pointers[MPI_COMM_GROUP_T] = (void (*)(void)) &QMPI_Comm_group;
    MPIR_QMPI_pointers[MPI_COMM_IDUP_T] = (void (*)(void)) &QMPI_Comm_idup;
    MPIR_QMPI_pointers[MPI_COMM_JOIN_T] = (void (*)(void)) &QMPI_Comm_join;
    MPIR_QMPI_pointers[MPI_COMM_RANK_T] = (void (*)(void)) &QMPI_Comm_rank;
    MPIR_QMPI_pointers[MPI_COMM_REMOTE_GROUP_T] = (void (*)(void)) &QMPI_Comm_remote_group;
    MPIR_QMPI_pointers[MPI_COMM_REMOTE_SIZE_T] = (void (*)(void)) &QMPI_Comm_remote_size;
    MPIR_QMPI_pointers[MPI_COMM_SET_ATTR_T] = (void (*)(void)) &QMPI_Comm_set_attr;
    MPIR_QMPI_pointers[MPI_COMM_SET_ERRHANDLER_T] = (void (*)(void)) &QMPI_Comm_set_errhandler;
    MPIR_QMPI_pointers[MPI_COMM_SET_INFO_T] = (void (*)(void)) &QMPI_Comm_set_info;
    MPIR_QMPI_pointers[MPI_COMM_SET_NAME_T] = (void (*)(void)) &QMPI_Comm_set_name;
    MPIR_QMPI_pointers[MPI_COMM_SIZE_T] = (void (*)(void)) &QMPI_Comm_size;
    MPIR_QMPI_pointers[MPI_COMM_SPAWN_T] = (void (*)(void)) &QMPI_Comm_spawn;
    MPIR_QMPI_pointers[MPI_COMM_SPAWN_MULTIPLE_T] = (void (*)(void)) &QMPI_Comm_spawn_multiple;
    MPIR_QMPI_pointers[MPI_COMM_SPLIT_T] = (void (*)(void)) &QMPI_Comm_split;
    MPIR_QMPI_pointers[MPI_COMM_SPLIT_TYPE_T] = (void (*)(void)) &QMPI_Comm_split_type;
    MPIR_QMPI_pointers[MPI_COMM_TEST_INTER_T] = (void (*)(void)) &QMPI_Comm_test_inter;
    MPIR_QMPI_pointers[MPI_COMPARE_AND_SWAP_T] = (void (*)(void)) &QMPI_Compare_and_swap;
    MPIR_QMPI_pointers[MPI_DIMS_CREATE_T] = (void (*)(void)) &QMPI_Dims_create;
    MPIR_QMPI_pointers[MPI_DIST_GRAPH_CREATE_T] = (void (*)(void)) &QMPI_Dist_graph_create;
    MPIR_QMPI_pointers[MPI_DIST_GRAPH_CREATE_ADJACENT_T] =
        (void (*)(void)) &QMPI_Dist_graph_create_adjacent;
    MPIR_QMPI_pointers[MPI_DIST_GRAPH_NEIGHBORS_T] = (void (*)(void)) &QMPI_Dist_graph_neighbors;
    MPIR_QMPI_pointers[MPI_DIST_GRAPH_NEIGHBORS_COUNT_T] =
        (void (*)(void)) &QMPI_Dist_graph_neighbors_count;
    MPIR_QMPI_pointers[MPI_ERRHANDLER_FREE_T] = (void (*)(void)) &QMPI_Errhandler_free;
    MPIR_QMPI_pointers[MPI_ERROR_CLASS_T] = (void (*)(void)) &QMPI_Error_class;
    MPIR_QMPI_pointers[MPI_ERROR_STRING_T] = (void (*)(void)) &QMPI_Error_string;
    MPIR_QMPI_pointers[MPI_EXSCAN_T] = (void (*)(void)) &QMPI_Exscan;
    MPIR_QMPI_pointers[MPI_FETCH_AND_OP_T] = (void (*)(void)) &QMPI_Fetch_and_op;
    MPIR_QMPI_pointers[MPI_FILE_C2F_T] = (void (*)(void)) &QMPI_File_c2f;
    MPIR_QMPI_pointers[MPI_FILE_CALL_ERRHANDLER_T] = (void (*)(void)) &QMPI_File_call_errhandler;
    MPIR_QMPI_pointers[MPI_FILE_CLOSE_T] = (void (*)(void)) &QMPI_File_close;
    MPIR_QMPI_pointers[MPI_FILE_CREATE_ERRHANDLER_T] =
        (void (*)(void)) &QMPI_File_create_errhandler;
    MPIR_QMPI_pointers[MPI_FILE_DELETE_T] = (void (*)(void)) &QMPI_File_delete;
    MPIR_QMPI_pointers[MPI_FILE_F2C_T] = (void (*)(void)) &QMPI_File_f2c;
    MPIR_QMPI_pointers[MPI_FILE_GET_AMODE_T] = (void (*)(void)) &QMPI_File_get_amode;
    MPIR_QMPI_pointers[MPI_FILE_GET_ATOMICITY_T] = (void (*)(void)) &QMPI_File_get_atomicity;
    MPIR_QMPI_pointers[MPI_FILE_GET_BYTE_OFFSET_T] = (void (*)(void)) &QMPI_File_get_byte_offset;
    MPIR_QMPI_pointers[MPI_FILE_GET_ERRHANDLER_T] = (void (*)(void)) &QMPI_File_get_errhandler;
    MPIR_QMPI_pointers[MPI_FILE_GET_GROUP_T] = (void (*)(void)) &QMPI_File_get_group;
    MPIR_QMPI_pointers[MPI_FILE_GET_INFO_T] = (void (*)(void)) &QMPI_File_get_info;
    MPIR_QMPI_pointers[MPI_FILE_GET_POSITION_T] = (void (*)(void)) &QMPI_File_get_position;
    MPIR_QMPI_pointers[MPI_FILE_GET_POSITION_SHARED_T] =
        (void (*)(void)) &QMPI_File_get_position_shared;
    MPIR_QMPI_pointers[MPI_FILE_GET_SIZE_T] = (void (*)(void)) &QMPI_File_get_size;
    MPIR_QMPI_pointers[MPI_FILE_GET_TYPE_EXTENT_T] = (void (*)(void)) &QMPI_File_get_type_extent;
    MPIR_QMPI_pointers[MPI_FILE_GET_VIEW_T] = (void (*)(void)) &QMPI_File_get_view;
    MPIR_QMPI_pointers[MPI_FILE_IREAD_T] = (void (*)(void)) &QMPI_File_iread;
    MPIR_QMPI_pointers[MPI_FILE_IREAD_ALL_T] = (void (*)(void)) &QMPI_File_iread_all;
    MPIR_QMPI_pointers[MPI_FILE_IREAD_AT_T] = (void (*)(void)) &QMPI_File_iread_at;
    MPIR_QMPI_pointers[MPI_FILE_IREAD_AT_ALL_T] = (void (*)(void)) &QMPI_File_iread_at_all;
    MPIR_QMPI_pointers[MPI_FILE_IREAD_SHARED_T] = (void (*)(void)) &QMPI_File_iread_shared;
    MPIR_QMPI_pointers[MPI_FILE_IWRITE_T] = (void (*)(void)) &QMPI_File_iwrite;
    MPIR_QMPI_pointers[MPI_FILE_IWRITE_ALL_T] = (void (*)(void)) &QMPI_File_iwrite_all;
    MPIR_QMPI_pointers[MPI_FILE_IWRITE_AT_T] = (void (*)(void)) &QMPI_File_iwrite_at;
    MPIR_QMPI_pointers[MPI_FILE_IWRITE_AT_ALL_T] = (void (*)(void)) &QMPI_File_iwrite_at_all;
    MPIR_QMPI_pointers[MPI_FILE_IWRITE_SHARED_T] = (void (*)(void)) &QMPI_File_iwrite_shared;
    MPIR_QMPI_pointers[MPI_FILE_OPEN_T] = (void (*)(void)) &QMPI_File_open;
    MPIR_QMPI_pointers[MPI_FILE_PREALLOCATE_T] = (void (*)(void)) &QMPI_File_preallocate;
    MPIR_QMPI_pointers[MPI_FILE_READ_T] = (void (*)(void)) &QMPI_File_read;
    MPIR_QMPI_pointers[MPI_FILE_READ_ALL_T] = (void (*)(void)) &QMPI_File_read_all;
    MPIR_QMPI_pointers[MPI_FILE_READ_ALL_BEGIN_T] = (void (*)(void)) &QMPI_File_read_all_begin;
    MPIR_QMPI_pointers[MPI_FILE_READ_ALL_END_T] = (void (*)(void)) &QMPI_File_read_all_end;
    MPIR_QMPI_pointers[MPI_FILE_READ_AT_T] = (void (*)(void)) &QMPI_File_read_at;
    MPIR_QMPI_pointers[MPI_FILE_READ_AT_ALL_T] = (void (*)(void)) &QMPI_File_read_at_all;
    MPIR_QMPI_pointers[MPI_FILE_READ_AT_ALL_BEGIN_T] =
        (void (*)(void)) &QMPI_File_read_at_all_begin;
    MPIR_QMPI_pointers[MPI_FILE_READ_AT_ALL_END_T] = (void (*)(void)) &QMPI_File_read_at_all_end;
    MPIR_QMPI_pointers[MPI_FILE_READ_ORDERED_T] = (void (*)(void)) &QMPI_File_read_ordered;
    MPIR_QMPI_pointers[MPI_FILE_READ_ORDERED_BEGIN_T] =
        (void (*)(void)) &QMPI_File_read_ordered_begin;
    MPIR_QMPI_pointers[MPI_FILE_READ_ORDERED_END_T] = (void (*)(void)) &QMPI_File_read_ordered_end;
    MPIR_QMPI_pointers[MPI_FILE_READ_SHARED_T] = (void (*)(void)) &QMPI_File_read_shared;
    MPIR_QMPI_pointers[MPI_FILE_SEEK_T] = (void (*)(void)) &QMPI_File_seek;
    MPIR_QMPI_pointers[MPI_FILE_SEEK_SHARED_T] = (void (*)(void)) &QMPI_File_seek_shared;
    MPIR_QMPI_pointers[MPI_FILE_SET_ATOMICITY_T] = (void (*)(void)) &QMPI_File_set_atomicity;
    MPIR_QMPI_pointers[MPI_FILE_SET_ERRHANDLER_T] = (void (*)(void)) &QMPI_File_set_errhandler;
    MPIR_QMPI_pointers[MPI_FILE_SET_INFO_T] = (void (*)(void)) &QMPI_File_set_info;
    MPIR_QMPI_pointers[MPI_FILE_SET_SIZE_T] = (void (*)(void)) &QMPI_File_set_size;
    MPIR_QMPI_pointers[MPI_FILE_SET_VIEW_T] = (void (*)(void)) &QMPI_File_set_view;
    MPIR_QMPI_pointers[MPI_FILE_SYNC_T] = (void (*)(void)) &QMPI_File_sync;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_T] = (void (*)(void)) &QMPI_File_write;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_ALL_T] = (void (*)(void)) &QMPI_File_write_all;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_ALL_BEGIN_T] = (void (*)(void)) &QMPI_File_write_all_begin;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_ALL_END_T] = (void (*)(void)) &QMPI_File_write_all_end;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_AT_T] = (void (*)(void)) &QMPI_File_write_at;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_AT_ALL_T] = (void (*)(void)) &QMPI_File_write_at_all;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_AT_ALL_BEGIN_T] =
        (void (*)(void)) &QMPI_File_write_at_all_begin;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_AT_ALL_END_T] = (void (*)(void)) &QMPI_File_write_at_all_end;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_ORDERED_T] = (void (*)(void)) &QMPI_File_write_ordered;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_ORDERED_BEGIN_T] =
        (void (*)(void)) &QMPI_File_write_ordered_begin;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_ORDERED_END_T] =
        (void (*)(void)) &QMPI_File_write_ordered_end;
    MPIR_QMPI_pointers[MPI_FILE_WRITE_SHARED_T] = (void (*)(void)) &QMPI_File_write_shared;
    MPIR_QMPI_pointers[MPI_FINALIZE_T] = (void (*)(void)) &QMPI_Finalize;
    MPIR_QMPI_pointers[MPI_FINALIZED_T] = (void (*)(void)) &QMPI_Finalized;
    MPIR_QMPI_pointers[MPI_FREE_MEM_T] = (void (*)(void)) &QMPI_Free_mem;
    MPIR_QMPI_pointers[MPI_GATHER_T] = (void (*)(void)) &QMPI_Gather;
    MPIR_QMPI_pointers[MPI_GATHERV_T] = (void (*)(void)) &QMPI_Gatherv;
    MPIR_QMPI_pointers[MPI_GET_T] = (void (*)(void)) &QMPI_Get;
    MPIR_QMPI_pointers[MPI_GET_ACCUMULATE_T] = (void (*)(void)) &QMPI_Get_accumulate;
    MPIR_QMPI_pointers[MPI_GET_ADDRESS_T] = (void (*)(void)) &QMPI_Get_address;
    MPIR_QMPI_pointers[MPI_GET_COUNT_T] = (void (*)(void)) &QMPI_Get_count;
    MPIR_QMPI_pointers[MPI_GET_ELEMENTS_T] = (void (*)(void)) &QMPI_Get_elements;
    MPIR_QMPI_pointers[MPI_GET_ELEMENTS_X_T] = (void (*)(void)) &QMPI_Get_elements_x;
    MPIR_QMPI_pointers[MPI_GET_LIBRARY_VERSION_T] = (void (*)(void)) &QMPI_Get_library_version;
    MPIR_QMPI_pointers[MPI_GET_PROCESSOR_NAME_T] = (void (*)(void)) &QMPI_Get_processor_name;
    MPIR_QMPI_pointers[MPI_GET_VERSION_T] = (void (*)(void)) &QMPI_Get_version;
    MPIR_QMPI_pointers[MPI_GRAPH_CREATE_T] = (void (*)(void)) &QMPI_Graph_create;
    MPIR_QMPI_pointers[MPI_GRAPH_GET_T] = (void (*)(void)) &QMPI_Graph_get;
    MPIR_QMPI_pointers[MPI_GRAPH_MAP_T] = (void (*)(void)) &QMPI_Graph_map;
    MPIR_QMPI_pointers[MPI_GRAPH_NEIGHBORS_T] = (void (*)(void)) &QMPI_Graph_neighbors;
    MPIR_QMPI_pointers[MPI_GRAPH_NEIGHBORS_COUNT_T] = (void (*)(void)) &QMPI_Graph_neighbors_count;
    MPIR_QMPI_pointers[MPI_GRAPHDIMS_GET_T] = (void (*)(void)) &QMPI_Graphdims_get;
    MPIR_QMPI_pointers[MPI_GREQUEST_COMPLETE_T] = (void (*)(void)) &QMPI_Grequest_complete;
    MPIR_QMPI_pointers[MPI_GREQUEST_START_T] = (void (*)(void)) &QMPI_Grequest_start;
    MPIR_QMPI_pointers[MPI_GROUP_COMPARE_T] = (void (*)(void)) &QMPI_Group_compare;
    MPIR_QMPI_pointers[MPI_GROUP_DIFFERENCE_T] = (void (*)(void)) &QMPI_Group_difference;
    MPIR_QMPI_pointers[MPI_GROUP_EXCL_T] = (void (*)(void)) &QMPI_Group_excl;
    MPIR_QMPI_pointers[MPI_GROUP_FREE_T] = (void (*)(void)) &QMPI_Group_free;
    MPIR_QMPI_pointers[MPI_GROUP_INCL_T] = (void (*)(void)) &QMPI_Group_incl;
    MPIR_QMPI_pointers[MPI_GROUP_INTERSECTION_T] = (void (*)(void)) &QMPI_Group_intersection;
    MPIR_QMPI_pointers[MPI_GROUP_RANGE_EXCL_T] = (void (*)(void)) &QMPI_Group_range_excl;
    MPIR_QMPI_pointers[MPI_GROUP_RANGE_INCL_T] = (void (*)(void)) &QMPI_Group_range_incl;
    MPIR_QMPI_pointers[MPI_GROUP_RANK_T] = (void (*)(void)) &QMPI_Group_rank;
    MPIR_QMPI_pointers[MPI_GROUP_SIZE_T] = (void (*)(void)) &QMPI_Group_size;
    MPIR_QMPI_pointers[MPI_GROUP_TRANSLATE_RANKS_T] = (void (*)(void)) &QMPI_Group_translate_ranks;
    MPIR_QMPI_pointers[MPI_GROUP_UNION_T] = (void (*)(void)) &QMPI_Group_union;
    MPIR_QMPI_pointers[MPI_IALLGATHER_T] = (void (*)(void)) &QMPI_Iallgather;
    MPIR_QMPI_pointers[MPI_IALLGATHERV_T] = (void (*)(void)) &QMPI_Iallgatherv;
    MPIR_QMPI_pointers[MPI_IALLREDUCE_T] = (void (*)(void)) &QMPI_Iallreduce;
    MPIR_QMPI_pointers[MPI_IALLTOALL_T] = (void (*)(void)) &QMPI_Ialltoall;
    MPIR_QMPI_pointers[MPI_IALLTOALLV_T] = (void (*)(void)) &QMPI_Ialltoallv;
    MPIR_QMPI_pointers[MPI_IALLTOALLW_T] = (void (*)(void)) &QMPI_Ialltoallw;
    MPIR_QMPI_pointers[MPI_IBARRIER_T] = (void (*)(void)) &QMPI_Ibarrier;
    MPIR_QMPI_pointers[MPI_IBCAST_T] = (void (*)(void)) &QMPI_Ibcast;
    MPIR_QMPI_pointers[MPI_IBSEND_T] = (void (*)(void)) &QMPI_Ibsend;
    MPIR_QMPI_pointers[MPI_IEXSCAN_T] = (void (*)(void)) &QMPI_Iexscan;
    MPIR_QMPI_pointers[MPI_IGATHER_T] = (void (*)(void)) &QMPI_Igather;
    MPIR_QMPI_pointers[MPI_IGATHERV_T] = (void (*)(void)) &QMPI_Igatherv;
    MPIR_QMPI_pointers[MPI_IMPROBE_T] = (void (*)(void)) &QMPI_Improbe;
    MPIR_QMPI_pointers[MPI_IMRECV_T] = (void (*)(void)) &QMPI_Imrecv;
    MPIR_QMPI_pointers[MPI_INEIGHBOR_ALLGATHER_T] = (void (*)(void)) &QMPI_Ineighbor_allgather;
    MPIR_QMPI_pointers[MPI_INEIGHBOR_ALLGATHERV_T] = (void (*)(void)) &QMPI_Ineighbor_allgatherv;
    MPIR_QMPI_pointers[MPI_INEIGHBOR_ALLTOALL_T] = (void (*)(void)) &QMPI_Ineighbor_alltoall;
    MPIR_QMPI_pointers[MPI_INEIGHBOR_ALLTOALLV_T] = (void (*)(void)) &QMPI_Ineighbor_alltoallv;
    MPIR_QMPI_pointers[MPI_INEIGHBOR_ALLTOALLW_T] = (void (*)(void)) &QMPI_Ineighbor_alltoallw;
    MPIR_QMPI_pointers[MPI_INFO_CREATE_T] = (void (*)(void)) &QMPI_Info_create;
    MPIR_QMPI_pointers[MPI_INFO_DELETE_T] = (void (*)(void)) &QMPI_Info_delete;
    MPIR_QMPI_pointers[MPI_INFO_DUP_T] = (void (*)(void)) &QMPI_Info_dup;
    MPIR_QMPI_pointers[MPI_INFO_FREE_T] = (void (*)(void)) &QMPI_Info_free;
    MPIR_QMPI_pointers[MPI_INFO_GET_T] = (void (*)(void)) &QMPI_Info_get;
    MPIR_QMPI_pointers[MPI_INFO_GET_NKEYS_T] = (void (*)(void)) &QMPI_Info_get_nkeys;
    MPIR_QMPI_pointers[MPI_INFO_GET_NTHKEY_T] = (void (*)(void)) &QMPI_Info_get_nthkey;
    MPIR_QMPI_pointers[MPI_INFO_GET_VALUELEN_T] = (void (*)(void)) &QMPI_Info_get_valuelen;
    MPIR_QMPI_pointers[MPI_INFO_SET_T] = (void (*)(void)) &QMPI_Info_set;
    MPIR_QMPI_pointers[MPI_INIT_T] = (void (*)(void)) &QMPI_Init;
    MPIR_QMPI_pointers[MPI_INIT_THREAD_T] = (void (*)(void)) &QMPI_Init_thread;
    MPIR_QMPI_pointers[MPI_INITIALIZED_T] = (void (*)(void)) &QMPI_Initialized;
    MPIR_QMPI_pointers[MPI_INTERCOMM_CREATE_T] = (void (*)(void)) &QMPI_Intercomm_create;
    MPIR_QMPI_pointers[MPI_INTERCOMM_MERGE_T] = (void (*)(void)) &QMPI_Intercomm_merge;
    MPIR_QMPI_pointers[MPI_IPROBE_T] = (void (*)(void)) &QMPI_Iprobe;
    MPIR_QMPI_pointers[MPI_IRECV_T] = (void (*)(void)) &QMPI_Irecv;
    MPIR_QMPI_pointers[MPI_IREDUCE_T] = (void (*)(void)) &QMPI_Ireduce;
    MPIR_QMPI_pointers[MPI_IREDUCE_SCATTER_T] = (void (*)(void)) &QMPI_Ireduce_scatter;
    MPIR_QMPI_pointers[MPI_IREDUCE_SCATTER_BLOCK_T] = (void (*)(void)) &QMPI_Ireduce_scatter_block;
    MPIR_QMPI_pointers[MPI_IRSEND_T] = (void (*)(void)) &QMPI_Irsend;
    MPIR_QMPI_pointers[MPI_IS_THREAD_MAIN_T] = (void (*)(void)) &QMPI_Is_thread_main;
    MPIR_QMPI_pointers[MPI_ISCAN_T] = (void (*)(void)) &QMPI_Iscan;
    MPIR_QMPI_pointers[MPI_ISCATTER_T] = (void (*)(void)) &QMPI_Iscatter;
    MPIR_QMPI_pointers[MPI_ISCATTERV_T] = (void (*)(void)) &QMPI_Iscatterv;
    MPIR_QMPI_pointers[MPI_ISEND_T] = (void (*)(void)) &QMPI_Isend;
    MPIR_QMPI_pointers[MPI_ISSEND_T] = (void (*)(void)) &QMPI_Issend;
    MPIR_QMPI_pointers[MPI_KEYVAL_CREATE_T] = (void (*)(void)) &QMPI_Keyval_create;
    MPIR_QMPI_pointers[MPI_KEYVAL_FREE_T] = (void (*)(void)) &QMPI_Keyval_free;
    MPIR_QMPI_pointers[MPI_LOOKUP_NAME_T] = (void (*)(void)) &QMPI_Lookup_name;
    MPIR_QMPI_pointers[MPI_MPROBE_T] = (void (*)(void)) &QMPI_Mprobe;
    MPIR_QMPI_pointers[MPI_MRECV_T] = (void (*)(void)) &QMPI_Mrecv;
    MPIR_QMPI_pointers[MPI_NEIGHBOR_ALLGATHER_T] = (void (*)(void)) &QMPI_Neighbor_allgather;
    MPIR_QMPI_pointers[MPI_NEIGHBOR_ALLGATHERV_T] = (void (*)(void)) &QMPI_Neighbor_allgatherv;
    MPIR_QMPI_pointers[MPI_NEIGHBOR_ALLTOALL_T] = (void (*)(void)) &QMPI_Neighbor_alltoall;
    MPIR_QMPI_pointers[MPI_NEIGHBOR_ALLTOALLV_T] = (void (*)(void)) &QMPI_Neighbor_alltoallv;
    MPIR_QMPI_pointers[MPI_NEIGHBOR_ALLTOALLW_T] = (void (*)(void)) &QMPI_Neighbor_alltoallw;
    MPIR_QMPI_pointers[MPI_OP_COMMUTATIVE_T] = (void (*)(void)) &QMPI_Op_commutative;
    MPIR_QMPI_pointers[MPI_OP_CREATE_T] = (void (*)(void)) &QMPI_Op_create;
    MPIR_QMPI_pointers[MPI_OP_FREE_T] = (void (*)(void)) &QMPI_Op_free;
    MPIR_QMPI_pointers[MPI_OPEN_PORT_T] = (void (*)(void)) &QMPI_Open_port;
    MPIR_QMPI_pointers[MPI_PACK_T] = (void (*)(void)) &QMPI_Pack;
    MPIR_QMPI_pointers[MPI_PACK_EXTERNAL_T] = (void (*)(void)) &QMPI_Pack_external;
    MPIR_QMPI_pointers[MPI_PACK_EXTERNAL_SIZE_T] = (void (*)(void)) &QMPI_Pack_external_size;
    MPIR_QMPI_pointers[MPI_PACK_SIZE_T] = (void (*)(void)) &QMPI_Pack_size;
    MPIR_QMPI_pointers[MPI_PCONTROL_T] = (void (*)(void)) &QMPI_Pcontrol;
    MPIR_QMPI_pointers[MPI_PROBE_T] = (void (*)(void)) &QMPI_Probe;
    MPIR_QMPI_pointers[MPI_PUBLISH_NAME_T] = (void (*)(void)) &QMPI_Publish_name;
    MPIR_QMPI_pointers[MPI_PUT_T] = (void (*)(void)) &QMPI_Put;
    MPIR_QMPI_pointers[MPI_QUERY_THREAD_T] = (void (*)(void)) &QMPI_Query_thread;
    MPIR_QMPI_pointers[MPI_RACCUMULATE_T] = (void (*)(void)) &QMPI_Raccumulate;
    MPIR_QMPI_pointers[MPI_RECV_T] = (void (*)(void)) &QMPI_Recv;
    MPIR_QMPI_pointers[MPI_RECV_INIT_T] = (void (*)(void)) &QMPI_Recv_init;
    MPIR_QMPI_pointers[MPI_REDUCE_T] = (void (*)(void)) &QMPI_Reduce;
    MPIR_QMPI_pointers[MPI_REDUCE_LOCAL_T] = (void (*)(void)) &QMPI_Reduce_local;
    MPIR_QMPI_pointers[MPI_REDUCE_SCATTER_T] = (void (*)(void)) &QMPI_Reduce_scatter;
    MPIR_QMPI_pointers[MPI_REDUCE_SCATTER_BLOCK_T] = (void (*)(void)) &QMPI_Reduce_scatter_block;
    MPIR_QMPI_pointers[MPI_REQUEST_FREE_T] = (void (*)(void)) &QMPI_Request_free;
    MPIR_QMPI_pointers[MPI_REQUEST_GET_STATUS_T] = (void (*)(void)) &QMPI_Request_get_status;
    MPIR_QMPI_pointers[MPI_RGET_T] = (void (*)(void)) &QMPI_Rget;
    MPIR_QMPI_pointers[MPI_RGET_ACCUMULATE_T] = (void (*)(void)) &QMPI_Rget_accumulate;
    MPIR_QMPI_pointers[MPI_RPUT_T] = (void (*)(void)) &QMPI_Rput;
    MPIR_QMPI_pointers[MPI_RSEND_T] = (void (*)(void)) &QMPI_Rsend;
    MPIR_QMPI_pointers[MPI_RSEND_INIT_T] = (void (*)(void)) &QMPI_Rsend_init;
    MPIR_QMPI_pointers[MPI_SCAN_T] = (void (*)(void)) &QMPI_Scan;
    MPIR_QMPI_pointers[MPI_SCATTER_T] = (void (*)(void)) &QMPI_Scatter;
    MPIR_QMPI_pointers[MPI_SCATTERV_T] = (void (*)(void)) &QMPI_Scatterv;
    MPIR_QMPI_pointers[MPI_SEND_T] = (void (*)(void)) &QMPI_Send;
    MPIR_QMPI_pointers[MPI_SEND_INIT_T] = (void (*)(void)) &QMPI_Send_init;
    MPIR_QMPI_pointers[MPI_SENDRECV_T] = (void (*)(void)) &QMPI_Sendrecv;
    MPIR_QMPI_pointers[MPI_SENDRECV_REPLACE_T] = (void (*)(void)) &QMPI_Sendrecv_replace;
    MPIR_QMPI_pointers[MPI_SSEND_T] = (void (*)(void)) &QMPI_Ssend;
    MPIR_QMPI_pointers[MPI_SSEND_INIT_T] = (void (*)(void)) &QMPI_Ssend_init;
    MPIR_QMPI_pointers[MPI_START_T] = (void (*)(void)) &QMPI_Start;
    MPIR_QMPI_pointers[MPI_STARTALL_T] = (void (*)(void)) &QMPI_Startall;
    MPIR_QMPI_pointers[MPI_STATUS_SET_CANCELLED_T] = (void (*)(void)) &QMPI_Status_set_cancelled;
    MPIR_QMPI_pointers[MPI_STATUS_SET_ELEMENTS_T] = (void (*)(void)) &QMPI_Status_set_elements;
    MPIR_QMPI_pointers[MPI_STATUS_SET_ELEMENTS_X_T] = (void (*)(void)) &QMPI_Status_set_elements_x;
    MPIR_QMPI_pointers[MPI_T_CATEGORY_CHANGED_T] = (void (*)(void)) &QMPI_T_category_changed;
    MPIR_QMPI_pointers[MPI_T_CATEGORY_GET_CATEGORIES_T] =
        (void (*)(void)) &QMPI_T_category_get_categories;
    MPIR_QMPI_pointers[MPI_T_CATEGORY_GET_CVARS_T] = (void (*)(void)) &QMPI_T_category_get_cvars;
    MPIR_QMPI_pointers[MPI_T_CATEGORY_GET_INDEX_T] = (void (*)(void)) &QMPI_T_category_get_index;
    MPIR_QMPI_pointers[MPI_T_CATEGORY_GET_INFO_T] = (void (*)(void)) &QMPI_T_category_get_info;
    MPIR_QMPI_pointers[MPI_T_CATEGORY_GET_NUM_T] = (void (*)(void)) &QMPI_T_category_get_num;
    MPIR_QMPI_pointers[MPI_T_CATEGORY_GET_PVARS_T] = (void (*)(void)) &QMPI_T_category_get_pvars;
    MPIR_QMPI_pointers[MPI_T_CVAR_GET_INDEX_T] = (void (*)(void)) &QMPI_T_cvar_get_index;
    MPIR_QMPI_pointers[MPI_T_CVAR_GET_INFO_T] = (void (*)(void)) &QMPI_T_cvar_get_info;
    MPIR_QMPI_pointers[MPI_T_CVAR_GET_NUM_T] = (void (*)(void)) &QMPI_T_cvar_get_num;
    MPIR_QMPI_pointers[MPI_T_CVAR_HANDLE_ALLOC_T] = (void (*)(void)) &QMPI_T_cvar_handle_alloc;
    MPIR_QMPI_pointers[MPI_T_CVAR_HANDLE_FREE_T] = (void (*)(void)) &QMPI_T_cvar_handle_free;
    MPIR_QMPI_pointers[MPI_T_CVAR_READ_T] = (void (*)(void)) &QMPI_T_cvar_read;
    MPIR_QMPI_pointers[MPI_T_CVAR_WRITE_T] = (void (*)(void)) &QMPI_T_cvar_write;
    MPIR_QMPI_pointers[MPI_T_ENUM_GET_INFO_T] = (void (*)(void)) &QMPI_T_enum_get_info;
    MPIR_QMPI_pointers[MPI_T_ENUM_GET_ITEM_T] = (void (*)(void)) &QMPI_T_enum_get_item;
    MPIR_QMPI_pointers[MPI_T_FINALIZE_T] = (void (*)(void)) &QMPI_T_finalize;
    MPIR_QMPI_pointers[MPI_T_INIT_THREAD_T] = (void (*)(void)) &QMPI_T_init_thread;
    MPIR_QMPI_pointers[MPI_T_PVAR_GET_INDEX_T] = (void (*)(void)) &QMPI_T_pvar_get_index;
    MPIR_QMPI_pointers[MPI_T_PVAR_GET_INFO_T] = (void (*)(void)) &QMPI_T_pvar_get_info;
    MPIR_QMPI_pointers[MPI_T_PVAR_GET_NUM_T] = (void (*)(void)) &QMPI_T_pvar_get_num;
    MPIR_QMPI_pointers[MPI_T_PVAR_HANDLE_ALLOC_T] = (void (*)(void)) &QMPI_T_pvar_handle_alloc;
    MPIR_QMPI_pointers[MPI_T_PVAR_HANDLE_FREE_T] = (void (*)(void)) &QMPI_T_pvar_handle_free;
    MPIR_QMPI_pointers[MPI_T_PVAR_READ_T] = (void (*)(void)) &QMPI_T_pvar_read;
    MPIR_QMPI_pointers[MPI_T_PVAR_READRESET_T] = (void (*)(void)) &QMPI_T_pvar_readreset;
    MPIR_QMPI_pointers[MPI_T_PVAR_RESET_T] = (void (*)(void)) &QMPI_T_pvar_reset;
    MPIR_QMPI_pointers[MPI_T_PVAR_SESSION_CREATE_T] = (void (*)(void)) &QMPI_T_pvar_session_create;
    MPIR_QMPI_pointers[MPI_T_PVAR_SESSION_FREE_T] = (void (*)(void)) &QMPI_T_pvar_session_free;
    MPIR_QMPI_pointers[MPI_T_PVAR_START_T] = (void (*)(void)) &QMPI_T_pvar_start;
    MPIR_QMPI_pointers[MPI_T_PVAR_STOP_T] = (void (*)(void)) &QMPI_T_pvar_stop;
    MPIR_QMPI_pointers[MPI_T_PVAR_WRITE_T] = (void (*)(void)) &QMPI_T_pvar_write;
    MPIR_QMPI_pointers[MPI_TEST_T] = (void (*)(void)) &QMPI_Test;
    MPIR_QMPI_pointers[MPI_TEST_CANCELLED_T] = (void (*)(void)) &QMPI_Test_cancelled;
    MPIR_QMPI_pointers[MPI_TESTALL_T] = (void (*)(void)) &QMPI_Testall;
    MPIR_QMPI_pointers[MPI_TESTANY_T] = (void (*)(void)) &QMPI_Testany;
    MPIR_QMPI_pointers[MPI_TESTSOME_T] = (void (*)(void)) &QMPI_Testsome;
    MPIR_QMPI_pointers[MPI_TOPO_TEST_T] = (void (*)(void)) &QMPI_Topo_test;
    MPIR_QMPI_pointers[MPI_TYPE_COMMIT_T] = (void (*)(void)) &QMPI_Type_commit;
    MPIR_QMPI_pointers[MPI_TYPE_CONTIGUOUS_T] = (void (*)(void)) &QMPI_Type_contiguous;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_DARRAY_T] = (void (*)(void)) &QMPI_Type_create_darray;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_F90_COMPLEX_T] =
        (void (*)(void)) &QMPI_Type_create_f90_complex;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_F90_INTEGER_T] =
        (void (*)(void)) &QMPI_Type_create_f90_integer;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_F90_REAL_T] = (void (*)(void)) &QMPI_Type_create_f90_real;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_HINDEXED_T] = (void (*)(void)) &QMPI_Type_create_hindexed;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_HINDEXED_BLOCK_T] =
        (void (*)(void)) &QMPI_Type_create_hindexed_block;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_HVECTOR_T] = (void (*)(void)) &QMPI_Type_create_hvector;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_INDEXED_BLOCK_T] =
        (void (*)(void)) &QMPI_Type_create_indexed_block;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_KEYVAL_T] = (void (*)(void)) &QMPI_Type_create_keyval;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_RESIZED_T] = (void (*)(void)) &QMPI_Type_create_resized;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_STRUCT_T] = (void (*)(void)) &QMPI_Type_create_struct;
    MPIR_QMPI_pointers[MPI_TYPE_CREATE_SUBARRAY_T] = (void (*)(void)) &QMPI_Type_create_subarray;
    MPIR_QMPI_pointers[MPI_TYPE_DELETE_ATTR_T] = (void (*)(void)) &QMPI_Type_delete_attr;
    MPIR_QMPI_pointers[MPI_TYPE_DUP_T] = (void (*)(void)) &QMPI_Type_dup;
    MPIR_QMPI_pointers[MPI_TYPE_FREE_T] = (void (*)(void)) &QMPI_Type_free;
    MPIR_QMPI_pointers[MPI_TYPE_FREE_KEYVAL_T] = (void (*)(void)) &QMPI_Type_free_keyval;
    MPIR_QMPI_pointers[MPI_TYPE_GET_ATTR_T] = (void (*)(void)) &QMPI_Type_get_attr;
    MPIR_QMPI_pointers[MPI_TYPE_GET_CONTENTS_T] = (void (*)(void)) &QMPI_Type_get_contents;
    MPIR_QMPI_pointers[MPI_TYPE_GET_ENVELOPE_T] = (void (*)(void)) &QMPI_Type_get_envelope;
    MPIR_QMPI_pointers[MPI_TYPE_GET_EXTENT_T] = (void (*)(void)) &QMPI_Type_get_extent;
    MPIR_QMPI_pointers[MPI_TYPE_GET_EXTENT_X_T] = (void (*)(void)) &QMPI_Type_get_extent_x;
    MPIR_QMPI_pointers[MPI_TYPE_GET_NAME_T] = (void (*)(void)) &QMPI_Type_get_name;
    MPIR_QMPI_pointers[MPI_TYPE_GET_TRUE_EXTENT_T] = (void (*)(void)) &QMPI_Type_get_true_extent;
    MPIR_QMPI_pointers[MPI_TYPE_GET_TRUE_EXTENT_X_T] =
        (void (*)(void)) &QMPI_Type_get_true_extent_x;
    MPIR_QMPI_pointers[MPI_TYPE_INDEXED_T] = (void (*)(void)) &QMPI_Type_indexed;
    MPIR_QMPI_pointers[MPI_TYPE_MATCH_SIZE_T] = (void (*)(void)) &QMPI_Type_match_size;
    MPIR_QMPI_pointers[MPI_TYPE_SET_ATTR_T] = (void (*)(void)) &QMPI_Type_set_attr;
    MPIR_QMPI_pointers[MPI_TYPE_SET_NAME_T] = (void (*)(void)) &QMPI_Type_set_name;
    MPIR_QMPI_pointers[MPI_TYPE_SIZE_T] = (void (*)(void)) &QMPI_Type_size;
    MPIR_QMPI_pointers[MPI_TYPE_SIZE_X_T] = (void (*)(void)) &QMPI_Type_size_x;
    MPIR_QMPI_pointers[MPI_TYPE_VECTOR_T] = (void (*)(void)) &QMPI_Type_vector;
    MPIR_QMPI_pointers[MPI_UNPACK_T] = (void (*)(void)) &QMPI_Unpack;
    MPIR_QMPI_pointers[MPI_UNPACK_EXTERNAL_T] = (void (*)(void)) &QMPI_Unpack_external;
    MPIR_QMPI_pointers[MPI_UNPUBLISH_NAME_T] = (void (*)(void)) &QMPI_Unpublish_name;
    MPIR_QMPI_pointers[MPI_WAIT_T] = (void (*)(void)) &QMPI_Wait;
    MPIR_QMPI_pointers[MPI_WAITALL_T] = (void (*)(void)) &QMPI_Waitall;
    MPIR_QMPI_pointers[MPI_WAITANY_T] = (void (*)(void)) &QMPI_Waitany;
    MPIR_QMPI_pointers[MPI_WAITSOME_T] = (void (*)(void)) &QMPI_Waitsome;
    MPIR_QMPI_pointers[MPI_WIN_ALLOCATE_T] = (void (*)(void)) &QMPI_Win_allocate;
    MPIR_QMPI_pointers[MPI_WIN_ALLOCATE_SHARED_T] = (void (*)(void)) &QMPI_Win_allocate_shared;
    MPIR_QMPI_pointers[MPI_WIN_ATTACH_T] = (void (*)(void)) &QMPI_Win_attach;
    MPIR_QMPI_pointers[MPI_WIN_CALL_ERRHANDLER_T] = (void (*)(void)) &QMPI_Win_call_errhandler;
    MPIR_QMPI_pointers[MPI_WIN_COMPLETE_T] = (void (*)(void)) &QMPI_Win_complete;
    MPIR_QMPI_pointers[MPI_WIN_CREATE_T] = (void (*)(void)) &QMPI_Win_create;
    MPIR_QMPI_pointers[MPI_WIN_CREATE_DYNAMIC_T] = (void (*)(void)) &QMPI_Win_create_dynamic;
    MPIR_QMPI_pointers[MPI_WIN_CREATE_ERRHANDLER_T] = (void (*)(void)) &QMPI_Win_create_errhandler;
    MPIR_QMPI_pointers[MPI_WIN_CREATE_KEYVAL_T] = (void (*)(void)) &QMPI_Win_create_keyval;
    MPIR_QMPI_pointers[MPI_WIN_DELETE_ATTR_T] = (void (*)(void)) &QMPI_Win_delete_attr;
    MPIR_QMPI_pointers[MPI_WIN_DETACH_T] = (void (*)(void)) &QMPI_Win_detach;
    MPIR_QMPI_pointers[MPI_WIN_FENCE_T] = (void (*)(void)) &QMPI_Win_fence;
    MPIR_QMPI_pointers[MPI_WIN_FLUSH_T] = (void (*)(void)) &QMPI_Win_flush;
    MPIR_QMPI_pointers[MPI_WIN_FLUSH_ALL_T] = (void (*)(void)) &QMPI_Win_flush_all;
    MPIR_QMPI_pointers[MPI_WIN_FLUSH_LOCAL_T] = (void (*)(void)) &QMPI_Win_flush_local;
    MPIR_QMPI_pointers[MPI_WIN_FLUSH_LOCAL_ALL_T] = (void (*)(void)) &QMPI_Win_flush_local_all;
    MPIR_QMPI_pointers[MPI_WIN_FREE_T] = (void (*)(void)) &QMPI_Win_free;
    MPIR_QMPI_pointers[MPI_WIN_FREE_KEYVAL_T] = (void (*)(void)) &QMPI_Win_free_keyval;
    MPIR_QMPI_pointers[MPI_WIN_GET_ATTR_T] = (void (*)(void)) &QMPI_Win_get_attr;
    MPIR_QMPI_pointers[MPI_WIN_GET_ERRHANDLER_T] = (void (*)(void)) &QMPI_Win_get_errhandler;
    MPIR_QMPI_pointers[MPI_WIN_GET_GROUP_T] = (void (*)(void)) &QMPI_Win_get_group;
    MPIR_QMPI_pointers[MPI_WIN_GET_INFO_T] = (void (*)(void)) &QMPI_Win_get_info;
    MPIR_QMPI_pointers[MPI_WIN_GET_NAME_T] = (void (*)(void)) &QMPI_Win_get_name;
    MPIR_QMPI_pointers[MPI_WIN_LOCK_T] = (void (*)(void)) &QMPI_Win_lock;
    MPIR_QMPI_pointers[MPI_WIN_LOCK_ALL_T] = (void (*)(void)) &QMPI_Win_lock_all;
    MPIR_QMPI_pointers[MPI_WIN_POST_T] = (void (*)(void)) &QMPI_Win_post;
    MPIR_QMPI_pointers[MPI_WIN_SET_ATTR_T] = (void (*)(void)) &QMPI_Win_set_attr;
    MPIR_QMPI_pointers[MPI_WIN_SET_ERRHANDLER_T] = (void (*)(void)) &QMPI_Win_set_errhandler;
    MPIR_QMPI_pointers[MPI_WIN_SET_INFO_T] = (void (*)(void)) &QMPI_Win_set_info;
    MPIR_QMPI_pointers[MPI_WIN_SET_NAME_T] = (void (*)(void)) &QMPI_Win_set_name;
    MPIR_QMPI_pointers[MPI_WIN_SHARED_QUERY_T] = (void (*)(void)) &QMPI_Win_shared_query;
    MPIR_QMPI_pointers[MPI_WIN_START_T] = (void (*)(void)) &QMPI_Win_start;
    MPIR_QMPI_pointers[MPI_WIN_SYNC_T] = (void (*)(void)) &QMPI_Win_sync;
    MPIR_QMPI_pointers[MPI_WIN_TEST_T] = (void (*)(void)) &QMPI_Win_test;
    MPIR_QMPI_pointers[MPI_WIN_UNLOCK_T] = (void (*)(void)) &QMPI_Win_unlock;
    MPIR_QMPI_pointers[MPI_WIN_UNLOCK_ALL_T] = (void (*)(void)) &QMPI_Win_unlock_all;
    MPIR_QMPI_pointers[MPI_WIN_WAIT_T] = (void (*)(void)) &QMPI_Win_wait;
    MPIR_QMPI_pointers[MPI_WTICK_T] = (void (*)(void)) &QMPI_Wtick;
    MPIR_QMPI_pointers[MPI_WTIME_T] = (void (*)(void)) &QMPI_Wtime;

    return 0;
}

int MPII_qmpi_setup(void)
{
    int mpi_errno = MPI_SUCCESS;
    size_t len = 0;

    mpi_errno = MPIR_T_env_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* Parse environment variable to get the number and list of tools */
    if (MPIR_CVAR_QMPI_TOOL_LIST == NULL) {
        MPIR_QMPI_num_tools = 0;
        goto fn_exit;
    } else {
        MPIR_QMPI_num_tools = 1;
        len = strlen(MPIR_CVAR_QMPI_TOOL_LIST);
    }

    for (int i = 0; i < len; i++) {
        if (MPIR_CVAR_QMPI_TOOL_LIST[i] == ':') {
            MPIR_QMPI_num_tools++;
        }
    }
    MPIR_QMPI_tool_names = MPL_calloc(MPIR_QMPI_num_tools + 1, sizeof(char *), MPL_MEM_OTHER);
    if (!MPIR_QMPI_tool_names) {
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_num_tools");
        goto fn_exit;
    }
    for (int i = 0; i < MPIR_QMPI_num_tools + 1; i++) {
        MPIR_QMPI_tool_names[i] =
            MPL_calloc(QMPI_MAX_TOOL_NAME_LENGTH, sizeof(char), MPL_MEM_OTHER);
        if (!MPIR_QMPI_tool_names[i]) {
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_num_tools");
            goto fn_exit;
        }
    }

    char *save_ptr, *val;
    char *tmp_str = MPL_strdup(MPIR_CVAR_QMPI_TOOL_LIST);
    val = strtok_r(tmp_str, ":", &save_ptr);
    int tool_num = 1;
    while (val != NULL) {
        strncpy(MPIR_QMPI_tool_names[tool_num], val, QMPI_MAX_TOOL_NAME_LENGTH);
        /* Make sure the string is null terminated */
        MPIR_QMPI_tool_names[tool_num][QMPI_MAX_TOOL_NAME_LENGTH - 1] = '\0';
        tool_num++;
        val = strtok_r(NULL, ":", &save_ptr);
    }
    MPL_free(tmp_str);
    MPIR_QMPI_num_tools = tool_num - 1;

    /* Allocate space for as many tools as we say we support */
    MPIR_QMPI_pointers = MPL_calloc(1, sizeof(generic_mpi_func *) * MPI_LAST_FUNC_T *
                                    (MPIR_QMPI_num_tools + 1), MPL_MEM_OTHER);
    if (!MPIR_QMPI_pointers) {
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_pointers");
        goto fn_exit;
    }

    MPIR_QMPI_storage = MPL_calloc(1, sizeof(void *) * (MPIR_QMPI_num_tools + 1), MPL_MEM_OTHER);
    if (!MPIR_QMPI_storage) {
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_storage");
        goto fn_exit;
    }

    MPIR_QMPI_tool_init_callbacks = MPL_calloc(1, sizeof(generic_mpi_func *) * MPI_LAST_FUNC_T *
                                               (MPIR_QMPI_num_tools + 1), MPL_MEM_OTHER);
    if (!MPIR_QMPI_tool_init_callbacks) {
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_QMPI_pointers");
        goto fn_exit;
    }

    /* Call generated function to register the internal function pointers. */
    MPII_qmpi_register_internal_functions();

    MPIR_QMPI_storage[0] = NULL;

    MPIR_QMPI_is_initialized = 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*@
   QMPIX_Register - Register a QMPI tool.

Output Parameters:
. funcs - A struct of functions that includes every function for every attached QMPI tool, in
addition to the MPI library itself.

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int QMPI_Register_tool_name(const char *tool_name, void (*init_function_ptr) (int tool_id))
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_QMPIX_REGISTER);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_QMPIX_REGISTER);

    /* ... body of routine ...  */

    /* If the QMPI globals have not yet been set up, do so now. This does not need to be protected
     * by a mutex because it will be called before the application has even reached its main
     * function. */
    if (!MPIR_QMPI_is_initialized) {
        MPII_qmpi_setup();
    }

    if (MPIR_QMPI_num_tools) {
        /* Copy the tool information into internal arrays */
        bool found = false;
        for (int i = 1; i < MPIR_QMPI_num_tools + 1; i++) {
            if (strcmp(MPIR_QMPI_tool_names[i], tool_name) == 0) {
                MPIR_QMPI_tool_init_callbacks[i] = init_function_ptr;
            }
        }
        if (!found) {
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**qmpi_invalid_name", "**qmpi_invalid_name %s",
                                 tool_name);
            goto fn_exit;
        }
    }

    /* ... end of body of routine ... */

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_QMPIX_REGISTER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int QMPI_Register_tool_storage(int tool_id, void *tool_storage)
{
    int mpi_errno = MPI_SUCCESS;

    /* Store the tool's context information */
    MPIR_QMPI_storage[tool_id] = tool_storage;

    return mpi_errno;
}

int QMPI_Register_function(int tool_id, enum QMPI_Functions_enum function_enum,
                           void (*function_ptr) (void))
{
    int mpi_errno = MPI_SUCCESS;

    if (function_enum < 0 && function_enum >= MPI_LAST_FUNC_T) {
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                             MPI_ERR_ARG, "**arg", "**arg %s", "invalid enum value");
        goto fn_exit;
    }

    MPIR_QMPI_pointers[tool_id * MPI_LAST_FUNC_T + function_enum] = function_ptr;

  fn_exit:
    return mpi_errno;
}

int QMPI_Get_function(int calling_tool_id, enum QMPI_Functions_enum function_enum,
                      void (**function_ptr) (void), QMPI_Context * next_tool_context,
                      int *next_tool_id)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = calling_tool_id - 1; i >= 0; i--) {
        if (MPIR_QMPI_pointers[i * MPI_LAST_FUNC_T + function_enum] != NULL) {
            *function_ptr = MPIR_QMPI_pointers[i * MPI_LAST_FUNC_T + function_enum];
            *next_tool_id = i;
            return mpi_errno;
        }
    }

    return mpi_errno;
}
