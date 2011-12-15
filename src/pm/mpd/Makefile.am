# Makefile.in for mpd2
# 
# If we want to slightly alter the way in which our python scripts are installed
# then we could probably replace this long-ish, error-prone, largely-manual
# makefile with an automake-flavored one.  As it is, the current version is
# basically the original Makefile.in modified slightly to account for the
# elimination of the separate install/install-alt mechanism.

# COMPILE and LINK are the automake names for these variables
C_COMPILE = $(COMPILE)
C_LINK	  = $(LINK)

bin_PROGRAMS = mpdroot
EXTRA_PROGRAMS = sigcatcher infloop
noinst_HEADERS = mpdconf.h mpdroot.h
AM_CPPFLAGS = '-DSYSCONFDIR="$(sysconfdir)"'

MPD      = mpd
MPDBOOT  = mpdboot
CMDS     = mpiexec mpdtrace mpdringtest mpdlistjobs mpdkilljob mpdsigjob \
           mpdexit mpdallexit mpdcleanup mpdhelp mpdcheck
HELPERS  = mpdgdbdrv mpdman mpdlib mpdchkpyver Zeroconf

PYTHON_SOURCE =  ${MPD} ${MPDBOOT} ${CMDS} ${HELPERS}

mpdroot_sources  = mpdroot.c
example_sources	 = sigcatcher.c infloop.c

# make sure that all of these files end up in a "make dist" tarball
EXTRA_DIST = ${PYTHON_SOURCE:=.py} autogen.sh

all-local: ${programs}
	@if [ ! -x ${srcdir}/install.sh ] ; then \
	    echo "chmod a+x ${srcdir}/install.sh" ;\
	    if [ ! `chmod a+x ${srcdir}/install.sh` ] ; then \
	        echo "  chmod ok" ;\
	    else \
	        echo "Unable to set ${srcdir}/install.sh file to be executable." ;\
		echo "Please fix and rerun make" ;\
	        exit 1 ;\
	    fi ;\
	fi
	@if [ "${DO_MTV}" = "yes" ] ; then \
	    currdir=`pwd` ; cd ${srcdir}  &&  ${PYTHON} ./mtv_setup.py build_ext --build-lib=$$currdir --build-temp=$$currdir ;\
	fi

