#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage:  mpdboot --totalnum=<n_to_start> [--file=<hostsfile>]  [--help] \ 
                [--rsh=<rshcmd>] [--user=<user>] [--mpd=<mpdcmd>] \ 
                [--loccons] [--remcons] [--shell] [--verbose] [-1]
                [--ncpus=<ncpus>] [--ifhn=<ifhn>]
 or, in short form, 
        mpdboot -n n_to_start [-f <hostsfile>] [-h] [-r <rshcmd>] [-u <user>] \ 
                [-m <mpdcmd>]  -s -v [-1]

--totalnum specifies the total number of mpds to start; at least
  one mpd will be started locally, and others on the machines specified
  by the file argument; by default, only one mpd per host will be
  started even if the hostname occurs multiple times in the hosts file
-1 means remove the restriction of starting only one mpd per machine; 
  in this case, at most the first mpd on a host will have a console
--file specifies the file of machines to start the rest of the mpds on;
  it defaults to mpd.hosts
--mpd specifies the full path name of mpd on the remote hosts if it is
  not in your path
--rsh specifies the name of the command used to start remote mpds; it
  defaults to ssh; an alternative is rsh
--shell says that the Bourne shell is your default for rsh' 
--verbose shows the ssh attempts as they occur; it does not provide
  confirmation that the sshs were successful
--loccons says you do not want a console available on local mpd(s)
--remcons says you do not want consoles available on remote mpd(s)
--ncpus indicates how many cpus you want to show for the local machine;
  others are listed in the hosts file
--ifhn indicates the interface hostname to use for the local mpd; others
  may be specified in the hostsfile
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.1 $"
__credits__ = ""

import re

from os     import environ, system, path, kill, access, X_OK
from sys    import argv, exit, stdout
from popen2 import Popen4, popen2
from socket import gethostname, gethostbyname_ex
from select import select, error
from signal import SIGKILL
from mpdlib import mpd_set_my_id, mpd_get_my_username, mpd_raise, mpdError, \
                   mpd_same_ips, mpd_get_ranks_in_binary_tree, mpd_print, \
                   mpd_get_inet_socket_and_connect, mpd_send_one_msg, mpd_recv_one_msg

global myHost, fullDirName, topMPDBoot, user

