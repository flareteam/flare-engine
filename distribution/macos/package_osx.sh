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
  # Homebrew lives in /opt/homebrew on Apple Silicon and /usr/local on Intel
  BREW_PREFIX=$(brew --prefix)
  LIB=${DST}/lib
  mkdir ${LIB}

  # Recursively copy all Homebrew libraries needed by the given binary
  copy_brew_deps() {
    local BIN_DIR=$(dirname "$1")
    local DEP SRC NAME
    for DEP in $(otool -L "$1" | tail -n +2 | awk '{print $1}'); do
      case ${DEP} in
        ${BREW_PREFIX}/*) SRC=${DEP} ;;
        # Homebrew sets up @rpath/@loader_path relative to the library itself
        @rpath/*|@loader_path/*) SRC=${BIN_DIR}/${DEP#*/} ;;
        *) continue ;;
      esac
      NAME=$(basename "${DEP}")
      if [ ! -f "${LIB}/${NAME}" ]; then
        echo "copying ${NAME}"
        cp "${SRC}" "${LIB}/${NAME}"
        chmod u+w "${LIB}/${NAME}"
        copy_brew_deps "${SRC}"
      fi
    done
  }

  # Licenses
  cp $(brew --prefix sdl2)/LICENSE.txt ${LIB}/SDL2-LICENSE.txt
  cp $(brew --prefix sdl2)/README.md ${LIB}/SDL2-README.md
  cp $(brew --prefix libvorbis)/COPYING ${LIB}/VORBIS-COPYING
  cp $(brew --prefix libogg)/COPYING ${LIB}/OGG-COPYING

  # Homebrew's sdl2 is now sdl2-compat, which loads SDL3 at runtime with dlopen()
  SDL3_LIB=$(brew --prefix sdl3 2>/dev/null)/lib/libSDL3.0.dylib
  if [ -f "${SDL3_LIB}" ]; then
    cp ${SDL3_LIB} ${LIB}
    ln -s libSDL3.0.dylib ${LIB}/libSDL3.dylib
    copy_brew_deps ${SDL3_LIB}
  fi

  copy_brew_deps ${FLARE_EXE}

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
