#!/bin/sh
# parses the '.baz' files of the repository with the grammar and fails on
# 'ERROR' and 'MISSING' nodes, which the compiler may accept while the editor
# loses the types around them
# error tests are skipped: the first line of their '.out' that is not empty
# starts with 'NAME.baz:line:column: '
# usage: parse-sources.sh
set -eu
cd "$(dirname "$0")/.."

ROOT=../../..

# valid programs that the grammar does not parse on purpose
# 0851.baz: bodies of generics that are never instantiated are not checked
SKIP="qa/coverage/tests/0851.baz"

LIST="$(mktemp)"
LOG="$(mktemp)"
trap 'rm -f "$LIST" "$LOG"' EXIT

git -C "$ROOT" ls-files --cached --others --exclude-standard '*.baz' | while read -r file; do
    case " $SKIP " in
    *" $file "*)
        continue
        ;;
    esac

    out="$ROOT/${file%.baz}.out"
    name="$(basename "$file")"
    if [ -f "$out" ] && grep -a -m 1 . "$out" | grep -qE "^$name:[0-9]+:[0-9]+: "; then
        continue
    fi

    echo "$ROOT/$file"
done >"$LIST"

if ! xargs tree-sitter parse --quiet <"$LIST" >"$LOG" 2>&1; then
    sed "s|^$ROOT/||" "$LOG"
    echo "parse sources: grammar errors in the files above"
    exit 1
fi

echo "parse sources: $(wc -l <"$LIST") files ok"
