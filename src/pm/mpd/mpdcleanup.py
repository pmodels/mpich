#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

## NOTE: we do NOT allow this pgm to run via mpdroot

"""
usage: mpdcleanup [-h] [-v] [-f <hostsfile>] [-r <rshcmd>] [-u <user>] [-c <cleancmd>] [-k 'killcmd'] [-n <num_from_hostsfile>]
   or: mpdcleanup [--help] [--verbose] [--file=<hostsfile>] [--rsh=<rshcmd>] [--user=<user>]
                  [--clean=<cleancmd>] [--kill="killcmd"]
Removes the Unix socket on local (the default) and remote machines
This is useful in case the mpd crashed badly and did not remove it, which it normally does
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.11 $"
__credits__ = ""


import sys, os, socket

from getopt import getopt
from mpdlib import mpd_get_my_username, mpd_same_ips, mpd_set_tmpdir

def mpdcleanup():
    rshCmd    = 'ssh'
    user      = mpd_get_my_username()
    killCmd   = ''  # perhaps '~/bin/kj mpd'  (in quotes)
    cleanCmd  = 'rm -f '
    hostsFile = ''
    verbose = 0
    numFromHostsFile = 0  # chgd below
    try:
	(opts, args) = getopt(sys.argv[1:], 'hvf:r:u:c:k:n:',
                              ['help', 'verbose', 'file=', 'rsh=', 'user=', 'clean=','kill='])
    except:
        print 'invalid arg(s) specified'
	usage()
    else:
	for opt in opts:
	    if opt[0] == '-r' or opt[0] == '--rsh':
		rshCmd = opt[1]
	    elif opt[0] == '-u' or opt[0] == '--user':
		user   = opt[1]
	    elif opt[0] == '-f' or opt[0] == '--file':
		hostsFile = opt[1]
	    elif opt[0] == '-h' or opt[0] == '--help':
		usage()
	    elif opt[0] == '-v' or opt[0] == '--verbose':
		verbose = 1
	    elif opt[0] == '-n':
		numFromHostsFile = int(opt[1])
	    elif opt[0] == '-c' or opt[0] == '--clean':
		cleanCmd = opt[1]
	    elif opt[0] == '-k' or opt[0] == '--kill':
		killCmd = opt[1]
    if args:
        print 'invalid arg(s) specified: ' + ' '.join(args)
	usage()

    if os.environ.has_key('MPD_CON_EXT'):
        conExt = '_' + os.environ['MPD_CON_EXT']
    else:
        conExt = ''
    if os.environ.has_key('MPD_TMPDIR'):
        tmpdir = os.environ['MPD_TMPDIR']
    else:
        tmpdir = '/tmp'
    cleanFile = tmpdir + '/mpd2.console_' + user + conExt
    if rshCmd == 'ssh':
	xOpt = '-x'
    else:
	xOpt = ''
    try: localIP = socket.gethostbyname_ex(socket.gethostname())[2]
    except: localIP = 'unknownlocal'

    if hostsFile:
        try:
	    f = open(hostsFile,'r')
        except:
	    print 'Not cleaning up on remote hosts; file %s not found' % hostsFile
	    sys.exit(0)
        hosts = f.readlines()
        if numFromHostsFile:
            hosts = hosts[0:numFromHostsFile]
        for host in hosts:
	    host = host.strip()
	    if host[0] != '#':
                try: remoteIP = socket.gethostbyname_ex(host)[2]
                except: remoteIP = 'unknownremote'
                if localIP == remoteIP:  # local machine handled last below loop
                    continue
	        cmd = '%s %s -n %s %s %s &' % (rshCmd, xOpt, host, cleanCmd, cleanFile)
                if verbose:
	            print 'cmd=:%s:' % (cmd)
	        os.system(cmd)
                if killCmd:
	            cmd = "%s %s -n %s \"/bin/sh -c '%s' &\"" % (rshCmd, xOpt, host, killCmd)
                    if verbose:
	                print "cmd=:%s:" % (cmd)
	            os.system(cmd)

    ## clean up local machine last
    cmd = '%s %s' % (cleanCmd,cleanFile)
    if verbose:
        print 'cmd=:%s:' % (cmd)
    os.system(cmd)
    if killCmd:
        if verbose:
            print 'cmd=:%s:' % (killCmd)
        os.system(killCmd)

def usage():
    print __doc__
    sys.exit(-1)


if __name__ == '__main__':
    mpdcleanup()
