## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_AD_LUSTRE

noinst_HEADERS += adio/ad_lustre/ad_lustre.h	\
		  adio/ad_lustre/ad_lustre_cyaml.h

romio_other_sources +=                   \
    adio/ad_lustre/ad_lustre.c           \
    adio/ad_lustre/ad_lustre_open.c      \
    adio/ad_lustre/ad_lustre_rwcontig.c  \
    adio/ad_lustre/ad_lustre_wrcoll.c    \
    adio/ad_lustre/ad_lustre_wrstr.c     \
    adio/ad_lustre/ad_lustre_hints.c     \
    adio/ad_lustre/ad_lustre_aggregate.c \
    adio/ad_lustre/ad_lustre_layout.c	 \
    adio/ad_lustre/ad_lustre_cyaml.c

if LUSTRE_YAML
LUSTRE_YAMLLIB = -lyaml
else
LUSTRE_YAMLLIB = -lm
endif
external_libs += $(LUSTRE_YAMLLIB)

if LUSTRE_LOCKAHEAD
romio_other_sources +=                   \
    adio/ad_lustre/ad_lustre_lock.c
endif LUSTRE_LOCKAHEAD

endif BUILD_AD_LUSTRE
