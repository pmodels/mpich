#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage:
mpiexec [-h or -help or --help]    # get this message
mpiexec -file filename             # filename contains XML job description
mpiexec [global args] [local args] executable [args]
   where global args may be
      -l                           # line labels by MPI rank
      -if                          # network interface to use locally
      -kx                          # keep generated xml for debugging
      -bnr                         # MPICH1 compatibility mode
      -g<local arg name>           # global version of local arg (below)
      -machinefile                 # file mapping procs to machines
      -s <spec>                    # direct stdin to "all" or 1,2 or 2-4,6 
      -gdb                         # run procs under gdb
      -gdba jobid                  # gdb-attach to existing jobid
    and local args may be
      -n <n> or -np <n>            # number of processes to start
      -wdir <dirname>              # working directory to start in
      -path <dirname>              # place to look for executables
      -host <hostname>             # host to start on
      -soft <spec>                 # modifier of -n value
      -arch <arch>                 # arch type to start on (not implemented)
      -envall                      # pass all env vars in current environment
      -envnone                     # pass no env vars
      -envlist <list of env var names> # pass current values of these vars
      -env <name> <value>          # pass this value of this env var
mpiexec [global args] [local args] executable args : [local args] executable...
mpiexec -configfile filename       # filename contains cmd line segs as lines
  (See User Guide for more details)

Examples:
   mpiexec -l -n 10 cpi 100
   mpiexec -genv QPL_LICENSE 4705 -n 3 a.out
   mpiexec -n 1 -host foo master : -n 4 -host mysmp slave
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.1 $"
__credits__ = ""


from sys    import argv, exit
from os     import environ, execvpe, getpid, getuid, getcwd, access, X_OK, path, unlink, \
                   open as osopen, fdopen, O_CREAT, O_WRONLY, O_EXCL, O_RDONLY
from popen2 import Popen3
from pwd    import getpwuid
from urllib import quote
import re
import xml.dom.minidom

global totalProcs, nextRange, argvCopy, configLines, configIdx, appnum
global validGlobalArgs, globalArgs, validLocalArgs, localArgSets

def mpiexec():
    global totalProcs, nextRange, argvCopy, configLines, configIdx, appnum
    global validGlobalArgs, globalArgs, validLocalArgs, localArgSets

    validGlobalArgs = { '-l' : 0, '-usize' : 1, '-gdb' : 0, '-gdba' : 1, '-bnr' : 0, '-tv' : 0,
                        '-if' : 1, '-machinefile' : 1, '-kx' : 0, '-s' : 1,
                        '-gn' : 1, '-gnp' : 1, '-ghost' : 1, '-gpath' : 1, '-gwdir' : 1,
			'-gsoft' : 1, '-garch' : 1, '-gexec' : 1,
			'-genvall' : 0, '-genv' : 2, '-genvnone' : 0,
			'-genvlist' : 1 }
    validLocalArgs  = { '-n' : 1, '-np' : 1, '-host' : 1, '-path' : 1, '-wdir' : 1,
                        '-soft' : 1, '-arch' : 1,
			'-envall' : 0, '-env' : 2, '-envnone' : 0, '-envlist' : 1 }

    globalArgs   = {}
    localArgSets = {}
    localArgSets[0] = []

    totalProcs    = 0
    nextRange     = 0
    configLines   = []
    configIdx     = 0
    xmlForArgsets = []
    appnum        = 0

    if len(argv) < 2  or  argv[1] == '-h'  or  argv[1] == '-help'  or  argv[1] == '--help':
	usage()
    fullDirName = path.abspath(path.split(argv[0])[0])  # normalize for platform also
    mpdrun = path.join(fullDirName,'mpdrun.py')
    if not access(mpdrun,X_OK):
        print 'mpiexec: cannot execute mpdrun %s' % mpdrun
        exit(0);
    if argv[1] == '-file':
	if len(argv) != 3:
	    usage()
        xmlFilename = argv[2]
        globalArgs['-kx'] = 1
    else:
        if argv[1] == '-gdba':
            if len(argv) != 3:
                print '-gdba must be used only with a jobid'
                usage()
            execvpe(mpdrun,[mpdrun,'-ga',argv[2]],environ)
        elif argv[1] == '-configfile':
	    if len(argv) != 3:
	        usage()
            configFileFD = osopen(argv[2],O_RDONLY)
            configFile = fdopen(configFileFD,'r',0)
            configLines = configFile.readlines()
            configLines = [ x.strip() + ' : '  for x in configLines if x[0] != '#' ]
            tempargv = []
            for line in configLines:
                shOut = Popen3("/bin/sh -c 'for a in $*; do echo _$a; done' -- %s" % (line))
                for shline in shOut.fromchild:
                    tempargv.append(shline[1:].strip())    # 1: strips off the leading _
	    tempargv = [argv[0]] + tempargv[0:-1]   # strip off the last : I added
            collect_args(tempargv)
        else:
            collect_args(argv)

        machineFileInfo = read_machinefile(globalArgs['-machinefile'])
        xmlDOC = xml.dom.minidom.Document()
        xmlCPG = xmlDOC.createElement('create-process-group')
        xmlDOC.appendChild(xmlCPG)
        for k in localArgSets.keys():
            handle_argset(localArgSets[k],xmlDOC,xmlCPG,machineFileInfo)
        xmlCPG.setAttribute('totalprocs', str(totalProcs) )  # after handling argsets
        if globalArgs['-l']:
            xmlCPG.setAttribute('output', 'label')
        if globalArgs['-if']:
            xmlCPG.setAttribute('net_interface', globalArgs['-if'])
        if globalArgs['-s']:
            xmlCPG.setAttribute('stdin_goes_to_who', globalArgs['-s'])
        if globalArgs['-bnr']:
            xmlCPG.setAttribute('doing_bnr', '1')
        if globalArgs['-gdb']:
            xmlCPG.setAttribute('gdb', '1')
        if globalArgs['-tv']:
            xmlCPG.setAttribute('tv', '1')
        submitter = getpwuid(getuid())[0]
        xmlCPG.setAttribute('submitter', submitter)
        xmlFilename = '/tmp/%s_tempxml_%d' % (submitter,getpid())
        try:    unlink(xmlFilename)
	except: pass
        xmlFileFD = osopen(xmlFilename,O_CREAT|O_WRONLY|O_EXCL,0600)
        xmlFile = fdopen(xmlFileFD,'w',0)
        print >>xmlFile, xmlDOC.toprettyxml(indent='   ')
        # print xmlDOC.toprettyxml(indent='   ')    #### RMB: TEMP DEBUG
        xmlFile.close()
    if globalArgs['-kx']:
        execvpe(mpdrun,[mpdrun,'-f',xmlFilename],environ)
    else:
        execvpe(mpdrun,[mpdrun,'-delxmlfile',xmlFilename],environ)
    print 'mpiexec: exec failed for %s' % mpdrun
    exit(0);

