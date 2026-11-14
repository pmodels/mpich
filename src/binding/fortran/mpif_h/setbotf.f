!
!     Copyright (C) by Argonne National Laboratory
! SPDX-License-Identifier: Apache-2.0
!
       subroutine mpirinitf( )
       include 'mpif.h'

       call mpirinitc(MPI_STATUS_IGNORE, MPI_STATUSES_IGNORE,
     & MPI_BOTTOM, MPI_IN_PLACE, MPI_BUFFER_AUTOMATIC,
     & MPI_UNWEIGHTED, MPI_ERRCODES_IGNORE, MPI_ARGV_NULL,
     & MPI_ARGVS_NULL, MPI_WEIGHTS_EMPTY)
       return
       end
