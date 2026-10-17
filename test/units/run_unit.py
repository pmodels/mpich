import os
import sys
import re
import subprocess
import shlex
import argparse
import signal

class G:
    unit_name = None
    template_lines = []
    tests = []
    # optional blocks: real sources to compile, extra include dirs, and shim dirs
    sources = []
    includes = []
    shims = []
    unit_dir = None
    top_srcdir = None
    outdir = None
    cc = []
    cflags = []
    timeout = 60
    num_tests = 0
    num_failed = 0

class RE:
    m = None
    def match(pat, str, flags=0):
        RE.m = re.match(pat, str, flags)
        return RE.m

def main():
    G.unit_dir = os.path.abspath(sys.path[0])
    G.top_srcdir = os.path.dirname(os.path.dirname(G.unit_dir))
    # build under test/units/out of the current directory, e.g. the build tree
    # for "make unit-testing", or under out/ when started from test/units
    cwd = os.path.realpath(os.getcwd())
    if cwd == os.path.realpath(G.unit_dir):
        G.outdir = os.path.join(G.unit_dir, "out")
    else:
        G.outdir = os.path.join(cwd, "test", "units", "out")
    os.chdir(G.unit_dir)

    parser = argparse.ArgumentParser(description="Build and run the unit tests in test/units")
    parser.add_argument("units", nargs="*", help="units to run (default: all *.unit files)")
    parser.add_argument("--no-sanitize", action="store_true",
                        help="do not build with -fsanitize=address,undefined")
    parser.add_argument("--timeout", type=int, default=60,
                        help="seconds before a test is killed (default: 60)")
    opts = parser.parse_args()
    G.timeout = opts.timeout

    os.makedirs(G.outdir, exist_ok=True)

    G.cc = shlex.split(os.environ.get('CC', 'gcc'))
    G.cflags = ["-g", "-Wall"]
    if not opts.no_sanitize:
        os.chdir(G.outdir)
        G.cflags += probe_sanitizers()
        os.chdir(G.unit_dir)
    G.cflags += shlex.split(os.environ.get('UNIT_CFLAGS', ''))
    print("# %s" % ' '.join(G.cc + G.cflags))

    if opts.units:
        args = opts.units
    else:
        args = load_test_units()
    for a in args:
        a = re.sub(r'\.unit$', '', os.path.basename(a))
        G.unit_name = a
        G.template_lines = []
        G.tests = []
        G.sources = []
        G.includes = []
        G.shims = []
        load_unit(a + '.unit')

        # build and run each unit in its own directory
        unit_out = os.path.join(G.outdir, a)
        os.makedirs(unit_out, exist_ok=True)
        os.chdir(unit_out)
        if build_unit():
            run_tests()
        os.chdir(G.unit_dir)

    # TAP plan and summary
    print("1..%d" % G.num_tests)
    if G.num_failed:
        print("# FAILED %d of %d" % (G.num_failed, G.num_tests))
        sys.exit(1)
    else:
        print("# all %d passed" % G.num_tests)

def load_test_units():
    files = os.listdir()
    units = []
    for f in sorted(files):
        if RE.match(r'(\w+)\.unit', f):
            units.append(RE.m.group(1))
    return units

def load_unit(file_spec):
    cur_block = None
    cur_test = None
    in_expect = False

    with open(file_spec, "r") as In:
        for line in In:
            if RE.match(r'(\w+):', line):
                cur_block = RE.m.group(1)
            elif cur_block == 'template':
                if RE.match(r'\s+\[(\S+):(\w+)\]', line):
                    load_source(RE.m.group(1), RE.m.group(2))
                else:
                    # trim the leading indentation
                    G.template_lines.append(re.sub('^    ', '', line))
            elif cur_block == 'TESTS':
                if in_expect and RE.match(r'        \s*(\S.*)', line):
                    # a line indented deeper than "expect:" continues it
                    if cur_test['expect']:
                        cur_test['expect'] += "\n" + RE.m.group(1).rstrip()
                    else:
                        cur_test['expect'] = RE.m.group(1).rstrip()
                    continue
                in_expect = False
                if RE.match(r'\s+cmd:\s*(.+)', line):
                    cmd = RE.m.group(1).replace('$top_srcdir', shlex.quote(G.top_srcdir))
                    cur_test = {'cmd': cmd, 'expect': None, 'expect_err': None}
                    G.tests.append(cur_test)
                elif RE.match(r'\s+expect:\s*(.*)', line):
                    cur_test['expect'] = RE.m.group(1).rstrip()
                    in_expect = True
                elif RE.match(r'\s+expect_err:\s*(.*)', line):
                    cur_test['expect_err'] = RE.m.group(1).rstrip()
            elif cur_block in ('sources', 'include', 'shim'):
                item = re.sub(r'#.*', '', line).strip()
                if item:
                    if cur_block == 'sources':
                        G.sources.append(item)
                    elif cur_block == 'include':
                        G.includes.append(item)
                    else:
                        G.shims.append(item)

