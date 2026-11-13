##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

nodist_pkgconfig_DATA += packaging/pkgconfig/mpich-cxx.pc

if BUILD_MPICH_LIB
nodist_pkgconfig_DATA += packaging/pkgconfig/mpich-mpich-abi.pc
endif BUILD_MPICH_LIB

if BUILD_ABI_LIB
nodist_pkgconfig_DATA += packaging/pkgconfig/mpich-mpi-abi.pc
endif BUILD_ABI_LIB

# mpich.pc links to the ABI that mpicc uses by default.
# The generic names are used by e.g. CMake's FindMPI.
install-data-hook:
	cd ${DESTDIR}${pkgconfigdir} && ln -f -s @PC_DEFAULT_ABI@ mpich.pc
	if test "@enable_generic_pkgconfig@" = "yes" ; then \
	    cd ${DESTDIR}${pkgconfigdir} && \
	    ln -f -s mpich.pc mpi-c.pc && \
	    ln -f -s mpich-cxx.pc mpi-cxx.pc ; \
	fi
