
program main
    use mpi_f08
    implicit none

    integer size, rank, ierr

    call mpi_init(ierr)

    call mpi_comm_size(MPI_COMM_WORLD, size, ierr)
    call mpi_comm_rank(MPI_COMM_WORLD, rank, ierr)

    print *, 'Hello, I am ', rank, ' of ', size, '.'

    ! Simple scalar
    call test1(rank)

    ! Send/Recv iar(10)
    call test2(rank)

    ! Send/Recv iar(2:7)
    call test3(rank)

    ! Send/Recv iar(2:8:2)
    call test4(rank)

    ! Send/Recv iar_2d(9,9)
    call test5(rank)

    ! Send/Recv 2d row iar_2d(:,3)
    call test6(rank)

    ! Send/Recv 2d row iar_2d(6,:)
    call test7(rank)

    ! Send/Recv 2d row iar_2d(:,2:6:2)
    call test8(rank)

    ! Send/Recv 2d row iar_2d(1:7:3,2:6:2)
    call test9(rank)

    ! Isend/Irecv 2d row iar_2d(:,2:6:2)
    call test10(rank)

    ! Isend/Irecv iar(10)
    call test11(rank)

    ! Isend/Irecv iar(2:7)
    call test12(rank)

    ! Isend/Irecv 2d row iar_2d(:,2:6:2)
    call test18(rank)

    ! Isend/Irecv 2d row iar_2d(1:7:3,2:6:2)
    call test19(rank)

    call mpi_finalize(ierr)
end program main

