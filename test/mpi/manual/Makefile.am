## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

include $(top_srcdir)/Makefile.mtest

noinst_PROGRAMS = singjoin testconnect testconnectserial dimsbalanced \
                  spawntest_master spawntest_child segfault

testconnectserial_SOURCES = testconnectserial.c tchandlers.c tcutil.c
testconnectserial_LDADD   = $(LDADD) -lm

## why does this override rule exist in the simplemake version?
##testconnectserial.o: testconnectserial.c
##	$(C_COMPILE) -c -DMPICHLIBSTR=\"$(MPIDIR)\" $<

CLEANFILES += test-port

