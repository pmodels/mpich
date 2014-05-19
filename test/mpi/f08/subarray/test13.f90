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

integer sint, iar(10), iar_2d(9,9), iar_3d(9,9,9)
type(MPI_Status) status
integer i, j, k
integer junk(1000)
integer save_my_rank

errs = 0
verbose = .false.

call MTest_Init(ierr)
call mpi_comm_size(MPI_COMM_WORLD, size, ierr)
call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

save_my_rank = rank

! ----------------------------------------------------------
if (rank .eq. 0) then
    sint = 789
    if (verbose) then
        write (0,*) ' ===== Trivial send/recv ====='
        print *, ' ===== Trivial send/recv ====='
        print *, "rank 0 sends ", sint
    endif
    call mpi_send(sint, 1, MPI_INTEGER, 1, 567, MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        if (verbose) print *, "PE ", rank, ": mpi_send exited in error (",ierr,")"
        errs = errs + 1
    endif
else
    call mpi_recv(sint, 1, MPI_INTEGER, 0, 567, MPI_COMM_WORLD, status, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        if (verbose) print *, "PE ", rank,": mpi_recv exited in error (",ierr,")"
        errs = errs + 1
    endif
    if (verbose) print *, "rank 1 receives ",sint
    if (sint .eq. 789) then
        if (verbose) print *, "PE ", rank,": PASS - Trivial send/recv"
    else
        if (verbose) print *, "PE ", rank,": FAIL - Trivial send/recv"
        errs = errs + 1
    endif
endif

! ----------------------------------------------------------
call mpi_barrier(MPI_COMM_WORLD, ierr)
if (rank .eq. 0) then
    if (verbose) then
        write (0,*) ' ===== Send/Recv iar(2:8:2) ====='
        print *, ' ===== Send/Recv iar(2:8:2) ====='
    endif

    do i=1,10
        iar(i)=i
    end do

    if (verbose) then
        print *, 'PE ', rank, '(0),[', save_my_rank, ']: loc(iar)=',loc(iar)
        print *, 'PE 0: loc(iar)=',loc(iar)
    endif

    call mpi_send(iar(2:8:2), 4, MPI_INTEGER, 1, 678, MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        if (verbose) print *, "PE ", rank, "(0): mpi_send exited in error (",ierr,")"
        errs = errs + 1
    endif
else if (rank .eq. 1) then
    if (verbose) print *, 'PE 1: loc(iar)=',loc(iar)
    call mpi_recv(iar(2:8:2), 4, MPI_INTEGER, 0, 678, MPI_COMM_WORLD, status, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        if (verbose) print *, "PE ", rank, "(1): mpi_recv exited in error (",ierr,")"
        errs = errs + 1
    endif
    do i=2,8,2
        if (iar(i) .ne. i) then
            if (verbose) print *, "rank 1: iar(", i, ")=", iar(i), ", expected ",i
            errs = errs + 1
        endif
    end do
    if (errs .eq. 0) then
        if (verbose) print *, "PE ", rank,": PASS - Send/Recv iar(2:8:2)"
    else
        if (verbose) print *, "PE ", rank,": FAIL - Send/Recv iar(2:8:2)"
    endif
endif

! ----------------------------------------------------------
call mpi_barrier(MPI_COMM_WORLD, ierr)
if (rank .eq. 0) then
    if (verbose) then
        write (0, *) " ======== Simple 2d array test (iar_2d) ======== "
        print *, " ======== Simple 2d array test (iar_2d) ======== "
    endif

    do i=1,9
        do j=1,9
            iar_2d(j,i) = (j * 10) + i
        end do
    end do

    do j=1,9
        if (verbose) print *, iar_2d(1,j), iar_2d(2,j), iar_2d(3,j), iar_2d(4,j), &
            iar_2d(5,j), iar_2d(6,j), iar_2d(7,j), iar_2d(8,j), iar_2d(9,j)
    end do

else
    do i=1,9
        do j=1,9
            iar_2d(j,i) = 0
        end do
    end do

endif

if (rank .eq. 0) then
    if (verbose) print *, 'PE 0: Sending: loc(iar_2d)=',loc(iar_2d)
    call mpi_send(iar_2d, 81, MPI_INTEGER, 1, 789, MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        if (verbose) print *, "PE ", rank, ": mpi_send exited in error (",ierr,")"
        errs = errs + 1
    endif
else
    if (verbose) print *, 'PE 1: Receiving: loc(iar_2d)=',loc(iar_2d)
    call mpi_recv(iar_2d, 81, MPI_INTEGER, 0, 789, MPI_COMM_WORLD, status, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        if (verbose) print *, "PE ", rank, "(1): mpi_recv exited in error (",ierr,")"
        errs = errs + 1
    endif
    do i=1,9
        do j=1,9
            if (iar_2d(j,i) .ne. (j * 10) + i) then
                if (verbose) print *, "rank 1: iar_2d(", j, ",", i, ")=", iar_2d(j,i), ", expected ", (j * 10) + i
                errs = errs + 1
            endif
        end do
    end do
    if (errs .eq. 0) then
        if (verbose) print *, "PE ", rank,": PASS - Send/Recv iar_2d()"
    else
        if (verbose) print *, "PE ", rank,": FAIL - Send/Recv iar_2d()"
    endif
endif

! ----------------------------------------------------------
call mpi_barrier(MPI_COMM_WORLD, ierr)
if (rank .eq. 0) then
    if (verbose) then
        write (0, *) " ======== Simple 3d array test (iar_3d) ======== "
        print *, " ======== Simple 3d array test (iar_3d) ======== "
    endif

    do i=1,9
        do j=1,9
            do k=1,9
                iar_3d(k,j,i) = (k * 100) + (j * 10) + i
            end do
        end do
    end do

else
    do i=1,9
        do j=1,9
            do k=1,9
                iar_3d(k, j,i) = 0
            end do
        end do
    end do

endif

if (rank .eq. 0) then
    if (verbose) print *, 'PE 0: Sending: loc(iar_3d)=',loc(iar_3d)
    call mpi_send(iar_3d, 729, MPI_INTEGER, 1, 891, MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        if (verbose) print *, "PE ", rank, ": mpi_send exited in error (",ierr,")"
        errs = errs + 1
    endif
else
    if (verbose) print *, 'PE 1: Receiving: loc(iar_3d)=',loc(iar_3d)
    call mpi_recv(iar_3d, 729, MPI_INTEGER, 0, 891, MPI_COMM_WORLD, status, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        if (verbose) print *, "PE ", rank, "(1): mpi_recv exited in error (",ierr,")"
        errs = errs + 1
    endif

    do i=1,9
        do j=1,9
            do k=1,9
                if (iar_3d(k, j,i) .ne. (k * 100) + (j * 10) + i) then
                    if (verbose) print *, "rank 1: iar_3d(", k, ",", j, ",", i, ")=", &
                        iar_3d(k, j,i), ", expected ", (k * 100) + (j * 10) + i
                    errs = errs + 1
                endif
            end do
        end do
    end do

    if (errs .eq. 0) then
        if (verbose) print *, "PE ", rank,": PASS - Send/Recv iar_3d()"
    else
        if (verbose) print *, "PE ", rank,": FAIL - Send/Recv iar_3d()"
    endif
endif

call MTest_Finalize(errs)
call MPI_Finalize(ierr)
end program
