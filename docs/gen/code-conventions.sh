#!/bin/bash

HTMLFILE="../code-conventions.html"
PAGETITLE="Code Conventions"
WIKIPATH="../../../flare-engine.wiki"

if [ -d "$WIKIPATH" ]; then
	sed -e "s/PAGETITLE/$PAGETITLE/g" header.txt > "$HTMLFILE"
	markdown "$WIKIPATH/Code-Conventions.md" >> "$HTMLFILE"
	cat footer.txt >> "$HTMLFILE"
fi

