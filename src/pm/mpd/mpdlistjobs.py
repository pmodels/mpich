#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdlistjobs [-u | --user=username] [-a | --alias=jobalias] [-j | --jobid=jobid]
    (only use one of jobalias or jobid)'
lists jobs being run by an mpd ring, all by default, or filtered'
    by user, mpd job id, or alias assigned when the job was submitted'
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.21 $"
__credits__ = ""


import sys, os, signal, socket

from  mpdlib  import  mpd_set_my_id, mpd_uncaught_except_tb, mpd_print, \
                      mpd_handle_signal, mpd_get_my_username, MPDConClientSock, MPDParmDB

def mpdlistjobs():
    import sys    # to get access to excepthook in next line
    sys.excepthook = mpd_uncaught_except_tb
    signal.signal(signal.SIGINT, sig_handler)
    mpd_set_my_id(myid='mpdlistjobs')
    uname    = ''
    jobid    = ''
    sjobid   = ''
    jobalias = ''
    sssPrintFormat = 0
    if len(sys.argv) > 1:
        aidx = 1
        while aidx < len(sys.argv):
            if sys.argv[aidx] == '-h'  or  sys.argv[aidx] == '--help':
                usage()
            if sys.argv[aidx] == '-u':    # or --user=
                uname = sys.argv[aidx+1]
                aidx += 2
            elif sys.argv[aidx].startswith('--user'):
                splitArg = sys.argv[aidx].split('=')
                try:
                    uname = splitArg[1]
                except:
                    print 'mpdlistjobs: invalid argument:', sys.argv[aidx]
                    usage()
                aidx += 1
            elif sys.argv[aidx] == '-j':    # or --jobid=
                jobid = sys.argv[aidx+1]
                aidx += 2
                sjobid = jobid.split('@')    # jobnum and originating host
            elif sys.argv[aidx].startswith('--jobid'):
                splitArg = sys.argv[aidx].split('=')
                try:
                    jobid = splitArg[1]
                    sjobid = jobid.split('@')    # jobnum and originating host
                except:
                    print 'mpdlistjobs: invalid argument:', sys.argv[aidx]
                    usage()
                aidx += 1
            elif sys.argv[aidx] == '-a':    # or --alias=
                jobalias = sys.argv[aidx+1]
                aidx += 2
            elif sys.argv[aidx].startswith('--alias'):
                splitArg = sys.argv[aidx].split('=')
                try:
                    jobalias = splitArg[1]
                except:
                    print 'mpdlistjobs: invalid argument:', sys.argv[aidx]
                    usage()
                aidx += 1
            elif sys.argv[aidx] == '--sss':
                sssPrintFormat = 1
                aidx +=1
            else:
                print 'unrecognized arg: %s' % sys.argv[aidx]
                sys.exit(-1)

    parmdb = MPDParmDB(orderedSources=['cmdline','xml','env','rcfile','thispgm'])
    parmsToOverride = {
                        'MPD_USE_ROOT_MPD'            :  0,
                        'MPD_SECRETWORD'              :  '',
                      }
    for (k,v) in parmsToOverride.items():
        parmdb[('thispgm',k)] = v
    parmdb.get_parms_from_env(parmsToOverride)
    parmdb.get_parms_from_rcfile(parmsToOverride)
    if (hasattr(os,'getuid')  and  os.getuid() == 0)  or  parmdb['MPD_USE_ROOT_MPD']:
        fullDirName = os.path.abspath(os.path.split(sys.argv[0])[0])  # normalize
        mpdroot = os.path.join(fullDirName,'mpdroot')
        conSock = MPDConClientSock(mpdroot=mpdroot,secretword=parmdb['MPD_SECRETWORD'])
    else:
        conSock = MPDConClientSock(secretword=parmdb['MPD_SECRETWORD'])

    msgToSend = { 'cmd' : 'mpdlistjobs' }
    conSock.send_dict_msg(msgToSend)
    msg = conSock.recv_dict_msg(timeout=5.0)
    if not msg:
        mpd_print(1,'no msg recvd from mpd before timeout')
    if msg['cmd'] != 'local_mpdid':     # get full id of local mpd for filters later
        mpd_print(1,'did not recv local_mpdid msg from local mpd; instead, recvd: %s' % msg)
    else:
        if len(sjobid) == 1:
            sjobid.append(msg['id'])
    done = 0
    while not done:
        msg = conSock.recv_dict_msg()
        if not msg.has_key('cmd'):
            mpd_print(1,'mpdlistjobs: INVALID msg=:%s:' % (msg) )
            sys.exit(-1)
        if msg['cmd'] == 'mpdlistjobs_info':
            smjobid = msg['jobid'].split('  ')  # jobnum, mpdid, and alias (if present)
            if len(smjobid) < 3:
                smjobid.append('')
            print_based_on_uname    = 0    # default
            print_based_on_jobid    = 0    # default
            print_based_on_jobalias = 0    # default
            if not uname  or  uname == msg['username']:
                print_based_on_uname = 1
            if not jobid  and  not jobalias:
                print_based_on_jobid = 1
                print_based_on_jobalias = 1
            else:
                if sjobid  and  sjobid[0] == smjobid[0]  and  sjobid[1] == smjobid[1]:
                    print_based_on_jobid = 1
                if jobalias  and  jobalias == smjobid[2]:
                    print_based_on_jobalias = 1
            if not smjobid[2]:
                smjobid[2] = '          '  # just for printing
            if print_based_on_uname and (print_based_on_jobid or print_based_on_jobalias):
                if sssPrintFormat:
                    print "%s %s %s"%(msg['host'],msg['clipid'],msg['sid'])
                else:
                    print 'jobid    = %s@%s' % (smjobid[0],smjobid[1])
                    print 'jobalias = %s'    % (smjobid[2])
                    print 'username = %s'    % (msg['username'])
                    print 'host     = %s'    % (msg['host'])
                    print 'pid      = %s'    % (msg['clipid'])
                    print 'sid      = %s'    % (msg['sid'])
                    print 'rank     = %s'    % (msg['rank'])
                    print 'pgm      = %s'    % (msg['pgm'])
                    print
        else:  # mpdlistjobs_trailer
            done = 1
    conSock.close()

def sig_handler(signum,frame):
    mpd_handle_signal(signum,frame)  # not nec since I exit next
    sys.exit(-1)

def usage():
    print __doc__
    sys.exit(-1)

if __name__ == '__main__':
    mpdlistjobs()