def collect_args(args):
    global validGlobalArgs, globalArgs, validLocalArgs, localArgSets
    globalArgs['-l']        = 0
    globalArgs['-if']       = ''
    globalArgs['-s']        = '0'
    globalArgs['-kx']       = 0
    globalArgs['-usize']    = 0
    globalArgs['-gdb']      = 0
    globalArgs['-bnr']      = 0
    globalArgs['-tv']       = 0
    globalArgs['-machinefile'] = ''
    globalArgs['-gn']       = 1
    globalArgs['-ghost']    = '_any_'
    globalArgs['-gpath']    = environ['PATH']
    globalArgs['-gwdir']    = path.abspath(getcwd())
    globalArgs['-gsoft']    = 0
    globalArgs['-garch']    = ''
    globalArgs['-gexec']    = ''
    globalArgs['-genv']     = {}
    globalArgs['-genvlist'] = []
    globalArgs['-genvnone'] = 0
    argidx = 1
    while argidx < len(args)  and  args[argidx] in validGlobalArgs.keys():
        garg = args[argidx]
	if garg == '-gnp':    # alias for '-gn'
	    garg = '-gn'
	if garg == '-gdba':
	    print '-gdba must appear alone with jobid'
            exit(-1)
        if validGlobalArgs[garg] > 0:
            if garg == '-genv':
                globalArgs['-genv'][args[argidx+1]] = args[argidx+2]
                argidx += 3
            else:
		if garg == '-garch':
                    print '** -garch is accepted but not used'
		if garg == '-usize'  or  garg == '-gn':
                    if args[argidx+1].isdigit():
                        globalArgs[garg] = int(args[argidx+1])
                    else:
                        print 'argument to %s must be numeric' % (garg)
                        usage()
		else:
                    globalArgs[garg] = args[argidx+1]
                argidx += 2
        else:
            globalArgs[garg] = 1
            argidx += 1
    if argidx < len(args)  and  args[argidx] == ':':
        argidx += 1
    localArgsKey = 0
    while argidx < len(args):
        if args[argidx] == '-ghost'  or  args[argidx] == '-host':
            if globalArgs['-machinefile']:
                print '-host (or -ghost) and -machinefile are incompatible'
                exit(-1)
        if args[argidx] == ':':
            localArgsKey += 1
            localArgSets[localArgsKey] = []
        else:
            localArgSets[localArgsKey].append(args[argidx])
        argidx += 1

