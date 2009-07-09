#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
mpdcheck

This script is a work in progress and may change frequently as we work
with users and gain additional insights into how to improve it.

This script prints useful information about the host on which it runs.
It is here to help us help users detect problems with configurations of
their computers.  For example, some computers are configured to think
of themselves simply as 'localhost' with 127.0.0.1 as the IP address.
This might present problems if a process on that computer wishes to
identify itself by host and port to a process on another computer.
The process on the other computer would try to contact 'localhost'.

If you are having problems running parallel jobs via mpd on one or more
hosts, you might try running this script once on each of those hosts.

Any output with *** at the beginning indicates a potential problem
that you may have to resolve before being able to run parallel jobs
via mpd.

For help:
    mpdcheck -h (or --help)
        prints this message

In the following modes, the -v (verbose) option provides info about what
mpdcheck is doing; the -l (long messages) option causes long informational
messages to print in situations where problems are spotted.

The three major modes of operation for this program are:

    mpdcheck
        looks for config problems on 'this' host; prints as nec

    mpdcheck -pc
        print config info about 'this' host, e.g. contents of /etc/hosts, etc.

    mpdcheck -f some_file [-ssh]
        prints info about 'this' host and locatability info about the ones
        listed in some_file as well (note the file might be mpd.hosts);
        the -ssh option can be used in conjunction with the -f option to
        cause ssh tests to be run to each remote host

    mpdcheck -s
        runs this program as a server on one host
    mpdcheck -c server_host server_port
        runs a client on another (or same) host; connects to the specifed
        host/port where you previously started the server
