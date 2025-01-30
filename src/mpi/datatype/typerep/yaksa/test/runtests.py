#! /usr/bin/env python3
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

import sys
import os
import time
import argparse
import subprocess
import signal
import datetime
import xml.etree.ElementTree as ET

class colors:
    FAILURE = '\033[1;31m' # red
    SUCCESS = '\033[1;32m' # green
    INFO    = '\033[1;33m' # yellow
    PREFIX  = '\033[1;36m' # cyan
    OTHER   = '\033[1;35m' # purple
    END     = '\033[0m'    # reset

opts = {"verbose": 0}

# junit variables
num_tests = 0
num_failures = 0
testnames = []
testtimes = []
testretvals = []
testoutputs = []


def init_colors():
    if not sys.stdout.isatty():
        colors.FAILURE = ''
        colors.SUCCESS = ''
        colors.INFO = ''
        colors.PREFIX = ''
        colors.OTHER = ''
        colors.END = ''


def printout_make(line, ret, elapsed_time, output):
    global num_tests
    global num_failures

    sys.stdout.write(colors.PREFIX + ">>>> " + colors.END)
    sys.stdout.write("make return status: ")
    if (ret == 0):
        sys.stdout.write(colors.SUCCESS + "SUCCESS\n" + colors.END)
    else:
        sys.stdout.write(colors.FAILURE + "FAILURE\n" + colors.END)
    sys.stdout.write(colors.PREFIX + ">>>> " + colors.END)
    sys.stdout.write("elapsed time: %f sec\n" % elapsed_time)
    if (ret != 0):
        num_tests = num_tests + 1
        num_failures = num_failures + 1
        if (output != ""):
            execname = line.split(' ', 1)[0].rstrip()
            print(colors.FAILURE + "\n==== \"make %s\" failed ====" % execname + colors.END)
            print(output)
            print(colors.FAILURE + "==== make output complete ====\n" + colors.END)
        testnames.append(line)
        testtimes.append(elapsed_time)
        testretvals.append(ret)
        testoutputs.append(output)


def printout_exec(line, ret, elapsed_time, output):
    global num_tests
    global num_failures

    num_tests = num_tests + 1
    sys.stdout.write(colors.PREFIX + ">>>> " + colors.END)
    sys.stdout.write("exec return status: ")
    if (ret == 0):
        sys.stdout.write(colors.SUCCESS + "SUCCESS\n" + colors.END)
    else:
        sys.stdout.write(colors.FAILURE + "FAILURE\n" + colors.END)
        num_failures = num_failures + 1
    sys.stdout.write(colors.PREFIX + ">>>> " + colors.END)
    sys.stdout.write("elapsed time: %f sec\n" % elapsed_time)
    if (ret != 0):
        if (output != ""):
            print(colors.FAILURE + "==== execution failed with the following output ====" + colors.END)
            print(output)
            print(colors.FAILURE + "==== execution output complete ====\n" + colors.END)

    testnames.append(line)
    testtimes.append(elapsed_time)
    testretvals.append(ret)
    testoutputs.append(output)


def getlines(fh):
    alllines = fh.readlines()
    reallines = []
    for line in alllines:
        # skip comments
        if line.startswith("#"):
            continue
        # skip empty lines
        if not line.strip():
            continue
        reallines.append(line)
    return reallines


def create_summary(summary_file):
    global num_tests
    global num_failures

    # open the summary file and write to it
    try:
        fh = open(summary_file, "w")
    except:
        sys.stderr.write(colors.FAILURE + ">>>> ERROR: " + colors.END)
        sys.stderr.write("could not open summary file %s\n" % summary_file)
        sys.exit()
    fh.write("<testsuites>\n")
    fh.write("  <testsuite failures=\"%d\"\n" % num_failures)
    fh.write("             errors=\"0\"\n")
    fh.write("             skipped=\"0\"\n")
    fh.write("             tests=\"%d\"\n" % num_tests)
    fh.write("             date=\"%s\"\n" % datetime.datetime.now())
    fh.write("             name=\"summary_junit_xml\">\n")

    for x in range(len(testnames)):
        fh.write("    <testcase name=\"%s\" time=\"%f\">\n" % (testnames[x].strip(), testtimes[x]))
        if (testretvals[x] != 0):
            fh.write("      <failure><![CDATA[\n")
            if (testoutputs[x]):
                fh.write(testoutputs[x] + "\n")
            else:
                fh.write("test failed\n")
            fh.write("      ]]></failure>\n")
        fh.write("    </testcase>\n")

    fh.write("  </testsuite>\n")
    fh.write("</testsuites>\n")
    fh.close()


def wait_with_signal(p):
    try:
        ret = p.wait()
    except:
        p.kill()
        p.wait()
        sys.exit()
    return ret


def run_testlist(testlist):
    try:
        fh = open(testlist, "r")
    except:
        sys.stderr.write(colors.FAILURE + ">>>> ERROR: " + colors.END)
        sys.stderr.write("could not open testlist %s\n" % testlist)
        sys.exit()

    print(colors.INFO + "\n==== executing testlist %s ====" % testlist + colors.END)

    lines = getlines(fh)

    firstline = 1
    for line in lines:
        if (firstline):
            firstline = 0
        else:
            print("")


        ############################################################################
        # if the first argument is a directory, step into the
        # directory and reexecute make
        ############################################################################
        dirname = line.split(' ', 1)[0].rstrip()
        if (os.path.isdir(dirname)):
            sys.stdout.write(colors.PREFIX + ">>>> " + colors.END)
            sys.stdout.write(colors.OTHER + "stepping into directory %s\n" % dirname + colors.END)
            olddir = os.getcwd()
            os.chdir(dirname)
            chdirargs = "make -s testing".split(' ')
            chdirargs = map(lambda s: s.strip(), chdirargs)
            p = subprocess.Popen(chdirargs)
            wait_with_signal(p)
            os.chdir(olddir)
            continue


        # command line to process
        sys.stdout.write(line)


        ############################################################################
        # make executable
        ############################################################################
        execname = line.split(' ', 1)[0].rstrip()
        if opts['verbose']:
            print("make " + execname)
        start = time.time()
        p = subprocess.Popen(['make', execname], stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
        ret = wait_with_signal(p)
        out = p.communicate()
        end = time.time()
        if opts['verbose']:
            print(out[0].decode().strip())
        printout_make(line, ret, end - start, out[0].decode().strip())
        if (ret != 0):
            continue  # skip over to the next line


        ############################################################################
        # run the executable
        ############################################################################
        fullcmd = "./" + line
        cmdargs = fullcmd.split(' ')
        cmdargs = map(lambda s: s.strip(), cmdargs)
        start = time.time()
        p = subprocess.Popen(cmdargs, stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
        ret = wait_with_signal(p)
        out = p.communicate()
        end = time.time()
        printout_exec(line, ret, end - start, out[0].decode().strip())

    fh.close()
    print(colors.INFO + "==== done executing testlist %s ====" % testlist + colors.END)


if __name__ == '__main__':
    init_colors()

    parser = argparse.ArgumentParser()
    parser.add_argument('testlists', help='testlist files to execute', nargs='+')
    parser.add_argument('--summary', help='file to write the summary to', required=True)
    args = parser.parse_args()

    if os.environ.get('V'):
        opts["verbose"] = 1

    for testlist in args.testlists:
        run_testlist(os.path.abspath(testlist))
    create_summary(os.path.abspath(args.summary))


