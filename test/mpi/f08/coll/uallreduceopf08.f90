!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

! Test user-defined operations whose procedures conform to the MPI_User_function and
! MPI_User_function_c interfaces (TYPE(C_PTR), VALUE buffers), for MPI_Op_create and
! MPI_Op_create_c.

module uop_mod
    use mpi_f08
    use, intrinsic :: iso_c_binding, only : c_ptr, c_f_pointer
    implicit none
contains
    subroutine uop(invec, inoutvec, len, datatype)
        type(c_ptr), value :: invec, inoutvec
        integer :: len
        type(MPI_Datatype) :: datatype
        integer, pointer :: cin(:), cout(:)

        call c_f_pointer(invec, cin, [len])
        call c_f_pointer(inoutvec, cout, [len])
        cout = cin + cout
    end subroutine

    subroutine uop_c(invec, inoutvec, len, datatype)
        type(c_ptr), value :: invec, inoutvec
        integer(kind=MPI_COUNT_KIND) :: len
        type(MPI_Datatype) :: datatype
        integer, pointer :: cin(:), cout(:)

        call c_f_pointer(invec, cin, [len])
        call c_f_pointer(inoutvec, cout, [len])
        cout = cin + cout
    end subroutine
end module

program main
    use mpi_f08
    use uop_mod
    implicit none
    integer :: ierr, errs, i, size, count
    integer :: vin(1024), vout(1024)
    type(MPI_Op) :: sumop, sumop_c

    errs = 0
    call mtest_init(ierr)
    call MPI_Op_create(uop, .true., sumop, ierr)
    call MPI_Op_create_c(uop_c, .true., sumop_c, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)

    count = 1
    do while (count <= 1024)
        do i = 1, count
            vin(i) = i
            vout(i) = -1
        end do
        call MPI_Allreduce(vin, vout, count, MPI_INTEGER, sumop, MPI_COMM_WORLD, ierr)
        do i = 1, count
            if (vout(i) /= i * size) errs = errs + 1
        end do

        vout(1:count) = -1
        call MPI_Allreduce(vin, vout, count, MPI_INTEGER, sumop_c, MPI_COMM_WORLD, ierr)
        do i = 1, count
            if (vout(i) /= i * size) errs = errs + 1
        end do
        count = count * 2
    end do

    call MPI_Op_free(sumop, ierr)
    call MPI_Op_free(sumop_c, ierr)
    call mtest_finalize(errs)
end program
