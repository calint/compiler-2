# the constructs of the language as told by 'etc/tutorial.baz'
#
# each construct has a pattern that finds it in the text of a program and a
# small program that uses it with a known result. 'fuzz-constructs.py' builds
# the programs, 'fuzz-coverage.py' counts the constructs the fuzzers emit
#
# a 'Construct' program ends with exit code 0 and writes 'stdout', a 'Rejected'
# program has a line that ends with '# error' and the compiler must report the
# error on that line
import re

ASSERT = "func assert(ok bool) { if not ok exit(1) }\n"
BOTH = ("x86_64", "rv32i-fpga")


class Construct:
    def __init__(self, name, pattern, main, files=None, stdout=b"",
                 targets=BOTH, stdin=b"", options=(), gate=True):
        self.name = name
        # a regular expression or a function of the text of a program
        self.pattern = pattern if callable(pattern) \
            else re.compile(pattern, re.M)
        # false for a construct that is not in the text of the program, such
        # as a command line option
        self.gate = gate
        # 'files' maps a path to its text, 'main' is the text of 'main.baz'
        self.files = dict(files or {})
        self.files["main.baz"] = main
        self.stdout = stdout
        self.targets = targets
        self.stdin = stdin
        self.options = tuple(options)
        self.rejected = False


    def found_in(self, source):
        if callable(self.pattern):
            return self.pattern(source)
        return self.pattern.search(source) is not None


def named_like_field(source):
    # a method with the name of a field of its type
    for m in re.finditer(r"^func (?:mut )?(\w+)\.(\w+)\(", source, re.M):
        body = re.search(r"^type " + m.group(1) + r"\b[^{\n]*\{([^}]*)\}",
                         source, re.M)
        if body and re.search(r"(^|,)\s*" + m.group(2) + r"\b",
                              body.group(1)):
            return True
    return False


def let_passed_to_call(source):
    # a 'let' that is an argument of a call
    for m in re.finditer(r"\blet (\w+) = ", source):
        if re.search(r"\w\([^)\n]*\b" + m.group(1) + r"\b[^)\n]*\)",
                     source[m.end():]):
            return True
    return False


class Rejected(Construct):
    def __init__(self, name, pattern, main, targets=("x86_64",), options=()):
        super().__init__(name, pattern, main, targets=targets,
                         options=options)
        self.rejected = True


def program(body, head=""):
    return ASSERT + head + "func main() {\n" + body + "\n}\n"


GENERIC = (
"type buffer<capacity> {\nitems i8[capacity],\ncount,\n}\n"
        "type small = buffer<4>\ntype large = buffer<16>\n"
        "func mut buffer.add(v) {\nself.items[self.count] = i8(v)\n"
        "self.count = self.count + 1\n}\n"
        "func buffer.is_full() res bool { res = self.count == capacity }\n"
        "func make<T type>(n) res T {\nres.items = {}\nres.count = n\n}\n"
        "func fill<T type>(b mut T) {\nloop {\nif b.is_full() break\n"
        "b.add(1)\n}\n}\n"
)