def mpdboot():
    global myHost, fullDirName, topMPDBoot, user
    mpd_set_my_id('mpdboot_rank_notset')
    fullDirName  = path.abspath(path.split(argv[0])[0])
    rshCmd     = 'ssh'
    user       = mpd_get_my_username()
    mpdCmd     = path.join(fullDirName,'mpd.py')
    mpdbootCmd = path.join(fullDirName,'mpdboot.py')
    hostsFilename  = 'mpd.hosts'
    totalNum   = 1    # may get chgd below
    debug      = 0
    verbosity  = 0
    localConsoleArg  = ''
    remoteConsoleArg = ''
    myConsoleVal = ''
    oneMPDPerHost = 1
    entryHost = ''
    entryPort = ''
    topMPDBoot = 1
    myHost = gethostname()
    myNcpus = 1
    myIfhn = ''
    try:
        shell = path.split(environ['SHELL'])[-1]
    except:
        shell = 'csh'

    argidx = 1    # skip arg 0
    while argidx < len(argv):
        if   argv[argidx] == '-h' or argv[argidx] == '--help':
            usage()
        elif argv[argidx] == '-zentry':    # entry host and port
            if ':' not in argv[argidx+1]:
                print 'invalid pair of entry host and entry port for -zentry option'
                usage()
            (entryHost,entryPort) = argv[argidx+1].split(':')
            try:
                ip = gethostbyname_ex(entryHost)[2]    # may fail if invalid host
            except:
                print 'invalid entry host ', entryHost ; stdout.flush()
                usage()
            if not entryPort.isdigit():
                print 'invalid (nonumeric) entry port ', entryPort ; stdout.flush()
                usage()
            entryHost = entryHost
            entryPort = entryPort
            argidx += 2
        elif argv[argidx] == '-zrank':
            topMPDBoot = 0
            myBootRank = int(argv[argidx+1])
            argidx += 2
        elif argv[argidx] == '-zhosts':
	    zhosts = argv[argidx+1]
	    zhosts = zhosts.split(',')
            hostsAndInfo = []
            for zhost in zhosts:
                (host,ncpus,ifhn) = zhost.split(':')
                hostsAndInfo.append({'host':host,'ncpus':ncpus,'ifhn':ifhn})
            argidx += 2
        elif argv[argidx] == '-r':    # or --rsh=
            rshCmd = argv[argidx+1]
            argidx += 2
        elif argv[argidx].startswith('--rsh'):
            splitArg = argv[argidx].split('=')
            try:
                rshCmd = splitArg[1]
            except:
                print 'mpdboot: invalid argument:', argv[argidx]
                usage()
            argidx += 1
        elif argv[argidx] == '-u':    # or --user=
            user = argv[argidx+1]
            argidx += 2
        elif argv[argidx].startswith('--user'):
            splitArg = argv[argidx].split('=')
            try:
                user = splitArg[1]
            except:
                print 'mpdboot: invalid argument:', argv[argidx]
                usage()
            argidx += 1
        elif argv[argidx] == '-m':    # or --mpd=
            mpdCmd = argv[argidx+1]
            argidx += 2
        elif argv[argidx].startswith('--mpd'):
            splitArg = argv[argidx].split('=')
            try:
                mpdCmd = splitArg[1]
            except:
                print 'mpdboot: invalid argument:', argv[argidx]
                usage()
            argidx += 1
        elif argv[argidx] == '-f':    # or --file=
            hostsFilename = argv[argidx+1]
            argidx += 2
        elif argv[argidx].startswith('--file'):
            splitArg = argv[argidx].split('=')
            try:
                hostsFilename = splitArg[1]
            except:
                print 'mpdboot: invalid argument:', argv[argidx]
                usage()
            argidx += 1
        elif argv[argidx].startswith('--ncpus'):
            splitArg = argv[argidx].split('=')
            try:
                myNcpus = splitArg[1]
            except:
                print 'mpdboot: invalid argument:', argv[argidx]
                usage()
            argidx += 1
        elif argv[argidx].startswith('--ifhn'):
            splitArg = argv[argidx].split('=')
            myIfhn = splitArg[1]
            myHost = splitArg[1]
            argidx += 1
        elif argv[argidx] == '-n':    # or --totalnum=
            totalNum = int(argv[argidx+1])
            argidx += 2
        elif argv[argidx].startswith('--totalnum'):
            splitArg = argv[argidx].split('=')
            try:
                totalNum = int(splitArg[1])
            except:
                print 'mpdboot: invalid argument:', argv[argidx]
                usage()
            argidx += 1
        elif argv[argidx] == '-d' or argv[argidx] == '--debug':
            debug = 1
            argidx += 1
        elif argv[argidx] == '-s' or argv[argidx] == '--shell':
            shell = 'bourne'
            argidx += 1
        elif argv[argidx] == '-v' or argv[argidx] == '--verbose':
            verbosity = 1
            argidx += 1
        elif argv[argidx] == '-1':
            oneMPDPerHost = 0
            argidx += 1
        elif argv[argidx] == '--loccons':
            localConsoleArg  = '--loccons'
            argidx += 1
        elif argv[argidx] == '--remcons':
            remoteConsoleArg = '--remcons'
            argidx += 1
        else:
            print 'mpdboot: unrecognized argument:', argv[argidx]
            usage()

    if topMPDBoot:
        lines = []
        if totalNum > 1:
            try:
                f = open(hostsFilename,'r')
                for line in f:
                    lines.append(line)
            except:
                print 'unable to open (or read) hostsfile %s' % (hostsFilename)
                exit(-1)
        hostsAndInfo = [ {'host' : myHost, 'ncpus' : myNcpus, 'ifhn' : myIfhn} ]
        for line in lines:
            line = line.strip()
            if not line  or  line[0] == '#':
                continue
            splitLine = re.split(r'\s+',line)
            host = splitLine[0]
            ncpus = 1  # default
            if ':' in host:
                (host,ncpus) = host.split(':',1)
                ncpus = int(ncpus)
            ifhn = ''  # default
            for kv in splitLine[1:]:
                (k,v) = kv.split('=',1)
                if k == 'ifhn':
                    ifhn = v
            hostsAndInfo.append( {'host' : host, 'ncpus' : ncpus, 'ifhn' : ifhn} )
        if oneMPDPerHost  and  totalNum > 1:
	    oldHosts = hostsAndInfo[:]
	    hostsAndInfo = []
	    for x in oldHosts:
	       keep = 1
	       for y in hostsAndInfo:
	           if mpd_same_ips(x['host'],y['host']):
		       keep = 0
		       break
	       if keep:
	           hostsAndInfo.append(x)
        if len(hostsAndInfo) < totalNum:    # one is local
            print 'totalNum=%d  num hosts=%d' % (totalNum,len(hostsAndInfo))
            print 'there are not enough hosts on which to start all processes'
            exit(-1)
        myBootRank = 0
        if localConsoleArg:
            myConsoleVal = '-n'
    else:
        if remoteConsoleArg:
            myConsoleVal = '-n'
    anMPDalreadyHere = 0
    for i in range(myBootRank):
        if mpd_same_ips(hostsAndInfo[i]['host'],myHost):    # if one before me on this host
            myConsoleVal = '-n'
	    anMPDalreadyHere = 1
	    break
    if not anMPDalreadyHere:
        try:
            system('%s/mpdallexit.py > /dev/null' % (fullDirName))  # stop any current mpds
        except:
            pass

    mpd_set_my_id('mpdboot_%s_%d' % (myHost,myBootRank) )
    if debug:
        mpd_print(1, 'starting')
    (parent,lchild,rchild) = mpd_get_ranks_in_binary_tree(myBootRank,totalNum)
    if debug:
        mpd_print(1, 'p=%d l=%d r=%d' % (parent,lchild,rchild) )

    if myIfhn:
        ifhnVal = '--if %s' % (myIfhn)
    elif hostsAndInfo[myBootRank]['ifhn']:
        ifhnVal = '--if %s' % (hostsAndInfo[myBootRank]['ifhn'])
    else:
        ifhnVal = ''
    if entryHost:
        cmd = '%s %s -h %s -p %s -d -e --ncpus %s %s' % \
	      (mpdCmd,myConsoleVal,entryHost,entryPort,myNcpus,ifhnVal)
    else:
        cmd = '%s %s -d -e --ncpus %s %s' % \
	      (mpdCmd,myConsoleVal,myNcpus,ifhnVal)
    if verbosity:
        mpd_print(1,'starting local mpd on %s' % (myHost) )
    if debug:
        mpd_print(1, 'cmd to run local mpd = :%s:' % (cmd) )

    if not access(mpdCmd,X_OK):
        err_exit('cannot access mpd cmd :%s:' % (mpdCmd) )
    locMPD = Popen4(cmd, 0)
    locMPDFD = locMPD.fromchild
    locMPDPort = locMPDFD.readline().strip()
    if locMPDPort.isdigit():
	# can't do this until he's already in his ring
        locMPDSocket = mpd_get_inet_socket_and_connect(myHost,int(locMPDPort))
        if locMPDSocket:
            msgToSend = { 'cmd' : 'ping', 'host' : 'ping', 'port' : 0} # dummy host & port
            mpd_send_one_msg(locMPDSocket, { 'cmd' : 'ping', 'host' : myHost, 'port' : 0} )
            msg = mpd_recv_one_msg(locMPDSocket)    # RMB: WITH TIMEOUT ??
            if not msg  or  not msg.has_key('cmd')  or  msg['cmd'] != 'ping_ack':
                err_exit('%d: unable to ping local mpd; invalid msg from mpd :%s:' % (myBootRank,msg) )
            locMPDSocket.close()
        else:
            err_exit('failed to connect to mpd' )
    else:
        err_exit('%d: invalid port from mpd %s' % (myBootRank,str(locMPDPort)) )

    if not entryHost:
        entryHost = myHost
        entryPort = locMPDPort

    if rshCmd == 'ssh':
        xOpt = '-x'
    else:
        xOpt = ''

    lfd = 0
    rfd = 0
    fdsToSelect = []
    if debug:
        debugArg = '-d'
    else:
        debugArg = ''
    if verbosity:
        verboseArg = '-v'
    else:
        verboseArg = ''
    if lchild >= 0:
        zhosts = [ "%s:%s:%s" % (h['host'],h['ncpus'],h['ifhn']) for h in hostsAndInfo ]
	if hostsAndInfo[lchild]['ifhn']:
	    ifhnVal = '--ifhn=%s' % (hostsAndInfo[lchild]['ifhn'])
        else:
	    ifhnVal = ''
        cmd = "%s %s %s -n '%s --ncpus=%s %s -r %s -m %s -n %d %s %s %s -zentry %s:%s -zrank %s -zhosts %s </dev/null ' " % \
              (rshCmd, xOpt, hostsAndInfo[lchild]['host'], mpdbootCmd,
               hostsAndInfo[lchild]['ncpus'],ifhnVal,
	       rshCmd, mpdCmd, totalNum, debugArg, verboseArg, remoteConsoleArg, entryHost,
	       entryPort, lchild,
	       ','.join(zhosts) )
        if verbosity:
            mpd_print(1, 'starting remote mpd on %s' % (hostsAndInfo[lchild]) )
        if debug:
            mpd_print(1, 'cmd to run lchild boot = :%s:' % (cmd) )
        lchildMPDBoot = Popen4(cmd, 0)
        lfd = lchildMPDBoot.fromchild
        fdsToSelect.append(lfd)
    if rchild >= 0:
        zhosts = [ "%s:%s:%s" % (h['host'],h['ncpus'],h['ifhn']) for h in hostsAndInfo ]
	if hostsAndInfo[rchild]['ifhn']:
	    ifhnVal = '--ifhn=%s' % (hostsAndInfo[rchild]['ifhn'])
        else:
	    ifhnVal = ''
        cmd = "%s %s %s -n '%s --ncpus=%s %s -r %s -m %s -n %d %s %s %s -zentry %s:%s -zrank %s -zhosts %s </dev/null ' " % \
              (rshCmd, xOpt, hostsAndInfo[rchild]['host'], mpdbootCmd,
               hostsAndInfo[rchild]['ncpus'],ifhnVal,
	       rshCmd, mpdCmd, totalNum, debugArg, verboseArg, remoteConsoleArg, entryHost,
	       entryPort, rchild,
	       ','.join(zhosts) )
        if verbosity:
            mpd_print(1, 'starting remote mpd on %s' % (hostsAndInfo[rchild]) )
        if debug:
            mpd_print(1, 'cmd to run rchild boot = :%s:' % (cmd) )
        rchildMPDBoot = Popen4(cmd, 0)
        rfd = rchildMPDBoot.fromchild
        fdsToSelect.append(rfd)

    lfd_first_line = 1
    rfd_first_line = 1
    while fdsToSelect:
        try:
            (readyFDs,unused1,unused2) = select(fdsToSelect,[],[],0.1)
        except error, errmsg:
            mpd_raise('mpdboot: select failed: errmsg=:%s:' % (errmsg) )
        if lfd  and  lfd in readyFDs:
            line = lfd.readline()
            if line:
                if line.find('RC=MPDBOOT_ERREXIT') >= 0:
                    err_exit('RC=MPDBOOT_ERREXIT')
                else:
		    if not verbosity  and  lfd_first_line:
		        lfd_first_line = 0
			mpd_print(1,"error trying to start mpd(boot) at %d %s; output:" % (lchild,hostsAndInfo[lchild]))
                    print '  ', line, ; stdout.flush()
            else:
                lfd.close()
                fdsToSelect.remove(lfd)
        if rfd  and  rfd in readyFDs:
            line = rfd.readline()
            if line:
                if line.find('RC=MPDBOOT_ERREXIT') >= 0:
                    err_exit('RC=MPDBOOT_ERREXIT')
                else:
		    if not verbosity  and  rfd_first_line:
		        rfd_first_line = 0
			mpd_print(1,"error trying to start mpd(boot) at %d %s; output:" % (rchild,hostsAndInfo[rchild]))
                    print '  ', line, ; stdout.flush()
            else:
                rfd.close()
                fdsToSelect.remove(rfd)


