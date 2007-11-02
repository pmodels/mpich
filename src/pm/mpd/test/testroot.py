#!/usr/bin/env python
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#
# This program does installs, etc and needs to run start to finish.
# Presently, it does NOT use the 'testing' MPD_CON_EXT.

import os, sys

USERDIR = "/tmp/rmbmpd"
ROOTDIR = "/tmp/rootmpd"

# install as user
print "install as user ---------------------------------------------------"
if not os.access("./mpdroot",os.X_OK):
    os.system("./configure")  # use prefix on makes below
    os.system("make")
os.system("make prefix=%s install" % (USERDIR) )

# test:
print "TEST mpd as user ; mpdtrace as user"
PYEXT = ''
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.system("%s/bin/mpdallexit%s 1> /dev/null 2> /dev/null" % (USERDIR,PYEXT) )
os.system("%s/bin/mpdboot%s -n %d" % (USERDIR,PYEXT,NMPDS) )
expout = ['%s' % (socket.gethostname())]
rv = mpdtest.run(cmd="%s/bin/mpdtrace%s -l" % (USERDIR,PYEXT), grepOut=1, expOut=expout )
os.system("%s/bin/mpdallexit%s 1> /dev/null 2> /dev/null" % (USERDIR,PYEXT) )

# install as root
print "install as root ---------------------------------------------------"
if not os.access("./mpdroot",os.X_OK):
    os.system("./configure")  # use prefix on makes below
    os.system("make")
os.system("sudo make prefix=%s install" % (ROOTDIR) )    # sudo did not work here

# test:
print "TEST mpd as root ; mpdtrace as user"
PYEXT = ''
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.system("sudo %s/bin/mpdallexit%s 1> /dev/null 2> /dev/null" % (ROOTDIR,PYEXT) )
os.system("sudo %s/bin/mpd%s -d" % (ROOTDIR,PYEXT) )  # not using boot here
import time
time.sleep(2)
os.environ['MPD_USE_ROOT_MPD'] = '1'
# os.system("%s/bin/mpdtrace%s -l" % (ROOTDIR,PYEXT))
expout = ['%s' % (socket.gethostname())]
rv = mpdtest.run(cmd="%s/bin/mpdtrace%s -l" % (ROOTDIR,PYEXT), grepOut=1, expOut=expout )

print "TEST that user cannot remove files owned by root"
os.system("sudo touch /tmp/testroot")
expout = ['cannot remove']
rv = mpdtest.run(cmd="%s/bin/mpiexec%s -n 1 rm -f /tmp/testroot" % (ROOTDIR,PYEXT),
                 grepOut=1, expOut=expout )

os.system("sudo rm -f /tmp/testroot")
os.system("sudo %s/bin/mpdallexit%s 1> /dev/null 2> /dev/null" % (ROOTDIR,PYEXT) )
