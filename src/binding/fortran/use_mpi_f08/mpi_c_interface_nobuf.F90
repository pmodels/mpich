!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
module mpi_c_interface_nobuf

implicit none

interface

function MPIR_Buffer_detach_c(buffer_addr, size) &
    bind(C, name="PMPI_Buffer_detach") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    implicit none
    type(c_ptr), intent(out) :: buffer_addr
    integer(c_int), intent(out) :: size
    integer(c_int) :: ierror
end function MPIR_Buffer_detach_c

function MPIR_Cancel_c(request) &
    bind(C, name="PMPI_Cancel") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    ! No value attribute for request due to an oversight in MPI-1.0
    ! See P72 of MPI-3.0
    integer(c_Request), intent(in) :: request
    integer(c_int) :: ierror
end function MPIR_Cancel_c

function MPIR_Get_count_c(status, datatype, count) &
    bind(C, name="PMPI_Get_count") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    type(c_ptr), value, intent(in) :: status
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), intent(out) :: count
    integer(c_int) :: ierror
end function MPIR_Get_count_c

function MPIR_Start_c(request) &
    bind(C, name="PMPI_Start") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_Request), intent(inout) :: request
    integer(c_int) :: ierror
end function MPIR_Start_c

function MPIR_Startall_c(count, array_of_requests) &
    bind(C, name="PMPI_Startall") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_int), value, intent(in) :: count
    integer(c_Request), intent(inout) :: array_of_requests(count)
    integer(c_int) :: ierror
end function MPIR_Startall_c

function MPIR_Test_c(request, flag, status) &
    bind(C, name="PMPI_Test") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_Request), intent(inout) :: request
    integer(c_int), intent(out) :: flag
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Test_c

function MPIR_Testall_c(count, array_of_requests, flag, array_of_statuses) &
    bind(C, name="PMPI_Testall") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_int), value, intent(in) :: count
    integer(c_Request), intent(inout) :: array_of_requests(count)
    integer(c_int), intent(out) :: flag
    type(c_ptr), value, intent(in) :: array_of_statuses
    integer(c_int) :: ierror
end function MPIR_Testall_c

function MPIR_Testany_c(count, array_of_requests, index, flag, status) &
    bind(C, name="PMPI_Testany") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_int), value, intent(in) :: count
    integer(c_Request), intent(inout) :: array_of_requests(count)
    integer(c_int), intent(out) :: index
    integer(c_int), intent(out) :: flag
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Testany_c

function MPIR_Testsome_c(incount, array_of_requests, outcount,  &
          array_of_indices, array_of_statuses) bind(C, name="PMPI_Testsome") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_int), value, intent(in) :: incount
    integer(c_Request), intent(inout) :: array_of_requests(incount)
    integer(c_int), intent(out) :: outcount, array_of_indices(*)
    type(c_ptr), value, intent(in) :: array_of_statuses
    integer(c_int) :: ierror
end function MPIR_Testsome_c

function MPIR_Test_cancelled_c(status, flag) &
    bind(C, name="PMPI_Test_cancelled") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    implicit none
    type(c_ptr), value, intent(in) :: status
    integer(c_int), intent(out) :: flag
    integer(c_int) :: ierror
end function MPIR_Test_cancelled_c

function MPIR_Wait_c(request, status) &
    bind(C, name="PMPI_Wait") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_Request), intent(inout) :: request
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Wait_c

function MPIR_Waitall_c(count, array_of_requests, array_of_statuses) &
    bind(C, name="PMPI_Waitall") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_int), value, intent(in) :: count
    integer(c_Request), intent(inout) :: array_of_requests(count)
    type(c_ptr), value, intent(in) :: array_of_statuses
    integer(c_int) :: ierror
end function MPIR_Waitall_c

function MPIR_Waitany_c(count, array_of_requests, index, status) &
    bind(C, name="PMPI_Waitany") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_int), value, intent(in) :: count
    integer(c_Request), intent(inout) :: array_of_requests(count)
    integer(c_int), intent(out) :: index
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Waitany_c

function MPIR_Waitsome_c(incount, array_of_requests, outcount, &
           array_of_indices, array_of_statuses) &
    bind(C, name="PMPI_Waitsome") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_int), value, intent(in) :: incount
    integer(c_Request), intent(inout) :: array_of_requests(incount)
    integer(c_int), intent(out) :: outcount, array_of_indices(*)
    type(c_ptr), value, intent(in) :: array_of_statuses
    integer(c_int) :: ierror
end function MPIR_Waitsome_c

function MPIR_Get_elements_c(status, datatype, count) &
    bind(C, name="PMPI_Get_elements") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    type(c_ptr), value, intent(in) :: status
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), intent(out) :: count
    integer(c_int) :: ierror
end function MPIR_Get_elements_c

function MPIR_Get_elements_x_c(status, datatype, count) &
    bind(C, name="PMPI_Get_elements_x") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08_compile_constants, only : MPI_COUNT_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    type(c_ptr), value, intent(in) :: status
    integer(c_Datatype), value, intent(in) :: datatype
    integer(MPI_COUNT_KIND), intent(out) :: count
    integer(c_int) :: ierror
end function MPIR_Get_elements_x_c

function MPIR_Pack_external_size_c(datarep, incount, datatype, size) &
    bind(C, name="PMPI_Pack_external_size") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), value, intent(in) :: incount
    character(kind=c_char), intent(in) :: datarep(*)
    integer(MPI_ADDRESS_KIND), intent(out) :: size
    integer(c_int) :: ierror
end function MPIR_Pack_external_size_c

function MPIR_Pack_size_c(incount, datatype, comm, size) &
    bind(C, name="PMPI_Pack_size") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    integer(c_int), value, intent(in) :: incount
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(out) :: size
    integer(c_int) :: ierror
end function MPIR_Pack_size_c

function MPIR_Type_commit_c(datatype) &
    bind(C, name="PMPI_Type_commit") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), intent(inout) :: datatype
    integer(c_int) :: ierror
end function MPIR_Type_commit_c

function MPIR_Type_contiguous_c(count, oldtype, newtype) &
    bind(C, name="PMPI_Type_contiguous") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: oldtype
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_contiguous_c

function MPIR_Type_create_darray_c(size, rank, ndims, array_of_gsizes, &
           array_of_distribs, array_of_dargs, array_of_psizes, order, oldtype, newtype) &
    bind(C, name="PMPI_Type_create_darray") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: size, rank, ndims, order
    integer(c_int), intent(in) :: array_of_gsizes(ndims), array_of_distribs(ndims)
    integer(c_int), intent(in) :: array_of_dargs(ndims), array_of_psizes(ndims)
    integer(c_Datatype), value, intent(in) :: oldtype
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_create_darray_c

function MPIR_Type_create_hindexed_c(count, array_of_blocklengths, &
           array_of_displacements, oldtype, newtype) &
    bind(C, name="PMPI_Type_create_hindexed") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: count
    integer(c_int), intent(in) :: array_of_blocklengths(count)
    integer(MPI_ADDRESS_KIND), intent(in) :: array_of_displacements(count)
    integer(c_Datatype), value, intent(in) :: oldtype
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_create_hindexed_c

function MPIR_Type_create_hvector_c(count, blocklength, stride, oldtype, newtype) &
    bind(C, name="PMPI_Type_create_hvector") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: count, blocklength
    integer(MPI_ADDRESS_KIND), value, intent(in) :: stride
    integer(c_Datatype), value, intent(in) :: oldtype
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_create_hvector_c

function MPIR_Type_create_indexed_block_c(count, blocklength, &
            array_of_displacements, oldtype, newtype) &
    bind(C, name="PMPI_Type_create_indexed_block") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: count, blocklength
    integer(c_int), intent(in) :: array_of_displacements(count)
    integer(c_Datatype), value, intent(in) :: oldtype
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_create_indexed_block_c

function MPIR_Type_create_hindexed_block_c(count, blocklength, &
           array_of_displacements, oldtype, newtype) &
    bind(C, name="PMPI_Type_create_hindexed_block") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: count, blocklength
    integer(MPI_ADDRESS_KIND), intent(in) :: array_of_displacements(count)
    integer(c_Datatype), value, intent(in) :: oldtype
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_create_hindexed_block_c

function MPIR_Type_create_resized_c(oldtype, lb, extent, newtype) &
    bind(C, name="PMPI_Type_create_resized") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(MPI_ADDRESS_KIND), value, intent(in) :: lb, extent
    integer(c_Datatype), value, intent(in) :: oldtype
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_create_resized_c

function MPIR_Type_create_struct_c(count, array_of_blocklengths, &
           array_of_displacements, array_of_types, newtype) &
    bind(C, name="PMPI_Type_create_struct") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: count
    integer(c_int), intent(in) :: array_of_blocklengths(count)
    integer(MPI_ADDRESS_KIND), intent(in) :: array_of_displacements(count)
    integer(c_Datatype), intent(in) :: array_of_types(count)
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_create_struct_c

