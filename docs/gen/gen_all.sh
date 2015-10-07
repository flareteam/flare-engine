#!/bin/bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "$DIR"

. ./attribute-reference.sh

# copy pages from wiki
# these scripts assume you have a "flare-engine.wiki" directory next to your
# "flare-engine" directory
. ./translations.sh
. ./code-conventions.sh