clean-local: 
	@-rm -f *.o *.dep *~ ${EXAMPLES} core* *.1 *.pyc
	@-rm -f *.gcno *.gcda *.bb *.bbg
	@-rm -f ${srcdir}/*.gcno ${srcdir}/*.gcda

CLEANFILES = mtv.so

# if configured with wrong prefix, use:
#     make prefix=new_install_dir install
# Note the conditional test for files in PYTHON_SOURCE; this is to allow
# us to install Zeroconf if present (it is not part of the MPICH2 release
# in order to avoid Copyright problems).
install-exec-local:
if PRIMARY_PM_MPD
	@if [ ! -d $(DESTDIR)${bindir} ] ; then \
	    echo "mkdir -p $(DESTDIR)${bindir} " ;\
	    mkdir -p $(DESTDIR)${bindir} ;\
	fi   
	-@if [ -n "`id | grep uid=0`" ] ; then \
	    ${INSTALL} -m 4755 mpdroot $(DESTDIR)${bindir}/mpdroot ;\
        else \
	    ${INSTALL} -m  755 mpdroot $(DESTDIR)${bindir}/mpdroot ;\
	fi
	@echo "copying python files/links into " $(DESTDIR)${bindir}
	-@for cmd in ${PYTHON_SOURCE} ; do \
	    if [ ! -s $(srcdir)/$$cmd.py ] ; then continue ; fi ; \
		sed \
			-e 's#/usr/bin/env python.*#/usr/bin/env ${PYTHON}#' \
			-e 's,/etc/mpd.conf,$(sysconfdir)/mpd.conf,g' \
			${srcdir}/$$cmd.py > $(DESTDIR)${bindir}/temp$$cmd.py ; \
	    ${INSTALL} -m 755 $(DESTDIR)${bindir}/temp$$cmd.py $(DESTDIR)${bindir}/$$cmd.py; \
	    ( rm -f $(DESTDIR)${bindir}/temp$$cmd.py >/dev/null 2>&1 || exit 0 ) ; \
	done
	-@cd $(DESTDIR)${bindir} && rm -f mpd
	-@cd $(DESTDIR)${bindir} && $(LN_S) mpd.py mpd
	-@cd $(DESTDIR)${bindir} && rm -f mpdboot
	-@cd $(DESTDIR)${bindir} && $(LN_S) mpdboot.py mpdboot
	-@for cmd in ${CMDS} ; do \
	    ( cd $(DESTDIR)${bindir} && rm -f $$cmd ; ) ; \
	    ( cd $(DESTDIR)${bindir} && $(LN_S) $$cmd.py $$cmd ; ) \
	done
	-@( rm -f $(DESTDIR)${bindir}/mpdrun >/dev/null 2>&1 || exit 0 )
	-@( rm -f $(DESTDIR)${bindir}/mpdrun.py >/dev/null 2>&1 || exit 0 )
	-@( rm -f $(DESTDIR)${bindir}/mpirun >/dev/null 2>&1 || exit 0 )
	-@( rm -f $(DESTDIR)${bindir}/mpirun.py >/dev/null 2>&1 || exit 0 )
	-@cd $(DESTDIR)${bindir} && $(LN_S) mpiexec mpdrun
	-@cd $(DESTDIR)${bindir} && $(LN_S) mpiexec.py mpdrun.py
	-@cd $(DESTDIR)${bindir} && $(LN_S) mpiexec mpirun
	-@cd $(DESTDIR)${bindir} && $(LN_S) mpiexec.py mpirun.py
	-@if [ "${DO_MTV}" = "yes" ] ; then \
	    ${INSTALL} -m 755 mtv.so $(DESTDIR)${bindir}/mtv.so ;\
	fi
else !PRIMARY_PM_MPD
## This is the former body of the "install-alt" target
## The purpose of this "target" is to allow users to use mpiexec.mpd to run
## MPICH2 programs using the MPD process manager when they configure MPICH2
## to build multiple process managers.
	@echo "MPD alternate PM installation"
	@echo "copying python files/links into " $(DESTDIR)${bindir}
	@if [ ! -d $(DESTDIR)${bindir} ] ; then \
	    echo "mkdir -p $(DESTDIR)${bindir} " ;\
	    mkdir -p $(DESTDIR)${bindir} ;\
	fi   
	for cmd in ${PYTHON_SOURCE} ; do \
	    if [ ! -s $(srcdir)/$$cmd.py ] ; then continue ; fi ; \
	    sed -e 's#/usr/bin/env python.*#/usr/bin/env ${PYTHON}#' ${srcdir}/$$cmd.py > $(DESTDIR)${bindir}/temp$$cmd.py ; \
	    ${INSTALL} -m 755 $(DESTDIR)${bindir}/temp$$cmd.py $(DESTDIR)${bindir}/$$cmd.py; \
	    ( rm -f $(DESTDIR)${bindir}/temp$$cmd.py >/dev/null 2>&1 || exit 0 ) ; \
	done
	cd $(DESTDIR)${bindir} && rm -f mpd
	cd $(DESTDIR)${bindir} && $(LN_S) mpd.py mpd
	cd $(DESTDIR)${bindir} && rm -f mpdboot
	cd $(DESTDIR)${bindir} && $(LN_S) mpdboot.py mpdboot
	for cmd in ${CMDS} ; do \
	    if [ $$cmd = "mpiexec" ] ; then continue ; fi ; \
	    ( cd $(DESTDIR)${bindir} && rm -f $$cmd ; ) ; \
	    ( cd $(DESTDIR)${bindir} && $(LN_S) $$cmd.py $$cmd ; ) \
	done
	( rm -f $(DESTDIR)${bindir}/mpdrun >/dev/null 2>&1 || exit 0 )
	( rm -f $(DESTDIR)${bindir}/mpdrun.py >/dev/null 2>&1 || exit 0 )
	mv -f $(DESTDIR)${bindir}/mpiexec.py $(DESTDIR)${bindir}/mpiexec.mpd
	cd $(DESTDIR)${bindir} && $(LN_S) mpiexec.mpd mpdrun
endif !PRIMARY_PM_MPD

uninstall-local:
	-rm -f $(DESTDIR)${bindir}/mpdroot
	-rm -f $(DESTDIR)${bindir}/Zeroconf.py
	-rm -f $(DESTDIR)${bindir}/mpd
	-rm -f $(DESTDIR)${bindir}/mpd.py
	-rm -f $(DESTDIR)${bindir}/mpdallexit
	-rm -f $(DESTDIR)${bindir}/mpdallexit.py
	-rm -f $(DESTDIR)${bindir}/mpdboot
	-rm -f $(DESTDIR)${bindir}/mpdboot.py
	-rm -f $(DESTDIR)${bindir}/mpdcheck
	-rm -f $(DESTDIR)${bindir}/mpdcheck.py
	-rm -f $(DESTDIR)${bindir}/mpdchkpyver.py
	-rm -f $(DESTDIR)${bindir}/mpdcleanup
	-rm -f $(DESTDIR)${bindir}/mpdcleanup.py
	-rm -f $(DESTDIR)${bindir}/mpdexit
	-rm -f $(DESTDIR)${bindir}/mpdexit.py
	-rm -f $(DESTDIR)${bindir}/mpdgdbdrv.py
	-rm -f $(DESTDIR)${bindir}/mpdhelp
	-rm -f $(DESTDIR)${bindir}/mpdhelp.py
	-rm -f $(DESTDIR)${bindir}/mpdkilljob
	-rm -f $(DESTDIR)${bindir}/mpdkilljob.py
	-rm -f $(DESTDIR)${bindir}/mpdlib.py
	-rm -f $(DESTDIR)${bindir}/mpdlib.pyc
	-rm -f $(DESTDIR)${bindir}/mpdlistjobs
	-rm -f $(DESTDIR)${bindir}/mpdlistjobs.py
	-rm -f $(DESTDIR)${bindir}/mpdman.py
	-rm -f $(DESTDIR)${bindir}/mpdman.pyc
	-rm -f $(DESTDIR)${bindir}/mpdringtest
	-rm -f $(DESTDIR)${bindir}/mpdringtest.py
	-rm -f $(DESTDIR)${bindir}/mpdrun
	-rm -f $(DESTDIR)${bindir}/mpdrun.py
	-rm -f $(DESTDIR)${bindir}/mpdsigjob
	-rm -f $(DESTDIR)${bindir}/mpdsigjob.py
	-rm -f $(DESTDIR)${bindir}/mpdtrace
	-rm -f $(DESTDIR)${bindir}/mpdtrace.py
	-rm -f $(DESTDIR)${bindir}/mpecc.in
	-rm -f $(DESTDIR)${bindir}/mpefc.in
	-rm -f $(DESTDIR)${bindir}/mpiexec
	-rm -f $(DESTDIR)${bindir}/mpiexec.py
	-rm -f $(DESTDIR)${bindir}/mpirun
	-rm -f $(DESTDIR)${bindir}/mpirun.py

## DJG: these two rules seem to be unused...
mpich2-mpdroot: mpich2-mpdroot.o
	$(C_LINK) -o mpich2-mpdroot mpich2-mpdroot.o $(LIBS)
	rm -f mpich2-mpdroot.o

mpich2-mpdroot.o: ${srcdir}/mpdroot.c ${srcdir}/mpdroot.h mpdconf.h
	${INSTALL} -m 644 ${srcdir}/mpdroot.c mpich2-mpdroot.c
	${C_COMPILE} -DSYSCONFDIR=\"$(sysconfdir)\" -c mpich2-mpdroot.c
	rm -f mpich2-mpdroot.c

# Empty documentations targets (MPICH2 wants this target)
htmldoc:
latexdoc:

# make MPICH2 happy
coverage:

# EXPERIMENTAL TOTALVIEW STUFF

# EXPERIMENTAL MAN STUFF
mandoc:
	-@for cmd in ${PYTHON_SOURCE} ; do \
	    cd ${srcdir} &&  pydoc $$cmd | txt2man -t $$cmd -s 1 -v "mpd cmds" | awk ' !/Help on module/ { print } ' > ../../../man/man1/$$cmd.1 ; \
	done
# (Note to makefile maintainers: not all make programs accept comments in
# the commands for a target; all Makefile comments must begin in the first
# column)
# example:  nroff -man mpd.1 | less

install-man-local:
	-@for cmd in ${PYTHON_SOURCE} ; do \
		${INSTALL} -m 644 $$cmd.1 $(DESTDIR)${mandir}/man1/$$cmd.1 ; \
	done
