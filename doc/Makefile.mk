##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

DOC_SUBDIRS += doc/mansrc doc/userguide doc/installguide
doc3_src += doc/mansrc/mpiconsts.adoc

userdocs:
	for dir in $(DOC_SUBDIRS) ; do \
	    (cd $$dir && $(MAKE) ) ; done
