#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

import os, sys, socket, commands, time
sys.path.append("..")         # where to find mpdlib, etc.
from mpdlib import MPDTest    # do this AFTER extending sys.path

clusterHosts = []
# clusterHosts = ['bp400','bp401','bp402','bp403']
MPI_srcdir = "/home/rbutler/mpich2"
MPI_1_dir  = "/home/rbutler/mpich1i"
USER_INSTALL_DIR = "/tmp/usermpd"
ROOT_INSTALL_DIR = "/tmp/rootmpd"
os.environ['PATH'] = "..:" + os.environ['PATH']
PYEXT = ".py"
HFILE = "temph"
os.environ['MPD_CON_EXT'] = "testing"


testsToRun = [ 0 for x in range(0,10000) ]         # init all to 0; override below

# for i in range(0,10000):    testsToRun[i] = 1    # run all tests
for i in range(0,1000):     testsToRun[i] = 1    # run tests for mpd
# for i in range(1000,2000):  testsToRun[i] = 1    # run tests for mpiexec
# for i in range(2000,3000):  testsToRun[i] = 1    # run MPI tests
# for i in range(3000,4000):  testsToRun[i] = 1    # run console pgm tests
# for i in range(8000,9000):  testsToRun[i] = 1    # run misc tests
# for i in range(9000,10000): testsToRun[i] = 1    # run root tests
# testsToRun[3003] = 1                               # run this one test


if 1 in testsToRun[0:1000]:
    print "mpd tests---------------------------------------------------"

if testsToRun[0]:
    # test: simple with 1 mpd (mpdboot uses mpd's -e and -d options)
    print "TEST -e -d"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %s" % (PYEXT,NMPDS) )
    expout = 'hello\nhello\nhello\n'
    mpdtest.run(cmd="mpiexec%s -n 3 /bin/echo hello" % (PYEXT), chkOut=1, expOut=expout )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[1]:
    # test: simple with 2 mpds on same machine (mpdboot uses mpd's -n option)
    print "TEST -n"
    NMPDS = 2
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        temph = open(HFILE,'w')
        for i in range(NMPDS): print >>temph, '%s' % (socket.gethostname())
        temph.close()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
        expout = 'hello\nhello\n'
        mpdtest.run(cmd="mpiexec%s -n 2 /bin/echo hello" % (PYEXT), chkOut=1, expOut=expout )
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.unlink(HFILE)

if testsToRun[2]:
    # test: simple with 3 mpds on 3 machines
    print "TEST simple hello msg on 3 nodes"
    NMPDS = 3
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        temph = open(HFILE,'w')
        for host in clusterHosts: print >>temph, host
        temph.close()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
        expout = 'hello\nhello\nhello\n'
        mpdtest.run(cmd="mpiexec%s -n 3 /bin/echo hello" % (PYEXT), chkOut=1, expOut=expout )
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.unlink(HFILE)

if testsToRun[3]:
    # test: simple 2 mpds on local machine (-l, -h, and -p option)
    print "TEST -l, -h, and -p"
    NMPDS = 3
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        temph = open(HFILE,'w')
        for host in clusterHosts: print >>temph, host
        temph.close()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.system("mpd%s -d -l 12345" % (PYEXT) )
        os.system("mpd%s -d -n -h %s -p 12345" % (PYEXT,socket.gethostname()) )
        expout = 'hello\nhello\nhello\n'
        mpdtest.run(cmd="mpiexec%s -n 3 /bin/echo hello" % (PYEXT), chkOut=1, expOut=expout )
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.unlink(HFILE)

if testsToRun[4]:
    # test: simple with 2 mpds on 2 machines  (--ncpus option)
    print "TEST --ncpus"
    NMPDS = 2
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        temph = open(HFILE,'w')
        for host in clusterHosts: print >>temph, "%s:2" % (host)
        temph.close()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.system("mpdboot%s -f %s -n %d --ncpus=2" % (PYEXT,HFILE,NMPDS) )
        myHost = socket.gethostname()
        expout = '0: %s\n1: %s\n2: %s\n3: %s\n' % (myHost,myHost,clusterHosts[0],clusterHosts[0])
        mpdtest.run(cmd="mpiexec%s -l -n 4 /bin/hostname" % (PYEXT), chkOut=1, expOut=expout )
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.unlink(HFILE)

