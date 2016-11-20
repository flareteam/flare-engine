#!/bin/sh

SOURCE=./docs/gen/attribute-reference.md
DESTINATION=./Attribute-Reference.md

cp ${SOURCE} ${DESTINATION}
./extract_xml.sh | xsltproc wiki.xslt - | sed -e '1,3d' >> ${DESTINATION}

echo "Created ${DESTINATION}"
echo "Now move it to the wiki."
