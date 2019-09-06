NAME    := mpich
SRC_EXT := gz
SOURCE   = https://www.mpich.org/static/downloads/$(VERSION)/$(NAME)-$(VERSION).tar.$(SRC_EXT)
PATCHES := mpich.macros mpich.pth.py2 mpich.pth.py3 mpich-modules.patch fix-version.patch
#	   0003-soften-version-check.patch daos_adio.patch \
#	   daos_adio-hwloc.patch daos_adio-izem.patch daos_adio-ucx.patch      \
#	   daos_adio-libfabric.patch
# daos_adio-all.patch
ADD_REPOS := openpa libfabric pmix ompi mercury spdk isa-l fio dpdk   \
	     protobuf-c fuse pmdk argobots raft cart@daos_devel1 daos \
	     automake libtool

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

include packaging/Makefile_packaging.mk

GIT_COMMIT := $(shell git rev-parse --short HEAD)

$(NAME)-$(DL_VERSION)-$(GIT_COMMIT).tar:
	mkdir -p rpmbuild
	git archive --prefix=$(subst -$(GIT_COMMIT).tar,,$@)/ -o $@ HEAD
	git submodule update --init
	set -x; p=$$PWD && (echo .; git submodule foreach) |                          \
	    while read junk path; do                                          \
	    temp="$${path%\'}";                                               \
	    temp="$${temp#\'}";                                               \
	    path=$$temp;                                                      \
	    [ "$$path" = "" ] && continue;                                    \
	    (cd $$path && git archive --prefix=$(subst -$(GIT_COMMIT).tar,,$@)/$$path/ HEAD \
	      > $$p/rpmbuild/tmp.tar &&                                       \
	     tar --concatenate --file=$$p/$@                                  \
	       $$p/rpmbuild/tmp.tar && rm $$p/rpmbuild/tmp.tar);              \
	done
	#tar tvf $@

$(NAME)-$(DL_VERSION).tar.$(SRC_EXT): $(NAME)-$(DL_VERSION)-$(GIT_COMMIT).tar
	older_tarballs=$$(ls $(NAME)-$(DL_VERSION)-*.tar |                   \
	                  grep -v $(NAME)-$(DL_VERSION)-$(GIT_COMMIT).tar) ; \
	if [ -n "$$older_tarballs" ]; then                                   \
	    rm -f "$$older_tarballs";                                        \
	fi
	rm -f $@
	gzip < $< > $@
