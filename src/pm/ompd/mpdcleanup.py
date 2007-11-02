#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

## NOTE: we do NOT allow this pgm to run via mpdroot

"""
usage: mpdcleanup', '[-f <hostsfile>] [-r <rshcmd>] [-u <user>] [-c <cleancmd>] or
   or: mpdcleanup', '[--file=<hostsfile>] [--rsh=<rshcmd>] [-user=<user>] [-clean=<cleancmd>]
Removes the Unix socket on local (the default) and remote machines
This is useful in case the mpd crashed badly and did not remove it, which it normally does
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.2 $"
__credits__ = ""


from sys    import argv, exit
from os     import environ, system
from getopt import getopt
from mpdlib import mpd_set_my_id, mpd_get_my_username, mpd_raise, mpdError

def mpdcleanup():
    rshCmd    = 'ssh'
    user      = mpd_get_my_username()
    cleanCmd  = 'rm -f '
    hostsFile = ''
    try:
	(opts, args) = getopt(argv[1:], 'hf:r:u:c:', ['help', 'file=', 'rsh=', 'user=', 'clean='])
    except:
	usage()
        mpd_raise('invalid arg(s) specified')
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
	    elif opt[0] == '-c' or opt[0] == '--clean':
		cleanCmd = opt[1]
    if args:
	usage()
        mpd_raise('invalid arg(s) specified: ' + ' '.join(args) )

    if environ.has_key('MPD_CON_EXT'):
        conExt = '_' + environ['MPD_CON_EXT']
    else:
        conExt = ''
    cleanFile = '/tmp/mpd2.console_' + user + conExt
    system( '%s %s' % (cleanCmd,cleanFile) )
    if rshCmd == 'ssh':
	xOpt = '-x'
    else:
	xOpt = ''

    if hostsFile:
        try:
	    f = open(hostsFile,'r')
        except:
	    print 'Not cleaning up on remote hosts; file %s not found' % hostsFile
	    exit(0)
        hosts  = f.readlines()
        for host in hosts:
	    host = host.strip()
	    if host[0] != '#':
	        cmd = '%s %s -n %s %s %s &' % (rshCmd, xOpt, host, cleanCmd, cleanFile)
	        # print 'cmd=:%s:' % (cmd)
	        system(cmd)

def usage():
    print __doc__
    exit(-1)


if __name__ == '__main__':
    try:
        mpdcleanup()
    except SystemExit, errmsg:
        pass
    except mpdError, errmsg:
	print 'mpdcleanup failed: %s' % (errmsg)
