# Common Makefile for including
# Needs the following variables set at a minium:
# NAME :=
# SRC_EXT :=
# SOURCE =

ifeq ($(DEB_NAME),)
DEB_NAME := $(NAME)
endif

CALLING_MAKEFILE := $(word 1, $(MAKEFILE_LIST))

# Find out what we are
ID_LIKE := $(shell . /etc/os-release; echo $$ID_LIKE)
# Of course that does not work for SLES-12
ID := $(shell . /etc/os-release; echo $$ID)
VERSION_ID := $(shell . /etc/os-release; echo $$VERSION_ID)
ifeq ($(findstring opensuse,$(ID)),opensuse)
ID_LIKE := suse
DISTRO_ID := sl$(VERSION_ID)
endif
ifeq ($(ID),sles)
# SLES-12 or 15 detected.
ID_LIKE := suse
DISTRO_ID := sle$(VERSION_ID)
endif

BUILD_OS ?= leap.42.3
PACKAGING_CHECK_DIR ?= ../packaging
COMMON_RPM_ARGS := --define "%_topdir $$PWD/_topdir"
DIST    := $(shell rpm $(COMMON_RPM_ARGS) --eval %{?dist})
ifeq ($(DIST),)
SED_EXPR := 1p
else
SED_EXPR := 1s/$(DIST)//p
endif
SPEC    := $(NAME).spec
VERSION := $(shell rpm $(COMMON_RPM_ARGS) --specfile --qf '%{version}\n' $(SPEC) | sed -n '1p')
DOT     := .
DEB_VERS := $(subst rc,~rc,$(VERSION))
DEB_RVERS := $(subst $(DOT),\$(DOT),$(DEB_VERS))
DEB_BVERS := $(basename $(subst ~rc,$(DOT)rc,$(DEB_VERS)))
RELEASE := $(shell rpm $(COMMON_RPM_ARGS) --specfile --qf '%{release}\n' $(SPEC) | sed -n '$(SED_EXPR)')
SRPM    := _topdir/SRPMS/$(NAME)-$(VERSION)-$(RELEASE)$(DIST).src.rpm
RPMS    := $(addsuffix .rpm,$(addprefix _topdir/RPMS/x86_64/,$(shell rpm --specfile $(SPEC))))
DEB_TOP := _topdir/BUILD
DEB_BUILD := $(DEB_TOP)/$(NAME)-$(DEB_VERS)
DEB_TARBASE := $(DEB_TOP)/$(DEB_NAME)_$(DEB_VERS)
SOURCES := $(addprefix _topdir/SOURCES/,$(notdir $(SOURCE)) $(PATCHES))
ifeq ($(ID_LIKE),debian)
DEBS    := $(addsuffix _$(DEB_VERS)-1_amd64.deb,$(shell sed -n '/-udeb/b; s,^Package:[[:blank:]],$(DEB_TOP)/,p' debian/control))
#Ubuntu Containers do not set a UTF-8 environment by default.
ifndef LANG
export LANG = C.UTF-8
endif
ifndef LC_ALL
export LC_ALL = C.UTF-8
endif
TARGETS := $(DEBS)
else
# CentOS/Suse packages that want a locale set need this.
ifndef LANG
export LANG = en_US.utf8
endif
ifndef LC_ALL
export LC_ALL = en_US.utf8
endif
TARGETS := $(RPMS) $(SRPM)
endif
all: $(TARGETS)

%/:
	mkdir -p $@

%.gz: %
	rm -f $@
	gzip $<

_topdir/SOURCES/%: % | _topdir/SOURCES/
	rm -f $@
	ln $< $@

# At least one spec file, SLURM (sles), has a different version for the
# download file than the version in the spec file.
ifeq ($(DL_VERSION),)
DL_VERSION = $(VERSION)
endif

$(NAME)-$(DL_VERSION).tar.$(SRC_EXT).asc: $(NAME).spec $(CALLING_MAKEFILE)
	rm -f ./$(NAME)-*.tar.{gz,bz*,xz}.asc
	curl -f -L -O '$(SOURCE).asc'