function MPIR_Type_create_subarray_c(ndims, array_of_sizes, array_of_subsizes, &
           array_of_starts, order, oldtype, newtype) &
    bind(C, name="PMPI_Type_create_subarray") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: ndims, order
    integer(c_int), intent(in) :: array_of_sizes(ndims), array_of_subsizes(ndims)
    integer(c_int), intent(in) :: array_of_starts(ndims)
    integer(c_Datatype), value, intent(in) :: oldtype
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_create_subarray_c

function MPIR_Type_dup_c(oldtype, newtype) &
    bind(C, name="PMPI_Type_dup") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: oldtype
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_dup_c

function MPIR_Type_free_c(datatype) &
    bind(C, name="PMPI_Type_free") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), intent(inout) :: datatype
    integer(c_int) :: ierror
end function MPIR_Type_free_c

function MPIR_Type_get_contents_c(datatype, max_integers, max_addresses, max_datatypes, &
           array_of_integers, array_of_addresses, array_of_datatypes) &
    bind(C, name="PMPI_Type_get_contents") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), value, intent(in) :: max_integers, max_addresses, max_datatypes
    integer(c_int), intent(out) :: array_of_integers(max_integers)
    integer(MPI_ADDRESS_KIND), intent(out) :: array_of_addresses(max_addresses)
    integer(c_Datatype), intent(out) :: array_of_datatypes(max_datatypes)
    integer(c_int) :: ierror
end function MPIR_Type_get_contents_c

function MPIR_Type_get_envelope_c(datatype, num_integers, num_addresses, num_datatypes, combiner) &
    bind(C, name="PMPI_Type_get_envelope") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), intent(out) :: num_integers, num_addresses, num_datatypes, combiner
    integer(c_int) :: ierror
end function MPIR_Type_get_envelope_c

function MPIR_Type_get_extent_c(datatype, lb, extent) &
    bind(C, name="PMPI_Type_get_extent") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(MPI_ADDRESS_KIND), intent(out) :: lb, extent
    integer(c_int) :: ierror
end function MPIR_Type_get_extent_c

function MPIR_Type_get_extent_x_c(datatype, lb, extent) &
    bind(C, name="PMPI_Type_get_extent_x") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_COUNT_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(MPI_COUNT_KIND), intent(out) :: lb, extent
    integer(c_int) :: ierror
end function MPIR_Type_get_extent_x_c

function MPIR_Type_get_true_extent_c(datatype, true_lb, true_extent) &
    bind(C, name="PMPI_Type_get_true_extent") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(MPI_ADDRESS_KIND), intent(out) :: true_lb, true_extent
    integer(c_int) :: ierror
end function MPIR_Type_get_true_extent_c

function MPIR_Type_get_true_extent_x_c(datatype, true_lb, true_extent) &
    bind(C, name="PMPI_Type_get_true_extent_x") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_COUNT_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(MPI_COUNT_KIND), intent(out) :: true_lb, true_extent
    integer(c_int) :: ierror
end function MPIR_Type_get_true_extent_x_c

function MPIR_Type_indexed_c(count, array_of_blocklengths, &
           array_of_displacements, oldtype, newtype) &
    bind(C, name="PMPI_Type_indexed") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: count
    integer(c_int), intent(in) :: array_of_blocklengths(count), array_of_displacements(count)
    integer(c_Datatype), value, intent(in) :: oldtype
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_indexed_c

function MPIR_Type_size_c(datatype, size) &
    bind(C, name="PMPI_Type_size") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), intent(out) :: size
    integer(c_int) :: ierror
end function MPIR_Type_size_c

function MPIR_Type_size_x_c(datatype, size) &
    bind(C, name="PMPI_Type_size_x") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_COUNT_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(MPI_COUNT_KIND), intent(out) :: size
    integer(c_int) :: ierror
end function MPIR_Type_size_x_c

function MPIR_Type_vector_c(count, blocklength, stride, oldtype, newtype) &
    bind(C, name="PMPI_Type_vector") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: count, blocklength, stride
    integer(c_Datatype), value, intent(in) :: oldtype
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_vector_c


function MPIR_Barrier_c(comm) &
    bind(C, name="PMPI_Barrier") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Barrier_c

function MPIR_Ibarrier_c(comm, request) &
    bind(C, name="PMPI_Ibarrier") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm, c_Request
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ibarrier_c

function MPIR_Op_commutative_c(op, commute) &
    bind(C, name="PMPI_Op_commutative") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Op
    implicit none
    integer(c_Op), value, intent(in) :: op
    integer(c_int), intent(out) :: commute
    integer(c_int) :: ierror
end function MPIR_Op_commutative_c

function MPIR_Op_create_c(user_fn, commute, op) &
    bind(C, name="PMPI_Op_create") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_c_interface_types, only : c_Op
    implicit none
    type(c_funptr), value :: user_fn
    integer(c_int), intent(in) :: commute
    integer(c_Op), intent(out) :: op
    integer(c_int) :: ierror
end function MPIR_Op_create_c

function MPIR_Op_free_c(op) &
    bind(C, name="PMPI_Op_free") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Op
    implicit none
    integer(c_Op), intent(inout) :: op
    integer(c_int) :: ierror
end function MPIR_Op_free_c

function MPIR_Comm_compare_c(comm1,comm2,result) &
    bind(C, name="PMPI_Comm_compare") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm1, comm2
    integer(c_int), intent(out) :: result
    integer(c_int) :: ierror
end function MPIR_Comm_compare_c

function MPIR_Comm_create_c(comm, group, newcomm) &
    bind(C, name="PMPI_Comm_create") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group, c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Group), value, intent(in) :: group
    integer(c_Comm), intent(out) :: newcomm
    integer(c_int) :: ierror
end function MPIR_Comm_create_c

function MPIR_Comm_create_group_c(comm, group, tag, newcomm) &
    bind(C, name="PMPI_Comm_create_group") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group, c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Group), value, intent(in) :: group
    integer(c_int), value, intent(in) :: tag
    integer(c_Comm), intent(out) :: newcomm
    integer(c_int) :: ierror
end function MPIR_Comm_create_group_c

function MPIR_Comm_create_keyval_c(comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state) &
    bind(C, name="PMPI_Comm_create_keyval") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(c_funptr), value :: comm_copy_attr_fn
    type(c_funptr), value :: comm_delete_attr_fn
    integer(c_int), intent(out) :: comm_keyval
    integer(MPI_ADDRESS_KIND), value, intent(in) :: extra_state
    integer(c_int) :: ierror
end function MPIR_Comm_create_keyval_c

function MPIR_Comm_delete_attr_c(comm, comm_keyval) &
    bind(C, name="PMPI_Comm_delete_attr") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: comm_keyval
    integer(c_int) :: ierror
end function MPIR_Comm_delete_attr_c

function MPIR_Comm_dup_c(comm, newcomm) &
    bind(C, name="PMPI_Comm_dup") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Comm), intent(out) :: newcomm
    integer(c_int) :: ierror
end function MPIR_Comm_dup_c

function MPIR_Comm_dup_with_info_c(comm, info, newcomm) &
    bind(C, name="PMPI_Comm_dup_with_info") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm, c_Info
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Info), value, intent(in) :: info
    integer(c_Comm), intent(out) :: newcomm
    integer(c_int) :: ierror
end function MPIR_Comm_dup_with_info_c

function MPIR_Comm_idup_c(comm, newcomm, request) &
    bind(C, name="PMPI_Comm_idup") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm, c_Request
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Comm), intent(out), asynchronous :: newcomm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Comm_idup_c

function MPIR_Comm_free_c(comm) &
    bind(C, name="PMPI_Comm_free") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), intent(inout) :: comm
    integer(c_int) :: ierror
end function MPIR_Comm_free_c

function MPIR_Comm_free_keyval_c(comm_keyval) &
    bind(C, name="PMPI_Comm_free_keyval") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), intent(inout) :: comm_keyval
    integer(c_int) :: ierror
end function MPIR_Comm_free_keyval_c

! Not bind to MPI_Comm_get_attr directly due to the attr_type tag
function MPIR_Comm_get_attr_c(comm, comm_keyval, attribute_val, flag, attr_type) &
    bind(C, name="MPIR_CommGetAttr") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Comm
    use :: mpi_c_interface_glue, only : MPIR_ATTR_AINT
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: comm_keyval
    integer(MPI_ADDRESS_KIND), intent(out) :: attribute_val
    integer(c_int), intent(out) :: flag
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type
    integer(c_int) :: ierror
end function MPIR_Comm_get_attr_c

function MPIR_Comm_get_name_c(comm, comm_name, resultlen) &
    bind(C, name="PMPI_Comm_get_name") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_MAX_OBJECT_NAME
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    character(kind=c_char), intent(out) :: comm_name(MPI_MAX_OBJECT_NAME+1)
    integer(c_int), intent(out) :: resultlen
    integer(c_int) :: ierror