CONSTRUCTS = [
    Construct("comment", r"#", "# a\nfunc main() { # b\n exit(0) # c\n}\n"),
    Construct("include", r'^include "', 'include "lib/a.baz"\ninclude "b.baz"\n'
              + ASSERT + "func main() {\nassert(a_val() == 5 and b_val() == 7)"
              "\n}\n", files={
                  "lib/a.baz": 'include "../b.baz"\n'
                               "func a_val() res { res = b_val() - 2 }\n",
                  "b.baz": "func b_val() res { res = 7 }\n",
              }),
    Construct("let-constant", r"\blet \w+ = \d", program(
        "let a = 42\nlet b = a * 2\nassert(b == 84)")),
    Construct("let-top-level", r"^let \w+ = ", program(
        "assert(size == 4)", "let size = 4\n")),
    Construct("let-variable", r"\blet \w+ = [a-z]\w*\(|\blet \w+ = \w+ \*",
              program("var c = 3\nlet d = c * 2\nassert(d == 6)")),
    Construct("var-default-type", r"\bvar \w+ = \d", program(
        "var c = 1\nc = c + 41\nassert(c == 42)")),
    Construct("hex-literal", r"\b0x[0-9a-fA-F]", program(
        "assert(0xff == 255 and 0xFf == 255)")),
    Construct("bin-literal", r"\b0b[01]", program(
        "assert(0b11111111 == 255 and 0b0 == 0)")),
    Construct("char-literal", r"'[^\\']'", program(
        "assert('a' == 97 and ' ' == 32)")),
    Construct("char-escape", r"'\\[^']+'", program(
        "assert('\\n' == 10 and '\\x41' == 65)")),
    Construct("unary-minus", r"[=(,] ?-\w", program(
        "var d = 255\nassert(-d == -255)")),
    Construct("unary-invert", r"~", program(
        "var d = 255\nassert(~d == -256)")),
    Construct("most-negative", r"- 1\b", program(
        "var m = i32(-2147483647 - 1)\nassert(m < 0)")),
    Construct("arith", r"[a-z0-9)] [+*/%-] [a-z0-9(]", program(
        "var a = 7\nvar b = 2\nassert(a + b == 9 and a - b == 5)\n"
        "assert(a * b == 14 and a / b == 3 and a % b == 1)\n"
        "assert(-a / b == -3 and -a % b == -1)")),
    Construct("bitwise", r"[a-z0-9)] [&|^] [a-z0-9(]", program(
        "var a = 7\nassert(a & 3 == 3 and a | 8 == 15 and a ^ 1 == 6)")),
    Construct("shift", r"<<|>>", program(
        "var a = 7\nassert(a << 2 == 28 and -a >> 1 == -4)")),
    Construct("precedence", r"\w \+ \w \* \w|\w & \w \^ \w|\w \| ", program(
        "var one = 1\nvar b = 2\nassert(one + 2 * 3 == 7)\n"
        "assert(one + one << 3 == 9 and b * one | 2 == 6)\n"
        "assert(6 & 3 ^ one == 2)\nassert((one + one) << 3 == 16)")),
    Construct("conversion-i8", r"\bi8\(", program(
        "var s = i8(100)\ns = i8(s + s)\nassert(s == -56)")),
    Construct("conversion-i16", r"\bi16\(", program(
        "var s = i16(30000)\ns = i16(s + s)\nassert(s == -5536)")),
    Construct("conversion-i32", r"\bi32\(", program(
        "var s = i32(0x7fffffff)\ns = i32(s + 1)\nassert(s < 0)")),
    Construct("conversion-i64", r"\bi64\(", program(
        "var s = i64(0x7fffffff)\nassert(s + 1 == 0x80000000)"),
        targets=("x86_64",)),
    Construct("conversion-int", r"\bint\(", program(
        "var s = i8(-56)\nvar w = int(s)\nassert(w == -56)")),
    Construct("int-array", r"\bint\[", program(
        "var a = int[4]\na[3] = 5\nassert(a[3] == 5 and a[0] == 0)")),
    Construct("first-element-width", r"\b(small|s8)\w* \+ 1", program(
        "var s = i8(100)\nvar w = int(s)\nvar t = s + 27\nassert(t == 127)\n"
        "var u = 1 + s\nu = w\nassert(u == 100)\n"
        "var v = s + i8(-50)\nassert(v == 50)\n"
        "assert(int(s) + w == 200)")),
    Construct("bool-values", r"\b(true|false)\b", program(
        "var yes = true\nvar no = false\nassert(yes and not no)")),
    Construct("comparison", r"[a-z0-9)] (==|!=|<=|>=|<|>) [a-z0-9(]", program(
        "var x = 1\nassert(x < 2 and x <= 1 and x > 0 and x >= 1)\n"
        "assert(x == 1 and x != 2)")),
    Construct("and-or-not", r"\bnot\b|\band\b|\bor\b", program(
        "var x = 1\nassert(not (false or x > 2) and (true and x == 1))")),
    Construct("short-circuit", r"\b(and|or)\b", program(
        "assert(not (false and bump()))\nassert(true or bump())\n"
        "assert(calls == 0)\nassert(true and bump())\nassert(calls == 1)",
        "dat calls = 0\nfunc bump() res bool { calls = calls + 1\n"
        "res = true }\n")),
    Construct("if-else", r"\bif\b.*\{|\belse\b", program(
        "var n = 3\nvar k = 0\nif n < 0 { k = -1 } else if n == 0 { k = 0 }\n"
        "else { k = 1 }\nassert(k == 1)")),
    Construct("if-single-statement", r"\bif [^{\n]*[^{\n]$", program(
        "var k = 0\nif k == 0 k = 2\nassert(k == 2)")),
    Construct("loop-break-continue", r"\bloop\b", program(
        "var sum = 0\nvar i = 0\nloop {\ni = i + 1\nif i == 3 continue\n"
        "if i > 5 break\nsum = sum + i\n}\nassert(sum == 12)")),
    Construct("exit", r"\bexit\(", program("exit(0)")),
    Construct("scope-block", r"^\s*\{\s*$", program(
        "let level = 1\nvar x = 1\n{\nlet level = 2\nvar x = 2\n"
        "assert(level == 2 and x == 2)\n}\nassert(level == 1 and x == 1)")),
    Construct("array-zero", r"\bi\d+\[\w+\]\s*$|\bi\d+\[\d+\]", program(
        "var z = i32[4]\nassert(z[3] == 0)\nvar y = i32[4]{}\n"
        "assert(y[1] == 0)\nvar x = i8[size]\nassert(array_length(x) == 4)",
        "let size = 4\n")),
    Construct("array-literal", r"\[\]\s*\{", program(
        "var p = []{2, 3, 5, 7}\nassert(array_length(p) == 4)\n"
        "var q = i32[]{9, 8}\nassert(q[1] == 8)\nvar r = i8[4]{1, 2}\n"
        "assert(r[1] == 2 and r[3] == 0)")),
    Construct("array-index-expression", r"\[\w+ [+*-] \w+\]", program(
        "var p = []{2, 3, 5, 7}\nvar ix = 2\np[ix] = p[ix + 1] * 2\n"
        "assert(p[2] == 14)")),
    Construct("array-let", r"\blet \w+ = (\w+)?\[", program(
        "let t = []{10, 20, 30}\nassert(t[1] == 20)")),
    Construct("foo", r"\bfoo \w+ \{", program(
        "var p = []{2, 3, 14, 7}\nvar s = 0\nfoo p { s = s + e }\n"
        "assert(s == 26)\nfoo p { e = i + n }\nassert(p[0] == 4 and "
        "p[3] == 7)")),
    Construct("foo-count", r"\bfoo [\w.\[\]]+, ", program(
        "var p = []{4, 5, 6, 7}\nvar c = 3\nvar s = 0\n"
        "foo p, c { s = s + e }\nassert(s == 15)\nfoo p, 2 { s = s + n }\n"
        "assert(s == 23)")),
    Construct("foo-nested", r"\bfoo .*\{[^}]*\bfoo\b", program(
        "var p = []{1, 2}\nvar s = 0\nfoo p {\nvar a = e\nfoo p { s = s + a "
        "* e }\n}\nassert(s == 9)")),
    Construct("array-copy-init", r"\bvar \w+ = \w+$", program(
        "var p = []{4, 5}\nvar c = p\nc[0] = 0\nassert(p[0] == 4)")),
    Construct("array-copy", r"\barray_copy\(", program(
        "var p = []{4, 5, 6, 7}\nvar c = p\nc[0] = 0\narray_copy(p[1], c, 3)\n"
        "assert(c[0] == 5 and c[2] == 7 and c[3] == 7)")),
    Construct("arrays-equal", r"\barrays_equal\(", program(
        "var p = []{4, 5, 6}\nvar c = p\nassert(arrays_equal(p[1], c[1], 2))\n"
        "c[2] = 0\nassert(not arrays_equal(p, c, 3))")),
    Construct("array-compare", r"\w+ (==|!=) \w+\s*\)", program(
        "var p = []{4, 5}\nvar c = p\nassert(p == c)\nc[0] = 1\n"
        "assert(p != c)")),
    Construct("array-length", r"\barray_length\(", program(
        "var p = i8[5]\nassert(array_length(p) == 5)")),
    Construct("string-dat", r'^dat \w+ = "', program(
        "assert(array_length(g) == 3 and g[0] == 'a')", 'dat g = "abc"\n')),
    Construct("string-var", r'\bvar \w+ = "', program(
        'var w = "baz\\n"\nassert(array_length(w) == 4 and w[3] == 10)')),
    Construct("write", r"\bwrite\(1, \w+\)", program(
        'var w = "hi\\n"\nwrite(1, w)'), stdout=b"hi\n"),
    Construct("write-count-start", r"\bwrite\(1, \w+, \d+, \d+\)", program(
        'var w = "baz\\n"\nwrite(1, w, 2, 1)\nwrite(1, w, 1, 3)'),
        stdout=b"az\n"),
    Construct("write-count", r"\bwrite\(1, \w+, \w+\)", program(
        'var w = "baz\\n"\nwrite(1, w, 3)'), stdout=b"baz"),
    Construct("read", r"\bread\(0, ", program(
        "var line = i8[16]\nvar got = read(0, line, 16)\nwrite(1, line, got)"
        "\nvar again = read(0, line)\nassert(again == 0)"),
        stdin=b"abc\n", stdout=b"abc\n"),
    Construct("read-start", r"\bread\(0, \w+, \w+, \w+\)", program(
        "var line = i8[16]\nvar got = read(0, line, 8, 2)\n"
        "write(1, line, got, 2)"), stdin=b"ab\n", stdout=b"ab\n"),
    Construct("function-res", r"\bres = ", program(
        "assert(twice(3) == 6 and twice(3 + 1) == 8)",
        "func twice(v) res { res = v * 2 }\n")),
    Construct("function-mut", r"\w+ mut\b", program(
        "var x = 3\nincrement(x)\nassert(x == 4)",
        "func increment(v mut) { v = v + 1 }\n")),
    Construct("function-return", r"\breturn\b", program(
        "assert(sign(-4) == -1 and sign(0) == 0 and sign(9) == 1)",
        "func sign(v) res {\nres = 0\nif v == 0 return\nres = 1\n"
        "if v < 0 res = -1\n}\n")),
    Construct("function-typed", r"\) res i\d+", program(
        "var w = i32(0x1234)\nassert(low(w) == 0x34)",
        "func low(v i32) res i8 { res = i8(v & 0xff) }\n")),
    Construct("function-array-argument", r"\w\[\]\)|\w \w+\[\]\)", program(
        "var a = i32[]{9, 8}\nassert(first(a) == 9)\nlet it = []{1, 2, 3}\n"
        "assert(total(it) == 6)",
        "func first(arr i32[]) res i32 { res = arr[0] }\n"
        "func total(values[]) res {\nres = 0\nfoo values { res = res + e }\n"
        "}\n")),
    Construct("function-let-argument", let_passed_to_call, program(
        "let fixed = 5\nvar x = 4\nassert(sum_of(x, fixed) == 9)",
        "func sum_of(a, b) res { res = a + b }\n")),
    Construct("function-mut-array", r"\w+ mut \[\]", program(
        "var a = []{1, 2, 3}\nbump_all(a)\nassert(a[0] == 2 and a[2] == 4)",
        "func bump_all(values mut []) { foo values { e = e + 1 } }\n")),
    Construct("type-fields", r"^type \w+ ?\{", program(
        "var p = point{1, 2}\nassert(p.x == 1 and p.y == 2)",
        "type point {x, y}\n")),
    Construct("type-zero", r"\bvar \w+ = \w+$", program(
        "var o = point\nvar q = point{}\nvar r = point{7}\nassert(o.x == 0 "
        "and q.y == 0 and r.x == 7 and r.y == 0)", "type point {x, y}\n")),
    Construct("type-nested", r"\{\{", program(
        "var px = pixel{{3, 4}, 0xff}\npx.pos.x = 5\npx.tags[1] = 'z'\n"
        "assert(px.pos.x == 5 and px.color == 0xff and px.tags[1] == 'z')",
        "type point {x, y}\ntype pixel {\npos point,\ncolor i32,\n"
        "tags i8[2], # trailing comma\n}\n")),
    Construct("type-copy-compare", r"\bif [a-z]\w* [!=]= [a-z]\w* exit|= [a-z]\w* [!=]= [a-z]\w*$", program(
        "var px = pixel{{3, 4}, 1}\nvar q = px.pos\nq.y = 4\n"
        "assert(q == px.pos)\nq.y = 5\nassert(q != px.pos)",
        "type point {x, y}\ntype pixel {\npos point,\ncolor i32,\n}\n")),
    Construct("type-array-of-instances", r"\w+\[\d+\]\{\{", program(
        "var ps = point[3]{{1, 2}, {3, 4}}\nps[2] = ps[0]\n"
        "assert(ps[2].y == 2 and ps[1].x == 3)", "type point {x, y}\n")),
    Construct("method-mut", r"^func mut \w+\.\w+\(", program(
        "var p = point{1, 2}\np.move(10, 20)\nassert(p.x == 11 and p.y == 22)",
        "type point {x, y}\nfunc mut point.move(dx, dy) {\n"
        "self.x = self.x + dx\nself.y = self.y + dy\n}\n")),
    Construct("method-read-only", r"^func \w+\.\w+\(", program(
        "let p = point{1, 2}\nassert(p.sum() == 3)",
        "type point {x, y}\nfunc point.sum() res { res = self.x + self.y }\n")),
    Construct("method-named-like-field", named_like_field, program(
        "var p = point{1, 2}\np.x(5)\nassert(p.x == 5)",
        "type point {x, y}\nfunc mut point.x(x) { self.x = x }\n")),
    Construct("constructor", r"\) self \{", program(
        "var p = point.at(-1, -2)\nassert(p.x == -1 and p.y == -2)\n"
        "var pts = point[2]\npts[1] = point.at(3, 4)\nassert(pts[1].y == 4)",
        "type point {x, y}\nfunc point.at(x, y) self {\nself.x = x\n"
        "self.y = y\n}\n")),
    Construct("array-field", r"^type \w+ ?\{[^}\n]*\b\w+\[\d+\]\s*[,}]|^\s+\w+\[\d+\],?$", program(
        "var s = stack{{7, 8}, 2}\nassert(s.items[1] == 8 and s.items[2] == 0)"
        "\ns.push(9)\nassert(s.pop() == 9 and s.count == 2)\n"
        "assert(array_length(s.items) == 4 and sum_all(s.items) == 24)\n"
        "var c = s\nc.items[0] = 0\nassert(s != c and s.items[0] == 7)",
        "type stack {\nitems[4],\ncount,\n}\n"
        "func mut stack.push(v) {\nself.items[self.count] = v\n"
        "self.count = self.count + 1\n}\n"
        "func mut stack.pop() res {\nself.count = self.count - 1\n"
        "res = self.items[self.count]\n}\n"
        "func sum_all(arr[]) res {\nres = 0\nfoo arr { res = res + e }\n}\n")),
    Construct("dat", r"^dat \w+ = ", program(
        "assert(powers[3] == 8)\npowers[0] = 3\nassert(powers[0] == 3)\n"
        "count_call()\ncount_call()\nassert(calls == 2)\nhome.move(1, 1)\n"
        "assert(home.x == 11 and home.y == 21)\nhistory.push(3)\n"
        "assert(history.items[2] == 3 and history.count == 3)\n"
        "var local = powers\nlocal[1] = 0\nassert(powers[1] == 2)",
        "type point {x, y}\ntype stack {\nitems[4],\ncount,\n}\n"
        "func mut point.move(dx, dy) {\nself.x = self.x + dx\n"
        "self.y = self.y + dy\n}\n"
        "func mut stack.push(v) {\nself.items[self.count] = v\n"
        "self.count = self.count + 1\n}\n"
        "dat powers = []{1, 2, 4, 8}\ndat calls = 0\ndat home = "
        "point{10, 20}\ndat history = stack{{1, 2}, 2}\n"
        "func count_call() { calls = calls + 1 }\n")),
    Construct("noinline", r"\bnoinline\b", program(
        "assert(factorial(5) == 120 and factorial(5 - 2) == 6)",
        "func noinline factorial(n) res {\nres = 1\nif n <= 1 return\n"
        "var m = n - 1\nvar partial = factorial(m)\nres = n * partial\n}\n")),
    Construct("noinline-array-argument", r"\bnoinline \w+\([^)]*\[\]", program(
        "var a = []{1, 2, 3}\nvar b = []{1, 2}\nassert(add_all(a) == 6)\n"
        "assert(add_all(b) == 3)",
        "func noinline add_all(a []) res {\nres = 0\nfoo a { res = res + e }\n"
        "}\n")),
    Construct("noinline-record-argument", r"\bnoinline \w+\(\w+ \w+\)", program(
        "assert(sum(point{1, 2}) == 3 and sum({3, 4}) == 7)",
        "type point {x, y}\nfunc noinline sum(p point) res {\n"
        "res = p.x + p.y\n}\n")),
    Construct("generic-type", r"^type \w+<\w+", program(
        "var s = small\nvar l = large\nassert(array_length(s.items) == 4 and "
        "array_length(l.items) == 16)\ns.add(7)\nassert(s.items[0] == 7 and "
        "not s.is_full())",
        "type buffer<capacity> {\nitems i8[capacity],\ncount,\n}\n"
        "type small = buffer<4>\ntype large = buffer<16>\n"
        "func mut buffer.add(v) {\nself.items[self.count] = i8(v)\n"
        "self.count = self.count + 1\n}\n"
        "func buffer.is_full() res bool { res = self.count == capacity }\n")),
    Construct("generic-function", r"^func \w+<\w+ type", program(
        "var s = small\nvar l = large\nfill<small>(s)\nfill<large>(l)\n"
        "assert(s.is_full() and l.is_full() and l.count == 16)",
        GENERIC)),
    Construct("generic-deduced", r"\b(fill|make)\([a-z]\w*[,)]", program(
        "var s = small\nfill(s)\nassert(s.is_full())", GENERIC)),
    Construct("generic-result-type", r"\bmake<|\bmake\(", program(
        "var m = make<small>(2)\nassert(m.count == 2)\nm = make(3)\n"
        "assert(m.count == 3)", GENERIC)),
    Construct("generic-type-parameters", r"^type \w+<\w+ type", program(
        "var p = small_large\np.a.add(1)\np.b.add(2)\n"
        "assert(p.a.count == 1 and p.b.items[0] == 2)",
        GENERIC + "type pair<first type, second type> {\na first,\n"
        "b second,\n}\ntype small_large = pair<small, large>\n")),
    Construct("generic-comparison", r"\w+ < \w+", program(
        "var capacity = 3\nvar b = 4\nassert(capacity < b)", GENERIC)),
    Construct("checks-noub", r"--checks", program(
        "var a = [4]\nvar ix = 3\na[ix] = 1\nassert(a[3] == 1)"),
        options=("--checks=upper,lower,line",), gate=False),
    Construct("alias-ok", r"\w+ = \w+\(\w+\)", program(
        "var x = 5\nvar y = add_to_zero(x)\nassert(y == 5)\n"
        "var p = point{1, 2}\np = point{2, 1}\nassert(p.x == 2 and p.y == 1)",
        "type point {x, y}\nfunc add_to_zero(v) res {\nres = 0\nres = res + v\n"
        "}\n")),
    Construct("main", r"^func main\(\)", "func main() {\n}\n"),
]


