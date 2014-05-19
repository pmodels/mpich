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

    character (len=10) :: name='test12'
    character (len=80) :: title='test 12: Isend/Irecv array slice - iar(2:7)'

    integer :: sint, i
    integer, dimension(10) ::  iar, iar_check
    type(MPI_Request) request
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

        block
            ASYNCHRONOUS :: iar

            call mpi_isend(iar(2:7), 6, MPI_INTEGER, 1, 678, MPI_COMM_WORLD, request, ierr);
            if (ierr .ne. MPI_SUCCESS) then
                if (verbose) print *,"PE ",rank,": ",name,": mpi_isend exited in error (",ierr,")"
                errs = errs + 1
            endif

            call mpi_wait(request, status, ierr)
            if (ierr .ne. MPI_SUCCESS) then
                if (verbose) print *,"PE ",rank,": ",name,": mpi_wait exited in error (",ierr,")"
                errs = errs + 1
            endif

        end block

    else if (rank .eq. 1) then
        do i=1,10
            iar(i)=0
            iar_check(i)=0
        end do
        do i=2,7
            iar_check(i)=i
        end do

        block
            ASYNCHRONOUS :: iar

            call mpi_irecv(iar(2:7), 6, MPI_INTEGER, 0, 678, MPI_COMM_WORLD, request, ierr);
            if (ierr .ne. MPI_SUCCESS) then
                if (verbose) print *,"PE ",rank,": ",name,"mpi_irecv exited in error (",ierr,")"
                errs = errs + 1
            endif

            call mpi_wait(request, status, ierr)
            if (ierr .ne. MPI_SUCCESS) then
                if (verbose) print *,"PE ",rank,": ",name,": mpi_wait exited in error (",ierr,")"
                errs = errs + 1
            endif

        end block

        do i=1,10
            if (iar(i) .ne. iar_check(i)) then
                if (verbose) print *, "rank 1: iar(", i, ")=", iar(i), ", expected ",iar_check(i)
                errs = errs + 1
            endif
        end do
        if (errs .eq. 0) then
            if (verbose) print *, "PE ", rank,": PASS - test 12 - ", trim(title)
        else
            if (verbose) print *, "PE ", rank,": FAIL - test 12 - ", trim(title)
        endif
        if (verbose) print *, iar(1), iar(2), iar(3), iar(4), iar(5), iar(6), iar(7), iar(8), iar(9)
    endif

    call MTest_Finalize(errs)
    call MPI_Finalize(ierr)
end program
