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

    character (len=10) :: name='test15'
    character (len=80) :: title='test 15: Isend/Irecv 2d array column slice - iar_2d(1:7:3,2:6:2)'

    integer :: i, j
    integer, dimension(9,9) :: iar_2d, iar_2dch
    type(MPI_Status) status
    type(MPI_Request) request

    errs = 0
    verbose = .false.

    call MTest_Init(ierr)
    call mpi_comm_size(MPI_COMM_WORLD, size, ierr)
    call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

    if (rank .eq. 0) then
        if (verbose) print *, ' ===== ', trim(title), ' ====='

        do i=1,9
            do j=1,9
                iar_2d(j,i) = (j * 10) + i
            end do
        end do

        do j=1,9
            if (verbose) print *, (iar_2d(i,j),i=1,9)
        end do

    else
        do i=1,9
            do j=1,9
                iar_2d(j,i) = 0
                iar_2dch(j,i) = 0
            end do
        end do
        do i=2,6,2
            do j=1,7,3
                iar_2dch(j,i) = (j * 10) + i
            end do
        end do

    endif

    if (rank .eq. 0) then
        block
            ASYNCHRONOUS :: iar_2d
            call mpi_isend(iar_2d(1:7:3,2:6:2), 9, MPI_INTEGER, 1, 123, MPI_COMM_WORLD, request, ierr);
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
        block
            ASYNCHRONOUS :: iar_2d

            call mpi_irecv(iar_2d(1:7:3,2:6:2), 9, MPI_INTEGER, 0, 123, MPI_COMM_WORLD, request, ierr);
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

        do i=2,6,2
            do j=1,7,3
                if (iar_2d(j,i) .ne. iar_2dch(j,i)) then
                    if (verbose) print *, "rank 1: iar_2d(", j, ",", i, ")=", iar_2d(j,i), ", expected ", iar_2dch(j,i)
                    errs = errs + 1
                endif
            end do
        end do
        if (errs .eq. 0) then
            if (verbose) print *, "PE ", rank,": PASS - ", trim(title)
        else
            if (verbose) print *, "PE ", rank,": FAIL - ", trim(title)
        endif
        do j=1,9
            if (verbose) print *, (iar_2d(i,j),i=1,9)
        end do
    endif

    call MTest_Finalize(errs)
    call MPI_Finalize(ierr)
end program
