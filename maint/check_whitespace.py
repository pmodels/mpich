#! /usr/bin/env python3
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# Checks C sources for the layout that maint/code-cleanup.bash (GNU indent
# 2.2.11) produces, without needing indent. It reads whole files and reports
# problems as "file:line:col: CODE message"; it never rewrites a file.
#
# Usage: maint/check_whitespace.py [--all] [--rule CODE,...] [files...]
#
# Rules:
#   A1 trailing whitespace          A2 tab character
#   A3 keyword spacing              A4 space inside () or []
#   A5 space before a call's (      A6 space before ; or ,
#   A7 space after a cast           A8 space around a pointer '*'
#   A9 space around operators       A10 space after ,
#   A11 double space between words
#   B1 if/else braces               B2 do/while braces
#   B3 struct and initializer braces
#   B4 function body brace          B5 one statement per line
#   C1 statement indentation        C2 case indentation
#   C3 label indentation            C4 top-level indentation
#   C5 preprocessor directive placement
#   D1 comment indentation          D3 comment continuation lines
#   D4 trailing comment column      D6 comment column after a directive
#   E1 blank line after a function body

import re
import subprocess
import sys

FILE_TYPES = r"\.c$|\.h$|\.c\.in$|\.h\.in$"

# The ignore list of maint/code-cleanup.bash (less the files no longer in the tree)
# plus the exclusions in maint/hooks/pre-commit.
IGNORE_LIST = [
    r"doc/",
    r"src/mpid/ch3/doc",
    r"src/mpid/ch3/include",
    r"src/mpid/ch3/src",
    r"src/mpid/ch3/util",
    r"src/mpid/ch3/channels/nemesis/include",
    r"src/mpid/ch3/channels/nemesis/src",
    r"src/mpid/ch3/channels/nemesis/utils",
    r"test/mpi/errors/f77/io/addsize.h.in",
    r"test/mpi/errors/f77/io/iooffset.h.in",
    r"test/mpi/f77/attr/attraints.h.in",
    r"test/mpi/f77/datatype/typeaints.h.in",
    r"test/mpi/f77/ext/add1size.h.in",
    r"test/mpi/f77/io/ioaint.h.in",
    r"test/mpi/f77/io/iodisp.h.in",
    r"test/mpi/f77/io/iooffset.h.in",
    r"test/mpi/f77/pt2pt/attr1aints.h.in",
    r"test/mpi/f77/rma/addsize.h.in",
    r"test/mpi/f77/spawn/type1aint.h.in",
    r"src/include/mpi.h.in",
    r"src/include/mpix.h",
    r"src/include/mpi_legacy.h",
    r"src/binding/fortran/mpif_h/mpi_fortran.h.in",
    r"src/mpi/romio/include/mpio.h.in",
    r"src/mpi/romio/adio/include/adioi_errmsg.h",
    r"src/binding/abi/mpi_abi.h",
    # maint/hooks/pre-commit
    r"mpi\.h\.in$",
    r"mpio\.h\.in$",
    r"mpitestcxx\.h$",
    r"^src/mpid/ch3/",
    r"^doc/",
]

ALL_RULES = ["A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "A10", "A11",
             "B1", "B2", "B3", "B4", "B5", "C1", "C2", "C3", "C4", "C5", "D1", "D3", "D4", "D6",
             "E1"]

KEYWORDS = {
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else",
    "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register",
    "restrict", "return", "short", "signed", "sizeof", "static", "struct", "switch",
    "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof",
    "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn", "_Static_assert",
    "_Thread_local", "__attribute__", "__attribute", "__asm__", "__asm", "asm",
    "__inline__", "__inline", "__restrict__", "__restrict", "__volatile__", "__volatile",
    "__const__", "__const", "__extension__", "__typeof__", "__typeof", "typeof",
    "__thread", "__signed__", "__builtin_va_arg", "defined",
}

# keywords that start a new statement even in the middle of one
STMT_KEYWORDS = {"if", "for", "while", "switch", "do", "return", "break", "continue", "goto",
                 "case", "default", "else"}

AGGREGATES = {"struct", "union", "enum"}

TYPE_KEYWORDS = {"char", "short", "int", "long", "float", "double", "void", "signed", "unsigned",
                 "_Bool", "const", "volatile", "struct", "union", "enum"}

# keywords after which a '(' may open a cast
UNARY_KEYWORDS = {"return", "case", "else", "do"}

BINARY_OPS = {"=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
              "==", "!=", "<", ">", "<=", ">=", "&&", "||", "/", "%", "<<", ">>", "|", "^"}

TOKEN_RE = re.compile(r"""
    (?P<ws>[ \t\f\v\r]+)
  | (?P<nl>\n)
  | (?P<splice>\\[ \t]*\n)
  | (?P<cmt>/\*.*?(?:\*/|\Z))
  | (?P<lcmt>//[^\n]*)
  | (?P<str>(?:L|u8|u|U)?"(?:[^"\\\n]|\\.|\\\n)*")
  | (?P<chr>(?:L|u|U)?'(?:[^'\\\n]|\\.)*')
  | (?P<id>[A-Za-z_$][A-Za-z0-9_$]*)
  | (?P<num>\.?[0-9](?:[eEpP][-+]|[0-9A-Za-z_.])*)
  | (?P<op>\.\.\.|>>=|<<=|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||\#\#|::|[-+*/%&|^!=]=|.)
""", re.X | re.S)

PP_COND_OPEN = {"if", "ifdef", "ifndef"}
PP_COND_ELSE = {"else", "elif", "elifdef", "elifndef"}

INDENT_OFF_RE = re.compile(r"^\s*(/\*\s*\*INDENT-OFF\*\s*\*/|//\s*\*INDENT-OFF\*)\s*$")
INDENT_ON_RE = re.compile(r"^\s*(/\*\s*\*INDENT-ON\*\s*\*/|//\s*\*INDENT-ON\*)\s*$")


class Tok:
    __slots__ = ("kind", "text", "line", "col", "eline", "ecol", "pp", "first", "idx")

    def __init__(self, kind, text, line, col, eline, ecol, pp):
        self.kind = kind
        self.text = text
        self.line = line        # 1-based
        self.col = col          # 0-based visual column (tabs expanded to 8)
        self.eline = eline
        self.ecol = ecol        # visual column just past the token on eline
        self.pp = pp            # index of the enclosing directive, or None
        self.first = False      # first token on its line

    def __repr__(self):
        return "%s:%d:%d:%r" % (self.kind, self.line, self.col, self.text)


class Directive:
    __slots__ = ("name", "line", "col", "gap", "toks", "head", "body")

    def __init__(self, line, col):
        self.name = ""
        self.line = line
        self.col = col
        self.gap = 0
        self.toks = []


