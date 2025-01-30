#! /usr/bin/env python3
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import sys
import argparse
import tempfile
import os
import subprocess
import re
from datetime import datetime
import warnings

class colors:
    FAILURE = '\033[1;31m' # red
    SUCCESS = '\033[1;32m' # green
    INFO    = '\033[1;33m' # yellow
    PREFIX  = '\033[1;36m' # cyan
    OTHER   = '\033[1;35m' # purple
    END     = '\033[0m'    # reset


##### run_cmd function
def runcmd(runargs, critical=True, silent=False):
    sys.stdout.write(colors.PREFIX + ">>>>> " + colors.END)
    for x in runargs:
        sys.stdout.write(x + " ")
    sys.stdout.write("\n")
    p = subprocess.Popen(runargs, stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
    ret = p.wait()
    out = p.communicate()[0].decode().strip()
    if (ret != 0):
        if (critical):
            print(colors.FAILURE + "==== command failed (critical) ====\n" + colors.END)
            print(out)
            print(colors.FAILURE + "==== command output complete ====\n" + colors.END)
            sys.exit()
        else:
            print(colors.FAILURE + "==== command failed (warning) ====\n" + colors.END)
            print(out)
            print(colors.FAILURE + "==== command output complete ====\n" + colors.END)
            sys.exit()
    return out


##### main function
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--version', help='tarball version to create', required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as tmpdir:
        oldpwd = os.getcwd()

        # go to the tmpdir
        os.chdir(tmpdir)

        # checkout the release tag
        runcmd(['git', 'clone', 'https://github.com/pmodels/yaksa.git'])
        os.chdir('yaksa')
        runcmd(['git', 'checkout', args.version])

        # verify release version is correctly set
        v = open("maint/version.m4", "r")
        lines = v.readlines()
        v.close()
        newlines = []
        for line in lines:
            if 'YAKSA_VERSION_m4' in line:
                version = re.sub(r'm4_define\(\[YAKSA_VERSION_m4\],\[(.*)\]\)dnl', r'\1', line).rstrip()
                if (version != args.version):
                    warnings.warn("Version in maint/version.m4 does not match git tree-ish")
                    newversion = runcmd(['git', 'rev-parse', '--short=12', 'HEAD'], True, True).rstrip()
                    line = line.replace(version, newversion)
                    version = newversion
            if 'libyaksa_so_version_m4' in line:
                libv = re.sub(r'm4_define\(\[libyaksa_so_version_m4\],\[(.*)\]\)dnl', r'\1', line).rstrip()
                prerelease = re.match(r'.*[a-z].*', version)
                if (prerelease and libv != "0:0:0"):
                    warnings.warn("libyaksa_so_version is " + libv + " instead of 0:0:0")
                elif (prerelease == False and libv == "0:0:0"):
                    warnings.warn("libyaksa_so_version is 0:0:0, which is not allowed for full releases")
            if 'YAKSA_RELEASE_DATE_m4' in line:
                d = re.sub(r'm4_define\(\[YAKSA_RELEASE_DATE_m4\],\[(.*)\]\)dnl', r'\1', line).rstrip()
                curtime = datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S UTC")
                line = line.replace(d, curtime)
            newlines.append(line)

        # update the release date in the version file
        v = open("maint/version.m4", "w")
        for line in newlines:
            v.write(line)
        v.close()

        # check autotools versions
        autoconf_v = runcmd(['autoconf', '--version'], True, True).rstrip().splitlines()[0].split()[3]
        assert autoconf_v == "2.69"
        automake_v = runcmd(['automake', '--version'], True, True).rstrip().splitlines()[0].split()[3]
        assert automake_v == "1.15"
        libtool_v = runcmd(['libtool', '--version'], True, True).rstrip().splitlines()[0].split()[3]
        assert libtool_v == "2.4.6"

        # create tarball
        runcmd(['./autogen.sh'])
        runcmd(['./configure'])
        runcmd(['make', 'dist'])
        runcmd(['mv', 'yaksa-' + version + '.tar.gz', oldpwd])
