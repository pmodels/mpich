##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

all:
	latex users-guide.tex
	bibtex users-guide
	latex users-guide.tex
	latex users-guide.tex
	dvipdfm users-guide
	dvips users-guide

clean:
	-rm -f users-guide.toc users-guide.log users-guide.dvi \
	       users-guide.aux users-guide.bbl users-guide.ps \
               users-guide.blg
