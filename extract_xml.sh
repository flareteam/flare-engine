#!/bin/bash
IFS=$'\n'
HAVE_CLASS=0
echo "<?xml version=\"1.0\"?>"
echo "<classes>"
for line in `grep '@CLASS\|@ATTR' src/*.cpp | sed s/^.*@//g | sed 's/CLASS /CLASS|/g' | sed 's/ATTR /ATTR|/g'`
do
    IFS='|'
    fields=($line)
    # Handle CLASS 
    if [ "${fields[0]}" == "CLASS" ]; then
	if [ $HAVE_CLASS -eq 1 ]; then 
	    echo '  </attributes>'
	    echo '</class>' 
	fi
	echo '<class name="'${fields[1]}'">'
	HAVE_CLASS=1
	echo '  <description>'${fields[2]}'</description>'
	echo '  <attributes>'
    fi
    # Handle ATTR
    if [ "${fields[0]}" == "ATTR" ]; then
	echo '    <attribute name="'${fields[1]}'" type="'${fields[2]}'">'${fields[3]}'</attribute>'
    fi
done

# close last class if available
if [ $HAVE_CLASS -eq 1 ]; then
    echo '</attributes>'
    echo '</class>'
fi
echo "</classes>"