$(NAME)-$(DL_VERSION).tar.$(SRC_EXT): $(NAME).spec $(CALLING_MAKEFILE)
	rm -f ./$(NAME)-*.tar.{gz,bz*,xz}
	curl -f -L -O '$(SOURCE)'

v$(DL_VERSION).tar.$(SRC_EXT): $(NAME).spec $(CALLING_MAKEFILE)
	rm -f ./v*.tar.{gz,bz*,xz}
	curl -f -L -O '$(SOURCE)'

$(DL_VERSION).tar.$(SRC_EXT): $(NAME).spec $(CALLING_MAKEFILE)
	rm -f ./*.tar.{gz,bz*,xz}
	curl -f -L -O '$(SOURCE)'

$(DEB_TOP)/%: % | $(DEB_TOP)/

$(DEB_BUILD)/%: % | $(DEB_BUILD)/

$(DEB_BUILD).tar.$(SRC_EXT): $(notdir $(SOURCE)) | $(DEB_TOP)/
	ln -f $< $@

$(DEB_TARBASE).orig.tar.$(SRC_EXT) : $(DEB_BUILD).tar.$(SRC_EXT)
	rm -f $(DEB_TOP)/*.orig.tar.*
	ln -f $< $@

$(DEB_TOP)/.detar: $(notdir $(SOURCE)) $(DEB_TARBASE).orig.tar.$(SRC_EXT)
	# Unpack tarball
	rm -rf ./$(DEB_BUILD)/*
	mkdir -p $(DEB_BUILD)
	tar -C $(DEB_BUILD) --strip-components=1 -xpf $<
	touch $@

# Extract patches for Debian
$(DEB_TOP)/.patched: $(PATCHES) check-env $(DEB_TOP)/.detar | \
	$(DEB_BUILD)/debian/
	mkdir -p ${DEB_BUILD}/debian/patches
	mkdir -p $(DEB_TOP)/patches
	for f in $(PATCHES); do \
          rm -f $(DEB_TOP)/patches/*; \
	  if git mailsplit -o$(DEB_TOP)/patches < "$$f" ;then \
	      fn=$$(basename "$$f"); \
	      for f1 in $(DEB_TOP)/patches/*;do \
	        [ -e "$$f1" ] || continue; \
	        f1n=$$(basename "$$f1"); \
	        echo "$${fn}_$${f1n}" >> $(DEB_BUILD)/debian/patches/series ; \
	        mv "$$f1" $(DEB_BUILD)/debian/patches/$${fn}_$${f1n}; \
	      done; \
	  else \
	    fb=$$(basename "$$f"); \
	    cp "$$f" $(DEB_BUILD)/debian/patches/ ; \
	    echo "$$fb" >> $(DEB_BUILD)/debian/patches/series ; \
	    if ! grep -q "^Description:\|^Subject:" "$$f" ;then \
	      sed -i '1 iSubject: Auto added patch' \
	        "$(DEB_BUILD)/debian/patches/$$fb" ;fi ; \
	    if ! grep -q "^Origin:\|^Author:\|^From:" "$$f" ;then \
	      sed -i '1 iOrigin: other' \
	        "$(DEB_BUILD)/debian/patches/$$fb" ;fi ; \
	  fi ; \
	done
	touch $@


# Move the debian files into the Debian directory.
ifeq ($(ID_LIKE),debian)
$(DEB_TOP)/.deb_files : $(shell find debian -type f) \
	  $(DEB_TOP)/.detar | \
	  $(DEB_BUILD)/debian/
	find debian -maxdepth 1 -type f -exec cp '{}' '$(DEB_BUILD)/{}' ';'
	if [ -e debian/source ]; then \
	  cp -r debian/source $(DEB_BUILD)/debian; fi
	if [ -e debian/local ]; then \
	  cp -r debian/local $(DEB_BUILD)/debian; fi
	if [ -e debian/examples ]; then \
	  cp -r debian/examples $(DEB_BUILD)/debian; fi
	if [ -e debian/upstream ]; then \
	  cp -r debian/upstream $(DEB_BUILD)/debian; fi
	if [ -e debian/tests ]; then \
	  cp -r debian/tests $(DEB_BUILD)/debian; fi
	rm -f $(DEB_BUILD)/debian/*.ex $(DEB_BUILD)/debian/*.EX
	rm -f $(DEB_BUILD)/debian/*.orig
	touch $@
endif

# see https://stackoverflow.com/questions/2973445/ for why we subst
# the "rpm" for "%" to effectively turn this into a multiple matching
# target pattern rule
$(subst rpm,%,$(RPMS)): $(SPEC) $(SOURCES)
	rpmbuild -bb $(COMMON_RPM_ARGS) $(RPM_BUILD_OPTIONS) $(SPEC)

$(subst deb,%,$(DEBS)): $(DEB_BUILD).tar.$(SRC_EXT) \
	  $(DEB_TOP)/.deb_files $(DEB_TOP)/.detar $(DEB_TOP)/.patched
	rm -f $(DEB_TOP)/*.deb $(DEB_TOP)/*.ddeb $(DEB_TOP)/*.dsc \
	      $(DEB_TOP)/*.dsc $(DEB_TOP)/*.build* $(DEB_TOP)/*.changes \
	      $(DEB_TOP)/*.debian.tar.*
	rm -rf $(DEB_TOP)/*-tmp
	cd $(DEB_BUILD); debuild --no-lintian -b -us -uc
	cd $(DEB_BUILD); debuild -- clean
	git status
	rm -rf $(DEB_TOP)/$(NAME)-tmp
	lfile1=$(shell echo $(DEB_TOP)/$(NAME)[0-9]*_$(DEB_VERS)-1_amd64.deb);\
	  lfile=$$(ls $${lfile1}); \
	  lfile2=$${lfile##*/}; lname=$${lfile2%%_*}; \
	  dpkg-deb -R $${lfile} \
	    $(DEB_TOP)/$(NAME)-tmp; \
	  if [ -e $(DEB_TOP)/$(NAME)-tmp/DEBIAN/symbols ]; then \
	    sed 's/$(DEB_RVERS)-1/$(DEB_BVERS)/' \
	    $(DEB_TOP)/$(NAME)-tmp/DEBIAN/symbols \
	    > $(DEB_BUILD)/debian/$${lname}.symbols; fi
	cd $(DEB_BUILD); debuild -us -uc
	rm $(DEB_BUILD).tar.$(SRC_EXT)
	for f in $(DEB_TOP)/*.deb; do \
	  echo $$f; dpkg -c $$f; done

$(SRPM): $(SPEC) $(SOURCES)
	rpmbuild -bs $(COMMON_RPM_ARGS) $(RPM_BUILD_OPTIONS) $(SPEC)

srpm: $(SRPM)

$(RPMS): $(SRPM) $(CALLING_MAKEFILE)

rpms: $(RPMS)

$(DEBS): $(CALLING_MAKEFILE)

debs: $(DEBS)

ls: $(TARGETS)
	ls -ld $^

ifeq ($(ID_LIKE),rhel fedora)
chrootbuild: $(SRPM) $(CALLING_MAKEFILE)
	if [ -w /etc/mock/default.cfg ]; then                                    \
	    echo -e "config_opts['yum.conf'] += \"\"\"\n" >> /etc/mock/default.cfg;  \
	    for repo in $(ADD_REPOS); do                                             \
	        if [[ $$repo = *@* ]]; then                                          \
	            branch="$${repo#*@}";                                            \
	            repo="$${repo%@*}";                                              \
	        else                                                                 \
	            branch="master";                                                 \
	        fi;                                                                  \
	        echo -e "[$$repo:$$branch:lastSuccessful]\n\
name=$$repo:$$branch:lastSuccessful\n\
baseurl=$${JENKINS_URL}job/daos-stack/job/$$repo/job/$$branch/lastSuccessfulBuild/artifact/artifacts/centos7/\n\
enabled=1\n\
gpgcheck = False\n" >> /etc/mock/default.cfg;                                        \
	    done;                                                                    \
	    echo "\"\"\"" >> /etc/mock/default.cfg;                                  \
	else                                                                         \
	    echo "Unable to update /etc/mock/default.cfg.";                          \
            echo "You need to make sure it has the needed repos in it yourself.";    \
	fi
	mock $(MOCK_OPTIONS) $(RPM_BUILD_OPTIONS) $<
else
sle12_REPOS += --repo https://download.opensuse.org/repositories/science:/HPC/openSUSE_Leap_42.3/     \
	       --repo http://cobbler/cobbler/repo_mirror/sdkupdate-sles12.3-x86_64/                   \
	       --repo http://cobbler/cobbler/repo_mirror/sdk-sles12.3-x86_64                          \
	       --repo http://download.opensuse.org/repositories/openSUSE:/Backports:/SLE-12/standard/ \
	       --repo http://cobbler/cobbler/repo_mirror/updates-sles12.3-x86_64                      \
	       --repo http://cobbler/cobbler/pub/SLES-12.3-x86_64/

sl42_REPOS += --repo https://download.opensuse.org/repositories/science:/HPC/openSUSE_Leap_42.3 \
	      --repo http://download.opensuse.org/update/leap/42.3/oss/                         \
	      --repo http://download.opensuse.org/distribution/leap/42.3/repo/oss/suse/

sl15_REPOS += --repo http://download.opensuse.org/update/leap/15.1/oss/            \
	      --repo http://download.opensuse.org/distribution/leap/15.1/repo/oss/

chrootbuild: $(SRPM) $(CALLING_MAKEFILE)
	add_repos="";                                                       \
	for repo in $(ADD_REPOS); do                                        \
	    if [[ $$repo = *@* ]]; then                                     \
	        branch="$${repo#*@}";                                       \
	        repo="$${repo%@*}";                                         \
	    else                                                            \
	        branch="master";                                            \
	    fi;                                                             \
	    case $(DISTRO_ID) in                                            \
	        sle12.3) distro="sles12.3";                                 \
	        ;;                                                          \
	        sl42.3) distro="leap42.3";                                  \
	        ;;                                                          \
	        sl15.1) distro="leap15.1";                                  \
	        ;;                                                          \
	    esac;                                                           \
	    baseurl=$${JENKINS_URL}job/daos-stack/job/$$repo/job/$$branch/; \
	    baseurl+=lastSuccessfulBuild/artifact/artifacts/$$distro/;      \
            add_repos+=" --repo $$baseurl";                                 \
        done;                                                               \
	curl -O http://download.opensuse.org/repositories/science:/HPC/openSUSE_Leap_42.3/repodata/repomd.xml.key; \
	sudo rpm --import repomd.xml.key;                                   \
	sudo build $(BUILD_OPTIONS) $$add_repos                             \
	     $($(basename $(DISTRO_ID))_REPOS)                              \
	     --dist $(DISTRO_ID) $(RPM_BUILD_OPTIONS) $(SRPM)
endif

docker_chrootbuild:
	docker build --build-arg UID=$$(id -u) -t $(BUILD_OS)-chrootbuild \
	             -f packaging/Dockerfile.$(BUILD_OS) .
	docker run --privileged=true -w $$PWD -v=$$PWD:$$PWD              \
	           -it $(BUILD_OS)-chrootbuild bash -c "make chrootbuild"

rpmlint: $(SPEC)
	rpmlint $<

packaging_check:
	diff --exclude \*.sw?                       \
	     --exclude debian                       \
	     --exclude .git                         \
	     --exclude Jenkinsfile                  \
	     --exclude libfabric.spec               \
	     --exclude Makefile                     \
	     --exclude README.md                    \
	     --exclude _topdir                      \
	     --exclude \*.tar.\*                    \
	     -bur $(PACKAGING_CHECK_DIR)/ packaging/

check-env:
ifndef DEBEMAIL
	$(error DEBEMAIL is undefined)
endif
ifndef DEBFULLNAME
	$(error DEBFULLNAME is undefined)
endif

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

show_targets:
	@echo $(TARGETS)

show_makefiles:
	@echo $(MAKEFILE_LIST)

show_calling_makefile:
	@echo $(CALLING_MAKEFILE)

.PHONY: srpm rpms debs ls chrootbuild rpmlint FORCE \
        show_version show_release show_rpms show_source show_sources \
        show_targets check-env