if testsToRun[5]:
    # test: simple with 2 mpds on 2 machines  (--ifhn option)
    #   this is not a great test, but shows working with real ifhn, then failure with 127.0.0.1
    print "TEST minimal use of --ifhn"
    NMPDS = 2
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        temph = open(HFILE,'w')
        for host in clusterHosts:
            hostinfo = socket.gethostbyname_ex(host)
            IP = hostinfo[2][0]
            print >>temph, '%s ifhn=%s' % (host,IP)
        temph.close()
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
            if lines.find('failed to handshake') < 0:
                print "probable error in ifhn test using 127.0.0.1; printing lines of output next:"
                print lines
                sys.exit(-1)
        os.unlink(HFILE)

if testsToRun[6]:
    print "TEST MPD_CON_INET_HOST_PORT"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    import random
    port = random.randrange(2000,9000)
    print "    using port", port
    os.environ['MPD_CON_INET_HOST_PORT'] = "localhost:%d" % (port)
    os.system("mpd.py &")
    time.sleep(1)  ##     time to get going
    expout = ['0: hello']
    rv = mpdtest.run(cmd="mpiexec%s -l -n 1 echo hello" % (PYEXT), expOut=expout,grepOut=1)
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.unsetenv('MPD_CON_INET_HOST_PORT')   # get rid of this for later tests


if 1 in testsToRun[1000:2000]:
    print "mpiexec tests-------------------------------------------"

