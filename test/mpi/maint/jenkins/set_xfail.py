import re
import sys
import os

class G:
    opts = {'jobname': '-', 'compiler': '-', 'jenkins_configure': '-', 'label': '-', 'netmod': '-'}
    states = []
    xfails = {}

class RE:
    m = None
    def match(pat, str, flags=0):
        RE.m = re.match(pat, str, flags)
        return RE.m
    def search(pat, str, flags=0):
        RE.m = re.search(pat, str, flags)
        return RE.m

def main():
    parse_args()
    load_xfail_conf()
    if os.path.exists('test/mpi'):
        os.chdir('test/mpi')
    apply_xfails()

# ---- subroutines --------------------------------------------
def parse_args():
    opt_names = {'j': "jobname", 'c': "compiler", 'o': "jenkins_configure", 'q': "label", 'm': "netmod", 'f': "XFAIL_CONF"}
    last_opt = ''
    for a in sys.argv[1:]:
        if last_opt:
            G.opts[last_opt] = a
            last_opt = ""
        elif RE.match(r'-(\w)$', a):
            if RE.m.group(1) in opt_names:
                last_opt = opt_names[RE.m.group(1)]
            else:
                last_opt = RE.m.group(1)
        elif RE.match(r'--(\w+)=(.*)', a):
            G.opts[RE.m.group(1)] = RE.m.group(2)
        elif os.path.exists(a):
            G.opts['XFAIL_CONF'] = a
        else:
            raise Exception("Unrecognized option [%s]\n" % a)

    if 'dir' in G.opts:
        os.chdir(G.opts['dir'])

    G.states = (G.opts['jobname'], G.opts['compiler'], G.opts['jenkins_configure'], G.opts['netmod'], G.opts['label'])

    if 'XFAIL_CONF' not in G.opts:
        raise Exception("%s: missing -f XFAIL_CONF\n" % 0)

    str_states = ' | '.join(G.states)
    print("set_xfail states: %s" % str_states)

def load_xfail_conf():
    with open(G.opts['XFAIL_CONF'], "r") as In:
        for line in In:
            if RE.match(r'^\s*#', line):
                pass
            elif RE.match(r'^\s*$', line):
                pass
            elif RE.match(r'\s*(.*?)\s*sed\s+-i\s*"([^"]+)"\s+(test.mpi.*)', line):
                # -- old "sed" pattern
                cond, pat, testlist = RE.m.group(1, 2, 3)
                if match_states(cond, G.states):
                    cond = re.sub(r'\s\s+', ' ', cond)
                    if testlist not in G.xfails:
                        G.xfails[testlist] = []
                    if RE.search(r's[\+\|]\\\((.*)\\\)[\+\|]\\1\s+xfail=(.*)[\+\|]g', pat):
                        G.xfails[testlist].append({'cond': cond, 'pat': RE.m.group(1), 'reason': RE.m.group(2)})
                    else:
                        raise Exception("Unrecognized xfail.conf rule: [%s]\n" % pat)
                else:
                    # print(" Unmatched state [%s] - [%s] - %s" % (cond, pat, testlist))
                    pass
            elif RE.match(r'\s*(.*?)\s*\/(.*)\/\s*xfail=(\w*)\s*(\S+)\s*$', line):
                # -- new direct pattern
                cond, pat, reason, testlist = RE.m.group(1, 2, 3, 4)
                testlist = re.sub(r'^test\/mpi\/', '', testlist)
                if match_states(cond, G.states):
                    cond = re.sub(r'\s\s+', ' ', cond)
                    if testlist not in G.xfails:
                        G.xfails[testlist] = []
                    G.xfails[testlist].append({'cond': cond, 'pat': pat, 'reason': reason})
                else:
                    # print(" Unmatched state [%s] - [%s] - %s" % (cond, pat, testlist))
                    pass
            else:
                print("Not parsed: [%s]" % line.rstrip())

def apply_xfails():
    for f in sorted (G.xfails.keys()):
        if not os.path.exists(f):
            print("! testlist: %s does not exist\n" % f)
            continue
        rules = G.xfails[f]
        lines = []
        n_applied = 0
        print("testlist: %s ..." % f)
        with open(f, "r") as In:
            for line in In:
                for r in rules:
                    if not RE.search(r'xfail=', line):
                        if RE.search(r['pat'], line):
                            print("    %s:\t%s\t-> xfail=%s" % (r['cond'], r['pat'], r['reason']))
                            if "dryrun" not in G.opts:
                                line = line.rstrip() + " xfail=%s\n" % (r['reason'])
                            n_applied+=1
                            break
                lines.append(line)

        if n_applied:
            with open(f, "w") as Out:
                for l in lines:
                    print(l, end='', file=Out)
        else:
            print("    %s not changed. Is there a mistake? Rules list:" % f, file=sys.stderr)
            for r in rules:
                print("        %s \t:%s" % (r['pat'], r['reason']), file=sys.stderr)

def match_states(cond, states):
    tlist = cond.split()
    for i in range(5):
        if not match(tlist[i], G.states[i]):
            return 0
    return 1

def match(pat, var):
    if pat == '*':
        return 1
    elif RE.match(pat, var):
        return 1
    else:
        return 0

# ---------------------------------------------------------
if __name__ == "__main__":
    main()
