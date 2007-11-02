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

print "mpi tests---------------------------------------------------"

clusterHosts = [ 'bp4%02d' % (i)  for i in range(0,8) ]
print "clusterHosts=", clusterHosts

MPIDir = "/home/rbutler/mpich2"

# test: cpi
print "TEST cpi"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = ['Process 0 of 3','Process 1 of 3','Process 2 of 3']
rv = mpdtest.run(cmd="mpiexec%s -n 3 %s/examples/cpi" % (PYEXT,MPIDir),
                 grepOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: spawn1
print "TEST spawn1"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = ['No Errors']
olddir = os.getcwd()
os.chdir('%s/test/mpi/spawn' % (MPIDir))
rv = mpdtest.run(cmd="%s/mpiexec%s -n 1 spawn1" % (olddir,PYEXT),  # -n 1
                 grepOut=1, expOut=expout )
os.chdir(olddir)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: spawn2
print "TEST spawn2"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = ['No Errors']
olddir = os.getcwd()
os.chdir('%s/test/mpi/spawn' % (MPIDir))
rv = mpdtest.run(cmd="%s/mpiexec%s -n 1 spawn2" % (olddir,PYEXT),  # -n 1
                 grepOut=1, expOut=expout )
os.chdir(olddir)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: spawnmult2
print "TEST spawnmult2"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = ['No Errors']
olddir = os.getcwd()
os.chdir('%s/test/mpi/spawn' % (MPIDir))
rv = mpdtest.run(cmd="%s/mpiexec%s -n 2 spawnmult2" % (olddir,PYEXT),  # -n 2
                 grepOut=1, expOut=expout )
os.chdir(olddir)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: spawnargv
print "TEST spawnargv"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = ['No Errors']
olddir = os.getcwd()
os.chdir('%s/test/mpi/spawn' % (MPIDir))
rv = mpdtest.run(cmd="%s/mpiexec%s -n 1 spawnargv" % (olddir,PYEXT),  # -n 2
                 grepOut=1, expOut=expout )
os.chdir(olddir)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: spawnintra
print "TEST spawnintra"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = ['No Errors']
olddir = os.getcwd()
os.chdir('%s/test/mpi/spawn' % (MPIDir))
rv = mpdtest.run(cmd="%s/mpiexec%s -n 1 spawnintra" % (olddir,PYEXT),  # -n 2
                 grepOut=1, expOut=expout )
os.chdir(olddir)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: namepub
print "TEST namepub"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = ['No Errors']
olddir = os.getcwd()
os.chdir('%s/test/mpi/spawn' % (MPIDir))
rv = mpdtest.run(cmd="%s/mpiexec%s -n 1 namepub" % (olddir,PYEXT),  # -n 2
                 grepOut=1, expOut=expout )
os.chdir(olddir)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
