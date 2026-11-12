#!/bin/sh
# tags HEAD with the time, e.g. '2026-11-13--09-30', and pushes the tag
# usage: scripts/tag-and-push.sh [suffix]
set -eu
cd "$(dirname "$0")"

cd ..

TAG=$(date "+%Y-%m-%d--%H-%M")
if [ -n "${1:-}" ]; then
    TAG="${TAG}--$1"
fi

echo "creating tag: $TAG"

git tag "$TAG"
git push origin "$TAG"
