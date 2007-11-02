#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#
"""
usage: mpdsigjob  sigtype  [-j jobid OR -a jobalias] [-s|g]
    sigtype must be the first arg
    jobid can be obtained via mpdlistjobs and is of the form:
        jobnum@mpdid where mpdid is mpd where first process runs, e.g.:
            1@linux02_32996 (may need \@ in some shells)
            1  is sufficient if you are on the machine where the job started
    one of -j or -a must be specified (but only one)
    -s or -g specify whether to signal the single user process or its group (default is g)
Delivers a Unix signal to all the application processes in the job
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.21 $"
__credits__ = ""


from os     import environ, getuid, close, path
from sys    import argv, exit
from socket import socket, fromfd, AF_UNIX, SOCK_STREAM
from signal import signal, SIG_DFL, SIGINT, SIGTSTP, SIGCONT, SIGALRM
from  mpdlib  import  mpd_set_my_id, mpd_uncaught_except_tb, mpd_print, \
                      mpd_handle_signal, mpd_get_my_username, MPDConClientSock, MPDParmDB

def mpdsigjob():
    import sys    # to get access to excepthook in next line
    sys.excepthook = mpd_uncaught_except_tb
    if len(argv) < 3  or  argv[1] == '-h'  or  argv[1] == '--help':
        usage()
    signal(SIGINT, sig_handler)
    mpd_set_my_id(myid='mpdsigjob')
    sigtype = argv[1]
    if sigtype.startswith('-'):
        sigtype = sigtype[1:]
    if sigtype.startswith('SIG'):
        sigtype = sigtype[3:]
    import signal as tmpsig  # just to get valid SIG's
    if sigtype.isdigit():
        if int(sigtype) > tmpsig.NSIG:
            print 'invalid signum: %s' % (sigtype)
            exit(-1)
    else:
	if not tmpsig.__dict__.has_key('SIG' + sigtype):
	    print 'invalid sig type: %s' % (sigtype)
	    exit(-1)
    jobalias = ''
    jobnum = ''
    mpdid = ''
    single_or_group = 'g'
    i = 2
    while i < len(argv):
        if argv[i] == '-a':
            if jobnum:      # should not have both alias and jobid
                print '** cannot specify both jobalias and jobid'
                usage()
            jobalias = argv[i+1]
            i += 1
            jobnum = '0'
        elif argv[i] == '-j':
            if jobalias:    # should not have both alias and jobid
                print '** cannot specify both jobalias and jobid'
                usage()
            jobid = argv[i+1]
            i += 1
            sjobid = jobid.split('@')
            jobnum = sjobid[0]
            if len(sjobid) > 1:
                mpdid = sjobid[1]
        elif argv[i] == '-s':
            single_or_group = 's'
        elif argv[i] == '-g':
            single_or_group = 'g'
        else:
            print '** unrecognized arg: %s' % (argv[i])
            usage()
        i += 1

    parmdb = MPDParmDB(orderedSources=['cmdline','xml','env','rcfile','thispgm'])
    parmsToOverride = {
                        'MPD_USE_ROOT_MPD'            :  0,
                        'MPD_SECRETWORD'              :  '',
                      }
    for (k,v) in parmsToOverride.items():
        parmdb[('thispgm',k)] = v
    parmdb.get_parms_from_env(parmsToOverride)
    parmdb.get_parms_from_rcfile(parmsToOverride)
    if getuid() == 0  or  parmdb['MPD_USE_ROOT_MPD']:
        fullDirName = path.abspath(path.split(argv[0])[0])  # normalize
        mpdroot = path.join(fullDirName,'mpdroot')
        conSock = MPDConClientSock(mpdroot=mpdroot,secretword=parmdb['MPD_SECRETWORD'])
    else:
        conSock = MPDConClientSock(secretword=parmdb['MPD_SECRETWORD'])

    msgToSend = {'cmd' : 'mpdsigjob', 'sigtype': sigtype, 'jobnum' : jobnum,
                 'mpdid' : mpdid, 'jobalias' : jobalias, 's_or_g' : single_or_group,
                 'username' : mpd_get_my_username() }
    conSock.send_dict_msg(msgToSend)
    msg = conSock.recv_dict_msg(timeout=5.0)
    if not msg:
        mpd_print(1,'no msg recvd from mpd before timeout')
    if msg['cmd'] != 'mpdsigjob_ack':
        if msg['cmd'] == 'already_have_a_console':
            mpd_print(1,'mpd already has a console (e.g. for long ringtest); try later')
        else:
            mpd_print(1,'unexpected message from mpd: %s' % (msg) )
        exit(-1)
    if not msg['handled']:
        print 'job not found'
        exit(-1)
    conSock.close()

def sig_handler(signum,frame):
    mpd_handle_signal(signum,frame)  # not nec since I exit next
    exit(-1)

def usage():
    print __doc__
    exit(-1)


if __name__ == '__main__':
    mpdsigjob()
