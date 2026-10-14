##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

nodist_pkgconfig_DATA += packaging/pkgconfig/mpich.pc \
                         packaging/pkgconfig/mpich-cxx.pc

# generic names used by e.g. CMake's FindMPI
install-data-hook:
	if test "@enable_generic_pkgconfig@" = "yes" ; then \
	    cd ${DESTDIR}${pkgconfigdir} && \
	    ln -f -s mpich.pc mpi-c.pc && \
	    ln -f -s mpich-cxx.pc mpi-cxx.pc ; \
	fi
