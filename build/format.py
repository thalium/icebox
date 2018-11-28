import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import timeit

def read_file(filename):
    with open(filename, "rb") as fh:
        return fh.read().decode("utf-8")

def write_file(filename, data):
    fd = tempfile.NamedTemporaryFile(mode="wb", prefix=os.path.basename(filename), dir=os.path.dirname(filename), delete=False)
    try:
        fd.write(data.encode("utf-8"))
    finally:
        fd.close()
    shutil.move(fd.name, filename)

def compile_fields(fields):
    reply = []
    for p, a, sep, b in fields:
        cmt = r"\/\/ *.+"
        full = r"\n((?:" + a + sep + b + "(?: +" + cmt + r")?\n)+)"
        line = r"(" + a + sep + ")(" + b + ") *( " + cmt + r")?\n"
        re_full = re.compile(full)
        re_line = re.compile(line)
        reply.append((re_full, re_line, p))
    return reply

def align_fields(data, re_blobs, re_lines, align):
    for blob in re_blobs.findall(data):
        if blob.count("\n") < 2:
            continue
        #print(blob)
        pad_a = 0
        pad_b = 0
        lines = re_lines.findall(blob)
        num_c = 0
        for a, b, c in lines:
            pad_a = max(pad_a, len(a))
            if len(c) > 0:
                pad_b = max(pad_b, len(b))
                num_c += 1
        if num_c < 2:
            pad_b = 0
        if align:
            pad_a = (pad_a + align - 1) & -align
        dst = ""
        for a, b, c in lines:
            dst += a.ljust(pad_a) + b.ljust(pad_b) + c + "\n"
        data = data.replace(blob, dst)
    return data

def apply_patterns(data, patterns):
    for p, r in patterns:
        dst = ""
        while dst != data:
            dst = p.sub(r, data)
            data, dst = dst, data
    return data

def process(data, pre_patterns, re_fields, post_patterns):
    if not len(data):
        return data
    if data[-1] != '\n':
        data += '\n'
    data = apply_patterns(data, pre_patterns)
    for blobs, lines, align in re_fields:
        data = align_fields(data, blobs, lines, align)
    data = apply_patterns(data, post_patterns)
    return data

def main():
    pre_patterns = [
        # break line after lambda
        (r"(\n *)(.+\[[^]]*\](?: *\([^)]*\))?(?: -> .+)?) {\n", r"\1\2\1{\n"),
        # break line after union
        (r"(\n *)union {\n", r"\1union\1{\n"),
    ]
    fields = [
        # align case ...: return ...;
        (4, r" +case .+:", " +", r".+;"),
        # align member ... : bitfield;
        (4, r" +.+[^ ]", " +", r": \d+;"),
        # align #define MACRO(...) ...
        (4, r"# *define [a-zA-Z_][a-zA-Z_0-9_]*(?:\([^)]+\))?", " +", r".+?"),
        # align members (or try to...)
        (0, r" *[a-zA-Z_][a-zA-Z0-9_:<>*&, ]*", " +", r"[a-zA-Z_][a-zA-Z0-9_[\]]*(?:\[\d+\])?;"),
        # align ... = ...
        (0, r" *\b(?:using |(?:const )?auto[&*]? )?[^\n ]+", " +", r"= .+?"),
        # align method names
        (4, r" *\b[^\n=]*?[^\n=, ]", " +", r"(?:\b\w+|\(\*\w+\)) *\([^\n=]*\)(?: *const)?(?: override| = 0)?;"),
        # align method parameters
        (4, r" *\b[^\n=]*?[^\n=, ] +(?:\b\w+|\(\*\w+\))", " *", r"\([^\n=]*\)(?: *const)?(?: override| = 0)?;"),
    ]
    post_patterns = [
        # align constructor with destructor
        (r"\n( +)(\w+)(\(.*\));\n\1~\2\(\);\n", r"\n\1 \2\3;\n\1~\2();\n"),
        # put forward declarations on one line
        (r"\n(namespace \w+)\n{\n +((?:\w+) \w+;)\n}\n", r"\n\1 { \2 }\n"),
        # remove empty namespaces close comments
        (r"(\n *}) // namespace\n", r"\1\n"),
        # align back ...] =\n{\n
        (r"(\n *)(.+?)\] =\n +{", r"\1\2] =\1{"),
        # remove struct ...; alignment
        (r"struct +([a-zA-Z_][a-zA-Z0-9_]+;)", r"struct \1"),
        # remove whitespaces after return
        (r"return +", r"return "),
        # remove trailing whitespace
        (r"[ \t\r]+\n", r"\n"),
    ]
    for i, (p, t) in enumerate(pre_patterns):
        pre_patterns[i] = re.compile(p), t
    for i, (p, t) in enumerate(post_patterns):
        post_patterns[i] = re.compile(p), t
    re_fields = compile_fields(fields)

    total = 0
    def round_time(arg):
        return int(round(arg * 1000))

    # store last modified times
    files = sys.argv[2:]
    targets = []
    for filename in files:
        data = read_file(filename)
        st = os.stat(filename)
        atime = st.st_atime
        mtime = st.st_mtime
        targets.append((filename, data, (atime, mtime)))

    # call clang on all files which *WILL* modify them
    clang = os.path.abspath(sys.argv[1])
    subprocess.check_output([clang, "-i", "-style=file"] + files)

    for filename, data, modified in targets:
        t = timeit.default_timer()
        after_clang = read_file(filename)
        value = process(after_clang, pre_patterns, re_fields, post_patterns)
        step = timeit.default_timer() - t
        #print("%4dms: %s" % (round_time(step), f))
        total += step
        write_file(filename, value)
        # if content is the same, we reset modified time
        # so that build systems do not rebuild them
        if value == data:
            os.utime(filename, modified)
        else:
            print("fmt: %s" % os.path.basename(filename))
    print("fmt: %d files %dms" % (len(files), round_time(total)))

if __name__ == "__main__":
    main()
    sys.exit(0)
