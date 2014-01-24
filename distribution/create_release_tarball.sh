#!/bin/sh

# get the name from the git tag:
name=$(git describe --tags)
# go into the root of the repository
cd $(git rev-parse --show-toplevel)

# create the tarball
filename=flare-engine-${name}.tar.gz
git archive --format=tar.gz --prefix=flare-engine-${name}/ HEAD > ${filename}

echo "Released to $(git rev-parse --show-toplevel)/${filename}"