end function MPIR_Comm_get_name_c

function MPIR_Comm_group_c(comm, group) &
    bind(C, name="PMPI_Comm_group") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm, c_Group
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Group), intent(out) :: group
    integer(c_int) :: ierror
end function MPIR_Comm_group_c

function MPIR_Comm_rank_c(comm, rank) &
    bind(C, name="PMPI_Comm_rank") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(out) :: rank
    integer(c_int) :: ierror
end function MPIR_Comm_rank_c

function MPIR_Comm_remote_group_c(comm, group) &
    bind(C, name="PMPI_Comm_remote_group") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm, c_Group
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Group), intent(out) :: group
    integer(c_int) :: ierror
end function MPIR_Comm_remote_group_c

function MPIR_Comm_remote_size_c(comm, size) &
    bind(C, name="PMPI_Comm_remote_size") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(out) :: size
    integer(c_int) :: ierror
end function MPIR_Comm_remote_size_c

function MPIR_Comm_set_attr_c(comm, comm_keyval, attribute_val, attr_type) &
    bind(C, name="MPIR_CommSetAttr") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Comm
    use :: mpi_c_interface_glue, only : MPIR_ATTR_AINT
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: comm_keyval
    integer(MPI_ADDRESS_KIND), value, intent(in) :: attribute_val
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type
    integer(c_int) :: ierror
end function MPIR_Comm_set_attr_c

function MPIR_Comm_set_name_c(comm, comm_name) &
    bind(C, name="PMPI_Comm_set_name") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    character(kind=c_char), intent(in) :: comm_name(*)
    integer(c_int) :: ierror
end function MPIR_Comm_set_name_c

function MPIR_Comm_size_c(comm, size) &
    bind(C, name="PMPI_Comm_size") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(out) :: size
    integer(c_int) :: ierror
end function MPIR_Comm_size_c

function MPIR_Comm_split_c(comm, color, key, newcomm) &
    bind(C, name="PMPI_Comm_split") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: color, key
    integer(c_Comm), intent(out) :: newcomm
    integer(c_int) :: ierror
end function MPIR_Comm_split_c

function MPIR_Comm_test_inter_c(comm, flag) &
    bind(C, name="PMPI_Comm_test_inter") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(out) :: flag
    integer(c_int) :: ierror
end function MPIR_Comm_test_inter_c

function MPIR_Comm_get_info_c(comm, info_used) &
    bind(C, name="PMPI_Comm_get_info") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm, c_Info
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Info), intent(out) :: info_used
    integer(c_int) :: ierror
end function MPIR_Comm_get_info_c

function MPIR_Comm_set_info_c(comm, info) &
    bind(C, name="PMPI_Comm_set_info") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm, c_Info
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Info), value, intent(in) :: info
    integer(c_int) :: ierror
end function MPIR_Comm_set_info_c

function MPIR_Group_compare_c(group1,group2,result) &
    bind(C, name="PMPI_Group_compare") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group1, group2
    integer(c_int), intent(out) :: result
    integer(c_int) :: ierror
end function MPIR_Group_compare_c

function MPIR_Group_difference_c(group1,group2,newgroup) &
    bind(C, name="PMPI_Group_difference") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group1, group2
    integer(c_Group), intent(out) :: newgroup
    integer(c_int) :: ierror
end function MPIR_Group_difference_c

function MPIR_Group_excl_c(group, n,ranks, newgroup) &
    bind(C, name="PMPI_Group_excl") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group
    integer(c_int), value, intent(in) :: n
    integer(c_int), intent(in) :: ranks(n)
    integer(c_Group), intent(out) :: newgroup
    integer(c_int) :: ierror
end function MPIR_Group_excl_c

function MPIR_Group_free_c(group) &
    bind(C, name="PMPI_Group_free") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_Group), intent(inout) :: group
    integer(c_int) :: ierror
end function MPIR_Group_free_c

function MPIR_Group_incl_c(group, n,ranks, newgroup) &
    bind(C, name="PMPI_Group_incl") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_int), value, intent(in) :: n
    integer(c_int), intent(in) :: ranks(n)
    integer(c_Group), value, intent(in) :: group
    integer(c_Group), intent(out) :: newgroup
    integer(c_int) :: ierror
end function MPIR_Group_incl_c

function MPIR_Group_intersection_c(group1,group2,newgroup) &
    bind(C, name="PMPI_Group_intersection") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group1, group2
    integer(c_Group), intent(out) :: newgroup
    integer(c_int) :: ierror
end function MPIR_Group_intersection_c

function MPIR_Group_range_excl_c(group, n,ranges, newgroup) &
    bind(C, name="PMPI_Group_range_excl") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group
    integer(c_int), value, intent(in) :: n
    integer(c_int), intent(in) :: ranges(3,n)
    integer(c_Group), intent(out) :: newgroup
    integer(c_int) :: ierror
end function MPIR_Group_range_excl_c

function MPIR_Group_range_incl_c(group, n,ranges, newgroup) &
    bind(C, name="PMPI_Group_range_incl") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group
    integer(c_int), value, intent(in) :: n
    integer(c_int), intent(in) :: ranges(3,n)
    integer(c_Group), intent(out) :: newgroup
    integer(c_int) :: ierror
end function MPIR_Group_range_incl_c

function MPIR_Group_rank_c(group, rank) &
    bind(C, name="PMPI_Group_rank") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group
    integer(c_int), intent(out) :: rank
    integer(c_int) :: ierror
end function MPIR_Group_rank_c

function MPIR_Group_size_c(group, size) &
    bind(C, name="PMPI_Group_size") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group
    integer(c_int), intent(out) :: size
    integer(c_int) :: ierror
end function MPIR_Group_size_c

function MPIR_Group_translate_ranks_c(group1,n, ranks1,group2,ranks2) &
    bind(C, name="PMPI_Group_translate_ranks") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group1, group2
    integer(c_int), value, intent(in) :: n
    integer(c_int), intent(in) :: ranks1(n)
    integer(c_int), intent(out) :: ranks2(n)
    integer(c_int) :: ierror
end function MPIR_Group_translate_ranks_c

function MPIR_Group_union_c(group1,group2,newgroup) &
    bind(C, name="PMPI_Group_union") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group1, group2
    integer(c_Group), intent(out) :: newgroup
    integer(c_int) :: ierror
end function MPIR_Group_union_c

function MPIR_Intercomm_create_c(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm) &
    bind(C, name="PMPI_Intercomm_create") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: local_comm, peer_comm
    integer(c_int), value, intent(in) :: local_leader, remote_leader, tag
    integer(c_Comm), intent(out) :: newintercomm
    integer(c_int) :: ierror
end function MPIR_Intercomm_create_c

function MPIR_Intercomm_merge_c(intercomm, high, newintracomm) &
    bind(C, name="PMPI_Intercomm_merge") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: intercomm
    integer(c_int), intent(in) :: high
    integer(c_Comm), intent(out) :: newintracomm
    integer(c_int) :: ierror
end function MPIR_Intercomm_merge_c

function MPIR_Type_create_keyval_c(type_copy_attr_fn, type_delete_attr_fn, type_keyval, extra_state) &
    bind(C, name="PMPI_Type_create_keyval") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(c_funptr), value :: type_copy_attr_fn
    type(c_funptr), value :: type_delete_attr_fn
    integer(c_int), intent(out) :: type_keyval
    integer(MPI_ADDRESS_KIND), value, intent(in) :: extra_state
    integer(c_int) :: ierror
end function MPIR_Type_create_keyval_c

function MPIR_Type_delete_attr_c(datatype, type_keyval) &
    bind(C, name="PMPI_Type_delete_attr") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), value, intent(in) :: type_keyval
    integer(c_int) :: ierror
end function MPIR_Type_delete_attr_c

function MPIR_Type_free_keyval_c(type_keyval) &
    bind(C, name="PMPI_Type_free_keyval") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), intent(inout) :: type_keyval
    integer(c_int) :: ierror
end function MPIR_Type_free_keyval_c

function MPIR_Type_get_attr_c(datatype, type_keyval, attribute_val, flag, attr_type) &
    bind(C, name="MPIR_TypeGetAttr") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    use :: mpi_c_interface_glue, only : MPIR_ATTR_AINT
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), value, intent(in) :: type_keyval
    integer(MPI_ADDRESS_KIND), intent(out) :: attribute_val
    integer(c_int), intent(out) :: flag
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type
    integer(c_int) :: ierror
end function MPIR_Type_get_attr_c

function MPIR_Type_get_name_c(datatype, type_name, resultlen) &
    bind(C, name="PMPI_Type_get_name") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_MAX_OBJECT_NAME
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    character(kind=c_char), intent(out) :: type_name(MPI_MAX_OBJECT_NAME+1)
    integer(c_int), intent(out) :: resultlen
    integer(c_int) :: ierror
