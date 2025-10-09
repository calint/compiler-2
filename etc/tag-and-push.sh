#!/bin/sh
set -e
cd "$(dirname "$0")"

cd ..
TAG=$(date "+%Y-%m-%d--%H-%M")
if [ -n "$1" ]; then
  TAG="${TAG}--$1"
fi

echo "Creating tag: $TAG"

git tag "$TAG"
git push origin "$TAG"
