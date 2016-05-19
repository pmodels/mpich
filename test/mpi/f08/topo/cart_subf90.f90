! -*- Mode: Fortran; -*-
!
!  (C) 2016 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
!  Portions of this code were written by Intel Corporation.
!  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
!  to Argonne National Laboratory subject to Software Grant and Corporate
!  Contributor License Agreement dated February 8, 2012.

program main
use mpi_f08
implicit none
integer rank, size

type(MPI_Comm) :: comm_cart, comm_new
integer dims(2), coords(2)
logical periods(2), reorder, remain_dims(2)
integer errs

dims(1:2) = 0
periods(1) = .TRUE.
periods(2) = .FALSE.
reorder    = .TRUE.
remain_dims(1) = .TRUE.
remain_dims(2) = .FALSE.
errs = 0

call MTEST_Init()
call MPI_Comm_rank(MPI_COMM_WORLD, rank)
call MPI_Comm_size(MPI_COMM_WORLD, size)
call MPI_Dims_create(size, 2, dims)
call MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, comm_cart)
call MPI_Comm_rank(comm_cart, rank)
call MPI_Cart_coords(comm_cart, rank, 2, coords)

call MPI_Cart_sub(comm_cart, remain_dims, comm_new)
call MPI_Comm_size(comm_new, size)

call MTEST_Finalize(errs)

end program
