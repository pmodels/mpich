!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_compare_f08(comm1,comm2,result, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_compare_c

    implicit none

    type(MPI_Comm), intent(in) :: comm1
    type(MPI_Comm), intent(in) :: comm2
    integer, intent(out) :: result
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm1_c
    integer(c_Comm) :: comm2_c
    integer(c_int) :: result_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_compare_c(comm1%MPI_VAL, comm2%MPI_VAL, result)
    else
        comm1_c = comm1%MPI_VAL
        comm2_c = comm2%MPI_VAL
        ierror_c = MPIR_Comm_compare_c(comm1_c, comm2_c, result_c)
        result = result_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_compare_f08
