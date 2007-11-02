ALL: namepub.ps namepub.pdf
SHELL = /bin/sh

#
# Use doctext in /home/gropp/bin/$ARCH/doctext 
DOCTEXT = doctext
.SUFFIXES: .ps .pdf .dvi .tex .eps .fig

namepub.dvi: namepub.tex code.sty twopm.eps

.dvi.ps:
	dvips $*
.dvi.pdf:
	(unset TEXMF ; dvipdfm $* )
.tex.dvi:
	-latex $* </dev/null 2>&1 >/dev/null
	-bibtex $*
	-latex $* </dev/null 2>&1 >/dev/null
	-latex $*

code.sty: ../adi3/code.sty
	ln -s ../adi3/code.sty

clean:
	rm -f *.log *.aux *.dvi *.bbl 
	rm -f *.ilg namepub.ind namepub.ind *.blg *.toc *.out 
distclean: clean
	rm -f namepub.ps namepub.pdf

install-devdocs: namepub.pdf
	if [ ! -d "$(DEV_INSTALL_PREFIX)" ] ; then \
	    echo "You must set DEV_INSTALL_PREFIX first" ; \
	    exit 1 ; fi
	cp -f namepub.pdf $(DEV_INSTALL_PREFIX)

# Handle older fig2dev programs
.fig.eps:
	-fig2dev -L eps -l landscape $*.fig > $*.eps
	if [ ! -s $*.eps ] ; then \
		fig2dev -L ps -l landscape $*.fig > $*.eps ; fi


