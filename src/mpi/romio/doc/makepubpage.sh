#!/bin/sh

# if you have bibtext2html installed (http://www.lri.fr/~filliatr/bibtex2html/
# but I know there are other packages by that name), then you can re-generate
# the "ROMIO publication page"
# (http://www.mcs.anl.gov/research/projects/romio/pubs.html)

# If you update the command below, please be sure to retain the link to the
# older papers

WEB_HOST=login3.mcs.anl.gov
WEB_DIR=/mcs/web/research/projects/romio

bibtex2html -t "Papers using ROMIO" \
	--header "Please help us keep this list up to date. Contact romio-maint@mcs.anl.gov for any corrections or additions.<p>" \
	--footer "<p> <a href=papers-old.html>Older papers</a>" \
	-r -d -both pubs.bib

if [ $? -eq 0 ] ; then
	"scp pubs* ${WEB_HOST}:${WEB_DIR}"
else
	"error running bibtex2html.  website not updated"
fi
