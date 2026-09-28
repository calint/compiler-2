# the syntax tree of 'src/main.cpp' parsed by libclang, shared by the lint
# scripts, which run from the repository root

import pathlib
import subprocess
import sys

import clang.cindex as ci

MAIN = "src/main.cpp"
ARGS = ["-x", "c++", "-std=c++26"]


def resource_args():
    # libclang does not find the compiler's own headers e.g. 'stddef.h'
    resource_dir = subprocess.run(
        ["clang", "-print-resource-dir"],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()

    return [f"-resource-dir={resource_dir}"]


def parse(unsaved_files=None):
    tu = ci.Index.create().parse(
        MAIN, args=ARGS + resource_args(), unsaved_files=unsaved_files
    )
    errors = [d for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
    if errors and unsaved_files is None:
        for d in errors:
            print(d, file=sys.stderr)
        sys.exit(1)

    return tu


def is_in(cursor, root):
    if cursor.location.file is None:
        return False

    return pathlib.Path(cursor.location.file.name).resolve().is_relative_to(root)
