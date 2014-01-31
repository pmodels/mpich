## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

DEVELOPER_SUBDIRS = doc/pmi doc/namepub
DOC_SUBDIRS += doc/mansrc doc/userguide doc/installguide doc/logging \
	       doc/design

userdocs:
	for dir in $(DOC_SUBDIRS) ; do \
	    (cd $$dir && $(MAKE) ) ; done

devdocs:
	for dir in $(DEVELOPER_SUBDIRS) ; do \
	    (cd $$dir && $(MAKE) ) ; done

install-devdocs:
	if [ -z "$(DEV_INSTALL_PREFIX)" ] ; then \
	    echo "DEV_INSTALL_PREFIX must be defined" ; \
	    exit 1 ; \
	fi
	for dir in $(DEVELOPER_SUBDIRS) ; do \
	        export DEV_INSTALL_PREFIX ; \
	        ( cd $$dir && $(MAKE) install-devdocs ) ; \
	done