def err_exit(msg):
    global myHost, fullDirName, topMPDBoot, user
    mpd_print(1, 'mpd failed to start correctly on %s' % (myHost) )
    mpdPid = 0
    if msg != 'RC=MPDBOOT_ERREXIT':
        print '  reason: %s' % (msg) ; stdout.flush()
        try:
            logfile = open('/tmp/mpd2.logfile_%s' % (user),'r')
            mpd_print(1, '  contents of mpd logfile in /tmp:')
            for line in logfile:
                print '    ', line, ; stdout.flush()
	        if line.startswith('logfile for mpd with pid'):
	            mpdPid = re.findall(r'logfile for mpd with pid (\d+)',line)
        except:
            pass
        try:
            system('%s/mpdallexit.py > /dev/null' % (fullDirName))  # stop any current mpds
        except:
             pass
        try:
            kill(mpdPid,SIGKILL)
        except:
             pass
        if not topMPDBoot:
            mpd_print(1, 'RC=MPDBOOT_ERREXIT')    # printable rc
    exit(-1)


def usage():
    print __doc__
    stdout.flush()
    exit(-1)

    
if __name__ == '__main__':
    try:
        mpdboot()
    except mpdError, errmsg:
        print 'mpdboot failed: %s' % (errmsg)
    ## RMB: I commented these 2 lines out as unnec (I think) on 10/5/04
    # except SystemExit, errmsg:
        # pass
