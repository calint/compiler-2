#!/bin/sh
set -e
cd $(dirname "$0")

cd ..
TAG=$(date "+%Y-%m-%d--%H-%M") && git tag $TAG && git push origin $TAG