REJECTED = [
    Rejected("reject-let-assign", r"\blet\b", program(
        "var c = 3\nlet d = c * 2\nd = 0 # error")),
    Rejected("reject-narrowing", r"\bi8\(", program(
        "var small = i8(100)\nvar wide = int(small)\n"
        "small = wide # error")),
    Rejected("reject-wider-element", r"\bi8\(", program(
        "var small = i8(100)\nvar wide = int(small)\n"
        "var sum = small + wide # error")),
    Rejected("reject-let-array-write", r"\blet\b", program(
        "let table = []{10, 20, 30}\ntable[1] = 0 # error")),
    Rejected("reject-unassigned-result", r"\bres\b", program(
        "assert(f(1) == 0)", "func f(v) res { # error\nif v == 0 return\n"
        "res = 1\n}\n")),
    Rejected("reject-read-only-argument", r"\bfunc\b", program(
        "assert(f(1) == 1)", "func f(a) res {\na = 0 # error\nres = a\n}\n")),
    Rejected("reject-read-only-array-argument", r"\[\]", program(
        "var a = []{1}\ng(a)", "func g(v[]) {\nfoo v { e = 0 } # error\n}\n")),
    Rejected("reject-constant-index", r"\[\d+\]", program(
        "var a = [4]\na[4] = 1 # error")),
    Rejected("reject-dat-initializer", r"^dat \w+ = \w+$", program(
        "assert(twice == 0)", "dat calls = 0\ndat twice = calls # error\n")),
    Rejected("reject-alias", r"\w+ = \w+\(\w+\)", program(
        "var x = 5\nx = add_to_zero(x) # error",
        "func add_to_zero(v) res {\nres = 0\nres = res + v\n}\n")),
    Rejected("reject-record-self-read", r"\w+\{\w+\.\w+", program(
        "var p = point{1, 2}\np = point{p.y, p.x} # error",
        "type point {x, y}\n")),
    Rejected("reject-literal-range", r"\d{9,}", program(
        "var m = -9223372036854775808 # error")),
    Rejected("reject-constant-shift", r"<<", program(
        "var a = 1\na = a << 64 # error")),
    Rejected("reject-break-outside-loop", r"\bbreak\b", program(
        "break # error")),
    Rejected("reject-repeated-name", r"\bvar\b", program(
        "var a = 1\nvar a = 2 # error")),
    Rejected("reject-undefined-name", r"\w", program(
        "var a = b # error")),
    Rejected("reject-constant-overflow", r"\+", program(
        "var a = i8(100) + 100 # error")),
    Rejected("reject-deduce-from-var", r"\bmake\(", program(
        "var m = make(3) # error", GENERIC)),
    Rejected("reject-deduce-from-array", r"\bfill\(", program(
        "fill([4]) # error", GENERIC)),
    Rejected("reject-deduce-from-literal", r"\bfill\(", program(
        "fill({}) # error", GENERIC)),
]
