
subroutine test1(rank)
    use mpi_f08
    implicit none

    character*8 :: name='test1'
    character*80 :: title='test 1: Send/Recv - send integer'
    
    integer :: rank, ierr, sint, i
    integer, dimension(10) :: iar
    type(MPI_Status) status
    
    if (rank .eq. 0) then
        sint = 789
        print *, ' ===== ', trim(title), ' ====='
        print *, "rank 0 sends ", sint
        ierr = MPI_SUCCESS
        call mpi_send(sint, 1, MPI_INTEGER, 1, 567, MPI_COMM_WORLD, ierr);
        if (ierr .ne. MPI_SUCCESS) then
            print *,"PE ",rank,": ",name,": mpi_send exited in error (",ierr,")"
        endif
    else
        ierr = MPI_SUCCESS
        call mpi_recv(sint, 1, MPI_INTEGER, 0, 567, MPI_COMM_WORLD, status, ierr);
        if (ierr .ne. MPI_SUCCESS) then
            print *,"PE ",rank,": ",name,"mpi_recv exited in error (",ierr,")"
        endif
        print *, "rank 1 receives ",sint
        if (sint .eq. 789) then
            print *, "PE ", rank,": PASS - ", trim(title)
        else
            print *, "PE ", rank,": FAIL - ", trim(title)
        endif
    endif

    call MPI_Barrier(MPI_COMM_WORLD, ierr);
    if (ierr .ne. MPI_SUCCESS) then
        print *,"PE ",rank,": ",name,": mpi_barrier exited in error (",ierr,")"
        call MPI_Abort(MPI_COMM_WORLD, 9);
    endif
end
