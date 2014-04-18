!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_spawn_f08(command, argv, maxprocs, info, root, comm, intercomm, &
    array_of_errcodes, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char, c_ptr, c_loc, c_associated
    use :: mpi_f08, only : MPI_Info, MPI_Comm
    use :: mpi_f08, only : MPI_ARGV_NULL, MPI_ERRCODES_IGNORE
    use :: mpi_f08, only : MPIR_C_MPI_ARGV_NULL, MPIR_C_MPI_ERRCODES_IGNORE
    use :: mpi_c_interface, only : c_Info, c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_spawn_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    character(len=*), intent(in) :: command
    character(len=*), intent(in), target :: argv(*)
    integer, intent(in) :: maxprocs
    integer, intent(in) :: root
    type(MPI_Info), intent(in) :: info
    type(MPI_Comm), intent(in) :: comm
    type(MPI_Comm), intent(out) :: intercomm
    integer, target :: array_of_errcodes(*)
    integer, optional, intent(out) :: ierror

    character(kind=c_char) :: command_c(len_trim(command)+1)
    integer(c_int) :: maxprocs_c
    integer(c_int) :: root_c
    integer(c_Info) :: info_c
    integer(c_Comm) :: comm_c
    integer(c_Comm) :: intercomm_c
    integer(c_int), target :: array_of_errcodes_c(maxprocs) ! Use automatic array to avoid memory allocation
    integer(c_int) :: ierror_c
    type(c_ptr) :: argv_cptr
    type(c_ptr) :: array_of_errcodes_cptr
    logical :: has_errcodes_ignore = .false.

    call MPIR_Fortran_string_f2c(command, command_c)

    argv_cptr = c_loc(argv)
    if (c_associated(argv_cptr, c_loc(MPI_ARGV_NULL))) then
        argv_cptr = MPIR_C_MPI_ARGV_NULL
    end if

    array_of_errcodes_cptr = c_loc(array_of_errcodes)
    if (c_associated(array_of_errcodes_cptr, c_loc(MPI_ERRCODES_IGNORE))) then
        array_of_errcodes_cptr = MPIR_C_MPI_ERRCODES_IGNORE
        has_errcodes_ignore = .true.
    end if

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_spawn_c(command_c, argv_cptr, maxprocs, info%MPI_VAL, root, &
                                     comm%MPI_VAL, intercomm%MPI_VAL, array_of_errcodes_cptr, len(argv))
    else
        maxprocs_c = maxprocs
        info_c = info%MPI_VAL
        root_c = root
        comm_c = comm%MPI_VAL

        if (.not. has_errcodes_ignore) then
            array_of_errcodes_cptr = c_loc(array_of_errcodes_c)
        end if

        ierror_c = MPIR_Comm_spawn_c(command_c, argv_cptr, maxprocs_c, info_c, root_c, &
                                     comm_c, intercomm_c, array_of_errcodes_cptr, len(argv))

        if (.not. has_errcodes_ignore) then
            array_of_errcodes(1:maxprocs) = array_of_errcodes_c(1:maxprocs)
        end if

        intercomm%MPI_VAL = intercomm_c
    end if

    if (present(ierror)) ierror = ierror_c
end subroutine MPI_Comm_spawn_f08