end function MPIR_Type_get_name_c

function MPIR_Type_set_attr_c(datatype, type_keyval, attribute_val, attr_type) &
    bind(C, name="MPIR_TypeSetAttr") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    use :: mpi_c_interface_glue, only : MPIR_ATTR_AINT
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), value, intent(in) :: type_keyval
    integer(MPI_ADDRESS_KIND), value, intent(in) :: attribute_val
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type
    integer(c_int) :: ierror
end function MPIR_Type_set_attr_c

function MPIR_Type_set_name_c(datatype, type_name) &
    bind(C, name="PMPI_Type_set_name") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_Datatype), value, intent(in) :: datatype
    character(kind=c_char), intent(in) :: type_name(*)
    integer(c_int) :: ierror
end function MPIR_Type_set_name_c

function MPIR_Win_create_keyval_c(win_copy_attr_fn, win_delete_attr_fn, win_keyval, extra_state) &
    bind(C, name="PMPI_Win_create_keyval") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(c_funptr), value :: win_copy_attr_fn
    type(c_funptr), value :: win_delete_attr_fn
    integer(c_int), intent(out) :: win_keyval
    integer(MPI_ADDRESS_KIND), value, intent(in) :: extra_state
    integer(c_int) :: ierror
end function MPIR_Win_create_keyval_c

function MPIR_Win_delete_attr_c(win, win_keyval) &
    bind(C, name="PMPI_Win_delete_attr") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_int), value, intent(in) :: win_keyval
    integer(c_int) :: ierror
end function MPIR_Win_delete_attr_c

function MPIR_Win_free_keyval_c(win_keyval) &
    bind(C, name="PMPI_Win_free_keyval") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), intent(inout) :: win_keyval
    integer(c_int) :: ierror
end function MPIR_Win_free_keyval_c

function MPIR_Win_get_attr_c(win, win_keyval, attribute_val, flag, attr_type) &
    bind(C, name="MPIR_WinGetAttr") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Win
    use :: mpi_c_interface_glue, only : MPIR_ATTR_AINT
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_int), value, intent(in) :: win_keyval
    integer(MPI_ADDRESS_KIND), intent(out) :: attribute_val
    integer(c_int), intent(out) :: flag
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type
    integer(c_int) :: ierror
end function MPIR_Win_get_attr_c

function MPIR_Win_get_name_c(win, win_name, resultlen) &
    bind(C, name="PMPI_Win_get_name") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_MAX_OBJECT_NAME
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), value, intent(in) :: win
    character(kind=c_char), intent(out) :: win_name(MPI_MAX_OBJECT_NAME+1)
    integer(c_int), intent(out) :: resultlen
    integer(c_int) :: ierror
end function MPIR_Win_get_name_c

function MPIR_Win_set_attr_c(win, win_keyval, attribute_val, attr_type) &
    bind(C, name="MPIR_WinSetAttr") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Win
    use :: mpi_c_interface_glue, only : MPIR_ATTR_AINT
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_int), value, intent(in) :: win_keyval
    integer(MPI_ADDRESS_KIND), value, intent(in) :: attribute_val
    integer(kind(MPIR_ATTR_AINT)), value, intent(in) :: attr_type
    integer(c_int) :: ierror
end function MPIR_Win_set_attr_c

function MPIR_Win_set_name_c(win, win_name) &
    bind(C, name="PMPI_Win_set_name") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), value, intent(in) :: win
    character(kind=c_char), intent(in) :: win_name(*)
    integer(c_int) :: ierror
end function MPIR_Win_set_name_c

function MPIR_Cartdim_get_c(comm, ndims) &
    bind(C, name="PMPI_Cartdim_get") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(out) :: ndims
    integer(c_int) :: ierror
end function MPIR_Cartdim_get_c

function MPIR_Cart_coords_c(comm, rank, maxdims, coords) &
    bind(C, name="PMPI_Cart_coords") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: rank, maxdims
    integer(c_int), intent(out) :: coords(maxdims)
    integer(c_int) :: ierror
end function MPIR_Cart_coords_c

function MPIR_Cart_create_c(comm_old, ndims, dims, periods, reorder, comm_cart) &
    bind(C, name="PMPI_Cart_create") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm_old
    integer(c_int), value, intent(in) :: ndims
    integer(c_int), intent(in) :: dims(ndims)
    integer(c_int), intent(in) :: periods(ndims), reorder
    integer(c_Comm), intent(out) :: comm_cart
    integer(c_int) :: ierror
end function MPIR_Cart_create_c

function MPIR_Cart_get_c(comm, maxdims, dims, periods, coords) &
    bind(C, name="PMPI_Cart_get") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: maxdims
    integer(c_int), intent(out) :: dims(maxdims), coords(maxdims)
    integer(c_int), intent(out) :: periods(maxdims)
    integer(c_int) :: ierror
end function MPIR_Cart_get_c

function MPIR_Cart_map_c(comm, ndims, dims, periods, newrank) &
    bind(C, name="PMPI_Cart_map") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: ndims
    integer(c_int), intent(in) :: dims(ndims)
    integer(c_int), intent(in) :: periods(ndims)
    integer(c_int), intent(out) :: newrank
    integer(c_int) :: ierror
end function MPIR_Cart_map_c

function MPIR_Cart_rank_c(comm, coords, rank) &
    bind(C, name="PMPI_Cart_rank") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(in) :: coords(*)
    integer(c_int), intent(out) :: rank
    integer(c_int) :: ierror
end function MPIR_Cart_rank_c

function MPIR_Cart_shift_c(comm, direction, disp, rank_source, rank_dest) &
    bind(C, name="PMPI_Cart_shift") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: direction, disp
    integer(c_int), intent(out) :: rank_source, rank_dest
    integer(c_int) :: ierror
end function MPIR_Cart_shift_c

function MPIR_Cart_sub_c(comm, remain_dims, newcomm) &
    bind(C, name="PMPI_Cart_sub") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(in) :: remain_dims(*)
    integer(c_Comm), intent(out) :: newcomm
    integer(c_int) :: ierror
end function MPIR_Cart_sub_c

function MPIR_Dims_create_c(nnodes, ndims, dims) &
    bind(C, name="PMPI_Dims_create") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), value, intent(in) :: nnodes, ndims
    integer(c_int), intent(inout) :: dims(ndims)
    integer(c_int) :: ierror
end function MPIR_Dims_create_c

function MPIR_Dist_graph_create_c(comm_old, n,sources, degrees, destinations, weights, &
           info, reorder, comm_dist_graph) &
    bind(C, name="PMPI_Dist_graph_create") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Comm, c_Info
    implicit none
    integer(c_Comm), value, intent(in) :: comm_old
    integer(c_int), value, intent(in) :: n
    integer(c_int), intent(in) :: sources(n), degrees(n), destinations(*)
    type(c_ptr), value, intent(in) :: weights
    integer(c_Info), value, intent(in) :: info
    integer(c_int), intent(in) :: reorder
    integer(c_Comm), intent(out) :: comm_dist_graph
    integer(c_int) :: ierror
end function MPIR_Dist_graph_create_c

function MPIR_Dist_graph_create_adjacent_c(comm_old, indegree, sources, sourceweights, &
           outdegree, destinations, destweights, info, reorder, comm_dist_graph) &
    bind(C, name="PMPI_Dist_graph_create_adjacent") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Comm, c_Info
    implicit none
    integer(c_Comm), value, intent(in) :: comm_old
    integer(c_int), value, intent(in) :: indegree, outdegree
    integer(c_int), intent(in) :: sources(indegree), destinations(outdegree)
    type(c_ptr), value, intent(in) :: sourceweights
    type(c_ptr), value, intent(in) :: destweights
    integer(c_Info), value, intent(in) :: info
    integer(c_int), intent(in) :: reorder
    integer(c_Comm), intent(out) :: comm_dist_graph
    integer(c_int) :: ierror
end function MPIR_Dist_graph_create_adjacent_c

function MPIR_Dist_graph_neighbors_c(comm, maxindegree, sources, sourceweights, &
           maxoutdegree, destinations, destweights) &
    bind(C, name="PMPI_Dist_graph_neighbors") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: maxindegree, maxoutdegree
    integer(c_int), intent(out) :: sources(maxindegree), destinations(maxoutdegree)
    integer(c_int), intent(out) :: sourceweights(maxindegree), destweights(maxoutdegree)
    integer(c_int) :: ierror
end function MPIR_Dist_graph_neighbors_c

function MPIR_Dist_graph_neighbors_count_c(comm, indegree, outdegree, weighted) &
    bind(C, name="PMPI_Dist_graph_neighbors_count") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(out) :: indegree, outdegree
    integer(c_int), intent(out) :: weighted
    integer(c_int) :: ierror
end function MPIR_Dist_graph_neighbors_count_c

