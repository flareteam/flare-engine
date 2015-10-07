#!/bin/bash

HTMLFILE="../translations.html"
PAGETITLE="Translations"
WIKIPATH="../../../flare-engine.wiki"

if [ -d "$WIKIPATH" ]; then
	sed -e "s/PAGETITLE/$PAGETITLE/g" header.txt > "$HTMLFILE"
	markdown "$WIKIPATH/Translations.md" >> "$HTMLFILE"
	cat footer.txt >> "$HTMLFILE"
fi