if testsToRun[1000]:
    print "TEST -machinefile"
    NMPDS = 4
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        temph = open(HFILE,'w')
        for host in clusterHosts: print >>temph, host
        temph.close()
        tempm = open('tempm','w')
        for host in clusterHosts: print >>tempm, '%s:2 ifhn=%s' % (host,host)
        tempm.close()
        mpdtest = MPDTest()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.system("mpdboot%s -1 -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
        expout = '0: bp400\n1: bp400\n2: bp401\n3: bp401\n'   # 2 per host because of :2's in tempm
        mpdtest.run(cmd="mpiexec%s -l -machinefile %s -n 4 hostname" % (PYEXT,'tempm'), chkOut=1, expOut=expout)
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.system("rm -f tempm %s" % (HFILE))

if testsToRun[1001]:
    print "TEST -file"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
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
    
if testsToRun[1002]:
    print "TEST -configfile"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    tempfilename = '/tmp/%s_tempin2' % (os.environ['USER'])
    f = open(tempfilename,'w')
    print >>f, "-l\n-n 1 echo hello there\n-n 1 echo hello again"
    f.close()
    expout = '0: hello there\n1: hello again\n'
    mpdtest.run(cmd="mpiexec%s -configfile %s" % (PYEXT,tempfilename),chkOut=1,expOut=expout)
    os.unlink(tempfilename)
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    
if testsToRun[1003]:
    print "TEST -l"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = '0: hello\n1: bye\n'
    mpdtest.run(cmd="mpiexec%s -l -n 1 echo hello : -n 1 echo bye" % (PYEXT),chkOut=1,expOut=expout)
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[1004]:
    print "TEST -m"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = '0-1:  hello\n'
    mpdtest.run(cmd="mpiexec%s -m -n 2 echo hello" % (PYEXT),chkOut=1,expOut=expout)
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[1005]:
    print "TEST -ecfn"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = 'hello\n'
    rv = mpdtest.run(cmd="mpiexec%s -ecfn tempxout -n 1 echo hello" % (PYEXT),chkOut=1,expOut=expout)
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    linesAsStr = commands.getoutput("cat tempxout")
    os.unlink("tempxout")
    if linesAsStr.find('exit-codes') < 0:
        print "ecfn: Failed to create correct contents of xml file:"
        print linesAsStr
        sys.exit(-1)

if testsToRun[1006]:
    print "TEST -s"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n 1" % (PYEXT) )
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

if testsToRun[1007]:
    print "TEST -1"
    NMPDS = 2
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        temph = open(HFILE,'w')
        for host in clusterHosts: print >>temph, host
        temph.close()
        os.system("mpdboot%s -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
        expout = '%s\n' % (clusterHosts[0])
        rv = mpdtest.run(cmd="mpiexec%s -1 -n 1 /bin/hostname" % (PYEXT), expOut=expout,chkOut=1)
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.unlink(HFILE)

if testsToRun[1008]:
    print "TEST -ifhn"
    # not a particularly good test; you can hang/fail with an invalid ifhn
    # ifhn is not very useful for mpiexec since mpd can fill it in as needed
    NMPDS = 2
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        temph = open(HFILE,'w')
        for host in clusterHosts: print >>temph, host
        temph.close()
        os.system("mpdboot%s -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
        expout = 'hello\n'
        rv = mpdtest.run(cmd="mpiexec%s -ifhn 127.0.0.1 -n 1 /bin/echo hello" % (PYEXT),
                         expOut=expout,chkOut=1)
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.unlink(HFILE)

if testsToRun[1009]:
    print "TEST -n"
    NMPDS = 1
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
        expout = '0: hello\n1: bye\n'
        mpdtest.run(cmd="mpiexec%s -l -n 1 echo hello : -n 1 echo bye" % (PYEXT),chkOut=1,expOut=expout)
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[1010]:
    print "TEST -wdir"
    # not a particularly good test; you can hang/fail with an invalid ifhn
    # ifhn is not very useful for mpiexec since mpd can fill it in as needed
    NMPDS = 2
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        temph = open(HFILE,'w')
        for host in clusterHosts: print >>temph, host
        temph.close()
        os.system("mpdboot%s -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
        expout = '/tmp\n/tmp\n'
        rv = mpdtest.run(cmd="mpiexec%s -wdir /tmp -n 2 /bin/pwd" % (PYEXT),
                         expOut=expout,chkOut=1)
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.unlink(HFILE)

if testsToRun[1011]:
    print "TEST -path"
    # not a particularly good test; you can hang/fail with an invalid ifhn
    # ifhn is not very useful for mpiexec since mpd can fill it in as needed
    NMPDS = 2
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        temph = open(HFILE,'w')
        for host in clusterHosts: print >>temph, host
        temph.close()
        os.system("mpdboot%s -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
        expout = '/tmp:/bin\n/tmp:/bin\n'
        mpdtest.run(cmd="mpiexec%s -path /tmp:/bin -n 2 /usr/bin/printenv | grep PATH" % (PYEXT),
                    expOut=expout,chkOut=1)
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.unlink(HFILE)

if testsToRun[1012]:
    print "TEST -host"
    NMPDS = 5
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        temph = open(HFILE,'w')
        for host in clusterHosts: print >>temph, host
        temph.close()
        os.system("mpdboot%s -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
        expout = '%s\n' % clusterHosts[3]
        rv = mpdtest.run(cmd="mpiexec%s -n 1 -host %s /bin/hostname" % (PYEXT,clusterHosts[3]),
                         expOut=expout,chkOut=1)
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.unlink(HFILE)

if testsToRun[1013]:
    print "TEST -soft"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = 'hello\nhello\nhello\nhello\nhello\n'  # 5 times
    rv = mpdtest.run(cmd="mpiexec%s -n 9 -soft 1:5:2 /bin/echo hello" % (PYEXT),
                     expOut=expout,chkOut=1)
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[1014]:
    print "TEST -envall (the default)"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = 'BAR\n'
    os.environ['FOO'] = 'BAR'
    rv = mpdtest.run(cmd="mpiexec%s -n 1 -envall sh -c '/bin/echo $FOO'" % (PYEXT),
                     expOut=expout,chkOut=1)
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.unsetenv('FOO')   # get rid of this for later tests

if testsToRun[1015]:
    print "TEST -envnone"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = '\n'
    os.environ['FOO'] = ''
    rv = mpdtest.run(cmd="mpiexec%s -n 1 -envnone sh -c '/bin/echo $FOO'" % (PYEXT),
                     expOut=expout,chkOut=1)
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.unsetenv('FOO')   # get rid of this for later tests

if testsToRun[1016]:
    print "TEST -env"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = 'BAR\n'
    rv = mpdtest.run(cmd="mpiexec%s -n 1 -env FOO BAR sh -c '/bin/echo $FOO'" % (PYEXT),
                     expOut=expout,chkOut=1)
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[1017]:
    print "TEST -envlist"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    os.environ['FOO'] = 'BAR'
    os.environ['RMB'] = 'ZZZ'
    expout = 'BAR ZZZ\n'
    rv = mpdtest.run(cmd="mpiexec%s -n 1 -envlist FOO,RMB sh -c '/bin/echo $FOO $RMB'" % (PYEXT),
                     expOut=expout,chkOut=1)
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.unsetenv('FOO')   # get rid of this for later tests
    os.unsetenv('RMB')   # get rid of this for later tests

if testsToRun[1018]:
    print "TEST -gn"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = 'hello\nhello\nbye\nbye\n'
    rv = mpdtest.run(cmd="mpiexec%s -gn 2 /bin/echo hello : /bin/echo bye" % (PYEXT),
                     expOut=expout,chkOut=1)
    rv = mpdtest.run(cmd="mpiexec%s -gn 2 : /bin/echo hello : /bin/echo bye" % (PYEXT),
                     expOut=expout,chkOut=1)
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[1019]:
    print "TEST -gexec"
    NMPDS = 2
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        temph = open(HFILE,'w')
        for host in clusterHosts: print >>temph, host
        temph.close()
        os.system("mpdboot%s -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
        expout = '%s\n%s\n' % (socket.gethostname(),clusterHosts[0])
        rv = mpdtest.run(cmd="mpiexec%s -gexec hostname : -n 1 : -n 1" % (PYEXT),
                         expOut=expout,chkOut=1)
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.unlink(HFILE)

if testsToRun[1020]:
    print "TEST -genvlist"
    NMPDS = 2
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
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
        os.unlink(HFILE)
        os.unsetenv('FOO')   # get rid of this for later tests
        os.unsetenv('RMB')   # get rid of this for later tests


if 1 in testsToRun[2000:3000]:
    print "MPI tests-------------------------------------------"
    if not MPI_srcdir:
        print "    skipping all MPI tests because no MPI_srcidr is specified"
        for i in range(2000,3000): testsToRun[i] = 0

if testsToRun[2000]:
    print "TEST cpi"
    NMPDS = 1
    if not os.access("%s/examples/cpi" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/examples ; make cpi" % (MPI_srcdir) )
    mpdtest = MPDTest()
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = ['Process 0 of 3','Process 1 of 3','Process 2 of 3']
    rv = mpdtest.run(cmd="mpiexec%s -n 3 %s/examples/cpi" % (PYEXT,MPI_srcdir),
                     grepOut=1, expOut=expout )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[2001]:
    print "TEST spawn1"
    NMPDS = 1
    mpdtest = MPDTest()
    if not os.access("%s/test/mpi/spawn/spawn1" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/test/util ; make" % (MPI_srcdir) )
        os.system("cd %s/test/mpi/spawn ; make spawn1" % (MPI_srcdir) )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = ['No Errors']
    os.system('cp %s/test/mpi/spawn/spawn1 .' % (MPI_srcdir))
    rv = mpdtest.run(cmd="mpiexec%s -n 1 ./spawn1" % (PYEXT),  # -n 1
                     grepOut=1, expOut=expout )
    os.system('rm -f spawn1')
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[2002]:
    print "TEST spawn2"
    NMPDS = 1
    mpdtest = MPDTest()
    if not os.access("%s/test/mpi/spawn/spawn2" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/test/util ; make" % (MPI_srcdir) )
        os.system("cd %s/test/mpi/spawn ; make spawn2" % (MPI_srcdir) )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    os.system('cp %s/test/mpi/spawn/spawn2 .' % (MPI_srcdir))
    expout = ['No Errors']
    rv = mpdtest.run(cmd="mpiexec%s -n 1 ./spawn2" % (PYEXT),  # -n 1
                     grepOut=1, expOut=expout )
    os.system('rm -f spawn2')
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[2003]:
    print "TEST spawnmult2"
    NMPDS = 1
    mpdtest = MPDTest()
    if not os.access("%s/test/mpi/spawn/spawnmult2" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/test/util ; make" % (MPI_srcdir) )
        os.system("cd %s/test/mpi/spawn ; make spawnmult2" % (MPI_srcdir) )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    os.system('cp %s/test/mpi/spawn/spawnmult2 .' % (MPI_srcdir))
    expout = ['No Errors']
    rv = mpdtest.run(cmd="mpiexec%s -n 2 ./spawnmult2" % (PYEXT),  # -n 2
                     grepOut=1, expOut=expout )
    os.system('rm -f spawnmult2')
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    
if testsToRun[2004]:
    print "TEST spawnargv"
    NMPDS = 1
    mpdtest = MPDTest()
    if not os.access("%s/test/mpi/spawn/spawnargv" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/test/util ; make" % (MPI_srcdir) )
        os.system("cd %s/test/mpi/spawn ; make spawnargv" % (MPI_srcdir) )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    os.system('cp %s/test/mpi/spawn/spawnargv .' % (MPI_srcdir))
    expout = ['No Errors']
    rv = mpdtest.run(cmd="mpiexec%s -n 1 ./spawnargv" % (PYEXT),  # -n 2
                     grepOut=1, expOut=expout )
    os.system('rm -f spawnargv')
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[2005]:
    print "TEST spawnintra"
    NMPDS = 1
    mpdtest = MPDTest()
    if not os.access("%s/test/mpi/spawn/spawnintra" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/test/util ; make" % (MPI_srcdir) )
        os.system("cd %s/test/mpi/spawn ; make spawnintra" % (MPI_srcdir) )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    os.system('cp %s/test/mpi/spawn/spawnintra .' % (MPI_srcdir))
    expout = ['No Errors']
    rv = mpdtest.run(cmd="mpiexec%s -n 1 ./spawnintra" % (PYEXT),  # -n 2
                     grepOut=1, expOut=expout )
    os.system('rm -f spawnintra')
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    
if testsToRun[2006]:
    print "TEST namepub"
    NMPDS = 1
    mpdtest = MPDTest()
    if not os.access("%s/test/mpi/spawn/namepub" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/test/util ; make" % (MPI_srcdir) )
        os.system("cd %s/test/mpi/spawn ; make namepub" % (MPI_srcdir) )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = ['No Errors']
    os.system('cp %s/test/mpi/spawn/namepub .' % (MPI_srcdir))
    rv = mpdtest.run(cmd="mpiexec%s -n 1 ./namepub" % (PYEXT),  # -n 2
                     grepOut=1, expOut=expout )
    os.system('rm -f namepub')
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    
if testsToRun[2007]:
    print "TEST concurrent_spawns"
    NMPDS = 1
    mpdtest = MPDTest()
    if not os.access("%s/test/mpi/spawn/concurrent_spawns" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/test/util ; make" % (MPI_srcdir) )
        os.system("cd %s/test/mpi/spawn ; make concurrent_spawns" % (MPI_srcdir) )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = ['No Errors']
    os.system('cp %s/test/mpi/spawn/concurrent_spawns .' % (MPI_srcdir))
    rv = mpdtest.run(cmd="mpiexec%s -n 1 ./concurrent_spawns" % (PYEXT),  # -n 2
                     grepOut=1, expOut=expout )
    os.system('rm -f concurrent_spawns')
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[2008]:
    print "TEST singleton init (cpi)"
    NMPDS = 1
    mpdtest = MPDTest()
    if not os.access("%s/examples/cpi" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/examples ; make cpi" % (MPI_srcdir) )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    expout = ['Process 0 of 1','pi is approximately 3']
    rv = mpdtest.run(cmd="%s/examples/cpi" % (MPI_srcdir), grepOut=1, expOut=expout )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )

if testsToRun[2009]:
    print "TEST bnr (mpich1-compat using cpi)"
    if not MPI_1_dir:
        print "    skipping; no mpich1 dir specified"
    else:
        NMPDS = 1
        mpdtest = MPDTest()
        if not os.access("%s/examples/cpi" % (MPI_srcdir),os.R_OK):  
            os.system("cd %s/examples ; make cpi" % (MPI_srcdir) )
            os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
            os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
            expout = ['Process 0 on','Process 1 on','Process 2 on','pi is approximately 3']
            rv = mpdtest.run(cmd="mpiexec%s -bnr -n 3 %s/examples/cpi" % (PYEXT,MPI_1_dir),
                             grepOut=1, expOut=expout )
            os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )


if 1 in testsToRun[3000:4000]:
    print "console pgm tests---------------------------------------------------"

if testsToRun[3000]:
    print "TEST mpdtrace"
    NMPDS = 3
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
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
        os.unlink(HFILE)

if testsToRun[3001]:
    print "TEST mpdringtest"
    NMPDS = 3
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        temph = open(HFILE,'w')
        for host in clusterHosts: print >>temph, host
        temph.close()
        os.system("mpdboot%s -f %s -n %d" % (PYEXT,HFILE,NMPDS) )
        nLoops = 100
        expout = ['time for %d loops' % (nLoops) ]
        rv = mpdtest.run(cmd="mpdringtest%s %d" % (PYEXT,nLoops), grepOut=1, expOut=expout )
        os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
        os.unlink(HFILE)

if testsToRun[3002]:
    print "TEST mpdexit"
    NMPDS = 3
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
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
        os.unlink(HFILE)

if testsToRun[3003]:
    print "TEST mpdlistjobs and mpdkilljob"
    NMPDS = 1
    mpdtest = MPDTest()
    if not os.access("%s/src/pm/mpd/infloop" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/src/pm/mpd ; cc -o infloop infloop.c" % (MPI_srcdir) )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    if not os.access("./infloop",os.R_OK):  
       os.system('cp %s/src/pm/mpd/infloop .' % (MPI_srcdir))
    os.system("mpiexec%s -s all -a ralph -n 2 ./infloop -p &" % (PYEXT))  # -p => don't print
    import time    ## give the mpiexec
    time.sleep(2)  ##     time to get going
    expout = ['ralph']
    rv = mpdtest.run(cmd="mpdlistjobs%s" % (PYEXT), grepOut=1, expOut=expout )
    rv = mpdtest.run(cmd="mpdkilljob%s -a ralph" % (PYEXT), chkOut=0 )
    expout = ''
    rv = mpdtest.run(cmd="mpdlistjobs%s" % (PYEXT), chkOut=1, expOut=expout )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("killall -q infloop")  ## just to be safe
    os.system('rm -f infloop')
    
if testsToRun[3004]:
    print "TEST mpdsigjob"
    NMPDS = 1
    mpdtest = MPDTest()
    if not os.access("%s/src/pm/mpd/infloop" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/src/pm/mpd ; cc -o infloop infloop.c" % (MPI_srcdir) )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    if not os.access("./infloop",os.R_OK):  
        os.system('cp %s/src/pm/mpd/infloop .' % (MPI_srcdir))
    os.system("mpiexec%s -s all -a ralph -n 2 ./infloop -p &" % (PYEXT))  # -p => don't print
    import time    ## give the mpiexec
    time.sleep(2)  ##     time to get going
    expout = ['ralph']
    rv = mpdtest.run(cmd="mpdlistjobs%s" % (PYEXT), grepOut=1, expOut=expout )
    rv = mpdtest.run(cmd="mpdsigjob%s INT -a ralph -g" % (PYEXT), chkOut=0 )
    expout = ''
    rv = mpdtest.run(cmd="mpdlistjobs%s" % (PYEXT), chkOut=1, expOut=expout )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system('rm -f infloop')
    

if 1 in testsToRun[8000:9000]:
    print "misc tests---------------------------------------------------"

if testsToRun[8000]:
    print "TEST ^C to mpiexec"
    NMPDS = 1
    mpdtest = MPDTest()
    if not os.access("%s/src/pm/mpd/infloop" % (MPI_srcdir),os.R_OK):  
        os.system("cd %s/src/pm/mpd ; cc -o infloop infloop.c" % (MPI_srcdir) )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("mpdboot%s -n %d" % (PYEXT,NMPDS) )
    if not os.access("./infloop",os.R_OK):  
        os.system('cp %s/src/pm/mpd/infloop .' % (MPI_srcdir))
    import popen2
    runner = popen2.Popen4("mpiexec%s -n 2 ./infloop -p" % (PYEXT))  # -p => don't print
    import time    ## give the mpiexec
    time.sleep(2)  ##     time to get going
    os.system("kill -INT %d" % (runner.pid) )  # simulate user ^C
    expout = ''
    rv = mpdtest.run(cmd="mpdlistjobs%s" % (PYEXT), chkOut=1, expOut=expout )
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (PYEXT) )
    os.system("killall -q infloop")  ## just to be safe
    os.system('rm -f infloop')

if testsToRun[8001]:
    print "TEST re-knit a ring"
    NMPDS = 3
    if NMPDS > len(clusterHosts)+1:
        print "    skipping; too few hosts"
    else:
        mpdtest = MPDTest()
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

if testsToRun[8002]:
    print "TEST installing mpd as user"
    if not os.access("%s/src/pm/mpd/mpdroot" % (MPI_srcdir),os.X_OK):
        os.system("cd %s/src/pm/mpd ; ./configure ; make")
    os.system("cd %s/src/pm/mpd ; make prefix=%s install" % (MPI_srcdir,USER_INSTALL_DIR) )
    print "TEST the mpd installed as user ; run mpdtrace as user"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("%s/bin/mpdallexit%s 1> /dev/null 2> /dev/null" % (USER_INSTALL_DIR,PYEXT) )
    os.system("%s/bin/mpdboot%s -n %d" % (USER_INSTALL_DIR,PYEXT,NMPDS) )
    expout = ['%s' % (socket.gethostname())]
    rv = mpdtest.run(cmd="%s/bin/mpdtrace%s -l" % (USER_INSTALL_DIR,PYEXT), grepOut=1, expOut=expout )
    os.system("%s/bin/mpdallexit%s 1> /dev/null 2> /dev/null" % (USER_INSTALL_DIR,PYEXT) )


if 1 in testsToRun[9000:10000]:
    print "root tests---------------------------------------------------"
    import popen2
    runner = popen2.Popen4("sudo echo hello")
    sout_serr = runner.fromchild
    line = sout_serr.readline()
    if line.find('hello') < 0:
        print "    ** you do not have sudo capability; terminating\n"
        sys.exit(-1)

if testsToRun[9000]:
    os.environ['MPD_CON_EXT'] = ""    # get rid of this for root testing
    print "TEST installing mpd as root"
    if not os.access("%s/src/pm/mpd/mpdroot" % (MPI_srcdir),os.X_OK):
        os.system("cd %s/src/pm/mpd ; ./configure ; make")
    os.system("cd %s/src/pm/mpd ; sudo make prefix=%s install" % (MPI_srcdir,ROOT_INSTALL_DIR) )
    print "TEST mpd as root ; mpdtrace as user"
    NMPDS = 1
    mpdtest = MPDTest()
    os.system("sudo %s/bin/mpdallexit%s 1> /dev/null 2> /dev/null" % (ROOT_INSTALL_DIR,PYEXT) )
    os.system("sudo %s/bin/mpd%s -d" % (ROOT_INSTALL_DIR,PYEXT) )  # not using boot here
    import time
    time.sleep(2)
    os.environ['MPD_USE_ROOT_MPD'] = '1'
    # os.system("%s/bin/mpdtrace%s -l" % (ROOT_INSTALL_DIR,PYEXT))
    expout = ['%s' % (socket.gethostname())]
    rv = mpdtest.run(cmd="%s/bin/mpdtrace%s -l" % (ROOT_INSTALL_DIR,PYEXT), grepOut=1, expOut=expout )

    print "TEST that user cannot remove files owned by root"
    os.system("sudo touch /tmp/testroot")
    expout = ['cannot remove']
    rv = mpdtest.run(cmd="%s/bin/mpiexec%s -n 1 /bin/rm -f /tmp/testroot" % (ROOT_INSTALL_DIR,PYEXT),
                     grepOut=1, expOut=expout )
    os.system("sudo rm -f /tmp/testroot")
    os.system("sudo %s/bin/mpdallexit%s 1> /dev/null 2> /dev/null" % (ROOT_INSTALL_DIR,PYEXT) )
