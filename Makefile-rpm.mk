NAME    := mpich
SRC_EXT := gz
SOURCE   = https://www.mpich.org/static/downloads/$(VERSION)/$(NAME)-$(VERSION).tar.$(SRC_EXT)
PATCHES := mpich.macros mpich.pth.py2 mpich.pth.py3 mpich-modules.patch fix-version.patch
#	   0003-soften-version-check.patch daos_adio.patch \
#	   daos_adio-hwloc.patch daos_adio-izem.patch daos_adio-ucx.patch      \
#	   daos_adio-libfabric.patch
# daos_adio-all.patch
GIT_TAG := v3.3

TOPDIR  := $(shell echo $$PWD)

# sadly, git is too old on EL7 to do this
daos_adio-all.patch:
	# actually need to configure and then generate a diff
	# against the upstream tarball
	#git submodule update
	#git diff --submodule=diff $(GIT_TAG)..HEAD  > $@
	echo ./autogen.sh
	echo ./configure
	echo rm -rf pristine
	echo mkdir pristine
	echo tar -C pristine -xzvf $(notdir $(SOURCE))
	diff -ur --exclude Dockerfile.centos.7 --exclude Jenkinsfile     \
	         --exclude Makefile-rpm.mk --exclude mpich-modules.patch \
	         --exclude mpich.macros --exclude mpich.pth.py2          \
	         --exclude mpich.pth.py3 --exclude mpich.spec            \
	         --exclude pristine --exclude .git --exclude .github     \
	         --exclude _topdir           \
	         --exclude daos_adio\*.patch \
	         --exclude make\ rpms.out\*  \
	         --exclude \*.swp            \
	         --exclude \*.gz            \
	         pristine/* . > $@ || true
	echo rm -rf pristine

# so instead we get a patch for each submodule
define gen_submod_patch
	set -e;                                              \
	DIR=$(1);                                            \
        A=$$(git ls-tree $(GIT_TAG) $$DIR |                  \
	     sed -e 's/[^ ]* [^ ]* \([^ ]*\)	.*/\1/');    \
        B=$$(git ls-tree HEAD $$DIR |                        \
	     sed -e 's/[^ ]* [^ ]* \([^ ]*\)	.*/\1/');    \
	echo "Create patch for $$DIR from $$A..$$B";         \
	cd $$DIR;                                            \
	git diff --src-prefix=a/$$DIR/ --dst-prefix=b/$$DIR/ \
	    $$A..$$B > $(TOPDIR)/$@
endef

daos_adio-hwloc.patch:
	$(call gen_submod_patch,src/hwloc)

daos_adio-izem.patch:
	$(call gen_submod_patch,src/izem)

daos_adio-libfabric.patch:
	$(call gen_submod_patch,src/mpid/ch4/netmod/ofi/libfabric)

daos_adio-ucx.patch:
	$(call gen_submod_patch,src/mpid/ch4/netmod/ucx/ucx)

daos_adio.patch:
	# get all to-be-generated files generated
	#./configure
	# some day (post git 1.8 in EL7) can do this:
	#git diff $(GIT_TAG)..HEAD --                      \
	#    ':(exclude)README.vin'                        \
	#    ':(exclude)src/hwloc'                         \
	#    ':(exclude)src/izem'                          \
	#    ':(exclude)src/mpid/ch4/netmod/ofi/libfabric' \
	#    ':(exclude)src/mpid/ch4/netmod/ucx/ucx'       \
	#
	git diff $(GIT_TAG)..HEAD |                        \
	    sed -e '/^diff --git/d' -e '/^index /d' |      \
	    filterdiff -p 1                                \
	    -x README.vin                                  \
	    -x src/hwloc                                   \
	    -x src/izem                                    \
	    -x src/mpid/ch4/netmod/ofi/libfabric           \
	    -x src/mpid/ch4/netmod/ucx/ucx                 \
	    > $@

