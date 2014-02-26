FC=gfortran

PROG=fort_mpi_tests

TEST_OBJS=\
	fort_mpi_test1.o \
	fort_mpi_test2.o \
	fort_mpi_test3.o \
	fort_mpi_test4.o \
	fort_mpi_test5.o \
	fort_mpi_test6.o \
	fort_mpi_test7.o \
	fort_mpi_test8.o \
	fort_mpi_test9.o \
	fort_mpi_test10.o \
	fort_mpi_test11.o \
	fort_mpi_test12.o \
	fort_mpi_test18.o \
	fort_mpi_test19.o \

F08_MOD=../MPI_F08.mod
F08_MOD_INC=-I ../
F08_MOD_LINK=-L../ -lmpi_f08
BINDLIB=../cdesc/libcdesc.a
BINDLIB_LINK=-L../cdesc -L../f08ts -lf08ts -lcdesc

all: $(PROG)

fort_mpi_tests:	fort_mpi_tests.o $(F08_MOD) $(TEST_OBJS) $(BINDLIB)
	$(FC) -o fort_mpi_tests fort_mpi_tests.o $(TEST_OBJS) $(BINDLIB_LINK) $(F08_MOD_LINK)

fort_mpi_tests.o:	fort_mpi_tests.f90 $(F08_MOD)
	$(FC) -c fort_mpi_tests.f90 $(F08_MOD_INC)

fort_mpi_test1.o:	fort_mpi_test1.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test1.f90  $(F08_MOD_INC)

fort_mpi_test2.o:	fort_mpi_test2.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test2.f90  $(F08_MOD_INC)

fort_mpi_test3.o:	fort_mpi_test3.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test3.f90  $(F08_MOD_INC)

fort_mpi_test4.o:	fort_mpi_test4.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test4.f90  $(F08_MOD_INC)

fort_mpi_test5.o:	fort_mpi_test5.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test5.f90  $(F08_MOD_INC)

fort_mpi_test6.o:	fort_mpi_test6.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test6.f90  $(F08_MOD_INC)

fort_mpi_test7.o:	fort_mpi_test7.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test7.f90  $(F08_MOD_INC)

fort_mpi_test8.o:	fort_mpi_test8.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test8.f90  $(F08_MOD_INC)

fort_mpi_test9.o:	fort_mpi_test9.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test9.f90  $(F08_MOD_INC)

fort_mpi_test10.o:	fort_mpi_test10.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test10.f90  $(F08_MOD_INC)

fort_mpi_test11.o:	fort_mpi_test11.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test11.f90  $(F08_MOD_INC)

fort_mpi_test12.o:	fort_mpi_test12.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test12.f90  $(F08_MOD_INC)

fort_mpi_test18.o:	fort_mpi_test18.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test18.f90  $(F08_MOD_INC)

fort_mpi_test19.o:	fort_mpi_test19.f90 $(F08_MOD)
	$(FC) -c fort_mpi_test19.f90  $(F08_MOD_INC)

clean:
	rm -f *.o *.mod

clobber:	clean
	rm -f $(PROG)

checkrcs:
	sh ../util/checkrcs.sh

# OLD - but should still work

fort_test_2d:	$(F08_MOD) $(BINDLIB) fort_test_2d.o
	$(FC) -o fort_test_2d fort_test_2d.o $(F08_MOD_LINK) $(BINDLIB_LINK)

fort_test_2d.o:	fort_test_2d.f90
	$(FC) -c fort_test_2d.f90 -I mpi_f08

fort_test:	$(F08_MOD) $(BINDLIB) fort_test.o
	$(FC) -o fort_test fort_test.o  $(F08_MOD_LINK) $(BINDLIB_LINK)

fort_test.o:	fort_test.f90 $(F08_MOD)
	$(FC) -c fort_test.f90 -I mpi_f08