function MPIR_Graphdims_get_c(comm, nnodes, nedges) &
    bind(C, name="PMPI_Graphdims_get") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(out) :: nnodes, nedges
    integer(c_int) :: ierror
end function MPIR_Graphdims_get_c

function MPIR_Graph_create_c(comm_old, nnodes, index, edges, reorder, comm_graph) &
    bind(C, name="PMPI_Graph_create") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm_old
    integer(c_int), value, intent(in) :: nnodes
    integer(c_int), intent(in) :: index(nnodes), edges(*)
    integer(c_int), intent(in) :: reorder
    integer(c_Comm), intent(out) :: comm_graph
    integer(c_int) :: ierror
end function MPIR_Graph_create_c

function MPIR_Graph_get_c(comm, maxindex, maxedges, index, edges) &
    bind(C, name="PMPI_Graph_get") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: maxindex, maxedges
    integer(c_int), intent(out) :: index(maxindex), edges(maxedges)
    integer(c_int) :: ierror
end function MPIR_Graph_get_c

function MPIR_Graph_map_c(comm, nnodes, index, edges, newrank) &
    bind(C, name="PMPI_Graph_map") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: nnodes
    integer(c_int), intent(in) :: index(nnodes), edges(*)
    integer(c_int), intent(out) :: newrank
    integer(c_int) :: ierror
end function MPIR_Graph_map_c

function MPIR_Graph_neighbors_c(comm, rank, maxneighbors, neighbors) &
    bind(C, name="PMPI_Graph_neighbors") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: rank, maxneighbors
    integer(c_int), intent(out) :: neighbors(maxneighbors)
    integer(c_int) :: ierror
end function MPIR_Graph_neighbors_c

function MPIR_Graph_neighbors_count_c(comm, rank, nneighbors) &
    bind(C, name="PMPI_Graph_neighbors_count") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: rank
    integer(c_int), intent(out) :: nneighbors
    integer(c_int) :: ierror
end function MPIR_Graph_neighbors_count_c

function MPIR_Topo_test_c(comm, status) &
    bind(C, name="PMPI_Topo_test") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(out) :: status
    integer(c_int) :: ierror
end function MPIR_Topo_test_c

function  MPIR_Wtick_c() &
    bind(C, name="PMPI_Wtick") result(res)
    use, intrinsic :: iso_c_binding, only : c_double
    implicit none
    real(c_double) :: res
end function MPIR_Wtick_c

function  MPIR_Wtime_c() &
    bind(C, name="PMPI_Wtime") result(res)
    use, intrinsic :: iso_c_binding, only : c_double
    implicit none
    real(c_double) :: res
end function MPIR_Wtime_c

function MPIR_Abort_c(comm, errorcode) &
    bind(C, name="PMPI_Abort") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: errorcode
    integer(c_int) :: ierror
end function MPIR_Abort_c

function MPIR_Add_error_class_c(errorclass) &
    bind(C, name="PMPI_Add_error_class") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), intent(out) :: errorclass
    integer(c_int) :: ierror
end function MPIR_Add_error_class_c

function MPIR_Add_error_code_c(errorclass, errorcode) &
    bind(C, name="PMPI_Add_error_code") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    integer(c_int), value, intent(in) :: errorclass
    integer(c_int), intent(out) :: errorcode
    integer(c_int) :: ierror
end function MPIR_Add_error_code_c

function MPIR_Add_error_string_c(errorcode, string) &
    bind(C, name="PMPI_Add_error_string") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    integer(c_int), value, intent(in) :: errorcode
    character(kind=c_char), intent(in) :: string(*)
    integer(c_int) :: ierror
end function MPIR_Add_error_string_c

function MPIR_Alloc_mem_c(size, info, baseptr) &
    bind(C, name="PMPI_Alloc_mem") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_ptr, c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(MPI_ADDRESS_KIND), value, intent(in) :: size
    integer(c_Info), value, intent(in) :: info
    type(c_ptr), intent(out) :: baseptr
    integer(c_int) :: ierror
end function MPIR_Alloc_mem_c

function MPIR_Comm_call_errhandler_c(comm, errorcode) &
    bind(C, name="PMPI_Comm_call_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: errorcode
    integer(c_int) :: ierror
end function MPIR_Comm_call_errhandler_c

function MPIR_Comm_create_errhandler_c(comm_errhandler_fn, errhandler) &
    bind(C, name="PMPI_Comm_create_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_c_interface_types, only : c_Errhandler
    implicit none
    type(c_funptr), value :: comm_errhandler_fn
    integer(c_Errhandler), intent(out) :: errhandler
    integer(c_int) :: ierror
end function MPIR_Comm_create_errhandler_c

function MPIR_Comm_get_errhandler_c(comm, errhandler) &
    bind(C, name="PMPI_Comm_get_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm, c_Errhandler
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Errhandler), intent(out) :: errhandler
    integer(c_int) :: ierror
end function MPIR_Comm_get_errhandler_c

function MPIR_Comm_set_errhandler_c(comm, errhandler) &
    bind(C, name="PMPI_Comm_set_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm, c_Errhandler
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Errhandler), value, intent(in) :: errhandler
    integer(c_int) :: ierror
end function MPIR_Comm_set_errhandler_c

function MPIR_Errhandler_free_c(errhandler) &
    bind(C, name="PMPI_Errhandler_free") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Errhandler
    implicit none
    integer(c_Errhandler), intent(inout) :: errhandler
    integer(c_int) :: ierror
end function MPIR_Errhandler_free_c

function MPIR_Error_class_c(errorcode, errorclass) &
    bind(C, name="PMPI_Error_class") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), value, intent(in) :: errorcode
    integer(c_int), intent(out) :: errorclass
    integer(c_int) :: ierror
end function MPIR_Error_class_c

function MPIR_Error_string_c(errorcode, string, resultlen) &
    bind(C, name="PMPI_Error_string") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_MAX_ERROR_STRING
    implicit none
    integer(c_int), value, intent(in) :: errorcode
    character(kind=c_char), intent(out) :: string(MPI_MAX_ERROR_STRING+1)
    integer(c_int), intent(out) :: resultlen
    integer(c_int) :: ierror
end function MPIR_Error_string_c

function MPIR_File_call_errhandler_c(fh, errorcode) &
    bind(C, name="PMPI_File_call_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(c_int), value, intent(in) :: errorcode
    integer(c_int) :: ierror
end function MPIR_File_call_errhandler_c

function MPIR_File_create_errhandler_c(file_errhandler_fn, errhandler) &
    bind(C, name="PMPI_File_create_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_c_interface_types, only : c_Errhandler
    implicit none
    type(c_funptr), value :: file_errhandler_fn
    integer(c_Errhandler), intent(out) :: errhandler
    integer(c_int) :: ierror
end function MPIR_File_create_errhandler_c

function MPIR_File_get_errhandler_c(file, errhandler) &
    bind(C, name="PMPI_File_get_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Errhandler
    implicit none
    integer(c_File), value, intent(in) :: file
    integer(c_Errhandler), intent(out) :: errhandler
    integer(c_int) :: ierror
end function MPIR_File_get_errhandler_c

function MPIR_File_set_errhandler_c(file, errhandler) &
    bind(C, name="PMPI_File_set_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Errhandler
    implicit none
    integer(c_File), value, intent(in) :: file
    integer(c_Errhandler), value, intent(in) :: errhandler
    integer(c_int) :: ierror
end function MPIR_File_set_errhandler_c

function MPIR_Finalize_c() &
    bind(C, name="PMPI_Finalize") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int) :: ierror
end function MPIR_Finalize_c

function MPIR_Finalized_c(flag) &
    bind(C, name="PMPI_Finalized") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), intent(out) :: flag
    integer(c_int) :: ierror
end function MPIR_Finalized_c

function MPIR_Get_processor_name_c(name, resultlen) &
    bind(C, name="PMPI_Get_processor_name") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_MAX_PROCESSOR_NAME
    implicit none
    character(kind=c_char), intent(out) :: name(MPI_MAX_PROCESSOR_NAME+1)
    integer(c_int), intent(out) :: resultlen
    integer(c_int) :: ierror
end function MPIR_Get_processor_name_c

function MPIR_Get_version_c(version, subversion) &
    bind(C, name="PMPI_Get_version") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), intent(out) :: version, subversion
    integer(c_int) :: ierror
end function MPIR_Get_version_c

function MPIR_Init_c(argc, argv) &
    bind(C, name="PMPI_Init") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    implicit none
    type(c_ptr), value, intent(in) :: argc
    type(c_ptr), value, intent(in) :: argv
    integer(c_int) :: ierror
end function MPIR_Init_c

function MPIR_Initialized_c(flag) &
    bind(C, name="PMPI_Initialized") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), intent(out) :: flag
    integer(c_int) :: ierror
end function MPIR_Initialized_c

function MPIR_Win_call_errhandler_c(win, errorcode) &
    bind(C, name="PMPI_Win_call_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_int), value, intent(in) :: errorcode
    integer(c_int) :: ierror
end function MPIR_Win_call_errhandler_c

function MPIR_Win_create_errhandler_c(win_errhandler_fn, errhandler) &
    bind(C, name="PMPI_Win_create_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_c_interface_types, only : c_Errhandler
    implicit none
    type(c_funptr), value :: win_errhandler_fn
    integer(c_Errhandler), intent(out) :: errhandler
    integer(c_int) :: ierror
end function MPIR_Win_create_errhandler_c

function MPIR_Win_get_errhandler_c(win, errhandler) &
    bind(C, name="PMPI_Win_get_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win, c_Errhandler
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_Errhandler), intent(out) :: errhandler
    integer(c_int) :: ierror
end function MPIR_Win_get_errhandler_c

function MPIR_Win_set_errhandler_c(win, errhandler) &
    bind(C, name="PMPI_Win_set_errhandler") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win, c_Errhandler
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_Errhandler), value, intent(in) :: errhandler
    integer(c_int) :: ierror
end function MPIR_Win_set_errhandler_c

function MPIR_Info_create_c(info) &
    bind(C, name="PMPI_Info_create") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(c_Info), intent(out) :: info
    integer(c_int) :: ierror
end function MPIR_Info_create_c

function MPIR_Info_delete_c(info, key) &
    bind(C, name="PMPI_Info_delete") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(c_Info), value, intent(in) :: info
    character(kind=c_char), intent(in) :: key(*)
    integer(c_int) :: ierror
end function MPIR_Info_delete_c

function MPIR_Info_dup_c(info, newinfo) &
    bind(C, name="PMPI_Info_dup") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(c_Info), value, intent(in) :: info
    integer(c_Info), intent(out) :: newinfo
    integer(c_int) :: ierror
end function MPIR_Info_dup_c

function MPIR_Info_free_c(info) &
    bind(C, name="PMPI_Info_free") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(c_Info), intent(inout) :: info
    integer(c_int) :: ierror
end function MPIR_Info_free_c

function MPIR_Info_get_c(info, key, valuelen, value, flag) &
    bind(C, name="PMPI_Info_get") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(c_Info), value, intent(in) :: info
    character(kind=c_char), intent(in) :: key(*)
    integer(c_int), value, intent(in) :: valuelen
    character(kind=c_char), intent(out) :: value(valuelen+1)
    integer(c_int), intent(out) :: flag
    integer(c_int) :: ierror
end function MPIR_Info_get_c

function MPIR_Info_get_nkeys_c(info, nkeys) &
    bind(C, name="PMPI_Info_get_nkeys") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(c_Info), value, intent(in) :: info
    integer(c_int), intent(out) :: nkeys
    integer(c_int) :: ierror
end function MPIR_Info_get_nkeys_c

function MPIR_Info_get_nthkey_c(info, n,key) &
    bind(C, name="PMPI_Info_get_nthkey") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(c_Info), value, intent(in) :: info
    integer(c_int), value, intent(in) :: n
    character(kind=c_char), intent(out) :: key(*)
    integer(c_int) :: ierror
end function MPIR_Info_get_nthkey_c

function MPIR_Info_get_valuelen_c(info, key, valuelen, flag) &
    bind(C, name="PMPI_Info_get_valuelen") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(c_Info), value, intent(in) :: info
    character(kind=c_char), intent(in) :: key(*)
    integer(c_int), intent(out) :: valuelen
    integer(c_int), intent(out) :: flag
    integer(c_int) :: ierror
end function MPIR_Info_get_valuelen_c

function MPIR_Info_set_c(info, key, value) &
    bind(C, name="PMPI_Info_set") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(c_Info), value, intent(in) :: info
    character(kind=c_char), intent(in) :: key(*), value(*)
    integer(c_int) :: ierror
end function MPIR_Info_set_c

function MPIR_Close_port_c(port_name) &
    bind(C, name="PMPI_Close_port") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    implicit none
    character(kind=c_char), intent(in) :: port_name(*)
    integer(c_int) :: ierror
end function MPIR_Close_port_c

function MPIR_Comm_accept_c(port_name, info, root, comm, newcomm) &
    bind(C, name="PMPI_Comm_accept") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Info, c_Comm
    implicit none
    character(kind=c_char), intent(in) :: port_name(*)
    integer(c_Info), value, intent(in) :: info
    integer(c_int), value, intent(in) :: root
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Comm), intent(out) :: newcomm
    integer(c_int) :: ierror
end function MPIR_Comm_accept_c

function MPIR_Comm_connect_c(port_name, info, root, comm, newcomm) &
    bind(C, name="PMPI_Comm_connect") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Info, c_Comm
    implicit none
    character(kind=c_char), intent(in) :: port_name(*)
    integer(c_Info), value, intent(in) :: info
    integer(c_int), value, intent(in) :: root
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Comm), intent(out) :: newcomm
    integer(c_int) :: ierror