COMMON_RPM_ARGS := --define "%_topdir $$PWD/_topdir"
DIST    := $(shell rpm $(COMMON_RPM_ARGS) --eval %{?dist})
ifeq ($(DIST),)
SED_EXPR := 1p
else
SED_EXPR := 1s/$(DIST)//p
endif
SPEC    := $(NAME).spec
VERSION := $(shell rpm $(COMMON_RPM_ARGS) --specfile --qf '%{version}\n' $(SPEC) | sed -n '1p')
RELEASE := $(shell rpm $(COMMON_RPM_ARGS) --specfile --qf '%{release}\n' $(SPEC) | sed -n '$(SED_EXPR)')
SRPM    := _topdir/SRPMS/$(NAME)-$(VERSION)-$(RELEASE)$(DIST).src.rpm
RPMS    := $(addsuffix .rpm,$(addprefix _topdir/RPMS/x86_64/,$(shell rpm --specfile $(SPEC))))
SOURCES := $(addprefix _topdir/SOURCES/,$(notdir $(SOURCE)) $(PATCHES))
TARGETS := $(RPMS) $(SRPM)

all: $(TARGETS)

%/:
	mkdir -p $@

_topdir/SOURCES/%: % | _topdir/SOURCES/
	rm -f $@
	ln $< $@

$(NAME)-$(VERSION).tar.$(SRC_EXT).asc:
	curl -f -L -O '$(SOURCE).asc'

%.gz: %
	rm -f $@
	gzip $<

#$(NAME)-$(VERSION).tar.$(SRC_EXT):
#	curl -f -L -O '$(SOURCE)'
$(NAME)-$(VERSION).tar: Makefile-rpm.mk
	mkdir -p rpmbuild
	git archive --prefix=$(subst .tar,,$@)/ -o $@ HEAD
	git submodule update --init
	set -x; p=$$PWD && (echo .; git submodule foreach) |                          \
	    while read junk path; do                                          \
	    temp="$${path%\'}";                                               \
	    temp="$${temp#\'}";                                               \
	    path=$$temp;                                                      \
	    [ "$$path" = "" ] && continue;                                    \
	    (cd $$path && git archive --prefix=$(subst .tar,,$@)/$$path/ HEAD \
	      > $$p/rpmbuild/tmp.tar &&                                       \
	     tar --concatenate --file=$$p/$@                                  \
	       $$p/rpmbuild/tmp.tar && rm $$p/rpmbuild/tmp.tar);              \
	done
	tar tvf $@

v$(VERSION).tar.$(SRC_EXT):
	curl -f -L -O '$(SOURCE)'

$(VERSION).tar.$(SRC_EXT):
	curl -f -L -O '$(SOURCE)'

# see https://stackoverflow.com/questions/2973445/ for why we subst
# the "rpm" for "%" to effectively turn this into a multiple matching
# target pattern rule
$(subst rpm,%,$(RPMS)): $(SPEC) $(SOURCES)
	rpmbuild -bb $(COMMON_RPM_ARGS) $(RPM_BUILD_OPTIONS) $(SPEC)

$(SRPM): $(SPEC) $(SOURCES)
	rpmbuild -bs $(COMMON_RPM_ARGS) $(SPEC)

srpm: $(SRPM) $(NAME).spec

$(RPMS): Makefile-rpm.mk

rpms: $(RPMS)

ls: $(TARGETS)
	ls -ld $^

mockbuild: $(SRPM) Makefile-rpm.mk
	mock $(MOCK_OPTIONS) $<

rpmlint: $(SPEC)
	rpmlint $<

show_version:
	@echo $(VERSION)

show_release:
	@echo $(RELEASE)

show_rpms:
	@echo $(RPMS)

show_source:
	@echo $(SOURCE)

show_sources:
	@echo $(SOURCES)

show_spec:
	@ls -l $(SPEC)

.PHONY: srpm rpms ls mockbuild rpmlint FORCE show_version show_release show_rpms show_source show_sources
