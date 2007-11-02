ALL: faq.htm faq.pdf

.SUFFIXES:
.SUFFIXES: .htm .tex .ps .pdf .dvi

faq.htm faq.tex: faqbuild faq.txt Makefile
	@if [ ! -x /usr/bin/perl ] ; then \
	    echo "faqbuild expects to find perl is /usr/bin/perl" ; \
	    echo "edit faqbuild and Makefile with the path to perl" ; \
	    echo "and rerun make" ; \
	    exit 1; \
	fi
	./faqbuild -dosnl

.dvi.ps:
	dvips $*
.dvi.pdf:
	(unset TEXMF ; dvipdfm $*)
.tex.dvi:
	-latex $* </dev/null >/dev/null 2>&1
	latex $*

WEBDIR = /mcs/www/mpi/mpich2
REQFAQDIR = /mcs/request/mpich2-maint/faq

install-web: faq.htm faq.pdf
	@if [ -z "${WEBDIR}" ] ; then \
	    echo "Must set WEBDIR first" ; fi
	cp -f faq.htm ${WEBDIR}/faq.htm
	cp -f faq.pdf ${WEBDIR}/faq.pdf

clean:
	-rm -f faq.htm *.dvi faq.pdf *.log faq.ps faq.out faq.aux