end function MPIR_Comm_connect_c

function MPIR_Comm_disconnect_c(comm) &
    bind(C, name="PMPI_Comm_disconnect") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), intent(inout) :: comm
    integer(c_int) :: ierror
end function MPIR_Comm_disconnect_c

function MPIR_Comm_get_parent_c(parent) &
    bind(C, name="PMPI_Comm_get_parent") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_Comm), intent(out) :: parent
    integer(c_int) :: ierror
end function MPIR_Comm_get_parent_c

function MPIR_Comm_join_c(fd, intercomm) &
    bind(C, name="PMPI_Comm_join") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_int), value, intent(in) :: fd
    integer(c_Comm), intent(out) :: intercomm
    integer(c_int) :: ierror
end function MPIR_Comm_join_c

function MPIR_Lookup_name_c(service_name, info, port_name) &
    bind(C, name="PMPI_Lookup_name") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_MAX_PORT_NAME
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    character(kind=c_char), intent(in) :: service_name(*)
    integer(c_Info), value, intent(in) :: info
    character(kind=c_char), intent(out) :: port_name(MPI_MAX_PORT_NAME+1)
    integer(c_int) :: ierror
end function MPIR_Lookup_name_c

function MPIR_Open_port_c(info, port_name) &
    bind(C, name="PMPI_Open_port") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_MAX_PORT_NAME
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(c_Info), value, intent(in) :: info
    character(kind=c_char), intent(out) :: port_name(MPI_MAX_PORT_NAME+1)
    integer(c_int) :: ierror
end function MPIR_Open_port_c

function MPIR_Publish_name_c(service_name, info, port_name) &
    bind(C, name="PMPI_Publish_name") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    integer(c_Info), value, intent(in) :: info
    character(kind=c_char), intent(in) :: service_name(*), port_name(*)
    integer(c_int) :: ierror
end function MPIR_Publish_name_c

function MPIR_Unpublish_name_c(service_name, info, port_name) &
    bind(C, name="PMPI_Unpublish_name") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    character(kind=c_char), intent(in) :: service_name(*), port_name(*)
    integer(c_Info), value, intent(in) :: info
    integer(c_int) :: ierror
end function MPIR_Unpublish_name_c

function MPIR_Win_allocate_c(size, disp_unit, info, comm, baseptr, win) &
    bind(C, name="PMPI_Win_allocate") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Info, c_Comm, c_Win
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    integer(kind=MPI_ADDRESS_KIND), value, intent(in) :: size
    integer(c_int), value, intent(in) :: disp_unit
    integer(c_Info), value, intent(in) :: info
    integer(c_Comm), value, intent(in) :: comm
    type(c_ptr), intent(out) :: baseptr
    integer(c_Win), intent(out) :: win
    integer(c_int) :: ierror
end function MPIR_Win_allocate_c

function MPIR_Win_allocate_shared_c(size, disp_unit, info, comm, baseptr, win) &
    bind(C, name="PMPI_Win_allocate_shared") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Info, c_Comm, c_Win
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    integer(kind=MPI_ADDRESS_KIND), value, intent(in) :: size
    integer(c_int), value, intent(in) :: disp_unit
    integer(c_Info), value, intent(in) :: info
    integer(c_Comm), value, intent(in) :: comm
    type(c_ptr), intent(out) :: baseptr
    integer(c_Win), intent(out) :: win
    integer(c_int) :: ierror
end function MPIR_Win_allocate_shared_c

function MPIR_Win_complete_c(win) &
    bind(C, name="PMPI_Win_complete") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_complete_c

function MPIR_Win_create_dynamic_c(info, comm, win) &
    bind(C, name="PMPI_Win_create_dynamic") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Info, c_Comm, c_Win
    implicit none
    integer(c_Info), value, intent(in) :: info
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Win), intent(out) :: win
    integer(c_int) :: ierror
end function MPIR_Win_create_dynamic_c

function MPIR_Win_fence_c(assert, win) &
    bind(C, name="PMPI_Win_fence") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_int), value, intent(in) :: assert
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_fence_c

