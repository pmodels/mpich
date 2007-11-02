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
__version__ = "$Revision: 1.1 $"
__credits__ = ""


from sys    import argv, exit
from os     import environ, getuid, close
from socket import socket, fromfd, AF_UNIX, SOCK_STREAM
from re     import sub
from signal import signal, alarm, SIG_DFL, SIGINT, SIGTSTP, SIGCONT, SIGALRM
from mpdlib import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_my_username, mpd_raise, mpdError, mpd_send_one_line

def mpdlistjobs():
    mpd_set_my_id('mpdlistjobs_')
    username = mpd_get_my_username()
    uname    = ''
    jobid    = ''
    sjobid   = ''
    jobalias = ''
    sssPrintFormat = 0
    if len(argv) > 1:
        aidx = 1
        while aidx < len(argv):
            if argv[aidx] == '-h'  or  argv[aidx] == '--help':
                usage()
            if argv[aidx] == '-u':    # or --user=
                uname = argv[aidx+1]
                aidx += 2
            elif argv[aidx].startswith('--user'):
                splitArg = argv[aidx].split('=')
                try:
                    uname = splitArg[1]
                except:
                    print 'mpdlistjobs: invalid argument:', argv[aidx]
                    usage()
                aidx += 1
            elif argv[aidx] == '-j':    # or --jobid=
                jobid = argv[aidx+1]
                aidx += 2
                sjobid = jobid.split('@')    # jobnum and originating host
            elif argv[aidx].startswith('--jobid'):
                splitArg = argv[aidx].split('=')
                try:
                    jobid = splitArg[1]
                    sjobid = jobid.split('@')    # jobnum and originating host
                except:
                    print 'mpdlistjobs: invalid argument:', argv[aidx]
                    usage()
                aidx += 1
            elif argv[aidx] == '-a':    # or --alias=
                jobalias = argv[aidx+1]
                aidx += 2
            elif argv[aidx].startswith('--alias'):
                splitArg = argv[aidx].split('=')
                try:
                    jobalias = splitArg[1]
                except:
                    print 'mpdlistjobs: invalid argument:', argv[aidx]
                    usage()
                aidx += 1
            elif argv[aidx] == '--sss':
                sssPrintFormat = 1
                aidx +=1
            else:
                print 'unrecognized arg: %s' % argv[aidx]
                exit(-1)
    if environ.has_key('UNIX_SOCKET'):
        conFD = int(environ['UNIX_SOCKET'])
        conSocket = fromfd(conFD,AF_UNIX,SOCK_STREAM)
        close(conFD)
    else:
        if environ.has_key('MPD_CON_EXT'):
            conExt = '_' + environ['MPD_CON_EXT']
        else:
            conExt = ''
        consoleName = '/tmp/mpd2.console_' + username + conExt
        conSocket = socket(AF_UNIX,SOCK_STREAM)  # note: UNIX socket
        try:
            conSocket.connect(consoleName)
        except Exception, errmsg:
            print 'mpdlistjobs: cannot connect to local mpd (%s); possible causes:' % consoleName
            print '    1. no mpd running on this host'
            print '    2. mpd is running but was started without a "console" (-n option)'
	    print 'you can start an mpd with the "mpd" command; to get help, run:'
	    print '    mpd -h'
            exit(-1)
        msgToSend = 'realusername=%s\n' % username
        mpd_send_one_line(conSocket,msgToSend)
    msgToSend = { 'cmd' : 'mpdlistjobs' }
    mpd_send_one_msg(conSocket,msgToSend)
    msg = recv_one_msg_with_timeout(conSocket,5)
    if not msg:
        mpd_raise('no msg recvd from mpd before timeout')
    if msg['cmd'] != 'local_mpdid':     # get full id of local mpd for filters later
        mpd_raise('did not recv local_mpdid msg from local mpd; instead, recvd: %s' % msg)
    else:
        if len(sjobid) == 1:
            sjobid.append(msg['id'])
    while 1:
        msg = mpd_recv_one_msg(conSocket)
        if not msg.has_key('cmd'):
            raise RuntimeError, 'mpdlistjobs: INVALID msg=:%s:' % (msg)
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
        else:
            break  # mpdlistjobs_trailer


def usage():
    print __doc__
    exit(-1)

def signal_handler(signum,frame):
    if signum == SIGALRM:
        pass
    else:
        exit(-1)

def recv_one_msg_with_timeout(sock,timeout):
    oldTimeout = alarm(timeout)
    msg = mpd_recv_one_msg(sock)    # fails WITHOUT a msg if sigalrm occurs
    alarm(oldTimeout)
    return(msg)

if __name__ == '__main__':
    signal(SIGINT,signal_handler)
    signal(SIGALRM,signal_handler)
    try:
        mpdlistjobs()
    except mpdError, errmsg:
	print 'mpdlistjobs failed: %s' % (errmsg)
