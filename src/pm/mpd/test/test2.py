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

print "mpiexec tests-------------------------------------------"

clusterHosts = [ 'bp4%02d' % (i)  for i in range(0,8) ]
print "clusterHosts=", clusterHosts

# test:
print "TEST -machinefile"
PYEXT = '.py'
NMPDS = 4
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
temph = open(HFILE,'w')
for host in clusterHosts: print >>temph, host
temph.close()
tempm = open('tempm','w')
for host in clusterHosts: print >>tempm, '%s:2 ifhn=%s' % (host,host)
tempm.close()
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = '0: bp400\n1: bp400\n2: bp401\n3: bp401\n'   # 2 per host because of :2's in tempm
mpdtest.run(cmd="mpiexec%s -l -machinefile %s -n 4 hostname" % (PYEXT,'tempm'), chkOut=1, expOut=expout)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -file"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
tempfilename = '/tmp/%s_tempin2' % (os.environ['USER'])
f = open(tempfilename,'w')
print >>f, """
   <create-process-group totalprocs='3'>
      <process-spec
	   range='0-2'
	   exec='/bin/echo'>
        <arg idx='1' value="hello"> </arg>
        <arg idx='2' value="again"> </arg>
      </process-spec>
   </create-process-group>
"""
f.close()
expout = 'hello again\nhello again\nhello again\n'
mpdtest.run(cmd="mpiexec%s -file %s" % (PYEXT,tempfilename),chkOut=1,expOut=expout)
os.unlink(tempfilename)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -configfile"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
tempfilename = '/tmp/%s_tempin2' % (os.environ['USER'])
f = open(tempfilename,'w')
print >>f, "-l\n-n 1 echo hello there\n-n 1 echo hello again"
f.close()
expout = '0: hello there\n1: hello again\n'
mpdtest.run(cmd="mpiexec%s -configfile %s" % (PYEXT,tempfilename),chkOut=1,expOut=expout)
os.unlink(tempfilename)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -l"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = '0: hello\n1: bye\n'
mpdtest.run(cmd="mpiexec%s -l -n 1 echo hello : -n 1 echo bye" % (PYEXT),chkOut=1,expOut=expout)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -m"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = '0-1:  hello\n'
mpdtest.run(cmd="mpiexec%s -m -n 2 echo hello" % (PYEXT),chkOut=1,expOut=expout)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -ecfn"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = 'hello\n'
rv = mpdtest.run(cmd="mpiexec%s -ecfn tempxout -n 1 echo hello" % (PYEXT),chkOut=1,expOut=expout)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
linesAsStr = commands.getoutput("cat tempxout")
os.unlink("tempxout")
if linesAsStr.find('exit-codes') < 0:
    print "ecfn: Failed to create correct contents of xml file:"
    print linesAsStr
    sys.exit(-1)

# test:
print "TEST -s"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -n 1" % (PYEXT) )
expin = 'hello\n'
expout = '0: hello\n'
rv = mpdtest.run(cmd="mpiexec%s -l -n 4 cat -" % (PYEXT),
                 expOut=expout,chkOut=1,expIn=expin)
expout = '0: hello\n1: hello\n2: hello\n3: hello\n'
rv = mpdtest.run(cmd="mpiexec%s -l -s 0-3 -n 4 cat -" % (PYEXT),
                 expOut=expout,chkOut=1,expIn=expin)
expout = '0: hello\n1: hello\n2: hello\n3: hello\n'
rv = mpdtest.run(cmd="mpiexec%s -l -s all -n 4 cat -" % (PYEXT),
                 expOut=expout,chkOut=1,expIn=expin)
expout = '0: hello\n2: hello\n3: hello\n'
rv = mpdtest.run(cmd="mpiexec%s -l -s 0,2-3 -n 4 cat -" % (PYEXT),
                 expOut=expout,chkOut=1,expIn=expin)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -1"