function MPIR_Win_flush_c(rank, win) &
    bind(C, name="PMPI_Win_flush") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_int), value, intent(in) :: rank
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_flush_c

function MPIR_Win_flush_all_c(win) &
    bind(C, name="PMPI_Win_flush_all") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_flush_all_c

function MPIR_Win_flush_local_c(rank, win) &
    bind(C, name="PMPI_Win_flush_local") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_int), value, intent(in) :: rank
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_flush_local_c

function MPIR_Win_flush_local_all_c(win) &
    bind(C, name="PMPI_Win_flush_local_all") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_flush_local_all_c

function MPIR_Win_free_c(win) &
    bind(C, name="PMPI_Win_free") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), intent(inout) :: win
    integer(c_int) :: ierror
end function MPIR_Win_free_c

function MPIR_Win_get_group_c(win, group) &
    bind(C, name="PMPI_Win_get_group") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win, c_Group
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_Group), intent(out) :: group
    integer(c_int) :: ierror
end function MPIR_Win_get_group_c

function MPIR_Win_get_info_c(win, info_used) &
    bind(C, name="PMPI_Win_get_info") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win, c_Info
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_Info), intent(out) :: info_used
    integer(c_int) :: ierror
end function MPIR_Win_get_info_c

function MPIR_Win_lock_c(lock_type, rank, assert, win) &
    bind(C, name="PMPI_Win_lock") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_int), value, intent(in) :: lock_type, rank, assert
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_lock_c

function MPIR_Win_lock_all_c(assert, win) &
    bind(C, name="PMPI_Win_lock_all") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_int), value, intent(in) :: assert
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_lock_all_c

function MPIR_Win_post_c(group, assert, win) &
    bind(C, name="PMPI_Win_post") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win, c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group
    integer(c_int), value, intent(in) :: assert
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_post_c

function MPIR_Win_set_info_c(win, info) &
    bind(C, name="PMPI_Win_set_info") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win, c_Info
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_Info), value, intent(in) :: info
    integer(c_int) :: ierror
end function MPIR_Win_set_info_c

function MPIR_Win_shared_query_c(win, rank, size, disp_unit, baseptr) &
    bind(C, name="PMPI_Win_shared_query") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Win
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_int), value, intent(in) :: rank
    integer(kind=MPI_ADDRESS_KIND), intent(out) :: size
    integer(c_int), intent(out) :: disp_unit
    type(c_ptr), intent(out) :: baseptr
    integer(c_int) :: ierror
end function MPIR_Win_shared_query_c

function MPIR_Win_start_c(group, assert, win) &
    bind(C, name="PMPI_Win_start") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win, c_Group
    implicit none
    integer(c_Group), value, intent(in) :: group
    integer(c_int), value, intent(in) :: assert
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_start_c

function MPIR_Win_sync_c(win) &
    bind(C, name="PMPI_Win_sync") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_sync_c

function MPIR_Win_test_c(win, flag) &
    bind(C, name="PMPI_Win_test") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_int), intent(out) :: flag
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_test_c

function MPIR_Win_unlock_c(rank, win) &
    bind(C, name="PMPI_Win_unlock") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_int), value, intent(in) :: rank
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_unlock_c

function MPIR_Win_unlock_all_c(win) &
    bind(C, name="PMPI_Win_unlock_all") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_unlock_all_c

function MPIR_Win_wait_c(win) &
    bind(C, name="PMPI_Win_wait") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Win_wait_c

function MPIR_Grequest_complete_c(request) &
    bind(C, name="PMPI_Grequest_complete") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_Request), value, intent(in) :: request
    integer(c_int) :: ierror
end function MPIR_Grequest_complete_c

function MPIR_Grequest_start_c(query_fn, free_fn, cancel_fn, extra_state, request) &
    bind(C, name="PMPI_Grequest_start") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    type(c_funptr), value :: query_fn
    type(c_funptr), value :: free_fn
    type(c_funptr), value :: cancel_fn
    integer(MPI_ADDRESS_KIND), intent(in) :: extra_state
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Grequest_start_c

function MPIR_Init_thread_c(argc, argv, required, provided) &
    bind(C, name="PMPI_Init_thread") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    implicit none
    type(c_ptr), value, intent(in) :: argc
    type(c_ptr), value, intent(in) :: argv
    integer(c_int), value, intent(in) :: required
    integer(c_int), intent(out) :: provided
    integer(c_int) :: ierror
end function MPIR_Init_thread_c

function MPIR_Is_thread_main_c(flag) &
    bind(C, name="PMPI_Is_thread_main") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), intent(out) :: flag
    integer(c_int) :: ierror
end function MPIR_Is_thread_main_c

function MPIR_Query_thread_c(provided) &
    bind(C, name="PMPI_Query_thread") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), intent(out) :: provided
    integer(c_int) :: ierror
end function MPIR_Query_thread_c

function MPIR_Status_set_cancelled_c(status, flag) &
    bind(C, name="PMPI_Status_set_cancelled") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    implicit none
    type(c_ptr), value, intent(in) :: status
    integer(c_int), intent(out) :: flag
    integer(c_int) :: ierror
end function MPIR_Status_set_cancelled_c

function MPIR_Status_set_elements_c(status, datatype, count) &
    bind(C, name="PMPI_Status_set_elements") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    type(c_ptr), value, intent(in) :: status
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), value, intent(in) :: count
    integer(c_int) :: ierror
end function MPIR_Status_set_elements_c

function MPIR_Status_set_elements_x_c(status, datatype, count) &
    bind(C, name="PMPI_Status_set_elements_x") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08_compile_constants, only : MPI_COUNT_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    type(c_ptr), value, intent(in) :: status
    integer(c_Datatype), value, intent(in) :: datatype
    integer(MPI_COUNT_KIND), value, intent(in) :: count
    integer(c_int) :: ierror
end function MPIR_Status_set_elements_x_c

function MPIR_File_close_c(fh) &
    bind(C, name="PMPI_File_close") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), intent(inout) :: fh
    integer(c_int) :: ierror
end function MPIR_File_close_c

function MPIR_File_delete_c(filename, info) &
    bind(C, name="PMPI_File_delete") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Info
    implicit none
    character(kind=c_char), intent(in) :: filename(*)
    integer(c_Info), value, intent(in) :: info
    integer(c_int) :: ierror
end function MPIR_File_delete_c

function MPIR_File_get_amode_c(fh, amode) &
    bind(C, name="PMPI_File_get_amode") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(c_int), intent(out) :: amode
    integer(c_int) :: ierror
end function MPIR_File_get_amode_c

function MPIR_File_get_atomicity_c(fh, flag) &
    bind(C, name="PMPI_File_get_atomicity") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(c_int), intent(out) :: flag
    integer(c_int) :: ierror
end function MPIR_File_get_atomicity_c

function MPIR_File_get_byte_offset_c(fh, offset, disp) &
    bind(C, name="PMPI_File_get_byte_offset") result(ierror)
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    integer(MPI_OFFSET_KIND), intent(out) :: disp
    integer(c_int) :: ierror
end function MPIR_File_get_byte_offset_c

function MPIR_File_get_group_c(fh, group) &
    bind(C, name="PMPI_File_get_group") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Group
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(c_Group), intent(out) :: group
    integer(c_int) :: ierror
end function MPIR_File_get_group_c

function MPIR_File_get_info_c(fh, info_used) &
    bind(C, name="PMPI_File_get_info") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Info
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(c_Info), intent(out) :: info_used
    integer(c_int) :: ierror
end function MPIR_File_get_info_c

function MPIR_File_get_position_c(fh, offset) &
    bind(C, name="PMPI_File_get_position") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), intent(out) :: offset
    integer(c_int) :: ierror
end function MPIR_File_get_position_c

function MPIR_File_get_position_shared_c(fh, offset) &
    bind(C, name="PMPI_File_get_position_shared") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), intent(out) :: offset
    integer(c_int) :: ierror
end function MPIR_File_get_position_shared_c

function MPIR_File_get_size_c(fh, size) &
    bind(C, name="PMPI_File_get_size") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), intent(out) :: size
    integer(c_int) :: ierror
end function MPIR_File_get_size_c

function MPIR_File_get_type_extent_c(fh, datatype, extent) &
    bind(C, name="PMPI_File_get_type_extent") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(c_Datatype), value, intent(in) :: datatype
    integer(MPI_ADDRESS_KIND), intent(out) :: extent
    integer(c_int) :: ierror
end function MPIR_File_get_type_extent_c

function MPIR_File_get_view_c(fh, disp, etype, filetype, datarep) &
    bind(C, name="PMPI_File_get_view") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), intent(out) :: disp
    integer(c_Datatype), intent(out) :: etype
    integer(c_Datatype), intent(out) :: filetype
    character(kind=c_char), intent(out) :: datarep(*)
    integer(c_int) :: ierror
end function MPIR_File_get_view_c

