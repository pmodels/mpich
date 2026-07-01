##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

DOC_SUBDIRS += doc/mansrc doc/userguide doc/installguide
doc3_src += doc/mansrc/mpiconsts.adoc

userdocs:
	for dir in $(DOC_SUBDIRS) ; do \
	    (cd $$dir && $(MAKE) ) ; done