PYEXT = '.py'
NMPDS = 2
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
expout = '%s\n' % (clusterHosts[0])
rv = mpdtest.run(cmd="mpiexec%s -1 -n 1 /bin/hostname" % (PYEXT), expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -ifhn"
# not a particularly good test; you can hang/fail with an invalid ifhn
# ifhn is not very useful for mpiexec since mpd can fill it in as needed
PYEXT = '.py'
NMPDS = 2
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
expout = 'hello\n'
rv = mpdtest.run(cmd="mpiexec%s -ifhn 127.0.0.1 -n 1 /bin/echo hello" % (PYEXT),
                 expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -n"
PYEXT = '.py'
NMPDS = 1
HFILE = 'temph'
import os,socket
from mpdlib import MPDTest
mpdtest = MPDTest()
os.environ['MPD_CON_EXT'] = 'testing'
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
expout = '0: hello\n1: bye\n'
mpdtest.run(cmd="mpiexec%s -l -n 1 echo hello : -n 1 echo bye" % (PYEXT),chkOut=1,expOut=expout)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -wdir"
# not a particularly good test; you can hang/fail with an invalid ifhn
# ifhn is not very useful for mpiexec since mpd can fill it in as needed
PYEXT = '.py'
NMPDS = 2
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
expout = '/tmp\n/tmp\n'
rv = mpdtest.run(cmd="mpiexec%s -wdir /tmp -n 2 /bin/pwd" % (PYEXT),
                 expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -path"
# not a particularly good test; you can hang/fail with an invalid ifhn
# ifhn is not very useful for mpiexec since mpd can fill it in as needed
PYEXT = '.py'
NMPDS = 2
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
expout = '/tmp:/bin\n/tmp:/bin\n'
mpdtest.run(cmd="mpiexec%s -path /tmp:/bin -n 2 sh -c '/bin/echo $PATH'" % (PYEXT),
            expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -host"
# not a particularly good test; you can hang/fail with an invalid ifhn
# ifhn is not very useful for mpiexec since mpd can fill it in as needed
PYEXT = '.py'
NMPDS = 5
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
expout = '%s\n' % clusterHosts[3]
rv = mpdtest.run(cmd="mpiexec%s -n 1 -host %s /bin/hostname" % (PYEXT,clusterHosts[3]),
                 expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -soft"
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
expout = 'hello\nhello\nhello\nhello\nhello\n'  # 5 times
rv = mpdtest.run(cmd="mpiexec%s -n 9 -soft 1:5:2 /bin/echo hello" % (PYEXT),
                 expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -envall (the default)"
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
expout = 'BAR\n'
os.environ['FOO'] = 'BAR'
rv = mpdtest.run(cmd="mpiexec%s -n 1 -envall sh -c '/bin/echo $FOO'" % (PYEXT),
                 expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -envnone"
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
expout = '\n'
os.environ['FOO'] = ''
rv = mpdtest.run(cmd="mpiexec%s -n 1 -envnone sh -c '/bin/echo $FOO'" % (PYEXT),
                 expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -env"
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
expout = 'BAR\n'
rv = mpdtest.run(cmd="mpiexec%s -n 1 -env FOO BAR sh -c '/bin/echo $FOO'" % (PYEXT),
                 expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -envlist"
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
os.environ['FOO'] = 'BAR'
os.environ['RMB'] = 'ZZZ'
expout = 'BAR ZZZ\n'
rv = mpdtest.run(cmd="mpiexec%s -n 1 -envlist FOO,RMB sh -c '/bin/echo $FOO $RMB'" % (PYEXT),
                 expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -gn"
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
expout = 'hello\nhello\nbye\nbye\n'
rv = mpdtest.run(cmd="mpiexec%s -gn 2 /bin/echo hello : /bin/echo bye" % (PYEXT),
                 expOut=expout,chkOut=1)
rv = mpdtest.run(cmd="mpiexec%s -gn 2 : /bin/echo hello : /bin/echo bye" % (PYEXT),
                 expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -gexec"
PYEXT = '.py'
NMPDS = 2
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
expout = '%s\n%s\n' % (socket.gethostname(),clusterHosts[0])
rv = mpdtest.run(cmd="mpiexec%s -gexec hostname : -n 1 : -n 1" % (PYEXT),
                 expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

# test:
print "TEST -genvlist"
PYEXT = '.py'
NMPDS = 2
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
os.environ['FOO'] = 'BAR'
os.environ['RMB'] = 'ZZZ'
expout = 'BAR ZZZ\n'
rv = mpdtest.run(cmd="mpiexec%s -genvlist FOO,RMB : sh -c '/bin/echo $FOO $RMB'" % (PYEXT),
                 expOut=expout,chkOut=1)
os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