def vcol(line_text, offset):
    """visual column of offset in line_text, tabs expanded to 8"""
    if "\t" not in line_text:
        return offset
    return len(line_text[:offset].expandtabs(8))


def lex(text, lines):
    """Returns (tokens, directives, literal_tabs). Comments are tokens; directive
    tokens carry the directive's index in .pp."""
    toks = []
    directives = []
    literal_tabs = set()  # (line, offset) of tabs inside string/char literals
    line_starts = [0]
    for m in re.finditer("\n", text):
        line_starts.append(m.end())

    pos = 0
    n = len(text)
    lineno = 1
    bol = True          # only whitespace so far on this line
    cur_pp = None       # current directive
    match = TOKEN_RE.match
    while pos < n:
        m = match(text, pos)
        kind = m.lastgroup
        end = m.end()
        if kind == "ws":
            pos = end
            continue
        if kind == "nl":
            lineno += 1
            bol = True
            cur_pp = None
            pos = end
            continue
        if kind == "splice":
            lineno += 1
            pos = end
            continue
        s = m.group()
        ls = line_starts[lineno - 1]
        col = vcol(lines[lineno - 1], pos - ls)
        nl = s.count("\n")
        if nl:
            eline = lineno + nl
            els = line_starts[eline - 1]
            ecol = vcol(lines[eline - 1], end - els)
        else:
            eline = lineno
            ecol = col + (end - pos) if "\t" not in s else vcol(lines[lineno - 1], end - ls)
        if bol and s == "#" and cur_pp is None:
            cur_pp = Directive(lineno, col)
            directives.append(cur_pp)
            t = Tok("op", s, lineno, col, eline, ecol, len(directives) - 1)
            toks.append(t)
            cur_pp.toks.append(t)
            bol = False
            pos = end
            continue
        if kind in ("str", "chr") and "\t" in s:
            ln, off = lineno, pos - ls
            for ch in s:
                if ch == "\t":
                    literal_tabs.add((ln, off))
                if ch == "\n":
                    ln += 1
                    off = 0
                else:
                    off += 1
        t = Tok(kind, s, lineno, col, eline, ecol, None)
        if cur_pp is not None:
            t.pp = len(directives) - 1
            if not cur_pp.name and kind == "id" and len(cur_pp.toks) == 1:
                cur_pp.name = s
                cur_pp.gap = col - cur_pp.toks[0].ecol
            cur_pp.toks.append(t)
        toks.append(t)
        bol = False
        lineno = eline
        pos = end
    prev_line = 0
    for i, t in enumerate(toks):
        t.idx = i
        if t.line != prev_line:
            t.first = True
            prev_line = t.eline
        elif t.eline != t.line:
            prev_line = t.eline
    return toks, directives, literal_tabs


class Frame:
    __slots__ = ("kind", "indent", "inner", "close", "kw", "state", "paren", "hdr_line",
                 "brace", "saved", "depth", "init", "else_tok", "macro")

    def __init__(self, kind, indent=0, inner=0, close=0):
        self.kind = kind      # top extern func block switch struct opaque ctl
        self.indent = indent  # statement column (ctl) or switch column
        self.inner = inner    # column of statements inside
        self.close = close    # column of the closing brace
        self.kw = None        # ctl: if else for while do switch
        self.state = None     # ctl: header body-pending body else-wait while-wait need-semi
        self.paren = 0
        self.hdr_line = 0     # line where the ctl header (or else/do) ended
        self.brace = None     # ctl: closing brace of a braced body
        self.saved = None     # statement state suspended by this frame
        self.depth = 0        # opaque: brace depth
        self.init = False     # opaque: initializer at statement level
        self.else_tok = None
        self.macro = False

    def copy(self):
        f = Frame(self.kind)
        for k in Frame.__slots__:
            v = getattr(self, k)
            if k == "saved" and v is not None:
                v = dict(v)
            setattr(f, k, v)
        return f


