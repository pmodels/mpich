#!/bin/bash
#
# Copyright © 2012-2017 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

set -e
set -x

# Define the version
if test x$1 = x; then
  echo "Missing branch name as first argument"
  exit 1
fi
export hwloc_branch=$1

# environment variables
test -f $HOME/.ciprofile && . $HOME/.ciprofile

# remove everything but the last 10 builds
ls | grep -v ^hwloc- | grep -v ^job- | xargs rm -rf || true
ls -td hwloc-* | tail -n +11 | xargs chmod u+w -R || true
ls -td hwloc-* | tail -n +11 | xargs rm -rf || true

# find the tarball, extract it
tarball=$(ls -tr hwloc-*.tar.gz | grep -v build.tar.gz | tail -1)
basename=$(basename $tarball .tar.gz)
test -d $basename && chmod -R u+rwX $basename && rm -rf $basename
tar xfz $tarball
rm $tarball
cd $basename

# ignore clock problems
touch configure

# Configure, Make and Install
export CFLAGS="-O0 -g -fPIC --coverage -Wall -Wunused-parameter -Wundef -Wno-long-long -Wsign-compare -Wmissing-prototypes -Wstrict-prototypes -Wcomment -pedantic -fdiagnostics-show-option -fno-inline"
export LDFLAGS="--coverage"
./configure
make V=1 |tee hwloc-build.log

# Execute unitary tests (autotest)
make check

# Collect coverage data
find . -path '*/.libs/*.gcno' -exec rename 's@/.libs/@/@' {} \;
find . -path '*/.libs/*.gcda' -exec rename 's@/.libs/@/@' {} \;
lcov --directory . --capture --output-file hwloc.lcov
lcov_cobertura.py hwloc.lcov --output hwloc-coverage.xml

# Run cppcheck analysis
SOURCES_TO_ANALYZE="src tests utils"
SOURCES_TO_EXCLUDE="-itests/ports -isrc/topology-aix.c -isrc/topology-bgq.c -isrc/topology-cuda.c -isrc/topology-darwin.c -isrc/topology-freebsd.c -isrc/topology-gl.c -isrc/topology-hpux.c -isrc/topology-netbsd.c -isrc/topology-nvml.c -isrc/topology-opencl.c -isrc/topology-osf.c -isrc/topology-solaris-chiptype.c -isrc/topology-solaris.c -isrc/topology-windows.c"
CPPCHECK_INCLUDES="-Iinclude -Isrc -Iutils/lstopo -Iutils/hwloc"
CPPCHECK="cppcheck -v -f --language=c --platform=unix64 --enable=all --xml --xml-version=2 --suppress=purgedConfiguration --suppress=missingIncludeSystem ${CPPCHECK_INCLUDES}"
# Main cppcheck
DEFINITIONS="-Dhwloc_getpagesize=getpagesize -DMAP_ANONYMOUS=0x20 -Dhwloc_thread_t=pthread_t -DSYS_gettid=128"
${CPPCHECK} ${DEFINITIONS} ${SOURCES_TO_ANALYZE} ${SOURCES_TO_EXCLUDE} 2> hwloc-cppcheck.xml
# cppcheck on non-Linux ports
DEFINITIONS="-DR_THREAD=6 -DP_DEFAULT=0 -DR_L2CSDL=11 -DR_PCORESDL=12 -DR_REF1SDL=13"
${CPPCHECK} ${DEFINITIONS} src/topology-aix.c -Itests/ports/include/aix 2> hwloc-cppcheck-aix.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} src/topology-bgq.c -Itests/ports/include/bgq 2> hwloc-cppcheck-bgq.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} src/topology-darwin.c -Itests/ports/include/darwin 2> hwloc-cppcheck-darwin.xml
DEFINITIONS="-Dhwloc_thread_t=pthread_t"
${CPPCHECK} ${DEFINITIONS} src/topology-freebsd.c -Itests/ports/include/freebsd 2> hwloc-cppcheck-freebsd.xml
DEFINITIONS="-DMAP_MEM_FIRST_TOUCH=2 -Dhwloc_thread_t=pthread_t"
${CPPCHECK} ${DEFINITIONS} src/topology-hpux.c -Itests/ports/include/hpux 2> hwloc-cppcheck-hpux.xml
DEFINITIONS="-Dhwloc_thread_t=pthread_t"
${CPPCHECK} ${DEFINITIONS} src/topology-osf.c -Itests/ports/include/osf 2> hwloc-cppcheck-osf.xml
DEFINITIONS="-Dhwloc_thread_t=pthread_t"
${CPPCHECK} ${DEFINITIONS} src/topology-netbsd.c -Itests/ports/include/netbsd 2> hwloc-cppcheck-netbsd.xml
DEFINITIONS="-DMADV_ACCESS_LWP=7"
${CPPCHECK} ${DEFINITIONS} src/topology-solaris.c src/topology-solaris-chiptype.c -Itests/ports/include/solaris 2> hwloc-cppcheck-solaris.xml
DEFINITIONS="-Dhwloc_thread_t=HANDLE"
${CPPCHECK} ${DEFINITIONS} src/topology-windows.c -Itests/ports/include/windows 2> hwloc-cppcheck-windows.xml
# cppcheck on GPU ports
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} src/topology-cuda.c -Itests/ports/include/cuda 2> hwloc-cppcheck-cuda.xml
DEFINITIONS="-DNV_CTRL_PCI_DOMAIN=306"
${CPPCHECK} ${DEFINITIONS} src/topology-gl.c -Itests/ports/include/gl 2> hwloc-cppcheck-gl.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} src/topology-nvml.c -Itests/ports/include/nvml 2> hwloc-cppcheck-nvml.xml
DEFINITIONS="-DCL_DEVICE_BOARD_NAME_AMD=0x4038 -DCL_DEVICE_TOPOLOGY_AMD=0x4037"
${CPPCHECK} ${DEFINITIONS} src/topology-opencl.c -Itests/ports/include/opencl 2> hwloc-cppcheck-opencl.xm

