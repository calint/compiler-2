#!/bin/sh

set -eu
cd "$(dirname "$0")"

# set environment
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

#CC="g++ -std=c++23"
#CW="-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow"
CF="-Wfatal-errors -Werror"
DBG=-g
OPT=-O0

#
# AddressSanitizer + UndefinedBehaviorSanitizer (can be combined)
# Best for catching string_view lifetime issues and memory bugs
ASAN_FLAGS="-fsanitize=address,undefined,leak,nullability,bounds,local-bounds,float-divide-by-zero \
  -fsanitize-address-use-after-scope \
  -fsanitize-address-use-after-return=always \
  -fstack-protector-strong -fno-omit-frame-pointer -fno-optimize-sibling-calls \
  -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_LIBCPP_ENABLE_ASSERTIONS=1"
# note: -fsanitize=integer gives error on common while(i--) type of use

# MemorySanitizer (cannot be combined with ASAN - use separately)
# Detects uninitialized memory reads - requires rebuilding stdlib
MSAN_FLAGS="-fsanitize=memory \
  -fsanitize-memory-track-origins=2 \
  -fno-omit-frame-pointer -fno-optimize-sibling-calls"

# ThreadSanitizer (for multi-threading bugs - cannot combine with ASAN/MSAN)
TSAN_FLAGS="-fsanitize=thread \
  -fno-omit-frame-pointer"

PROF_FLAGS="-fprofile-instr-generate -fcoverage-mapping"

# Parse parameters
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
    *)
        echo "unknown parameter: $arg"
        echo "usage: $0 [asan|msan|tsan] [prof] [-O3] [build]"
        echo ""
        echo "sanitizers (choose one option):"
        echo "  asan - AddressSanitizer + UndefinedBehaviorSanitizer"
        echo "  msan - uninitialized memory, requires special build"
        echo "  tsan - data races, for multi-threaded code"
        echo ""
        echo "options:"
        echo "  prof  - enable profiling/coverage"
        echo "  -O3   - optimize for faster execution"
        echo "  build - build only, don't run"
        exit 1
        ;;
    esac
done

# Check for incompatible sanitizer combinations
SANITIZER_COUNT=0
[ -n "$ASAN" ] && SANITIZER_COUNT=$((SANITIZER_COUNT + 1))
[ -n "$MSAN" ] && SANITIZER_COUNT=$((SANITIZER_COUNT + 1))
[ -n "$TSAN" ] && SANITIZER_COUNT=$((SANITIZER_COUNT + 1))

if [ $SANITIZER_COUNT -gt 1 ]; then
    echo "Error: Cannot combine multiple sanitizers (asan/msan/tsan)"
    echo "Use only one at a time"
    exit 1
fi

SANITIZERS="$ASAN$MSAN$TSAN"

CMD="$CC src/main.cpp -o baz $DBG $OPT $RTTI $CF $CW $SANITIZERS $PROF"
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
