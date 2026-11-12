#!/bin/sh
# builds the compiler 'baz' from 'src/main.cpp' and writes the flags for
# clangd to 'compile_commands.json'; without 'build' it then compiles and runs
# 'prog.baz' with 'run.sh'
# usage: make.sh [asan|msan|tsan] [prof] [-O3] [build], see 'usage'
set -eu
cd "$(dirname "$0")"

CC="clang++ -std=c++26"
CW="-Weverything \
    -Wno-c++98-compat \
    -Wno-c++98-compat-pedantic \
    -Wno-pre-c++20-compat-pedantic \
    -Wno-weak-vtables -Wno-padded \
    -Wno-braced-scalar-init \
    -Wno-lifetime-safety-intra-tu-suggestions \
    -Wno-lifetime-safety-cross-tu-suggestions \
    -Wno-lifetime-safety-invalidation \
"

CF="-Wfatal-errors -Werror"
DBG=-g
OPT=-O0

# address and undefined behavior sanitizers, e.g. for the lifetime of a
# 'string_view'
ASAN_FLAGS="-fsanitize=address,undefined,leak,nullability,bounds,local-bounds,float-divide-by-zero \
  -fsanitize-address-use-after-scope \
  -fsanitize-address-use-after-return=always \
  -fstack-protector-strong -fno-omit-frame-pointer -fno-optimize-sibling-calls \
  -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_LIBCPP_ENABLE_ASSERTIONS=1"
# note: -fsanitize=integer gives error on common while(i--) type of use

# reads of uninitialized memory, not with asan, needs a libc++ built with it
MSAN_FLAGS="-fsanitize=memory \
  -fsanitize-memory-track-origins=2 \
  -fno-omit-frame-pointer -fno-optimize-sibling-calls"

# data races, not with asan or msan
TSAN_FLAGS="-fsanitize=thread \
  -fno-omit-frame-pointer"

PROF_FLAGS="-fprofile-instr-generate -fcoverage-mapping"

usage() {
    echo "usage: $0 [asan|msan|tsan] [prof] [-O3] [build]"
    echo ""
    echo "sanitizers (choose one option):"
    echo "  asan - AddressSanitizer + UndefinedBehaviorSanitizer"
    echo "  msan - uninitialized memory, needs a libc++ built with msan and"
    echo "         '-stdlib=libc++' (with its paths) added to CC, else false reports"
    echo "  tsan - data races, for multi-threaded code"
    echo ""
    echo "options:"
    echo "  prof  - enable profiling/coverage"
    echo "  -O3   - optimize for faster execution"
    echo "  build - build only, don't run"
}

ASAN=
MSAN=
TSAN=
PROF=
BUILD_ONLY=0
# asan keeps rtti because its vptr check needs it
RTTI=-fno-rtti

for arg in "$@"; do
    case "$arg" in
    asan)
        ASAN="$ASAN_FLAGS"
        RTTI=
        ;;
    msan)
        MSAN="$MSAN_FLAGS"
        ;;
    tsan)
        TSAN="$TSAN_FLAGS"
        ;;
    prof)
        PROF="$PROF_FLAGS"
        ;;
    build)
        BUILD_ONLY=1
        ;;
    -O3)
        OPT=-O3
        ;;
    -h | --help)
        usage
        exit 0
        ;;
    *)
        echo "unknown parameter: $arg" >&2
        usage >&2
        exit 1
        ;;
    esac
done

# at most one sanitizer
SANITIZER_COUNT=0
[ -n "$ASAN" ] && SANITIZER_COUNT=$((SANITIZER_COUNT + 1))
[ -n "$MSAN" ] && SANITIZER_COUNT=$((SANITIZER_COUNT + 1))
[ -n "$TSAN" ] && SANITIZER_COUNT=$((SANITIZER_COUNT + 1))

if [ $SANITIZER_COUNT -gt 1 ]; then
    echo "Error: Cannot combine multiple sanitizers (asan/msan/tsan)" >&2
    echo "Use only one at a time" >&2
    exit 1
fi

SANITIZERS="$ASAN$MSAN$TSAN"

# the version '--version' prints, the tag and commit of the build
# note: a copy without '.git' inside another repository is not described by it
VERSION=unknown
if [ -e .git ]; then
    VERSION="$(git describe --tags --always --dirty 2>/dev/null || echo unknown)"
fi
VERSION_DEF="-DBAZ_VERSION=\"$VERSION\""

CMD="$CC src/main.cpp -o baz $DBG $OPT $RTTI $CF $CW $SANITIZERS $PROF $VERSION_DEF"
echo "$CMD"
$CMD

# the flags of the plain build for clangd, so every build writes the same
# file; the unquoted words collapse the spaces
EDITOR_FLAGS=$(echo $CC -fno-rtti $CF $CW)
printf '[\n  {\n    "directory": "%s",\n    "command": "%s src/main.cpp",\n    "file": "src/main.cpp"\n  }\n]\n' \
    "$PWD" "$EDITOR_FLAGS" >compile_commands.json

if [ $BUILD_ONLY -eq 1 ]; then
    exit 0
fi

./run.sh prog.baz