def load_source(file_src, name):
    capture = False
    with open(os.path.join(G.top_srcdir, file_src), "r") as In:
        for line in In:
            if RE.match(r'\s*\/\*\s*START UNIT\s*-\s*(\w+)', line) and RE.m.group(1) == name:
                capture = True
            elif RE.match(r'\s*\/\*\s*END UNIT', line):
                capture = False
            elif capture:
                G.template_lines.append(line)

def build_unit():
    with open("t.c", "w") as Out:
        for line in G.template_lines:
            Out.write(line)

    if os.path.exists("t"):
        os.remove("t")

    # shim dirs go first so that their headers replace the real ones; any .c
    # file in a shim dir is compiled in as well
    cmd = G.cc + G.cflags
    srcs = ["t.c"]
    for a in G.shims:
        shim_dir = os.path.join(G.unit_dir, "shim", a)
        if not os.path.isdir(shim_dir):
            report(False, "%s: build" % G.unit_name, "shim %s not found" % a)
            return False
        cmd.append("-I" + shim_dir)
        srcs += sorted(os.path.join(shim_dir, f) for f in os.listdir(shim_dir) if f.endswith(".c"))
    for a in G.includes:
        cmd.append("-I" + os.path.join(G.top_srcdir, a))
    for a in G.sources:
        srcs.append(os.path.join(G.top_srcdir, a))
    cmd += ["-o", "t"] + srcs

    ret = subprocess.run(cmd, encoding='utf-8', errors='replace',
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    ok = ret.returncode == 0 and os.path.exists("t")
    if ok:
        # show warnings, if any
        report(ok, "%s: build" % G.unit_name, ret.stdout)
    else:
        report(ok, "%s: build" % G.unit_name, ' '.join(cmd) + "\n" + ret.stdout)
    return ok

def probe_sanitizers():
    """Return the sanitizer flags if a test program builds and runs with them"""
    flags = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
    with open("probe.c", "w") as Out:
        Out.write("int main(void) { return 0; }\n")
    ret = subprocess.run(G.cc + flags + ["-o", "probe", "probe.c"], capture_output=True)
    if ret.returncode == 0:
        ret = subprocess.run(["./probe"], capture_output=True)
    if ret.returncode == 0:
        return flags
    else:
        print("# sanitizers are not available, building without them")
        return []

def run_tests():
    for test in G.tests:
        ret = run_cmd(test['cmd'])
        # ignore trailing whitespace on each line
        output = '\n'.join(l.rstrip() for l in ret.stdout.strip().split('\n'))
        diag = ""
        if ret.returncode is None:
            ok = False
            diag = "timed out after %d seconds\n" % G.timeout
        elif re.search(r'ERROR: \w+Sanitizer|runtime error:', ret.stderr):
            # a sanitizer report fails the test even if a failure is expected
            ok = False
        elif test['expect_err'] is not None:
            # expect a failure exit and the error message in stdout
            ok = ret.returncode > 0 and test['expect_err'] in output
            if not ok:
                diag = "exit code = %d, expect_err: %s\n" % (ret.returncode, test['expect_err'])
        elif ret.returncode:
            ok = False
            diag = "exit code = %d\n" % ret.returncode
        elif test['expect'] is not None and test['expect'] != output:
            ok = False
            diag = "expect: %s\n" % test['expect']
        else:
            ok = True
        if not ok:
            diag += "got: %s\n%s" % (output, ret.stderr)
        report(ok, "%s: %s" % (G.unit_name, test['cmd']), diag)

def run_cmd(cmd):
    """Run a test command in its own process group; returncode is None if it timed out"""
    proc = subprocess.Popen(cmd, shell=True, encoding='utf-8', errors='replace',
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            start_new_session=True)
    try:
        stdout, stderr = proc.communicate(timeout=G.timeout)
        return subprocess.CompletedProcess(cmd, proc.returncode, stdout, stderr)
    except subprocess.TimeoutExpired:
        os.killpg(proc.pid, signal.SIGKILL)
        stdout, stderr = proc.communicate()
        return subprocess.CompletedProcess(cmd, None, stdout, stderr)

def report(ok, desc, diag=""):
    """Print one TAP test line, followed by diagnostics as '#' lines"""
    G.num_tests += 1
    if ok:
        print("ok %d - %s" % (G.num_tests, desc))
    else:
        G.num_failed += 1
        print("not ok %d - %s" % (G.num_tests, desc))
    if diag.strip():
        for line in diag.rstrip().split("\n"):
            print("#   %s" % line)

#---------------------------------------- 
if __name__ == "__main__":
    main()
