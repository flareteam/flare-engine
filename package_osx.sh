#!/usr/bin/env bash -e

FLARE_EXE=$1

if [ -z "$FLARE_EXE" ]; then
  echo "usage: $0 <path to flare executable>"
  exit 1
fi
if [ ! -f ${FLARE_EXE} ]; then
  echo "no Flare executable found at $FLARE_EXE. Please follow README in order to build Flare engine first"
  exit 1
fi
if [ `otool -L ${FLARE_EXE} | egrep libSDL2 | wc -l` -lt 1 ]; then
  echo "invalid Flare executable"
  exit 1
fi

DST=/tmp/___flare.build
rm -fr ${DST} && mkdir -p ${DST}

cp -r RELEASE_NOTES.txt \
  README.engine.md \
  CREDITS.engine.txt \
  COPYING \
  ${FLARE_EXE} \
  mods ${DST}

#feel free to build dependencies by yourself btw
wget 'http://files.ruads.org/flare_osx_dependencies.tar.gz' -P ${DST}
tar -zxf ${DST}/flare_osx_dependencies.tar.gz -C ${DST}
rm -f ${DST}/flare_osx_dependencies.tar.gz 

echo '#!/bin/sh' >> ${DST}/start.sh
echo 'cd "$(dirname "${BASH_SOURCE[0]}")"' >> ${DST}/start.sh
echo 'DYLD_LIBRARY_PATH=./lib ./flare'  >> ${DST}/start.sh
chmod +x ${DST}/start.sh

echo "packaging"
tar -zcf flare_osx.tar.gz -C ${DST} .
rm -fr ${DST}
echo "done"
