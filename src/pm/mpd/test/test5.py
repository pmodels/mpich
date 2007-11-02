#!/usr/bin/env python
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#
# Note that I repeat code for each test just in case I want to
# run one separately.  I can simply copy it out of here and run it. 
# A single test can typically be chgd simply by altering its value(s)
# for one or more of:
#     PYEXT, NMPDS, HFILE

import os, sys, commands
sys.path += [os.getcwd()]  # do this once

print "odd tests---------------------------------------------------"

clusterHosts = [ 'bp4%02d' % (i)  for i in range(0,8) ]
print "clusterHosts=", clusterHosts

MPIDir = "/home/rbutler/mpich2"
MPI_1_Dir = "/home/rbutler/mpich1i"

# test: singleton init (cpi)
print "TEST singleton init (cpi)"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = ['Process 0 of 1','pi is approximately 3']
rv = mpdtest.run(cmd="%s/examples/cpi" % (MPIDir), grepOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: bnr (mpich1-compat using cpi)
print "TEST bnr (mpich1-compat using cpi)"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = ['Process 0 on','Process 1 on','Process 2 on','pi is approximately 3']
rv = mpdtest.run(cmd="mpiexec%s -bnr -n 3 %s/examples/cpi" % (PYEXT,MPI_1_Dir),
                 grepOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST ^C to mpiexec"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
temph = open(HFILE,'w')
for host in clusterHosts: print >>temph, host
temph.close()
os.system("mpdboot%s -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
import popen2
runner = popen2.Popen4("mpiexec%s -n 2 infloop -p" % (PYEXT))  # -p => don't print
import time    ## give the mpiexec
time.sleep(2)  ##     time to get going
os.system("kill -INT %d" % (runner.pid) )  # simulate user ^C
expout = ''
rv = mpdtest.run(cmd="mpdlistjobs%s #2" % (PYEXT), chkOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("killall -q infloop")  ## just to be safe

# test:
print "TEST re-knit a ring"
PYEXT = '.py'
NMPDS = 3
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
import popen2, time
mpd1 = popen2.Popen4("mpd%s -l 12345" % (PYEXT))
time.sleep(2)
mpd2 = popen2.Popen4("mpd%s -n -h %s -p 12345" % (PYEXT,socket.gethostname()) )
mpd3 = popen2.Popen4("mpd%s -n -h %s -p 12345" % (PYEXT,socket.gethostname()) )
time.sleep(2)
rv = mpdtest.run(cmd="mpdtrace%s" % (PYEXT), chkOut=0 )
if len(rv['OUT']) != NMPDS:
    print "a: unexpected number of lines of output from mpdtrace", rv['OUT']
    sys.exit(-1)
hostname = socket.gethostname()
for line in rv['OUT']:
    if line.find(hostname) < 0:
        print "a: bad lines in output of mpdtrace", rv['OUT']
        sys.exit(-1)
os.system("kill -9 %d" % (mpd3.pid) )
time.sleep(1)
rv = mpdtest.run(cmd="mpdtrace%s" % (PYEXT), chkOut=0 )
if len(rv['OUT']) != NMPDS-1:
    print "b: unexpected number of lines of output from mpdtrace", rv['OUT']
    sys.exit(-1)
hostname = socket.gethostname()
for line in rv['OUT']:
    if line.find(hostname) < 0:
        print "b: bad lines in output of mpdtrace", rv['OUT']
        sys.exit(-1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
