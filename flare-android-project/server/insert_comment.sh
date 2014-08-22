#!/bin/bash

# run this from outside of mods folder

for i in $( find  mods -type f -name '*.txt'); do
	sed -i '1s/^/## android comment to workaround getline() issue\n/' $i
done
