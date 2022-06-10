##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

DOC_SUBDIRS += doc/mansrc doc/userguide doc/installguide
doc3_src_txt += doc/mansrc/mpiconsts.txt

userdocs:
	for dir in $(DOC_SUBDIRS) ; do \
	    (cd $$dir && $(MAKE) ) ; done
