!
!     Copyright (C) by Argonne National Laboratory
!         See COPYRIGHT in top-level directory
!
       subroutine mpirinitf( )
       include 'mpif.h'
!      make sure the block data is linked, e.g. from a static library
       external mpirinitf_data

       call mpirinitc(MPI_STATUS_IGNORE, MPI_STATUSES_IGNORE,
     & MPI_BOTTOM, MPI_IN_PLACE, MPI_BUFFER_AUTOMATIC,
     & MPI_UNWEIGHTED, MPI_ERRCODES_IGNORE, MPI_ARGV_NULL,
     & MPI_ARGVS_NULL, MPI_WEIGHTS_EMPTY)
       return
       end

!     Initialize the common blocks so they are defined as initialized
!     data (.data) instead of uninitialized (.bss). Otherwise, a shared
!     library that includes mpif.h may get its own copy of the common
!     blocks, and the sentinels (e.g. MPI_IN_PLACE) it passes will not
!     be recognized, e.g. when it is loaded with RTLD_DEEPBIND. Only the
!     addresses matter. The values must be nonzero, since zero-initialized
!     data still goes to .bss.
       block data mpirinitf_data
       include 'mpifnoext.h'
       data MPI_UNWEIGHTED /1/
       data MPI_WEIGHTS_EMPTY /1/
       data MPI_BUFFER_AUTOMATIC /1/
       data MPI_BOTTOM /1/
       data MPI_ERRCODES_IGNORE /1/
       data MPI_ARGV_NULL /'X'/
       end
