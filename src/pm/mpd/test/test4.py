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

print "console pgm tests---------------------------------------------------"

clusterHosts = [ 'bp4%02d' % (i)  for i in range(0,8) ]
print "clusterHosts=", clusterHosts

if not os.access("infloop",os.X_OK):
    os.system("make infloop")

# test:
print "TEST mpdtrace"
PYEXT = '.py'
NMPDS = 3
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
expout = ['%s' % (socket.gethostname()),
          '%s' % (clusterHosts[0]),
          '%s' % (clusterHosts[1])]
rv = mpdtest.run(cmd="mpdtrace%s -l" % (PYEXT), grepOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST mpdringtest"
PYEXT = '.py'
NMPDS = 3
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
nLoops = 100
expout = ['time for %d loops' % (nLoops) ]
rv = mpdtest.run(cmd="mpdringtest%s %d" % (PYEXT,nLoops), grepOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST mpdexit"
PYEXT = '.py'
NMPDS = 3
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
linesAsStr = commands.getoutput("mpdtrace%s -l" % (PYEXT) )
for line in linesAsStr.split('\n'):
    if line.find(clusterHosts[0]) >= 0:
        mpdid = line.split(' ')[0]  # strip off the (ifhn) stuff
        break
else:
    mpdid = ''
if mpdid:
    rv = mpdtest.run(cmd="mpdexit%s %s" % (PYEXT,mpdid), chkOut=1 )
else:
    print 'failed to find %s in mpdtrace for mpdexit' % (clusterHosts[0])
    sys.exit(-1)
expout = ['%s' % (socket.gethostname()), '%s' % (clusterHosts[1])]
rv = mpdtest.run(cmd="mpdtrace%s -l" % (PYEXT),
                 grepOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST mpdlistjobs and mpdkilljob"
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
os.system("mpiexec%s -s all -a ralph -n 2 infloop -p &" % (PYEXT))  # -p => don't print
import time    ## give the mpiexec
time.sleep(2)  ##     time to get going
expout = ['ralph']
rv = mpdtest.run(cmd="mpdlistjobs%s #1" % (PYEXT), grepOut=1, expOut=expout )
rv = mpdtest.run(cmd="mpdkilljob%s -a ralph" % (PYEXT), chkOut=0 )
expout = ''
rv = mpdtest.run(cmd="mpdlistjobs%s #2" % (PYEXT), chkOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("killall -q infloop")  ## just to be safe

# test:
print "TEST mpdsigjob"
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
os.system("mpiexec%s -s all -a ralph -n 2 infloop -p &" % (PYEXT))  # -p => don't print
import time    ## give the mpiexec
time.sleep(2)  ##     time to get going
expout = ['ralph']
rv = mpdtest.run(cmd="mpdlistjobs%s #1" % (PYEXT), grepOut=1, expOut=expout )
rv = mpdtest.run(cmd="mpdsigjob%s INT -a ralph -g" % (PYEXT), chkOut=0 )
expout = ''
rv = mpdtest.run(cmd="mpdlistjobs%s #2" % (PYEXT), chkOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