def handle_argset(argset,xmlDOC,xmlCPG,machineFileInfo):
    global totalProcs, nextRange, argvCopy, configLines, configIdx, appnum
    global validGlobalArgs, globalArgs, validLocalArgs, localArgSets

    host   = globalArgs['-ghost']
    wdir   = globalArgs['-gwdir']
    wpath  = globalArgs['-gpath']
    nProcs = globalArgs['-gn']
    usize  = globalArgs['-usize']
    gexec  = globalArgs['-gexec']
    softness =  globalArgs['-gsoft']
    if globalArgs['-genvnone']:
        envall = 0
    else:
        envall = 1
    if globalArgs['-genvlist']:
        globalArgs['-genvlist'] = globalArgs['-genvlist'].split(',')
    localEnvlist = []
    localEnv  = {}
    
    argidx = 0
    while argidx < len(argset):
        if argset[argidx] not in validLocalArgs:
            if argset[argidx][0] == '-':
                if validGlobalArgs.has_key(argset[argidx]):
                    print 'global arg %s must not appear among local args' % \
                          (argset[argidx])
                else:
                    print 'unknown option: %s' % argset[argidx]
                usage()
            break                       # since now at executable
        if argset[argidx] == '-n' or argset[argidx] == '-np':
            if len(argset) < (argidx+2):
                print '** missing arg to -n'
                usage()
            nProcs = argset[argidx+1]
            if not nProcs.isdigit():
                print '** non-numeric arg to -n: %s' % nProcs
                usage()
            nProcs = int(nProcs)
            argidx += 2
        elif argset[argidx] == '-host':
            if len(argset) < (argidx+2):
                print '** missing arg to -host'
                usage()
            host = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-path':
            if len(argset) < (argidx+2):
                print '** missing arg to -path'
                usage()
            wpath = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-wdir':
            if len(argset) < (argidx+2):
                print '** missing arg to -wdir'
                usage()
            wdir = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-soft':
            if len(argset) < (argidx+2):
                print '** missing arg to -soft'
                usage()
            softness = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-arch':
            if len(argset) < (argidx+2):
                print '** missing arg to -arch'
                usage()
            print '** -arch is accepted but not used'
            argidx += 2
        elif argset[argidx] == '-envall':
            envall = 1
            argidx += 1
        elif argset[argidx] == '-envnone':
            envall = 0
            argidx += 1
        elif argset[argidx] == '-envlist':
            localEnvlist = argset[argidx+1].split(',')
            argidx += 2
        elif argset[argidx] == '-env':
            if len(argset) < (argidx+3):
                print '** missing arg to -env'
                usage()
            var = argset[argidx+1]
            val = argset[argidx+2]
            localEnv[var] = val
            argidx += 3
        else:
            print 'unknown option: %s' % argset[argidx]
            usage()

    if softness:
        nProcs = adjust_nprocs(nProcs,softness)

    cmdAndArgs = []
    if argidx < len(argset):
        while argidx < len(argset):
            cmdAndArgs.append(argset[argidx])
            argidx += 1
    else:
        if gexec:
            cmdAndArgs = [gexec]
    if not cmdAndArgs:
        print 'no cmd specified'
        usage()

    argsetLoRange = nextRange
    argsetHiRange = nextRange + nProcs - 1
    loRange = argsetLoRange
    hiRange = argsetHiRange

    defaultHostForArgset = host
    while loRange <= argsetHiRange:
        host = defaultHostForArgset
        ifhn = ''
        if machineFileInfo:
            if len(machineFileInfo) <= hiRange:
                print 'too few entries in machinefile'
                exit(-1)
            host = machineFileInfo[loRange]['host']
            ifhn = machineFileInfo[loRange]['ifhn']
            for i in range(loRange+1,hiRange+1):
                if machineFileInfo[i]['host'] != host  or  machineFileInfo[i]['ifhn'] != ifhn:
                    hiRange = i - 1
                    break

        xmlPROCSPEC = xmlDOC.createElement('process-spec')
        xmlCPG.appendChild(xmlPROCSPEC)
        xmlPROCSPEC.setAttribute('user',getpwuid(getuid())[0])
        xmlPROCSPEC.setAttribute('exec',cmdAndArgs[0])
        xmlPROCSPEC.setAttribute('path',wpath)
        xmlPROCSPEC.setAttribute('cwd',wdir)
        xmlPROCSPEC.setAttribute('host',host)
        xmlPROCSPEC.setAttribute('range','%d-%d' % (loRange,hiRange))

        for i in xrange(1,len(cmdAndArgs[1:])+1):
            arg = cmdAndArgs[i]
            xmlARG = xmlDOC.createElement('arg')
            xmlPROCSPEC.appendChild(xmlARG)
            xmlARG.setAttribute('idx', '%d' % (i) )
            xmlARG.setAttribute('value', '%s' % (quote(arg)))

        envToSend = {}
        if envall:
            for envvar in environ.keys():
                envToSend[envvar] = environ[envvar]
        for envvar in globalArgs['-genvlist']:
            if not environ.has_key(envvar):
                print '%s in envlist does not exist in your env' % (envvar)
                exit(-1)
            envToSend[envvar] = environ[envvar]
        for envvar in localEnvlist:
            if not environ.has_key(envvar):
                print '%s in envlist does not exist in your env' % (envvar)
                exit(-1)
            envToSend[envvar] = environ[envvar]
        for envvar in globalArgs['-genv'].keys():
            envToSend[envvar] = globalArgs['-genv'][envvar]
        for envvar in localEnv.keys():
            envToSend[envvar] = localEnv[envvar]
        for envvar in envToSend.keys():
            xmlENVVAR = xmlDOC.createElement('env')
            xmlPROCSPEC.appendChild(xmlENVVAR)
            xmlENVVAR.setAttribute('name',  '%s' % (envvar))
            xmlENVVAR.setAttribute('value', '%s' % (envToSend[envvar]))
        if usize:
            xmlENVVAR = xmlDOC.createElement('env')
            xmlPROCSPEC.appendChild(xmlENVVAR)
            xmlENVVAR.setAttribute('name', 'MPI_UNIVERSE_SIZE')
            xmlENVVAR.setAttribute('value', '%s' % (usize))
        xmlENVVAR = xmlDOC.createElement('env')
        xmlPROCSPEC.appendChild(xmlENVVAR)
        xmlENVVAR.setAttribute('name', 'MPI_APPNUM')
        xmlENVVAR.setAttribute('value', '%s' % str(appnum))
        if ifhn:
            xmlENVVAR.setAttribute('name', 'MPICH_INTERFACE_HOSTNAME')
            xmlENVVAR.setAttribute('value', '%s' % (ifhn))

        loRange = hiRange + 1
        hiRange = argsetHiRange  # again

    appnum += 1
    nextRange += nProcs
    totalProcs += nProcs

