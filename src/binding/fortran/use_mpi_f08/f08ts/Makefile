FC=gfortran

FSRCS = \
	abort_f08ts.f90  \
	barrier_f08ts.f90  \
	bsend_f08ts.f90  \
	bsend_init_f08ts.f90  \
	buffer_attach_f08ts.f90  \
	buffer_detach_f08ts.f90  \
	cancel_f08ts.f90  \
	comm_rank_f08ts.f90  \
	comm_size_f08ts.f90  \
	error_string_f08ts.f90  \
	finalize_f08ts.f90  \
	init_f08ts.f90  \
	irecv_f08ts.f90  \
	isend_f08ts.f90  \
	recv_f08ts.f90  \
	send_f08ts.f90  \
	test_f08ts.f90  \
	type_create_f90_complex_f08ts.f90  \
	type_create_f90_integer_f08ts.f90  \
	type_create_f90_real_f08ts.f90  \
	type_match_size_f08ts.f90  \
	wait_f08ts.f90  \
	wtick_f08ts.f90  \
	wtime_f08ts.f90

FOBJS=$(FSRCS:.f90=.o)

FFLAGS =

%.o : %.f90
	$(FC) -c -o $@ $(FFLAGS) $< -I ../

LIBFCALLC=libf08ts.a

all : $(LIBFCALLC)

$(LIBFCALLC) : $(FOBJS)
	ar -cr $@ $^

clean:
	rm -f *.o *.a

clobber : clean
	rm -f $(LIBFCALLC)
