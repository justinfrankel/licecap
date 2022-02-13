#!/bin/sh

DIR="$(dirname $0)"
if [ "v$DIR" = "v" ]; then DIR=. ; fi

if [ -x /usr/bin/php ]; then EXE=/usr/bin/php
elif [ -x /opt/homebrew/bin/php ]; then EXE=/opt/homebrew/bin/php
else EXE=php ; fi

$EXE "$DIR/swell_resgen.php" $*