# Adjust nProcs (called maxprocs in the Standard) according to soft:
# Our interpretation is that we need the largest number <= nProcs that is
# consistent with the list of possible values described by soft.  I.e.
# if the user says
#
#   mpiexec -n 10 -soft 5 a.out
#
# we adjust the 10 down to 5.  This may not be what was intended in the Standard,
# but it seems to be what it says.

def adjust_nprocs(nProcs,softness):
    biglist = []
    list1 = softness.split(',')
    for triple in list1:                # triple is a or a:b or a:b:c
        thingy = triple.split(':')     
        if len(thingy) == 1:
            a = int(thingy[0])
            if a <= nProcs and a >= 0:
                biglist.append(a)
        elif len(thingy) == 2:
            a = int(thingy[0])
            b = int(thingy[1])
            for i in range(a,b+1):
                if i <= nProcs and i >= 0:
                    biglist.append(i)
        elif len(thingy) == 3:
            a = int(thingy[0])
            b = int(thingy[1])
            c = int(thingy[2])
            for i in range(a,b+1,c):
                if i <= nProcs and i >= 0:
                    biglist.append(i)                
        else:
            print 'invalid subargument to -soft: %s' % (softness)
            print 'should be a or a:b or a:b:c'
            usage()

        if len(biglist) == 0:
            print '-soft argument %s allows no valid number of processes' % (softness)
            usage()
        else:
            return max(biglist)


def read_machinefile(machineFilename):
    if not machineFilename:
        return None
    try:
        machineFile = open(machineFilename,'r')
    except:
        print '** unable to open machinefile'
        exit(-1)
    procID = 0
    machineFileInfo = {}
    for line in machineFile:
        line = line.strip()
        if not line  or  line[0] == '#':
            continue
        splitLine = re.split(r'\s+',line)
        host = splitLine[0]
        if ':' in host:
            (host,nprocs) = host.split(':',1)
            nprocs = int(nprocs)
        else:
            nprocs = 1
        kvps = {'ifhn' : ''}
        for kv in splitLine[1:]:
            (k,v) = kv.split('=',1)
            if k == 'ifhn':  # interface hostname
                kvps[k] = v
            else:  # may be other kv pairs later
                print 'unrecognized key in machinefile:', k
                exit(-1)
        for i in range(procID,procID+nprocs):
            machineFileInfo[i] = { 'host' : host, 'nprocs' : nprocs }
            machineFileInfo[i].update(kvps)
        procID += nprocs
    return machineFileInfo


def usage():
    print __doc__
    exit(-1)


if __name__ == '__main__':
    try:
        mpiexec()
    except SystemExit, errmsg:
        pass

