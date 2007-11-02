ALL: paper.ps pmi.ps pmi.pdf 
SHELL = /bin/sh

DOCTEXT = doctext
LATEX=latex
DVIPDF=dvipdfm
.SUFFIXES: .ps .pdf .dvi .tex .eps .fig

paper.dvi: paper.tex paper.bib
	-$(LATEX) paper
	-bibtex paper
	-$(LATEX) paper
	-$(LATEX) paper

paper.ps: paper.dvi
	dvips paper

paper.pdf: paper.dvi
	$(DVIPDF) paper

pmi.dvi: pmi.tex

.dvi.ps:
	dvips $*
.dvi.pdf:
	(unset TEXMF ; dvipdfm $* )
.tex.dvi:
	-latex $*
	-bibtex $*
	-latex $*
	-latex $*

PMI_FILES = pmi.tex ../../src/include/pmi.h Makefile

pmi.dvi: $(PMI_FILES)
	touch pmi.ind
	-if [ ! -d pmiman ] ; then mkdir pmiman ; fi
	-rm -f pmiman/*.tex pmiman/tmp.*
	(cd pmiman ; \
	${DOCTEXT} -latex -nolocation -quotefmt ../../../src/include/pmi.h ;\
	../manextract ../../../src/include/pmi.h > /dev/null )
	latex pmi < /dev/null
	-bibtex pmi
	sed -e 's/{\\tt *\\char *`\\_ *}/_/g' \
	    -e 's/MPID_\([A-Za-z0-9]*\)/\1_MPID/g' \
	    -e 's/MPIU_\([A-Za-z0-9]*\)/\1_MPIU/g' \
		pmi.idx > tmp.idx
	-makeindex tmp
	sed -e 's/\([A-Za-z0-9]*\)_MPID/MPID_\1/g' \
	    -e 's/\([A-Za-z0-9]*\)_MPIU/MPIU_\1/g' \
	    -e 's/_/\\texttt{\\char`\\_}/g' tmp.ind > pmi.ind 
	-bibtex pmi
	latex pmi < /dev/null

pmipdf.dvi: $(PMI_FILES)
	sed -e 's/%\\def\\foobar/\\def\\foobar/' pmi.tex > pmipdf.tex
	touch pmipdf.ind
	-if [ ! -d pmiman ] ; then mkdir pmiman ; fi
	-rm -f pmiman/*.tex pmiman/tmp.*
	(cd pmiman ; \
	${DOCTEXT} -latex -nolocation -quotefmt ../../../src/include/pmi.h ;\
	../manextract ../../../src/include/pmi.h > /dev/null )
	latex pmipdf < /dev/null
	-bibtex pmipdf
	sed -e 's/{\\tt *\\char *`\\_ *}/_/g' \
	    -e 's/MPID_\([A-Za-z0-9]*\)/\1_MPID/g' \
	    -e 's/MPIU_\([A-Za-z0-9]*\)/\1_MPIU/g' \
		pmipdf.idx > tmp.idx
	-makeindex tmp
	sed -e 's/\([A-Za-z0-9]*\)_MPID/MPID_\1/g' \
	    -e 's/\([A-Za-z0-9]*\)_MPIU/MPIU_\1/g' \
	    -e 's/_/\\texttt{\\char`\\_}/g' tmp.ind > pmipdf.ind 
	-bibtex pmipdf
	latex pmipdf < /dev/null

pmi.ps: pmi.dvi
	dvips pmi

pmi.pdf: pmipdf.dvi
	rm -f pmi.pdf pmipdf.pdf
	dvipdfm pmipdf
	mv pmipdf.pdf pmi.pdf

clean:
	rm -f *.log *.aux *.dvi pmi.ps pmi.pdf *.bbl
	rm -f *.ilg *.ind *.idx *.blg *.toc
	rm -f pmiman/*.tex

distclean: clean
	rm -f paper.ps paper.pdf pmi.ps pmi.pdf

install-devdocs: pmi.pdf
	if [ ! -d "$(DEV_INSTALL_PREFIX)" ] ; then \
	    echo "You must set DEV_INSTALL_PREFIX first" ; \
	    exit 1 ; fi
	cp -f pmi.pdf $(DEV_INSTALL_PREFIX)

# Handle older fig2dev programs
.fig.eps:
	-fig2dev -L eps -l landscape $*.fig > $*.eps
	if [ ! -s $*.eps ] ; then \
		fig2dev -L ps -l landscape $*.fig > $*.eps ; fi

