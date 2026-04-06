#!/bin/bash

cd "`dirname "$0"`"
cd ../../

FLARE_GAME_PATH="../flare-game"

if [ ! -d "$FLARE_GAME_PATH" ]; then
    FLARE_GAME_PATH=""
fi

if [ "$1" ]; then
    FLARE_GAME_PATH="$1"
else
    FLARE_GAME_PATH=""
fi

if [ -z "$FLARE_GAME_PATH" ]; then
    echo "Usage: $0 <path to flare-game>"
    echo "No path to game data provided, packaging engine alone."

	cp build-steamrt/flare flare

	tar cvf "flare-engine-linux-steam-$(git describe --tags).tar.gz" \
		flare \
		mods/default \
		mods/mods.txt \
		COPYING \
		README.engine.md \
		CREDITS.engine.txt \
		RELEASE_NOTES.txt

	rm flare
else
	cp -r "$FLARE_GAME_PATH"/mods/fantasycore mods/fantasycore
	cp -r "$FLARE_GAME_PATH"/mods/empyrean_campaign mods/empyrean_campaign
	cp -r "$FLARE_GAME_PATH"/mods/centered_statbars mods/centered_statbars

	cp "$FLARE_GAME_PATH"/CREDITS.txt CREDITS.txt
	cp "$FLARE_GAME_PATH"/README README.game.md
	cp "$FLARE_GAME_PATH"/LICENSE.txt LICENSE.txt

	cp build-steamrt/flare flare

	pushd "$FLARE_GAME_PATH"
	FLARE_VERSION=$(git describe --tags)
	popd

	tar cvf "flare-linux-steam-$FLARE_VERSION.tar.gz" \
		flare \
		mods/default \
		mods/fantasycore \
		mods/empyrean_campaign \
		mods/centered_statbars \
		mods/mods.txt \
		COPYING \
		README.engine.md \
		CREDITS.engine.txt \
		RELEASE_NOTES.txt \
		CREDITS.txt \
		LICENSE.txt \
		README.game.md

	rm -rf mods/fantasycore
	rm -rf mods/empyrean_campaign
	rm -rf mods/centered_statbars
	rm CREDITS.txt
	rm README.game.md
	rm LICENSE.txt
	rm flare
fi
