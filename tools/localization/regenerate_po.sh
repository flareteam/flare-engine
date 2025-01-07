#!/bin/bash
# Run this script in the language directory to update the pot and all *.po files

SCRIPT_DIR="$(dirname $(realpath $0))"
DO_MERGE=0
LANG_DIR=""

print_usage() {
    echo "$0 [options] -l [directory]"
    echo "options:"
    echo "-h        show help"
	echo "-m        merge pot files into po files"
    echo "-l        language directory"
}

while getopts 'hml:' flag; do
    case "${flag}" in
        h)
            print_usage
            exit 0
            ;;
        m) DO_MERGE=1 ;;
        l) LANG_DIR="${OPTARG}" ;;
        *)
            print_usage
            exit 1
            ;;
    esac
done

if [ ! -d "$LANG_DIR" ]; then
	print_usage
	echo "Please specify a language directory with -l"
	exit 1
fi

cd "$LANG_DIR"

# For the engine
# To generate the appropriate .pot file, you need to run the following command from the languages directory:
if [ -e engine.pot ] ; then
	echo "Generating engine.pot"

	xgettext --keyword=get --keyword=getv -o engine.pot ../../../src/*.cpp

	# xgettext doesn't allow defining a charset, but we want UTF-8 across the board
	sed -i "s/charset=CHARSET/charset=UTF-8/" engine.pot

	if [ $DO_MERGE == 1 ]; then
	# To update existing .po files, you need to run the following command from the languages directory:
	# msgmerge -U -N <name_of_.po_file> <name_of_.pot_file>

		for f in $(ls engine.*.po) ; do
		echo "Processing $f"
		msgmerge -U -N --backup=none $f engine.pot
	done
	fi
fi

if [ -e data.pot ] ; then
	echo "Generating data.pot"

	# For mods:
	"$SCRIPT_DIR/xgettext.py"

	if [ $DO_MERGE == 1 ]; then
		for f in $(ls data.*.po) ; do
			echo "Processing $f"
			msgmerge -U -N --backup=none $f data.pot
		done
	fi
fi
