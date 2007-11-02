#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
mpdstdcp is a program that is not in current use by mpd.
It is a demo of how we do some things in support of co-processes.
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.1 $"
__credits__ = ""


import os, re, socket, select
from mpdlib import *

def mpdstdcp():
    if os.environ.has_key('MPDCP_AM_MSHIP'):
        mpd_set_my_id(socket.gethostname() + '_copgm_mship_' + `os.getpid()`)
        mpd_print(0000, "BECOMING mothership" )
        mshipPort = int(os.environ['MPDCP_MSHIP_PORT'])
        mshipFD = int(os.environ['MPDCP_MSHIP_FD'])
        mshipNProcs = int(os.environ['MPDCP_MSHIP_NPROCS'])
        mshipSocket = socket.fromfd(mshipFD,socket.AF_INET,socket.SOCK_STREAM)
        numDone = 0
        socketsToSelect = { mshipSocket : 1 }
        while numDone < 1:    # just one satellite connected to me
            (readySockets,unused,unused) = select.select(socketsToSelect.keys(),[],[],30)
            for readySocket in readySockets:
                if readySocket == mshipSocket:
                    (newSocket,newConnAddr) = mshipSocket.accept()
                    socketsToSelect[newSocket] = 1
                    # print "MS: accepted from ", newConnAddr
                else:
                    msg = readySocket.recv(1024)
                    if not msg:
                        del socketsToSelect[readySocket]
                        numDone += 1
                    else:
                        print msg,
                        # print 'MS: %s' % (msg.strip())
        # print 'ENDING mothership'
    else:
        mpd_set_my_id(socket.gethostname() + '_copgm_satellite_' + `os.getpid()`)
        socketsToSelect = {}
        listenFD = int(os.environ['MPDCP_MY_LISTEN_FD'])
        listenSocket = socket.fromfd(listenFD,socket.AF_INET,socket.SOCK_STREAM)
        (manSocket,manConnAddr) = listenSocket.accept()
        socketsToSelect[manSocket] = 1
        myRank = int(os.environ['MPDCP_RANK'])
        mpd_set_my_id(socket.gethostname() + '_copgm_sat_%d_' % (myRank) + `os.getpid()`)
        if myRank == 0:
            mshipHost = os.environ['MPDCP_MSHIP_HOST']
            mshipPort = int(os.environ['MPDCP_MSHIP_PORT'])
            mpd_print(0000, "contacting mship on %s %d" % (mshipHost,mshipPort) )
            mshipSocket = mpd_get_inet_socket_and_connect(mshipHost,mshipPort)
        nprocs = int(os.environ['MPDCP_NPROCS'])

        # setup ring for PMI
        if myRank == 0:
            lhsRank = nprocs - 1
        else:
            lhsRank = myRank - 1
        msgToSend = { 'cmd' : 'get_copgm_listen_info',
                      'to_rank' : str(lhsRank),
                      'from_rank' : str(myRank) }
        mpd_send_one_msg(manSocket,msgToSend)
        msg = mpd_recv_one_msg(manSocket)
        if not msg  or  (not msg.has_key('cmd'))  or  (msg['cmd'] != 'copgm_listen_info'):
            mpd_print(1, 'failed to recv copgm_listen_info msg=:%s:')
            sys.exit(0)
        lhsHost = msg['host']
        lhsPort = int(msg['port'])
        lhsSocket = mpd_get_inet_socket_and_connect(lhsHost,lhsPort)
        socketsToSelect[lhsSocket] = 1
        msgToSend = { 'cmd' : 'new_rhs', 'from_rank' : str(myRank) }
        mpd_send_one_msg(lhsSocket,msgToSend)
        (rhsSocket,rhsConnAddr) = listenSocket.accept()
        msg = mpd_recv_one_msg(rhsSocket)
        if msg  and  msg.has_key('cmd')  and  msg['cmd'] == 'new_rhs' \
        and msg.has_key('from_rank'):
            if (int(msg['from_rank']) == (myRank+1))  \
            or (int(msg['from_rank']) == 0  and  myRank == (nprocs-1)):
                socketsToSelect[rhsSocket] = 1
        # and connect to the client telling it that we are providing 
        #   pmi service
        clientListenPort = int(os.environ['MPDCP_CLI_LISTEN_PORT'])
        pmiSocket = mpd_get_inet_socket_and_connect('localhost',clientListenPort)
        pmiFile = os.fdopen(pmiSocket.fileno(),'r')
        pmiSocket.sendall('cmd=pmi_handler\n')    # handshake
        socketsToSelect[pmiSocket] = 1
        default_kvs = {}

        # setup stdio tree
        (parent,lchild,rchild) = mpd_get_ranks_in_binary_tree(myRank,nprocs)
        lchildSocket = 0
        rchildSocket = 0
        if parent >= 0:
            msgToSend = { 'cmd' : 'get_copgm_listen_info',
                          'to_rank' : str(parent),
                          'from_rank' : str(myRank) }
            mpd_send_one_msg(manSocket,msgToSend)
            msg = mpd_recv_one_msg(manSocket)
            if not msg  or  (not msg.has_key('cmd'))  or  (msg['cmd'] != 'copgm_listen_info'):
                mpd_print(1, 'failed to recv copgm_listen_info msg=:%s:')
                sys.exit(0)
            parentHost = msg['host']
            parentPort = int(msg['port'])
            parentSocket = mpd_get_inet_socket_and_connect(parentHost,parentPort)
            msgToSend = { 'cmd' : 'child_in_tree', 'from_rank' : str(myRank) }
            mpd_send_one_msg(parentSocket,msgToSend)
        else:
            parentSocket = mshipSocket
        numToDo = 0
        numToAccept = len([x for x in [lchild,rchild] if x > 0])
        for i in range(numToAccept):
            (tempSocket,tempConnAddr) = listenSocket.accept()
            msg = mpd_recv_one_msg(tempSocket)
            if msg  and  msg.has_key('cmd')  and  msg['cmd'] == 'child_in_tree' \
            and msg.has_key('from_rank'):
                if int(msg['from_rank']) == lchild:
                    (lchildSocket,lchildConnAddr) = (tempSocket,tempConnAddr)
                    socketsToSelect[lchildSocket] = 1
                    lchildFile = os.fdopen(lchildSocket.fileno(),'r')
                    numToDo += 1
                elif int(msg['from_rank']) == rchild:
                    (rchildSocket,rchildConnAddr) = (tempSocket,tempConnAddr)
                    socketsToSelect[rchildSocket] = 1
                    rchildFile = os.fdopen(rchildSocket.fileno(),'r')
                    numToDo += 1
                else:
                    mpd_print(1, 'bad rank in child_in_tree msg %s' % msg['from_rank'])
            else:
                mpd_print(1, 'invalid msg recvd for child_in_tree :%s:' % msg )
        clientStdoutFD = int(os.environ['MPDCP_CLI_STDOUT_FD'])
        clientStdoutFile = os.fdopen(clientStdoutFD,'r')
        socketsToSelect[clientStdoutFD] = 1
        numToDo += 1

        pmiBarrierInRecvd = 0
        pmiBarrierLoop1Recvd = 0
        endBarrierLoop1Recvd = 0
        endBarrierDone = 0
        numDone = 0
        while not endBarrierDone:
            (inReadySockets,unused,unused) = select.select(socketsToSelect.keys(),[],[],30)
            for readySocket in inReadySockets:
                if readySocket not in socketsToSelect.keys():
                    continue
                if readySocket == clientStdoutFD:
                    line = clientStdoutFile.readline()
                    if not line:
                        del socketsToSelect[clientStdoutFD]
                        os.close(clientStdoutFD)
                        numDone += 1
                        if numDone >= numToDo:
                            if parentSocket:
                                parentSocket.close()
                                parentSocket = 0
                            if myRank == 0 or endBarrierLoop1Recvd:
                                endBarrierLoop1Recvd = 0
                                msgToSend = {'cmd' : 'end_barrier_loop_1'}
                                mpd_send_one_msg(rhsSocket,msgToSend)
                    else:
                        # parentSocket.sendall('STDOUT by %d: |%s|' % (myRank,line) )
                        parentSocket.sendall(line)
                elif readySocket == lchildSocket:
                    line = lchildFile.readline()
                    if not line:
                        del socketsToSelect[lchildSocket]
                        lchildSocket.close()
                        numDone += 1
                        if numDone >= numToDo:
                            if parentSocket:
                                parentSocket.close()
                                parentSocket = 0
                            if myRank == 0 or endBarrierLoop1Recvd:
                                endBarrierLoop1Recvd = 0
                                msgToSend = {'cmd' : 'end_barrier_loop_1'}
                                mpd_send_one_msg(rhsSocket,msgToSend)
                    else:
                        parentSocket.sendall(line)
                        # parentSocket.sendall('FWD by %d: |%s|' % (myRank,line) )
                elif readySocket == rchildSocket:
                    line = rchildFile.readline()
                    if not line:
                        del socketsToSelect[rchildSocket]
                        rchildSocket.close()
                        numDone += 1
                        if numDone >= numToDo:
                            if parentSocket:
                                parentSocket.close()
                                parentSocket = 0
                            if myRank == 0 or endBarrierLoop1Recvd:
                                endBarrierLoop1Recvd = 0
                                msgToSend = {'cmd' : 'end_barrier_loop_1'}
                                mpd_send_one_msg(rhsSocket,msgToSend)
                    else:
                        parentSocket.sendall(line)
                        # parentSocket.sendall('FWD by %d: |%s|' % (myRank,line) )
                elif readySocket == lhsSocket:
                    msg = mpd_recv_one_msg(lhsSocket)
                    if not msg:
                        del socketsToSelect[lhsSocket]
                        lhsSocket.close()
                    elif msg['cmd'] == 'end_barrier_loop_1':
                        if myRank == 0:
                            msgToSend = { 'cmd' : 'end_barrier_loop_2' }
                            mpd_send_one_msg(rhsSocket,msgToSend)
                        else:
                            if numDone >= numToDo:
                                msgToSend = {'cmd' : 'end_barrier_loop_1'}
                                mpd_send_one_msg(rhsSocket,msgToSend)
                            else:
                                endBarrierLoop1Recvd = 1
                    elif msg['cmd'] == 'end_barrier_loop_2':
                        endBarrierDone = 1
                        if myRank != 0:
                            mpd_send_one_msg(rhsSocket,msg)
                    elif msg['cmd'] == 'pmi_barrier_loop_1':
                        if myRank == 0:
                            msgToSend = { 'cmd' : 'pmi_barrier_loop_2' }
                            mpd_send_one_msg(rhsSocket,msgToSend)
                            pmiMsgToSend = 'cmd=barrier_out\n'
                            pmiSocket.sendall(pmiMsgToSend)
                        else:
                            pmiBarrierLoop1Recvd = 1
                            if pmiBarrierInRecvd:
                                mpd_send_one_msg(rhsSocket,msg)
                    elif msg['cmd'] == 'pmi_barrier_loop_2':
                        pmiBarrierInRecvd = 0
                        pmiBarrierLoop1Recvd = 0
                        if myRank != 0:
                            pmiMsgToSend = 'cmd=barrier_out\n'
                            pmiSocket.sendall(pmiMsgToSend)
                            mpd_send_one_msg(rhsSocket,msg)
                    elif msg['cmd'] == 'pmi_get':
                        if msg['from_rank'] == myRank:
                            pmiMsgToSend = 'cmd=get_result rc=-1 msg="%s"\n' % msg['key']
                            pmiSocket.sendall(pmiMsgToSend)
                            mpd_print(0000, "RMB: SENT pmimsg=:%s:" % pmiMsgToSend )
                        else:
                            kvsname = msg['kvsname']
                            key = msg['key']
                            cmd = 'value = ' + kvsname + '["' + key + '"]'
                            try:
                                exec(cmd)
                                gotit = 1
                            except Exception, errmsg:
                                gotit = 0
                            if gotit:
                                msgToSend = { 'cmd' : 'pmi_get_response', 'value' : value, 'to_rank' : msg['from_rank'] }
                                mpd_send_one_msg(rhsSocket,msgToSend)
                            else:
                                mpd_send_one_msg(rhsSocket,msg)
                    elif msg['cmd'] == 'pmi_get_response':
                        pmiMsgToSend = 'cmd=get_result rc=0 value=%s\n' % (msg['value'])
                        pmiSocket.sendall(pmiMsgToSend)
                    else:
                        mpd_print(1, 'unrecognized msg on lhs :%s:' % (msg) )
                elif readySocket == rhsSocket:
                    msg = mpd_recv_one_msg(rhsSocket)
                    if not msg:
                        del socketsToSelect[rhsSocket]
                        rhsSocket.close()
                    else:
                        mpd_print(1, 'unexpected msg from rhs :%s:' % (msg) )
                elif readySocket == pmiSocket:
                    line = pmiFile.readline()
                    mpd_print(0000, 'RMB: got msg from pmi :%s:' % (line) )
                    if not line:
                        del socketsToSelect[pmiSocket]
                        pmiSocket.close()
                    else:
                        parsedMsg = parse_pmi_msg(line)
                        if parsedMsg['cmd'] == 'get_my_kvsname':
                            pmiMsgToSend = 'cmd=my_kvsname kvsname=default_kvs\n'
                            pmiSocket.sendall(pmiMsgToSend)
			elif parsedMsg['cmd'] == 'get_maxes':
			    pmiMsgToSend = 'cmd=maxes kvsname_max=4096 ' + \
			                   'keylen_max=4096 vallen_max=4096\n'
                            pmiSocket.sendall(pmiMsgToSend)
                        elif parsedMsg['cmd'] == 'put':
                            kvsname = parsedMsg['kvsname']
                            key = parsedMsg['key']
                            value = parsedMsg['value']
                            cmd = kvsname + '["' + key + '"] = "' + value + '"'
                            mpd_print(0000, "RMB: cmd=:%s:" % cmd )
                            try:
                                exec(cmd)
                                mpd_print(0000, "RMB: finished cmd=:%s:" % cmd )
                                pmiMsgToSend = 'cmd=put_result rc=0\n'
                                pmiSocket.sendall(pmiMsgToSend)
                            except Exception, errmsg:
                                pmiMsgToSend = 'cmd=put_result rc=-1 msg="%s"\n' % errmsg
                                pmiSocket.sendall(pmiMsgToSend)
                        elif parsedMsg['cmd'] == 'barrier_in':
                            pmiBarrierInRecvd = 1
                            if myRank == 0  or  pmiBarrierLoop1Recvd:
                                msgToSend = { 'cmd' : 'pmi_barrier_loop_1' }
                                mpd_send_one_msg(rhsSocket,msgToSend)
                        elif parsedMsg['cmd'] == 'get':
                            kvsname = parsedMsg['kvsname']
                            key = parsedMsg['key']
                            cmd = 'value = ' + kvsname + '["' + key + '"]'
                            mpd_print(0000, "RMB: cmd=:%s:" % cmd )
                            try:
                                exec(cmd)
                                gotit = 1
                            except Exception, errmsg:
                                gotit = 0
                            if gotit:
                                pmiMsgToSend = 'cmd=get_result rc=0 value=%s\n' % (value)
                                pmiSocket.sendall(pmiMsgToSend)
                                mpd_print(0000, "RMB: SENT pmimsg=:%s:" % pmiMsgToSend )
                            else:
                                msgToSend = { 'cmd' : 'pmi_get', 'key' : key,
                                              'kvsname' : kvsname, 'from_rank' : myRank }
                                mpd_send_one_msg(rhsSocket,msgToSend)
                        else:
                            mpd_print(0000, "RMB: unrecognized pmi msg :%s:" % line )
                elif readySocket == manSocket:
                    mpd_print(1, 'unexpected msg from manSocket')
                    del socketsToSelect[manSocket]
                    manSocket.close()
                else:
                    mpd_print(1, 'msg on unexpected socket %s' % readySocket)
        mpd_print(0000, "out of loop")
        for openSocket in socketsToSelect:
            openSocket.close()
        if myRank == 0:
            mshipSocket.close()
    mpd_print(0000, "EXITING")

def parse_pmi_msg(msg):
    parsed_msg = {}
    sm = re.findall(r'\S+',msg)
    for e in sm:
        se = e.split('=')
        parsed_msg[se[0]] = se[1]
    return parsed_msg

if __name__ == '__main__':
    # mpdstdcp()
    print __doc__
    exit(-1)
    ## raise RuntimeError, 'mpdstdcp is not a stand-alone pgm'
