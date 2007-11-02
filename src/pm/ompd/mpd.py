#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpd --help
       mpd [--host=<host> --port=<portnum>] [--noconsole] \ 
           [--trace] [--echo] [--daemon] [--bulletproof] \ 
           [--if=<interface/hostname>] [--listenport=<listenport>]
           [--pid=<pidfilename>]

Long parameter names may be abbreviated to their first letters by using
  only one hyphen and no equal sign:
     mpd -h donner -p 4268 -n
  is equivalent to
     mpd --host=donner --port=4268 --noconsole

--host and --port must be specified together; they tell the new mpd where
  to enter an existing ring;  if they are omitted, the new mpd forms a
  stand-alone ring that other mpds may enter later
--noconsole is useful for running 2 mpds on the same machine; only one of
  them will accept mpd commands
--trace yields lots of traces thru mpd routines; currently too verbose
--echo causes the mpd echo its listener port by which other mpds may connect
--daemon causes mpd to run backgrounded, with no controlling tty
--bulletproof says to turn bulletproofing on (experimental)
--if specifies an alternate interface/hostname for the host this mpd is running
  on; e.g. may be used to specify the alias for an interface other than default
--listenport specifies a port for this mpd to listen on; by default it will
  acquire one from the system
--pid specifies a filename where the mpd will write its pid; most useful with --daemon

A file named .mpd.conf file must be present in the user's home directory
  with read and write access only for the user, and must contain at least
  a line with secretword=<secretword>

