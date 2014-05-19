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

    character*8 :: name='test10'
    character*80 :: title='test 10: Isend/Irecv - send integer'

    integer :: sint, i
    integer, dimension(10) :: iar
    type(MPI_Request) request
    type(MPI_Status) status

    errs = 0
    verbose = .false.

    call MTest_Init(ierr)
    call mpi_comm_size(MPI_COMM_WORLD, size, ierr)
    call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

    if (rank .eq. 0) then
        sint = 789
        if (verbose) then
            print *, ' ===== ', trim(title), ' ====='
            print *, "rank 0 sends ", sint
        endif

        block
            ASYNCHRONOUS :: sint

            call mpi_isend(sint, 1, MPI_INTEGER, 1, 567, MPI_COMM_WORLD, request, ierr);
            if (ierr .ne. MPI_SUCCESS) then
                print *,"PE ",rank,": ",name,": mpi_isend exited in error (",ierr,")"
                errs = errs + 1
            endif

            call mpi_wait(request, status, ierr)
            if (ierr .ne. MPI_SUCCESS) then
                print *,"PE ",rank,": ",name,": mpi_wait exited in error (",ierr,")"
                errs = errs + 1
            endif
        end block
    else
        block
            ASYNCHRONOUS :: sint

            call mpi_irecv(sint, 1, MPI_INTEGER, 0, 567, MPI_COMM_WORLD, request, ierr);
            if (ierr .ne. MPI_SUCCESS) then
                print *,"PE ",rank,": ",name,"mpi_irecv exited in error (",ierr,")"
                errs = errs + 1
            endif

            call mpi_wait(request, status, ierr)
            if (ierr .ne. MPI_SUCCESS) then
                print *,"PE ",rank,": ",name,": mpi_wait exited in error (",ierr,")"
                errs = errs + 1
            endif
        end block

        if (verbose) print *, "rank 1 receives ",sint
        if (sint .eq. 789) then
            if (verbose) print *, "PE ", rank,": PASS - ", trim(title)
        else
            print *, "PE ", rank,": FAIL - ", trim(title)
            errs = errs + 1
        endif
    endif

    call MTest_Finalize(errs)
    call MPI_Finalize(ierr)
end program
