!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_spawn_multiple_f08(count, array_of_commands, array_of_argv, array_of_maxprocs, &
    array_of_info, root, comm, intercomm, array_of_errcodes, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char, c_ptr, c_loc, c_associated
    use :: mpi_f08, only : MPI_Info, MPI_Comm
    use :: mpi_f08, only : MPI_ARGVS_NULL, MPI_ERRCODES_IGNORE
    use :: mpi_f08, only : MPIR_C_MPI_ARGVS_NULL, MPIR_C_MPI_ERRCODES_IGNORE
    use :: mpi_c_interface, only : c_Info, c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_spawn_multiple_c

    implicit none

    integer, intent(in) :: count
    character(len=*), intent(in), target :: array_of_commands(*)
    character(len=*), intent(in), target :: array_of_argv(count, *)
    integer, intent(in) :: array_of_maxprocs(*)
    type(MPI_Info), intent(in) :: array_of_info(*)
    integer, intent(in) :: root
    type(MPI_Comm), intent(in) :: comm
    type(MPI_Comm), intent(out) :: intercomm
    integer, target :: array_of_errcodes(*)
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_int) :: array_of_maxprocs_c(count)
    integer(c_Info) :: array_of_info_c(count)
    integer(c_int) :: root_c
    integer(c_Comm) :: comm_c
    integer(c_Comm) :: intercomm_c
    integer(c_int), target :: array_of_errcodes_c(sum(array_of_maxprocs(1:count)))
    integer(c_int) :: ierror_c
    integer :: length
    type(c_ptr) :: array_of_argv_cptr
    type(c_ptr) :: array_of_errcodes_cptr
    logical :: has_errcodes_ignore = .false.

    array_of_argv_cptr = c_loc(array_of_argv)
    if (c_associated(array_of_argv_cptr, c_loc(MPI_ARGVS_NULL))) then
        array_of_argv_cptr = MPIR_C_MPI_ARGVS_NULL
    end if

    array_of_errcodes_cptr = c_loc(array_of_errcodes)
    if (c_associated(array_of_errcodes_cptr, c_loc(MPI_ERRCODES_IGNORE))) then
        array_of_errcodes_cptr = MPIR_C_MPI_ERRCODES_IGNORE
        has_errcodes_ignore = .true.
    end if

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_spawn_multiple_c(count, c_loc(array_of_commands), array_of_argv_cptr, &
            array_of_maxprocs, array_of_info(1:count)%MPI_VAL, root, comm%MPI_VAL, intercomm%MPI_VAL, &
            array_of_errcodes_cptr, len(array_of_commands), len(array_of_argv))
    else
        count_c = count
        array_of_maxprocs_c(1:count) = array_of_maxprocs(1:count)
        array_of_info_c = array_of_info(1:count)%MPI_VAL
        root_c = root
        comm_c = comm%MPI_VAL

        if (.not. has_errcodes_ignore) then
            array_of_errcodes_cptr = c_loc(array_of_errcodes_c)
        end if

        ierror_c = MPIR_Comm_spawn_multiple_c(count_c, c_loc(array_of_commands), array_of_argv_cptr, &
             array_of_maxprocs_c, array_of_info_c, root_c, comm_c, intercomm_c, array_of_errcodes_cptr, &
             len(array_of_commands), len(array_of_argv))

        if (.not. has_errcodes_ignore) then
            length = sum(array_of_maxprocs(1:count))
            array_of_errcodes(1:length) = array_of_errcodes_c
        end if

        intercomm%MPI_VAL = intercomm_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_spawn_multiple_f08
