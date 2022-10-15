import os
import sys
import re
import subprocess

class G:
    unit_name = None
    template_lines = []
    tests = []

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

def load_test_units():
    files = os.listdir()
    units = []
    for f in files:
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

    os.system(cc + " -o t t.c")

    return os.path.exists("t")


def run_tests():
    errs = 0
    print("Test - %s" % G.unit_name)
    for i, test in enumerate(G.tests):
        print("  %d:" % (i + 1))
        print("    %s" % test['cmd'])
        ret = subprocess.run(test['cmd'], shell=True, encoding='ascii', capture_output=True)
        if ret.returncode:
            print("    not ok, exit code = %d" % ret.returncode)
            print(ret.stdout)
        else:
            output = ret.stdout.strip()
            print("    %s" % output)
            if test['expect'] and test['expect'] != output:
                print("    not ok, expect: %s" % test['expect'])
                errs += 1
            else:
                print("    ok");
    return errs

#---------------------------------------- 
if __name__ == "__main__":
    main()
