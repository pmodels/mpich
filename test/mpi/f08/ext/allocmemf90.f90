! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
        program main
        use mpi_f08
        use, intrinsic :: iso_c_binding
        real, pointer :: a(:,:)
        integer (kind=MPI_ADDRESS_KIND) asize
        type(c_ptr) cptr

        integer ierr, sizeofreal, errs
        integer i,j
!
        errs = 0
        call mtest_init(ierr)
        call mpi_type_size( MPI_REAL, sizeofreal, ierr )
! Make sure we pass in an integer of the correct type
        asize = sizeofreal * 100 * 100
        call mpi_alloc_mem( asize,MPI_INFO_NULL,cptr,ierr )

        call c_f_pointer(cptr, a, [100, 100])

        do i=1,100
            do j=1,100
                a(i,j) = -1
            enddo
        enddo
        a(3,5) = 10.0

        call mpi_free_mem( a, ierr )
        call mtest_finalize(errs)
        call mpi_finalize(ierr)

        end
