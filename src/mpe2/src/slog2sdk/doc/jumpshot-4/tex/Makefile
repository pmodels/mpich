TARGET     = usersguide
HTML_DIR   = ../html

# This Makefile requires the following tools:
# LYX pdflatex, latex2html
LYX        = lyx
PDFLATEX   = pdflatex
# LATEX2HTML = /usr/local/bin/latex2html
LATEX2HTML = latex2html

# basic Unix command
CP         = cp
MV         = mv
RM         = rm

.SUFFIXES:	.lyx .tex .pdf

#  Generate TEX file from LYX file.
.lyx.tex:
	$(LYX) --export latex $<

#  Generate PDF file from TEX file. Parse it twice to make figure number correct
.tex.pdf:
	$(PDFLATEX) $< ; $(PDFLATEX) $< ; $(PDFLATEX) $<

#  Clean up, Update txt file, Generate PDF file, then html files.
all:	clean $(TARGET).pdf html

#  Update the $(TARGET).txt in the top directory
txt:	$(TARGET).txt
	$(CP) -f -a $(TARGET).txt ../..

#  Latex2html on the *.tex file generated from lyx.
html:
	$(RM) -f $(HTML_DIR)/* ; \
	$(LATEX2HTML) -mkdir -dir $(HTML_DIR) -noaddress -show_section_numbers \
	              -no_footnode -nonumbered_footnotes -local_icons \
	              $(TARGET).tex
	./rm_html_scale_fig

clean:
	$(RM) -f pic/*.eps pic/gif2png/*.eps
	for ext in out toc aux dvi log ; do \
	    $(RM) -f $(TARGET).$$ext ; \
	done
	$(RM) -f $(TARGET).pdf

distclean: clean
