#!/bin/bash

HTMLFILE="../attribute-reference.html"
PAGETITLE="Attribute Reference"

sed -e "s/PAGETITLE/$PAGETITLE/g" header.txt > "$HTMLFILE"
markdown -f -smarty "attribute-reference.md" >> "$HTMLFILE"
cd ../../
./extract_xml.sh | xsltproc wiki.xslt - | sed -e '1,3d' | markdown -f -smarty >> "docs/gen/$HTMLFILE"


cd "docs/gen"
cat footer.txt >> "$HTMLFILE"

