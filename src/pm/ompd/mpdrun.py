#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdrun [args] pgm_to_execute [pgm_args]
   where args may be: -a alias -np nprocs -cpm master_copgm -cpr remote_copgm -l -m -1 -s
       (nprocs must be a positive integer)
       (-l means attach line labels identifying which client prints each line)
       (-m means merge identical outputs into a single line;
           implies that program produces whole lines;
           implies -l)
       (-1 means do NOT start the first process locally)
       (-a means assign this alias to the job)
       (-s means send stdin to all processes; not just first)
       (-g means assume user will be running gdb and send some initial setup;
           implies -m and -l and initially -s );
       (-if is the interface to use on the local machine)
or:    mpdrun -f input_xml_filename [-r output_xml_exit_codes_filename]
   where filename contains all the arguments in xml format
"""

try:
    from signal          import signal, alarm, SIG_DFL, SIG_IGN, SIGINT, SIGTSTP, \
                                SIGCONT, SIGALRM, SIGKILL, SIGTTIN
except KeyboardInterrupt:
    exit(0)

from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.1 $"
__credits__ = ""


signal(SIGINT,SIG_IGN)
signal(SIGTSTP,SIG_IGN)
signal(SIGCONT,SIG_IGN)
signal(SIGTTIN,SIG_IGN)

from sys             import argv, exit, stdin, stdout, stderr
from os              import environ, fork, execvpe, getuid, getpid, path, getcwd, \
                            close, wait, waitpid, kill, unlink, system, _exit,  \
                            WIFSIGNALED, WEXITSTATUS, strerror
from pwd             import getpwnam
from socket          import socket, fromfd, AF_UNIX, SOCK_STREAM, gethostname, \
                            gethostbyname_ex, gethostbyaddr
from select          import select, error
from time            import time, sleep
from errno           import EINTR
from exceptions      import Exception
from re              import findall
from urllib          import unquote
from mpdlib          import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                            mpd_get_inet_listen_socket, mpd_get_my_username, \
                            mpd_raise, mpdError, mpd_version, mpd_print, \
                            mpd_read_one_line, mpd_send_one_line, mpd_recv, \
			    mpd_which
import xml.dom.minidom

class mpdrunInterrupted(Exception):
    def __init__(self,args=None):
        self.args = args

global nprocs, pgm, pgmArgs, mship, rship, argsFilename, delArgsFile, \
       try0Locally, lineLabels, jobAlias, mergingOutput, conSocket
global stdinGoesToWho, myExitStatus, manSocket, jobid, username, cwd, totalview
global outXmlDoc, outXmlEC, outXmlFile, linesPerRank, gdb, gdbAttachJobid
global execs, users, cwds, paths, args, envvars, limits, hosts, hostList
global singinitPID, singinitPORT, doingBNR, myHost, myIP, myIfhn


def mpdrun():
    global nprocs, pgm, pgmArgs, mship, rship, argsFilename, delArgsFile, \
           try0Locally, lineLabels, jobAlias, mergingOutput, conSocket
    global stdinGoesToWho, myExitStatus, manSocket, jobid, username, cwd, totalview
    global outXmlDoc, outXmlEC, outXmlFile, linesPerRank, gdb, gdbAttachJobid
    global execs, users, cwds, paths, args, envvars, limits, hosts, hostList
    global singinitPID, singinitPORT, doingBNR, myHost, myIP, myIfhn

    mpd_set_my_id('mpdrun_' + `getpid()`)
    pgm = ''
    mship = ''
    rship = ''
    nprocs = 0
    jobAlias = ''
    argsFilename = ''
    outExitCodesFilename = ''
    outXmlFile = ''
    outXmlDoc = ''
    outXmlEC = ''
    delArgsFile = 0
    try0Locally = 1
    lineLabels = 0
    stdinGoesToWho = '0'
    mergingOutput = 0
    hostList = []
    gdb = 0
    gdbAttachJobid = ''
    singinitPID  = 0
    singinitPORT = 0
    doingBNR = 0
    totalview = 0
    myHost = gethostname()   # default; may be chgd by -if arg
    myIfhn = ''
    known_rlimit_types = ['core','cpu','fsize','data','stack','rss',
                          'nproc','nofile','ofile','memlock','as','vmem']
    username = mpd_get_my_username()
    cwd = path.abspath(getcwd())
    recvTimeout = 20

    execs   = {}
    users   = {}
    cwds    = {}
    paths   = {}
    args    = {}
    envvars = {}
    limits  = {}
    hosts   = {}

    get_args_from_cmdline()    # verify args as much as possible before connecting to mpd

    (listenSocket,listenPort) = mpd_get_inet_listen_socket('',0)
    signal(SIGALRM,sig_handler)
    if environ.has_key('MPDRUN_TIMEOUT'):
        jobTimeout = int(environ['MPDRUN_TIMEOUT'])
    elif environ.has_key('MPIEXEC_TIMEOUT'):
        jobTimeout = int(environ['MPIEXEC_TIMEOUT'])
    else:
        jobTimeout = 0
    if environ.has_key('MPIEXEC_BNR'):
        doingBNR = 1
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
            print 'cannot connect to local mpd (%s); possible causes:' % consoleName
            print '    1. no mpd running on this host'
            print '    2. mpd is running but was started without a "console" (-n option)'
	    print 'you can start an mpd with the "mpd" command; to get help, run:'
	    print '    mpd -h'
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
            # mpd_raise('cannot connect to local mpd; errmsg: %s' % (str(errmsg)) )
        msgToSend = 'realusername=%s\n' % username
        mpd_send_one_line(conSocket,msgToSend)
        msgToSend = { 'cmd' : 'get_mpd_version' }
        mpd_send_one_msg(conSocket,msgToSend)
        msg = recv_one_msg_with_timeout(conSocket,recvTimeout)
        if not msg:
            mpd_raise('no msg recvd from mpd during version check')
        elif msg['cmd'] != 'mpd_version_response':
            mpd_raise('unexpected msg from mpd :%s:' % (msg) )
        if msg['mpd_version'] != mpd_version:
            mpd_raise('mpd version %s does not match mine %s' % (msg['mpd_version'],mpd_version) )

    if argsFilename:    # get these after we have a conn to mpd
        get_args_from_file()
    try:
        hostinfo = gethostbyname_ex(myHost)
    except:
        print 'mpd failed: gethostbyname_ex failed for %s' % (myHost)
        exit(-1)
    myIP = hostinfo[2][0]
    if gdbAttachJobid:
        get_vals_for_attach()
    elif not argsFilename:    # if only had cmd-line args
        if not nprocs:
            print 'you have to indicate how many processes to start'
            usage()
        execs   = { (0,nprocs-1) : pgm }
        users   = { (0,nprocs-1) : username }
        cwds    = { (0,nprocs-1) : cwd }
        paths   = { (0,nprocs-1) : environ['PATH'] }
        args    = { (0,nprocs-1) : pgmArgs }
        limits  = { (0,nprocs-1) : {} }
        cli_environ = {} ; cli_environ.update(environ)
        envvars = { (0,nprocs-1) : cli_environ }
        hosts   = { (0,nprocs-1) : '_any_' }
    else:
        pass    # args already defined by get_args_from_file

    if mship:
        (mshipSocket,mshipPort) = mpd_get_inet_listen_socket('',0)
        mshipPid = fork()
        if mshipPid == 0:
            conSocket.close()
            environ['MPDCP_AM_MSHIP'] = '1'
            environ['MPDCP_MSHIP_PORT'] = str(mshipPort)
            environ['MPDCP_MSHIP_FD'] = str(mshipSocket.fileno())
            environ['MPDCP_MSHIP_NPROCS'] = str(nprocs)
            try:
                execvpe(mship,[mship],environ)
            except Exception, errmsg:
                mpd_raise('execvpe failed for copgm %s; errmsg=:%s:' % (mship,errmsg) )
            _exit(0);  # do NOT do cleanup
        mshipSocket.close()
    else:
        mshipPid = 0

    # make sure to do this after nprocs has its value
    linesPerRank = {}  # keep this a dict instead of a list
    for i in range(nprocs):
        linesPerRank[i] = []

    msgToSend = { 'cmd'      : 'mpdrun',
                  'conhost'  : myHost,
                  'conip'    : myIP,
                  'conifhn'  : myIfhn,
                  'conport'  : listenPort,
                  'spawned'  : 0,
                  'nstarted' : 0,
                  'nprocs'   : nprocs,
                  'hosts'    : hosts,
                  'execs'    : execs,
                  'jobalias' : jobAlias,
                  'users'    : users,
                  'cwds'     : cwds,
                  'paths'    : paths,
                  'args'     : args,
                  'envvars'  : envvars,
                  'limits'   : limits,
                  'gdb'      : gdb,
                  'totalview'      : totalview,
                  'singinitpid'    : singinitPID,
                  'singinitport'   : singinitPORT,
                  'host_spec_pool' : hostList,
                }
    if try0Locally:
        msgToSend['try_0_locally'] = 1
    if lineLabels:
        msgToSend['line_labels'] = 1
    if rship:
        msgToSend['rship'] = rship
        msgToSend['mship_host'] = gethostname()
        msgToSend['mship_port'] = mshipPort
    if doingBNR:
        msgToSend['doing_bnr'] = 1
    if stdinGoesToWho == 'all':
        stdinGoesToWho = '0-%d' % (nprocs-1)
    msgToSend['stdin_goes_to_who'] = stdinGoesToWho

    mpd_send_one_msg(conSocket,msgToSend)
    msg = recv_one_msg_with_timeout(conSocket,recvTimeout)
    if not msg:
        mpd_raise('no msg recvd from mpd when expecting ack of request')
    elif msg['cmd'] == 'mpdrun_ack':
        currRingSize = msg['ringsize']
        currRingNCPUs = msg['ring_ncpus']
    else:
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
        elif msg['cmd'] == 'job_failed':
            if  msg['reason'] == 'some_procs_not_started':
                print 'mpdrun: unable to start all procs; may have invalid machine names'
                print '    remaining specified hosts:'
                for host in msg['remaining_hosts'].values():
                    if host != '_any_':
                        try:
                            print '        %s (%s)' % (host,gethostbyaddr(host)[0])
                        except:
                            print '        %s' % (host)
            elif  msg['reason'] == 'invalid_username':
                print 'mpdrun: invalid username %s at host %s' % \
                      (msg['username'],msg['host'])
            else:
                print 'mpdrun: job failed; reason=:%s:' % (msg['reason'])
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
        else:
            mpd_raise('unexpected message from mpd: %s' % (msg) )
    conSocket.close()
    if jobTimeout:
        alarm(jobTimeout)

    (manSocket,addr) = listenSocket.accept()
    msg = mpd_recv_one_msg(manSocket)
    if (not msg  or  not msg.has_key('cmd') or msg['cmd'] != 'man_checking_in'):
        mpd_raise('mpdrun: from man, invalid msg=:%s:' % (msg) )
    msgToSend = { 'cmd' : 'ring_ncpus', 'ring_ncpus' : currRingNCPUs,
                  'ringsize' : currRingSize }
    mpd_send_one_msg(manSocket,msgToSend)
    msg = mpd_recv_one_msg(manSocket)
    if (not msg  or  not msg.has_key('cmd')):
        mpd_raise('mpdrun: from man, invalid msg=:%s:' % (msg) )
    if (msg['cmd'] == 'job_started'):
        jobid = msg['jobid']
        if outXmlEC:
            outXmlEC.setAttribute('jobid',jobid.strip())
        # print 'mpdrun: job %s started' % (jobid)
        if totalview:
	    if not mpd_which('totalview'):
		print 'cannot find "totalview" in your $PATH:'
                print '    ', environ['PATH']
		myExitStatus = -1  # used in main
		exit(myExitStatus) # really forces jump back into main
            import mtv
            tv_cmd = 'dattach python ' + `getpid()` + '; dgo; dassign MPIR_being_debugged 1'
            system('totalview -e "%s" &' % (tv_cmd) )
            mtv.wait_for_debugger()
            mtv.allocate_proctable(nprocs)
            # extract procinfo (rank,hostname,exec,pid) tuples from msg
            for i in range(nprocs):
                host = msg['procinfo'][i][0]
                pgm  = msg['procinfo'][i][1]
                pid  = msg['procinfo'][i][2]
                # print "%d %s %s %d" % (i,host,pgm,pid)
                mtv.append_proctable_entry(host,pgm,pid)
            mtv.complete_spawn()
            msgToSend = { 'cmd' : 'tv_ready' }
            mpd_send_one_msg(manSocket,msgToSend)
    else:
        mpd_raise('mpdrun: from man, unknown msg=:%s:' % (msg) )

    (manCliStdoutSocket,addr) = listenSocket.accept()
    (manCliStderrSocket,addr) = listenSocket.accept()
    socketsToSelect = { manSocket : 1, manCliStdoutSocket : 1, manCliStderrSocket : 1,
                        stdin : 1 }
    signal(SIGINT,sig_handler)
    signal(SIGTSTP,sig_handler)
    signal(SIGCONT,sig_handler)

    timeDelayForPrints = 2  # seconds
    timeForPrint = time() + timeDelayForPrints   # to get started
    done = 0
    while done < 3:    # man, client stdout, and client stderr
        try:
            try:
                (readySockets,unused1,unused2) = select(socketsToSelect.keys(),[],[],1)
            except error, data:
                if data[0] == EINTR:    # interrupted by timeout for example
                    continue
                else:
                    mpd_raise('select error: %s' % strerror(data[0]))
            if mergingOutput:
                if timeForPrint < time():
                    print_ready_merged_lines(1)
                    timeForPrint = time() + timeDelayForPrints
                else:
                    print_ready_merged_lines(nprocs)
            for readySocket in readySockets:
                if readySocket == manSocket:
                    msg = mpd_recv_one_msg(manSocket)
                    if not msg:
                        del socketsToSelect[manSocket]
                        # manSocket.close()
                        tempManSocket = manSocket    # keep a ref to it
                        manSocket = 0
                        done += 1
                    elif not msg.has_key('cmd'):
                        mpd_raise('mpdrun: from man, invalid msg=:%s:' % (msg) )
                    elif msg['cmd'] == 'execution_problem':
                        # print 'rank %d (%s) in job %s failed to find executable %s' % \
                              # ( msg['rank'], msg['src'], msg['jobid'], msg['exec'] )
                        host = msg['src'].split('_')[0]
                        reason = unquote(msg['reason'])
                        print 'problem with execution of %s  on  %s:  %s ' % \
                              (msg['exec'],host,reason)
                        # keep going until all man's finish
                    elif msg['cmd'] == 'job_aborted_early':
                        print 'rank %d in job %s caused collective abort of all ranks' % \
                              ( msg['rank'], msg['jobid'] )
                        status = msg['exit_status']
                        if WIFSIGNALED(status):
                            if status > myExitStatus:
                                myExitStatus = status
                            killed_status = status & 0x007f  # AND off core flag
                            print '  exit status of rank %d: killed by signal %d ' % \
                                  (msg['rank'],killed_status)
                        else:
                            exit_status = WEXITSTATUS(status)
                            if exit_status > myExitStatus:
                                myExitStatus = exit_status
                            print '  exit status of rank %d: return code %d ' % \
                                  (msg['rank'],exit_status)
                    elif msg['cmd'] == 'job_aborted':
                        print 'job aborted; reason = %s' % (msg['reason'])
                    elif msg['cmd'] == 'client_exit_status':
                        if outXmlDoc:
                            outXmlProc = outXmlDoc.createElement('exit-code')
                            outXmlEC.appendChild(outXmlProc)
                            outXmlProc.setAttribute('rank',str(msg['cli_rank']))
                            outXmlProc.setAttribute('status',str(msg['cli_status']))
                            outXmlProc.setAttribute('pid',str(msg['cli_pid']))
                            outXmlProc.setAttribute('host',msg['cli_host'])
                        # print "exit info: rank=%d  host=%s  pid=%d  status=%d" % \
                              # (msg['cli_rank'],msg['cli_host'],
                               # msg['cli_pid'],msg['cli_status'])
                        status = msg['cli_status']
                        if WIFSIGNALED(status):
                            if status > myExitStatus:
                                myExitStatus = status
                            killed_status = status & 0x007f  # AND off core flag
                            # # print 'exit status of rank %d: killed by signal %d ' % (msg['cli_rank'],killed_status)
                        else:
                            exit_status = WEXITSTATUS(status)
                            if exit_status > myExitStatus:
                                myExitStatus = exit_status
                            # # print 'exit status of rank %d: return code %d ' % (msg['cli_rank'],exit_status)
                    else:
                        print 'unrecognized msg from manager :%s:' % msg
                elif readySocket == manCliStdoutSocket:
                    if mergingOutput:
                        line = mpd_read_one_line(manCliStdoutSocket.fileno())
                        if not line:
                            del socketsToSelect[readySocket]
                            done += 1
                        else:
                            if gdb:
                                line = line.replace('(gdb)\n','(gdb) ')
                            try:
                                (rank,rest) = line.split(':',1)
                                rank = int(rank)
                                linesPerRank[rank].append(rest)
                            except:
                                print line
                            print_ready_merged_lines(nprocs)
                    else:
                        msg = mpd_recv(manCliStdoutSocket,1024)
                        if not msg:
                            del socketsToSelect[readySocket]
                            # readySocket.close()
                            done += 1
                        else:
                            stdout.write(msg)
                            stdout.flush()
                elif readySocket == manCliStderrSocket:
                    msg = mpd_recv(manCliStderrSocket,1024)
                    if not msg:
                        del socketsToSelect[readySocket]
                        # readySocket.close()
                        done += 1
                    else:
                        # print >>stderr, msg,
                        # print >>stderr, 'MS: %s' % (msg.strip())
                        stderr.write(msg)
                        stderr.flush()
                elif readySocket == stdin:
                    try:
                        line = stdin.readline()
                    except IOError:
                        stdin.flush()  # probably does nothing
                    else:
                        if line:    # not EOF
                            msgToSend = { 'cmd' : 'stdin_from_user', 'line' : line } # default
                            if gdb and line.startswith('z'):
                                line = line.rstrip()
                                if len(line) < 3:    # just a 'z'
                                    line += ' 0-%d' % (nprocs-1)
                                s1 = line[2:].rstrip().split(',')
                                for s in s1:
                                    s2 = s.split('-')
                                    for i in s2:
                                        if not i.isdigit():
                                            print 'invalid arg to z :%s:' % i
                                            continue
                                msgToSend = { 'cmd' : 'stdin_goes_to_who',
                                              'stdin_procs' : line[2:] }
                                stdout.softspace = 0
                                print '%s:  (gdb) ' % (line[2:]),
                            elif gdb and line.startswith('q'):
                                msgToSend = { 'cmd' : 'stdin_goes_to_who','stdin_procs' : '0-%d' % (nprocs-1) }
                                if manSocket:
                                    mpd_send_one_msg(manSocket,msgToSend)
                                msgToSend = { 'cmd' : 'stdin_from_user','line' : 'q\n' }
                            elif gdb and line.startswith('^'):
                                msgToSend = { 'cmd' : 'stdin_goes_to_who','stdin_procs' : '0-%d' % (nprocs-1) }
                                if manSocket:
                                    mpd_send_one_msg(manSocket,msgToSend)
                                msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGINT' }
                            if manSocket:
                                mpd_send_one_msg(manSocket,msgToSend)
                        else:
                            del socketsToSelect[stdin]
                            stdin.close()
                            if manSocket:
                                mpd_send_one_msg(manSocket,{ 'cmd' : 'stdin_from_user', 'eof' : '' })
                else:
                    mpd_raise('unrecognized ready socket :%s:' % (readySocket) )
        except mpdError, errmsg:
            print 'mpdrun failed: %s' % (errmsg)
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
        except mpdrunInterrupted, errmsg:
            if errmsg.args == 'SIGINT':
                if manSocket:
                    msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGINT' }
                    mpd_send_one_msg(manSocket,msgToSend)
                    # next code because no longer exiting
                    ### del socketsToSelect[manSocket]
                    ### # manSocket.close()
                    ### tempManSocket = manSocket
                    ### manSocket = 0
                    ### done += 1
                # exit(-1)
                if not gdb:
                    myExitStatus = -1  # used in main
                    exit(myExitStatus) # really forces jump back into main
            elif errmsg.args == 'SIGTSTP':
                if manSocket:
                    msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGTSTP' }
                    mpd_send_one_msg(manSocket,msgToSend)
                signal(SIGTSTP,SIG_DFL)      # stop myself
                kill(getpid(),SIGTSTP)
                signal(SIGTSTP,sig_handler)  # restore this handler
            elif errmsg.args == 'SIGALRM':
                mpd_print(1, 'mpdrun terminating due to timeout %d seconds' % \
                          (jobTimeout))
                if manSocket:
                    msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGKILL' }
                    mpd_send_one_msg(manSocket,msgToSend)
                    manSocket.close()
                myExitStatus = -1  # used in main
                exit(myExitStatus) # really forces jump back into main

    if mergingOutput:
        print_ready_merged_lines(1)
    if mshipPid:
        (donePid,status) = wait()    # waitpid(mshipPid,0)
    if outXmlFile:
        print >>outXmlFile, outXmlDoc.toprettyxml(indent='   ')
        outXmlFile.close()

def sig_handler(signum,frame):
    # for some reason, I (rmb) was unable to handle TSTP and CONT in the same way
    global manSocket
    if signum == SIGINT:
        raise mpdrunInterrupted, 'SIGINT'
    elif signum == SIGTSTP:
        raise mpdrunInterrupted, 'SIGTSTP'
    elif signum == SIGCONT:
        if manSocket:
            msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGCONT' }
            mpd_send_one_msg(manSocket,msgToSend)
    elif signum == SIGALRM:
        raise mpdrunInterrupted, 'SIGALRM'

def recv_one_msg_with_timeout(sock,timeout):
    oldTimeout = alarm(timeout)
    msg = mpd_recv_one_msg(sock)    # fails WITHOUT a msg if sigalrm occurs
    alarm(oldTimeout)
    return(msg)

def format_sorted_ranks(ranks):
    all = []
    one = []
    prevRank = -999
    for i in range(len(ranks)):
        if i != 0  and  ranks[i] != (prevRank+1):
            all.append(one)
            one = []
        one.append(ranks[i])
        if i == (len(ranks)-1):
            all.append(one)
        prevRank = ranks[i]
    pline = ''
    for i in range(len(all)):
        if len(all[i]) > 1:
            pline += '%d-%d' % (all[i][0],all[i][-1])
        else:
            pline += '%d' % (all[i][0])
        if i != (len(all)-1):
            pline += ','
    return pline

def print_ready_merged_lines(minRanks):
    global nprocs, linesPerRank
    printFlag = 1  # default to get started
    while printFlag:
        printFlag = 0
        for r1 in range(nprocs):
            if not linesPerRank[r1]:
                continue
            sortedRanks = []
            lineToPrint = linesPerRank[r1][0]
            for r2 in range(nprocs):
                if linesPerRank[r2] and linesPerRank[r2][0] == lineToPrint: # myself also
                    sortedRanks.append(r2)
            if len(sortedRanks) >= minRanks:
                fsr = format_sorted_ranks(sortedRanks)
                stdout.softspace = 0
                print '%s: %s' % (fsr,lineToPrint),
                for r2 in sortedRanks:
                    linesPerRank[r2] = linesPerRank[r2][1:]
                printFlag = 1
    stdout.flush()

def get_args_from_cmdline():
    global nprocs, pgm, pgmArgs, mship, rship, argsFilename, delArgsFile, try0Locally, \
           lineLabels, jobAlias, stdinGoesToWho, jobid, mergingOutput, totalview
    global outXmlDoc, outXmlEC, outXmlFile, gdb, gdbAttachJobid
    global singinitPID, singinitPORT, doingBNR, myHost, myIP, myIfhn

    if len(argv) < 3:
        usage()
    argidx = 1
    if argv[1] == '-delxmlfile':  # special case for mpiexec
        delArgsFile = 1
        argsFilename = argv[2]   # initialized to '' in main
        argidx = 3
    elif argv[1] == '-f':
        argsFilename = argv[2]   # initialized to '' in main
        argidx += 2
        if len(argv) > 3:
            if len(argv) > 5  or  argv[3] != '-r':
                print '-r is the only arg that can be used with -f'
                usage()
            else:
                outExitCodesFilename = argv[4]   # initialized to '' in main
                outXmlFile = open(outExitCodesFilename,'w')
                outXmlDoc = xml.dom.minidom.Document()
                outXmlEC = outXmlDoc.createElement('exit-codes')
                outXmlDoc.appendChild(outXmlEC)
                argidx += 2
    elif argv[1] == '-ga':
        if len(argv) != 3:
            print '-ga (and its jobid) must be the only cmd-line args'
            usage()
        gdb = 1
        mergingOutput = 1   # implied
        lineLabels = 1      # implied
        stdinGoesToWho = 'all'    # chgd to 0 - nprocs-1 when nprocs avail
        gdbAttachJobid = argv[2]
        return
    elif argv[1] == '-p':
        singinitPID = argv[2]
        singinitPORT = argv[3]
        pgm = argv[4]
        nprocs = 1
        pgmArgs = []
        try0Locally = 1
        return
    if not argsFilename:
        while pgm == '':
            if argidx >= len(argv):
                usage()
            if argv[argidx][0] == '-':
                if argv[argidx] == '-np' or argv[argidx] == '-n':
                    if not argv[argidx+1].isdigit():
                        print 'non-numeric arg to -n or -np'
                        usage()
                    else:
                        nprocs = int(argv[argidx+1])
                        if nprocs < 1:
                            usage()
                        else:
                            argidx += 2
                elif argv[argidx] == '-f':
                    print '-f must be first and only -r can appear with it'
                    usage()
                elif argv[argidx] == '-r':
                    outExitCodesFilename = argv[argidx+1]   # initialized to '' in main
                    outXmlFile = open(outExitCodesFilename,'w')
                    outXmlDoc = xml.dom.minidom.Document()
                    outXmlEC = outXmlDoc.createElement('exit-codes')
                    outXmlDoc.appendChild(outXmlEC)
                    argidx += 2
                elif argv[argidx] == '-a':
                    jobAlias = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-if':
                    myIfhn = argv[argidx+1]
                    myHost = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-cpm':
                    mship = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-bnr':
                    doingBNR = 1
                    argidx += 1
                elif argv[argidx] == '-cpr':
                    rship = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-l':
                    lineLabels = 1
                    argidx += 1
                elif argv[argidx] == '-m':
                    mergingOutput = 1
                    lineLabels = 1  # implied
                    argidx += 1
                elif argv[argidx] == '-g':
                    gdb = 1
                    mergingOutput = 1   # implied
                    lineLabels = 1      # implied
                    stdinGoesToWho = 'all'    # chgd to 0 - nprocs-1 when nprocs avail
                    argidx += 1
                elif argv[argidx] == '-1' or argv[argidx] == '-nolocal':
                    try0Locally = 0
                    argidx += 1
                elif argv[argidx] == '-s':
                    stdinGoesToWho = 'all'    # chgd to 0 - nprocs-1 when nprocs avail
                    argidx += 1
                elif argv[argidx] == '-tv':
                    totalview = 1
                    argidx += 1
                else:
                    usage()
            else:
                pgm = argv[argidx]
                argidx += 1
    pgmArgs = []
    while argidx < len(argv):
        pgmArgs.append(argv[argidx])
        argidx += 1

def get_args_from_file():
    global nprocs, pgm, pgmArgs, mship, rship, argsFilename, delArgsFile, \
           try0Locally, lineLabels, jobAlias, mergingOutput, conSocket
    global stdinGoesToWho, myExitStatus, manSocket, jobid, username, cwd, totalview
    global outXmlDoc, outXmlEC, outXmlFile, linesPerRank, gdb, gdbAttachJobid
    global execs, users, cwds, paths, args, envvars, limits, hosts, hostList
    global singinitPID, singinitPORT, doingBNR, myHost, myIP

    try:
        argsFile = open(argsFilename,'r')
    except:
        print 'could not open job specification file %s' % (argsFilename)
        myExitStatus = -1  # used in main
        exit(myExitStatus) # really forces jump back into main
    file_contents = argsFile.read()
    if delArgsFile:
        unlink(argsFilename)
    try: 
        from xml.dom.minidom import parseString   #import only if needed
    except:
        print 'need xml parser like xml.dom.minidom'
        myExitStatus = -1  # used in main
        exit(myExitStatus) # really forces jump back into main
    parsedArgs = parseString(file_contents)
    if parsedArgs.documentElement.tagName != 'create-process-group':
        print 'expecting create-process-group; got unrecognized doctype: %s' % \
              (parsedArgs.documentElement.tagName)
        myExitStatus = -1  # used in main
        exit(myExitStatus) # really forces jump back into main
    createReq = parsedArgs.getElementsByTagName('create-process-group')[0]
    if createReq.hasAttribute('totalprocs'):
        nprocs = int(createReq.getAttribute('totalprocs'))
    else:
        print '** totalprocs not specified in %s' % argsFilename
        myExitStatus = -1  # used in main
        exit(myExitStatus) # really forces jump back into main
    if createReq.hasAttribute('dont_try_0_locally'):
        try0Locally = 0
    if createReq.hasAttribute('output')  and  \
       createReq.getAttribute('output') == 'label':
        lineLabels = 1
    if createReq.hasAttribute('net_interface'):
        myHost = createReq.getAttribute('net_interface')
        myIfhn = myHost
    if createReq.hasAttribute('pgid'):    # our jobalias
        jobAlias = createReq.getAttribute('pgid')
    if createReq.hasAttribute('stdin_goes_to_who'):
        stdinGoesToWho = createReq.getAttribute('stdin_goes_to_who')
    if createReq.hasAttribute('doing_bnr'):
        doingBNR = int(createReq.getAttribute('doing_bnr'))
    if createReq.hasAttribute('gdb'):
        gdb = int(createReq.getAttribute('gdb'))
        if gdb:
            mergingOutput = 1   # implied
            lineLabels = 1      # implied
            stdinGoesToWho = 'all'    # chgd to 0 - nprocs-1 when nprocs avail
    if createReq.hasAttribute('tv'):
        totalview = int(createReq.getAttribute('tv'))

    nextHost = 0
    hostSpec = createReq.getElementsByTagName('host-spec')
    if hostSpec:
        for node in hostSpec[0].childNodes:
            node = node.data.strip()
            hostnames = findall(r'\S+',node)
            for hostname in hostnames:
                if hostname:    # some may be the empty string
                    try:
                        ipaddr = gethostbyname_ex(hostname)[2][0]
                    except:
                        print 'unable to determine IP info for host %s' % (hostname)
                        myExitStatus = -1  # used in main
                        exit(myExitStatus) # really forces jump back into main
                    if ipaddr.startswith('127.0.0'):
                        hostList.append(myHost)
                    else:
                        hostList.append(ipaddr)
    if hostSpec and hostSpec[0].hasAttribute('check'):
        hostSpecMode = hostSpec[0].getAttribute('check')
        if hostSpecMode == 'yes':
            msgToSend = { 'cmd' : 'verify_hosts_in_ring', 'host_list' : hostList }
            mpd_send_one_msg(conSocket,msgToSend)
            msg = recv_one_msg_with_timeout(conSocket,5)
            if not msg:
                mpd_raise('no msg recvd from mpd mpd during chk hosts up')
            elif msg['cmd'] != 'verify_hosts_in_ring_response':
                mpd_raise('unexpected msg from mpd :%s:' % (msg) )
            if msg['host_list']:
                print 'These hosts are not in the mpd ring:'
                for host in  msg['host_list']:
                    if host[0].isdigit():
                        print '    %s' % (host),
                        try:
                            print ' (%s)' % (gethostbyaddr(host)[0])
                        except:
                            print ''
                    else:
                        print '    %s' % (host)
                myExitStatus = -1  # used in main
                exit(myExitStatus) # really forces jump back into main

    covered = [0] * nprocs 
    procSpec = createReq.getElementsByTagName('process-spec')
    if not procSpec:
        print 'No process-spec specified'
        usage()
    for p in procSpec:
        if p.hasAttribute('range'):
            therange = p.getAttribute('range')
            splitRange = therange.split('-')
            if len(splitRange) == 1:
                loRange = int(splitRange[0])
                hiRange = loRange
            else:
                (loRange,hiRange) = (int(splitRange[0]),int(splitRange[1]))
        else:
            (loRange,hiRange) = (0,nprocs-1)
        for i in xrange(loRange,hiRange+1):
            if i >= nprocs:
                print '*** exiting; rank %d is greater than nprocs for args' % (i)
                myExitStatus = -1  # used in main
                exit(myExitStatus) # really forces jump back into main
            if covered[i]:
                print '*** exiting; rank %d is doubly used in proc specs' % (i)
                myExitStatus = -1  # used in main
                exit(myExitStatus) # really forces jump back into main
            covered[i] = 1
        if p.hasAttribute('exec'):
            execs[(loRange,hiRange)] = p.getAttribute('exec')
        else:
            print '*** exiting; range %d-%d has no exec' % (loRange,hiRange)
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
        if p.hasAttribute('user'):
            tempuser = p.getAttribute('user')
            try:
                pwent = getpwnam(tempuser)
            except:
                pwent = None
            if not pwent:
                print tempuser, 'is an invalid username'
                myExitStatus = -1  # used in main
                exit(myExitStatus) # really forces jump back into main
            if tempuser == username  or  getuid() == 0:
                users[(loRange,hiRange)] = p.getAttribute('user')
            else:
                print tempuser, 'username does not match yours and you are not root'
                myExitStatus = -1  # used in main
                exit(myExitStatus) # really forces jump back into main
        else:
            users[(loRange,hiRange)] = username
        if p.hasAttribute('cwd'):
            cwds[(loRange,hiRange)] = p.getAttribute('cwd')
        else:
            cwds[(loRange,hiRange)] = cwd
        if p.hasAttribute('path'):
            paths[(loRange,hiRange)] = p.getAttribute('path')
        else:
            paths[(loRange,hiRange)] = environ['PATH']
        if p.hasAttribute('host'):
            host = p.getAttribute('host')
            if host.startswith('_any_'):
                hosts[(loRange,hiRange)] = host
            else:
                try:
                    hosts[(loRange,hiRange)] = gethostbyname_ex(host)[2][0]
                except:
                    print 'unable to do find info for host %s' % (host)
                    myExitStatus = -1  # used in main
                    exit(myExitStatus) # really forces jump back into main
        else:
            if hostList:
                hosts[(loRange,hiRange)] = '_any_from_pool_'
            else:
                hosts[(loRange,hiRange)] = '_any_'

        argDict = {}
        argList = p.getElementsByTagName('arg')
        for argElem in argList:
            argDict[int(argElem.getAttribute('idx'))] = argElem.getAttribute('value')
        argVals = [0] * len(argList)
        for i in argDict.keys():
            argVals[i-1] = unquote(argDict[i])
        args[(loRange,hiRange)] = argVals

        limitDict = {}
        limitList = p.getElementsByTagName('limit')
        for limitElem in limitList:
            type = limitElem.getAttribute('type')
            if type in known_rlimit_types:
                limitDict[type] = limitElem.getAttribute('value')
            else:
                print 'mpdrun: invalid type in limit: %s' % (type)
                myExitStatus = -1  # used in main
                exit(myExitStatus) # really forces jump back into main
        limits[(loRange,hiRange)] = limitDict

        envVals = {}
        envVarList = p.getElementsByTagName('env')
        for envVarElem in envVarList:
            envkey = envVarElem.getAttribute('name')
            envval = envVarElem.getAttribute('value')
            envVals[envkey] = envval
        envvars[(loRange,hiRange)] = envVals


def get_vals_for_attach():
    global nprocs, pgm, pgmArgs, mship, rship, argsFilename, delArgsFile, \
           try0Locally, lineLabels, jobAlias, mergingOutput, conSocket
    global stdinGoesToWho, myExitStatus, manSocket, jobid, username, cwd, totalview
    global outXmlDoc, outXmlEC, outXmlFile, linesPerRank, gdb, gdbAttachJobid
    global execs, users, cwds, paths, args, envvars, limits, hosts, hostList
    global singinitPID, singinitPORT, doingBNR, myHost, myIP

    sjobid = gdbAttachJobid.split('@')    # jobnum and originating host
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
    got_info = 0
    while 1:
        msg = mpd_recv_one_msg(conSocket)
        if not msg.has_key('cmd'):
            print 'mpdlistjobs: INVALID msg=:%s:' % (msg)
            exit(-1)
        if msg['cmd'] == 'mpdlistjobs_info':
            got_info = 1
            smjobid = msg['jobid'].split('  ')  # jobnum, mpdid, and alias (if present)
            if sjobid[0] == smjobid[0]  and  sjobid[1] == smjobid[1]:  # jobnum and mpdid
                rank = int(msg['rank'])
                users[(rank,rank)]   = msg['username']
                hosts[(rank,rank)]   = msg['host']
                execs[(rank,rank)]   = msg['pgm']
                cwds[(rank,rank)]    = cwd
                paths[(rank,rank)]   = environ['PATH']
                args[(rank,rank)]    = [msg['clipid']]
                envvars[(rank,rank)] = {}
                limits[(rank,rank)]  = {}
        elif  msg['cmd'] == 'mpdlistjobs_trailer':
            if not got_info:
                print 'no info on this jobid; probably invalid'
                exit(-1)
            break
        else:
            print 'invaild msg from mpd :%s:' % (msg)
            exit(-1)
    nprocs = len(execs.keys())    # all dicts are the same len here


def usage():
    global myExitStatus
    print 'mpdrun for mpd version: %s' % str(mpd_version)
    print __doc__
    myExitStatus = -1  # used in main
    exit(myExitStatus) # really forces jump back into main


if __name__ == '__main__':

    global manSocket, myExitStatus

    manSocket = 0    # set when we get conn'd to a manager
    myExitStatus = 0

    try:
        mpdrun()
    except mpdError, errmsg:
        print 'mpdrun failed: %s' % (errmsg)
    except SystemExit, errmsg:
        pass
    exit(myExitStatus)