To run mpd as root, install it while root and instead of a .mpd.conf file
use mpd.conf (no initial dot) in the /etc directory.' 
"""
from time import ctime
from mpdlib import mpd_version
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.1 $"
__version__ += "  " + str(mpd_version)
__credits__ = ""


from sys       import stdout, argv, settrace, exit, excepthook, __stdout__, __stderr__
from os        import environ, getpid, fork, setpgrp, waitpid, kill, chdir, \
                      setsid, getuid, setuid, setreuid, setregid, setgroups, \
                      umask, close, access, path, stat, unlink, strerror, \
                      dup2, R_OK, X_OK, WNOHANG, \
                      open, fdopen, O_CREAT, O_WRONLY, O_EXCL, O_RDONLY
from pwd       import getpwnam
from socket    import socket, AF_UNIX, SOCK_STREAM, gethostname, gethostbyname_ex
from errno     import EINTR
from select    import select, error
from getopt    import getopt
from types     import FunctionType
from signal    import signal, SIGCHLD, SIGKILL, SIGHUP, SIG_IGN
from atexit    import register
from time      import sleep
from random    import seed, randrange, random
from syslog    import syslog, openlog, closelog, LOG_DAEMON, LOG_INFO, LOG_ERR
from md5       import new
from cPickle   import dumps
from mpdlib    import mpd_print, mpd_print_tb, mpd_get_ranks_in_binary_tree, \
                      mpd_send_one_msg, mpd_recv_one_msg, \
                      mpd_get_inet_listen_socket, mpd_get_inet_socket_and_connect, \
                      mpd_set_procedures_to_trace, mpd_trace_calls, mpd_raise, mpdError, \
                      mpd_get_my_username, mpd_get_groups_for_username, \
                      mpd_set_my_id, mpd_check_python_version, mpd_version, \
                      mpd_socketpair, mpd_same_ips, mpd_uncaught_except_tb, \
                      mpd_recv_one_line
from mpdman    import mpdman

class _ActiveSockInfo:
    pass

class g:    # global data items
    pass


def _mpd_init():
    global stdout
    close(0)
    g.myPid = getpid()
    (g.mySocket,g.myPort) = mpd_get_inet_listen_socket('',g.listenPort)
    if g.echoPortNum:    # do this before becoming a daemon
        print g.myPort
        stdout.flush()
    g.myId = '%s_%d' % (g.myHost,g.myPort)
    mpd_set_my_id(g.myId)
    g.myrealUsername = mpd_get_my_username()
    g.currRingSize = 1    # default
    g.currRingNCPUs = 1   # default
    if environ.has_key('MPD_CON_EXT'):
        g.conExt = '_' + environ['MPD_CON_EXT']
    else:
        g.conExt = ''

    # setup syslog
    import sys    # to get access to excepthook in next line
    sys.excepthook = mpd_uncaught_except_tb
    openlog("mpd",0,LOG_DAEMON)
    syslog(LOG_INFO,"mpd starting  mpdid=%s (port=%d)" % (g.myId,g.myPort) )

    if g.pidFilename:
        pidFileFD = open(g.pidFilename,O_CREAT|O_WRONLY,0755)
        pidFile = fdopen(pidFileFD,'w',0)
        print >>pidFile, "%d" % (getpid())
        pidFile.close()
        
    if g.daemon:      # see if I should become a daemon with no controlling tty
        rc = fork()
        if rc != 0:   # parent exits; child in background
            exit(0)
        setsid()  # become session leader; no controlling tty
        signal(SIGHUP,SIG_IGN)  # make sure no sighup when leader ends
        ## leader exits; svr4: make sure do not get another controlling tty
        rc = fork()
        if rc != 0:
            exit(0)
        chdir("/")  # free up filesys for umount
        umask(0)
        g.logFilename = '/tmp/mpd2.logfile_' + mpd_get_my_username() + g.conExt
        try:    unlink(g.logFilename)
        except: pass
        logFileFD = open(g.logFilename,O_CREAT|O_WRONLY|O_EXCL,0600)
        logFile = fdopen(logFileFD,'w',0)
        stdout = logFile
        stderr = logFile
        print >>stdout, 'logfile for mpd with pid %d' % getpid()
        stdout.flush()
        dup2(logFile.fileno(),__stdout__.fileno())
        dup2(logFile.fileno(),__stderr__.fileno())

    mpd_print(0, 'starting ')
    g.activeSockets = {}
    _add_active_socket(g.mySocket,
                       'my (%s) listener socket' % g.myId, # name
                       '_handle_new_connection',           # handler
                       '',0,0)                             # host,ip,port
    g.nextJobInt = 1
    g.activeJobs = {}
    
    seed()
    g.correctChallengeResponse = {}
    g.conListenSocket = 0
    g.conSocket       = 0
    g.allExiting      = 0
    g.exiting         = 0    # for mpdexit
    if g.allowConsole:
        g.conListenName = '/tmp/mpd2.console_' + mpd_get_my_username() + g.conExt
        consoleAlreadyExists = 0
        if access(g.conListenName,R_OK):    # if console is there, see if mpd is listening
            tempSocket = socket(AF_UNIX,SOCK_STREAM)  # note: UNIX socket
            try:
                tempSocket.connect(g.conListenName)
                consoleAlreadyExists = 1
            except Exception, errmsg:
                tempSocket.close()
                unlink(g.conListenName)
        if consoleAlreadyExists:
            # mpd_raise('an mpd is already running with console at %s' %  (g.conListenName) )
            print 'An mpd is already running with console at %s on %s. ' %  (g.conListenName, g.myHost)
            print 'Start mpd with the -n option for second mpd on same host.'
            syslog(LOG_ERR,"%s: exiting; an mpd is already using the console" % (g.myId) )
            exit(-1)
        g.conListenSocket = socket(AF_UNIX,SOCK_STREAM)  # UNIX
        g.conListenSocket.bind(g.conListenName)
        g.conListenSocket.listen(1)
        _add_active_socket(g.conListenSocket,
                           'my (%s) console listen socket' % g.myId,  # name
                           'handled-inline',                          # handler
                           g.conListenName,0,0)               # host,ip,port
        
    g.generation = 0  # will chg when enter the ring
    if g.entryHost:
        rc = _enter_existing_ring()
        if rc < 0:    # fails if next g.generation <= current
            mpd_raise("Failed to enter existing ring at %s %d" % (g.entryHost,g.entryPort))
    else:
        _create_ring_of_one_mpd()
    
    g.pmi_published_names = {}

    signal(SIGCHLD,sigchld_handler)


def _mpd():
    global stdout
    g.pulse_ctr = 0
    # Main Loop
    done = 0
    while not done:
        socketsToSelect = g.activeSockets.keys()
        try:
            (inReadySockets,unused1,unused2) = select(socketsToSelect,[],[],3)
        except error, data:
            if data[0] == EINTR:        # will come here if receive SIGCHLD, for example
                continue
            else:
                mpd_raise('select error: %s' % strerror(data[0]))
        except Exception, data:
            mpd_raise('other error after select %s :%s:' % ( data.__class__, data) )
        if not inReadySockets:
            if g.pulse_ctr == 0  and  g.rhsSocket:
                mpd_send_one_msg(g.rhsSocket,{'cmd':'pulse'})
            g.pulse_ctr += 1
        if g.pulse_ctr >= 4:
            if g.activeSockets.has_key(g.rhsSocket):   # rhs must have disappeared
                del g.activeSockets[g.rhsSocket]
                g.rhsSocket.close()
            if g.activeSockets.has_key(g.lhsSocket):
                del g.activeSockets[g.lhsSocket]
                g.lhsSocket.close()
            reenter_ring()
            g.pulse_ctr = 0
        for readySocket in inReadySockets:
            if readySocket not in g.activeSockets.keys():  # deleted on another iteration ?
                # printLine = 'unexpected readySocket %d' % (readySocket.fileno())
                # if g.conSocket > 0:
                    # printLine += ', console fd=%d' % (g.conSocket.fileno())
                # print printLine
                if readySocket in socketsToSelect:
                    readySocket.close()
                continue
            if readySocket == g.mySocket:
                _handle_new_connection()
            elif readySocket == g.lhsSocket:
                _handle_lhs_input()
                if g.allExiting:          # got mpdallexit command
                    done = 1
                    break                # out of for loop, then out of while
                if g.exiting:            # got mpdexit cmd
                    done = 1
                    break                # out of for loop, then out of while
            elif readySocket == g.rhsSocket:
                _handle_rhs_input()    # ignoring rc=1 which means we re-entered ring
            elif readySocket == g.conSocket:
                _handle_console_input()
            elif g.activeSockets[readySocket].name == 'rhs_being_challenged':
                _handle_rhs_challenge_response(readySocket)
            elif g.activeSockets[readySocket].name == 'lhs_being_challenged':
                _handle_lhs_challenge_response(readySocket)
            elif g.activeSockets[readySocket].name == 'man_msgs':
                _handle_man_msgs(readySocket)
            elif readySocket == g.conListenSocket:
                _handle_console_connection()
            else:
                mpd_raise('unknown ready socket %s' %  \
                          (`g.activeSockets[readySocket].name`) )
        try:    unlink(g.logFilename)
        except: pass

def _handle_console_connection():
    if not g.conSocket:
        (g.conSocket,newConnAddr) = g.conListenSocket.accept()
        line = mpd_recv_one_line(g.conSocket)  # char-based msg
        line = line.strip()
        if not line:   # may be another mpd just seeing if I am here
            g.conSocket.close()
            g.conSocket = 0
            return
        splitLine = line.split('=',1)
        if len(splitLine) < 2  or  splitLine[0] != 'realusername':
            mpd_print(1, 'console sent bad msg :%s:' % line)
            mpd_send_one_msg(g.conSocket,{ 'cmd':'invalid_msg_received_from_you' })
            g.conSocket.close()
            g.conSocket = 0
            return
        _add_active_socket(g.conSocket,
                           'my (%s) console socket' % g.myId,  # name
                           '_handle_console_input',            # handler
                           g.conSocket,0,0)                 # host,ip,port
        g.activeSockets[g.conSocket].realUsername = splitLine[1]
    else:
        return  ## postpone it; hope the other one frees up soon
        ## we used to deny the second console; now just let it wait for us
        # mpd_print(1, 'rejecting console; already have one' )
        # (tempSocket,newConnAddr) = g.conListenSocket.accept()
        # msgToSend = { 'cmd' : 'already_have_a_console' }
        # mpd_send_one_msg(tempSocket,msgToSend)
        # tempSocket.close()

def _handle_console_input():
    msg = mpd_recv_one_msg(g.conSocket)
    if not msg:
        mpd_print(0000, 'console has disappeared; closing it')
        del g.activeSockets[g.conSocket]
        g.conSocket.close()
        g.conSocket = 0
        return
    if not msg.has_key('cmd'):
        mpd_print(1, 'console sent bad msg :%s:' % msg)
        mpd_send_one_msg(g.rhsSocket,{ 'cmd':'invalid_msg_received_from_you' })
        del g.activeSockets[g.conSocket]
        g.conSocket.close()
        g.conSocket = 0
        return
    if msg['cmd'] == 'mpdrun':
        # permit anyone to run but use THEIR own username
        #   thus, override any username specified by the user
        if g.activeSockets[g.conSocket].realUsername != 'root':
            msg['username'] = g.activeSockets[g.conSocket].realUsername
            msg['users'] = { (0,msg['nprocs']-1) : g.activeSockets[g.conSocket].realUsername }
        #
        msg['mpdid_mpdrun_start'] = g.myId
        msg['nstarted_on_this_loop'] = 0
        msg['first_loop'] = 1
        msg['ringsize'] = 0
        msg['ring_ncpus'] = 0
        if msg.has_key('try_0_locally'):
            _do_mpdrun(msg)
        else:
            mpd_send_one_msg(g.rhsSocket,msg)
        # send ack after job is going
    elif msg['cmd'] == 'get_mpd_version':
        msgToSend = { 'cmd' : 'mpd_version_response', 'mpd_version' : mpd_version }
        mpd_send_one_msg(g.conSocket,msgToSend)
    elif msg['cmd'] == 'mpdtrace':
        msgToSend = { 'cmd'     : 'mpdtrace_info',
                      'dest'    : g.myId,
                      'id'      : g.myId,
                      'lhshost' : '%s' % (g.lhsHost),
                      'lhsport' : '%s' % (g.lhsPort),
                      'lhsip'   : '%s' % (g.lhsIP),
                      'rhshost' : '%s' % (g.rhsHost),
                      'rhsport' : '%s' % (g.rhsPort),
                      'rhsip'   : '%s' % (g.rhsIP) }
        mpd_send_one_msg(g.rhsSocket,msgToSend)
        msgToSend = { 'cmd'  : 'mpdtrace_trailer',
                      'dest' : g.myId }
        mpd_send_one_msg(g.rhsSocket,msgToSend)
        # do not send an ack to console now; will send trace info later
    elif msg['cmd'] == 'mpdallexit':
        if g.activeSockets[g.conSocket].realUsername != g.myrealUsername:
            mpd_send_one_msg(g.conSocket,{ 'cmd':'invalid_username_to_make_this_request' })
            del g.activeSockets[g.conSocket]
            g.conSocket.close()
            g.conSocket = 0
            return
        g.allExiting = 1
        mpd_send_one_msg(g.rhsSocket, {'cmd' : 'mpdallexit', 'src' : g.myId} )
        mpd_send_one_msg(g.conSocket, {'cmd' : 'mpdallexit_ack'} )
    elif msg['cmd'] == 'mpdexit':
        if g.activeSockets[g.conSocket].realUsername != g.myrealUsername:
            mpd_send_one_msg(g.conSocket,{ 'cmd':'invalid_username_to_make_this_request' })
            del g.activeSockets[g.conSocket]
            g.conSocket.close()
            g.conSocket = 0
            return
        if msg['mpdid'] == 'localmpd':
            msg['mpdid'] = g.myId
        mpd_send_one_msg(g.rhsSocket, {'cmd' : 'mpdexit', 'src' : g.myId, 'done' : 0,
                                       'dest' : msg['mpdid']} )
    elif msg['cmd'] == 'mpdringtest':
        msg['src'] = g.myId
        mpd_send_one_msg(g.rhsSocket, msg)
        # do not send an ack to console now; will send ringtest info later
    elif msg['cmd'] == 'mpdlistjobs':
        msgToSend = { 'cmd'  : 'local_mpdid', 'id' : g.myId }
        mpd_send_one_msg(g.conSocket,msgToSend)
        for jobid in g.activeJobs.keys():
            for manPid in g.activeJobs[jobid]:
                msgToSend = { 'cmd' : 'mpdlistjobs_info',
                              'dest' : g.myId,
                              'jobid' : jobid,
                              'username' : g.activeJobs[jobid][manPid]['username'],
                              'host' : g.myHost,
                              'clipid' : str(g.activeJobs[jobid][manPid]['clipid']),
                              'sid' : str(manPid),  # may chg to actual sid later
                              'pgm'  : g.activeJobs[jobid][manPid]['pgm'],
                              'rank' : g.activeJobs[jobid][manPid]['rank'] }
                mpd_send_one_msg(g.conSocket, msgToSend)
        msgToSend = { 'cmd'  : 'mpdlistjobs_trailer', 'dest' : g.myId }
        mpd_send_one_msg(g.rhsSocket,msgToSend)
        # do not send an ack to console now; will send listjobs info later
    elif msg['cmd'] == 'mpdkilljob':
        # permit anyone to kill but use THEIR own username
        #   thus, override any username specified by the user
        if g.activeSockets[g.conSocket].realUsername != 'root':
            msg['username'] = g.activeSockets[g.conSocket].realUsername
        msg['src'] = g.myId
        msg['handled'] = 0
        if msg['mpdid'] == '':
            msg['mpdid'] = g.myId
        mpd_send_one_msg(g.rhsSocket, msg)
        # send ack to console after I get this msg back and do the kill myself
    elif msg['cmd'] == 'mpdsigjob':
        # permit anyone to sig but use THEIR own username
        #   thus, override any username specified by the user
        if g.activeSockets[g.conSocket].realUsername != 'root':
            msg['username'] = g.activeSockets[g.conSocket].realUsername
        msg['src'] = g.myId
        msg['handled'] = 0
        if msg['mpdid'] == '':
            msg['mpdid'] = g.myId
        mpd_send_one_msg(g.rhsSocket, msg)
        # send ack to console after I get this msg back
    elif msg['cmd'] == 'verify_hosts_in_ring':
        msgToSend = { 'cmd'  : 'verify_hosts_in_ring',
                      'dest' : g.myId,
                      'host_list' : msg['host_list'] }
        mpd_send_one_msg(g.rhsSocket,msgToSend)
        # do not send an ack to console now; will send trace info later
    else:
        msgToSend = { 'cmd' : 'invalid_msg_received_from_you' }
        mpd_send_one_msg(g.conSocket,msgToSend)
        badMsg = 'invalid msg received from console: %s' % (str(msg))
        mpd_print(1, badMsg)
        syslog(LOG_ERR,badMsg)

def _handle_lhs_input():
    msg = mpd_recv_one_msg(g.lhsSocket)
    mpd_print(0000, "MPD LHS GOT MSG :%s:" % msg)
    if not msg:    # lost lhs; don't worry
        mpd_print(0, "CLOSING g.lhsSocket ", g.lhsSocket )
        del g.activeSockets[g.lhsSocket]
        g.lhsSocket.close()
        return
    if msg['cmd'] == 'mpdrun':
        if msg.has_key('mpdid_mpdrun_start')  and  msg['mpdid_mpdrun_start'] == g.myId:
            if msg['first_loop']:
                g.currRingSize = msg['ringsize']
                g.currRingNCPUs = msg['ring_ncpus']
            if msg['nstarted'] == msg['nprocs']:
                if g.conSocket:
                    mpd_send_one_msg(g.conSocket, {'cmd' : 'mpdrun_ack',
                                                   'ringsize' : g.currRingSize,
                                                   'ring_ncpus' : g.currRingNCPUs} )
                return
            if not msg['first_loop']  and  msg['nstarted_on_this_loop'] == 0:
                if msg.has_key('jobid'):
                    mpd_send_one_msg(g.rhsSocket,
                                     {'cmd':'abortjob', 'src' : g.myId,
                                      'jobid' : msg['jobid'],
                                      'reason' : 'some_procs_not_started'})
                if g.conSocket:
                    mpd_send_one_msg(g.conSocket, {'cmd' : 'job_failed',
                                                   'reason' : 'some_procs_not_started',
                                                   'remaining_hosts' : msg['hosts'] } )
                return
            msg['first_loop'] = 0
            msg['nstarted_on_this_loop'] = 0
        _do_mpdrun(msg)
    elif msg['cmd'] == 'mpdtrace_info':
        if msg['dest'] == g.myId:
            mpd_send_one_msg(g.conSocket,msg)
        else:
            mpd_send_one_msg(g.rhsSocket,msg)
    elif msg['cmd'] == 'mpdtrace_trailer':
        if msg['dest'] == g.myId:
            mpd_send_one_msg(g.conSocket,msg)
        else:
            msgToSend = { 'cmd'     : 'mpdtrace_info',
                          'dest'    : msg['dest'],
                          'id'      : g.myId,
                          'lhshost' : '%s' % (g.lhsHost),
                          'lhsport' : '%s' % (g.lhsPort),
                          'lhsip'   : '%s' % (g.lhsIP),
                          'rhshost' : '%s' % (g.rhsHost),
                          'rhsport' : '%s' % (g.rhsPort),
                          'rhsip'   : '%s' % (g.rhsIP) }
            mpd_send_one_msg(g.rhsSocket, msgToSend)
            mpd_send_one_msg(g.rhsSocket, msg)
    elif msg['cmd'] == 'mpdlistjobs_info':
        if msg['dest'] == g.myId:
            mpd_send_one_msg(g.conSocket,msg)
        else:
            mpd_send_one_msg(g.rhsSocket,msg)
    elif msg['cmd'] == 'mpdlistjobs_trailer':
        if msg['dest'] == g.myId:
            mpd_send_one_msg(g.conSocket,msg)
        else:
            for jobid in g.activeJobs.keys():
                for manPid in g.activeJobs[jobid]:
                    msgToSend = { 'cmd' : 'mpdlistjobs_info',
                                  'dest' : msg['dest'],
                                  'jobid' : jobid,
                                  'username' : g.activeJobs[jobid][manPid]['username'],
                                  'host' : g.myHost,
                                  'clipid' : str(g.activeJobs[jobid][manPid]['clipid']),
                                  'sid' : str(manPid),  # may chg to actual sid later
                                  'pgm' : g.activeJobs[jobid][manPid]['pgm'],
                                  'rank' : g.activeJobs[jobid][manPid]['rank'] }
                    mpd_send_one_msg(g.rhsSocket, msgToSend)
            mpd_send_one_msg(g.rhsSocket, msg)
    elif msg['cmd'] == 'mpdallexit':
        g.allExiting = 1
        if msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket, msg)
    elif msg['cmd'] == 'mpdexit':
        if msg['dest'] == g.myId:
            msg['done'] = 1    # do this first
        if msg['src'] == g.myId:    # may be src and dest
            if g.conSocket:
                if msg['done']:
                    mpd_send_one_msg(g.conSocket,{'cmd' : 'mpdexit_ack'})
                else:
                    mpd_send_one_msg(g.conSocket,{'cmd' : 'mpdexit_failed'})
        else:
            mpd_send_one_msg(g.rhsSocket,msg)
        if msg['dest'] == g.myId:
            g.exiting = 1
            mpd_send_one_msg(g.lhsSocket, { 'cmd'     : 'mpdexiting',
                                            'rhshost' : g.rhsHost,
                                            'rhsip'   : g.rhsIP,
                                            'rhsport' : g.rhsPort })
    elif msg['cmd'] == 'mpdringtest':
        if msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket, msg)
        else:
            numLoops = msg['numloops'] - 1
            if numLoops > 0:
                msg['numloops'] = numLoops
                mpd_send_one_msg(g.rhsSocket, msg)
            else:
                if g.conSocket:    # may have closed it if user did ^C at console
                    mpd_send_one_msg(g.conSocket, {'cmd' : 'mpdringtest_done' })
    elif msg['cmd'] == 'mpdsigjob':
        forwarded = 0
        if msg['handled']  and  msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket,msg)
            forwarded = 1
        handledHere = 0
        for jobid in g.activeJobs.keys():
            sjobid = jobid.split('  ')  # jobnum and mpdid
            if (sjobid[0] == msg['jobnum']  and  sjobid[1] == msg['mpdid'])  \
            or (msg['jobalias']  and  sjobid[2] == msg['jobalias']):
                for manPid in g.activeJobs[jobid].keys():
                    if g.activeJobs[jobid][manPid]['username'] == msg['username']  \
                    or msg['username'] == 'root':
                        manSocket = g.activeJobs[jobid][manPid]['socktoman']
                        mpd_send_one_msg(manSocket, { 'cmd' : 'signal_to_handle',
                                                      's_or_g' : msg['s_or_g'],
                                                      'sigtype' : msg['sigtype'] } )
                        handledHere = 1
        if handledHere:
            msg['handled'] = 1
        if not forwarded  and  msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket,msg)
        if msg['src'] == g.myId:
            if g.conSocket:
                mpd_send_one_msg(g.conSocket, {'cmd' : 'mpdsigjob_ack',
                                               'handled' : msg['handled'] } )
    elif msg['cmd'] == 'mpdkilljob':
        forwarded = 0
        if msg['handled'] and msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket,msg)
            forwarded = 1
        handledHere = 0
        for jobid in g.activeJobs.keys():
            sjobid = jobid.split('  ')  # jobnum and mpdid
            if (sjobid[0] == msg['jobnum']  and  sjobid[1] == msg['mpdid'])  \
            or (msg['jobalias']  and  sjobid[2] == msg['jobalias']):
                for manPid in g.activeJobs[jobid].keys():
                    if g.activeJobs[jobid][manPid]['username'] == msg['username']  \
                    or msg['username'] == 'root':
                        try:
                            pgrp = manPid * (-1)  # neg manPid -> group
                            kill(pgrp,SIGKILL)
                            cliPid = g.activeJobs[jobid][manPid]['clipid']
                            pgrp = cliPid * (-1)  # neg Pid -> group
                            kill(pgrp,SIGKILL)  # neg Pid -> group
                            handledHere = 1
                        except:
                            pass
                # del g.activeJobs[jobid]  ## handled when child goes away
        if handledHere:
            msg['handled'] = 1
        if not forwarded  and  msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket,msg)
        if msg['src'] == g.myId:
            if g.conSocket:
                mpd_send_one_msg(g.conSocket, {'cmd' : 'mpdkilljob_ack',
                                               'handled' : msg['handled'] } )
    elif msg['cmd'] == 'abortjob':
        if msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket,msg)
        for jobid in g.activeJobs.keys():
            if jobid == msg['jobid']:
                for manPid in g.activeJobs[jobid].keys():
                    manSocket = g.activeJobs[jobid][manPid]['socktoman']
                    if manSocket:
                        mpd_send_one_msg(manSocket,msg)
                        sleep(0.5)  # give man a brief chance to deal with this
                    try:
                        pgrp = manPid * (-1)  # neg manPid -> group
                        kill(pgrp,SIGKILL)
                        cliPid = g.activeJobs[jobid][manPid]['clipid']
                        pgrp = cliPid * (-1)  # neg Pid -> group
                        kill(pgrp,SIGKILL)  # neg Pid -> group
                    except:
                        pass
                # del g.activeJobs[jobid]  ## handled when child goes away
    elif msg['cmd'] == 'pulse':
        mpd_send_one_msg(g.lhsSocket,{'cmd':'pulse_ack'})
    elif msg['cmd'] == 'verify_hosts_in_ring':
        while g.myIP in msg['host_list']  or  g.myHost in msg['host_list']:
            if g.myIP in msg['host_list']:
                msg['host_list'].remove(g.myIP)
            elif g.myHost in msg['host_list']:
                msg['host_list'].remove(g.myHost)
        if msg['dest'] == g.myId:
            msgToSend = { 'cmd' : 'verify_hosts_in_ring_response',
                          'host_list' : msg['host_list'] }
            mpd_send_one_msg(g.conSocket,msgToSend)
        else:
            mpd_send_one_msg(g.rhsSocket,msg)
    elif msg['cmd'] == 'pmi_lookup_name':
        if msg['src'] == g.myId:
            if msg.has_key('port') and msg['port'] != 0:
                msgToSend = msg
                msgToSend['cmd'] = 'lookup_result'
                msgToSend['info'] = 'ok'
            else:
                msgToSend = { 'cmd' : 'lookup_result', 'info' : 'unknown_service',
                              'port' : 0}
            jobid = msg['jobid']
            manpid = msg['manpid']
            manSocket = g.activeJobs[jobid][manpid]['socktoman']
            mpd_send_one_msg(manSocket,msgToSend)
        else:
            if g.pmi_published_names.has_key(msg['service']):
                msg['port'] = g.pmi_published_names[msg['service']]
            mpd_send_one_msg(g.rhsSocket,msg)
    elif msg['cmd'] == 'pmi_unpublish_name':
        if msg['src'] == g.myId:
            if msg.has_key('done'):
                msgToSend = msg
                msgToSend['cmd'] = 'unpublish_result'
                msgToSend['info'] = 'ok'
            else:
                msgToSend = { 'cmd' : 'unpublish_result', 'info' : 'unknown_service' }
            jobid = msg['jobid']
            manpid = msg['manpid']
            manSocket = g.activeJobs[jobid][manpid]['socktoman']
            mpd_send_one_msg(manSocket,msgToSend)
        else:
            if g.pmi_published_names.has_key(msg['service']):
                del g.pmi_published_names[msg['service']]
                msg['done'] = 1
            mpd_send_one_msg(g.rhsSocket,msg)
    else:
        mpd_print(1, 'unrecognized cmd from lhs: %s' % (msg) )

def _do_mpdrun(msg):
    mpd_print(0000, "DO_MPDRUN MSG=:%s:" % msg)
    if msg.has_key('jobid'):
        jobid = msg['jobid']
    else:
        jobid = str(g.nextJobInt) + '  ' + g.myId + '  ' + msg['jobalias']
        g.nextJobInt += 1
        msg['jobid'] = jobid
    handled_one__any_ = 0
    while 1:
        if msg['nstarted'] >= msg['nprocs']:
            break
        if g.activeJobs.has_key(jobid):
            procsHereForJob = len(g.activeJobs[jobid].keys())
        else:
            procsHereForJob = 0
        if handled_one__any_  and  procsHereForJob >= g.NCPUs:
            break
        hosts = msg['hosts']
        currRank = msg['nstarted']
        found = 0
        hostSpecForCurrRank = '_any_'
        for ranks in hosts.keys():
            (lo,hi) = ranks
            if currRank >= lo and currRank <= hi:
                hostSpecForCurrRank = hosts[ranks]
                break;
        if hostSpecForCurrRank == '_any_':
            handled_one__any_ = 1
            found = 1
        elif g.myIP == hostSpecForCurrRank  or  g.myHost == hostSpecForCurrRank:
            found = 1
        elif hostSpecForCurrRank == '_any_from_pool_':
            host_spec_pool = msg['host_spec_pool']
            if g.myIP in host_spec_pool  or  g.myHost in host_spec_pool:
                found = 1
                handled_one__any_ = 1
        if not found:
            break
        if lo < hi:
            msg['hosts'][(lo+1,hi)] = msg['hosts'][ranks]
        del msg['hosts'][ranks]
        msg['nstarted'] += 1
        msg['nstarted_on_this_loop'] += 1
        if currRank == 0:
            manLhsHost = 'dummy_host'
            manLhsIP   = '0.0.0.0'
            manLhsPort = 0
        else:
            manLhsHost = msg['lhshost']
            manLhsIP   = msg['lhsip']
            manLhsPort = msg['lhsport']
        (tempSocket,tempPort) = mpd_get_inet_listen_socket('',0)
        if not tempSocket:
            mpd_print(1,'_do_mpdrun failed to obtain listen socket')
            if g.conSocket:
                mpd_send_one_msg(g.conSocket, {'cmd' : 'job_failed',
                                               'reason' : 'failed_to_get_listen_socket'})
            return
        (toManSocket,toMpdSocket) = mpd_socketpair()
        if not toManSocket:
            mpd_print(1,'_do_mpdrun failed to obtain socketpair')
            if g.conSocket:
                mpd_send_one_msg(g.conSocket, {'cmd' : 'job_failed',
                                               'reason' : 'failed_to_get_socketpair'})
            return
        msg['lhshost'] = g.myHost
        msg['lhsip']   = g.myIP
        msg['lhsport'] = tempPort
        if currRank == 0:
            msg['host0'] = g.myHost
            msg['ip0']   = g.myIP
            msg['port0'] = tempPort
        manHost0 = msg['host0']
        manIP0   = msg['ip0']
        manPort0 = msg['port0']
        users = msg['users']
        for ranks in users.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                username = users[ranks]
                try:
                    pwent = getpwnam(username)
                except:
                    mpd_print(1,'%s is invalid username on %s' % (username,g.myHost))
                    mpd_send_one_msg(g.conSocket, {'cmd' : 'job_failed',
                                                   'reason' : 'invalid_username',
                                                   'username' : username,
                                                   'host' : g.myHost } )
                    return
                break
        execs = msg['execs']
        for ranks in execs.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgm = execs[ranks]
                break
        paths = msg['paths']
        for ranks in paths.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pathForExec = paths[ranks]
                break
        args = msg['args']
        for ranks in args.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmArgs = dumps(args[ranks])
                break
        envvars = msg['envvars']
        for ranks in envvars.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmEnvVars = dumps(envvars[ranks])
                break
        limits = msg['limits']
        for ranks in limits.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmLimits = dumps(limits[ranks])
                break
        cwds = msg['cwds']
        for ranks in cwds.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                cwd = cwds[ranks]
                break
        procsHereForJob += 1
        manPid = fork()
        if manPid == 0:
            mpd_set_my_id('%s_man_%d' % (g.myHost,g.myPid) )
            g.myId = '%s_man_%d' % (g.myHost,g.myPid)
            for sock in g.activeSockets:
                sock.close()
            toManSocket.close()
            setpgrp()
            environ['MPDMAN_MYHOST'] = g.myHost
            environ['MPDMAN_MYIP'] = g.myIP
            environ['MPDMAN_JOBID'] = jobid
            environ['MPDMAN_CLI_PGM'] = pgm
            environ['MPDMAN_CLI_PATH'] = pathForExec
            environ['MPDMAN_PGM_ARGS'] = pgmArgs
            environ['MPDMAN_PGM_ENVVARS'] = pgmEnvVars
            environ['MPDMAN_PGM_LIMITS'] = pgmLimits
            environ['MPDMAN_CWD'] = cwd
            environ['MPDMAN_SPAWNED'] = str(msg['spawned'])
            environ['MPDMAN_NPROCS'] = str(msg['nprocs'])
            environ['MPDMAN_MPD_LISTEN_PORT'] = str(g.myPort)
            environ['MPDMAN_MPD_CONF_SECRETWORD'] = g.configParams['secretword']
            environ['MPDMAN_CONHOST'] = msg['conhost']
	    if msg.has_key('conifhn')  and  msg['conifhn']:
                environ['MPDMAN_CONIP'] = msg['conifhn']
	    else:
                environ['MPDMAN_CONIP'] = g.myIP
            environ['MPDMAN_CONPORT'] = str(msg['conport'])
            environ['MPDMAN_RANK'] = str(currRank)
            environ['MPDMAN_LHSHOST'] = manLhsHost
            environ['MPDMAN_LHSIP']   = manLhsIP
            environ['MPDMAN_LHSPORT'] = str(manLhsPort)
            environ['MPDMAN_HOST0'] = manHost0
            environ['MPDMAN_IP0']   = manIP0
            environ['MPDMAN_PORT0'] = str(manPort0)
            environ['MPDMAN_MY_LISTEN_PORT'] = str(tempPort)
            environ['MPDMAN_MY_LISTEN_FD'] = str(tempSocket.fileno())
            environ['MPDMAN_TO_MPD_FD'] = str(toMpdSocket.fileno())
            environ['MPDMAN_STDIN_GOES_TO_WHO'] = msg['stdin_goes_to_who']
            environ['MPDMAN_TOTALVIEW'] = str(msg['totalview'])
            environ['MPDMAN_GDB'] = str(msg['gdb'])
            fullDirName = path.abspath(path.split(argv[0])[0])  # normalize
            environ['MPDMAN_FULLPATHDIR'] = fullDirName    # used to find gdbdrv
            environ['MPDMAN_SINGINIT_PID']  = str(msg['singinitpid'])
            environ['MPDMAN_SINGINIT_PORT'] = str(msg['singinitport'])
            if msg.has_key('line_labels'):
                environ['MPDMAN_LINE_LABELS'] = '1'
            else:
                environ['MPDMAN_LINE_LABELS'] = '0'
            if msg.has_key('rship'):
                environ['MPDMAN_RSHIP'] = msg['rship']
                environ['MPDMAN_MSHIP_HOST'] = msg['mship_host']
                environ['MPDMAN_MSHIP_PORT'] = str(msg['mship_port'])
            if msg.has_key('doing_bnr'):
                environ['MPDMAN_DOING_BNR'] = '1'
            else:
                environ['MPDMAN_DOING_BNR'] = '0'
            if getuid() == 0:
                uid = pwent[2]
                gid = pwent[3]
                setgroups(mpd_get_groups_for_username(username))
                setregid(gid,gid)
                setreuid(uid,uid)
            import atexit    # need to use full name of _exithandlers
            atexit._exithandlers = []    # un-register handlers in atexit module
            # import profile
            # print 'profiling the manager'
            # profile.run('mpdman()')
            mpdman()
            exit(0)  # do NOT do cleanup
        else:
            tempSocket.close()
            toMpdSocket.close()
            if not g.activeJobs.has_key(jobid):
                g.activeJobs[jobid] = {}
            g.activeJobs[jobid][manPid] = { 'pgm' : pgm, 'rank' : currRank,
                                            'username' : username,
                                            'clipid' : -1,    # until reported by man
                                            'socktoman' : toManSocket }
            _add_active_socket(toManSocket,'man_msgs','_handle_man_msgs',
                               'localhost',0,tempPort)
    msg['ringsize'] += 1
    msg['ring_ncpus'] += g.NCPUs
    mpd_print(0000, "FORWARDING MSG=:%s:" % msg)
    mpd_send_one_msg(g.rhsSocket,msg)  # forward it on around

def reenter_ring():
    if g.entryHost:
        inRing = 0
        numTries = 5
        while not inRing  and  numTries > 0:
            rc = _enter_existing_ring()
            if rc < 0:    # fails if next g.generation <= current
                sleep(2)
            else:
                inRing = 1
            numTries -= 1
    else:
        _create_ring_of_one_mpd()
        inRing = 1
    if not inRing:
        mpd_raise("Failed to reenter ring at %s %d" % (g.entryHost,g.entryPort))

def _handle_rhs_input():
    if g.allExiting:
        return
    msg = mpd_recv_one_msg(g.rhsSocket)
    if not msg:    # lost rhs; re-knit the ring
        del g.activeSockets[g.rhsSocket]
        g.rhsSocket.close()
        if g.activeSockets.has_key(g.lhsSocket):
            del g.activeSockets[g.lhsSocket]
            g.lhsSocket.close()
        mpd_print(1,'lost rhs; re-entering ring')
        reenter_ring()
        return 1
    if msg['cmd'] == 'pulse_ack':
        g.pulse_ctr = 0
    elif msg['cmd'] == 'mpdexiting':    # for mpdexit
        if g.rhsSocket:
            if g.activeSockets.has_key(g.rhsSocket):
                del g.activeSockets[g.rhsSocket]
            g.rhsSocket.close()
        # connect to new rhs
        g.rhsHost = msg['rhshost']
        g.rhsIP   = msg['rhsip']
        g.rhsPort = int(msg['rhsport'])
        mpd_print(0000,"TRYING TO CONN TO %s %s" % (g.rhsHost,g.rhsPort))
        if g.rhsIP == g.myIP  and  g.rhsPort == g.myPort:
            if g.lhsSocket:
                if g.activeSockets.has_key(g.lhsSocket):
                    del g.activeSockets[g.lhsSocket]
                g.lhsSocket.close()
            _create_ring_of_one_mpd()
            mpd_print(0000,"DID CONN TO MYSELF %s %s" % (g.rhsHost,g.rhsPort))
            return
        g.rhsSocket = mpd_get_inet_socket_and_connect(g.rhsIP,g.rhsPort)
        if not g.rhsSocket:
            mpd_print(1,'_handle_rhs_input failed to obtain rhs socket')
            return
        _add_active_socket(g.rhsSocket,'rhs','_handle_rhs_input',
                           g.rhsHost,g.rhsIP,g.rhsPort)
        msgToSend = { 'cmd' : 'request_to_enter_as_lhs',
                      'host' : g.myHost,
                      'ip'   : g.myIP,
                      'port' : g.myPort }
        mpd_send_one_msg(g.rhsSocket,msgToSend)
        msg = mpd_recv_one_msg(g.rhsSocket)
        if (not msg) or  \
           (not msg.has_key('cmd')) or  \
           (msg['cmd'] != 'challenge') or (not msg.has_key('randnum')):
            mpd_raise('failed to recv challenge from rhs; msg=:%s:' % (msg) )
        response = new(''.join([g.configParams['secretword'],msg['randnum']])).digest()
        msgToSend = { 'cmd' : 'challenge_response',
                      'response' : response,
                      'host'     : g.myHost,
                      'ip'       : g.myIP,
                      'port'     : g.myPort }
        mpd_send_one_msg(g.rhsSocket,msgToSend)
        msg = mpd_recv_one_msg(g.rhsSocket)
        if (not msg) or  \
           (not msg.has_key('cmd')) or  \
           (msg['cmd'] != 'OK_to_enter_as_lhs'):
            mpd_raise('NOT OK to enter ring; msg=:%s:' % (msg) )
        mpd_print(0000,"GOT CONN TO %s %s" % (g.rhsHost,g.rhsPort))
    else:
        mpd_print(1, 'unexpected from rhs; msg=:%s:' % (msg) )
    return 0

def _handle_new_connection():
    randHiRange = 10000
    (newConnSocket,newConnAddr) = g.mySocket.accept()
    # print 'newConnSocket = ', newConnSocket.getsockname() 
    msg = mpd_recv_one_msg(newConnSocket)
    if (not msg) or \
       (not msg.has_key('cmd')) or (not msg.has_key('host')) or  \
       (not msg.has_key('port')):
        mpd_print(1, 'INVALID msg from new connection :%s: msg=:%s:' % (newConnAddr,msg) )
        newConnSocket.close()
        return
    if msg['cmd'] == 'request_to_enter_as_rhs':
        if msg['mpd_version'] != mpd_version:
            msgToSend = { 'cmd'          : 'entry_rejected_bad_mpd_version',
                          'your_version' : msg['mpd_version'],
                          'my_version'   : mpd_version }
            mpd_send_one_msg(newConnSocket,msgToSend)
            return
        randNumStr = '%04d' % (randrange(1,randHiRange))  # 0001-(hi-1), inclusive
        g.correctChallengeResponse[newConnSocket] = \
            new(''.join([g.configParams['secretword'],randNumStr])).digest()
        msgToSend = { 'cmd'        : 'challenge',
                      'randnum'    : randNumStr,
                      'g.generation' : g.generation }  # only send to rhs
        mpd_send_one_msg(newConnSocket,msgToSend)
        _add_active_socket(newConnSocket,'rhs_being_challenged',
                           '_handle_rhs_challenge_response',
                           msg['host'],msg['ip'],msg['port'])
    elif msg['cmd'] == 'request_to_enter_as_lhs':
        randNumStr = '%04d' % (randrange(1,randHiRange))  # 0001-(hi-1), inclusive
        g.correctChallengeResponse[newConnSocket] = \
            new(''.join([g.configParams['secretword'],randNumStr])).digest()
        msgToSend = { 'cmd' : 'challenge',
                      'randnum' : randNumStr }
        mpd_send_one_msg(newConnSocket,msgToSend)
        _add_active_socket(newConnSocket,'lhs_being_challenged',
                           '_handle_lhs_challenge_response',
                           msg['host'],msg['ip'],msg['port'])
    elif msg['cmd'] == 'ping':
        msgToSend = { 'cmd' : 'ping_ack' }
        mpd_send_one_msg(newConnSocket,msgToSend)
        newConnSocket.close()
    else:
        mpd_print(1, 'INVALID msg from new connection :%s:  msg=:%s:' % (newConnAddr,msg) )
        newConnSocket.close()

def _handle_lhs_challenge_response(responseSocket):
    msg = mpd_recv_one_msg(responseSocket)
    if (not msg)   or  \
       (not msg.has_key('cmd'))   or  (not msg.has_key('response'))  or  \
       (not msg.has_key('host'))  or  (not msg.has_key('port'))  or  \
       (msg['response'] != g.correctChallengeResponse[responseSocket]):
        mpd_print(1, 'INVALID msg for lhs response msg=:%s: from host=%s port=%d' % \
                  (msg,g.activeSockets[responseSocket].host,
                   g.activeSockets[responseSocket].port) )
        msgToSend = { 'cmd' : 'invalid_response' }
        mpd_send_one_msg(responseSocket,msgToSend)
        del g.correctChallengeResponse[responseSocket]
        del g.activeSockets[responseSocket]
        responseSocket.close()
    else:
        msgToSend = { 'cmd' : 'OK_to_enter_as_lhs' }
        mpd_send_one_msg(responseSocket,msgToSend)
        if g.activeSockets.has_key(g.lhsSocket):
            del g.activeSockets[g.lhsSocket]
            g.lhsSocket.close()
        g.lhsSocket = responseSocket
        g.lhsHost = msg['host']
        g.lhsIP = msg['ip']
        g.lhsPort = int(msg['port'])
        _add_active_socket(g.lhsSocket,'lhs','_handle_lhs_input',
                           g.lhsHost,g.lhsIP,g.lhsPort)

def _handle_rhs_challenge_response(responseSocket):
    msg = mpd_recv_one_msg(responseSocket)
    if (not msg)   or  \
       (not msg.has_key('cmd'))   or  (not msg.has_key('response'))  or  \
       (not msg.has_key('host'))  or  (not msg.has_key('port')):
        mpd_print(1, 'INVALID msg for rhs response msg=:%s: from host=%s port=%d' % \
                  (msg,g.activeSockets[responseSocket].host,
                   g.activeSockets[responseSocket].port) )
        msgToSend = { 'cmd' : 'invalid_response' }
        mpd_send_one_msg(responseSocket,msgToSend)
        del g.correctChallengeResponse[responseSocket]
        del g.activeSockets[responseSocket]
        responseSocket.close()
    elif msg['response'] != g.correctChallengeResponse[responseSocket]:
        mpd_print(1, 'INVALID response in rhs response msg=:%s: from host=%s port=%d' % \
                  (msg,g.activeSockets[responseSocket].host,
                   g.activeSockets[responseSocket].port) )
        msgToSend = { 'cmd' : 'invalid_response' }
        mpd_send_one_msg(responseSocket,msgToSend)
        del g.correctChallengeResponse[responseSocket]
        del g.activeSockets[responseSocket]
        responseSocket.close()
    elif msg['response'] == 'bad_generation':
        mpd_print(1, 'someone failed entering my ring gen=%d msg=%s' % (g.generation,msg) )
        del g.correctChallengeResponse[responseSocket]
        del g.activeSockets[responseSocket]
        responseSocket.close()
    else:
        msgToSend = { 'cmd' : 'OK_to_enter_as_rhs',
                      'rhshost' : g.rhsHost,
                      'rhsip'   : g.rhsIP,
                      'rhsport' : g.rhsPort }
        mpd_send_one_msg(responseSocket,msgToSend)
        del g.activeSockets[g.rhsSocket]
        g.rhsSocket.close()
        g.rhsSocket = responseSocket
        g.rhsHost = msg['host']
        g.rhsIP   = msg['ip']
        g.rhsPort = int(msg['port'])
        _add_active_socket(g.rhsSocket,'rhs','_handle_rhs_input',
                           g.rhsHost,g.rhsIP,g.rhsPort)

def _handle_man_msgs(manSocket):
    msg = mpd_recv_one_msg(manSocket)
    if not msg:
        for jobid in g.activeJobs.keys():
            deleted = 0
            for manPid in g.activeJobs[jobid]:
                if manSocket == g.activeJobs[jobid][manPid]['socktoman']:
                    del g.activeJobs[jobid][manPid]
                    if len(g.activeJobs[jobid]) == 0:
                        del g.activeJobs[jobid]
                    deleted = 1
                    break
            if deleted:
                break
        del g.activeSockets[manSocket]
        manSocket.close()
        return
    if not msg.has_key('cmd'):
        mpd_print(1, 'INVALID msg for man request msg=:%s:' % (msg) )
        msgToSend = { 'cmd' : 'invalid_msg' }
        mpd_send_one_msg(manSocket,msgToSend)
        del g.activeSockets[manSocket]
        manSocket.close()
        return
    if msg['cmd'] == 'client_pid':
        jobid = msg['jobid']
        manPid = msg['manpid']
        g.activeJobs[jobid][manPid]['clipid'] = msg['clipid']
    elif msg['cmd'] == 'spawn':
        msg['cmd'] = 'mpdrun'  # handle much like an mpdrun from a console
        msg['mpdid_mpdrun_start'] = g.myId
        msg['nstarted_on_this_loop'] = 0
        msg['first_loop'] = 1
        msg['jobalias'] = ''
        msg['stdin_goes_to_who'] = '0'
        msg['ringsize'] = 0
        msg['ring_ncpus'] = 0
        msg['gdb'] = 0
        msg['totalview'] = 0
        mpd_send_one_msg(g.rhsSocket,msg)
        ## mpd_send_one_msg(manSocket, {'cmd' : 'mpdrun_ack', } )
    elif msg['cmd'] == 'publish_name':
        g.pmi_published_names[msg['service']] = msg['port']
        msgToSend = { 'cmd' : 'publish_result', 'info' : 'ok' }
        mpd_send_one_msg(manSocket,msgToSend)
    elif msg['cmd'] == 'lookup_name':
        if g.pmi_published_names.has_key(msg['service']):
            msgToSend = { 'cmd' : 'lookup_result', 'info' : 'ok',
                          'port' : g.pmi_published_names[msg['service']] }
            mpd_send_one_msg(manSocket,msgToSend)
        else:
            msg['cmd'] = 'pmi_lookup_name'    # add pmi_
            msg['src'] = g.myId
            msg['port'] = 0    # invalid
            mpd_send_one_msg(g.rhsSocket,msg)
    elif msg['cmd'] == 'unpublish_name':
        if g.pmi_published_names.has_key(msg['service']):
            del g.pmi_published_names[msg['service']]
            msgToSend = { 'cmd' : 'unpublish_result', 'info' : 'ok' }
            mpd_send_one_msg(manSocket,msgToSend)
        else:
            msg['cmd'] = 'pmi_unpublish_name'    # add pmi_
            msg['src'] = g.myId
            mpd_send_one_msg(g.rhsSocket,msg)
    else:
        mpd_print(1, 'INVALID request from man msg=:%s:' % (msg) )
        msgToSend = { 'cmd' : 'invalid_request' }
        mpd_send_one_msg(manSocket,msgToSend)

def _add_active_socket(socket,name,handler,host,ip,port):
    g.activeSockets[socket] = _ActiveSockInfo()
    g.activeSockets[socket].name    = name
    g.activeSockets[socket].handler = handler
    g.activeSockets[socket].host = host
    g.activeSockets[socket].ip   = ip
    g.activeSockets[socket].port = port

def _enter_existing_ring():
    # connect to lhs
    g.lhsHost = g.entryHost
    g.lhsIP   = g.entryIP
    g.lhsPort = g.entryPort
    inRing = 0
    g.generationFromMsg = -1    # default before loop below
    numEnterTries = 0
    while not inRing  and  numEnterTries < 8:
        numEnterTries += 1
        connected = 0
        numConnTries = 0
        while not connected  and  numConnTries < 8:
            numConnTries += 1
            g.lhsSocket = mpd_get_inet_socket_and_connect(g.lhsIP,g.lhsPort)
            if g.lhsSocket:
                connected = 1
            else:
                sleep(random())
        if not connected:
            continue
        _add_active_socket(g.lhsSocket,'lhs','_handle_lhs_input',
                           g.lhsHost,g.lhsIP,g.lhsPort)
        msgToSend = { 'cmd' : 'request_to_enter_as_rhs',
                      'host' : g.myHost,
                      'ip'   : g.myIP,
                      'port' : g.myPort,
                      'mpd_version' : mpd_version }
        mpd_send_one_msg(g.lhsSocket,msgToSend)
        msg = mpd_recv_one_msg(g.lhsSocket)
        if (not msg) or  \
           (not msg.has_key('cmd')) or  \
           (msg['cmd'] != 'challenge') or (not msg.has_key('randnum')) or  \
           (not msg.has_key('g.generation')):
            mpd_raise('invalid challenge msg: %s' % (msg) )
        g.generationFromMsg = int(msg['g.generation'])
        if g.generationFromMsg > g.generation:
            g.generation = g.generationFromMsg
            inRing = 1
        else:
            del g.activeSockets[g.lhsSocket]
            g.lhsSocket.close()
            sleep(random())
    if not inRing:
        if g.lhsSocket:
            msgToSend = { 'cmd' : 'challenge_response',
                          'host' : g.myHost,
                          'ip'   : g.myIP,
                          'port' : g.myPort,
                          'response' : 'bad_generation', 'gen' : g.generation }
            mpd_send_one_msg(g.lhsSocket,msgToSend)
            mpd_raise('Failed to enter ring at %s %s %d; my gen=%d other gen=%d ' % \
                      (g.lhsHost,g.lhsIP,g.lhsPort,g.generation,g.generationFromMsg) ) 
        else:
            return -1
    response = new(''.join([g.configParams['secretword'],msg['randnum']])).digest()
    msgToSend = { 'cmd' : 'challenge_response',
                  'response' : response,
                  'host' : g.myHost,
                  'ip'   : g.myIP,
                  'port' : g.myPort }
    mpd_send_one_msg(g.lhsSocket,msgToSend)
    msg = mpd_recv_one_msg(g.lhsSocket)
    if (not msg) or  \
       (not msg.has_key('cmd')) or (msg['cmd'] != 'OK_to_enter_as_rhs'):
        print('NOT OK to enter ring; one likely cause: mismatched secretwords')
        mpd_raise('failed to enter ring')
    if (not msg.has_key('rhshost'))  or (not msg.has_key('rhsport')):
        mpd_raise('invalid OK msg: %s' % (msg) )
    g.rhsHost = msg['rhshost']
    g.rhsIP   = msg['rhsip']
    g.rhsPort = int(msg['rhsport'])
    # connect to rhs
    g.rhsSocket = mpd_get_inet_socket_and_connect(g.rhsIP,g.rhsPort)
    if not g.rhsSocket:
        mpd_raise('unable to obtain socket for rhs in ring')
    _add_active_socket(g.rhsSocket,'rhs','_handle_rhs_input',
                       g.rhsHost,g.rhsIP,g.rhsPort)
    msgToSend = { 'cmd' : 'request_to_enter_as_lhs',
                  'host' : g.myHost,
                  'ip'   : g.myIP,
                  'port' : g.myPort }
    mpd_send_one_msg(g.rhsSocket,msgToSend)
    msg = mpd_recv_one_msg(g.rhsSocket)
    if (not msg) or  \
       (not msg.has_key('cmd')) or  \
       (msg['cmd'] != 'challenge') or (not msg.has_key('randnum')):
        mpd_raise('failed to recv challenge from rhs; msg=:%s:' % (msg) )
    response = new(''.join([g.configParams['secretword'],msg['randnum']])).digest()
    msgToSend = { 'cmd' : 'challenge_response',
                  'response' : response,
                  'host'     : g.myHost,
                  'ip'       : g.myIP,
                  'port'     : g.myPort }
    mpd_send_one_msg(g.rhsSocket,msgToSend)
    msg = mpd_recv_one_msg(g.rhsSocket)
    if (not msg) or  \
       (not msg.has_key('cmd')) or  \
       (msg['cmd'] != 'OK_to_enter_as_lhs'):
        mpd_raise('failed to enter ring; msg=:%s:' % (msg) )
    return 0

def _create_ring_of_one_mpd():
    # use a temp port and socket to avoid accidentally 
    #   accepting/handling connections by others
    (tempSocket,tempPort) = mpd_get_inet_listen_socket('',0)
    g.lhsSocket = mpd_get_inet_socket_and_connect(g.myIP,tempPort)
    g.lhsHost = g.myHost
    g.lhsIP   = g.myIP
    g.lhsPort = g.myPort
    _add_active_socket(g.lhsSocket,'lhs','_handle_lhs_input',
                       g.lhsHost,g.lhsIP,g.lhsPort)
    (g.rhsSocket,addr) = tempSocket.accept()
    g.rhsHost = g.myHost
    g.rhsIP   = g.myIP
    g.rhsPort = g.myPort
    _add_active_socket(g.rhsSocket,'rhs','_handle_rhs_input',
                       g.rhsHost,g.rhsIP,g.rhsPort)
    tempSocket.close()
    g.generation += 1

def _process_configfile_params():
    if getuid() == 0:    # if ROOT
        configFilename = '/etc/mpd.conf'
    else:
        configFilename = environ['HOME'] + '/.mpd.conf'
    try:
        mode = stat(configFilename)[0]
    except:
        mode = ''
    if not mode:
        # mpd_raise('%s: config file not found' % (configFilename) )
        print 'configuration file %s not found' % (configFilename)
        print 'A file named .mpd.conf file must be present in the user\'s home'
        print 'directory (/etc/mpd.conf if root) with read and write access'
        print 'only for the user, and must contain at least a line with:'
        print 'secretword=<secretword>'
        print 'One way to safely create this file is to do the following:'
        print '  cd $HOME'
        print '  touch .mpd.conf'
        print '  chmod 600 .mpd.conf'
        print 'and then use an editor to insert a line like'
        print '  secretword=mr45-j9z'
        print 'into the file.  (Of course use some other secret word than mr45-j9z.)' 
        exit(0)
    if  (mode & 0x3f):
        # mpd_raise('%s: config file accessible by others' % (configFilename) )
        print 'configuration file %s is accessible by others' % (configFilename)
        print 'change permissions to allow read and write access only by you'
        exit(0)
    configFileFD = open(configFilename,O_RDONLY)
    configFile = fdopen(configFileFD,'r',0)
    g.configParams = {}
    for line in configFile:
        line = line.rstrip()
        withoutComments = line.split('#')[0]
        splitLine = withoutComments.split('=')
        if len(splitLine) == 2:
            g.configParams[splitLine[0]] = splitLine[1]
        else:
            mpd_print(0, 'skipping config file line = :%s:' % (line) )
    # next check for backward compatibility
    if 'password' in g.configParams.keys() and 'secretword' not in g.configParams.keys():
        g.configParams['secretword'] = g.configParams['password']
    if 'secretword' not in g.configParams.keys():
        print 'configFile %s has no secretword' % (configFilename)
        print 'note: password has been replaced by secretword'
        exit(0)

def _process_cmdline_args():
    g.entryHost    = ''
    g.entryPort    = 0
    g.tracingMPD   = 0
    g.allowConsole = 1
    g.echoPortNum  = 0
    g.daemon       = 0
    g.pidFilename  = ''
    g.bulletproof  = 0
    g.listenPort   = 0
    g.NCPUs        = 1
    if g.configParams.has_key('if'):
        g.myHost   = g.configParams['if']
    else:
        g.myHost   = gethostname()
    try:
        (opts,args) = getopt(argv[1:],
                             'h:p:i:l:tnedb',
                             ['host=','port=','if=','listenport=','ncpus=','pid=',
                              'trace','noconsole','echo','daemon','bulletproof'])
    except:
        usage()

    for opt in opts:
        if   opt[0] == '-h'  or  opt[0] == '--host':
            g.entryHost = opt[1]
            try:
                g.entryIP = gethostbyname_ex(g.entryHost)[2][0]
            except:
                print 'mpd failed to enter ring; unknown host: %s' % (g.entryHost)
                usage()
        elif opt[0] == '-p'  or  opt[0] == '--port':
            g.entryPort = int(opt[1])
        elif opt[0] == '-i'  or  opt[0] == '--if':
            g.myHost = opt[1]
        elif opt[0] == '-l'  or  opt[0] == '--listenport':
            g.listenPort = int(opt[1])
        elif opt[0] == '-t'  or  opt[0] == '--trace':
            g.tracingMPD = 1
        elif opt[0] == '-n'  or  opt[0] == '--noconsole':
            g.allowConsole = 0
        elif opt[0] == '-e'  or  opt[0] == '--echo':
            g.echoPortNum = 1 
        elif opt[0] == '-d'  or  opt[0] == '--daemon':
            g.daemon = 1 
        elif opt[0] == '-b'  or  opt[0] == '--bulletproof':
            g.bulletproof = 1 
        elif opt[0] == '--ncpus':
            g.NCPUs = int(opt[1])
        elif opt[0] == '--pid':
            g.pidFilename = opt[1]
        else:
            pass    ## getopt raises an exception if not recognized
    if (g.entryHost and not g.entryPort) or (not g.entryHost and g.entryPort):
        print 'host and port must be specified together'
        exit(-1)
    try:
        hostinfo = gethostbyname_ex(g.myHost)
    except:
        print 'mpd failed: gethostbyname_ex failed for %s' % (g.myHost)
        exit(-1)
    g.myIP = hostinfo[2][0]

def sigchld_handler(signum,frame):
    done = 0
    while not done:
        try:
            (pid,status) = waitpid(-1,WNOHANG)
            if pid == 0:    # no existing child process is finished
                done = 1
        except:    # no more child processes to be waited for
            done = 1

def usage():
    print __doc__
    print "This version of mpd is", mpd_version
    exit(-1)

def _cleanup():
    try:
        mpd_print(0, "CLEANING UP" )
        syslog(LOG_INFO,"mpd ending mpdid=%s (inside _cleanup)" % (g.myId) )
        if g.conListenSocket in g.activeSockets:    # only delete if I put it there
            unlink(g.conListenName)
        closelog()
    except:
        pass


if __name__ == '__main__':
    try:
        g.myId = gethostname() + '_no_port_yet' + '_' + `getpid()`  # chgd later
        vinfo = mpd_check_python_version()
        if vinfo:
            print "mpd: your python version must be >= 2.2 ; current version is:", vinfo
            exit(-1)
        mpd_set_my_id(g.myId)  # chgd later
        proceduresToTrace = []
        for (symbol,symtype) in globals().items():
            if type(symtype) == FunctionType:
                proceduresToTrace.append(symbol)
        mpd_set_procedures_to_trace(proceduresToTrace)
        _process_configfile_params()
        _process_cmdline_args()
        if g.tracingMPD:
            settrace(mpd_trace_calls)
        register(_cleanup)
    
        _mpd_init()

        if g.bulletproof:
            try:
                from threading import Thread
            except:
                print '*** mpd terminating'
                print '    bulletproof option must be able to import threading-Thread'
                exit(-1)
            # may use SIG_IGN on all but SIGCHLD and SIGHUP (handled above)
            while 1:
                mpdtid = Thread(target=_mpd)
                mpdtid.start()
                # signals must be handled in main thread; thus we permit timeout of join
                while mpdtid.isAlive():
                    mpdtid.join(2)   # come out sometimes and handle signals
                if g.allExiting:
                    break
                if g.conSocket:
                    if g.activeSockets[g.conSocket]:
                        msgToSend = { 'cmd' : 'restarting_mpd' }
                        mpd_send_one_msg(g.conSocket,msgToSend)
                        del g.activeSockets[g.conSocket]
                    g.conSocket.close()
                    g.conSocket = 0
        else:
            #    import profile
            #    profile.run('_mpd()')
            _mpd()
    except mpdError, errmsg:
        print '%s failed ; cause: %s' % (g.myId,errmsg)
