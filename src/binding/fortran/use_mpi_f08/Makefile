
FC = gfortran
COBJ = mpi_c_interface.o mpi_c_interface_types.o mpi_c_interface_nobuf.o mpi_c_interface_cdesc.o
FOBJ = mpi_f08.o mpi_f08_types.o mpi_f08_constants.o mpi_f08_callbacks.o

FFLAGS = -c

all:	libmpichf08.a

libmpichf08.a:	 $(FOBJ) $(COBJ)
	ar -r $@  $^

mpi_c_interface.o:	mpi_c_interface.f90 mpi_c_interface_types.o mpi_c_interface_nobuf.o mpi_c_interface_cdesc.o
	$(FC)  $(FFLAGS) $<

mpi_c_interface_types.o:	mpi_c_interface_types.f90
	$(FC)  $(FFLAGS) $<

mpi_c_interface_nobuf.o: mpi_c_interface_nobuf.f90  mpi_c_interface_types.o
	$(FC)  $(FFLAGS) $<

mpi_c_interface_cdesc.o: mpi_c_interface_cdesc.f90  mpi_c_interface_types.o
	$(FC)  $(FFLAGS) $<


mpi_f08_types.o: mpi_f08_types.f90  mpi_c_interface_types.o
	$(FC) $(FFLAGS) $<

mpi_f08_constants.o: mpi_f08_constants.f90 mpi_f08_types.o mpi_c_interface_types.o
	$(FC) $(FFLAGS) $<

mpi_f08_callbacks.o: mpi_f08_callbacks.f90 mpi_f08_types.o mpi_f08_constants.o
	$(FC) $(FFLAGS) $<

mpi_f08.o: mpi_f08.f90  mpi_f08_callbacks.o mpi_f08_types.o mpi_f08_constants.o mpi_c_interface.o
	$(FC) -em $(FFLAGS) $<

clean:
	rm -f *.o *.mod *.a

clobber:	clean
	rm -f libmpi_f08.a
