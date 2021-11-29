#!/bin/bash
IFS=$'\n'
HAVE_CLASS=0
echo "<?xml version=\"1.0\"?>"
echo "<classes>"
for line in `grep '@CLASS\|@ATTR\|@TYPE' src/*.cpp | sed s/^.*@//g | sed 's/CLASS /CLASS|/g' | sed 's/ATTR /ATTR|/g' | sed 's/TYPE /TYPE|/g' | sed 's/"/\&quot;/g'`
do
    IFS='|'
    fields=($line)
    # Handle CLASS
    if [ "${fields[0]}" == "CLASS" ]; then
	if [ $HAVE_CLASS -eq 1 ]; then
	    echo '	  </attributes>'
	    echo '	</class>'
	fi
	echo '	<class name="'${fields[1]}'">'
	HAVE_CLASS=1
	echo '	  <description>'${fields[2]}'</description>'
	echo '	  <attributes>'
    fi
    # Handle ATTR
    if [ "${fields[0]}" == "ATTR" ]; then
	IFS=':'
	TYPE_VAL=(${fields[2]})
	IFS=''
	echo '		  <attribute name="'${fields[1]}'" type="'$(echo "${TYPE_VAL[0]}" | xargs)'" vars="'$(echo "${TYPE_VAL[1]}" | xargs)'">'${fields[3]}'</attribute>'
    fi
    # Handle TYPE
    if [ "${fields[0]}" == "TYPE" ]; then
	echo '		<type name="'${fields[1]}'">'${fields[2]}'</type>'
    fi
done

# close last class if available
if [ $HAVE_CLASS -eq 1 ]; then
    echo '	  </attributes>'
    echo '	</class>'
fi
echo "</classes>"