class Checker:
    def __init__(self, path, text, rules=None):
        self.path = path
        self.text = text
        self.rules = set(rules) if rules else set(ALL_RULES)
        self.reports = {}
        self.lost_at = None     # first line where the structural pass lost track
        if text.endswith("\n"):
            text = text[:-1]
        self.lines = text.split("\n")
        self.nlines = len(self.lines)

    def report(self, code, line, col, msg):
        if code not in self.rules:
            return
        if code not in ("A1", "A2") and self.off_lines and line in self.off_lines:
            return
        key = (line, code)
        if key not in self.reports:
            self.reports[key] = (line, col + 1, code, msg)

    def run(self):
        self.find_indent_off()
        self.check_lines()
        self.toks, self.directives, literal_tabs = lex(self.text, self.lines)
        self.check_tabs(literal_tabs)
        self.check_directives()
        self.check_spacing()
        self.check_operators()
        self.check_comments()
        self.check_trailing_comments()
        self.parse()
        return sorted(self.reports.values())

    # ---- line rules ----
    def find_indent_off(self):
        self.off_lines = set()
        start = None
        for i, s in enumerate(self.lines):
            if start is None:
                if "INDENT-OFF" in s and INDENT_OFF_RE.match(s):
                    start = i
            elif "INDENT-ON" in s and INDENT_ON_RE.match(s):
                self.off_lines.update(range(start + 1, i + 2))
                start = None
        if start is not None:
            self.off_lines.update(range(start + 1, self.nlines + 1))

    def check_lines(self):
        for i, s in enumerate(self.lines):
            if s.endswith((" ", "\t")):
                k = len(s.rstrip(" \t"))
                self.report("A1", i + 1, vcol(s, k), "trailing whitespace")

    def check_tabs(self, literal_tabs):
        for i, s in enumerate(self.lines):
            if "\t" not in s:
                continue
            off = s.find("\t")
            while off >= 0:
                if (i + 1, off) not in literal_tabs:
                    self.report("A2", i + 1, vcol(s, off), "tab character")
                    break
                off = s.find("\t", off + 1)

    def check_directives(self):
        for d in self.directives:
            if d.col != 0:
                self.report("C5", d.line, d.col, "'#' not in column 0")
            if d.name and d.gap > 0 and "\t" not in self.lines[d.line - 1].split(d.name, 1)[0]:
                self.report("C5", d.line, d.col + 1, "space after '#'")

    # ---- spacing rules ----
    def check_spacing(self):
        toks = self.toks
        directives = self.directives
        for d in directives:
            # '#', 'define' and the name; the space after the name decides between a
            # function-like and an object-like macro, so the next token starts fresh, and
            # so does the body after a parameter list
            d.head = set(d.toks[:3])
            d.body = None
            if d.name == "define" and len(d.toks) > 3 and d.toks[3].text == "(" \
                    and d.toks[3].col == d.toks[2].ecol:
                depth = 0
                for k, t in enumerate(d.toks[3:], 3):
                    if t.text == "(":
                        depth += 1
                    elif t.text == ")":
                        depth -= 1
                        if depth == 0:
                            if k + 1 < len(d.toks):
                                d.body = d.toks[k + 1]
                            break
        prev = None
        for t in toks:
            if t.kind in ("cmt", "lcmt"):
                prev = None
                continue
            if t.pp is not None:
                d = directives[t.pp]
                if d.name != "define" or t in d.head:
                    prev = None
                    continue
            if t.text in ("(", "[") and \
                    re.match(r"[ \t]+\\$", self.lines[t.line - 1].expandtabs(8)[t.ecol:]):
                # the sed pass turns '(   \' into '(\'
                self.report("A4", t.line, t.ecol, "space after '%s'" % t.text)
            if prev is None or prev.eline != t.line or prev.pp != t.pp or \
                    (t.pp is not None and t is directives[t.pp].body):
                prev = t
                continue
            p = prev
            prev = t
            gap = t.col - p.ecol
            a = p.text
            b = t.text
            if b == "(":
                if a in ("if", "for", "while", "switch", "return", "__attribute__"):
                    if gap != 1:
                        self.report("A3", t.line, p.col, "need one space between '%s' and '('" % a)
                    continue
                if a == "sizeof":
                    if gap != 0:
                        self.report("A3", t.line, p.ecol, "no space between 'sizeof' and '('")
                    continue
                if gap > 0 and p.kind == "id" and a not in KEYWORDS and not self.fp_space(t):
                    self.report("A5", t.line, p.ecol, "space between '%s' and '('" % a)
                    continue
            if gap <= 0:
                continue
            if a in ("(", "["):
                self.report("A4", t.line, p.ecol, "space after '%s'" % a)
            elif b in (")", "]"):
                self.report("A4", t.line, p.ecol, "space before '%s'" % b)
            elif b == ";" or (b == "," and a != ","):
                self.report("A6", t.line, p.ecol, "space before '%s'" % b)

    def classify(self, code):
        """Role of each code token: 'opd' operand, 'bin' binary operator, 'un' unary
        operator, 'post' postfix ++/--, 'ptr' '*' after a type keyword, 'mem' '->' or
        '.', 'designator' a designator '.', 'colon' a ':' that is not ternary, 'amb' a '-',
        '+', '*' or '&' that may be unary or binary, or the kind of the '(' a ')'
        closes: 'call', 'ctl', 'kw', 'group', or a cast: 'cast' (with a type keyword),
        'tcast' (only identifiers and '*'), 'idcast' (only identifiers)."""
        role = [None] * len(code)
        value = False       # the previous token ends an operand
        stack = [["", 0]]   # open brackets: [kind, number of pending '?']
        for i, t in enumerate(code):
            s = t.text
            r = None
            if t.kind in ("num", "str", "chr") or (t.kind == "id" and s not in KEYWORDS):
                r = "opd"
            elif t.kind == "id":
                r = "kw"
            elif s == "(":
                p = code[i - 1] if i else None
                if role[i - 1] in ("tcast", "idcast") and \
                        self.cast_kind(code, i) in ("cast", "tcast"):
                    # '(MPI_Win) (void *) p'
                    kind = self.cast_kind(code, i)
                elif value:
                    kind = "call"
                elif p is not None and p.text in ("if", "for", "while", "switch"):
                    kind = "ctl"
                elif p is not None and p.kind == "id" and p.text not in UNARY_KEYWORDS:
                    kind = "kw"
                else:
                    kind = self.cast_kind(code, i)
                stack.append([kind, 0])
            elif s in ("[", "{"):
                stack.append([s, 0])
            elif s in (")", "]", "}"):
                if len(stack) > 1:
                    r = stack.pop()[0]
                if s == "]":
                    r = "opd"
            elif s in ("++", "--"):
                r = "post" if value else "un"
            elif s == "?":
                stack[-1][1] += 1
                if value:
                    r = "bin"
            elif s == ":":
                if stack[-1][1] > 0 and code[i - 1].text != "?":
                    stack[-1][1] -= 1
                    r = "bin"
                else:
                    r = "colon"
            elif s in ("*", "&", "-", "+"):
                if role[i - 1] in ("tcast", "idcast"):
                    # QUIRK(Q9): indent does not know the type, so '(foo_t *) &x' and
                    # '(a) - b' get its binary spacing
                    r = "amb"
                elif value:
                    r = "bin"
                elif s == "*" and i and code[i - 1].text in TYPE_KEYWORDS:
                    r = "ptr"
                elif s == "*" and role[i - 1] in ("bin", "star", "ptr", "ptr2") and \
                        code[i - 1].text == "*":
                    # the second '*' of 'T ** p' (QUIRK(Q2), unknown T), 'char **p' or
                    # 'a * *p'
                    r = "ptr2" if role[i - 1] in ("ptr", "ptr2") else "star"
                else:
                    r = "un"
            elif s in ("!", "~"):
                r = "un"
            elif s in BINARY_OPS:
                if value:
                    r = "bin"
            elif s == "->":
                r = "mem"
            elif s == ".":
                r = "designator" if i and code[i - 1].text in ("{", ",") else "mem"
            role[i] = r
            # '(a)' may be a parenthesized operand: '(a) == b'
            value = r in ("opd", "post") or \
                (s == ")" and r in ("call", "kw", "group", "tcast", "idcast"))
        return role

    def cast_kind(self, code, i):
        """kind of the '(' at code[i] in unary position: 'cast', 'tcast', 'idcast' or
        'group'"""
        # type specifiers, then '*'s and qualifiers: '(a * b)' is not a cast
        n = len(code)
        j = i + 1
        typekw = False
        star = False
        while j < n:
            u = code[j]
            if u.text == "*":
                star = True
            elif u.kind != "id" or (u.text in KEYWORDS and u.text not in TYPE_KEYWORDS):
                break
            elif star and u.text not in ("const", "volatile"):
                return "group"
            elif u.text in TYPE_KEYWORDS:
                typekw = True
            j += 1
        if j == i + 1 or code[i + 1].text == "*" or j >= n:
            return "group"
        if code[j].text == "(" and j + 1 < n and code[j + 1].text == "*":
            # function pointer: '(void (*)(void))', '(void (**)(int, int))'
            j += 1
            while j < n and code[j].text == "*":
                j += 1
            if j + 1 >= n or code[j].text != ")" or code[j + 1].text != "(":
                return "group"
            j += 1
            depth = 0
            while j < n:
                if code[j].text == "(":
                    depth += 1
                elif code[j].text == ")":
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            j += 1
            typekw = True
        if j >= n or code[j].text != ")":
            return "group"
        if typekw:
            return "cast"
        return "tcast" if star else "idcast"

    def check_operators(self):
        """A7-A11: spacing around casts, operators, commas and between words, in code
        outside directives"""
        code = [t for t in self.toks if t.pp is None and t.kind not in ("cmt", "lcmt")]
        role = self.classify(code)
        words = ("id", "num", "str", "chr")
        for i in range(1, len(code)):
            p = code[i - 1]
            t = code[i]
            if p.eline != t.line or t.idx != p.idx + 1:
                continue
            gap = t.col - p.ecol
            rp = role[i - 1]
            rt = role[i]
            a = p.text
            b = t.text
            # A7: one space after a cast
            if a == ")" and rp in ("cast", "tcast", "idcast") and \
                    (rt in ("opd", "un", "amb") or b == "(" or b == "sizeof"):
                # QUIRK(Q1): '(fn)(x)' is also a call through a parenthesized name;
                # QUIRK(Q15): a C99 'for (int *p = (int *)f(x);'
                if gap != 1 and not (gap == 0 and (b == "(" and rp == "idcast" or
                                                   self.in_for_decl(code, i))):
                    self.report("A7", t.line, p.ecol, "need one space after a cast")
                continue
            if rp == "amb":
                # QUIRK(Q9): one space or none after the operator
                if gap > 1:
                    self.report("A9", t.line, p.ecol, "more than one space after '%s'" % a)
                continue
            # A10: one space after ',' (not before an empty macro argument: 'F(,, x)', 'F(a,)',
            # 'F(a,;)'); QUIRK(Q6): '{.a = 1,.b = 2}'
            if a == ",":
                if b not in (";", ",", ")") and gap != 1 and not (gap == 0 and rt == "designator"):
                    self.report("A10", t.line, p.ecol, "need one space after ','")
                continue
            # A11: one space between words
            if p.kind in words and t.kind in words:
                if gap > 1:
                    self.report("A11", t.line, p.ecol, "more than one space")
                continue
            # A8: '*' after an operand or a type keyword
            if b == "*" and rt in ("bin", "ptr"):
                if gap != 1:
                    self.report("A8", t.line, p.ecol, "need one space before '*'")
                continue
            if rt == "ptr2" and gap != 0:
                self.report("A8", t.line, p.ecol, "no space between '*'s")
                continue
            if rp in ("ptr", "ptr2"):
                if gap != 0:
                    self.report("A8", t.line, p.ecol, "no space after '*'")
                continue
            # A9: operators; QUIRK(Q12): 'f()? 1 : 0'
            if rt == "bin":
                if gap != 1 and not (gap == 0 and b == "?" and a == ")"):
                    self.report("A9", t.line, p.ecol, "need one space before '%s'" % b)
                continue
            if rt in ("post", "mem") and gap != 0:
                self.report("A9", t.line, p.ecol, "no space before '%s'" % b)
                continue
            # QUIRK(Q2, Q14): any space after a binary '*': 'foo_t * p', '(int64_t) 2 *e';
            # a '&' before ',' or ')' is no operator but a C++ reference: 'f(MPI::Comm &, int)'
            if rp == "bin" and a != "*" and not (a == "&" and b in (",", ")")):
                if gap != 1:
                    self.report("A9", t.line, p.ecol, "need one space after '%s'" % a)
                continue
            # QUIRK(Q13): '! !x'
            if rp in ("un", "mem", "designator") and gap != 0 and not (a == "!" and b == "!" and gap == 1):
                self.report("A9", t.line, p.ecol, "no space after '%s'" % a)

    def in_for_decl(self, code, i):
        """code[i] follows a cast in the declaration of a C99 'for (int *p = (T *)...'"""
        depth = 0
        for j in range(i - 1, max(i - 40, 0), -1):
            s = code[j].text
            if s == ")":
                depth += 1
            elif s == "(":
                if depth == 0:
                    return code[j - 1].text == "for" and code[j + 1].text in TYPE_KEYWORDS | {"const"}
                depth -= 1
            elif s == ";" and depth == 0:
                return False
        return False

    def fp_space(self, t):
        """QUIRK(Q16): accept 'TYPE (*fp)(...)' and 'TYPE (*ap)[...]' as well as indent's
        'TYPE(*fp)'; calls like 'f (*p)' or 'f (*p > 0)' are still flagged"""
        toks = self.toks
        j = t.idx + 1
        if j >= len(toks) or toks[j].text != "*":
            return False
        while j < len(toks) and toks[j].text == "*":
            j += 1
        return j + 2 < len(toks) and toks[j].kind == "id" and toks[j + 1].text == ")" \
            and toks[j + 2].text in ("(", "[")

    # ---- comment rules ----
    def is_boxed(self, t):
        # The manual also calls a comment boxed if its '/*' is alone on the line and the
        # next line has a '*' under the '*', but indent 2.2.11 moves and realigns those
        # like any other comment (probe).
        s = t.text
        return len(s) > 2 and s[2] in "*=_-"

    def check_comments(self):
        # indent stars the lines of a comment whose second line has no '*', and moves
        # every line that starts with '*' under the '*' of '/*'
        for t in self.toks:
            if t.kind != "cmt" or t.eline == t.line or t.col == 0:
                continue
            if t.pp is not None and t.line != self.directives[t.pp].line:
                # inside a multi-line macro body indent leaves comments alone
                continue
            if self.is_boxed(t):
                continue
            s = self.lines[t.line]
            star_all = not s.lstrip(" \t").startswith("*")
            want = (t.col + 1,)
            if not t.first and t.idx > 0:
                # QUIRK(Q17): after code, indent may leave the '/*' and align the '*'s as if
                # the comment were at the next tab stop: 'while (/* x' then '* y' in column 18
                want += ((self.toks[t.idx - 1].ecol // 8 + 1) * 8 + 1,)
            for ln in range(t.line + 1, t.eline + 1):
                s = self.lines[ln - 1]
                k = len(s) - len(s.lstrip(" \t"))
                if s[k:k + 1] != "*":
                    if star_all:
                        self.report("D3", ln, vcol(s, k), "comment line should start with '*' under the '*' of '/*'")
                elif vcol(s, k) not in want:
                    self.report("D3", ln, vcol(s, k), "'*' should be under the '*' of '/*'")

    def check_trailing_comments(self):
        """D4: a comment that ends a code line starts at the next tab stop past the code,
        or at column 33; D6: the same after a directive, one space after #else/#endif"""
        toks = self.toks
        n = len(toks)
        for c in toks:
            if c.kind not in ("cmt", "lcmt") or c.first:
                continue
            p = toks[c.idx - 1]
            if p.kind in ("cmt", "lcmt") or p.eline != c.line or p.pp != c.pp:
                continue
            nxt = toks[c.idx + 1] if c.idx + 1 < n else None
            if nxt is not None and nxt.line == c.eline:
                continue
            if c.text.startswith(("/**", "/*!", "///", "//!")):
                continue
            tab = (p.ecol // 8 + 1) * 8
            if c.pp is None:
                # QUIRK(Q24): columns indent leaves alone
                if p.text in (":", "(", "[") or \
                        (p.text == "}" and nxt is not None and nxt.text == "else"):
                    continue
                # QUIRK(Q11): column 33 after a statement too
                want = (tab, 32) if p.ecol < 32 else (tab,)
                code = "D4"
            else:
                d = self.directives[c.pp]
                if c.line != d.line:
                    # inside a multi-line macro body
                    continue
                if d.name in ("else", "endif"):
                    want = (p.ecol + 1, 32)     # QUIRK(Q10)
                else:
                    want = (tab,)
                code = "D6"
            if c.col not in want:
                self.report(code, c.line, c.col, "comment at column %d, expected %d"
                            % (c.col + 1, want[0] + 1))

    # ---- structural rules ----
    def parse(self):
        toks = self.toks
        conds = PP_COND_OPEN | PP_COND_ELSE | {"endif"}
        self.code = []
        for t in toks:
            if t.pp is None:
                if t.kind not in ("cmt", "lcmt"):
                    self.code.append(t)
            else:
                d = self.directives[t.pp]
                if d.toks[0] is t and d.name in conds:
                    self.code.append(t)
        self.stack = [Frame("top")]
        self.stmt = None
        self.lost = False
        self.glued = False
        self.cond = []
        self.last_close = None   # last '}' that closed a ctl body
        code = self.code
        self.i = 0
        n = len(code)
        comments = [t for t in toks if t.kind in ("cmt", "lcmt") and t.pp is None and t.first]
        ci = 0
        while self.i < n:
            t = code[self.i]
            while ci < len(comments) and (comments[ci].line, comments[ci].col) < (t.line, t.col):
                self.own_line_comment(comments[ci], t)
                ci += 1
            if t.pp is not None:
                self.directive(self.directives[t.pp])
            elif not self.lost:
                self.step(t)
            self.i += 1

    def peek(self):
        j = self.peek_idx()
        return self.code[j] if j is not None else None

    def peek_idx(self):
        """index of the next code token that is not part of a directive"""
        code = self.code
        j = self.i + 1
        while j < len(code):
            if code[j].pp is None:
                return j
            j += 1
        return None

    def skip_to(self, j):
        """move to code[j], handling the directives in between; True if there were any"""
        found = False
        while self.i + 1 < j:
            self.i += 1
            t = self.code[self.i]
            if t.pp is not None:
                self.directive(self.directives[t.pp])
                found = True
        self.i = j
        return found

    def directive(self, d):
        if d.name in PP_COND_OPEN:
            self.cond.append(self.snapshot())
        elif d.name in PP_COND_ELSE:
            if self.cond:
                self.restore(self.cond[-1])
        elif d.name == "endif":
            if self.cond:
                self.cond.pop()

    def snapshot(self):
        return ([f.copy() for f in self.stack], dict(self.stmt) if self.stmt else None,
                self.lost, self.last_close)

    def restore(self, snap):
        stack, stmt, lost, last_close = snap
        self.stack = [f.copy() for f in stack]
        self.stmt = dict(stmt) if stmt else None
        self.lost = lost
        self.last_close = last_close

    def lose(self):
        if not self.lost:
            self.lost = True
            if self.lost_at is None:
                self.lost_at = self.code[self.i].line

    def check_col(self, t, want, code="C1", what="statement", alt=None):
        if t.first and t.col != want and t.col != alt and not self.lost:
            self.report(code, t.line, t.col, "%s at column %d, expected %d" % (what, t.col + 1, want + 1))

    def stmt_base(self, t):
        top = self.stack[-1]
        k = top.kind
        if k == "switch":
            return top.indent + 4 if t.text in ("case", "default") else top.indent + 8
        if k == "ctl":
            return top.indent + 4
        if k == "extern" and top.inner is None:
            # QUIRK(Q4): contents at column 0 or 4, whichever the first line uses
            top.inner = t.col if t.first and t.col in (0, 4) else 0
        return top.inner

    def step(self, t):
        top = self.stack[-1]
        s = t.text
        if top.kind == "opaque":
            if top.kw in ("enum", "init") and top.depth == 1 and top.hdr_line != t.line:
                self.list_member(top, t)
            if s == "{":
                top.depth += 1
            elif s == "}":
                top.depth -= 1
                if top.depth == 0:
                    self.stack.pop()
                    self.stmt = top.saved
                    if self.stmt is not None:
                        self.stmt["prev"] = t
                        if top.init:
                            self.stmt["init"] = True
                    if top.kw == "enum" and top.hdr_line != t.line:
                        # a one-line enum that is too long gets its name on the next line
                        nxt = self.peek()
                        if nxt is not None and nxt.line != t.line:
                            self.report("B3", nxt.line, nxt.col, "should be on the line of '}'")
            elif s in ("(", "["):
                top.paren += 1
            elif s in (")", "]"):
                top.paren -= 1
            return
        if top.kind == "ctl":
            st = top.state
            if st == "header":
                if top.paren == 0 and s != "(":
                    self.lose()
                    return
                if s in ("(", "["):
                    top.paren += 1
                elif s in (")", "]"):
                    top.paren -= 1
                    if top.paren == 0:
                        top.hdr_line = t.line
                        top.state = "need-semi" if top.kw == "do" else "body-pending"
                        top.else_tok = t
                elif s == "{":
                    self.push_opaque(t)
                elif s == "}":
                    self.lose()
                return
            if st == "body-pending":
                self.ctl_body(top, t)
                return
            if st == "else-wait":
                self.do_else(top, t)
                return
            if st == "while-wait":
                if s != "while":
                    self.lose()
                    return
                if top.brace is not None:
                    if t.line == top.brace.line:
                        if t.col - top.brace.ecol != 1:
                            self.report("B2", t.line, top.brace.ecol, "need one space between '}' and 'while'")
                    else:
                        # QUIRK(Q7): '}' then 'while (...);' on the next line
                        self.check_col(t, top.indent, "B2", "'while' of do-while")
                else:
                    self.check_col(t, top.indent, "C1", "'while' of do-while")
                top.state = "header"
                return
            if st == "need-semi":
                if s != ";":
                    self.lose()
                    return
                self.stack.pop()
                self.complete(t)
                return
        if self.stmt is None:
            self.start_statement(t, self.stmt_base(t))
        else:
            self.continue_statement(t)

    def list_member(self, top, t):
        """t is in the body of a multi-line enum, or of an initializer whose '{' ends
        its line"""
        j = self.i - 1
        while j >= 0 and self.code[j].pp is not None:
            j -= 1
        prev = self.code[j]
        what = "enumerator" if top.kw == "enum" else "initializer member"
        if t.text == "}" and top.paren == 0:
            if t.first:
                self.check_col(t, top.close, "C1", "'}'")
            elif top.kw == "enum":
                self.report("B3", t.line, t.col, "'}' should be on its own line")
        elif t.first and prev.text in (",", "{") and top.paren == 0:
            # QUIRK(Q18): indent puts the member in column 0 in '= {' / '0};', and takes
            # the line after 'x, /* a */ /* b */' for a continuation
            ncmt = sum(1 for c in self.toks[prev.idx + 1:t.idx] if c.kind in ("cmt", "lcmt"))
            if ncmt < 2 and not self.closes_on_line(t):
                self.check_col(t, top.inner, "C1", what)

    def closes_on_line(self, t):
        """the brace enclosing t closes on t's line"""
        depth = 0
        code = self.code
        for j in range(self.i, len(code)):
            u = code[j]
            if u.line != t.line:
                return False
            if u.text == "{":
                depth += 1
            elif u.text == "}":
                depth -= 1
                if depth < 0:
                    return True
        return False

    def push_opaque(self, t, init=False):
        f = Frame("opaque")
        f.depth = 1
        f.init = init
        f.saved = self.stmt
        self.stmt = None
        self.stack.append(f)

    def ctl_body(self, ctl, t):
        if t.text == "{":
            code = "B2" if ctl.kw == "do" else "B1"
            h = ctl.else_tok
            if t.line != ctl.hdr_line:
                # QUIRK(Q19): indent leaves the '{' where a comment or directive is in the
                # way, or where it would not fit in 100 columns
                if self.adjacent(h, t) and not self.separated(h.line, t.line) \
                        and h.ecol + 2 <= 100:
                    self.report(code, t.line, t.col, "'{' should be on the line of '%s'" % self.ctl_word(ctl))
            elif t.col - ctl.else_tok.ecol != 1:
                self.report(code, t.line, ctl.else_tok.ecol, "need one space before '{'")
            ctl.state = "body"
            if ctl.kw == "switch":
                f = Frame("switch", ctl.indent, ctl.indent + 8, ctl.indent)
            else:
                f = Frame("block", ctl.indent, ctl.indent + 4, ctl.indent)
            self.stack.append(f)
            self.after_open(t)
            return
        ctl.state = "body"
        if t.line == ctl.hdr_line and t.text != ";":
            self.report("B5", t.line, t.col, "statement on the same line as '%s'" % self.ctl_word(ctl))
        self.start_statement(t, ctl.indent + 4)

    def ctl_word(self, ctl):
        return {"if": "if (...)", "for": "for (...)", "while": "while (...)",
                "switch": "switch (...)"}.get(ctl.kw, ctl.kw)

    def after_open(self, t):
        """'{' that opens a block of statements: nothing may follow on its line"""
        nxt = self.peek()
        if nxt is not None and nxt.line == t.line and nxt.text != "}":
            self.report("B5", nxt.line, nxt.col, "statement on the same line as '{'")

    def do_else(self, ctl, t):
        # t is 'else' (checked by peek in complete())
        b = ctl.brace
        if b is not None:
            if t.line == b.line:
                if t.col - b.ecol != 1:
                    self.report("B1", t.line, b.ecol, "need one space between '}' and 'else'")
            else:
                # QUIRK(Q20): '}' and 'else' with a blank or comment line between
                if not self.separated(b.line, t.line) and self.adjacent(b, t):
                    self.report("B1", t.line, t.col, "'else' should be on the line of '}'")
                self.check_col(t, ctl.indent, "C1", "'else'")
        else:
            self.check_col(t, ctl.indent, "C1", "'else'")
        j = self.peek_idx()
        nxt = self.code[j] if j is not None else None
        if nxt is not None and nxt.text == "if":
            # else if: the if replaces this ctl at the same column; QUIRK(Q21): indent
            # keeps an 'if' on the next line there, indented by 4, or at the 'else'
            # column if a directive is in between
            self.stack.pop()
            f = Frame("ctl", ctl.indent)
            f.kw = "if"
            f.state = "header"
            self.stack.append(f)
            pp = self.skip_to(j)
            if nxt.line == t.line:
                if nxt.col - t.ecol != 1 and self.adjacent(t, nxt):
                    self.report("B1", t.line, t.ecol, "need one space between 'else' and 'if'")
            elif pp:
                self.check_col(nxt, ctl.indent, "C1", "'if' after 'else' and a directive")
            else:
                self.check_col(nxt, ctl.indent + 4, "C1", "'if' after 'else'")
            return
        ctl.kw = "else"
        ctl.state = "body-pending"
        ctl.hdr_line = t.line
        ctl.else_tok = t
        ctl.brace = None

    def adjacent(self, a, b):
        """no comment between tokens a and b"""
        for t in self.toks[a.idx + 1:b.idx]:
            if t.kind in ("cmt", "lcmt"):
                return False
        return True

    def separated(self, l1, l2):
        """True if a blank line, comment line or directive is between lines l1 and l2"""
        for ln in range(l1 + 1, l2):
            s = self.lines[ln - 1].strip()
            if s == "" or s.startswith(("/*", "//", "*", "#")):
                return True
        return False

    def start_statement(self, t, base):
        s = t.text
        top = self.stack[-1]
        glued = self.glued
        self.glued = False
        if s == "}":
            self.close_brace(t, glued)
            return
        if glued:
            # QUIRK(Q22): after a statement without ';' (a macro), indent's layout is
            # unpredictable
            t = None
        prev = self.code[self.i - 1] if self.i > 0 else None
        if t and prev is not None and prev.line == t.line and prev.pp is None and s != ";":
            if prev.text == ";" or (prev.text == "}" and top.kind not in ("top", "extern")):
                self.report("B5", t.line, t.col, "more than one statement on a line")
        t = self.code[self.i]
        if s in ("case", "default"):
            if top.kind == "switch":
                self.check_col(t, base, "C2", "'%s'" % s)
            self.stmt = {"kind": "case", "start": t, "base": base, "paren": 0, "prev": t}
            return
        if (t.kind == "id" and s not in KEYWORDS and top.kind not in ("top", "extern", "struct")):
            colon = self.label_colon()
            if colon is not None:
                if not glued:
                    self.check_col(t, max(base - 2, 0), "C3", "label")
                nxt = self.code[colon]
                self.i = colon
                after = self.peek()
                # QUIRK(Q5): 'fn_exit:x;'
                if after is not None and after.line == t.line and after.col - nxt.ecol != 0 \
                        and after.text != "}":
                    self.report("B5", after.line, after.col, "statement on the same line as a label")
                self.stmt = None
                return
        if top.kind in ("top", "extern"):
            chk = (t, base, "C4", "top-level code")
        elif top.kind == "switch":
            chk = (t, base, "C2", "statement under 'case'")
        else:
            chk = (t, base, "C1", "statement")
        generic = s not in (";", "{", "if", "for", "while", "switch", "do", "else")
        if generic and self.fp_decl(self.i):
            # QUIRK(Q16): indent writes 'TYPE(*fp) (...)' one column right for an unknown TYPE
            chk += (base + 1,)
        if not glued and not generic:
            self.check_col(*chk)
        if s == ";":
            self.complete(t)
            return
        if s == "{":
            f = Frame("block", base, base + 4, base)
            self.stack.append(f)
            self.after_open(t)
            return
        if s in ("if", "for", "while", "switch"):
            f = Frame("ctl", base)
            f.kw = s
            f.state = "header"
            self.stack.append(f)
            return
        if s == "do":
            f = Frame("ctl", base)
            f.kw = "do"
            f.state = "body-pending"
            f.hdr_line = t.line
            f.else_tok = t
            self.stack.append(f)
            return
        if s == "else":
            self.lose()
            return
        self.stmt = {"kind": "expr", "start": t, "base": base, "paren": 1 if s in "([" else 0, "prev": t,
                     "eq": False, "agg": s if s in AGGREGATES else False, "call": False,
                     "init": False, "extern": s == "extern"}
        if not glued:
            # checked when the statement ends with ';', as one without is likely a macro
            self.stmt["chk"] = chk

    def fp_decl(self, i):
        """code[i] starts 'name (*' (a function pointer with an unknown type, or a call)"""
        code = self.code
        t = code[i]
        return t.kind == "id" and t.text not in KEYWORDS and i + 2 < len(code) \
            and code[i + 1].text == "(" and code[i + 2].text == "*"

    def label_colon(self):
        """index of the ':' if the statement at self.i is a label: 'name:' or 'MACRO(...):'"""
        code = self.code
        t = code[self.i]
        j = self.i + 1
        if j < len(code) and code[j].text == "(" and code[j].pp is None:
            depth = 0
            while j < len(code) and code[j].line == t.line and code[j].pp is None:
                if code[j].text == "(":
                    depth += 1
                elif code[j].text == ")":
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            j += 1
        if j < len(code) and code[j].text == ":" and code[j].line == t.line and code[j].pp is None:
            return j
        return None

    def continue_statement(self, t):
        st = self.stmt
        s = t.text
        if st["kind"] == "case":
            if s == ":" and st["paren"] == 0:
                self.stmt = None
                j = self.peek_idx()
                nxt = self.code[j] if j is not None else None
                if nxt is None or nxt.line != t.line:
                    return
                base = st["base"]
                if nxt.text == "{":
                    # QUIRK(Q3): 'case 1:{' indents like indent, 'case 1: {' by 4
                    if nxt.col == t.ecol:
                        f = Frame("block", base, base + 8, base + 4)
                    else:
                        f = Frame("block", base, base + 4, base)
                    self.skip_to(j)
                    self.stack.append(f)
                    self.after_open(nxt)
                elif nxt.text != "}":
                    self.report("B5", nxt.line, nxt.col, "statement on the same line as '%s'" % st["start"].text)
                return
            if s in ("(", "["):
                st["paren"] += 1
            elif s in (")", "]"):
                st["paren"] -= 1
            st["prev"] = t
            return
        p = st["paren"]
        if "chk" in st and st["prev"] is st["start"] and st["start"].kind == "id" and \
                any(self.lines[ln - 1].lstrip().startswith("#")
                    for ln in range(st["start"].line + 1, t.line)):
            # QUIRK(Q22): a macro alone on its line, a directive, then more code: indent puts
            # both lines one column right ('MPID_DEV_COMM_DECL', '#endif', 'MPIR_Session *p;')
            del st["chk"]
        if p == 0 and t.kind == "id" and s in STMT_KEYWORDS:
            if s in ("case", "default") and t.line == st["prev"].line:
                # 'x case 1:' is not C; indent's layout of it is anyone's guess
                self.lose()
                return
            # a statement keyword ends a statement that lacks its ';' (macros)
            self.complete(t, implicit=True)
            if not self.lost:
                self.step(t)
            return
        if s in ("(", "["):
            st["paren"] = p + 1
            if p == 0 and s == "(":
                st["call"] = True
        elif s in (")", "]"):
            st["paren"] = p - 1
            if p - 1 < 0:
                self.lose()
                return
        elif p == 0:
            if s == ";":
                self.complete(t)
                return
            if s == "=":
                st["eq"] = True
            elif s == "{":
                self.open_in_statement(t)
                return
            elif s == "}":
                self.complete(t, implicit=True)
                if not self.lost:
                    self.step(t)
                return
            elif t.kind == "id" and s in AGGREGATES and not st["agg"]:
                st["agg"] = s
        elif s == "{":
            self.push_opaque(t)
            return
        st["prev"] = t

    def open_in_statement(self, t):
        st = self.stmt
        prev = st["prev"]
        base = st["base"]
        top = self.stack[-1]
        toplevel = top.kind in ("top", "extern")
        if st["eq"] or prev.text in ("=", ",", "(", "return") or st["start"].text == "return":
            # QUIRK(Q19): '{' left on the next line after a comment or past column 100
            if prev.text == "=" and t.line != prev.line and self.adjacent(prev, t) \
                    and prev.ecol + 1 + len(self.lines[t.line - 1].strip()) <= 100:
                self.report("B3", t.line, t.col, "'{' should be on the line of '='")
            self.push_opaque(t, init=True)
            nxt = self.peek()
            if prev.text == "=" and nxt is not None and nxt.line != t.line:
                # members of 'x = {' one per line at the statement column + 4
                f = self.stack[-1]
                f.kw = "init"
                f.hdr_line = t.line
                f.inner = base + 4
                f.close = base
            return
        if "chk" in st and not (st["agg"] and prev.kind == "id"):
            self.check_col(*st["chk"])
        if st["extern"] and prev.kind == "str" and toplevel:
            f = Frame("extern", 0, None, 0)
            self.stmt = None
            self.stack.append(f)
            return
        if prev.text == ")" or (toplevel and st["call"] and not st["agg"]):
            if toplevel:
                # function definition
                if t.line == prev.eline:
                    if top.kind != "extern":    # QUIRK(Q4)
                        self.report("B4", t.line, t.col, "function body '{' should be on its own line")
                else:
                    self.check_col(t, base, "B4", "function body '{'")
                    self.after_open(t)
                f = Frame("func", base, base + 4, base)
                self.stmt = None
                self.stack.append(f)
                return
            if top.kind == "struct":
                self.lose()
                return
            # a macro used like a loop: FOREACH(...) {
            f = Frame("block", base, base + 4, base)
            f.macro = True
            self.stmt = None
            self.stack.append(f)
            return
        if st["agg"] and prev.kind == "id":
            if t.line != prev.line:
                self.report("B3", t.line, t.col, "'{' should be on the line of '%s'" % prev.text)
            elif t.col - prev.ecol != 1:
                self.report("B3", t.line, prev.ecol, "need one space before '{'")
            if st["agg"] == "enum":
                self.push_opaque(t)
                self.stack[-1].kw = "enum"
                self.stack[-1].hdr_line = t.line
                self.stack[-1].inner = base + 4
                self.stack[-1].close = base
                return
            f = Frame("struct", base, base + 4, base)
            f.saved = st
            self.stmt = None
            self.stack.append(f)
            return
        self.push_opaque(t)

    def close_brace(self, t, glued=False):
        top = self.stack[-1]
        k = top.kind
        if k not in ("func", "block", "switch", "struct", "extern"):
            self.lose()
            return
        if not glued:
            self.check_col(t, top.close, "C1", "'}'")
        # QUIRK(Q4): the '}' of 'extern "C" {' may follow code
        if not t.first and k != "extern" and not glued:
            self.report("B5", t.line, t.col, "'}' should be on its own line")
        self.stack.pop()
        if k == "func":
            if not self.next_has_initializer(t.line):
                self.blank_after(t.line, "function body")
            self.stmt = None
            self.complete(t)
        elif k == "struct":
            self.stmt = top.saved
            self.stmt["prev"] = t
            nxt = self.peek()
            if nxt is not None and nxt.line != t.line:
                self.report("B3", nxt.line, nxt.col, "should be on the line of '}'")
        elif k == "extern":
            self.stmt = None
        else:
            self.stmt = None
            parent = self.stack[-1]
            if parent.kind == "ctl" and parent.state == "body" and not top.macro:
                parent.brace = t
            self.complete(t)

    def next_has_initializer(self, line):
        """QUIRK(Q23): the next top-level statement starts on the line after 'line' and
        has '= {' (indent then leaves out the blank line)"""
        code = self.code
        j = self.i + 1
        if j >= len(code) or code[j].line != line + 1 or code[j].pp is not None:
            return False
        while j < len(code) - 1:
            s = code[j].text
            if s in (";", "{", "}"):
                return False
            if s == "=" and code[j + 1].text == "{":
                return True
            j += 1
        return False

    def blank_after(self, line, what):
        if line >= self.nlines:
            return
        s = self.lines[line].strip()
        if s == "":
            return
        if re.match(r"#\s*(else|elif|endif)\b", s):
            return
        self.report("E1", line, 0, "need a blank line after %s" % what)

    def complete(self, t, implicit=False):
        """a statement ended at t"""
        st = self.stmt
        self.stmt = None
        top = self.stack[-1]
        if st is not None and "chk" in st:
            if not implicit:
                self.check_col(*st["chk"])
            else:
                self.glued = True    # QUIRK(Q22)
        if st is not None and st.get("init") and not implicit and top.kind in ("top", "extern") \
                and not self.lost and not self.next_has_initializer(t.line):
            self.blank_after(t.line, "a top-level initializer")
        while True:
            top = self.stack[-1]
            if top.kind != "ctl":
                return
            if top.state != "body":
                self.lose()
                return
            if top.kw == "if":
                nxt = t if implicit else self.peek()
                if nxt is not None and nxt.text == "else":
                    top.state = "else-wait"
                    return
            elif top.kw == "do":
                top.state = "while-wait"
                return
            self.stack.pop()

    def own_line_comment(self, c, nxt):
        """c is a comment that starts its line; nxt is the next code token"""
        if self.lost or c.col == 0 or (c.kind == "cmt" and self.is_boxed(c)):
            return
        if self.stmt is not None:
            return
        top = self.stack[-1]
        k = top.kind
        if k in ("func", "block", "struct"):
            want = [top.inner]
        elif k == "switch":
            want = [top.indent + 8]
            if nxt.text in ("case", "default"):
                want.append(top.indent + 4)    # QUIRK(Q8)
        elif k == "ctl" and top.state == "body-pending":
            # between 'if (...)' and its body, braced or not
            want = [top.indent + 4]
        elif k == "opaque" and top.kw == "enum" and top.depth == 1:
            want = [top.inner]
        else:
            return
        if c.col not in want:
            self.report("D1", c.line, c.col, "comment at column %d, expected %d" % (c.col + 1, want[0] + 1))


def check_file(path, rules=None, text=None):
    if text is None:
        with open(path, encoding="utf-8", errors="surrogateescape") as f:
            text = f.read()
    return Checker(path, text, rules).run()


def wanted(path):
    if not re.search(FILE_TYPES, path):
        return False
    for pat in IGNORE_LIST:
        if re.search(pat, path):
            return False
    return True


def main():
    args = sys.argv[1:]
    rules = None
    files = []
    use_all = False
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--all":
            use_all = True
        elif a == "--rule":
            i += 1
            rules = args[i].split(",")
        elif a.startswith("--rule="):
            rules = a[7:].split(",")
        elif a in ("-h", "--help"):
            print("Usage: %s [--all] [--rule CODE,...] [files...]" % sys.argv[0])
            return 0
        else:
            files.append(a)
        i += 1
    if rules:
        bad = [r for r in rules if r not in ALL_RULES]
        if bad:
            print("unknown rule: %s" % ",".join(bad), file=sys.stderr)
            return 2
    if use_all:
        out = subprocess.run(["git", "ls-files"], capture_output=True, text=True, check=True)
        files += out.stdout.split()
    ret = 0
    for path in files:
        if not wanted(path):
            continue
        for line, col, code, msg in check_file(path, rules):
            print("%s:%d:%d: %s %s" % (path, line, col, code, msg))
            ret = 1
    return ret


if __name__ == "__main__":
    sys.exit(main())
