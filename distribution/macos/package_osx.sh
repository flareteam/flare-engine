#!/usr/bin/env bash -e

cd "`dirname "$0"`"
cd ../../

FLARE_EXE=$1
FLARE_DEPS_SRC="http"
FLARE_GAME=""

if [ -z "${FLARE_EXE}" ]; then
  echo "usage: $0 <path to flare executable>"
  exit 1
fi
if [ ! -f ${FLARE_EXE} ]; then
  echo "no Flare executable found at ${FLARE_EXE}. Please follow README in order to build Flare engine first"
  exit 1
fi
if [ `otool -L ${FLARE_EXE} | egrep libSDL2 | wc -l` -lt 1 ]; then
  echo "invalid Flare executable"
  exit 1
fi

if [ "$2" != "" ]; then
 FLARE_DEPS_SRC=$2
fi
if [ ${FLARE_DEPS_SRC} == "http" ]; then
  echo "download dependencies from website"
elif [ ${FLARE_DEPS_SRC} == "homebrew" ]; then
  echo "copy dependencies from homebrew"
else
  echo "usage: $0 <path to flare executable> <http|homebrew>"
  exit 1
fi

if [ "$3" != "" ]; then
 FLARE_GAME=$3
fi

DST=/tmp/___flare.build
rm -fr ${DST} && mkdir -p ${DST}

cp -r RELEASE_NOTES.txt \
  README.engine.md \
  CREDITS.engine.txt \
  COPYING \
  ${FLARE_EXE} \
  mods ${DST}

if [ ${FLARE_DEPS_SRC} == "http" ]; then
  #feel free to build dependencies by yourself btw
  wget 'http://files.ruads.org/flare_osx_dependencies.tar.gz' -P ${DST}
  tar -zxf ${DST}/flare_osx_dependencies.tar.gz -C ${DST}
  rm -f ${DST}/flare_osx_dependencies.tar.gz
elif [ ${FLARE_DEPS_SRC} == "homebrew" ]; then
  LIB=${DST}/lib
  mkdir ${LIB}
  # SDL2
  cp /usr/local/opt/sdl2/COPYING.txt ${LIB}/SDL2-COPYING.txt
  cp /usr/local/opt/sdl2/README.txt ${LIB}/SDL2-README.txt
  cp /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib ${LIB}
  cp /usr/local/opt/sdl2_image/lib/libSDL2_image-2.0.0.dylib ${LIB}
  cp /usr/local/opt/sdl2_mixer/lib/libSDL2_mixer-2.0.0.dylib ${LIB}
  cp /usr/local/opt/sdl2_ttf/lib/libSDL2_ttf-2.0.0.dylib ${LIB}
  # VORBIS
  cp /usr/local/opt/libvorbis/COPYING ${LIB}/VORBIS-COPYING
  cp /usr/local/opt/libvorbis/lib/libvorbis.0.dylib ${LIB}
  cp /usr/local/opt/libvorbis/lib/libvorbisenc.2.dylib ${LIB}
  cp /usr/local/opt/libvorbis/lib/libvorbisfile.3.dylib ${LIB}
  # OGG
  cp /usr/local/opt/libogg/COPYING ${LIB}/OGG-COPYING
  cp /usr/local/opt/libogg/lib/libogg.0.dylib ${LIB}
  # PNG
  cp /usr/local/opt/libpng/lib/libpng16.16.dylib ${LIB}

  # Verify all homebrew deps using using otool
  for DYLIB in ${LIB}/*.dylib; do
    #echo "dylib: ${DYLIB}"
    for LINE in $(otool -L ${DYLIB}); do
      #echo "line: ${LINE}"
      if [[ $LINE == *"/usr/local/opt/"* ]]; then
	NEED=$(basename ${LINE})
	#echo "${DYLIB} need: ${NEED}"
	FILE="${LIB}/${NEED}"
	if [ ! -f "${FILE}" ]; then
          echo "${NEED} not found, copying"
	  cp ${LINE} ${LIB}
        fi
      fi
    done
  done

else
  echo "'${FLARE_DEPS_SRC}' unknown dependency source"
  exit 1
fi

if [ "${FLARE_GAME}" != "" ]; then
  FLARE_ENGINE_MOD_LIST=mods/mods.txt
  FLARE_GAME_MOD=${FLARE_GAME}/mods
  while IFS= read -r MOD; do
    MOD_DIR=${FLARE_GAME_MOD}/${MOD}
    if [ "${MOD}" != "" ] && [ -d "${MOD_DIR}" ]; then
      cp -r ${MOD_DIR} ${DST}/mods
      echo "copied ${MOD}"
    fi
  done < "${FLARE_ENGINE_MOD_LIST}"
  cp ${FLARE_GAME}/CREDITS.txt ${DST}
  cp ${FLARE_GAME}/LICENSE.txt ${DST}
fi

echo '#!/bin/sh' >> ${DST}/start.sh
echo 'cd "$(dirname "${BASH_SOURCE[0]}")"' >> ${DST}/start.sh
echo 'DYLD_LIBRARY_PATH=./lib ./flare'  >> ${DST}/start.sh
chmod +x ${DST}/start.sh

echo "packaging"
tar -zcf flare_osx.tar.gz -C ${DST} .
rm -fr ${DST}
echo "done"