function MPIR_File_open_c(comm, filename, amode, info, fh) &
    bind(C, name="PMPI_File_open") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_c_interface_types, only : c_Comm, c_Info, c_File
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    character(kind=c_char), intent(in) :: filename(*)
    integer(c_int), value, intent(in) :: amode
    integer(c_Info), value, intent(in) :: info
    integer(c_File), intent(out) :: fh
    integer(c_int) :: ierror
end function MPIR_File_open_c

function MPIR_File_preallocate_c(fh, size) &
    bind(C, name="PMPI_File_preallocate") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: size
    integer(c_int) :: ierror
end function MPIR_File_preallocate_c

function MPIR_File_seek_c(fh, offset, whence) &
    bind(C, name="PMPI_File_seek") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    integer(c_int), value, intent(in) :: whence
    integer(c_int) :: ierror
end function MPIR_File_seek_c

function MPIR_File_seek_shared_c(fh, offset, whence) &
    bind(C, name="PMPI_File_seek_shared") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    integer(c_int), value, intent(in) :: whence
    integer(c_int) :: ierror
end function MPIR_File_seek_shared_c

function MPIR_File_set_atomicity_c(fh, flag) &
    bind(C, name="PMPI_File_set_atomicity") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(c_int), intent(in) :: flag
    integer(c_int) :: ierror
end function MPIR_File_set_atomicity_c

function MPIR_File_set_info_c(fh, info) &
    bind(C, name="PMPI_File_set_info") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Info
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(c_Info), value, intent(in) :: info
    integer(c_int) :: ierror
end function MPIR_File_set_info_c

function MPIR_File_set_size_c(fh, size) &
    bind(C, name="PMPI_File_set_size") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: size
    integer(c_int) :: ierror
end function MPIR_File_set_size_c

function MPIR_File_set_view_c(fh, disp, etype, filetype, datarep, info) &
    bind(C, name="PMPI_File_set_view") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype, c_Info
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: disp
    integer(c_Datatype), value, intent(in) :: etype
    integer(c_Datatype), value, intent(in) :: filetype
    character(kind=c_char), intent(in) :: datarep(*)
    integer(c_Info), value, intent(in) :: info
    integer(c_int) :: ierror
end function MPIR_File_set_view_c

function MPIR_File_sync_c(fh) &
    bind(C, name="PMPI_File_sync") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(c_int) :: ierror
end function MPIR_File_sync_c

function MPIR_Register_datarep_c(datarep, read_conversion_fn, write_conversion_fn, dtype_file_extent_fn, extra_state) &
    bind(C, name="PMPI_Register_datarep") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char, c_funptr
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    character(kind=c_char), intent(in) :: datarep(*)
    type(c_funptr), value :: read_conversion_fn
    type(c_funptr), value :: write_conversion_fn
    type(c_funptr), value :: dtype_file_extent_fn
    integer(MPI_ADDRESS_KIND), intent(in) :: extra_state ! Why no value
    integer(c_int) :: ierror
end function MPIR_Register_datarep_c

function MPIR_Type_create_f90_complex_c(p, r, newtype) &
    bind(C, name="PMPI_Type_create_f90_complex") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: p, r
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_create_f90_complex_c

function MPIR_Type_create_f90_integer_c(r, newtype) &
    bind(C, name="PMPI_Type_create_f90_integer") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: r
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_create_f90_integer_c

function MPIR_Type_create_f90_real_c(p, r, newtype) &
    bind(C, name="PMPI_Type_create_f90_real") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: p, r
    integer(c_Datatype), intent(out) :: newtype
    integer(c_int) :: ierror
end function MPIR_Type_create_f90_real_c

function MPIR_Type_match_size_c(typeclass, size, datatype) &
    bind(C, name="PMPI_Type_match_size") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    integer(c_int), value, intent(in) :: typeclass, size
    integer(c_Datatype), intent(out) :: datatype
    integer(c_int) :: ierror
end function MPIR_Type_match_size_c

function MPIR_Pcontrol_c(level) &
    bind(C, name="PMPI_Pcontrol") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    integer(c_int), value, intent(in) :: level
    integer(c_int) :: ierror
end function MPIR_Pcontrol_c

function MPIR_Comm_split_type_c(comm, split_type, key, info, newcomm) &
    bind(C, name="PMPI_Comm_split_type") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Comm, c_Info
    implicit none
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), value, intent(in) :: split_type
    integer(c_int), value, intent(in) :: key
    integer(c_Info), value, intent(in) :: info
    integer(c_Comm), intent(out) :: newcomm
    integer(c_int) :: ierror
end function MPIR_Comm_split_type_c

function MPIR_Get_library_version_c(version, resultlen) &
    bind(C, name="PMPI_Get_library_version") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_MAX_LIBRARY_VERSION_STRING
    implicit none
    character(kind=c_char), intent(out) :: version(MPI_MAX_LIBRARY_VERSION_STRING+1)
    integer(c_int), intent(out) :: resultlen
    integer(c_int) :: ierror
end function MPIR_Get_library_version_c

function MPIR_Mprobe_c(source, tag, comm, message, status) &
    bind(C, name="PMPI_Mprobe") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Comm, c_Message
    implicit none
    integer(c_int), value, intent(in) :: source, tag
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Message), intent(out) :: message
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Mprobe_c

function MPIR_Improbe_c(source, tag, comm, flag, message, status) &
    bind(C, name="PMPI_Improbe") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Comm, c_Message
    implicit none
    integer(c_int), value, intent(in) :: source, tag
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(out) :: flag
    integer(c_Message), intent(out) :: message
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Improbe_c

function MPIR_Iprobe_c(source, tag, comm, flag, status) &
    bind(C, name="PMPI_Iprobe") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_int), value, intent(in) :: source, tag
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int), intent(out) :: flag
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Iprobe_c

function MPIR_Probe_c(source, tag, comm, status) &
    bind(C, name="PMPI_Probe") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Comm
    implicit none
    integer(c_int), value, intent(in) :: source, tag
    integer(c_Comm), value, intent(in) :: comm
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Probe_c

function MPIR_Request_free_c(request) &
    bind(C, name="PMPI_Request_free") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_Request), intent(inout) :: request
    integer(c_int) :: ierror
end function MPIR_Request_free_c

function MPIR_Request_get_status_c(request, flag, status) &
    bind(C, name="PMPI_Request_get_status") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Request
    implicit none
    integer(c_Request), intent(in) :: request
    integer(c_int), intent(out) :: flag
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Request_get_status_c

function MPIR_Comm_spawn_c(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes, argv_elem_len) &
    bind(C, name="MPIR_Comm_spawn_c") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char, c_ptr
    use :: mpi_c_interface_types, only : c_Info, c_Comm
    implicit none
    character(kind=c_char), dimension(*), intent(in) :: command
    type(c_ptr), value, intent(in) :: argv
    integer(c_int), value, intent(in) :: maxprocs, root
    integer(c_Info), value, intent(in) :: info
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Comm), intent(out) :: intercomm
    type(c_ptr), value, intent(in) :: array_of_errcodes
    integer(c_int), value, intent(in) :: argv_elem_len
    integer(c_int) :: ierror
end function MPIR_Comm_spawn_c

function MPIR_Comm_spawn_multiple_c(count, array_of_commands, array_of_argv, array_of_maxprocs, &
           array_of_info, root, comm, intercomm, array_of_errcodes, commands_elem_len, argv_elem_len) &
    bind(C, name="MPIR_Comm_spawn_multiple_c") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char, c_ptr
    use :: mpi_c_interface_types, only : c_Info, c_Comm
    implicit none
    integer(c_int), value, intent(in) :: count
    type(c_ptr), value, intent(in) :: array_of_commands
    type(c_ptr), value, intent(in) :: array_of_argv
    integer(c_int), intent(in) ::  array_of_maxprocs(*)
    integer(c_Info), intent(in) :: array_of_info(*)
    integer(c_int), value, intent(in) :: root
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Comm), intent(out) :: intercomm
    type(c_ptr), value, intent(in) :: array_of_errcodes
    integer(c_int), value, intent(in) :: commands_elem_len, argv_elem_len
    integer(c_int) :: ierror
end function MPIR_Comm_spawn_multiple_c

function  MPIR_Aint_add_c(base, disp) &
    bind(C, name="PMPI_Aint_add") result(res)
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    integer(MPI_ADDRESS_KIND), value, intent(in) :: base, disp
    integer(MPI_ADDRESS_KIND) :: res
end function MPIR_Aint_add_c

function  MPIR_Aint_diff_c(addr1, addr2) &
    bind(C, name="PMPI_Aint_diff") result(res)
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    integer(MPI_ADDRESS_KIND), value, intent(in) :: addr1, addr2
    integer(MPI_ADDRESS_KIND) :: res
end function MPIR_Aint_diff_c

end interface
end module mpi_c_interface_nobuf
