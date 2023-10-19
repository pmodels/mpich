!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

program main
    use mpi
    use, intrinsic :: iso_c_binding
    integer ierr, errs
    integer(kind=MPI_ADDRESS_KIND) :: sz
    type(c_ptr) :: baseptr
    integer, dimension(:,:), pointer :: a
    integer, dimension(2) :: shape

    errs = 0
    call mtest_init(ierr)

    shape = (/100,100/)
    sz = 4 * shape(1) * shape(2)
    call MPI_Alloc_mem(sz, MPI_INFO_NULL, baseptr, ierr);
    call C_F_Pointer(baseptr, a, shape);
    
    a(3, 5) = 35;

    call MPI_Free_mem(a, ierr);

    call mtest_finalize( errs )
end program main

