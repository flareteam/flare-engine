#!/bin/bash

rm -rf emscripten/*
mkdir -p emscripten

cp -r ../flare-game/mods/fantasycore mods/fantasycore
cp -r ../flare-game/mods/empyrean_campaign mods/empyrean_campaign

cmake .

emcc \
	-v \
    -Isrc/ \
    src/*.cpp \
    -O3 \
    -s ASSERTIONS=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s USE_SDL=2 \
    -s USE_SDL_IMAGE=2 \
    -s SDL2_IMAGE_FORMATS='["png"]' \
	-s USE_SDL_TTF=2 \
	-s USE_SDL_MIXER=2 \
	-s "EXTRA_EXPORTED_RUNTIME_METHODS=['print']" \
	-lidbfs.js \
    --preload-file mods \
    -o emscripten/index.html

rm -rf mods/fantasycore
rm -rf mods/empyrean_campaign

cp distribution/emscripten_template.html emscripten/index.html
