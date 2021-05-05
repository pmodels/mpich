!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

program pt2pt_large
    use mpi_f08
    integer:: ierr, errs
    integer, parameter:: BUFSIZE = 1024
    Type(MPI_Comm):: comm
    integer:: rank, nproc
    integer:: buf(BUFSIZE)
    integer(kind=MPI_COUNT_KIND):: cnt = BUFSIZE
    integer:: tag = 0

    errs = 0
    call MTest_Init(ierr)
    comm = MPI_COMM_WORLD
    call MPI_Comm_rank(comm, rank, ierr)
    call MPI_Comm_size(comm, nproc, ierr)
    if (nproc /= 2) then
        print *, "Run test with 2 procs!"
    else
        if (rank == 0) then
            call init_sendbuf(buf)
            call MPI_Send(buf, cnt, MPI_INT, 1, tag, comm)
        else
            call init_recvbuf(buf)
            call MPI_Recv(buf, cnt, MPI_INT, 0, tag, comm, MPI_STATUS_IGNORE)
            call check_recvbuf(buf, errs)
        endif
    endif

    call MTest_Finalize(errs)

    contains
    subroutine init_sendbuf(buf)
        integer, dimension(:), intent(inout) :: buf
        integer:: i

        do i = 1,size(buf)
            buf(i) = i
        enddo
    end subroutine

    subroutine init_recvbuf(buf)
        integer, dimension(:), intent(inout) :: buf
        integer:: i

        do i = 1,size(buf)
            buf(i) = -1
        enddo
    end subroutine

    subroutine check_recvbuf(buf, errs)
        integer, dimension(:), intent(in) :: buf
        integer, intent(inout):: errs
        integer:: i

        do i = 1,size(buf)
            if (buf(i) /= i) then
                errs = errs + 1
            endif
        enddo
    end subroutine
end