CPPCHECK_XMLS=$(ls -m hwloc-cppcheck*.xml)

# Run rats analysis
rats -w 3 --xml ${SOURCES_TO_ANALYZE} > hwloc-rats.xml

RATS_XMLS=$(ls -m hwloc-rats*.xml)

# Run valgrind analysis
VALGRIND="libtool --mode=execute valgrind --leak-check=full --xml=yes"
$VALGRIND --xml-file=hwloc-valgrind-local.xml utils/lstopo/lstopo-no-graphics - >/dev/null
$VALGRIND --xml-file=hwloc-valgrind-xml1.xml utils/lstopo/lstopo-no-graphics -i tests/xml/32em64t-2n8c2t-pci-normalio.xml -.xml >/dev/null

VALGRIND_XMLS=$(ls -m hwloc-valgrind*.xml)

# Disable GCC attributes, sonar-scanner doesn't support it
sed -e '/#define HWLOC_HAVE_ATTRIBUTE/d' -i include/private/autogen/config.h

# Create the config for sonar-scanner
cat > sonar-project.properties << EOF
sonar.host.url=https://hpclib-sed.bordeaux.inria.fr/sonarqube-dev
sonar.login=$(cat ~/.sonarqube-dev-hwloc-token)
sonar.links.homepage=https://www.open-mpi.org/projects/hwloc/
sonar.links.ci=https://ci.inria.fr/hwloc/
sonar.links.scm=https://github.com/open-mpi/hwloc.git
sonar.links.issue=https://github.com/open-mpi/hwloc/issues
sonar.projectKey=hwloc
sonar.projectName=hwloc
sonar.projectDescription=Hardware locality (hwloc)
sonar.projectVersion=git
sonar.branch=$hwloc_branch
sonar.scm.disabled=false
sonar.sourceEncoding=UTF-8
sonar.language=c++
sonar.sources=src, tests, utils
sonar.exclusions=tests/ports
sonar.cxx.errorRecoveryEnabled=true
sonar.cxx.compiler.parser=GCC
sonar.cxx.compiler.charset=UTF-8
sonar.cxx.compiler.regex=^(.*):([0-9]+):[0-9]+: warning: (.*)\\[(.*)\\]$
sonar.cxx.compiler.reportPath=hwloc-build.log
sonar.cxx.coverage.reportPath=hwloc-coverage.xml
sonar.cxx.cppcheck.reportPath=${CPPCHECK_XMLS}
sonar.cxx.includeDirectories=$(echo | gcc -E -Wp,-v - 2>&1 | grep "^ " | tr '\n' ',')include,src,utils/lstopo,utils/hwloc
sonar.cxx.rats.reportPath=${RATS_XMLS}
sonar.cxx.valgrind.reportPath=${VALGRIND_XMLS}
sonar.issue.ignore.multicriteria=e1,e2,e3,e4,e5,e6,e7,e8,e9,e10,e11,e12,e13,e14
# Complete the task associated to this TODO comment.
sonar.issue.ignore.multicriteria.e1.ruleKey=cxx:TodoTagPresence
sonar.issue.ignore.multicriteria.e1.resourceKey=**
# Missing curly brace.
sonar.issue.ignore.multicriteria.e2.ruleKey=cxx:MissingCurlyBraces
sonar.issue.ignore.multicriteria.e2.resourceKey=**
# Extract this magic number '3' into a constant, variable declaration or an enum.
sonar.issue.ignore.multicriteria.e3.ruleKey=cxx:MagicNumber
sonar.issue.ignore.multicriteria.e3.resourceKey=**
# Undocumented API: hwloc_noos_component
sonar.issue.ignore.multicriteria.e4.ruleKey=cxx:UndocumentedApi
sonar.issue.ignore.multicriteria.e4.resourceKey=**
# At most one statement is allowed per line, but 2 statements were found on this line.
sonar.issue.ignore.multicriteria.e5.ruleKey=cxx:TooManyStatementsPerLine
sonar.issue.ignore.multicriteria.e5.resourceKey=**
# Split this 166 characters long line (which is greater than 160 authorized).
sonar.issue.ignore.multicriteria.e6.ruleKey=cxx:TooLongLine
sonar.issue.ignore.multicriteria.e6.resourceKey=**
# 196 more comment lines need to be written to reach the minimum threshold of 25.0% comment density.
# BUG: doesn't seem to match properly, even with * or so on instead of ++
sonar.issue.ignore.multicriteria.e7.ruleKey=common-c++:InsufficientCommentDensity
sonar.issue.ignore.multicriteria.e7.resourceKey=**
# Double check that your buffer is as big as you specify. When using functions that accept a number n of bytes to copy, such as strncpy, be aware that if the dest buffer size = n it may not NULL-terminate the string.
sonar.issue.ignore.multicriteria.e8.ruleKey=rats:snprintf
sonar.issue.ignore.multicriteria.e8.resourceKey=**
# Double check that your buffer is as big as you specify. When using functions that accept a number n of bytes to copy, such as strncpy, be aware that if the dest buffer size = n it may not NULL-terminate the string.sonar.issue.ignore.multicriteria.e8.ruleKey=rats:snprintf
sonar.issue.ignore.multicriteria.e9.ruleKey=rats:memcpy
sonar.issue.ignore.multicriteria.e9.resourceKey=**
# Don't use on memory intended to be secure, because the old structure will not be zeroed out
sonar.issue.ignore.multicriteria.e10.ruleKey=rats:realloc
sonar.issue.ignore.multicriteria.e10.resourceKey=**
# This function does not properly handle non-NULL terminated strings. This does not result in exploitable code, but can lead to access violations.
sonar.issue.ignore.multicriteria.e11.ruleKey=rats:strlen
sonar.issue.ignore.multicriteria.e11.resourceKey=**
# Check to be sure that the format string passed as argument 2 to this function call does not come from an untrusted source that could have added formatting characters that the code is not prepared to handle. Additionally, the format string could contain '%s' without precision that could result in a buffer overflow.
sonar.issue.ignore.multicriteria.e12.ruleKey=rats:sprintf
sonar.issue.ignore.multicriteria.e12.resourceKey=**
# Extra care should be taken to ensure that character arrays that are allocated on the stack are used safely. They are prime targets for buffer overflow attacks.
sonar.issue.ignore.multicriteria.e13.ruleKey=rats:fixed size global buffer
sonar.issue.ignore.multicriteria.e13.resourceKey=**
# A potential race condition vulnerability exists here. Normally a call to this function is vulnerable only when a match check precedes it. No check was detected, however one could still exist that could not be detected.
sonar.issue.ignore.multicriteria.e14.ruleKey=rats:fixed size local buffer
sonar.issue.ignore.multicriteria.e14.resourceKey=**
EOF

# Run the sonar-scanner analysis and submit to SonarQube server
sonar-scanner -X > sonar.log

# cleanup
rm -rf doc
cd ..
tar cfz ${basename}.build.tar.gz $basename
rm -rf $basename
