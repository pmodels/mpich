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

    character*8 :: name='test1'
    character*80 :: title='test1: Send/Recv - send integer'

    integer :: sint, i
    integer, dimension(10) :: iar
    type(MPI_Status) status

    errs = 0
    verbose = .false.

    call MTest_Init(ierr)
    call mpi_comm_size(MPI_COMM_WORLD, size, ierr)
    call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

    if (rank .eq. 0) then
        sint = 789
        if (verbose) print *, ' ===== ', trim(title), ' ====='
        if (verbose) print *, "rank 0 sends ", sint
        call mpi_send(sint, 1, MPI_INTEGER, 1, 567, MPI_COMM_WORLD, ierr);
        if (ierr .ne. MPI_SUCCESS) then
            print *,"PE ",rank,": ",name,": mpi_send exited in error (",ierr,")"
            errs = errs + 1
        endif
    else
        call mpi_recv(sint, 1, MPI_INTEGER, 0, 567, MPI_COMM_WORLD, status, ierr);

        if (ierr .ne. MPI_SUCCESS) then
            print *,"PE ",rank,": ",name,"mpi_recv exited in error (",ierr,")"
            errs = errs + 1
        endif

        if (verbose) print *, "rank 1 receives ",sint

        if (sint .eq. 789) then
            if (verbose) print *, "PE ", rank,": PASS - ", trim(title)
        else
            errs = errs + 1
            print *, "PE ", rank,": FAIL - ", trim(title)
        endif
    endif

    call MTest_Finalize(errs)
    call MPI_Finalize(ierr)
end program
