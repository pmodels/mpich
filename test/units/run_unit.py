import os
import sys
import re
import subprocess

class G:
    unit_name = None
    template_lines = []
    tests = []
    num_tests = 0
    num_failed = 0

class RE:
    m = None
    def match(pat, str, flags=0):
        RE.m = re.match(pat, str, flags)
        return RE.m

def main():
    os.chdir(sys.path[0])

    if not os.path.exists("out"):
        os.mkdir("out")

    argc = len(sys.argv)
    if argc > 1:
        args = sys.argv[1:]
    else:
        args = load_test_units()
    for a in args:
        G.unit_name = a
        G.template_lines = []
        G.tests = []
        load_unit(a + '.unit')

        os.chdir("out")
        if build_unit():
            run_tests()
        os.chdir("..")

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
                if RE.match(r'\s+cmd:\s*(.+)', line):
                    cur_test = {'cmd': RE.m.group(1), 'expect': None}
                    G.tests.append(cur_test)
                elif RE.match(r'\s+expect:\s*(.*)', line):
                    cur_test['expect'] = RE.m.group(1).rstrip()

def load_source(file_src, name):
    capture = False
    # assume cwd is test/units
    with open("../../" + file_src, "r") as In:
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

    if 'CC' in os.environ:
        cc = os.environ['CC']
    else:
        cc = 'gcc'

    ret = subprocess.run(cc + " -o t t.c", shell=True, encoding='ascii', errors='replace',
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    ok = ret.returncode == 0 and os.path.exists("t")
    report(ok, "%s: build" % G.unit_name, ret.stdout)
    return ok

def run_tests():
    for test in G.tests:
        ret = subprocess.run(test['cmd'], shell=True, encoding='ascii', errors='replace',
                             capture_output=True)
        output = ret.stdout.strip()
        diag = ""
        if ret.returncode:
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

def report(ok, desc, diag=""):
    """Print one TAP test line, with diagnostics as '#' lines when it fails"""
    G.num_tests += 1
    if ok:
        print("ok %d - %s" % (G.num_tests, desc))
    else:
        G.num_failed += 1
        print("not ok %d - %s" % (G.num_tests, desc))
        for line in diag.rstrip().split("\n"):
            print("#   %s" % line)

#---------------------------------------- 
if __name__ == "__main__":
    main()
