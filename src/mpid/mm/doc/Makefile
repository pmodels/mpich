ALL: all-redirect
SHELL = /bin/sh

DOCS = mm.ps mm.pdf
all-redirect: ${DOCS} 

.SUFFIXES: 
.SUFFIXES: .ps .pdf .dvi .htm .tex .dvips

.tex.dvi:
	-latex $* < /dev/null > /dev/null 2>&1 
	-latex $* < /dev/null > /dev/null 2>&1 
	latex $*
	
.dvi.pdf:
	unset TEXMF ; if dvipdfm $* ; then : ; else ${DVIPDFM} $* ; fi
.dvi.ps:
	dvips $*

clean:
	-rm -f *~ *.log *.dvi *.aux *.bbl *.blg mm.ps mm.pdf *.toc mm.out

maintainerclean: clean
	-rm -rf ${DOCS} 