"""

# workaround to suppress deprecated module warnings in python2.6
# see https://trac.mcs.anl.gov/projects/mpich2/ticket/362 for tracking
import warnings
warnings.filterwarnings('ignore', '.*the popen2 module is deprecated.*', DeprecationWarning)

from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.19 $"
__credits__ = ""

import re

from  sys      import argv, exit, stdout
from  os       import path, kill, system
from  signal   import SIGKILL
from  socket   import gethostname, getfqdn, gethostbyname_ex, gethostbyaddr, socket
from  popen2   import Popen3
from  select   import select, error
from  commands import getoutput


if __name__ == '__main__':    # so I can be imported by pydoc
    do_ssh = 0
    fullDirName = path.abspath(path.split(argv[0])[0])  # normalize
    hostsFromFile = []
    verbose = 0
    long_messages = 0
    argidx = 1
    while argidx < len(argv):
        if argv[argidx] == '-h'  or argv[argidx] == '--help':
            print __doc__
            exit(0)
        elif argv[argidx] == '-s':
            lsock = socket()
            lsock.bind(('',0)) # anonymous port
            lsock.listen(5)
            print "server listening at INADDR_ANY on: %s %s" % (gethostname(),lsock.getsockname()[1])
            stdout.flush()
            (tsock,taddr) = lsock.accept()
            print "server has conn on %s from %s" % (tsock,taddr)
            msg = tsock.recv(64)
            if not msg:
                print "*** server failed to recv msg from client"
            else:
                print "server successfully recvd msg from client: %s" % (msg)
            tsock.sendall('ack_from_server_to_client')
            tsock.close()
            lsock.close()
            exit(0)
        elif argv[argidx] == '-c':
            sock = socket()
            sock.connect((argv[argidx+1],int(argv[argidx+2])))  # note double parens
            sock.sendall('hello_from_client_to_server')
            msg = sock.recv(64)
            if not msg:
                print "*** client failed to recv ack from server"
            else:
                print "client successfully recvd ack from server: %s" % (msg)
                stdout.flush()
            sock.close()
            exit(0)
        elif argv[argidx] == '-pc':
            print "--- print results of: gethostbyname_ex(gethostname())"
            print gethostbyname_ex(gethostname())
            print "--- try to run /bin/hostname"
            linesAsStr = getoutput("/bin/hostname")
            print linesAsStr
            print "--- try to run uname -a"
            linesAsStr = getoutput("/bin/uname -a")
            print linesAsStr
            print "--- try to print /etc/hosts"
            linesAsStr = getoutput("/bin/cat /etc/hosts")
            print linesAsStr
            print "--- try to print /etc/resolv.conf"
            linesAsStr = getoutput("/bin/cat /etc/resolv.conf")
            print linesAsStr
            print "--- try to run /sbin/ifconfig -a"
            linesAsStr = getoutput("/sbin/ifconfig -a")
            print linesAsStr
            print "--- try to print /etc/nsswitch.conf"
            linesAsStr = getoutput("/bin/cat /etc/nsswitch.conf")
            print linesAsStr
            exit(0)
        elif argv[argidx] == '-v':
            verbose = 1
            argidx += 1
        elif argv[argidx] == '-l':
            long_messages = 1
            argidx += 1
        elif argv[argidx] == '-f':
            try:
                hostsFile = open(argv[argidx+1])
            except:
                print 'unable to open file ', argv[argidx+1]
                exit(-1)
            for line in hostsFile:
                line = line.rstrip()
                if not line  or  line[0] == '#':
                    continue
                splitLine = re.split(r'\s+',line)
                host = splitLine[0]
                if ':' in host:
                    (host,ncpus) = host.split(':')
                hostsFromFile.append(host)
            argidx += 2
        elif argv[argidx] == '-ssh':
            do_ssh = 1
            argidx += 1
        else:
            print 'unrecognized arg:', argv[argidx]
            exit(0)
    
    
    # See if we can do gethostXXX, etc. for this host
    if verbose:
        print 'obtaining hostname via gethostname and getfqdn'
    uqhn1 = gethostname()
    fqhn1 = getfqdn()
    if verbose:
        print "gethostname gives ", uqhn1
        print "getfqdn gives ", fqhn1
    if verbose:
        print 'checking out unqualified hostname; make sure is not "localhost", etc.'
    if uqhn1.startswith('localhost'):
        if long_messages:
            msg = """
            **********
            The unqualified hostname seems to be localhost. This generally
            means that the machine's hostname is not set. You may change
            it by using the 'hostname' command, e.g.:
                hostname mybox1
            However, this will not remain after a reboot. To do this, you
            will need to consult the operating system's documentation. On
            Debian Linux systems, this can be done by:
                echo "mybox1" > /etc/hostname
            **********
            """
        else:
            msg = "*** the uq hostname seems to be localhost"
        print msg.strip().replace('        ','')
    elif uqhn1 == '':
        if long_messages:
            msg = """
            **********
            The unqualified hostname seems to be blank. This generally
            means that the machine's hostname is not set. You may change
            it by using the 'hostname' command, e.g.:
                hostname mybox1
            However, this will not remain after a reboot. To do this, you
            will need to consult the operating system's documentation. On
            Debian Linux systems, this can be done by:
                echo "mybox1" > /etc/hostname
            **********
            """
        else:
            msg = "*** the uq hostname seems to be localhost"
        print msg.replace('        ','')
    if verbose:
        print 'checking out qualified hostname; make sure is not "localhost", etc.'
    if fqhn1.startswith('localhost'):
        if long_messages:
            msg = """
            **********
            Your fully qualified hostname seems to be set to 'localhost'.
            This generally means that your machine's /etc/hosts file contains a line
            similar to this:
                127.0.0.1 mybox1 localhost.localdomain localhost
            You probably want to remove your hostname from this line and place it on
            a line by itself with your ipaddress, like this:
                $ipaddr mybox1
            **********
            """
        else:
            msg =  "*** the fq hostname seems to be localhost"
        print msg.rstrip().replace('        ','')
    elif fqhn1 == '':
        if long_messages:
            msg = """
            **********
            Your fully qualified hostname seems to be blank.
            **********
            """
        else:
            msg = "*** the fq hostname is blank"
        print msg.replace('        ','')
    
    if verbose:
        print 'obtain IP addrs via qualified and unqualified hostnames;',
        print ' make sure other than 127.0.0.1'
    uipaddr1 = 0
    try:
        ghbnu = gethostbyname_ex(uqhn1)
        if verbose:
            print "gethostbyname_ex: ", ghbnu
        uipaddr1 = ghbnu[2][0]
        if uipaddr1.startswith('127'):
            if long_messages:
                msg = """
                **********
                Your unqualified hostname resolves to 127.0.0.1, which is
                the IP address reserved for localhost. This likely means that
                you have a line similar to this one in your /etc/hosts file:
                127.0.0.1   $uqhn
                This should perhaps be changed to the following:
                127.0.0.1   localhost.localdomain localhost
                **********
                """
            else:
                msg = "*** first ipaddr for this host (via %s) is: %s" % (uqhn1,uipaddr1)
            print msg.replace('            ','')
        try:
            ghbau = gethostbyaddr(uipaddr1)
        except:
            print "*** gethostbyaddr failed for this hosts's IP %s" % (uipaddr1)
    except:
        if long_messages:
            msg = """
            **********
            The system call gethostbyname(3) failed to resolve your
            unqualified hostname, or $uqhn. This can be caused by
            missing info from your /etc/hosts file or your system not
            having correctly configured name resolvers, or by your IP 
            address not existing in resolution services.
            If you run DNS, you may wish to make sure that your
            DNS server has the correct forward A set up for your machine's
            hostname. If you are not using DNS and are only using hosts
            files, please check that a line similar to the one below exists
            in your /etc/hosts file:
                $ipaddr $uqdn
            If you plan to use DNS but you are not sure that it is
            correctly configured, please check that the file /etc/resolv.conf
            contains entries similar to the following:
                nameserver 1.2.3.4
            where 1.2.3.4 is an actual IP of one of your nameservers.
            **********
            """
        else:
            msg = "*** gethostbyname_ex failed for this host %s" % (uqhn1)
        print msg.replace('        ','')
    
    fipaddr1 = 0
    try:
        ghbnf = gethostbyname_ex(fqhn1)
        if verbose:
            print "gethostbyname_ex: ", ghbnf
        fipaddr1 = ghbnf[2][0]
        if fipaddr1.startswith('127'):
            msg = """
            **********
            Your fully qualified hostname resolves to 127.0.0.1, which
            is the IP address reserved for localhost. This likely means
            that you have a line similar to this one in your /etc/hosts file:
                 127.0.0.1   $fqhn
            This should be perhaps changed to the following:
                 127.0.0.1   localhost.localdomain localhost
            **********
            """
        try:
            ghbaf = gethostbyaddr(fipaddr1)
        except:
            print "*** gethostbyaddr failed for this hosts's IP %s" % (uipaddr1)
    except:
        if long_messages:
            msg = """
            **********
            The system call gethostbyname(3) failed to resolve your
            fully qualified hostname, or $fqhn. This can be caused by
            missing info from your /etc/hosts file or your system not
            having correctly configured name resolvers, or by your IP 
            address not existing in resolution services.
            If you run DNS, please check and make sure that your
            DNS server has the correct forward A record set up for your
            machine's hostname. If you are not using DNS and are only using
            hosts files, please check that a line similar to the one below
            exists in your /etc/hosts file:
                $ipaddr $fqhn
            If you intend to use DNS but you are not sure that it is
            correctly configured, please check that the file /etc/resolv.conf
            contains entries similar to the following:
                nameserver 1.2.3.4
            where 1.2.3.4 is an actual IP of one of your nameservers.
            **********
            """
        else:
            msg = "*** gethostbyname_ex failed for host %s" % (fqhn1)
        print msg.replace('        ','')
    
    if verbose:
        print 'checking that IP addrs resolve to same host'
    if uipaddr1 and fipaddr1 and uipaddr1 != fipaddr1:
        msg = """
            **********
            Your fully qualified and unqualified names do not resolve to
            the same IP. This likely means that your DNS domain name is not
            set correctly.  This might be fixed by adding a line similar
            to the following to your /etc/hosts:
                 $ipaddr             $fqhn   $uqdn
            **********
            """
        print msg.replace('        ','')
    
    
    if verbose:
        print 'now do some gethostbyaddr and gethostbyname_ex for machines in hosts file'
    # See if we can do gethostXXX, etc. for hosts in hostsFromFile
    for host in hostsFromFile:
        uqhn2 = host
        fqhn2 = getfqdn(uqhn2)
        uipaddr2 = 0
        if verbose:
            print 'checking gethostbyXXX for unqualified %s' % (uqhn2)
        try:
            ghbnu = gethostbyname_ex(uqhn2)
            if verbose:
                print "gethostbyname_ex: ", ghbnu
            uipaddr2 = ghbnu[2][0]
            try:
                ghbau = gethostbyaddr(uipaddr2)
            except:
                print "*** gethostbyaddr failed for remote hosts's IP %s" % (fipaddr2)
        except:
            print "*** gethostbyname_ex failed for host %s" % (fqhn2)
        if verbose:
            print 'checking gethostbyXXX for qualified %s' % (uqhn2)
        try:
            ghbnf = gethostbyname_ex(fqhn2)
            if verbose:
                print "gethostbyname_ex: ", ghbnf
            fipaddr2 = ghbnf[2][0]
            if uipaddr2  and  fipaddr2 != uipaddr2:
                print "*** ipaddr via uqn (%s) does not match via fqn (%s)" % (uipaddr2,fipaddr2)
            try:
                ghbaf = gethostbyaddr(fipaddr2)
            except:
                print "*** gethostbyaddr failed for remote hosts's IP %s" % (fipaddr2)
        except:
            print "*** gethostbyname_ex failed for host %s" % (fqhn2)
    
    
    # see if we can run /bin/date on remote hosts
    if not do_ssh:
        exit(0)
    
    for host in hostsFromFile:
        cmd = "ssh %s -x -n /bin/echo hello" % (host)
        if verbose:
            print 'trying: %s' % (cmd)
        runner = Popen3(cmd,1,0)
        runout = runner.fromchild
        runerr = runner.childerr
        runin  = runner.tochild
        runpid = runner.pid
        try:
            (readyFDs,unused1,unused2) = select([runout],[],[],9)
        except Exception, data:
            print 'select 1 error: %s ; %s' % ( data.__class__, data)
            exit(-1)
        if len(readyFDs) == 0:
            print '** ssh timed out to %s' % (host)
        line = ''
        failed = 0
        if runout in readyFDs:
            line = runout.readline()
            if not line.startswith('hello'):
                failed = 1
        else:
            failed = 1
        if failed:
            print '** ssh failed to %s' % (host)
            print '** here is the output:'
            if line:
                print line,
            done = 0
            fds = [runout,runerr]
            while not done:
                try:
                    (readyFDs,unused1,unused2) = select(fds,[],[],1)
                except Exception, data:
                    print 'select 2 error: %s ; %s' % ( data.__class__, data)
                    exit(-1)
                if runout in readyFDs:
                    line = runout.readline()
                    if line:
                        print line,
                    else:
                        fds.remove(runout)
                elif runerr in readyFDs:
                    line = runerr.readline()
                    if line:
                        print line,
                    else:
                        fds.remove(runerr)
                else:
                    done = 1
        try:
            kill(runpid,SIGKILL)
            runout.close()
            runerr.close()
            runin.close()
        except:
            pass
        if failed:
            exit(-1)
    
    # see if we can run mpdcheck on remote hosts
    for host in hostsFromFile:
        cmd1 = path.join(fullDirName,'mpdcheck.py') + ' -s'
        if verbose:
            print 'starting server: %s' % (cmd1)
        runner1 = Popen3(cmd1,1,0)
        runout1 = runner1.fromchild
        runerr1 = runner1.childerr
        runin1  = runner1.tochild
        runpid1 = runner1.pid
        try:
            (readyFDs,unused1,unused2) = select([runout1],[],[],9)
        except Exception, data:
            print 'select 3 error: %s ; %s' % ( data.__class__, data)
            exit(-1)
        if len(readyFDs) == 0:
            print '** timed out waiting for local server to produce output'
        line = ''
        failed = 0
        port = 0
        if runout1 in readyFDs:
            line = runout1.readline()
            if line.startswith('server listening at '):
                port = line.rstrip().split(' ')[-1]
            else:
                failed = 1
        else:
            failed = 1
        if failed:
            print 'could not start mpdcheck server'
            print 'here is the output:'
            if line:
                print line,
            done = 0
            fds = [runout1,runerr1]
            while not done:
                try:
                    (readyFDs,unused1,unused2) = select(fds,[],[],1)
                except Exception, data:
                    print 'select 4 error: %s ; %s' % ( data.__class__, data)
                    exit(-1)
                if runout in readyFDs:
                    line = runout.readline()
                    if line:
                        print line,
                    else:
                        fds.remove(runout)
                elif runerr in readyFDs:
                    line = runerr.readline()
                    if line:
                        print line,
                    else:
                        fds.remove(runerr)
                else:
                    done = 1
        if failed:
            try:
                kill(runpid1,SIGKILL)
            except:
                pass
            exit(-1)
        cmd2 = "ssh %s -x -n %s%smpdcheck.py -c %s %s" % (host,fullDirName,path.sep,fqhn1,port)
        if verbose:
            print 'starting client: %s' % (cmd2)
        runner2 = Popen3(cmd2,1,0)
        runout2 = runner2.fromchild
        runerr2 = runner2.childerr
        runin2  = runner2.tochild
        runpid2 = runner2.pid
        try:
            (readyFDs,unused1,unused2) = select([runout2],[],[],9)
        except Exception, data:
            print 'select 3 error: %s ; %s' % ( data.__class__, data)
            exit(-1)
        if len(readyFDs) == 0:
            print '** timed out waiting for client on %s to produce output' % (host)
        line = ''
        failed = 0
        port = 0
        if runout2 in readyFDs:
            line = runout2.readline()
            if not line.startswith('client successfully recvd'):
                failed = 1
        else:
            failed = 1
        if failed:
            print 'client on %s failed to access the server' % (host)
            print 'here is the output:'
            if line:
                print line,
            done = 0
            fds = [runout2,runerr2]
            while not done:
                try:
                    (readyFDs,unused1,unused2) = select(fds,[],[],1)
                except Exception, data:
                    print 'select 4 error: %s ; %s' % ( data.__class__, data)
                    exit(-1)
                if runout2 in readyFDs:
                    line = runout2.readline()
                    if line:
                        print line,
                    else:
                        fds.remove(runout2)
                elif runerr2 in readyFDs:
                    line = runerr2.readline()
                    if line:
                        print line,
                    else:
                        fds.remove(runerr2)
                else:
                    done = 1
        try:
            kill(runpid2,SIGKILL)
        except:
            pass
        if failed:
            try:
                kill(runpid1,SIGKILL)
            except:
                pass
            exit(-1)
