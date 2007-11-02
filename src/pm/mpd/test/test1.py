#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#
# Note that I repeat code for each test just in case I want to
# run one separately.  I can simply copy it out of here and run it. 
# A single test can typically be chgd simply by altering its value(s)
# for one or more of:
#     PYEXT, NMPDS, HFILE

import os, sys, commands, time
sys.path += [os.getcwd()]  # do this once

print "mpd tests---------------------------------------------------"

clusterHosts = [ 'bp4%02d' % (i)  for i in range(0,8) ]
print "clusterHosts=", clusterHosts

# test: simple with 1 mpd (mpdboot uses mpd's -e and -d options)
print "TEST -e -d"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = 'hello\nhello\nhello\n'
mpdtest.run(cmd="mpiexec%s -n 3 /bin/echo hello" % (PYEXT), chkOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: simple with 2 mpds on same machine (mpdboot uses mpd's -n option)
print "TEST -n"
PYEXT = '.py'
NMPDS = 2
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
temph = open(HFILE,'w')
for i in range(NMPDS): print >>temph, '%s' % (socket.gethostname())
temph.close()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = 'hello\nhello\n'
mpdtest.run(cmd="mpiexec%s -n 2 /bin/echo hello" % (PYEXT), chkOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: simple with 3 mpds on 3 machines
print "TEST simple hello msg on 3 nodes"
PYEXT = '.py'
NMPDS = 3
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
temph = open(HFILE,'w')
for host in clusterHosts: print >>temph, host
temph.close()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = 'hello\nhello\nhello\n'
mpdtest.run(cmd="mpiexec%s -n 3 /bin/echo hello" % (PYEXT), chkOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: simple 2 mpds on local machine (-l, -h, and -p option)
print "TEST -l, -h, and -p"
PYEXT = '.py'
NMPDS = 3
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
temph = open(HFILE,'w')
for host in clusterHosts: print >>temph, host
temph.close()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpd%s -d -l 12345" % (PYEXT) )
os.system("mpd%s -d -n -h %s -p 12345" % (PYEXT,socket.gethostname()) )
expout = 'hello\nhello\nhello\n'
mpdtest.run(cmd="mpiexec%s -n 3 /bin/echo hello" % (PYEXT), chkOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: simple with 2 mpds on 2 machines  (--ncpus option)
print "TEST --ncpus"
PYEXT = '.py'
NMPDS = 2
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
temph = open(HFILE,'w')
for host in clusterHosts: print >>temph, "%s:2" % (host)
temph.close()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -f %s -n %d --ncpus=2" % (PYEXT,HFILE,NMPDS) )
myHost = socket.gethostname()
expout = '0: %s\n1: %s\n2: %s\n3: %s\n' % (myHost,myHost,clusterHosts[0],clusterHosts[0])
mpdtest.run(cmd="mpiexec%s -l -n 4 /bin/hostname" % (PYEXT), chkOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test: simple with 2 mpds on 2 machines  (--ifhn option)
#   this is not a great test, but shows working with real ifhn, then failure with 127.0.0.1
print "TEST minimal use of --ifhn"
PYEXT = '.py'
NMPDS = 2
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
temph = open(HFILE,'w')
for host in clusterHosts:
    hostinfo = socket.gethostbyname_ex(host)
    IP = hostinfo[2][0]
    print >>temph, '%s ifhn=%s' % (host,IP)
temph.close()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
hostinfo = socket.gethostbyname_ex(socket.gethostname())
IP = hostinfo[2][0]
os.system("mpdboot%s -f %s -n %d --ifhn=%s" % (PYEXT,HFILE,NMPDS,IP) )
expout = 'hello\nhello\n'
mpdtest.run(cmd="mpiexec%s -n 2 /bin/echo hello" % (PYEXT), chkOut=1, expOut=expout )
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
## redo the above test with a local ifhn that should cause failure
lines = commands.getoutput("mpdboot%s -f %s -n %d --ifhn=127.0.0.1" % (PYEXT,HFILE,NMPDS) )
if len(lines) > 0:
    if lines.find('failed to ping') < 0:
        print "probable error in ifhn test using 127.0.0.1; printing lines of output next:"
        print lines
        sys.exit(-1)

# test:
print "TEST MPD_CON_INET_HOST_PORT"
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
os.environ['MPD_CON_INET_HOST_PORT'] = 'localhost:4444'
os.system("mpd.py &")
time.sleep(1)  ##     time to get going
expout = ['0: hello']
rv = mpdtest.run(cmd="mpiexec%s -l -n 1 echo hello" % (PYEXT), expOut=expout,grepOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
