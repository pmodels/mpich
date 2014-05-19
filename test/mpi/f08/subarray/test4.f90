!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!

program main
    use mpi_f08
    implicit none

    integer :: size, rank, ierr, errs
    logical :: verbose
    common /flags/ verbose

    character (len=10) :: name='test4'
    character (len=80) :: title='test 4: Send/Recv array slice with stride - iar(2:8:2)'

    integer :: i
    integer, dimension(10) ::  iar, iar_check
    type(MPI_Status) status

    errs = 0
    verbose = .false.

    call MTest_Init(ierr)
    call mpi_comm_size(MPI_COMM_WORLD, size, ierr)
    call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

    if (rank .eq. 0) then
        if (verbose) print *, ' ===== ', trim(title), ' ====='
        do i=1,10
            iar(i)=i
        end do
        call mpi_send(iar(2:8:2), 4, MPI_INTEGER, 1, 789, MPI_COMM_WORLD, ierr);
        if (ierr .ne. MPI_SUCCESS) then
            print *,"PE ",rank,": ",name,": mpi_send exited in error (",ierr,")"
            errs = errs + 1
        endif
    else if (rank .eq. 1) then
        do i=1,10
            iar(i)=0
            iar_check(i)=0
        end do
        do i=2,8,2
            iar_check(i)=i
        end do
        call mpi_recv(iar(2:8:2), 4, MPI_INTEGER, 0, 789, MPI_COMM_WORLD, status, ierr);
        if (ierr .ne. MPI_SUCCESS) then
            print *,"PE ",rank,": ",name,"mpi_recv exited in error (",ierr,")"
            errs = errs + 1
        endif
        do i=1,10
            if (iar(i) .ne. iar_check(i)) then
                print *, "rank 1: iar(", i, ")=", iar(i), ", expected ",iar_check(i)
                errs = errs + 1
            endif
        end do
        if (errs .eq. 0) then
            if (verbose) print *, "PE ", rank,": PASS - ", trim(title)
        else
            print *, "PE ", rank,": FAIL - ", trim(title)
        endif
        if (verbose) print *, iar(1), iar(2), iar(3), iar(4), iar(5), iar(6), iar(7), iar(8), iar(9)
    endif

    call MTest_Finalize(errs)
    call MPI_Finalize(ierr)
end program
